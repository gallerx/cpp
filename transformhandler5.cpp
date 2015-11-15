// TransformHandler5.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <iostream.h>
#include <stdio.h>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <odbcss.h>
#include <srv.h>
#include "cTH.h"
#include <vector>



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

#define EXPECTED_PARAMS					 0   //takes no parameters
#define MODE_PARAM						 1
#define XP_ERROR						 1
#define XP_NOERROR						 0
#define XP_TRANSFORMHANDLER_ERROR  70001
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
SRVRETCODE xp_TransformHandler(SRV_PROC* pSrvProc)
	{
	short				nOutputLen		= 0;
	short				FETCH_MODE		= SQL_FETCH_FIRST;
	short				IO_COLS			= 0;
	const short			ME_IO_COLS		= 5;
	const short			NME_IO_COLS		= ME_IO_COLS + 1;
	DBCHAR *			szColNames[7] = {"EXCLUSION_SPACE","PERMUTATION","PRIOR_PERMUTATION","ACTUAL_EDGE","DURATION","DISJOINT_SUBSET","ET_ID"};	

	HSTMT				hstmtTRACK		= SQL_NULL_HSTMT;
	SQLRETURN			sret;
	SRVRETCODE			rc;
	short				nNumCols		= 0;
	long				nRowCount		= 0;
	long				lNumPerms		= 0;
	ULONG				lNumMembers		= 0;
	cTH				   *pcAlgorithm     = NULL;
	long				lModeID			= 0;
	
	CHAR szBindToken[256];
	UCHAR szCallBindSession[] =
		"{call sp_bindsession(?)}";
	UCHAR szCallGetDeadCycles[] =
		"{call GetDeadCycles}";
	UCHAR szCallGetDeadEdges[] =
		"{call GetDeadEdgesWAssignment(?)}";
	UCHAR szCallGetNMECount[] = 
		"{call GetNMECount}";
	UCHAR szCallGetMECount[] = 
		"{call GetMECount}";
	UCHAR szCallDeleteTransform[] =
		"{call DeleteTransformProbs}";
	UCHAR szCallInsertTransform[] =
		"{Call InsertTransformProbs(?,?,?,?,?,?,?)}";

	UCHAR * pszCall = NULL;

	vector<cNME>::iterator itrNME   = NULL;
	//new code
	vector <cNME> vResults;
	//end new code
	
	//ExclusionSpace(ordinal 1, ulong),
	//PriorPermutation(ordinal 2, varbinary [binary seqeunce of ulongs]),
	//Permutation(ordinal3, varbinary [binary seqeunce of ulongs])
	//ActualEdge(ordinal 4, ulong) 
	//Duration(ordinal 5, ulong)
	// ordinal 6 only appears on mode 2
	//Disjoint Subset (ordinal 6, varbinary [binary seqeunce of ulongs])
	
	//for GetN|MECount
	SQLUINTEGER lExclusionSpace       = 0;
	SQLUINTEGER lActualEdge			  = 0;
	SQLUINTEGER lDuration			  = 0;
	SQLUINTEGER	lETID				  = 0;
	SQLUINTEGER lPermBuf			[sizeof(ULONG)*BUF_LEN];
	SQLUINTEGER lPriorPermBuf		[sizeof(ULONG)*BUF_LEN];
	SQLUINTEGER lDJSBuf				[sizeof(ULONG)*BUF_LEN];
	

	//for GetDeadEdgesWAssignment
	SQLUINTEGER lDeadEdgePKID		  = 0;
	SQLUINTEGER lAssignedToPKID		  = 0;
	
	//for GetDeadCycles
	SQLUINTEGER lDeadCyclePKID		  = 0;

	//from GetN|MECount
	SQLINTEGER cbExclusionSpace		  = 0;
	SQLINTEGER cbActualEdge			  = 0;
	SQLINTEGER cbDuration			  = 0;
	SQLINTEGER cbPerm				  = 0;
	SQLINTEGER cbPriorPerm			  = 0;
	SQLINTEGER cbDJS				  = 0;
	
	//for GetDeadEdgesWAssignment
	SQLINTEGER cbDeadEdgePKID		  = 0;
	SQLINTEGER cbAssignedToPKID		  = 0;
	
	//for GetDeadCycles
	SQLINTEGER cbDeadCyclePKID		  = 0;

/***********************************************************************************************/
/*  Validate the Input Parameter
/***********************************************************************************************/

//Parameter validation section: Ensure we have:
     //A. one valid parameters
	 //B. of data type long
	 //C. not of return parameter type
	 //D. lModeID 1 or 2

	//A. one valid parameters
	SRVRETCODE rcXP		= XP_ERROR;  //assume failure until proven otherwise 

//	if(EXPECTED_PARAMS != srv_rpcparams(pSrvProc))
//	{ //send error message a1nd return
		//PrintUsage (pSrvProc);
	//	return rcXP;
	//	}
	
	/***********************************************************************************************/
	/*  Set up our single hSTMT for scrollable snapshot records
	/***********************************************************************************************/

	//obtain a statement handle
	if( !(SQL_SUCCESS == (sret = SQLAllocHandle(SQL_HANDLE_STMT,g_hdbc,&hstmtTRACK))))
	{
		PrintError(pSrvProc,"SQLAllocHandle:Failed to obtain statement handle");
		goto SAFE_EXIT;
	}

	//set up for snapshot records 
	if( !(SQL_SUCCESS == (sret = SQLSetStmtAttr(hstmtTRACK,SQL_ATTR_CURSOR_TYPE,(SQLPOINTER)SQL_CURSOR_STATIC,SQL_IS_INTEGER))))
	
	{
		PrintError(pSrvProc,"SQLSetStmtAttr:Failed to set cursor static attribute");
		goto SAFE_EXIT;
	}
	
	/***********************************************************************************************/
	/*  Bind the token
	/***********************************************************************************************/

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
	SQLFreeStmt(hstmtTRACK,SQL_CLOSE);
	//done with overhead
	/***********************************************************************************************/
	/*  Initialize the algorithm
	/***********************************************************************************************/

	for (lModeID = ME_MODE; lModeID <= NME_MODE;lModeID++)
	{

		pcAlgorithm =  new cTH(lModeID);
		if (0 == pcAlgorithm)
		{
			PrintError(pSrvProc,"Failed to initialize Algorithm");
			goto SAFE_EXIT;
		}

		switch(lModeID)
		{
		case ME_MODE:
			IO_COLS = ME_IO_COLS;
			pszCall = szCallGetMECount;
			break;
		case NME_MODE:
			IO_COLS = NME_IO_COLS;
			pszCall = szCallGetNMECount;
			break;
		}

	/***********************************************************************************************/
	/*  GetNME or ME Count, and store line items in the algorithm class
	/***********************************************************************************************/

		//CallGetNME|MECount
		SQLFreeStmt(hstmtTRACK,SQL_RESET_PARAMS);
		SQLFreeStmt(hstmtTRACK,SQL_CLOSE);
		sret = SQLExecDirect(hstmtTRACK,pszCall,SQL_NTS);
		if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret)))
		{
			PrintError(pSrvProc,"SQLExecDirect:Failed calling ME or NME count");
			goto SAFE_EXIT;
		}

		//don't proceed unless TRACK rowcount is > 0.
		if (SQL_SUCCESS != (sret = SQLRowCount(hstmtTRACK,&nRowCount)))
		{
			PrintError(pSrvProc,"SQLRowCount:Failed calling TRACK");
			goto SAFE_EXIT;
		}
		
		if (0 != nRowCount )
		{
			//now look at the result set associated with hstmtTRACK
			//should have FIVE result columns for mode 1 (ME-outcome)
			//should have SIX result columns for mode 2 (NME-ODC), 

			//ExclusionSpace(ordinal 1, ulong),
			//PriorPermutation(ordinal 2, varbinary [binary seqeunce of ulongs]),
			//Permutation(ordinal3, varbinary [binary seqeunce of ulongs])
			//ActualEdge(ordinal 4, ulong) 
			//Duration(ordinal 5, ulong)
			// ordinal 6 only appears on mode 2
			//Disjoint Subset (ordinal 6, varbinary [binary seqeunce of ulongs])

			SQLNumResultCols(hstmtTRACK,&nNumCols);
			
			if(IO_COLS != nNumCols)
			{
				PrintError(pSrvProc,"Expected different number of return columns from GetME|NMETrack");
				goto SAFE_EXIT;
			}
			
			if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,1,SQL_C_ULONG,&lExclusionSpace,0,&cbExclusionSpace)))
			{
				PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on ExclusionSpace");
				goto SAFE_EXIT;
			}


			if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,2,SQL_C_BINARY,(BYTE*)lPriorPermBuf,sizeof(ULONG)*BUF_LEN,&cbPriorPerm)))
			{
				PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on PriorPermutation");
				goto SAFE_EXIT;
			}
			
			if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,3,SQL_C_BINARY,(BYTE*)lPermBuf,sizeof(ULONG)*BUF_LEN,&cbPerm)))
			{
				PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on Permutation");
				goto SAFE_EXIT;
			}
			
			if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,4,SQL_C_ULONG,&lActualEdge,0,&cbActualEdge)))
			{
				PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on ActualEdge");
				goto SAFE_EXIT;
			}

			if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,5,SQL_C_ULONG,&lDuration,0,&cbDuration)))
			{
				PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on Duration");
				goto SAFE_EXIT;
			}
			
			if  (NME_MODE == lModeID)
			{
				if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,6,SQL_C_BINARY,(BYTE*)lDJSBuf,sizeof(ULONG)*BUF_LEN,&cbDJS)))
				{
					PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on DisjointSubset");
					goto SAFE_EXIT;
				}
			}

			//zero out the buffers
			ZeroMemory(lPermBuf,sizeof(ULONG)*BUF_LEN);
			ZeroMemory(lPriorPermBuf,sizeof(ULONG)*BUF_LEN);
			ZeroMemory(lDJSBuf,sizeof(ULONG)*BUF_LEN);


			//push the members  
			while (SQL_NO_DATA != (sret = SQLFetch(hstmtTRACK)))
			{
				if (SQL_SUCCESS != sret)
				{
				 PrintError(pSrvProc,"SQLFetch: Failed calling SQLFetch on TRACK");
				 goto SAFE_EXIT;
				}

				if (SQL_NULL_DATA == cbPerm)
				{
					cbPerm = sizeof(ULONG);
					ZeroMemory(lPermBuf,sizeof(ULONG));
				}

				if(SQL_NULL_DATA == cbPriorPerm)
				{
					cbPriorPerm = sizeof(ULONG);
					ZeroMemory(lPriorPermBuf,sizeof(ULONG));
				}

				if (ME_MODE == lModeID)
				{
					if (!(pcAlgorithm->PushVertex (lExclusionSpace,lPriorPermBuf,cbPriorPerm/sizeof(ULONG),lPermBuf,cbPerm/sizeof(ULONG),lActualEdge,lDuration,lModeID)))
					{
					  PrintError(pSrvProc,"PushVertex: Failed attempting to push vertex");
					  goto SAFE_EXIT;
					}
				}
				else
				{
					if (SQL_NULL_DATA == cbDJS)
					{
						
						cbDJS = sizeof(ULONG);
						ZeroMemory(lDJSBuf,sizeof(ULONG));
					}
					if (!(pcAlgorithm->PushVertex (lDJSBuf,cbDJS/sizeof(ULONG),lExclusionSpace,lPriorPermBuf,cbPriorPerm/sizeof(ULONG),lPermBuf,cbPerm/sizeof(ULONG),lActualEdge,lDuration,lModeID))) 
					{                             
					  PrintError(pSrvProc,"PushVertex: Failed attempting to push vertex");
					  goto SAFE_EXIT;
					}
				}
			
				//zero out buffers
				ZeroMemory(lPermBuf,sizeof(ULONG)*BUF_LEN);
				ZeroMemory(lPriorPermBuf,sizeof(ULONG)*BUF_LEN);
				ZeroMemory(lDJSBuf,sizeof(ULONG)*BUF_LEN);
			
			}
		
		
		}
		SQLFreeStmt(hstmtTRACK,SQL_UNBIND);
		SQLFreeStmt(hstmtTRACK,SQL_CLOSE);
	/***********************************************************************************************/
	/*  GetDeadCycles and store in algorithm (no-parameter  stored procedure
	/***********************************************************************************************/

		sret = SQLExecDirect(hstmtTRACK,szCallGetDeadCycles,SQL_NTS);
		if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret)))
		{
			PrintError(pSrvProc,"SQLExecDirect:Failed calling GetDeadCycles");
			goto SAFE_EXIT;
		}


		//don't proceed unless TRACK rowcount is > 0.
		if (SQL_SUCCESS != (sret = SQLRowCount(hstmtTRACK,&nRowCount)))
		{
			PrintError(pSrvProc,"SQLRowCount:Failed calling TRACK");
			goto SAFE_EXIT;
		}
		
		//undocumented ODBC functionality. SQLGetRowCount returns 0 if no rows were returned
		//(empty result set)
		
		if (0 != nRowCount )
		{
		
			//1. DEADCYCLEPKID(ordinal 1, ULONG, not NULL),
			

			SQLNumResultCols(hstmtTRACK,&nNumCols);
			
			if(1 != nNumCols)
			{
				PrintError(pSrvProc,"Expected 1 return columns from GetDeadCycles");
				goto SAFE_EXIT;
			}
			
				if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,1,SQL_C_ULONG,&lDeadCyclePKID,0,&cbDeadCyclePKID)))
			{
				PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on DeadCyclePKID");
				goto SAFE_EXIT;
			}
			
			//now cycle through the GetDeadCycle Recordset
			while (SQL_NO_DATA != (sret = SQLFetch(hstmtTRACK)))//,FETCH_MODE,0)))
			{
				if (SQL_SUCCESS != sret)
				{
						
				 PrintError(pSrvProc,"SQLFetchScroll: Failed calling SQLFetchScroll on hstmtTRACK");
				 goto SAFE_EXIT;
				}

				if(!(pcAlgorithm->PushDeadCycle (lDeadCyclePKID)))
					{
					 PrintError(pSrvProc,"PushDeadCycle: Failed calling PushDeadCycle");
					 goto SAFE_EXIT;
					}
			}
		}
		SQLFreeStmt(hstmtTRACK,SQL_RESET_PARAMS);
		SQLFreeStmt(hstmtTRACK,SQL_UNBIND);
		SQLFreeStmt(hstmtTRACK,SQL_CLOSE);

	/***********************************************************************************************/
	/*  GetDeadEdgesAndAssignments, and store them in the algorithm class (one-parameter stored param)
	/***********************************************************************************************/
   
		if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lModeID,0,0)))
		{
			PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter ModeID");
			goto SAFE_EXIT;
		}

		
		sret = SQLExecDirect(hstmtTRACK,szCallGetDeadEdges,SQL_NTS);
		if(!( SQL_SUCCESS == sret || SQL_SUCCESS_WITH_INFO == sret || SQL_NO_DATA == sret ))
		{
		
			PrintError(pSrvProc,"SQLExecDirect:Failed calling GetDeadEdges");
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
		
		if (0 != nRowCount )
		{
		
			//1. DEADEDGEPKID(ordinal 1,ULONG, Not NULL),
			//2. ASSIGNED_TO (ordinal 2, ULONG, can be NULL)

			SQLNumResultCols(hstmtTRACK,&nNumCols);
			
			if(2 != nNumCols)
			{
				PrintError(pSrvProc,"Expected 2 return columns from GetDeadEdgesWAssignment");
				goto SAFE_EXIT;
			}
		
			if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,1,SQL_C_ULONG,&lDeadEdgePKID,0,&cbDeadEdgePKID)))
			{
				PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on DeadEdgePKID");
				goto SAFE_EXIT;
			}
			

			if(SQL_SUCCESS != (sret = SQLBindCol(hstmtTRACK,2,SQL_C_ULONG,&lAssignedToPKID,0,&cbAssignedToPKID)))
			{
				PrintError(pSrvProc,"SQLBindCol:Failed calling BindCol on AssignedToPKID");
				goto SAFE_EXIT;
			}
			

			//now cycle through the GetDeadCycle Recordset
		while (SQL_NO_DATA != (sret = SQLFetch(hstmtTRACK)))
			{
				if (SQL_SUCCESS != sret)
				{
				 PrintError(pSrvProc,"SQLFetchScroll: Failed calling SQLFetchScroll on hstmtTRACK");
				 goto SAFE_EXIT;
				}

				if(SQL_NULL_DATA == cbAssignedToPKID) {lAssignedToPKID = 0;}

				if(!(pcAlgorithm->PushDeadEdge (lDeadEdgePKID,lAssignedToPKID)))
					{
					 PrintError(pSrvProc,"PushDeadEdge: Failed calling PushDeadEdge");
					 goto SAFE_EXIT;
					}
			}

		}//		if (0 != nRowCount )

		SQLFreeStmt(hstmtTRACK,SQL_UNBIND);	
		SQLFreeStmt(hstmtTRACK,SQL_CLOSE);	

		if (!(pcAlgorithm->RingCycle()))
		{
		   PrintError(pSrvProc,"pcAlgorithm->RingCycle: Failed calling RingCycle");
		   goto SAFE_EXIT;
		}

		lNumMembers = 0;
		itrNME		= NULL;
		pcAlgorithm->GetIterator(itrNME,lNumMembers);

		if (lNumMembers > 0)
		{	for(long i=0;i<lNumMembers;i++)
			{vResults.push_back( *(itrNME+i));}
		}
		//destroy pcAlgorithm and loop

		if (0 != pcAlgorithm)
		{ 
		   delete pcAlgorithm;
		   pcAlgorithm = NULL;
		}

			
	}//	for (long lModeID = ME_MODE; lModeID <= NME_MODE;lModeID++)

//delete transform entries
	sret = SQLExecDirect(hstmtTRACK,szCallDeleteTransform,SQL_NTS);
	if(!( SQL_SUCCESS == sret || SQL_SUCCESS_WITH_INFO == sret || SQL_NO_DATA == sret ))
	{
	
		PrintError(pSrvProc,"SQLExecDirect:Failed calling delete Transform Probs");
		goto SAFE_EXIT;
	}

	SQLFreeStmt(hstmtTRACK,SQL_RESET_PARAMS);
	SQLFreeStmt(hstmtTRACK,SQL_CLOSE);


	lNumMembers = vResults.size();
	itrNME = vResults.begin();
	if(0 < lNumMembers)
	{
			for(long i=0;i<lNumMembers;i++)
			{
				lExclusionSpace		  = (itrNME + i)->GetExclusionSpace();
				lActualEdge			  = (itrNME + i)->GetActualEdge();
				lDuration			  = (itrNME + i)->GetDuration();
				lETID				  = (itrNME + i)->GetMode();

				cbPerm				  = sizeof(ULONG)*(itrNME+ i)->GetNumPerm();
				cbPriorPerm			  = sizeof(ULONG)*(itrNME+ i)->GetNumPriorPerm();
				cbDJS				  = sizeof(ULONG)*(itrNME+ i)->GetNumDJS();
				
				ZeroMemory(lPermBuf,	 BUF_LEN*sizeof(ULONG));
				ZeroMemory(lPriorPermBuf,BUF_LEN*sizeof(ULONG));
				ZeroMemory(lDJSBuf,		 BUF_LEN*sizeof(ULONG));
				
				(itrNME+i)->GetPerm(lPermBuf,BUF_LEN*sizeof(ULONG));
				(itrNME+i)->GetPriorPerm(lPriorPermBuf,BUF_LEN*sizeof(ULONG));
				(itrNME+i)->GetDJS(lDJSBuf,BUF_LEN*sizeof(ULONG));
			
				if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lExclusionSpace,0,0)))
				{
					PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 1");
					goto SAFE_EXIT;
				}

				if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,2,SQL_PARAM_INPUT,SQL_C_BINARY,SQL_VARBINARY,BUF_LEN*sizeof(ULONG),0,lPriorPermBuf,0,&cbPriorPerm)))
				{
					PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 2");
					goto SAFE_EXIT;
				}
				
				if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,3,SQL_PARAM_INPUT,SQL_C_BINARY,SQL_VARBINARY,BUF_LEN*sizeof(ULONG),0,lPermBuf,0,&cbPerm)))
				{
					PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 3");
					goto SAFE_EXIT;
				}
				
				if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,4,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lActualEdge,0,0)))
				{
					PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 4");
					goto SAFE_EXIT;
				}

				if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,5,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lDuration,0,0)))
				{
					PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 5");
					goto SAFE_EXIT;
				}


				if(ME_MODE == lETID) {cbDJS = SQL_NULL_DATA;}
				if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,6,SQL_PARAM_INPUT,SQL_C_BINARY,SQL_VARBINARY,BUF_LEN*sizeof(ULONG),0,lDJSBuf,0,&cbDJS)))
				{
					PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 6");
					goto SAFE_EXIT;
				}
			
				if(SQL_SUCCESS != (sret = SQLBindParameter(hstmtTRACK,7,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&lETID,0,0)))
				{
					PrintError(pSrvProc,"SQLBindParameter:Failed to bind parameter 7");
					goto SAFE_EXIT;
				}

				sret = SQLExecDirect(hstmtTRACK,szCallInsertTransform,SQL_NTS);
				if(!((SQL_SUCCESS == sret) || (SQL_SUCCESS_WITH_INFO == sret)))
				{
					PrintError(pSrvProc,"SQLExecDirect:Failed calling InsertTransform");
					goto SAFE_EXIT;
				}
		
				//unbind the Parameter from last call to SQLBindParameter
				SQLFreeStmt(hstmtTRACK,SQL_RESET_PARAMS);
				SQLFreeStmt(hstmtTRACK,SQL_CLOSE);
				
			}//for(i=0;i<lNumMembers;i++)

	}//if(0 < lNumMembers)

	rcXP = XP_NOERROR;
SAFE_EXIT:
	//free data buffers
	if (0 != pcAlgorithm)
	{ 
	   delete pcAlgorithm;
	   pcAlgorithm = NULL;
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
	srv_sendmsg(pSrvProc,SRV_MSG_ERROR,XP_TRANSFORMHANDLER_ERROR,16,1,NULL,0,(DBUSMALLINT) __LINE__,
				"Usage: exec xp_TransformHandler [No Parameters]",SRV_NULLTERM);
	srv_senddone(pSrvProc,(SRV_DONE_ERROR|SRV_DONE_MORE),0,0);
}
void PrintError(SRV_PROC* pSrvProc,char *szErrorMsg)
{
	srv_sendmsg(pSrvProc,SRV_MSG_ERROR,XP_TRANSFORMHANDLER_ERROR,16,1,NULL,0,(DBUSMALLINT) __LINE__,
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

//end spare code
*/
