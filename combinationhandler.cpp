// CombinationHandler.cpp : Defines the entry point for the DLL application.

#include "stdafx.h"
#include <iostream.h>
#include <stdio.h>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <odbcss.h>
#include <srv.h>
#include "cCombH.h"
#define  BUF_LEN 1024

HENV		g_henv		= SQL_NULL_HENV;
HDBC		g_hdbc		= SQL_NULL_HDBC;

void		PrintUsage(SRV_PROC*);
void		PrintError(SRV_PROC*,char*);

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
   
	
	UCHAR szConn[] = "Provider=MSDASQL;DRIVER={SQL SERVER};SERVER=DEV_SERVER;Database=ARM";
	//UCHAR szConn[] = "Provider=MSDASQL;DRIVER={SQL SERVER};SERVER=TERROR;Database=ARM";
	short nOutputLen	=0;
	UCHAR szOutput[BUF_LEN];
	BOOL rcXP = false;
	SQLRETURN	sret;

   switch(ul_reason_for_call)
   {
		case DLL_PROCESS_ATTACH:
			//Allocate an Environment Handle
			if (SQL_SUCCESS != (sret =  SQLAllocHandle(SQL_HANDLE_ENV,NULL,&g_henv)    ))
			{ goto SAFE_EXIT;}
			
			//Set the ODBC version Environment Attribute
			if(SQL_SUCCESS != (sret = SQLSetEnvAttr(g_henv,SQL_ATTR_ODBC_VERSION,(SQLPOINTER)SQL_OV_ODBC3,SQL_IS_INTEGER)))
			{ goto SAFE_EXIT;}
			

			//allocate a connection handle
			if(SQL_SUCCESS != (sret = SQLAllocHandle(SQL_HANDLE_DBC,g_henv,&g_hdbc)))
			{goto SAFE_EXIT;}
			

			//use cursor library to obtain support for scrollable cursors
			//per documentation, this call needs to precede SQLDriverConnect
			if(!(SQL_SUCCESS == (sret = 		
				SQLSetConnectAttr(g_hdbc, SQL_ATTR_ODBC_CURSORS,(SQLPOINTER)SQL_CUR_USE_ODBC,SQL_IS_INTEGER))))
			{   goto SAFE_EXIT;}

			 //connect to the data base
			sret = SQLDriverConnect(g_hdbc,0,szConn,sizeof(szConn),szOutput,sizeof(szOutput),&nOutputLen,SQL_DRIVER_NOPROMPT);
			if(!(SQL_SUCCESS == sret || SQL_SUCCESS_WITH_INFO == sret))
			{ goto SAFE_EXIT;}

			break;
		case DLL_PROCESS_DETACH:
			rcXP = true;
			goto SAFE_EXIT;	
			break;
		default:
			break;
	}

	return true;
SAFE_EXIT:

	if (SQL_NULL_HDBC != g_hdbc)
	{
		SQLDisconnect(g_hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC,g_hdbc);
	}
	if (SQL_NULL_HENV != g_henv)
	{
		SQLFreeEnv(g_henv);
	}

	return rcXP;

}

#define EXPECTED_PARAMS					 2
#define DEAL_PARAM						 1
#define TRACK_PARAM					     2 
#define XP_ERROR					     1
#define XP_NOERROR			   		     0
#define XP_COMBINATIONHANDLER_ERROR  60001
ULONG __GetXpVersion()
{
	return ODS_VERSION;
}

/************************************************************
*   CombinationHandler entry function.
*   Takes 2 parameters: both longs
*   Parameter1: lDealFKID: identifies the deal to run CombinationHandler On
*   Parameter2: lTrackFKID: identifies the track which should be analyzed for combinations
*   a 0 value for TrackFKID indicates look for cross-track combinations
*	
*************************************************************/
SRVRETCODE xp_CombinationHandler(SRV_PROC* pSrvProc)
	{

	BYTE				bType;
	ULONG				cbMaxLen;
	ULONG				cbActualLen;
	BOOL				fNull;
	ULONG				lLastUniqueEvent	= 0;
	SQLUINTEGER			lDealFKID			= 0;
	SQLUINTEGER			lTrackFKID			= 0;
	short				nOutputLen			= 0;
	short				FETCH_MODE			= SQL_FETCH_FIRST;

	HSTMT				hstmtTRACK			= SQL_NULL_HSTMT;
	SQLRETURN			sret;
	SRVRETCODE			rc;
	short				nNumCols			= 0;
	long				nRowCount			= 0;
	long				lNumCombs			= 0;
	LPCOMBINATION		pCOMB				= 0;

	
	CHAR szBindToken[256];
	UCHAR szCallBindSession[] =
		"{call sp_bindsession(?)}";
	

	UCHAR szCallGetNMESpaceByDealAndTrack[] =
		"{call GetNMESpaceByDealAndTrack(?,?)}";
	UCHAR szCallDeleteCombinationByDealAndTrack[] = 
		"{call DeleteCombinationByDealAndTrack(?,?)}";
	UCHAR szCallInsertCombination[] =
		"{call InsertCombination(?,?,?)}"; 

	//NME_SPACE(ordinal 1),
	//UNIQUE_EVENT(ordinal 2),
	//NME_EDGE (ordinal3 )
	//LINK_FKID (ordinal 4) 
	SQLUINTEGER lNME_SPACE			   = 0;
	SQLUINTEGER lUNIQUE_EVENT		   = 0;
	SQLUINTEGER lNME_EDGE			   = 0;
	SQLUINTEGER lLINK_FKID			   = 0;
	
	SQLINTEGER cbNME_SPACE			   = 0;
	SQLINTEGER cbUNIQUE_EVENT		   = 0;
	SQLINTEGER cbNME_EDGE			   = 0;
	SQLINTEGER cbLINK_FKID			   = 0;
		
	//Parameter validation section: Ensure we have:
    //A. two valid parameters
	//B. both of data type long
	//C. both not of return parameter type
	//D. lDealFKID must be > 0
	//E. lTrackFKID must by >= 0

	//A. Two Valid Parms
	SRVRETCODE rcXP		= XP_ERROR;  //assume failure until proven otherwise 

	if(EXPECTED_PARAMS != srv_rpcparams(pSrvProc))
	{ //send error message and return
		PrintUsage (pSrvProc);
		return rcXP;
	}
	
	//Get Parameter info for the DEAL Parameter
	if (FAIL == srv_paraminfo(pSrvProc,DEAL_PARAM,&bType,&cbMaxLen,&cbActualLen,NULL,&fNull))
	{
		PrintError(pSrvProc,"srv_paraminfo failed on call to query parameter 1");
		return rcXP;
	}
	
	//make sure DEALFKID wasn't passed as  a return parameter
	if (SRV_PARAMRETURN & srv_paramstatus(pSrvProc,DEAL_PARAM))
	{
		PrintUsage(pSrvProc);
		return rcXP;
	}

	//ensure DEALFKID passed as four bit integer
	if (!((SRVINT4 == bType) || (SRVINT2 == bType) ||(SRVINTN == bType))) 
	{
		PrintUsage(pSrvProc);
		return rcXP;
	}

	//Get Actual DEALFKID Parameter
	if (FAIL == srv_paraminfo(pSrvProc,DEAL_PARAM,&bType,&cbMaxLen,&cbActualLen,(BYTE*)&lDealFKID,&fNull))
	{
		PrintError(pSrvProc,"srv_paraminfo failed on call to retrieve parameter 1");
		return rcXP;
	}

	if(!(0 < lDealFKID))
	{
		PrintUsage(pSrvProc);
		return rcXP;
	}

	//Get Parameter info for the TRACK Parameter
	if (FAIL == srv_paraminfo(pSrvProc,TRACK_PARAM,&bType,&cbMaxLen,&cbActualLen,NULL,&fNull))
	{
		PrintError(pSrvProc,"srv_paraminfo failed on call to query parameter 2");
		return rcXP;
	}
	
	//make sure TRACKFKID wasn't passed as  a return parameter
	if (SRV_PARAMRETURN & srv_paramstatus(pSrvProc,TRACK_PARAM) )
	{
		PrintUsage(pSrvProc);
		return rcXP;
	}
	
	//ensure CYCLEFKID passed as four bit integer
	if (!((SRVINT4 == bType) || (SRVINT2 == bType) ||(SRVINTN == bType))) 
	{
		PrintUsage(pSrvProc);
		return rcXP;
	}

	//Get Actual TRACK Parameter
	if (FAIL == srv_paraminfo(pSrvProc,TRACK_PARAM,&bType,&cbMaxLen,&cbActualLen,(BYTE*)&lTrackFKID,&fNull))
	{
		PrintError(pSrvProc,"srv_paraminfo failed on call to retrieve parameter 2");
		return rcXP;
	}


	if(!(0 <= lTrackFKID))
	{
		PrintUsage(pSrvProc);
		return rcXP;
	}

	cCombH *pcAlgorithm = new cCombH();
	if (0 == pcAlgorithm)
	{
		PrintError(pSrvProc,"Failed to initialize CombHandler Algorithm");
		goto SAFE_EXIT;
	}

	//obtain a statement handle
	if( !(SQL_SUCCESS == (sret = SQLAllocHandle(SQL_HANDLE_STMT,g_hdbc,&hstmtTRACK))))
	{
	    PrintError(pSrvProc,"SQLAllocHandle:Failed to obtain statement handle");
		goto SAFE_EXIT;
	}

	//set up for scrollable, snapshot recurds 
	if( !(SQL_SUCCESS == (sret = SQLSetStmtAttr(hstmtTRACK,SQL_ATTR_CURSOR_TYPE,(SQLPOINTER)SQL_CURSOR_STATIC,SQL_IS_INTEGER))))
	
	{
		PrintError(pSrvProc,"SQLSetStmtAttr:Failed to set cursor static attribute");
		goto SAFE_EXIT;
	}
	
	if( !(SQL_SUCCESS == (sret = SQLSetStmtAttr(hstmtTRACK,SQL_ATTR_CURSOR_SCROLLABLE,(SQLPOINTER)SQL_SCROLLABLE,SQL_IS_INTEGER))))
	{
	   
		PrintError(pSrvProc,"SQLSetStmtAttr:Failed to set cursor scrollable attribute");
		goto SAFE_EXIT;
	}

	//get the client session token
	if(FAIL == (rc = srv_getbindtoken(pSrvProc,szBindToken)))
	{
		PrintError(pSrvProc,"Failed to obtain transaction bind token");
		goto SAFE_EXIT;
	}

	//bind this token as a parameter to the next call to sp_bindsession
	if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,255,0,szBindToken,256,NULL)))
	{
	    PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter");
		goto SAFE_EXIT;
	}
	//call sp_bindsession to bind our session to client's session such that we share transaction space
	sret = SQLExecDirect(hstmtTRACK,szCallBindSession,SQL_NTS);
	if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret)))
	{
	    PrintError(pSrvProc,"SQLExecDirect:Failed to establish bound transaction session");
		goto SAFE_EXIT;
	}

	//unbind the Parameter from last call to SQLBindParameter
	SQLFreeStmt(hstmtTRACK,SQL_RESET_PARAMS);
	//done with overhead

	//Call GetNMESpace
	if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lDealFKID,0,0)))
	{
	    PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter");
		goto SAFE_EXIT;
	}

	if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,2,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lTrackFKID,0,0)))
	{
	    PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter");
		goto SAFE_EXIT;
	}
		
	sret = SQLExecDirect(hstmtTRACK,szCallGetNMESpaceByDealAndTrack,SQL_NTS);
	if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret)))
	{
		PrintError(pSrvProc,"SQLExecDirect:Failed calling GetNMESpaceByDealAndTrack");
		goto SAFE_EXIT;
	}

	//unbind the Parameter from last call to SQLBindParameter
	SQLFreeStmt(hstmtTRACK,SQL_RESET_PARAMS);

	//don't proceed unless TRACK rowcount is > 0.
	if (SQL_SUCCESS != (sret = SQLRowCount(hstmtTRACK,&nRowCount)))
	{
		PrintError(pSrvProc,"SQLRowCount:Failed calling TRACK");
		goto SAFE_EXIT;
	}
	
	//undocumented ODBC functionality. SQLGetRowCount returns 0 if no rows were returned
	//(empty result set)
	//if the track is empty, indicate success, and finish processing
	if (0 == nRowCount )
	{
		rcXP = XP_NOERROR;
		goto SAFE_EXIT;
	}
	
	//now look at the result set associated with hstmtTRACK
	//should have FOUR result columns, 
	//NME_SPACE(ordinal 1),
	//UNIQUE_EVENT(ordinal 2),
	//NME_EDGE (ordinal3 )
	//LINK_FKID (ordinal 4) 
	// all four are unsigned longs
	//none can be null
	
	SQLNumResultCols(hstmtTRACK,&nNumCols);
	
	if(4 != nNumCols)
	{
		PrintError(pSrvProc,"Expected 4 return columns from GetPredecessorTableWithRecursion");
		goto SAFE_EXIT;
	}
	
	
	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,1,SQL_C_ULONG,&lNME_SPACE,0,&cbNME_SPACE)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on NME_SPACE");
		goto SAFE_EXIT;
	}

	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,2,SQL_C_ULONG,&lUNIQUE_EVENT,0,&cbUNIQUE_EVENT)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on UNIQUE_EVENT");
		goto SAFE_EXIT;
	}

	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,3,SQL_C_ULONG,&lNME_EDGE,0,&cbNME_EDGE)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on NME_EDGE");
		goto SAFE_EXIT;
	}
	
	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,4,SQL_C_ULONG,&lLINK_FKID,0,&cbLINK_FKID)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on LINK_FKID");
		goto SAFE_EXIT;
	}

	lLastUniqueEvent = 0;
	while (SQL_NO_DATA != (sret = SQLFetchScroll(hstmtTRACK,SQL_FETCH_NEXT,0)))
	{
		if (SQL_SUCCESS != sret)
		{
		 PrintError(pSrvProc,"SQLFetchScroll: Failed calling SQLFetchScroll on TRACK");
		 goto SAFE_EXIT;
		}

		if ((lLastUniqueEvent != lUNIQUE_EVENT) && (0 < lLastUniqueEvent))
		{
			if (!(pcAlgorithm->FlushBuffer()))
			{
			  PrintError(pSrvProc,"PushVertex: Failed attempting to flush comb buffer");
			  goto SAFE_EXIT;
			}
		}	
		if (!(pcAlgorithm->PushCombinationToBuffer (lNME_SPACE,lLINK_FKID)))
		{
		  PrintError(pSrvProc,"PushVertex: Failed attempting to push combination to buffer");
		  goto SAFE_EXIT;
		}

		if (!(pcAlgorithm->PushDisjointRegionToBuffer (lNME_EDGE)))
		{
		  PrintError(pSrvProc,"PushVertex: Failed attempting to push disjoint region to buffer");
		  goto SAFE_EXIT;
		}


		lLastUniqueEvent = lUNIQUE_EVENT;

	}

	//flush the buffer if we've run out of data
	if (0 < lLastUniqueEvent)
	{
		if (!(pcAlgorithm->FlushBuffer()))
		{
		  PrintError(pSrvProc,"PushVertex: Failed attempting to flush comb buffer");
		  goto SAFE_EXIT;
		}
	}	

	
	
	SQLFreeStmt(hstmtTRACK,SQL_UNBIND);
	SQLFreeStmt(hstmtTRACK,SQL_CLOSE);

	
	 //set up the call to szCallDeleteCombinationByDealAndTrack, using hstmtTRACK
	if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lDealFKID,0,0)))
	{
		PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter");
		goto SAFE_EXIT;
	}

	if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,2,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lTrackFKID,0,0)))
	{
		PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter");
		goto SAFE_EXIT;
	}
	//failure here
	sret = SQLExecDirect(hstmtTRACK,szCallDeleteCombinationByDealAndTrack,SQL_NTS);
	if(!( SQL_SUCCESS == sret || SQL_SUCCESS_WITH_INFO == sret || SQL_NO_DATA == sret ))
	{
	
		PrintError(pSrvProc,"SQLExecDirect:Failed calling deleting combination");
		goto SAFE_EXIT;
	}

	//unbind the Parameter from last call to SQLBindParameter
	SQLFreeStmt(hstmtTRACK,SQL_RESET_PARAMS);
	//	SQLFreeStmt(hstmtTRACK,SQL_CLOSE);

	pcAlgorithm->GetCombinations(lNumCombs,pCOMB);
	
	if (0 < lNumCombs)
	{
		
		SQLUINTEGER  lCOMB_NMESPACE					 = 0;
		SQLUINTEGER  lCOMB_LinkFKID				     = 0;
		SQLUINTEGER  lCOMB_RegionBuffer[BUF_LEN];
		
		
		//byte count
		SQLINTEGER cbCOMB_DisjointRegion			 = 0;
	
	   //set up the call to InsertCombination, using hstmtTRACK
		for(LPCOMBINATION p = pCOMB;p < pCOMB + lNumCombs; p++)
		{
		  
			lCOMB_NMESPACE			=	p->NME_SPACE;
			lCOMB_LinkFKID			=	p->LinkFKID; 
	   		cbCOMB_DisjointRegion	=   (p->lRegionCount)* sizeof(ULONG); 
			
			
			ZeroMemory(lCOMB_RegionBuffer,BUF_LEN*sizeof(ULONG));
			CopyMemory(lCOMB_RegionBuffer,p->plDisjointRegions,min(cbCOMB_DisjointRegion,BUF_LEN*sizeof(ULONG)));

			
			if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lCOMB_NMESPACE,0,0)))
			{
				PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 1");
				goto SAFE_EXIT;
			}

			if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,2,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lCOMB_LinkFKID,0,0)))
			{
				PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 2");
				goto SAFE_EXIT;
			}

			if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,3,SQL_PARAM_INPUT,SQL_C_BINARY,SQL_VARBINARY,BUF_LEN*sizeof(ULONG),0,lCOMB_RegionBuffer,0,&cbCOMB_DisjointRegion)))
			{
				PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 3");
				goto SAFE_EXIT;
			}

			sret = SQLExecDirect(hstmtTRACK,szCallInsertCombination,SQL_NTS);
			if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret)))
			{
				PrintError(pSrvProc,"SQLExecDirect:Failed calling InsertCombination");
				goto SAFE_EXIT;
			}
	
			//unbind the Parameter from last call to SQLBindParameter
			SQLFreeStmt(hstmtTRACK,SQL_RESET_PARAMS);
			SQLFreeStmt(hstmtTRACK,SQL_CLOSE);
			
		} 

	}//	if (0 < lNumPerms)
	rcXP = XP_NOERROR;

SAFE_EXIT:
	//free data buffers
	if (NULL != pcAlgorithm)
	{ 
	  delete pcAlgorithm;
	  pcAlgorithm = 0;
	}
	//free handles
	if (SQL_NULL_HSTMT != hstmtTRACK)
	{ 
		SQLFreeStmt(hstmtTRACK,SQL_UNBIND);
		SQLFreeStmt(hstmtTRACK,SQL_RESET_PARAMS);
		SQLFreeStmt(hstmtTRACK,SQL_CLOSE);
		SQLFreeHandle(SQL_HANDLE_STMT,hstmtTRACK);
	}

	return rcXP;
}

void PrintUsage(SRV_PROC* pSrvProc)
{
	srv_sendmsg(pSrvProc,SRV_MSG_ERROR,XP_COMBINATIONHANDLER_ERROR,16,1,NULL,0,(DBUSMALLINT) __LINE__,
				"Usage: exec xp_CombinationHandler @DEALFKID = x,@TRACKFKID = z|x>0;z>=0",SRV_NULLTERM);
	srv_senddone(pSrvProc,(SRV_DONE_ERROR|SRV_DONE_MORE),0,0);
}
void PrintError(SRV_PROC* pSrvProc,char *szErrorMsg)
{
	srv_sendmsg(pSrvProc,SRV_MSG_ERROR,XP_COMBINATIONHANDLER_ERROR,16,1,NULL,0,(DBUSMALLINT) __LINE__,
				szErrorMsg,SRV_NULLTERM);
	srv_senddone(pSrvProc,(SRV_DONE_ERROR|SRV_DONE_MORE),0,0);
}


/*
//"spare" code for calling SQLGetDiagRec
	SQLCHAR lclState[5];
				SQLINTEGER nlclError;
				SQLCHAR szErrorMsg[BUF_LEN];
				short nOutputLen;
				int i = 1;
				while(SQL_SUCCESS == 
					(sret = SQLGetDiagRec(SQL_HANDLE_STMT,hstmtTRACK,i,lclState,&nlclError,
										   szErrorMsg,BUF_LEN,&nOutputLen)))
					 
				{ i++;}
//end spare code
*/
