// CycleHandler5.cpp : Defines the entry point for the DLL application.
//

/*************************************************************************
* CycleHandler
*
*
**************************************************************************/

#include "stdafx.h"
#include <iostream.h>
#include <stdio.h>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <odbcss.h>
#include <srv.h>
#include "cCH.h"

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
	UCHAR szOutput[1024];
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





#define EXPECTED_PARAMS  2
#define TRACK_PARAM      1
#define CYCLE_PARAM      2 
#define XP_ERROR		 1
#define XP_NOERROR		 0
#define XP_CYCLEHANDLER_ERROR  50001
ULONG __GetXpVersion()
{
	return ODS_VERSION;
}

/************************************************************
*  CycleHandler entry function.
*   Takes 2 parameters: both longs
*   Parameter1: lTrackFKID: identifies the track to run CycleHandler On
*   Parameter2: lCycleFKID: identifies the specific cycle to run CycleHandler
*      if no cycle is available, lCycleFKID will be passed as 0.
*      Note that parameter 2 overspecifies parameter 1.
*	//based on xp_odbc sample which ships with MSSQL 2000 under devtools\samples 
*************************************************************/
SRVRETCODE xp_CycleHandler(SRV_PROC* pSrvProc)
{

	BYTE bType;
	ULONG cbMaxLen;
	ULONG cbActualLen;
	BOOL fNull;
	ULONG lLastPKID		= 0;
	long lTrackFKID		= 0;
	long lCycleFKID		= 0;
	long *plPKID		= 0;
	long *plMembers		= 0;
	short FETCH_MODE	 = SQL_FETCH_FIRST;

	HSTMT		hstmtTRACK	= SQL_NULL_HSTMT;
	HSTMT		hstmtCYCLE	= SQL_NULL_HSTMT;
	SQLRETURN	sret;
	SRVRETCODE	rc;
	short		nNumCols	= 0;
	long		nRowCount	= 0;
	
	CHAR szBindToken[256];
	UCHAR *szCycleStoredProc;
	UCHAR szCallBindSession[] =							"{call sp_bindsession(?)}";
	UCHAR szCallGetPredecessorTableByTrackFKID[] =		"{call GetPTableByTrackFKID(?)}";
	UCHAR szCallGetCycleByTrackFKID[] =					"{call GetCycleByTrackFKID(?)}";
	UCHAR szCallGetCycleByPKID[] =						"{call GetEdgeByPKID(?)}";		
	UCHAR szDeleteCycleVertexCorrByCycle[] =			"{call DeleteCycleVertexCorrByCycle(?)}";
	UCHAR szInsertCycleVertexCorr[] =					"{call InsertCycleVertexCorr(?,?)}";
	UCHAR szDeleteEdgeByPKID[] =						"{call DeleteEdgeByPKID(?)}";
	UCHAR szSetEdgeDead[] =								"{call SetEdgeDead(?,1)}"; //always set to dead(:=1), in this implementation

	SQLUINTEGER lVertexPKID			= 0;
	SQLUINTEGER lPredecessorPKID	= 0;
	SQLUINTEGER lDead				= 0;
	SQLINTEGER cbVertexPKID			= 0;
	SQLINTEGER cbPredecessorPKID	= 0;
	SQLINTEGER cbDead				= 0;

	SQLUINTEGER  lCYCLEPKID	  = 0;
	SQLINTEGER   cbCYCLEPKID  = 0;
	SQLUINTEGER  lFROMFKID    = 0;
	SQLINTEGER   cbFROMFKID   = 0;
	SQLUINTEGER lTOFKID       = 0;
	SQLINTEGER   cbTOFKID     = 0;
	

//Parameter validation section: Ensure we have:
     //A. two valid parameters
	 //B. both of data type long
	 //C. both not of return parameter type
	 //D. lTrackFKID must be > 0
	 //E. lCycleFKID must by >=0

	//A. Two Valid Parms

	SRVRETCODE rcXP		= XP_ERROR;  //assume failure until proven otherwise 

	if(EXPECTED_PARAMS != srv_rpcparams(pSrvProc))
	{ //send error message and return
		PrintUsage (pSrvProc);
		return rcXP;
	}

	//Get Parameter info for the TRACK Parameter
	if (FAIL == srv_paraminfo(pSrvProc,TRACK_PARAM,&bType,&cbMaxLen,&cbActualLen,NULL,&fNull))
	{
		PrintError(pSrvProc,"srv_paraminfo failed on call to query parameter 1");
		return rcXP;
	}
	
	//make sure TRACKFKID wasn't passed as  a return parameter
	if (SRV_PARAMRETURN & srv_paramstatus(pSrvProc,TRACK_PARAM))
	{
		PrintUsage(pSrvProc);
		return rcXP;
	}

	//ensure TRACKFKID passed as four bit integer
	if (!((SRVINT4 == bType) || (SRVINT2 == bType) ||(SRVINTN == bType))) 
	{
		PrintUsage(pSrvProc);
		return rcXP;
	}

	//Get Actual TRACK Parameter
	if (FAIL == srv_paraminfo(pSrvProc,TRACK_PARAM,&bType,&cbMaxLen,&cbActualLen,(BYTE*)&lTrackFKID,&fNull))
	{
		PrintError(pSrvProc,"srv_paraminfo failed on call to retrieve parameter 1");
		return rcXP;
	}

	if(!(0 < lTrackFKID))
	{
		PrintUsage(pSrvProc);
		return rcXP;
	}

	//Get Parameter info for the CYCLE Parameter
	if (FAIL == srv_paraminfo(pSrvProc,CYCLE_PARAM,&bType,&cbMaxLen,&cbActualLen,NULL,&fNull))
	{
		PrintError(pSrvProc,"srv_paraminfo failed on call to query parameter 2");
		return rcXP;
	}
	
	//make sure CYCLEFKID wasn't passed as  a return parameter
	if (SRV_PARAMRETURN & srv_paramstatus(pSrvProc,CYCLE_PARAM) )
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

	//Get Actual CYCLE Parameter
	if (FAIL == srv_paraminfo(pSrvProc,CYCLE_PARAM,&bType,&cbMaxLen,&cbActualLen,(BYTE*)&lCycleFKID,&fNull))
	{
		PrintError(pSrvProc,"srv_paraminfo failed on call to retrieve parameter 2");
		return rcXP;
	}


	if(!(0 <= lCycleFKID))
	{
		PrintUsage(pSrvProc);
		return rcXP;
	}

	cCH *pcAlgorithm = new cCH();
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

	
	//GetPredecessorTableByTrackFKID
	if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lTrackFKID,0,0)))
	{
	    PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter");
		goto SAFE_EXIT;
	}

	sret = SQLExecDirect(hstmtTRACK,szCallGetPredecessorTableByTrackFKID,SQL_NTS);
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
	//should have three result columns, VertexPKID(ordinal 1), and Predecessor(ordinal 2), both longs and dead (ordinal 3), which is a tiny int
	//predecessor and dead can be NULL, dead = 1 is Dead state, dead = 0 is live state
	
	SQLNumResultCols(hstmtTRACK,&nNumCols);
	
	if(3 != nNumCols)
	{
		PrintError(pSrvProc,"Expected 3 return columns from GetPredecessorTableByTrackFKID");
		goto SAFE_EXIT;
	}
	
	
	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,1,SQL_C_ULONG,&lVertexPKID,0,&cbVertexPKID)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on VERTEXPKID");
		goto SAFE_EXIT;
	}

	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,2,SQL_C_ULONG,&lPredecessorPKID,0,&cbPredecessorPKID)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on PREDECESSOR");
		goto SAFE_EXIT;
	}

	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,3,SQL_C_ULONG,&lDead,0,&cbDead)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on DEAD");
		goto SAFE_EXIT;
	}

	
	//push the vertices 
	
	while (SQL_NO_DATA != (sret = SQLFetchScroll(hstmtTRACK,SQL_FETCH_NEXT,0)))
	{
		if (SQL_SUCCESS != sret)
		{
		 PrintError(pSrvProc,"SQLFetchScroll: Failed calling SQLFetchScroll on TRACK");
		 goto SAFE_EXIT;
		}

		if (lLastPKID != lVertexPKID)
		{
			if (!(pcAlgorithm->PushVertex(lVertexPKID)))
			{
			  PrintError(pSrvProc,"PushVertex: Failed attempting to push vertex");
			  goto SAFE_EXIT;
			}

			lLastPKID = lVertexPKID;
		}
		
	}
	
	if (!(pcAlgorithm->InitializeMatrix()))
	{
		PrintError(pSrvProc,"InitializeMatrix Failed");
		goto SAFE_EXIT;
	}
	
	//move back to beginning of TRACK result set
/*
	if (SQL_NO_DATA != (sret = SQLFetchScroll(hstmtTRACK,SQL_FETCH_FIRST,0)))
	{
		if (SQL_SUCCESS != sret)
		{
		 PrintError(pSrvProc,"SQLFetchScroll: Failed calling SQLFetchScroll on TRACK");
		 goto SAFE_EXIT;
		}

		if (SQL_NULL_DATA != cbPredecessorPKID)
		{
			if (!(pcAlgorithm->SetHigh(lVertexPKID,lPredecessorPKID,lDead)))
			{
				PrintError(pSrvProc,"SetHigh: Failed calling SetHigh on TRACK");
				goto SAFE_EXIT;
			}

		}

	}
	//now just loop
	*/
	FETCH_MODE = SQL_FETCH_FIRST;
	while (SQL_NO_DATA != (sret = SQLFetchScroll(hstmtTRACK,FETCH_MODE,0)))
	{
		FETCH_MODE = SQL_FETCH_NEXT;
		if (SQL_SUCCESS != sret)
		{
		 PrintError(pSrvProc,"SQLFetchScroll: Failed calling SQLFetchScroll on TRACK");
		 goto SAFE_EXIT;
		}

		
		if (SQL_NULL_DATA != cbPredecessorPKID)
		{
			if (!(pcAlgorithm->SetHigh(lVertexPKID,lPredecessorPKID,lDead)))
			{
				PrintError(pSrvProc,"SetHigh: Failed calling SetHigh on TRACK");
				goto SAFE_EXIT;
			}

		}
		
	}

	SQLFreeStmt(hstmtTRACK,SQL_CLOSE);
	pcAlgorithm->QuickSchedule();
	



//now retrieve 	Cycle Data

	//obtain another statement handle
	if( !(SQL_SUCCESS == (sret = SQLAllocHandle(SQL_HANDLE_STMT,g_hdbc,&hstmtCYCLE))))
	{
	    PrintError(pSrvProc,"SQLAllocHandle:Failed to obtain statement handle");
		goto SAFE_EXIT;
	}

	//set up for scrollable, snapshot recurds 
	//WAS SQL_CURSOR_STATIC
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

	if (lCycleFKID)
	{	
		plPKID = &lCycleFKID;
		szCycleStoredProc = szCallGetCycleByPKID;
	}
	else
	{
		plPKID = &lTrackFKID;
		szCycleStoredProc = szCallGetCycleByTrackFKID;
	}
		
	if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtCYCLE,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,plPKID,0,0)))
	{
		PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter");
		goto SAFE_EXIT;
	}
	sret = SQLExecDirect(hstmtCYCLE,szCycleStoredProc,SQL_NTS);
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
	//now look at the result set associated with hstmtCYCLE
	//should have three result columns, CYCLEPKID(ordinal 1), FROM_FKID(ordinal 2),TO_FKID (ordinal 3)
	//all ULONGS. None of three can be NULL
	
	
	SQLNumResultCols(hstmtCYCLE,&nNumCols);
	
	if(3 != nNumCols)
	{
		PrintError(pSrvProc,"Expected 3 return columns from GetCycleByTrackFKID|GetEdgeByPKID");
		goto SAFE_EXIT;
	}
	
	
	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtCYCLE,1,SQL_C_ULONG,&lCYCLEPKID,0,&cbCYCLEPKID)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on CYCLEPKID");
		goto SAFE_EXIT;
	}

	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtCYCLE,2,SQL_C_ULONG,&lFROMFKID,0,&cbFROMFKID)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on FROM_FKID");
		goto SAFE_EXIT;
	}

	if(SQL_SUCCESS != (sret = SQLBindCol(hstmtCYCLE,3,SQL_C_ULONG,&lTOFKID,0,&cbTOFKID)))
	{
		PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on TO_FKID");
		goto SAFE_EXIT;
	}

	//now cycle through the Cycle Recordset
	while (SQL_NO_DATA != (sret = SQLFetchScroll(hstmtCYCLE,SQL_FETCH_NEXT,0)))
	{
		if (SQL_SUCCESS != sret)
		{
		 PrintError(pSrvProc,"SQLFetchScroll: Failed calling SQLFetchScroll on CYCLE");
		 goto SAFE_EXIT;
		}

		//set up the call to DeleteCycleVertexCorrByCycle, using hstmtTRACK
		//Delete CycleVertexCorrByCycle
		if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lCYCLEPKID,0,0)))
		{
			PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter");
			goto SAFE_EXIT;
		}
		sret = SQLExecDirect(hstmtTRACK,szDeleteCycleVertexCorrByCycle,SQL_NTS);
		if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret) || (SQL_NO_DATA == sret)  ))
		{
			PrintError(pSrvProc,"SQLExecDirect:Failed calling deleting cyclevertexcorr");
			goto SAFE_EXIT;
		}

		//unbind the Parameter from last call to SQLBindParameter
		SQLFreeStmt(hstmtTRACK,SQL_RESET_PARAMS);
		SQLFreeStmt(hstmtTRACK,SQL_CLOSE);

		long lNumMembers =0;
	
		pcAlgorithm->CycleMembers(lTOFKID,lFROMFKID,lNumMembers,plMembers);

		if (0 == lNumMembers)
		{

			//set up the call to DeleteEDGEByPKID, using hstmtTRACK
			//eliminate the cycle because it has been broken (0 included vertexs)
			if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lCYCLEPKID,0,0)))
			{
				PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter");
				goto SAFE_EXIT;
			}

			sret = SQLExecDirect(hstmtTRACK,szDeleteEdgeByPKID,SQL_NTS);
			if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret)|| (SQL_NO_DATA == sret)))
			{
				PrintError(pSrvProc,"SQLExecDirect:Failed calling delete edge by PKID");
				goto SAFE_EXIT;
			}

			//unbind the Parameter from last call to SQLBindParameter
			SQLFreeStmt(hstmtTRACK,SQL_RESET_PARAMS);
			SQLFreeStmt(hstmtTRACK,SQL_CLOSE);

		}

		else if (2 <= lNumMembers)
		{

			for(long i = 0; i<lNumMembers; i++)
			{
				long lInsertFROMFKID = lCYCLEPKID;
				long lInsertTOFKID = *(plMembers+i);
			
				//set up the call to InsertCycleVertexCorr, using hstmtTRACK
				//eliminate the cycle because it has been broken (0 included vertexs)
				if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lInsertFROMFKID,0,0)))
				{
					PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter");
					goto SAFE_EXIT;
				}

				if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,2,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lInsertTOFKID,0,0)))
				{
					PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter");
					goto SAFE_EXIT;
				}

				sret = SQLExecDirect(hstmtTRACK,szInsertCycleVertexCorr,SQL_NTS);
				if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret)))
				{
					PrintError(pSrvProc,"SQLExecDirect:Failed calling delete edge by PKID");
					goto SAFE_EXIT;
				}

				//unbind the Parameter from last call to SQLBindParameter
				SQLFreeStmt(hstmtTRACK,SQL_RESET_PARAMS);
				SQLFreeStmt(hstmtTRACK,SQL_CLOSE);

			}
			//clean up memory
			delete [] plMembers;
			plMembers = 0;

		}

	}

	//NEW CODE
	pcAlgorithm->BitShiftRight(); //moves the 'live' connectivity bit in pred matrix members to least significant bit
									  //moves the 'physical' connectivity bit out of the question
	
	//now cycle through the Cycle Recordset
	FETCH_MODE = SQL_FETCH_FIRST;
	while (SQL_NO_DATA != (sret = SQLFetchScroll(hstmtCYCLE,FETCH_MODE ,0)))
	{
		FETCH_MODE = SQL_FETCH_NEXT;
		if (SQL_SUCCESS != sret)
		{
		 PrintError(pSrvProc,"SQLFetchScroll: Failed calling SQLFetchScroll on CYCLE");
		 goto SAFE_EXIT;
		}

	    long lNumMembers =0;
		
		pcAlgorithm->CycleMembers(lTOFKID,lFROMFKID,lNumMembers,plMembers);
	
		if (0 == lNumMembers)
		{

			//set up the call to SetEdgeDead
			//set it dead because there is no live path between endpoints
			if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lCYCLEPKID,0,0)))
			{
				PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 1 to setedgedead");
				goto SAFE_EXIT;
			}
		
			sret = SQLExecDirect(hstmtTRACK,szSetEdgeDead,SQL_NTS);
		
			if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret)|| (SQL_NO_DATA == sret)))
			{
				PrintError(pSrvProc,"SQLExecDirect:Failed calling SetEdgeDead");
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
	if (0 != plMembers) 
	{delete [] plMembers;}

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
	srv_sendmsg(pSrvProc,SRV_MSG_ERROR,XP_CYCLEHANDLER_ERROR,16,1,NULL,0,(DBUSMALLINT) __LINE__,
				"Usage: exec xp_CycleHandler @TRACKFKID = x,@CYCLEFKID = z|x>0;z>=0",SRV_NULLTERM);
	srv_senddone(pSrvProc,(SRV_DONE_ERROR|SRV_DONE_MORE),0,0);
}

void PrintError(SRV_PROC* pSrvProc,char *szErrorMsg)
{

	srv_sendmsg(pSrvProc,SRV_MSG_ERROR,XP_CYCLEHANDLER_ERROR,16,1,NULL,0,(DBUSMALLINT) __LINE__,
				szErrorMsg,SRV_NULLTERM);
	srv_senddone(pSrvProc,(SRV_DONE_ERROR|SRV_DONE_MORE),0,0);

}

/*
//"spare" code for calling SQLGetDiagRec
	SQLCHAR lclState[5];
		SQLINTEGER nlclError;
		SQLCHAR szErrorMsg[1024];
		short nOutputLen;
		int i = 1;
		while(SQL_SUCCESS == 
			(sret = SQLGetDiagRec(SQL_HANDLE_DBC,hdbc,i,lclState,&nlclError,
			                       szErrorMsg,1024,&nOutputLen)))
			 
		{ i++;}

//end spare code

int TestFunction()
{



	PushVertex(10);
	PushVertex(20);
	PushVertex(30);
	PushVertex(40);
    PushVertex(50);
	PushVertex(60);
	PushVertex(70);
	PushVertex(80);

	for(int i = 0;i<g_lVertexCount;i++)
	{printf("Index: %d, PKID: %d.\n",i,g_plVertexVector[i]);}

	InitializeMatrix();	

	SetHigh(20,10);
	SetHigh(30,20);
	SetHigh(40,30);
	SetHigh(50,20);
	SetHigh(60,50);
	SetHigh(30,60);
	SetHigh(80,60);
	SetHigh(50,70);

	ScheduleLayer(1);

	for(i = 0; i < g_lVertexCount;i++)
	{ printf("Index item %d : PKID = %d : Layer= %d.\n",i,g_plVertexVector[i],g_plScheduleVector[i]);}

	for (long y = 10; y<90;y+=10)
	{
		for(long z = 10;z<90;z+=10)
		{
			printf("Is predecessor %d,%d: %d\n.",y,z,IsPredecessor(y,z));//1 = error
		}

	}
	
	long lNumMembers =0;
	long *plMemberArray = 0;
	long x = 10;
	long w = 40;
//	CycleMembers(x,w,lNumMembers,plMemberArray);
	printf("Num members: %d\n",lNumMembers);
	if (0==lNumMembers)
	{printf("No CycleMembers Returned.\n");}
	else
	{printf("Cycle Members %d,%d\n",x,w);
	for(i = 0; i<lNumMembers;i++)
	{	printf("Member %d: %d.\n",i,*(plMemberArray+i) );}
	}
	delete [] plMemberArray;

	TerminateMatrix();


	printf("complete\n");

    return 0;

}

*/
