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

// PotterDraw.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "PotterDraw.h"
#include "MainFrm.h"
#include "AboutDlg.h"

#include "ChildFrm.h"
#include "PotterDrawDoc.h"
#include "PotterDrawView.h"

#include "Win32Console.h"
#include "VersionInfo.h"
#include "PathStr.h"
#include "FocusEdit.h"
#include "Hyperlink.h"

#define HOME_PAGE_URL _T("http://potterdraw.sourceforge.net")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "HelpIDs.h"
const CPotterDrawApp::HELP_RES_MAP CPotterDrawApp::m_HelpResMap[] = {
	#include "HelpResMap.h"
};

// CPotterDrawApp

BEGIN_MESSAGE_MAP(CPotterDrawApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CPotterDrawApp::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CPotterDrawApp::OnFilePrintSetup)
	ON_COMMAND(ID_HELP_FINDER, CWinAppEx::OnHelp)
	ON_COMMAND(ID_HELP, CWinAppEx::OnHelp)
	ON_COMMAND(ID_APP_HOME_PAGE, OnAppHomePage)
END_MESSAGE_MAP()

// CPotterDrawApp construction

CPotterDrawApp::CPotterDrawApp()
{
	m_bHiColorIcons = TRUE;

	// support Restart Manager
//	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;

	SetAppID(_T("AnalSoftware.PotterDraw.PotterDraw.1.0"));

	m_gdiplusToken = 0;	// used by GDI+ init
	m_bHelpInit = false;

	// Place all significant initialization in InitInstance
}

// The one and only CPotterDrawApp object

CPotterDrawApp theApp;


// CPotterDrawApp initialization

BOOL CPotterDrawApp::InitInstance()
{
#ifdef _DEBUG
	Win32Console::Create();	// create console window
#endif

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	// Initialize OLE libraries
	if (!AfxOleInit()) {
		AfxMessageBox(_T("OLE init failed."));
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction();

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	if (Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok)
		AfxMessageBox(_T("Can't initialize Gdiplus."));

	// AfxInitRichEdit2() is required to use RichEdit control
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	SetRegistryKey(_T("Anal Software"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)
	m_Options.ReadProperties();

	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
			RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_POTTERDRAWTYPE,
										 RUNTIME_CLASS(CPotterDrawDoc),
										 RUNTIME_CLASS(CChildFrame), // custom MDI child frame
										 RUNTIME_CLASS(CPotterDrawView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME)) {
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;

	// call DragAcceptFiles only if there's a suffix
	//  In an MDI app, this should occur immediately after setting m_pMainWnd
	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);


	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// The main window has been initialized, so show and update it
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}

int CPotterDrawApp::ExitInstance()
{
	CloseHtmlHelp();

	m_Options.WriteProperties();

	AfxOleTerm(FALSE);

	Gdiplus::GdiplusShutdown(m_gdiplusToken);

	return CWinAppEx::ExitInstance();
}

void CPotterDrawApp::CloseHtmlHelp()
{
	// if HTML help was initialized, close all topics
	if (m_bHelpInit) {
		::HtmlHelp(NULL, NULL, HH_CLOSE_ALL, 0);
		m_bHelpInit = FALSE;
	}
}

#ifdef _DEBUG
CMainFrame* CPotterDrawApp::GetMainFrame() const // non-debug version is inline
{
	ASSERT(m_pMainWnd->IsKindOf(RUNTIME_CLASS(CMainFrame)));
	return (CMainFrame*)m_pMainWnd;
}
#endif //_DEBUG

bool CPotterDrawApp::HandleDlgKeyMsg(MSG* pMsg)
{
	static const LPCSTR	EditBoxCtrlKeys = "ACHVX";	// Z reserved for app undo
	CMainFrame	*pMain = theApp.GetMainFrame();
	ASSERT(pMain != NULL);	// main frame must exist
	switch (pMsg->message) {
	case WM_KEYDOWN:
		{
			int	VKey = INT64TO32(pMsg->wParam);
			bool	bTryMainAccels = FALSE;	// assume failure
			if ((VKey >= VK_F1 && VKey <= VK_F24) || VKey == VK_ESCAPE) {
				bTryMainAccels = TRUE;	// function key or escape
			} else {
				bool	IsAlpha = VKey >= 'A' && VKey <= 'Z';
				CEdit	*pEdit = CFocusEdit::GetEdit();
				if (pEdit != NULL) {	// if an edit control has focus
					if ((IsAlpha									// if (alpha key
					&& strchr(EditBoxCtrlKeys, VKey) == NULL		// and unused by edit
					&& (GetKeyState(VK_CONTROL) & GKS_DOWN))		// and Ctrl is down)
					|| ((VKey == VK_SPACE							// or (space key,
					|| VKey == VK_RETURN || VKey == VK_BACK)		// Enter or Backspace
					&& (GetKeyState(VK_CONTROL) & GKS_DOWN))		// and Ctrl is down)
					|| (VKey == VK_SPACE							// or (space key
					&& (GetKeyState(VK_SHIFT) & GKS_DOWN)))			// and Shift is down)
						bTryMainAccels = TRUE;	// give main accelerators a try
				} else {	// non-edit control has focus
					if (IsAlpha										// if alpha key
					|| VKey == VK_SPACE								// or space key
					|| (GetKeyState(VK_CONTROL) & GKS_DOWN)			// or Ctrl is down
					|| (GetKeyState(VK_SHIFT) & GKS_DOWN))			// or Shift is down
						bTryMainAccels = TRUE;	// give main accelerators a try
				}
			}
			if (bTryMainAccels) {
				HACCEL	hAccel = pMain->GetAccelTable();
				if (hAccel != NULL && TranslateAccelerator(pMain->m_hWnd, hAccel, pMsg)) {
					return(TRUE);	// message was translated, stop dispatching
				}
			}
		}
		break;
	case WM_SYSKEYDOWN:
		{
			if (GetKeyState(VK_SHIFT) & GKS_DOWN)	// if context menu
				return(FALSE);	// keep dispatching (false alarm)
			pMain->SetFocus();	// main frame must have focus to display menus
			HACCEL	hAccel = pMain->GetAccelTable();
			if (hAccel != NULL && TranslateAccelerator(pMain->m_hWnd, hAccel, pMsg)) {
				return(TRUE);	// message was translated, stop dispatching
			}
		}
		break;
	}
	return(FALSE);	// continue dispatching
}

bool CPotterDrawApp::GetTempPath(CString& Path)
{
	LPTSTR	pBuf = Path.GetBuffer(MAX_PATH);
	DWORD	retc = ::GetTempPath(MAX_PATH, pBuf);
	Path.ReleaseBuffer();
	return(retc != 0);
}

bool CPotterDrawApp::GetTempFileName(CString& Path, LPCTSTR Prefix, UINT Unique)
{
	CString	TempPath;
	if (!GetTempPath(TempPath))
		return(FALSE);
	if (Prefix == NULL)
		Prefix = m_pszAppName;
	LPTSTR	pBuf = Path.GetBuffer(MAX_PATH);
	DWORD	retc = ::GetTempFileName(TempPath, Prefix, Unique, pBuf);
	Path.ReleaseBuffer();
	return(retc != 0);
}

void CPotterDrawApp::GetCurrentDirectory(CString& Path)
{
	LPTSTR	pBuf = Path.GetBuffer(MAX_PATH);
	::GetCurrentDirectory(MAX_PATH, pBuf);
	Path.ReleaseBuffer();
}

bool CPotterDrawApp::GetSpecialFolderPath(int FolderID, CString& Path)
{
	LPTSTR	p = Path.GetBuffer(MAX_PATH);
	bool	retc = SUCCEEDED(SHGetSpecialFolderPath(NULL, p, FolderID, 0));
	Path.ReleaseBuffer();
	return(retc);
}

bool CPotterDrawApp::GetAppDataFolder(CString& Folder) const
{
	CPathStr	path;
	if (!GetSpecialFolderPath(CSIDL_APPDATA, path))
		return(FALSE);
	path.Append(m_pszAppName);
	Folder = path;
	return(TRUE);
}

CString CPotterDrawApp::GetAppPath()
{
	CString	s = GetCommandLine();
	s.TrimLeft();	// trim leading whitespace just in case
	if (s[0] == '"')	// if first char is a quote
		s = s.Mid(1).SpanExcluding(_T("\""));	// span to next quote
	else
		s = s.SpanExcluding(_T(" \t"));	// span to next whitespace
	return(s);
}

CString CPotterDrawApp::GetAppFolder()
{
	CPathStr	path(GetAppPath());
	path.RemoveFileSpec();
	return(path);
}

bool CPotterDrawApp::GetJobLogFolder(CString& Folder) const
{
	return(false);
}

CString CPotterDrawApp::GetVersionString()
{
	VS_FIXEDFILEINFO	AppInfo;
	CString	sVersion;
	CVersionInfo::GetFileInfo(AppInfo, NULL);
	sVersion.Format(_T("%d.%d.%d.%d"), 
		HIWORD(AppInfo.dwFileVersionMS), LOWORD(AppInfo.dwFileVersionMS),
		HIWORD(AppInfo.dwFileVersionLS), LOWORD(AppInfo.dwFileVersionLS));
#ifdef _WIN64
	sVersion += _T(" x64");
#else
	sVersion += _T(" x86");
#endif
#ifdef _DEBUG
	sVersion += _T(" Debug");
#else
	sVersion += _T(" Release");
#endif
	return sVersion;
}

bool CPotterDrawApp::CreateFolder(LPCTSTR Path)
{
	int	retc = SHCreateDirectoryEx(NULL, Path, NULL);	// create folder
	switch (retc) {
	case ERROR_SUCCESS:
	case ERROR_FILE_EXISTS:
	case ERROR_ALREADY_EXISTS:
		break;
	default:
		return false;
	}
	return true;
}

bool CPotterDrawApp::DeleteFolder(LPCTSTR Path, FILEOP_FLAGS nFlags)
{
	SHFILEOPSTRUCT fop = {NULL, FO_DELETE, Path, _T(""), nFlags};
	return !SHFileOperation(&fop);
}

// CPotterDrawApp message handlers


// App command to run the dialog
void CPotterDrawApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CPotterDrawApp customization load/save methods

void CPotterDrawApp::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	bNameValid = strName.LoadString(IDS_EXPLORER);
	ASSERT(bNameValid);
}

void CPotterDrawApp::LoadCustomState()
{
}

void CPotterDrawApp::SaveCustomState()
{
}

BOOL CPotterDrawApp::IsIdleMessage(MSG* pMsg)
{
	if (CWinApp::IsIdleMessage(pMsg)) {
		switch (pMsg->message) {	// don't call OnIdle after these messages
		case UWM_FRAME_TIMER:
			return FALSE;
		case WM_TIMER:
			if (pMsg->wParam == CMainFrame::FRAME_RATE_TIMER)
				return FALSE;
		default:
			return TRUE;
		}
	} else
		return FALSE;
}

BOOL CPotterDrawApp::OnIdle(LONG lCount)
{
	CPotterDrawView	*pView = GetMainFrame()->GetActiveMDIView();
	if (pView != NULL && pView->IsRecording()) {	// if recording
		pView->SendMessage(UWM_FRAME_TIMER);	// render a frame
		lCount = 0;	// request more idle time
	}
	return CWinAppEx::OnIdle(lCount);
}

int CPotterDrawApp::FindHelpID(int nResID)
{
	int	nElems = _countof(m_HelpResMap);
	for (int iElem = 0; iElem < nElems; iElem++) {	// for each map element
		if (nResID == m_HelpResMap[iElem].nResID)	// if resource ID found
			return(m_HelpResMap[iElem].nHelpID);	// return context help ID
	}
	return(0);
}

void CPotterDrawApp::WinHelp(DWORD_PTR dwData, UINT nCmd) 
{
//printf("dwData=%d:%d nCmd=%d\n", HIWORD(dwData), LOWORD(dwData), nCmd);
	CPathStr	HelpPath(GetAppFolder());
	HelpPath.Append(CString(m_pszAppName) + _T(".chm"));
	HWND	hMainWnd = GetMainFrame()->m_hWnd;
	UINT	nResID = LOWORD(dwData);
	int	nHelpID = FindHelpID(nResID);
	HWND	hWnd = 0;	// assume failure
	if (nHelpID)	// if context help ID was found
		hWnd = ::HtmlHelp(hMainWnd, HelpPath, HH_HELP_CONTEXT, nHelpID);
	if (!hWnd) {	// if context help wasn't available or failed
		hWnd = ::HtmlHelp(hMainWnd, HelpPath, HH_DISPLAY_TOC, 0);	// show contents
		if (!hWnd) {	// if help file not found
			CString	s;
			AfxFormatString1(s, IDS_APP_HELP_FILE_MISSING, HelpPath);
			AfxMessageBox(s);
			return;
		}
	}
	m_bHelpInit = true;
}

void CPotterDrawApp::WinHelpInternal(DWORD_PTR dwData, UINT nCmd)
{
	WinHelp(dwData, nCmd);	// route to our WinHelp override
}

// CPotterDrawApp message handlers

void CPotterDrawApp::OnAppHomePage() 
{
	if (!CHyperlink::GotoUrl(HOME_PAGE_URL))
		AfxMessageBox(IDS_HLINK_CANT_LAUNCH);
}
