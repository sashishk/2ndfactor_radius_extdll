// dllmain.cpp : Defines the initialization routines for the DLL.
//
#pragma comment(lib, "Winhttp.lib")
#include "stdafx.h"
#include <afxwin.h>
#include <afxdllx.h>
#include "authif.h"
#include <winhttp.h>
#include <ios>
#include <vector>
#include <intrin.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

vector<string> connect2Internet(CString radiusUser, CString radiusPwd);
CString char2CString(const char * strChar);
PRADIUS_ATTRIBUTE responseToClient(string str);

static AFX_EXTENSION_MODULE SecureIASExtDLL = { NULL, NULL };

FILE *fp;

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("SecureIASExt.DLL Initializing!\n");
		
		// Extension DLL one-time initialization
		if (!AfxInitExtensionModule(SecureIASExtDLL, hInstance))
			//return 0;

		// Insert this DLL into the resource chain
		// NOTE: If this Extension DLL is being implicitly linked to by
		//  an MFC Regular DLL (such as an ActiveX Control)
		//  instead of an MFC application, then you will want to
		//  remove this line from DllMain and put it in a separate
		//  function exported from this Extension DLL.  The Regular DLL
		//  that uses this Extension DLL should then explicitly call that
		//  function to initialize this Extension DLL.  Otherwise,
		//  the CDynLinkLibrary object will not be attached to the
		//  Regular DLL's resource chain, and serious problems will
		//  result.

		new CDynLinkLibrary(SecureIASExtDLL);

	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("SecureIASExt.DLL Terminating!\n");

		// Terminate the library before destructors are called
		AfxTermExtensionModule(SecureIASExtDLL);
	}
	return 1;   // ok
}

DWORD WINAPI RadiusExtensionInit( VOID )
{
  return NO_ERROR;
}

VOID WINAPI RadiusExtensionTerm( VOID )
{
  return;
}

VOID WINAPI RadiusExtensionFreeAttributes(PRADIUS_ATTRIBUTE pAttrs)
{
	for (int i = 0; pAttrs[i].dwAttrType != ratMinimum; i++)
		if (pAttrs[i].fDataType == rdtString)
			delete[] (char*) pAttrs[i].lpValue;

	delete[] pAttrs;
}

DWORD WINAPI RadiusExtensionProcessEx (
			  // pointer to array of input attributes
			  CONST RADIUS_ATTRIBUTE  * pInAttrs,
			  // pointer to array of output attributes
			  PRADIUS_ATTRIBUTE        * pOutAttrs,
			  // action that IAS should take
			  PRADIUS_ACTION            pfAction
			  )
{
	CString strUser;
	CString strPwd;
	
	for (int i = 0; pInAttrs[i].dwAttrType != ratMinimum; i++)
    {
		if (pInAttrs[i].dwAttrType == ratUserName)
		{
			if (pInAttrs[i].fDataType == rdtString)
			{
				char *tempUser = new char[pInAttrs[i].cbDataLength+1];
				memset(tempUser, '\0', pInAttrs[i].cbDataLength+1);
				memcpy(tempUser, pInAttrs[i].lpValue, pInAttrs[i].cbDataLength);
				CString temp(tempUser);
				delete[] tempUser;
				strUser = temp;
			}
		}
		if (pInAttrs[i].dwAttrType == ratUserPassword)
		{
			if (pInAttrs[i].fDataType == rdtString)
			{
				char *tempPwd = new char[pInAttrs[i].cbDataLength+1];
				memset(tempPwd, '\0', pInAttrs[i].cbDataLength+1);
				memcpy(tempPwd, pInAttrs[i].lpValue, pInAttrs[i].cbDataLength);
				CString temp(tempPwd);
				delete[] tempPwd;
				strPwd = temp;
			}
		}
    }
	
	try
	{
		vector<string> response = connect2Internet(strUser, strPwd);
		string str;
		
		/*fp=fopen("e:\\test\\log.txt","w+");
		if(fp)
			fwrite("\nResponse:", 1,10, fp);*/

		str = response.at(0);
		if(strlen(str.c_str())>0)
		{
			//fwrite("\nlength: ", 1,8, fp);
			//fwrite(str.c_str(), 1,str.size(), fp);
			char *s = new char[strlen(str.c_str())+1];
			//strcpy(s, str.c_str());
			memset(s, '\0', strlen(str.c_str())+1);
			memcpy(s, str.c_str(), strlen(str.c_str()));

			//fwrite("\nmodified: ", 1,11, fp);
			//fwrite(s, 1,strlen(str.c_str()), fp);
			if(strncmp(s, "SUCCESS", 4)==0)
			{
				*pfAction = (RADIUS_ACTION)raAccept;
				//fwrite("\nif", 1,3, fp);
			}
			else
			{
				//*pOutAttrs = responseToClient(str.substr(9, str.size()-10));
				*pfAction = (RADIUS_ACTION)raReject;
				//fwrite("\nelse", 1,5, fp);
			}
			delete[] s;
		}

		//fwrite("\nbeforeclose", 1,12, fp);
		//fclose(fp);
	}
	catch(...)
	{
		/*if(fp)
			fwrite("\nexception", 1,16, fp);*/
	}

	return NO_ERROR;
}

PRADIUS_ATTRIBUTE responseToClient(string str)
{
	PRADIUS_ATTRIBUTE pReturns = new RADIUS_ATTRIBUTE[1+1];
	::ZeroMemory(pReturns,sizeof(RADIUS_ATTRIBUTE)*(1+1));
	pReturns[1].dwAttrType = ratMinimum;

	fwrite("\nsubstr: ", 1,8, fp);
	fwrite(str.c_str(), 1,strlen(str.c_str()), fp);

	pReturns[0].lpValue = (const BYTE *)str.c_str();
	pReturns[0].dwAttrType = ratReplyMessage;
	pReturns[0].cbDataLength = strlen(str.c_str());
	pReturns[0].fDataType = rdtString;
	
	return pReturns;
}

CString char2CString(const char * strChar)
{
	USES_CONVERSION;
	CString str;
	if(str)
		str = A2T(strChar);
	
	return str;
}

//http://remote-001/ready/RadiusServlet?username=TstUsr&password=TstPwd

vector<string> connect2Internet(CString strUser, CString strPwd)
{
	//Variables 
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	LPSTR pszOutBuffer;
	vector <string>  vFileContent;
	BOOL  bResults = FALSE;
	HINTERNET  hSession = NULL, 
			   hConnect = NULL,
			   hRequest = NULL;

	try
	{
		// Use WinHttpOpen to obtain a session handle.
		hSession = WinHttpOpen( L"WinHTTP Example/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, 
								WINHTTP_NO_PROXY_BYPASS, 0);

		//Read registry parameter for server and port
		//Server - HKEY_LOCAL_MACHINE\Software\Secure\Server (String value)
		//Port - HKEY_LOCAL_MACHINE\Software\Secure\Server (DWORD value)
		/*HKEY hKey;
		LONG result = 0;
		char *server = new char[1024];
		DWORD port = 80;
		memset(server, '\0', 1024);

		DWORD dwBufLen = sizeof(server);

		result = RegOpenKeyEx( HKEY_LOCAL_MACHINE,L"SOFTWARE\\Secure", 0, KEY_QUERY_VALUE, &hKey );
		fp=fopen("e:\\test\\log.txt","w+");
			fwrite("\nregistry:", 1,9, fp);
		if(result == ERROR_SUCCESS)
		{
			fwrite("\nin registry:", 1,12, fp);
			result = RegQueryValueEx(hKey, L"Server", NULL, NULL, (LPBYTE)server, &dwBufLen);
			fwrite("\nserver:", 1,6, fp);
			fwrite(server, 1,strlen(server), fp);
			dwBufLen = sizeof(DWORD);
			result = RegQueryValueEx(hKey, L"Port", NULL, NULL, (LPBYTE)&port, &dwBufLen);
			fwrite("\nport:", 1,5, fp);
			fwrite((void*)port, 1,2, fp);
		}
		fclose(fp);
		RegCloseKey(hKey);*/

		// Specify an HTTP server.
		if (hSession)
			hConnect = WinHttpConnect( hSession, L"remote-001", 80, 0);
			//hConnect = WinHttpConnect( hSession, (LPCWSTR)server, port, 0);

		// Create an HTTP request handle.
		if (hConnect)
		{
			CString strTemp("/ready/RadiusServlet?username=" );

			strTemp.Append(strUser);
			strTemp.Append(char2CString("&password="));
			strTemp.Append(strPwd);
			CString temp = char2CString("\nRequestString : ") + strTemp;
			hRequest = WinHttpOpenRequest( hConnect, L"GET", strTemp, NULL, WINHTTP_NO_REFERER, NULL, NULL);
		}

		// Send a request.
		if (hRequest)
		{
			bResults = WinHttpSendRequest( hRequest,
							WINHTTP_NO_ADDITIONAL_HEADERS,
							0, WINHTTP_NO_REQUEST_DATA, 0, 
							0, 0);
		}

		// End the request.
		if (bResults)
			bResults = WinHttpReceiveResponse( hRequest, NULL);

		// Keep checking for data until there is nothing left.
		if (bResults)
		{
			vFileContent.clear();
			do 
			{
				// Check for available data.
				dwSize = 0;
				if (!WinHttpQueryDataAvailable( hRequest, &dwSize))
				{
					//printf( "Error %u in WinHttpQueryDataAvailable.\n",
					//GetLastError());
				}

				// Allocate space for the buffer.
				pszOutBuffer = new char[dwSize+1];
				if (!pszOutBuffer)
					dwSize=0;
				else
				{
					// Read the Data.
					ZeroMemory(pszOutBuffer, dwSize+1);
					if (!WinHttpReadData( hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded))
					{
						//printf( "Error %u in WinHttpReadData.\n", GetLastError());
					}
					else
					{
						// Data in vFileContent
						vFileContent.push_back(pszOutBuffer);
					}

					// Free the memory allocated to the buffer.
					delete [] pszOutBuffer;
				}
			} while (dwSize>0);
		}
		
		// Close any open handles.
		if (hRequest) WinHttpCloseHandle(hRequest);
		if (hConnect) WinHttpCloseHandle(hConnect);
		if (hSession) WinHttpCloseHandle(hSession);
	}
	catch(...)
	{
		if (hRequest) WinHttpCloseHandle(hRequest);
		if (hConnect) WinHttpCloseHandle(hConnect);
		if (hSession) WinHttpCloseHandle(hSession);
	}
	return vFileContent;
}

