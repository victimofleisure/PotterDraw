// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00		12mar17	initial version

		global definitions and inlines

*/

#pragma once

#include "Wrapx64.h"	// ck: special types for supporting both 32-bit and 64-bit
#include "WObject.h"	// ck: ultra-minimal base class used by many of my objects
#include "ArrayEx.h"	// ck: wraps MFC dynamic arrays, adding speed and features
#include "Round.h"		// ck: round floating point to integer

// define registry section for settings
#define REG_SETTINGS _T("Settings")

// key status bits for GetKeyState and GetAsyncKeyState
#define GKS_TOGGLED			0x0001
#define GKS_DOWN			0x8000

// clamp a value to a range
#define CLAMP(x, lo, hi) (min(max((x), (lo)), (hi)))

// trap bogus default case in switch statement
#define NODEFAULTCASE	ASSERT(0)

// load string from resource via temporary object
#define LDS(x) CString((LPCTSTR)x)

// ck: define containers for some useful built-in types
typedef CArrayEx<float, float> CFloatArray;
typedef CArrayEx<double, double> CDoubleArray;
typedef CArrayEx<char, char> CCharArray;

enum {	// application-wide user window messages, based on WP_APP
	UWM_FIRST = WM_APP,
	UWM_HANDLEDLGKEY,			// wParam: MSG pointer, lParam: none
	UWM_MODELESSDESTROY,		// wParam: CDialog*, lParam: none
	UWM_DEFERRED_UPDATE,		// wParam: none, lParam: none
	UWM_DEFERRED_SIZING,		// wParam: none, lParam: none
	UWM_FRAME_TIMER,			// wParam: none, lParam: none
	UWM_PROPERTY_CHANGE,		// wParam: iProp, lParam: CWnd*
	UWM_PROPERTY_SELECT,		// wParam: iProp or -1 if none, lParam: CWnd*
	UWM_PALETTE_CHANGE,			// wParam: iColor or -1 for all, lParam: COLORREF
	UWM_PALETTE_SELECTION,		// wParam: iColor, lParam: unused
	UWM_SET_RECORD,				// wParam: bEnable, lParam: none
	UWM_PROPERTY_HELP,			// wParam: iProp, lParam: CWnd*
};

// ck: wrapper for formatting system errors
CString FormatSystemError(DWORD ErrorCode);
CString	GetLastErrorString();

// ck: workaround for Aero animated progress bar's absurd lag
void SetTimelyProgressPos(CProgressCtrl& Progress, int nPos);

bool GetUserNameString(CString& sName);
bool GetComputerNameString(CString& sName);
bool CopyStringToClipboard(HWND m_hWnd, const CString& strData);

void EnableChildWindows(CWnd& Wnd, bool Enable, bool Deep = TRUE);
void UpdateMenu(CWnd *pWnd, CMenu *pMenu);

// data validation method to flunk a control
void DDV_Fail(CDataExchange* pDX, int nIDC);

// set non-zero for undo natter
#define UNDO_NATTER 0

class CPotterDrawApp;
extern CPotterDrawApp theApp;
inline CWinApp* FastAfxGetApp() { return (CWinApp*)&theApp; }
#define AfxGetApp FastAfxGetApp
