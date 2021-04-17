// Global.cpp : Defines utility and global functions
//

#include "stdafx.h"

BOOL isPasswordCheck(CONST RADIUS_ATTRIBUTE *pInAttrs)
{
	while (pInAttrs->dwAttrType != ratMinimum)
		{
			if (pInAttrs->dwAttrType == ratCode)
				return (pInAttrs->dwValue == 1);  // 1 is Access-Request
			pInAttrs++;
		}
	return false;
}

BOOL CheckPassword(BSTR realPassword,CONST RADIUS_ATTRIBUTE *pAttrs)
{
  // This routine implements PAP.  It will probably need to
  // implement CHAP at some point, although I'm unclear about how...
  
  _variant_t vPass;
  if (g_settings->variantFromRadiusAttributes(ratUserPassword,pAttrs,vPass))
    {
      // Simple password check
      return wcscmp(realPassword,V_BSTR(&vPass)) ? false : true;
    }
	else
		{
			// CHAP request???
			// Create MD5 hash of {first byte of chap password}{password}{ratAuthenticator}
			USES_CONVERSION;
			_variant_t vAuth;
			unsigned char hash[16];
			LPSTR szPassword = OLE2A(realPassword);

			// Find the Chap Password and Authenticator manually
			LPCSTR szChapPass=NULL,szAuth=NULL;
			DWORD dwChapPass=0,dwAuth=0;
			while (pAttrs->dwAttrType != ratMinimum)
				{
					if (pAttrs->dwAttrType==ratCHAPPassword)
						{
							szChapPass = pAttrs->lpValue;
							dwChapPass = pAttrs->cbDataLength;
						}
					if (pAttrs->dwAttrType==ratAuthenticator)
						{
							szAuth = pAttrs->lpValue;
							dwAuth = pAttrs->cbDataLength;
						}
					pAttrs++;
				}

			// If we can't find either, we can't do anything
			if (!szChapPass || !szAuth)
				return false;

			MD5_CTX mdContext;
			MD5Init(&mdContext);
			MD5Update(&mdContext,(unsigned char*)szChapPass,1);
			MD5Update(&mdContext,(unsigned char*)szPassword,strlen(szPassword));
			MD5Update(&mdContext,(unsigned char*)szAuth,dwAuth);
			MD5Final(hash,&mdContext);

			if (memcmp(szChapPass+1,hash,16))
				return false;
			else
				return true; // matches, you're in.
		}
	return false;
}

void LogAttributes(CONST RADIUS_ATTRIBUTE *pInAttrs)
{
  TCHAR   szLogEntry[10240];
  LPCTSTR aIns[2];
  aIns[0] = szLogEntry;
  aIns[1] = NULL;
  
  strcpy(szLogEntry,_T("\n\t"));

	try
		{  
			for (int i = 0; pInAttrs[i].dwAttrType != ratMinimum; i++)
				{
					_variant_t vOut;
					_bstr_t bName = g_settings->getAttributeName(pInAttrs[i].dwAttrType);
					_tcscat(szLogEntry,bName);
					_tcscat(szLogEntry,_T(":"));
					if (g_settings->variantFromRadiusAttributes(pInAttrs[i].dwAttrType,&(pInAttrs[i]),vOut))
						{
							_bstr_t bOut(vOut);
							_tcscat(szLogEntry,(LPTSTR)bOut);
							_tcscat(szLogEntry,_T("\n\t"));
						}
					else
						{
							_tcscat(szLogEntry,_T("****\n\t"));
						}
				}
		}
	catch (...)
		{
		}
  
  REPORT_INFO(EVMSG_HITLOG, 1, aIns);
}

