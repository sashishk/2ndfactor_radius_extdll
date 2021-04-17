// DBInterface.cpp: implementation of the DBInterface class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DBInterface.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DBInterface::DBInterface(SettingsManager &smgr) :
m_settings(smgr), m_valid(false), m_LRAttributeArray(NULL),
m_ACAttributeArray(NULL), m_GPAttributeArray(NULL),
m_passwordOutputIndex(-1L)
{
}

DBInterface::~DBInterface()
{
  if (m_LRAttributeArray) delete[] m_LRAttributeArray;
  if (m_GPAttributeArray) delete[] m_GPAttributeArray;
  if (m_ACAttributeArray) delete[] m_ACAttributeArray;
}

BSTR DBInterface::GetPassword(CONST RADIUS_ATTRIBUTE *pInAttrs)
{
  if (m_getPasswordCommand == NULL)
  {
    m_getPasswordCommand.CreateInstance(__uuidof( ADODB::Command ));
    m_GPAttributeArray = 
      m_settings.FillCommand(m_getPasswordCommand,PWDREQ_COMMAND,m_getPasswordConn);

    // Find out which attribute holds the password output string
    for (long i = 0; m_GPAttributeArray[i].radiusAttr != -1; i++)
    {
      if (m_GPAttributeArray[i].radiusAttr == ratUserPassword)
      {
        m_passwordOutputIndex = i;
        break;
      }
    }
  }

  CallCommand(m_getPasswordCommand,m_getPasswordConn,m_GPAttributeArray,pInAttrs);

  // Now return the ratUserPassword attribute
  if (m_passwordOutputIndex.lVal == -1)
    // Whatever, this isn't setup correctly.  
    // Should log a one-time event or something.
    return NULL;  

  // Should optimize this too
	_variant_t retVal = m_getPasswordCommand->Parameters->
		Item[m_passwordOutputIndex]->Value;
	if (retVal.vt == VT_NULL)
		return NULL;
  return _bstr_t(retVal);
}

BOOL DBInterface::isValid()
{
  return m_valid;
}

ADODB::_RecordsetPtr DBInterface::LoginRequest(CONST RADIUS_ATTRIBUTE *pInAttrs, BSTR *pwd)
{
  if (m_loginRequestCommand == NULL)
  {
    m_loginRequestCommand.CreateInstance(__uuidof( ADODB::Command ));
    m_LRAttributeArray = 
      m_settings.FillCommand(m_loginRequestCommand,LOGIN_COMMAND,m_loginRequestConn);

    // Find out which attribute holds the password output string, if any
		// We need to do this in all cases, since someone else may call in
		// with valid pwd request, in theory
		for (long i = 0; m_LRAttributeArray[i].radiusAttr != -1; i++)
			{
				if (m_LRAttributeArray[i].radiusAttr == ratUserPassword)
					{
						m_loginPasswordOutputIndex = i;
						break;
					}
			}
	}

	ADODB::_RecordsetPtr retPtr = CallCommand(m_loginRequestCommand,m_loginRequestConn,m_LRAttributeArray,pInAttrs);

	// Now return the ratUserPassword attribute if it's requested
	if (pwd)
		{
			if (m_loginPasswordOutputIndex.lVal == -1)
				// Whatever, this isn't setup correctly.  
				// Should log a one-time event or something.
				return retPtr;
			
			// Should optimize this too
			_variant_t retpwd = m_loginRequestCommand->Parameters->
				Item[m_loginPasswordOutputIndex]->Value;
			if (retpwd.vt == VT_NULL)
				*pwd = NULL;
			else
				*pwd = ::SysAllocString(retpwd.bstrVal);
		}
	return retPtr;

}

ADODB::_RecordsetPtr 
DBInterface::CallCommand(
			 ADODB::_CommandPtr cp, 
			 _bstr_t &connStr,
			 SettingsManager::CommandParameter *attrArray, 
			 CONST RADIUS_ATTRIBUTE *pAttrs)
{
  _variant_t indParm;
  // Set all the input parameters
  for (long j = 0; attrArray[j].radiusAttr != -1; j++)
  {
    if (attrArray[j].dir == ADODB::adParamInput ||
      attrArray[j].dir == ADODB::adParamInputOutput)
    {
      _variant_t     vOut;
      if (m_settings.variantFromRadiusAttributes(attrArray[j].radiusAttr,pAttrs,vOut) == false)
				vOut.Clear();
      indParm = j;
      cp->Parameters->Item[indParm]->Value = vOut;
    }
  }

	_bstr_t empty(L"");
  ADODB::_ConnectionPtr conn;
  conn.CreateInstance( __uuidof( ADODB::Connection ) );
  conn->CursorLocation = ADODB::adUseClient;
	conn->Open(connStr,empty,empty,ADODB::adConnectUnspecified);
  cp->ActiveConnection = conn;

  _variant_t emptyVar;
  _variant_t iRecAffected(0L);

  ADODB::_RecordsetPtr ret = 
    cp->Execute(&iRecAffected,NULL,ADODB::adCmdUnspecified);

  cp->ActiveConnection = NULL;

  return ret;
}

void DBInterface::AccountingRequest(CONST RADIUS_ATTRIBUTE *pInAttrs)
{
  if (m_accountingCommand == NULL)
  {
    m_accountingCommand.CreateInstance(__uuidof( ADODB::Command ));
    m_ACAttributeArray = 
      m_settings.FillCommand(m_accountingCommand,ACCT_COMMAND,m_accountingConn);
  }

  CallCommand(m_accountingCommand,m_accountingConn,m_ACAttributeArray,pInAttrs);
}
