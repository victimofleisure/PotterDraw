// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      12mar17	initial version
		
*/

// PotterDraw.h : main header file for the PotterDraw application
//
#pragma once

#ifndef __AFXWIN_H__
#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "Resource.h"       // main symbols
#include "ArrayEx.h"
#include "Options.h"

// CPotterDrawApp:
// See PotterDraw.cpp for the implementation of this class
//

class CMainFrame;
class CPrintStatusDlg;

class CPotterDrawApp : public CWinAppEx
{
public:
	CPotterDrawApp();

// Attributes
	CMainFrame	*GetMainFrame() const;
	bool	GetTempPath(CString& Path);
	bool	GetTempFileName(CString& Path, LPCTSTR Prefix = NULL, UINT Unique = 0);
	static	void	GetCurrentDirectory(CString& Path);
	static	bool	GetSpecialFolderPath(int FolderID, CString& Folder);
	bool	GetAppDataFolder(CString& Folder) const;
	bool	GetJobLogFolder(CString& Folder) const;
	static	CString GetAppPath();
	static	CString GetAppFolder();
	static	CString GetVersionString();
	static	bool	CreateFolder(LPCTSTR Path);
	static	bool	DeleteFolder(LPCTSTR Path, FILEOP_FLAGS nFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT);
	static	int		FindHelpID(int nResID);

	COptions	m_Options;	// options data
	bool	m_bHelpInit;			// true if help was initialized

// Operations
	bool	HandleDlgKeyMsg(MSG* pMsg);
	void	CloseHtmlHelp();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual void WinHelp(DWORD_PTR dwData, UINT nCmd = HELP_CONTEXT);
	virtual void WinHelpInternal(DWORD_PTR dwData, UINT nCmd = HELP_CONTEXT);

// Implementation
	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;
	ULONG_PTR	m_gdiplusToken;		// used by GDI+ initialization

	struct HELP_RES_MAP {
		WORD	nResID;		// resource identifier
		WORD	nHelpID;	// identifier of corresponding help topic
	};

// Constants
	static const HELP_RES_MAP	m_HelpResMap[];	// map resource IDs to help IDs

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();
	virtual	BOOL IsIdleMessage(MSG* pMsg);

	afx_msg void OnAppAbout();
	afx_msg void OnAppHomePage();
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnIdle(LONG lCount);
};

extern CPotterDrawApp theApp;

#ifndef _DEBUG  // debug version in PotterDraw.cpp
inline CMainFrame* CPotterDrawApp::GetMainFrame() const
{
	return reinterpret_cast<CMainFrame*>(m_pMainWnd);
}
#endif
