// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__9F053D1D_7E10_45A8_A5D4_E3BA76A1ED7E__INCLUDED_)
#define AFX_STDAFX_H__9F053D1D_7E10_45A8_A5D4_E3BA76A1ED7E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define _WIN32_DCOM

#include <stdlib.h>
#ifdef DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

// Insert your headers here
#include <windows.h>
#include <authif.h>
#include <tchar.h>
#include <fstream.h>
#include <objbase.h>
#include <atlbase.h>
#include <stdio.h>

#import "c:\program files\common files\system\ado\msado15.dll" \
      rename ( "EOF", "adoEOF" )
#import "msxml.dll"

#include "md5.h"

#include "Global.h"
#include "RadiusEvents.h"
#include "SettingsManager.h"
#include "DBInterface.h"

#define REPORT_INFO(ev,n,ar) ReportEvent(hEventSource,EVENTLOG_INFORMATION_TYPE,CATID_GENERAL,ev,NULL,n,0,ar,NULL)
#define REPORT_ERR(ev,n,ar) ReportEvent(hEventSource,EVENTLOG_ERROR_TYPE,CATID_GENERAL,ev,NULL,n,0,ar,NULL)

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__9F053D1D_7E10_45A8_A5D4_E3BA76A1ED7E__INCLUDED_)
