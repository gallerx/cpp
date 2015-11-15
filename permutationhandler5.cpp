// PermutationHandler5.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <iostream.h>
#include <stdio.h>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <odbcss.h>
#include <srv.h>
#include "cPH.h"
#define LINKED_BUT_NOT_RECURSION_POINT -1
#define BUF_LEN 1024

HENV		g_henv		= SQL_NULL_HENV;
HDBC		g_hdbc		= SQL_NULL_HDBC;

void		PrintUsage(SRV_PROC*);
void		PrintError(SRV_PROC*,char*);


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
   
	//UCHAR szConn[] = "Provider=MSDASQL;DRIVER={SQL SERVER};SERVER=WSTM0062728\\VSDOTNET;Database=ARM";
	//UCHAR szConn[] = "Provider=MSDASQL;DRIVER={SQL SERVER};SERVER=WSTM0000597;Database=ARM";
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

#define EXPECTED_PARAMS			   2
#define DEAL_PARAM				   1
#define TRACK_PARAM				   2 
#define XP_ERROR				   1
#define XP_NOERROR				   0
#define XP_PERMUTATIONHANDLER_ERROR  50001
ULONG __GetXpVersion()
{
	return ODS_VERSION;
}

/************************************************************
*   PermutationHandler entry function.
*   Takes 2 parameters: both longs
*   Parameter1: lDealFKID: identifies the deal to run PermutationHandler On
*   Parameter2: lTrackFKID: identifies the track which should be analyzed for permutations
*	//based on xp_odbc sample which ships with MSSQL 2000 under devtools\samples 
*************************************************************/
SRVRETCODE xp_PermutationHandler(SRV_PROC* pSrvProc)
	{

	BYTE				bType;
	ULONG				cbMaxLen;
	ULONG				cbActualLen;
	BOOL				fNull;
	ULONG				lLastPKID		= 0;
	SQLUINTEGER			lDealFKID		= 0;
	SQLUINTEGER			lTrackFKID		= 0;
	short				nOutputLen		= 0;
	long				*plMembers		= 0;
	short FETCH_MODE	 = SQL_FETCH_FIRST;

	HSTMT				hstmtTRACK		= SQL_NULL_HSTMT;
	HSTMT				hstmtCYCLE		= SQL_NULL_HSTMT;
	SQLRETURN			sret;
	SRVRETCODE			rc;
	short				nNumCols		= 0;
	long				nRowCount		= 0;
	long				lNumPerms		= 0;
	LPPERMUTATION		pPERM			= 0;


	
	CHAR szBindToken[256];
	UCHAR szCallBindSession[] =
		"{call sp_bindsession(?)}";
	UCHAR szCallGetPredecessorTableWithRecursionByDealAndTrack[] =
		"{call GetPTableWithRecursionByDealAndTrack(?,?)}";
	UCHAR szCallGetCycleMembershipByDealAndTrack[] =
		"{call GetCycleMembershipByDealAndTrack(?,?)}";
	UCHAR szCallDeletePermutationByDealAndTrack[] = 
		"{call DeletePermutationByDealAndTrack(?,?)}";
	UCHAR szCallInsertPermutation[] =
		"{call InsertPermutation(?,?,?,?)}"; //adds extra parameter for prior permutation

	
	SQLUINTEGER lEventPKID			   = 0;
	SQLUINTEGER lLinkPKID			   = 0;
	SQLUINTEGER lPredecessorPKID	   = 0;
	SQLUINTEGER lCyclePKID			   = 0;
	
	SQLINTEGER cbEventPKID			   = 0;
	SQLINTEGER cbLinkPKID			   = 0;
	SQLINTEGER cbPredecessorPKID	   = 0;
	SQLINTEGER cbCyclePKID			   = 0;
	
	
	SQLUINTEGER lCY_EventPKID		   = 0;
	SQLUINTEGER lCY_CyclePKID		   = 0;
	
	SQLINTEGER cbCY_CyclePKID          = 0;
	SQLINTEGER cbCY_EventPKID          = 0;

//Parameter validation section: Ensure we have:
     //A. two valid parameters
	 //B. both of data type long
	 //C. both not of return parameter type
	 //D. lDealFKID must be > 0
	 //E. lTrackFKID must by > 0

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


	if(!(0 < lTrackFKID))
	{
		PrintUsage(pSrvProc);
		return rcXP;
	}

	cPH *pcAlgorithm = new cPH();
	if (0 == pcAlgorithm)
	{
		PrintError(pSrvProc,"Failed to initialize Algorithm");
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

	//CallGetPredecessorTableWithRecursionByDealAndTrack
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
		
	sret = SQLExecDirect(hstmtTRACK,szCallGetPredecessorTableWithRecursionByDealAndTrack,SQL_NTS);
	if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret)))
	{
		PrintError(pSrvProc,"SQLExecDirect:Failed calling predecessortable");
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
	//EVENTPKID(ordinal 1),
	//LINKPKID (ordinal 2),
	//PREDECESSORPKID (ordinal3 )
	//CYCLEPKID (ordinal 4) 
	// all four are longs
	//LINKPKID,PREDECESSORPKID,CYCLEPKID can be NULL
	
	SQLNumResultCols(hstmtTRACK,&nNumCols);
	
	if(4 != nNumCols)
	{
		PrintError(pSrvProc,"Expected 4 return columns from GetPredecessorTableWithRecursion");
		goto SAFE_EXIT;
	}
	
	
	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,1,SQL_C_ULONG,&lEventPKID,0,&cbEventPKID)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on EventPKID");
		goto SAFE_EXIT;
	}

	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,2,SQL_C_ULONG,&lLinkPKID,0,&cbLinkPKID)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on LinkPKID");
		goto SAFE_EXIT;
	}

	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,3,SQL_C_ULONG,&lPredecessorPKID,0,&cbPredecessorPKID)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on PredecessorPKID");
		goto SAFE_EXIT;
	}
	
	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,4,SQL_C_ULONG,&lCyclePKID,0,&cbCyclePKID)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on CyclePKID");
		goto SAFE_EXIT;
	}

	//push the vertices 
	lLastPKID = 0;
	while (SQL_NO_DATA != (sret = SQLFetchScroll(hstmtTRACK,SQL_FETCH_NEXT,0)))
	{
		if (SQL_SUCCESS != sret)
		{
		 PrintError(pSrvProc,"SQLFetchScroll: Failed calling SQLFetchScroll on TRACK");
		 goto SAFE_EXIT;
		}

		if (lLastPKID != lEventPKID)
		{
			if (!(pcAlgorithm->PushVertex (lEventPKID)))
			{
			  PrintError(pSrvProc,"PushVertex: Failed attempting to push vertex");
			  goto SAFE_EXIT;
			}

			lLastPKID = lEventPKID;
		}
		
	}

	if (!(pcAlgorithm->InitializePredMatrix()))
	{
		PrintError(pSrvProc,"InitializeMatrix Failed");
		goto SAFE_EXIT;
	}

	//move back to beginning of TRACK result set and loop
	FETCH_MODE = SQL_FETCH_FIRST;
	while (SQL_NO_DATA != (sret = SQLFetchScroll(hstmtTRACK,FETCH_MODE,0)))
	{
		FETCH_MODE = SQL_FETCH_NEXT;
		if (SQL_SUCCESS != sret)
		{
		 PrintError(pSrvProc,"SQLFetchScroll: Failed calling SQLFetchScroll on hstmtTRACK");
		 goto SAFE_EXIT;
		}
		if (SQL_NULL_DATA != cbPredecessorPKID)
		{
			long lData =0;
			if(SQL_NULL_DATA != cbCyclePKID)
			{lData = lCyclePKID;}
			else
			{lData = LINKED_BUT_NOT_RECURSION_POINT;}
			
			if (!(pcAlgorithm->SetVertexHigh(lEventPKID,lPredecessorPKID,lData,lLinkPKID)))
			{
				PrintError(pSrvProc,"SetHigh: Failed calling SetHigh on hstmtTRACK");
				goto SAFE_EXIT;
			}
		}
	}

	SQLFreeStmt(hstmtTRACK,SQL_UNBIND);
	SQLFreeStmt(hstmtTRACK,SQL_CLOSE);

	
	//now retrieve 	Cycle Data
	//obtain another statement handle
	if( !(SQL_SUCCESS == (sret = SQLAllocHandle(SQL_HANDLE_STMT,g_hdbc,&hstmtCYCLE))))
	{
	    PrintError(pSrvProc,"SQLAllocHandle:Failed to obtain statement handle");
		goto SAFE_EXIT;
	}

	//set up for scrollable, snapshot recurds 
	if( !(SQL_SUCCESS == (sret = SQLSetStmtAttr(hstmtCYCLE,SQL_ATTR_CURSOR_TYPE,(SQLPOINTER)SQL_CURSOR_STATIC,SQL_IS_INTEGER))))
	{
	    PrintError(pSrvProc,"SQLAllocHandle:Failed to obtain statement handle");
		goto SAFE_EXIT;
	}

	if( !(SQL_SUCCESS == (sret = SQLSetStmtAttr(hstmtCYCLE,SQL_ATTR_CURSOR_SCROLLABLE,(SQLPOINTER)SQL_SCROLLABLE,SQL_IS_INTEGER))))
	{
	    PrintError(pSrvProc,"SQLAllocHandle:Failed to obtain statement handle");
		goto SAFE_EXIT;
	}

	if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtCYCLE,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lDealFKID,0,0)))
		{
			PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter");
			goto SAFE_EXIT;
		}

	if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtCYCLE,2,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lTrackFKID,0,0)))
		{
			PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter");
			goto SAFE_EXIT;
		}

	sret = SQLExecDirect(hstmtCYCLE,szCallGetCycleMembershipByDealAndTrack,SQL_NTS);
	if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret)))
	{
		PrintError(pSrvProc,"SQLExecDirect:Failed calling cycle data");
		goto SAFE_EXIT;
	}

	//unbind the Parameter from last call to SQLBindParameter
	SQLFreeStmt(hstmtCYCLE,SQL_RESET_PARAMS);
	
	//don't proceed unless CYCLE rowcount is > 0.
	if (SQL_SUCCESS != (sret = SQLRowCount(hstmtCYCLE,&nRowCount)))
	{
		PrintError(pSrvProc,"SQLRowCount:Failed calling CYCLE");
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

	//now go to cycle recordset and examine
	//now look at the result set associated with hstmtCYCLE
	//should have 2 result columns:
	//1. EVENTPKID(ordinal 1),
	//2. CYCLEPKID(ordinal 2),
	//both ULONGS no Nulls
	
	SQLNumResultCols(hstmtCYCLE,&nNumCols);
	
	if(2 != nNumCols)
	{
		PrintError(pSrvProc,"Expected 2 return columns from GetCycleMembership");
		goto SAFE_EXIT;
	}
	
		if(SQL_SUCCESS != (sret = SQLBindCol(hstmtCYCLE,1,SQL_C_ULONG,&lCY_EventPKID,0,&cbCY_EventPKID)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on EventPKID");
		goto SAFE_EXIT;
	}

	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtCYCLE,2,SQL_C_ULONG,&lCY_CyclePKID,0,&cbCY_CyclePKID)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on CyclePKID");
		goto SAFE_EXIT;
	}
	
	//now cycle through the Cycle Recordset
	lLastPKID = 0;
	while (SQL_NO_DATA != (sret = SQLFetchScroll(hstmtCYCLE,SQL_FETCH_NEXT,0)))
	{
		if (SQL_SUCCESS != sret)
		{
		 PrintError(pSrvProc,"SvQLFetchScroll: Failed calling SQLFetchScroll on hstmtCYCLE");
		 goto SAFE_EXIT;
		}

        if (lLastPKID != lCY_CyclePKID)
		{
			if(!(pcAlgorithm->PushCycle (lCY_CyclePKID)))
				{
				 PrintError(pSrvProc,"PushCycle: Failed calling PushCycle");
				 goto SAFE_EXIT;
				}

		    lLastPKID = lCY_CyclePKID;
		}
	}

	if (!(pcAlgorithm->InitializeVertexCycleMatrix()))
	{
		PrintError(pSrvProc,"InitializeVertexCycleMatrix Failed");
		goto SAFE_EXIT;
	}
	

	//move back to beginning of CYCLE result set and loop
	FETCH_MODE = SQL_FETCH_FIRST;
	while (SQL_NO_DATA != (sret = SQLFetchScroll(hstmtCYCLE,FETCH_MODE,0)))
	{
		FETCH_MODE = SQL_FETCH_NEXT;
		if (SQL_SUCCESS != sret)
		{
		 PrintError(pSrvProc,"SQLFetchScroll: Failed calling SQLFetchScroll on hstmtCYCLE");
		 goto SAFE_EXIT;
		}

		if (!(pcAlgorithm->SetVertexCycleHigh(lCY_EventPKID,lCY_CyclePKID)))
		{
			PrintError(pSrvProc,"SetVertexCycleHigh: Failed calling SetHigh on hstmtCYCLE");
			goto SAFE_EXIT;
		}

	}

	

	//deletePermutationsByDealandTrack
	 //set up the call to szCallDeletePermutationByDealAndTrack, using hstmtTRACK
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
		sret = SQLExecDirect(hstmtTRACK,szCallDeletePermutationByDealAndTrack,SQL_NTS);
		if(!( SQL_SUCCESS == sret || SQL_SUCCESS_WITH_INFO == sret || SQL_NO_DATA == sret ))
		{
		
			PrintError(pSrvProc,"SQLExecDirect:Failed calling deleting Permutation");
			goto SAFE_EXIT;
		}

		//unbind the Parameter from last call to SQLBindParameter
		SQLFreeStmt(hstmtTRACK,SQL_RESET_PARAMS);
		SQLFreeStmt(hstmtCYCLE,SQL_CLOSE);

		
		pcAlgorithm->GetPermutations(lNumPerms,pPERM);
	
		if (0 < lNumPerms)

		{
			
		SQLUINTEGER  lPERM_EventFKID	       = 0;
		SQLUINTEGER  lPERM_LinkFKID			   = 0;
		SQLUINTEGER  lPERM_CycleBuffer[BUF_LEN];
		SQLUINTEGER  lPERM_PriorCycleBuffer[BUF_LEN];
		
		SQLINTEGER cbPERM_EventFKID				= 0;
		SQLINTEGER cbPERM_LinkFKID				= 0;
		SQLINTEGER cbPERM_CycleBinary			= 0;
	    SQLINTEGER cbPERM_PriorCycleBinary      = 0;
		
		  //deletePermutationsByDealandTrack
		 //set up the call to InsertPermutation, using hstmtTRACK
		
			for(long i = 0; i < lNumPerms; i++)
			{
			  
				lPERM_EventFKID			=	(pPERM+ i)->EventFKID; 
				lPERM_LinkFKID			=	(pPERM+ i)->LinkFKID; 
	   		    cbPERM_CycleBinary		=   ((pPERM+ i)->lCycleCount)* sizeof(ULONG); 
				cbPERM_PriorCycleBinary	=   ((pPERM+ i)->lPriorCycleCount)* sizeof(ULONG); 
				
				ZeroMemory(lPERM_CycleBuffer,BUF_LEN*sizeof(ULONG));
				CopyMemory(lPERM_CycleBuffer,(pPERM+i)->plCycles, min(cbPERM_CycleBinary,BUF_LEN*sizeof(ULONG)));

				ZeroMemory(lPERM_PriorCycleBuffer,BUF_LEN*sizeof(ULONG));
				CopyMemory(lPERM_PriorCycleBuffer,(pPERM+i)->plPriorCycles, min(cbPERM_PriorCycleBinary,BUF_LEN*sizeof(ULONG)));


				if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lPERM_EventFKID,0,0)))
				{
					PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 1");
					goto SAFE_EXIT;
				}

				if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,2,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lPERM_LinkFKID,0,0)))
				{
					PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 2");
					goto SAFE_EXIT;
				}

				if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,3,SQL_PARAM_INPUT,SQL_C_BINARY,SQL_VARBINARY,BUF_LEN*sizeof(ULONG),0,lPERM_CycleBuffer,0,&cbPERM_CycleBinary)))
				{
					PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 3");
					goto SAFE_EXIT;
				}
				
				if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,4,SQL_PARAM_INPUT,SQL_C_BINARY,SQL_VARBINARY,BUF_LEN*sizeof(ULONG),0,lPERM_PriorCycleBuffer,0,&cbPERM_PriorCycleBinary)))
				{
					PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 4");
					goto SAFE_EXIT;
				}



				sret = SQLExecDirect(hstmtTRACK,szCallInsertPermutation,SQL_NTS);
				if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret)))
				{
					PrintError(pSrvProc,"SQLExecDirect:Failed calling InsertPermutation");
					goto SAFE_EXIT;
				}
		
				//unbind the Parameter from last call to SQLBindParameter
				SQLFreeStmt(hstmtTRACK,SQL_RESET_PARAMS);
				SQLFreeStmt(hstmtTRACK,SQL_CLOSE);
				
			}
	
		}
	rcXP = XP_NOERROR;
SAFE_EXIT:
	//free data buffers
	if (0 != pcAlgorithm)
	{ 
	  delete pcAlgorithm;
	  pcAlgorithm = 0;
	}
	//free handles
	if (SQL_NULL_HSTMT != hstmtCYCLE)
	{ 
		SQLFreeStmt(hstmtCYCLE,SQL_UNBIND);
		SQLFreeStmt(hstmtCYCLE,SQL_RESET_PARAMS);
		SQLFreeStmt(hstmtCYCLE,SQL_CLOSE);
		SQLFreeHandle(SQL_HANDLE_STMT,hstmtCYCLE);
	}
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
	srv_sendmsg(pSrvProc,SRV_MSG_ERROR,XP_PERMUTATIONHANDLER_ERROR,16,1,NULL,0,(DBUSMALLINT) __LINE__,
				"Usage: exec xp_PermutationHandler @DEALFKID = x,@TRACKFKID = z|x>0;z>0",SRV_NULLTERM);
	srv_senddone(pSrvProc,(SRV_DONE_ERROR|SRV_DONE_MORE),0,0);
}
void PrintError(SRV_PROC* pSrvProc,char *szErrorMsg)
{
	srv_sendmsg(pSrvProc,SRV_MSG_ERROR,XP_PERMUTATIONHANDLER_ERROR,16,1,NULL,0,(DBUSMALLINT) __LINE__,
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
			(sret = SQLGetDiagRec(SQL_HANDLE_DBC,hdbc,i,lclState,&nlclError,
			                       szErrorMsg,BUF_LEN,&nOutputLen)))
			 
		{ i++;}

//end spare code
*/
