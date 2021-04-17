// RadiusDB.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

HANDLE hEventSource = NULL;
DWORD  _tlsDBInterface = 0L;
SettingsManager *g_settings = NULL;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
#ifdef DEBUG
  int tmpFlag;
  char *leakBuf;
#endif

  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
    
#ifdef DEBUG
    tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
    tmpFlag |= _CRTDBG_CHECK_ALWAYS_DF;
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag( tmpFlag );
    leakBuf = (char*) malloc(128);
    strcpy(leakBuf,"Leak check working. Not a leak.");
#endif
    
    hEventSource = RegisterEventSource(NULL,_T("RadiusDB"));
    if (hEventSource == NULL)
      return FALSE;
    _tlsDBInterface = TlsAlloc();
    if (_tlsDBInterface == -1)
      return FALSE;
    break;
  case DLL_THREAD_ATTACH:
    break;
  case DLL_PROCESS_DETACH:
    if (hEventSource) DeregisterEventSource(hEventSource);
    {
      DBInterface *dbI = (DBInterface*) TlsGetValue(_tlsDBInterface);
      if (dbI)
	delete dbI;
    }
    if (_tlsDBInterface != -1)
      TlsFree(_tlsDBInterface);
    if (g_settings) delete g_settings;
    break;
  case DLL_THREAD_DETACH:
    {
      DBInterface *dbI = (DBInterface*) TlsGetValue(_tlsDBInterface);
      if (dbI)
	delete dbI;
    }
    break;
  }
  return TRUE;
}

DWORD WINAPI RadiusExtensionInit( VOID )
{
  // Load up all settings
  try
    {
      g_settings = new SettingsManager();
      
      // Test the db configuration
      DBInterface dbI(*g_settings);
    }
  catch (LPTSTR str)
    {
      LPCTSTR aIns[] = { str,NULL };
      REPORT_ERR(EVMSG_BADCONFIG,1,aIns);
      return ERROR_BADKEY;
    }
  catch (_com_error &pCE)
    {
      _bstr_t s=pCE.Source(), d=pCE.Description(), e=pCE.ErrorMessage();
      LPCTSTR aIns[] = { s,d,e,NULL };
      REPORT_ERR(EVMSG_COMERROR, 3, aIns);
      return ERROR_GEN_FAILURE;
    }
  catch (MSXML::IXMLDOMParseErrorPtr &err)
    {
      _bstr_t reason = err->reason, srcText = err->srcText;
      DWORD eCode = err->errorCode;
      LPCTSTR aIns[] = { reason, srcText, NULL };
      REPORT_ERR(EVMSG_XMLBAD, 2, aIns);
      return ERROR_BADKEY;
    }
  
  REPORT_INFO(EVMSG_STARTUP,0,NULL);
  return NO_ERROR;
}

VOID WINAPI RadiusExtensionTerm( VOID )
{
  REPORT_INFO(EVMSG_SHUTDOWN,0,NULL);
  return;
}

VOID WINAPI RadiusExtensionFreeAttributes(PRADIUS_ATTRIBUTE pAttrs)
{
  for (int i = 0; pAttrs[i].dwAttrType != ratMinimum; i++)
    {
      if (pAttrs[i].fDataType == rdtString)
	    delete[] (char*) pAttrs[i].lpValue;
    }
  delete[] pAttrs;
}

DWORD WINAPI 
RadiusExtensionProcessEx (
			  // pointer to array of input attributes
			  CONST RADIUS_ATTRIBUTE  * pInAttrs,
			  // pointer to array of output attributes
			  PRADIUS_ATTRIBUTE        * pOutAttrs,
			  // action that IAS should take
			  PRADIUS_ACTION            pfAction
			  )
{
  try
    {
#ifdef DEBUG
      LogAttributes(pInAttrs);
#endif
      
      // Create the thread-local DB Interface object if not done
      DBInterface *dbI = (DBInterface*) TlsGetValue(_tlsDBInterface);
      if (!dbI)
	{
#ifdef DEBUG
	  REPORT_INFO(EVMSG_TLSINIT,0,NULL);
#endif
	  dbI = new DBInterface(*g_settings);
	  TlsSetValue(_tlsDBInterface,dbI);
	}
      
      // Only try and auth if someone is going to listen to our answer
      if (pfAction && isPasswordCheck(pInAttrs))
	{
	  // The general behavior here is:
	  //   If a GetPassword routine is defined, get it and check it
	  //   If not, just call the LoginRequest routine and use the
	  //     password from there
	  ADODB::_RecordsetPtr recSet;
	  BSTR DBPassword;
	  if (g_settings->HasSeparatePasswordStep())
	    {
	      DBPassword = dbI->GetPassword(pInAttrs);
	    }
	  else
	    {
	      recSet = dbI->LoginRequest(pInAttrs,&DBPassword);
	    }
	  if (!DBPassword || !CheckPassword(DBPassword,pInAttrs))
	    {
#ifdef DEBUG
	      REPORT_INFO(EVMSG_REJECTED,0,NULL);
#endif
	      pOutAttrs = NULL;
	      *pfAction = raReject;
	      return NO_ERROR;
	    }

	  *pfAction = raAccept;
	  // If we're here, we have a valid password.
	  // If the record set is NULL, we didn't call LoginRequest
	  if (recSet == NULL)
	    {
				// We re-request DBPassword because if it comes back null
				// we can refuse access.  We also DO NOT set it back to
				// null, that way if LoginRequest function doesn't HAVE
				// a password return variable, it'll stay untouched
	      recSet = dbI->LoginRequest(pInAttrs,&DBPassword);
	    }

		if (!DBPassword)
			{
				pOutAttrs = NULL;
				*pfAction = raReject;
#ifdef DEBUG
	      REPORT_INFO(EVMSG_REJECTED,0,NULL);
#endif
				return NO_ERROR;
			}

	  // Now fill the out attributes with the returned values
	  int attCount = 0;
    try
    {
      attCount = recSet->RecordCount;
    }
    catch (_com_error &pCE)
			{
				if (pCE.Error() & 0xFFFF != ADODB::adErrObjectClosed)
					throw pCE;
			}

	  if (attCount == 0)
	    {
#ifdef DEBUG
	  REPORT_INFO(EVMSG_ACCEPTED,0,NULL);
#endif
	      pOutAttrs = NULL;
	      return NO_ERROR;
	    }
	  // *** Now write the attributes returned to the output.
	  // *** This isn't the prettiest, because the field order is
	  // *** required, but I don't have the patience to design
	  // *** something better, and you realistically can acheive
	  // *** all this flexibility in the db
	  RADIUS_ATTRIBUTE *pReturns = new RADIUS_ATTRIBUTE[attCount+1];
	  
    ::ZeroMemory(pReturns,sizeof(RADIUS_ATTRIBUTE)*(attCount+1));
    pReturns[attCount].dwAttrType = ratMinimum;
    int i = 0;
    while (!recSet->adoEOF)
    {
      if (g_settings->radiusAttributeFromVariant(&(pReturns[i]),recSet) == FALSE)
      {
        // Something went wrong, bail
        REPORT_ERR(EVMSG_ATTRETURNFAIL,0,NULL);
        RadiusExtensionFreeAttributes(pReturns);
        pReturns = NULL;
        break;
      }
      recSet->MoveNext();
      i++;
    }

    *pOutAttrs = pReturns;
#ifdef DEBUG
	  REPORT_INFO(EVMSG_ACCEPTED,0,NULL);
		LogAttributes(pReturns);
#endif
	  return NO_ERROR;
	}
      else  // This must be an accounting request...
	{
#ifdef DEBUG
	  REPORT_INFO(EVMSG_ACCT,0,NULL);
#endif
		// No need to fill in pfAction
		if (g_settings->HasAccoutingStep())
			dbI->AccountingRequest(pInAttrs);
	  return NO_ERROR;
	}
    }
  catch (_com_error &pCE)
    {
      _bstr_t s=pCE.Source(), d=pCE.Description(), e=pCE.ErrorMessage();
      LPCTSTR aIns[] = { s,d,e,NULL };
      REPORT_ERR(EVMSG_COMERROR, 3, aIns);
			*pOutAttrs = NULL;
			*pfAction = raReject;
      return NO_ERROR;
    }
  catch (...)
    {
      REPORT_ERR(EVMSG_EXCEPTION,0,NULL);
			*pOutAttrs = NULL;
			*pfAction = raReject;
      return NO_ERROR;
    }
}

BSTR WINAPI RadiusDBTestAuthentication()
{
  RADIUS_ATTRIBUTE pArr[5];
  pArr[0].dwAttrType = ratUserName;
  pArr[0].fDataType = rdtString;
  pArr[0].cbDataLength = 8;
  pArr[0].lpValue = "testuser";
  pArr[1].dwAttrType = ratUserPassword;
  pArr[1].fDataType = rdtString;
  pArr[1].cbDataLength = 8;
  pArr[1].lpValue = "password";
  pArr[2].dwAttrType = ratNASIPAddress;
  pArr[2].fDataType = rdtAddress;
  pArr[2].cbDataLength = sizeof(DWORD);
  pArr[2].dwValue = 10 << 24 | 100 << 16 | 1 << 8 | 20;
  pArr[3].dwAttrType = ratCode;
  pArr[3].fDataType = rdtInteger;
  pArr[3].cbDataLength = sizeof(DWORD);
  pArr[3].dwValue = 1;
  pArr[4].dwAttrType = ratMinimum;

  try
  {
    CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);
    RadiusExtensionInit();
    RADIUS_ACTION radAct;
    RADIUS_ATTRIBUTE *pOut = NULL;
    RadiusExtensionProcessEx(pArr,&pOut,&radAct);
    if (pOut) RadiusExtensionFreeAttributes(pOut);
    RadiusExtensionProcessEx(pArr,&pOut,&radAct);
    if (pOut) RadiusExtensionFreeAttributes(pOut);
    CoUninitialize();
  }
  catch (_com_error &pCE)
    {
      _bstr_t s=pCE.Source(), d=pCE.Description(), e=pCE.ErrorMessage();
      LPCTSTR aIns[] = { s,d,e,NULL };
      REPORT_ERR(EVMSG_COMERROR, 3, aIns);
      return NULL;
    }
  return NULL;
}


