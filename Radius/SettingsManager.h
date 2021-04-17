//////////////////////////////////////////////////////////////////////
// SettingsManager.h: interface for the SettingsManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SETTINGSMANAGER_H__F5C611A7_3705_433B_B058_124FEFE2402D__INCLUDED_)
#define AFX_SETTINGSMANAGER_H__F5C611A7_3705_433B_B058_124FEFE2402D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class SettingsManager  
{
public:

  struct CommandParameter
  {
    ADODB::ParameterDirectionEnum dir;
    DWORD                         radiusAttr;
  };
  
  enum DataTypeEnum {
    dataTypeEndMarker = 0,
    dataTypeString = 1,
    dataTypeIP = 2,
    dataTypeLong = 3,
		dataTypeBinary = 4
  };  
  
  SettingsManager();
  virtual ~SettingsManager();

  // Find and convert a Radius Attribute in the array to a variant
  BOOL variantFromRadiusAttributes(DWORD dwAttr, 
			      CONST RADIUS_ATTRIBUTE *pInAttrs, 
			      _variant_t &vOut);
	BOOL radiusAttributeFromVariant(RADIUS_ATTRIBUTE *pOut, ADODB::_RecordsetPtr recSet);

  // Fill an ADO Command with parameters for the given procedure
  CommandParameter *FillCommand(ADODB::_CommandPtr Rcmd,LPWSTR procName, _bstr_t &connString);

  // Returns true if a PasswordRequest step is needed
  BOOL HasSeparatePasswordStep() { return m_hasPWStep; }
	// Returns true if an AccoutingRequest step is needed
	BOOL HasAccoutingStep() { return m_accountingStep; }

	// Get the name of a given RADIUS attribute
  _bstr_t getAttributeName(DWORD dwAttrType);

protected:
  // XML Configuration document
  MSXML::IXMLDOMDocumentPtr m_xmlConfiguration;
  // All RADIUS attribute types
  DataTypeEnum              *m_attributeTypes;
  // How many RADIUS attributes
  DWORD                     m_attributes;
  // Do we have a PasswordRequest step
  BOOL                      m_hasPWStep;
  // Do we have an AccoutingRequest step
  BOOL                      m_accountingStep;
};

#endif // !defined(AFX_SETTINGSMANAGER_H__F5C611A7_3705_433B_B058_124FEFE2402D__INCLUDED_)
