// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      12mar17	initial version
		01		24aug17	add OnDropFiles handler to load texture files
		02		25aug17	add check for updates
		03		09oct17	add render frame rate
		
*/

// MainFrm.h : interface of the CMainFrame class
//

#pragma once

#include "PropertiesBar.h"
#include "ModulationBar.h"
#include "PaletteBar.h"
#include "OscilloscopeBar.h"
#include "SplineBar.h"

class CPotterDrawDoc;
class CPotterDrawView;
class CRecordStatusDlg;

class CMainFrame : public CMDIFrameWndEx
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

// Constants
	enum {
		FRAME_RATE_TIMER = 1791,
		FRAME_RATE_TIMER_PERIOD = 1000,
	};
	enum {
		INDICATOR_HINT,
		INDICATOR_RESOLUTION,
		INDICATOR_ZOOM,
		INDICATOR_FRAME_RATE,
	};

// Attributes
public:
	HACCEL	GetAccelTable() const;
	void	SetDeferredUpdate(bool bEnable);
	int		GetPaletteCurSel() const;
	bool	PropertiesBarHasFocus();
	bool	GetDeferredSizing() const;
	CPaletteBar&	GetPaletteBar();
	COscilloscopeBar&	GetOscilloscopeBar();
	CSplineBar&	GetSplineBar();
	CSplineWnd&	GetSplineWnd();
	double	GetRenderFrameRate() const;

// Operations
public:
	void	OutputText(const CString& txt);
	void	OnActivateView(CView *pView);
	CPotterDrawView	*GetActiveMDIView();
	CPotterDrawDoc	*GetActiveMDIDoc();
	void	OnUpdate(CView* pSender, LPARAM lHint = 0, CObject* pHint = NULL);
	void	UpdateToolbar();
	void	OnRecord(bool bEnable);
	void	ShowRecordStatusDlg(bool bShow);
	bool	CheckForUpdates(bool Explicit);
	void	UpdateModulationBar(const CPotterDrawDoc *pDoc);

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
// Types

protected:  // control bar embedded members
	CMFCMenuBar       m_wndMenuBar;
	CMFCToolBar       m_wndToolBar;
	CMFCStatusBar     m_wndStatusBar;
	CPropertiesBar	  m_wndPropertiesBar;
	CModulationBar	  m_wndModulationBar;
	CPaletteBar		  m_wndPaletteBar;
	COscilloscopeBar  m_wndOscilloscopeBar;
	CSplineBar		  m_wndSplineBar;
	CMFCToolBarImages m_UserImages;

protected:
	BOOL CreateDockingWindows();
	void SetDockingWindowIcons(BOOL bHiColorIcons);

protected:
// Data members
	CPotterDrawView	*m_pActiveView;		// pointer to active view if any
	CMDITabInfo m_mdiTabParams;			// MDI tab parameters from OnCreate
	CRecordStatusDlg	*m_pRecordStatusDlg;	// pointer to record status dialog if any
	bool	m_bMDITabs;					// true if using MDI tabs 
	bool	m_bPreFullScreenWasZoomed;	// true if maximized prior to going full screen
	bool	m_bDeferredUpdate;			// true while a deferred update remains pending
	bool	m_bDeferredSizing;			// true while a deferred sizing remains pending
	double	m_fRenderFrameRate;			// rendering frame rate, in frames per second

// Helpers
	CString	GetResolutionString() const;
	void	FullScreen(bool bEnable);
	void	UpdateOptions();
	void	GotoNextPane(bool bPrev = false);
	static	UINT	CheckForUpdatesThreadFunc(LPVOID Param);

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnWindowManager();
	afx_msg void OnViewCustomize();
	afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnUpdateFilePrintSetup(CCmdUI *pCmdUI);
	afx_msg void OnClose();
	afx_msg LRESULT OnAfterTaskbarActivate(WPARAM wParam, LPARAM lParam);
	afx_msg void OnUpdateIndicatorZoom(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorFrameRate(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorResolution(CCmdUI* pCmdUI);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnWindowFullScreen();
	afx_msg void OnViewAppLookMDITabs();
	afx_msg void OnUpdateViewAppLookMDITabs(CCmdUI* pCmdUI);
	afx_msg void OnUpdateWindowCascade(CCmdUI* pCmdUI);
	afx_msg LRESULT OnPropertyChange(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPropertySelect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPaletteChange(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPaletteSelection(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT	OnHandleDlgKey(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT	OnModelessDestroy(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDeferredSizing(WPARAM wParam, LPARAM lParam);
	afx_msg void OnToolsOptions();
	afx_msg void OnViewRecordStatus();
	afx_msg void OnUpdateViewRecordStatus(CCmdUI *pCmdUI);
	afx_msg void OnNextPane();
	afx_msg void OnPrevPane();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnAppCheckForUpdates();
	afx_msg LRESULT	OnDelayedCreate(WPARAM wParam, LPARAM lParam);
};

inline CPotterDrawView *CMainFrame::GetActiveMDIView()
{
	return(m_pActiveView);
}

inline HACCEL CMainFrame::GetAccelTable() const
{
	return(m_hAccelTable);
}

inline void CMainFrame::SetDeferredUpdate(bool bEnable)
{
	m_bDeferredUpdate = bEnable;
}

inline int CMainFrame::GetPaletteCurSel() const
{
	return m_wndPaletteBar.GetCurSel();
}

inline bool CMainFrame::PropertiesBarHasFocus()
{
	return ::IsChild(m_wndPropertiesBar.m_hWnd, ::GetFocus()) != 0;
}

inline bool CMainFrame::GetDeferredSizing() const
{
	return m_bDeferredSizing;
}

inline CPaletteBar& CMainFrame::GetPaletteBar()
{
	return m_wndPaletteBar;
}

inline COscilloscopeBar& CMainFrame::GetOscilloscopeBar()
{
	return m_wndOscilloscopeBar;
}

inline CSplineBar& CMainFrame::GetSplineBar()
{
	return m_wndSplineBar;
}

inline CSplineWnd& CMainFrame::GetSplineWnd()
{
	return m_wndSplineBar.m_wndSpline;
}

inline double CMainFrame::GetRenderFrameRate() const
{
	return m_fRenderFrameRate;
}

