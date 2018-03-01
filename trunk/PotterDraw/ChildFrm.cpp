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

// ChildFrm.cpp : implementation of the CChildFrame class
//

#include "stdafx.h"
#include "PotterDraw.h"
#include "ChildFrm.h"
#include "MainFrm.h"
#include "PotterDrawView.h"
#include "Persist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define RK_CHILD_FRAME	_T("ChildFrame")

// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWndEx)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWndEx)
	ON_WM_MDIACTIVATE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

// CChildFrame construction/destruction

CChildFrame::CChildFrame()
{
}

CChildFrame::~CChildFrame()
{
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CMDIChildWndEx::PreCreateWindow(cs))
		return FALSE;

	return TRUE;
}

// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
	CMDIChildWndEx::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWndEx::Dump(dc);
}
#endif //_DEBUG

// CChildFrame message handlers

void CChildFrame::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd) 
{
	CMDIChildWndEx::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);
	if (bActivate) {	// if activating
		theApp.GetMainFrame()->OnActivateView(GetActiveView());	// notify main frame
	} else {	// deactivating
		if (pActivateWnd == NULL)	// if no document
			theApp.GetMainFrame()->OnActivateView(NULL);	// notify main frame
	}
}

void CChildFrame::OnDestroy() 
{
	// save maximize setting in registry
	CPersist::SaveWnd(REG_SETTINGS, this, RK_CHILD_FRAME);
	CMDIChildWnd::OnDestroy();
}

void CChildFrame::ActivateFrame(int nCmdShow) 
{
	if (GetMDIFrame()->MDIGetActive())
		CMDIChildWnd::ActivateFrame(nCmdShow); 
	else {
		int	RegShow = CPersist::GetWndShow(REG_SETTINGS, RK_CHILD_FRAME);
		if (RegShow == SW_SHOWMAXIMIZED)
			nCmdShow = SW_SHOWMAXIMIZED;
		CMDIChildWnd::ActivateFrame(nCmdShow);
	}
}
