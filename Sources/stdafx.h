#pragma once

#define _CRT_SECURE_NO_DEPRECATE

#define WINVER         0x0501 // Change this to the appropriate value to target other versions of Windows.
#define _WIN32_WINNT   0x0501 // Change this to the appropriate value to target other versions of Windows.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#define _WIN32_IE      0x0600 // Change this to the appropriate value to target other versions of IE.

#include <afxwin.h>        // MFC core and standard components
#include <afxext.h>        // MFC extensions
#include <afxdisp.h>       // MFC Automation classes
#include <afxdtctl.h>      // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxdhtml.h>

