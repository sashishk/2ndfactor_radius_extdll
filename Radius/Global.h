// Global defines for the RadiusDB project
#ifndef _GLOBALRDB_H
#define _GLOBALRDB_H

// From SettingsManager
#define RDB_REG  _T("Software\\RadiusDB")
#define RDB_CONN _T("ConnectionString")
#define RDB_XML  _T("ConfigURL")

#define PWDREQ_COMMAND L"PasswordRequest"
#define LOGIN_COMMAND  L"LoginRequest"
#define ACCT_COMMAND   L"AccountingRequest"

#define MISSING_SECTION _T("Missing RadiusDB Section")
#define MISSING_XML     _T("Missing ConfigURL")
#define ERR_OUTOFMEM    _T("OUT OF MEMORY!")

#define CFG_ATTR_NODE     L"/RadiusConfig/Attributes"

#define CFG_ATTR_DSN      L"DSN"
#define CFG_ATTR_NUMBER   L"Number"
#define CFG_ATTR_TYPE     L"Type"
#define CFG_ATTR_NAME     L"Name"
#define CFG_ATTR_ADOTYPE  L"ADOType"
#define CFG_ATTR_ADOSIZE  L"ADOSize"
#define CFG_ATTR_DIR      L"Direction"
#define CFG_ATTR_RADIUS   L"RadiusAttribute"

#define ADO_DIR_IN     L"Input"
#define ADO_DIR_OUT    L"Output"

#define CFG_STR_TYPE     L"String"
#define CFG_IP_TYPE      L"IP"
#define CFG_LONG_TYPE    L"Long"
#define CFG_BINARY_TYPE  L"Binary"
#define CFG_UNKNOWN_TYPE L"Unknown Type"
#define CFG_UNKNOWN_ATTR L"Unknown Attribute"

#define CFG_ARGINFO_PATTERN \
          L"/RadiusConfig/Schema/%s/Parameter[@RadiusAttribute=\"%s\"]"
#define CFG_PROCINFO_PATTERN L"/RadiusConfig/Schema/%s"
#define CFG_ATTRINFO_PATTERN \
          L"/RadiusConfig/Attributes/Attribute[@Name=\"%s\"]"
#define CFG_ATTRNAME_PATTERN \
          L"/RadiusConfig/Attributes/Attribute[@Number=\"%u\"]"

class SettingsManager;

extern SettingsManager *g_settings;
extern HANDLE hEventSource;

// Utility functions
//     Check a password against a profile, which in the future 
//     will implement both PAP & CHAP
BOOL CheckPassword(BSTR realPassword, CONST RADIUS_ATTRIBUTE *pAttrs);
//     Log attributes of the request to the event log
void LogAttributes(CONST RADIUS_ATTRIBUTE *pInAttrs);
//     Is the request an Authentication request?
BOOL isPasswordCheck(CONST RADIUS_ATTRIBUTE *pInAttrs);

#endif // _GLOBALRDB_H
