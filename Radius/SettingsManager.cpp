// SettingsManager.cpp: implementation of the SettingsManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SettingsManager::SettingsManager()
{
  TCHAR regBuf[10240];
  DWORD dwSize=1024;

	CRegKey rKey;
  if (rKey.Open(HKEY_LOCAL_MACHINE,RDB_REG,KEY_READ) != ERROR_SUCCESS)
    throw MISSING_SECTION;
  if (rKey.QueryValue(regBuf,RDB_XML,&dwSize) != ERROR_SUCCESS)
  {
    throw MISSING_XML;
    rKey.Close();
  }
  rKey.Close();

  // *** Load and parse XML Configuration
  m_xmlConfiguration.CreateInstance("Microsoft.FreeThreadedXMLDOM");
  m_xmlConfiguration->async = false;
  m_xmlConfiguration->load(regBuf);
  MSXML::IXMLDOMParseErrorPtr error = m_xmlConfiguration->parseError;
  if (error != NULL && error->errorCode != 0)
  {
    throw error;
  }

  // *** Find out if there is a LoginRequest function
  WCHAR ptrn[256];
  swprintf(ptrn,CFG_PROCINFO_PATTERN,PWDREQ_COMMAND);
  if (m_xmlConfiguration->selectSingleNode(ptrn) != NULL)
    m_hasPWStep = true;
  else
    m_hasPWStep = false;
  swprintf(ptrn,CFG_PROCINFO_PATTERN,ACCT_COMMAND);
  if (m_xmlConfiguration->selectSingleNode(ptrn) != NULL)
    m_accountingStep = true;
  else
    m_accountingStep = false;

  // *** Read in all radius attribute types
  // *** First find max attribute number
  MSXML::IXMLDOMNodePtr node;
  MSXML::IXMLDOMNodeListPtr nodeList = 
    m_xmlConfiguration->selectSingleNode(CFG_ATTR_NODE)->childNodes;
  long maxNum = 0, i = 0, len = nodeList->length;
  for ( ; i < len; i++)
  {
    node = nodeList->item[i];
    _variant_t attrNumber = 
      node->attributes->getNamedItem(CFG_ATTR_NUMBER)->text;
    if ((long)attrNumber > maxNum)
      maxNum = (long) attrNumber;
  }

  // *** Now set them all up with info
  m_attributeTypes = new DataTypeEnum[maxNum+1];
  m_attributes = maxNum+1;
  for (i=0 ; i < len; i++)
  {
    node = nodeList->item[i];
    _variant_t attrNumber = 
      node->attributes->getNamedItem(CFG_ATTR_NUMBER)->text;
    _bstr_t attrType(node->attributes->getNamedItem(CFG_ATTR_TYPE)->text);
    if (!wcscmp(attrType,CFG_STR_TYPE))
      m_attributeTypes[(long)attrNumber] = dataTypeString;
    else if (!wcscmp(attrType, CFG_IP_TYPE))
      m_attributeTypes[(long)attrNumber] = dataTypeIP;
    else if (!wcscmp(attrType, CFG_LONG_TYPE))
      m_attributeTypes[(long)attrNumber] = dataTypeLong;
		else if (!wcscmp(attrType, CFG_BINARY_TYPE))
			m_attributeTypes[(long)attrNumber] = dataTypeBinary;
  }

}

SettingsManager::~SettingsManager()
{
  if (m_attributeTypes) delete[] m_attributeTypes;
}

SettingsManager::CommandParameter* 
SettingsManager::FillCommand(ADODB::_CommandPtr Rcmd, LPWSTR procName, _bstr_t &connString)
{
  // *** Fill the command object with all the right parameter objects.
  // *** First find the parameter nodes using XSL pattern
  WCHAR buffer[1024];
  swprintf(buffer, CFG_PROCINFO_PATTERN, procName);
  MSXML::IXMLDOMNodePtr node = m_xmlConfiguration->selectSingleNode(buffer);
  Rcmd->CommandTimeout = 10;
  Rcmd->CommandType = ADODB::adCmdStoredProc;
  Rcmd->CommandText = 
    node->attributes->getNamedItem(CFG_ATTR_NAME)->text;

	// Now add the connection...
	connString = node->attributes->getNamedItem(CFG_ATTR_DSN)->text;
	
  // *** Now add the parameter elements
  // *** we also keep track internally of types and directions
  // *** so we don't have to ask ADO every time.
  MSXML::IXMLDOMNodeListPtr nl = node->childNodes;
  int ctr = 0, len = nl->length;
  CommandParameter *ret = new CommandParameter[len+1];
  for (; ctr < len; ctr++)
  {
    MSXML::IXMLDOMNodePtr xelt = nl->item[ctr];
    _bstr_t dir(xelt->attributes->getNamedItem(CFG_ATTR_DIR)->text);
    _variant_t type = 
      xelt->attributes->getNamedItem(CFG_ATTR_ADOTYPE)->text;
    _variant_t size = 
      xelt->attributes->getNamedItem(CFG_ATTR_ADOSIZE)->text;
    type.ChangeType(VT_I4);
    size.ChangeType(VT_I4);
    Rcmd->Parameters->Append(
      Rcmd->CreateParameter(
        xelt->attributes->getNamedItem(CFG_ATTR_NAME)->text,
        (ADODB::DataTypeEnum) (long) type,
        // if wcsicmp != 0, dir is output
        wcsicmp(dir,ADO_DIR_IN) ? ADODB::adParamOutput : ADODB::adParamInput,
        size,
        _variant_t()));

    // Now fill in the array with the RADIUS attribute for this parameter
    MSXML::IXMLDOMNodePtr hasRadius = 
      xelt->attributes->getNamedItem(CFG_ATTR_RADIUS);

    if (hasRadius != NULL)
    {
      swprintf(buffer, CFG_ATTRINFO_PATTERN, (LPWSTR) hasRadius->text);
      hasRadius = m_xmlConfiguration->selectSingleNode(buffer);
      if (hasRadius != NULL)
      {
        _variant_t attrNumber = 
	  hasRadius->attributes->getNamedItem(CFG_ATTR_NUMBER)->text;
        ret[ctr].radiusAttr = (long) attrNumber;
        ret[ctr].dir = 
	  wcsicmp(dir,ADO_DIR_IN) ? ADODB::adParamOutput : ADODB::adParamInput;
      }
      else
      {
        ret[ctr].radiusAttr = 0;
      }
    }
    else
    {
      ret[ctr].radiusAttr = 0;
    }
  }
  ret[ctr].radiusAttr = -1;
  return ret;
}

BOOL SettingsManager::radiusAttributeFromVariant(RADIUS_ATTRIBUTE *pOut,ADODB::_RecordsetPtr recSet)
{
	static _variant_t zero(0L),one(1L);
  _variant_t attNum = recSet->Fields->Item[zero]->Value;
  _variant_t attVal = recSet->Fields->Item[one]->Value;
	if ((DWORD)attNum.lVal > m_attributes)
		return FALSE;
	DataTypeEnum dt = m_attributeTypes[attNum.lVal];
	switch (dt)
		{
	case dataTypeBinary:
		// Just translate the BSTR odd-byte for odd-byte into the value
		{
			attVal.ChangeType(VT_BSTR,NULL);
			int len = SysStringLen(attVal.bstrVal);
			char *tmp = new char[len+1];
			pOut->lpValue = tmp;
			for (int i = 0; i < len; i++)
				tmp[i] = (char) attVal.bstrVal[i];
			pOut->dwAttrType = (long) attNum;
			pOut->cbDataLength = len;
			pOut->fDataType = rdtUnknown;
		}
  case dataTypeString:
		// Use conversion with strcpy
		{
			attVal.ChangeType(VT_BSTR,NULL);
			int nConvertedLen = WideCharToMultiByte(CP_ACP,0,attVal.bstrVal,SysStringLen(attVal.bstrVal),
																							NULL, NULL, NULL, NULL);
			char *tmp = new char[nConvertedLen+1];
      tmp[nConvertedLen] = '\0';
			pOut->lpValue = tmp;
			WideCharToMultiByte(CP_ACP,0,attVal.bstrVal,SysStringLen(attVal.bstrVal),
													tmp, nConvertedLen+1, NULL, NULL);
			pOut->dwAttrType = (long) attNum;
			pOut->cbDataLength = nConvertedLen + 1;
			pOut->fDataType = rdtString;
		}
		return true;
  case dataTypeIP:
		// IP Address Translation
		{
			int ip1,ip2,ip3,ip4;
			attVal.ChangeType(VT_BSTR,NULL);
			if (swscanf(attVal.bstrVal,L"%u.%u.%u.%u",&ip4,&ip3,&ip2,&ip1) == 4)
				{
					pOut->dwAttrType = (long) attNum;
					pOut->fDataType = rdtInteger;
					pOut->cbDataLength = sizeof(DWORD);
					if (ip1 > 255 || ip2 > 255 || ip3 > 255 || ip4 > 255)
						return false;
					pOut->dwValue = ip4 >> 24 | ip3 >> 16 | ip2 >> 8 | ip1;
					return true;
				}
			else
				return false;
		}
  case dataTypeLong:
		// Simple number
		pOut->dwAttrType = (long) attNum;
		pOut->fDataType = rdtInteger;
		pOut->cbDataLength = sizeof(DWORD);
		pOut->dwValue = (long) attVal;
		return true;
		}
	return false;
}
BOOL SettingsManager::variantFromRadiusAttributes(DWORD dwAttrIndex, 
					     CONST RADIUS_ATTRIBUTE *pInAttrs, 
					     _variant_t &vOut)
{
  // If we don't know it, just pass empty
  if (dwAttrIndex >= m_attributes)
    return false;

  // Now find the attribute in the input
  while (pInAttrs->dwAttrType != ratMinimum && pInAttrs->dwAttrType != dwAttrIndex)
    pInAttrs++;
  if (pInAttrs->dwAttrType == ratMinimum) return false;

  static char hexDig[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };

  switch (m_attributeTypes[dwAttrIndex])
  {
	case dataTypeBinary:
		{
			// Get in into hex format, which means 2 bytes per char
			BSTR spot = ::SysAllocStringLen(NULL,pInAttrs->cbDataLength * 2 + 2);
			spot[0] = L'0';
			spot[1] = L'x';
			for (DWORD i = 0; i < pInAttrs->cbDataLength; i++)
				{
					spot[2*i+2] = (WCHAR) hexDig[(pInAttrs->lpValue[i]&0xF0)>>4];
					spot[2*i+3] = (WCHAR) hexDig[pInAttrs->lpValue[i]&0x0F];
				}
			spot[2+(2*pInAttrs->cbDataLength)] = L'\0';
      VARIANT vtTmp;
      vtTmp.vt = VT_BSTR;
      V_BSTR(&vtTmp) = spot;
      vOut.Attach(vtTmp);      
		}
		return true;
  case dataTypeString:
    {
      // Be memory efficient here, this is a high traffic routine.
      // Probably some better way to do this,
      // but I'm not sure of the exact type of the pInAttrs data
      int nConvertedLen = MultiByteToWideChar(CP_ACP,0,pInAttrs->lpValue,
					      pInAttrs->cbDataLength,NULL,NULL);
      BSTR conv = ::SysAllocStringLen(NULL, nConvertedLen);
      if (conv != NULL)
	{
	  MultiByteToWideChar(CP_ACP,0,pInAttrs->lpValue,
			      pInAttrs->cbDataLength,conv,nConvertedLen);
	}
      else
	{
	  throw ERR_OUTOFMEM;
	}
      VARIANT vtTmp;
      vtTmp.vt = VT_BSTR;
      V_BSTR(&vtTmp) = conv;
      vOut.Attach(vtTmp);      
    }
    return true;
  case dataTypeIP:
    {
      // The fact that I use "register" and ADO in the same 
      // project tells you how sick I am. Somebody please tell 
      // me there is some logic to my madness. ;-) (there is)
      register DWORD ipIn = pInAttrs->dwValue;
      unsigned long ip1=ipIn & 0xFF,ip2=(ipIn>>8) & 0xFF;
      unsigned long ip3=(ipIn>>16) & 0xFF,ip4=(ipIn>>24) & 0xFF;
      WCHAR ipOut[17];
      swprintf(ipOut,L"%u.%u.%u.%u",ip4,ip3,ip2,ip1);
      vOut = ipOut;
    }
    return true;
  case dataTypeLong:
    vOut = (long) pInAttrs->dwValue;
    return true;
  default:
    vOut = CFG_UNKNOWN_TYPE;
    return true;
  }
}

_bstr_t SettingsManager::getAttributeName(DWORD dwAttrType)
{
  if (dwAttrType >= m_attributes)
		return CFG_UNKNOWN_ATTR;

  WCHAR ptrn[256];
  swprintf(ptrn,CFG_ATTRNAME_PATTERN,dwAttrType);

  MSXML::IXMLDOMNodePtr node = 
		m_xmlConfiguration->selectSingleNode(ptrn);

  if (node == NULL)
		return CFG_UNKNOWN_ATTR;
	else
		return node->attributes->getNamedItem(CFG_ATTR_NAME)->text;
}
