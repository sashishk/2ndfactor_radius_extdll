// DBInterface.h: interface for the DBInterface class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DBINTERFACE_H__4B3AE378_BB57_4521_9AF3_F26A98A681BB__INCLUDED_)
#define AFX_DBINTERFACE_H__4B3AE378_BB57_4521_9AF3_F26A98A681BB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class DBInterface  
{
public:
  DBInterface(SettingsManager &sm);
  virtual ~DBInterface();

  // Run a login request
  ADODB::_RecordsetPtr LoginRequest(CONST RADIUS_ATTRIBUTE *pInAttrs, BSTR *pwd = NULL);
  // Run a password fetch request
  BSTR GetPassword(CONST RADIUS_ATTRIBUTE *pInAttrs);
  // Handle an accounting packet
  void AccountingRequest(CONST RADIUS_ATTRIBUTE *pInAttrs);

  BOOL isValid();
protected:
  BOOL m_valid;

  ADODB::_CommandPtr m_loginRequestCommand;
  _bstr_t            m_loginRequestConn;
  ADODB::_CommandPtr m_accountingCommand;
  _bstr_t            m_accountingConn;
  ADODB::_CommandPtr m_getPasswordCommand;
  _bstr_t            m_getPasswordConn;


  _variant_t         m_passwordOutputIndex;
  _variant_t         m_loginPasswordOutputIndex;
  
  SettingsManager::CommandParameter   *m_GPAttributeArray;
  SettingsManager::CommandParameter   *m_LRAttributeArray;
  SettingsManager::CommandParameter   *m_ACAttributeArray;
  
  SettingsManager &m_settings;

  // Execute an ADO command and return the recordset
  ADODB::_RecordsetPtr CallCommand(ADODB::_CommandPtr cp, 
																	 _bstr_t &connStr,
																	 SettingsManager::CommandParameter *attrArray,
																	 CONST RADIUS_ATTRIBUTE *pAttrs);
};

#endif // !defined(AFX_DBINTERFACE_H__4B3AE378_BB57_4521_9AF3_F26A98A681BB__INCLUDED_)
