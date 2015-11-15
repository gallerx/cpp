// EventTimeHandler5.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include <iostream.h>
#include <stdio.h>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <odbcss.h>
#include <srv.h>
#include "cETH.h"


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
	//UCHAR szConn[] = "Provider=MSDASQL;DRIVER={SQL SERVER};
	//		SERVER=TERROR;Database=ARM";
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
			if(SQL_SUCCESS != (sret = SQLSetEnvAttr(g_henv,SQL_ATTR_ODBC_VERSION,
			   (SQLPOINTER)SQL_OV_ODBC3,SQL_IS_INTEGER)))
			{ goto SAFE_EXIT;}
			

			//allocate a connection handle
			if(SQL_SUCCESS != (sret = SQLAllocHandle(SQL_HANDLE_DBC,g_henv,&g_hdbc)))
			{goto SAFE_EXIT;}
			

			//use cursor library to obtain support for scrollable cursors
			//per documentation, this call needs to precede SQLDriverConnect
			if(!(SQL_SUCCESS == (sret = 		
				SQLSetConnectAttr(g_hdbc, SQL_ATTR_ODBC_CURSORS,
				(SQLPOINTER)SQL_CUR_USE_ODBC,SQL_IS_INTEGER))))
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

#define EXPECTED_PARAMS			   1
#define DEAL_PARAM				   1
#define XP_ERROR				   1
#define XP_NOERROR				   0
#define XP_EVENTTIMEHANDLER_ERROR  40001
ULONG __GetXpVersion()
{
	return ODS_VERSION;
}

/************************************************************
*   EventTimeHandler entry function.
*   Takes 1 parameters of type long
*   Parameter1: lDealFKID: identifies the deal to run EventTimeHandler On
*	//based on xp_odbc sample which ships with MSSQL 2000 under devtools\samples 
*************************************************************/



SRVRETCODE xp_EventTimeHandler(SRV_PROC* pSrvProc)
	{

	BYTE				bType;
	ULONG				cbMaxLen;
	ULONG				cbActualLen;
	BOOL				fNull;
	ULONG				lLastPKID		= 0;
	SQLUINTEGER			lDealFKID		= 0;
	short				nOutputLen		= 0;

	HSTMT				hstmtDEAL		= SQL_NULL_HSTMT;
	SQLRETURN			sret;
	SRVRETCODE			rc;
	short				nNumCols		= 0;
	long				nRowCount		= 0;
	long				lNumDirtyET		= 0;
	LPEVENTTIME			pETDirty		= 0;


	
	CHAR szBindToken[256];
	UCHAR szCallBindSession[] =
		"{call sp_bindsession(?)}";
	UCHAR szCallGetDealAnnounceDate[] =
		"{call GetDealAnnounceDateByDealFKID(?)}";
	UCHAR szCallGetPredecessorTableWithTimesByDealFKID[] =
		"{call GetPTableWithTimesByDealFKID(?)}";
	UCHAR szCallUpdateEventTime[] =
		"{call UpdateEventTimeByXP(?,?,?,?,?)}";
	
	TIMESTAMP_STRUCT	              tsANNOUNCE_DATE;
	TIMESTAMP_STRUCT	  			  tsLIMIT_DATE; //persistent storage
	SQLINTEGER cbtsANNOUNCE_DATE       = sizeof(TIMESTAMP_STRUCT);
	
	SQLUINTEGER lEventPKID			   = 0;
	SQLUINTEGER lPredecessorPKID	   = 0;
	TIMESTAMP_STRUCT					tsStart;
	TIMESTAMP_STRUCT					tsFinish;
	SQLUINTEGER lDuration			   = 0;
	SQLINTEGER	ciConstraint           = 0;
	SQLINTEGER  bConflict              = 0;
	ULONG		lBROKEN_RULE_BUFFER[BUF_LEN];

	SQLINTEGER cbEventPKID			   = 0;
	SQLINTEGER cbPredecessorPKID	   = 0;
	SQLINTEGER cbStart				   = 0;
	SQLINTEGER cbFinish				   = 0;
	SQLINTEGER cbConstraint			   = 0;
	SQLINTEGER cbConflict			   = 0;
	SQLINTEGER cbDuration			   = 0;
	SQLINTEGER cbBrokenRule			   = 0;

//Parameter validation section: Ensure we have:
     //A. one valid parameters
	 //B.  of data type long
	 //C.  not of return parameter type
	 //D. lDealFKID must be > 0

	//A. Owo Valid Parms

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

	cETH *pcAlgorithm = new cETH();
	if (0 == pcAlgorithm)
	{
		PrintError(pSrvProc,"Failed to initialize Algorithm");
		goto SAFE_EXIT;
	}

	//obtain a statement handle
	if( !(SQL_SUCCESS == (sret = SQLAllocHandle(SQL_HANDLE_STMT,g_hdbc,&hstmtDEAL))))
	{
	    PrintError(pSrvProc,"SQLAllocHandle:Failed to obtain statement handle");
		goto SAFE_EXIT;
	}

	//set up for scrollable, snapshot recurds 
	if( !(SQL_SUCCESS == (sret = SQLSetStmtAttr(hstmtDEAL,SQL_ATTR_CURSOR_TYPE,(SQLPOINTER)SQL_CURSOR_STATIC,SQL_IS_INTEGER))))
	
	{
		PrintError(pSrvProc,"SQLSetStmtAttr:Failed to set cursor static attribute");
		goto SAFE_EXIT;
	}
	
	if( !(SQL_SUCCESS == (sret = SQLSetStmtAttr(hstmtDEAL,SQL_ATTR_CURSOR_SCROLLABLE,(SQLPOINTER)SQL_SCROLLABLE,SQL_IS_INTEGER))))
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
	if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtDEAL,1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,255,0,szBindToken,256,NULL)))
	{
	    PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter");
		goto SAFE_EXIT;
	}
	//call sp_bindsession to bind our session to client's session such that we share transaction space
	sret = SQLExecDirect(hstmtDEAL,szCallBindSession,SQL_NTS);
	if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret)))
	{
	    PrintError(pSrvProc,"SQLExecDirect:Failed to establish bound transaction session");
		goto SAFE_EXIT;
	}

	//unbind the Parameter from last call to SQLBindParameter
	SQLFreeStmt(hstmtDEAL,SQL_RESET_PARAMS);
	//done with overhead


/////////////////////////////////////////////////CALL GET ANNOUNCE DATE
//szCallGetDealAnnounceDate
	if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtDEAL,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lDealFKID,0,0)))
	{
	    PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter");
		goto SAFE_EXIT;
	}


	sret = SQLExecDirect(hstmtDEAL,szCallGetDealAnnounceDate,SQL_NTS);
	if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret)))
	{
		
		PrintError(pSrvProc,"SQLExecDirect:Failed calling DealAnnounceDate");
		goto SAFE_EXIT;
	}

	//unbind the Parameter from last call to SQLBindParameter
	SQLFreeStmt(hstmtDEAL,SQL_RESET_PARAMS);

	//don't proceed unless DEAL rowcount is > 0.
	if (SQL_SUCCESS != (sret = SQLRowCount(hstmtDEAL,&nRowCount)))
	{
		PrintError(pSrvProc,"SQLRowCount:Failed calling DealAnnounceDate");
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

	//now look at the result set associated with hstmtDEAL
	//should have one result columns, 
	//RETURNDATE(ordinal 1),
	
	SQLNumResultCols(hstmtDEAL,&nNumCols);
	
	if(1 != nNumCols)
	{
		PrintError(pSrvProc,"Expected 1 return columns from GetDealAnnounceDate");
		goto SAFE_EXIT;
	}

	

	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtDEAL,1,SQL_C_TYPE_TIMESTAMP,&tsANNOUNCE_DATE,0,&cbtsANNOUNCE_DATE)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on ReturnDate");
		goto SAFE_EXIT;
	}

	while (SQL_NO_DATA != (sret = SQLFetchScroll(hstmtDEAL,SQL_FETCH_NEXT,0)))
	{
		if (SQL_SUCCESS != sret)
		{
			
		PrintError(pSrvProc,"SQLFetchScroll: Failed calling SQLFetchScroll on DEAL");
		 goto SAFE_EXIT;
		}

		tsLIMIT_DATE = tsANNOUNCE_DATE;
	
	}

	SQLFreeStmt(hstmtDEAL,SQL_UNBIND);
	SQLFreeStmt(hstmtDEAL,SQL_RESET_PARAMS);
	SQLFreeStmt(hstmtDEAL,SQL_CLOSE);

	//////////////////////////////////////////////////END CALL GET ANNOUNCE DATE


	//szCallGetPredecessorTableWithTimesByDeal
	if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtDEAL,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lDealFKID,0,0)))
	{
	    PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter");
		goto SAFE_EXIT;
	}

		
	sret = SQLExecDirect(hstmtDEAL,szCallGetPredecessorTableWithTimesByDealFKID,SQL_NTS);
	if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret)))
	{
		PrintError(pSrvProc,"SQLExecDirect:Failed calling predecessortable");
		goto SAFE_EXIT;
	}

	//unbind the Parameter from last call to SQLBindParameter
	SQLFreeStmt(hstmtDEAL,SQL_RESET_PARAMS);

	//don't proceed unless DEAL rowcount is > 0.
	if (SQL_SUCCESS != (sret = SQLRowCount(hstmtDEAL,&nRowCount)))
	{
		PrintError(pSrvProc,"SQLRowCount:Failed calling DEAL");
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

	//now look at the result set associated with hstmtDEAL
	//should have EIGHT result columns, 
	//EVENTPKID(ordinal 1),
	//PREDECESSORPKID (ordinal2)
	//START TIME (ordinal 3)
	//FINISH TIME (ordinal 4)
	//DURATION(ordinal 5)
	//CONSTRAINT ID (ordinal 6: CI_ASAP|CI_SDK|CI_CE)
	//CONFLICT (ordinal 7)
	//BROKEN_RULE BINARY *ordinal 8)
	//BROKEN RULE BINARY CAN BE NULL. 
	

	SQLNumResultCols(hstmtDEAL,&nNumCols);
	
	if(8 != nNumCols)
	{
		PrintError(pSrvProc,"Expected 8 return columns from GetPredecessorTableWithEventTimesByDeal");
		goto SAFE_EXIT;
	}
	
	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtDEAL,1,SQL_C_ULONG,&lEventPKID,0,&cbEventPKID)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on EventPKID");
		goto SAFE_EXIT;
	}

	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtDEAL,2,SQL_C_ULONG,&lPredecessorPKID,0,&cbPredecessorPKID)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on PredecessorPKID");
		goto SAFE_EXIT;
	}

	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtDEAL,3,SQL_C_TYPE_TIMESTAMP,&tsStart,0,&cbStart)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on START_TIME");
		goto SAFE_EXIT;
	}

	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtDEAL,4,SQL_C_TYPE_TIMESTAMP,&tsFinish,0,&cbFinish)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on FINISH_TIME");
		goto SAFE_EXIT;
	}
	

	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtDEAL,5,SQL_C_ULONG,&lDuration,0,&cbDuration)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on DURATION");
		goto SAFE_EXIT;
	}


	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtDEAL,6,SQL_C_LONG,&ciConstraint,0,&cbConstraint)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on CONSTRAINT");
		goto SAFE_EXIT;
	}

	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtDEAL,7,SQL_C_LONG,&bConflict,0,&cbConstraint)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on CONSTRAINT");
		goto SAFE_EXIT;
	}

	//CAN BE NULL!!!!
	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtDEAL,8,SQL_C_BINARY,lBROKEN_RULE_BUFFER,BUF_LEN*sizeof(long),&cbBrokenRule)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on BROKEN_RULE_BINARY");
		goto SAFE_EXIT;
	}

	//zero out our buffer
	ZeroMemory(lBROKEN_RULE_BUFFER,BUF_LEN*sizeof(CHAR));
	
	//push the vertices 
	lLastPKID = 0;
	while (SQL_NO_DATA != (sret = SQLFetchScroll(hstmtDEAL,SQL_FETCH_NEXT,0)))
	{
		if (SQL_SUCCESS != sret)
		{
	
		PrintError(pSrvProc,"SQLFetchScroll: Failed calling SQLFetchScroll on DEAL");
		 goto SAFE_EXIT;
		}

		if (lLastPKID != lEventPKID)
		{
			
			if(SQL_NULL_DATA == cbBrokenRule) {cbBrokenRule = 0;} //check for null cbBrokenRule
			
				if (!(pcAlgorithm->PushVertex (lEventPKID,tsStart,tsFinish,lDuration,ciConstraint,bConflict,lBROKEN_RULE_BUFFER,cbBrokenRule)))
			{
			  PrintError(pSrvProc,"PushVertex: Failed attempting to push vertex");
			  goto SAFE_EXIT;
			}

			lLastPKID = lEventPKID;
		}
		//zero out our buffer
		ZeroMemory(lBROKEN_RULE_BUFFER,BUF_LEN*sizeof(long));
	
		
	}

	if (!(pcAlgorithm->InitializeMatrix()))
	{
		PrintError(pSrvProc,"InitializeMatrix Failed");
		goto SAFE_EXIT;
	}

	//move back to beginning of TRACK result set
	if (SQL_NO_DATA != (sret = SQLFetchScroll(hstmtDEAL,SQL_FETCH_FIRST,0)))
	{
			
		if (SQL_SUCCESS != sret)
		{
		 PrintError(pSrvProc,"SQLFetchScroll: Failed calling SQLFetchScroll on hstmtDEAL");
		 goto SAFE_EXIT;
		}
		if (SQL_NULL_DATA != cbPredecessorPKID)
		{
			if (!(pcAlgorithm->SetHigh(lEventPKID,lPredecessorPKID)))
			{
				PrintError(pSrvProc,"SetHigh: Failed calling SetHigh on DEAL");
				goto SAFE_EXIT;
			}

		}
	}
	//now just loop
	while (SQL_NO_DATA != (sret = SQLFetchScroll(hstmtDEAL,SQL_FETCH_NEXT,0)))
	{
		if (SQL_SUCCESS != sret)
		{
		 PrintError(pSrvProc,"SQLFetchScroll: Failed calling SQLFetchScroll on hstmtDEAL");
		 goto SAFE_EXIT;
		}
		if (SQL_NULL_DATA != cbPredecessorPKID)
		{
			if (!(pcAlgorithm->SetHigh(lEventPKID,lPredecessorPKID)))
			{
				PrintError(pSrvProc,"SetHigh: Failed calling SetHigh on DEAL");
				goto SAFE_EXIT;
			}

		}
	}

	SQLFreeStmt(hstmtDEAL,SQL_UNBIND);
	SQLFreeStmt(hstmtDEAL,SQL_CLOSE);
		
	pcAlgorithm->TimeHandler(tsLIMIT_DATE,lNumDirtyET,pETDirty);
	
	if (0 < lNumDirtyET)

	{
		SQLUINTEGER			lUET_FKID            = 0;  //UET = UpdateEventTime
		TIMESTAMP_STRUCT	tsUET_Start;
		TIMESTAMP_STRUCT    tsUET_Finish;
		SQLINTEGER			bUET_Conflict		 = 0;
		SQLUINTEGER			lUET_BrokenRuleBuffer[BUF_LEN];


		
		SQLINTEGER			cbUET_FKID           = 0;  //UET = UpdateEventTime
		SQLINTEGER			cbUET_Start			 = 0;
		SQLINTEGER			cbUET_Finish		 = 0;
		SQLINTEGER			cbUET_Conflict		 = 0;
		SQLINTEGER			cbUET_BrokenRules    = 0;

			

		//UpdateEventTimes
		//set up the call to UpdateEventTimes, using hstmtTRACK
		for(long i = 0; i < lNumDirtyET; i++)
		{
			  
			lUET_FKID            = (pETDirty+i)->lPKID;
			tsUET_Start			 = pcAlgorithm->ULargeIntegerToTimestamp((pETDirty+i)->ulStart);
			tsUET_Finish		 = pcAlgorithm->ULargeIntegerToTimestamp((pETDirty+i)->ulFinish);
			bUET_Conflict		 = (pETDirty+i)->bConflict;
			if(0== (pETDirty+i)->cbBrokenRules)//set up for inserting null 
			{ cbUET_BrokenRules = SQL_NULL_DATA;}
			else
			{
				cbUET_BrokenRules = (pETDirty+i)->cbBrokenRules;
				ZeroMemory(lUET_BrokenRuleBuffer,BUF_LEN*sizeof(long));
				CopyMemory(lUET_BrokenRuleBuffer,(pETDirty+i)->plBrokenRules, min((pETDirty+i)->cbBrokenRules,BUF_LEN*sizeof(long)));
			}
			
			if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtDEAL,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lUET_FKID,0,0)))
			{
				PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 1");
				goto SAFE_EXIT;
			}

			if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtDEAL,2,SQL_PARAM_INPUT,SQL_C_TYPE_TIMESTAMP,SQL_TYPE_TIMESTAMP,23,3,&tsUET_Start,0,0)))
			{
				
				PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 2");
				goto SAFE_EXIT;
			}

			if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtDEAL,3,SQL_PARAM_INPUT,SQL_C_TYPE_TIMESTAMP,SQL_TYPE_TIMESTAMP,23,3,&tsUET_Finish,0,0)))
			{
				PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 3");
				goto SAFE_EXIT;
			}

		
			if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtDEAL,4,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&bUET_Conflict,0,0)))
			{
				PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 4");
				goto SAFE_EXIT;
			}

			if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtDEAL,5,SQL_PARAM_INPUT,SQL_C_BINARY,SQL_VARBINARY,BUF_LEN*sizeof(ULONG),0,lUET_BrokenRuleBuffer,0,&cbUET_BrokenRules)))
			{
				PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 5");
				goto SAFE_EXIT;
			}
			

			sret = SQLExecDirect(hstmtDEAL,szCallUpdateEventTime,SQL_NTS);
			if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret)))
			{
				PrintError(pSrvProc,"SQLExecDirect:Failed calling UpdateEventTime");
				goto SAFE_EXIT;
			}
	
			//unbind the Parameter from last call to SQLBindParameter
			SQLFreeStmt(hstmtDEAL,SQL_RESET_PARAMS);
			SQLFreeStmt(hstmtDEAL,SQL_CLOSE);
			
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

	if (SQL_NULL_HSTMT != hstmtDEAL)
	{ 
		SQLFreeStmt(hstmtDEAL,SQL_UNBIND);
		SQLFreeStmt(hstmtDEAL,SQL_RESET_PARAMS);
		SQLFreeStmt(hstmtDEAL,SQL_CLOSE);
		SQLFreeHandle(SQL_HANDLE_STMT,hstmtDEAL);
	}
	return rcXP;
}


void PrintUsage(SRV_PROC* pSrvProc)
{
	srv_sendmsg(pSrvProc,SRV_MSG_ERROR,XP_EVENTTIMEHANDLER_ERROR,
		16,1,NULL,0,(DBUSMALLINT) __LINE__,
		"Usage: exec xp_CycleHandler @TRACKFKID = x,@CYCLEFKID = z|x>0;z>=0",SRV_NULLTERM);
	srv_senddone(pSrvProc,(SRV_DONE_ERROR|SRV_DONE_MORE),0,0);
}
void PrintError(SRV_PROC* pSrvProc,char *szErrorMsg)
{
	srv_sendmsg(pSrvProc,SRV_MSG_ERROR,XP_EVENTTIMEHANDLER_ERROR,
		16,1,NULL,0,(DBUSMALLINT) __LINE__,szErrorMsg,SRV_NULLTERM);
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

