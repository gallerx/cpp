/*
Console app that monitors a directory for changes, then FTP's any new or changed files to offsite server.
Very useful for offsite backup of an RDBMS
*/


#define _WIN32_WINNT 0x0400			//arbitrarily need this for ReadDirectoryChangesW
#include <stdio.h>
#include <windows.h>
#include <wininet.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>


//forward declarations
VOID CALLBACK MyStatusCallback(HINTERNET hInternet, 
							   DWORD dwContext, 
							   DWORD dwInternetStatus, 
							   LPVOID lpvStatusInformation,
							   DWORD dwStatusInformationLength);
void MonitorDirectory(HANDLE,LPTSTR);
void TransferFile(LPTSTR, LPTSTR );


//expects three command line arguments, 
	//executable as argv[0], 
	//directory to monitor as argv[1], 
	//and FTP server IP address for transfers as argv[2]
//usage "FTPMON.exe d:\SQLBACKUP xx.xxx.xxx.xx"

void _tmain(int argc, TCHAR* argv[])
{

	
	if(argc != 3)
	{ _tprintf(TEXT("Usage %s <dir> <ftp-server-ip>\n"),argv[0]);
	   return;
	}
	
	printf("\nStarting Directory Watch and FTP Transfer\n\n");

	printf("\nMonitoring (%s) directory\n", argv[1]);
	printf("\nFiles will be transfered to (%s) server\n", argv[2]);

	//to obtain a handle to an existing directory, call CreateFile function with FILE_FLAG_BACKUP_SEMANTICS flag
	HANDLE hDir = CreateFile(argv[1], 
							GENERIC_READ,
							FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING,
							FILE_FLAG_BACKUP_SEMANTICS,
							NULL);

	if(INVALID_HANDLE_VALUE == hDir)
	{

		printf("\n ERROR: CreateFile function failed. Possibly invalid directory. \n");
		ExitProcess(GetLastError());

	}

	//assume current directory ok if we've gotten here
	SetCurrentDirectory(argv[1]);


 
	while(TRUE)
	{MonitorDirectory(hDir,argv[2]);}

	CloseHandle(hDir);

}
	


void MonitorDirectory(HANDLE hDir,LPTSTR lpServer)
{

	time_t rawtime;


	TCHAR szBuffer[640] = {0};	
	DWORD dwOffset = 0;
	FILE_NOTIFY_INFORMATION* pInfo = NULL;
	DWORD dwbytes =0;
	
	BOOL retval;

	//synchronous (blocking) function. Will return when a monitored change has occured
	retval = ReadDirectoryChangesW(hDir,
						  szBuffer,
						  sizeof(szBuffer)/sizeof(TCHAR),
						  FALSE,
						  FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_SIZE,
						  &dwbytes,
						  NULL,
						  NULL);

	if (0 == retval)
	{
		printf("\n ERROR: ReadDirectoryChanges function failed. \n");
		return;
	}

	do
	{
	  pInfo = (FILE_NOTIFY_INFORMATION*) &szBuffer[dwOffset];
	  TCHAR szFileName[MAX_PATH] = {0};
	  WideCharToMultiByte(CP_ACP,NULL,pInfo->FileName,pInfo->FileNameLength,szFileName,sizeof(szFileName)/sizeof(TCHAR),NULL,NULL);

	  
	  if(FILE_ACTION_ADDED == pInfo->Action || FILE_ACTION_MODIFIED == pInfo->Action)
	  {
		time(&rawtime);
	  
	  _tprintf(TEXT("Filename (%s) changed at time %s.\n"), szFileName, ctime(&rawtime));
		TransferFile(szFileName,lpServer);
	  }

	  dwOffset += pInfo->NextEntryOffset;		
	} while (pInfo->NextEntryOffset != 0);


}

//ensure you set up WRITE and DELETE permission for user ref'd by "myuser"
void TransferFile(LPTSTR lpFile, LPTSTR lpServer)
{
	HINTERNET hSession = InternetOpen("BL_FTPCLIENT/1.0", INTERNET_OPEN_TYPE_DIRECT,0,0,0);
	InternetSetStatusCallback(hSession,&MyStatusCallback);
		
	HINTERNET hConnect = InternetConnect(hSession,
										lpServer,
										INTERNET_DEFAULT_FTP_PORT,
										"myuser",
										"mypassword",
										INTERNET_SERVICE_FTP,0,1);
	
	
	FtpPutFile(hConnect,
			   lpFile,
			   lpFile,
			   FTP_TRANSFER_TYPE_BINARY,
			   0);



	InternetCloseHandle(hConnect);
	InternetCloseHandle(hSession);


 	return;
}



VOID CALLBACK MyStatusCallback(HINTERNET hInternet, 
							   DWORD dwContext, 
							   DWORD dwInternetStatus, 
							   LPVOID lpvStatusInformation,
							   DWORD dwStatusInformationLength)
{
	LPCSTR szStatus;	
	
	switch(dwInternetStatus)
	{

	case INTERNET_STATUS_RESOLVING_NAME:       
		szStatus = "Resolving name...";
		break;
	case INTERNET_STATUS_NAME_RESOLVED:        
		szStatus = "Name resolved";
		break;
	case INTERNET_STATUS_CONNECTING_TO_SERVER:
		szStatus = "Connecting to server...";
		break;
	case INTERNET_STATUS_CONNECTED_TO_SERVER:  
		szStatus = "Connected to server";
		break;
	case INTERNET_STATUS_SENDING_REQUEST:      
		szStatus = "Sending request...";
		break;
	case INTERNET_STATUS_REQUEST_SENT:         
		szStatus = "Request sent";
		break;
	case INTERNET_STATUS_RECEIVING_RESPONSE:   
		szStatus = "Receiving response...";
		break;
	case INTERNET_STATUS_RESPONSE_RECEIVED:    
		szStatus = "Response received";
		break;
	case INTERNET_STATUS_CTL_RESPONSE_RECEIVED:
		szStatus = "Control response received";
		break;
	case INTERNET_STATUS_PREFETCH:             
		szStatus = "Prefetch...";
		break;
	case INTERNET_STATUS_CLOSING_CONNECTION:   
		szStatus = "Closing connection...";
		break;
	case INTERNET_STATUS_CONNECTION_CLOSED:    
		szStatus = "Connection closed";
		break;
	case INTERNET_STATUS_HANDLE_CREATED:       
		szStatus = "Handle created";
		break;
	case INTERNET_STATUS_HANDLE_CLOSING:       
		szStatus = "Handle closing...";
		break;
	case INTERNET_STATUS_REQUEST_COMPLETE:     		
		szStatus = "Request complete";
		break;
	case INTERNET_STATUS_REDIRECT:             
		szStatus = "Redirect";
		break;
	case INTERNET_STATUS_INTERMEDIATE_RESPONSE:
		szStatus = "Received intermediate status code";
		break;
	case INTERNET_STATUS_STATE_CHANGE:         
		szStatus = "Online/Offline state change";
		break;
	default:
		szStatus = "Unknown status";

	}
	printf("\n");
	printf(szStatus);
	printf("\n");
	
}

