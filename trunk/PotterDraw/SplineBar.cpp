// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      20jun17	initial version
		01		05sep17	in OnNotify, add changing notification to update view
		
*/

#include "stdafx.h"
#include "PotterDraw.h"
#include "SplineBar.h"
#include "MainFrm.h"
#include "PotterDrawDoc.h"
#include "PotterDrawView.h"
#include "UndoCodes.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CSplineBar

// spline notifications that aren't undoable edits don't have resource strings, 
// but their resource IDs still have to be defined for code generation to work
#define IDS_SPLINE_NM_CHANGING 0
#define IDS_SPLINE_NM_PAN 0
#define IDS_SPLINE_NM_ZOOM 0
#define IDS_SPLINE_NM_SELECTION 0
#define IDS_SPLINE_NM_STYLE 0
#define IDS_SPLINE_NM_GRID_SETUP 0

const int CSplineBar::m_nUndoTitleID[] = {
	0,	// reserved for NM_SPLINE_FIRST
	#define SPLINEMSGDEF(name) IDS_SPLINE_NM_##name,
	#include "SplineMsgDef.h"	// generate undo title resource IDs for spline notifications
};

CSplineBar::CSplineBar()
{
}

CSplineBar::~CSplineBar()
{
}

int CSplineBar::GetUndoTitle(int iCode)
{
	int	iTitle = iCode - CSplineWnd::NM_SPLINE_FIRST;
	ASSERT(iTitle >= 0 && iTitle < _countof(m_nUndoTitleID));
	return m_nUndoTitleID[iTitle];
}

BEGIN_MESSAGE_MAP(CSplineBar, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_SETFOCUS()
	ON_WM_PARENTNOTIFY()
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSplineBar message handlers

int CSplineBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	// create spline window
	CRect	rc;
	GetClientRect(rc);
	DWORD	dwStyle = WS_CHILD | WS_VISIBLE;
	if (!m_wndSpline.CreateEx(WS_EX_COMPOSITED, NULL, _T(""), dwStyle, rc, this, ID_SPLINE))
		return -1;
	ModifyStyle(0, WS_CLIPCHILDREN);	// dialog doesn't have this style by default

	return 0;
}

BOOL CSplineBar::OnEraseBkgnd(CDC* pDC)
{
	return CWnd::OnEraseBkgnd(pDC);
}

void CSplineBar::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	if (m_wndSpline.m_hWnd)
		m_wndSpline.MoveWindow(0, 0, cx, cy);
}

void CSplineBar::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_wndSpline.SetFocus();
}

void CSplineBar::OnParentNotify(UINT message, LPARAM lParam)
{
	CDockablePane::OnParentNotify(message, lParam);
	switch (message) {
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		m_wndSpline.SetFocus();	// so clicking child control activates bar
	}
}

BOOL CSplineBar::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (wParam == ID_SPLINE) {	// if spline notification
		CPotterDrawDoc	*pDoc = theApp.GetMainFrame()->GetActiveMDIDoc();
		if (pDoc != NULL) {
			NMHDR	*pNMHDR = reinterpret_cast<NMHDR *>(lParam); 
			switch (pNMHDR->code) {
			case CSplineWnd::NM_SPLINE_PAN:
				pDoc->m_arrSpline.SetPan(m_wndSpline.GetPan());
				break;
			case CSplineWnd::NM_SPLINE_ZOOM:
			case CSplineWnd::NM_SPLINE_SELECTION:
			case CSplineWnd::NM_SPLINE_STYLE:
			case CSplineWnd::NM_SPLINE_GRID_SETUP:
				// copy spline properties to document
				pDoc->m_arrSpline.CSplineProperties::operator=(m_wndSpline);
				break;
			case CSplineWnd::NM_SPLINE_CHANGING:
				if (theApp.m_Options.m_bViewSplineDrag) {	// if updating view during spline drag
					pDoc->UpdateAllViews(NULL, CPotterDrawDoc::HINT_SPLINE_DRAG);	// update view
					m_wndSpline.RedrawWindow();	// redraw spline explicitly to avoid paint lag
				}
				break;
			default:
				pDoc->NotifyUndoableEdit(pNMHDR->code, UCODE_SPLINE);
				m_wndSpline.GetState(pDoc->m_arrSpline);	//  copy spline to document
				pDoc->SetModifiedFlag();
				CView	*pSender = reinterpret_cast<CView *>(this);
				pDoc->UpdateAllViews(pSender, CPotterDrawDoc::HINT_SPLINE);
			}
		}
		return TRUE;
	}
	return CDockablePane::OnNotify(wParam, lParam, pResult);
}

BOOL CSplineBar::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message) {
	case WM_KEYDOWN:
		if (GetKeyState(VK_CONTROL) & GKS_DOWN) {	// if control key down
			int	nCmd;	// override view's zoom accelerators for spline window
			switch (pMsg->wParam) {
			case VK_OEM_PLUS:
				nCmd = ID_SPLINE_ZOOM_IN;
				break;
			case VK_OEM_MINUS:
				nCmd = ID_SPLINE_ZOOM_OUT;
				break;
			case '0':
				nCmd = ID_SPLINE_ZOOM_RESET;
				break;
			default:
				nCmd = 0;
			}
			if (nCmd) {
				m_wndSpline.SendMessage(WM_COMMAND, nCmd);
				return TRUE;
			}
		}
		break;
	}
	return CDockablePane::PreTranslateMessage(pMsg);
}

LRESULT CSplineBar::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
	theApp.WinHelp(GetDlgCtrlID());
	return TRUE;
}
