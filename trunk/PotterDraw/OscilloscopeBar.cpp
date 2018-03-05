// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      30mar17	initial version
		01		20feb18	refactor Update to allow secondary modulation
		
*/

#include "stdafx.h"
#include "PotterDraw.h"
#include "OscilloscopeBar.h"
#include "MainFrm.h"
#include "PotterDrawDoc.h"
#include "PotterDrawView.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// COscilloscopeBar

COscilloscopeBar::COscilloscopeBar()
{
	m_Plot.SetMargins(CRect(0, 16, 16, 0));
	m_Plot.SetFitToData(0);
	m_Plot.SetDragMask(CPlotCtrl::AXM_HORZ);
	m_bIsVisible = false;
	m_bShowAllModulations = false;
}

COscilloscopeBar::~COscilloscopeBar()
{
}

void COscilloscopeBar::SetShowAllModulations(bool bEnable)
{
	if (bEnable == m_bShowAllModulations)	// if already in requested state
		return;	// nothing to do
	m_bShowAllModulations = bEnable;
	if (!bEnable)
		m_Plot.SetSeriesCount(1);
	Update();
}

void COscilloscopeBar::Update(bool bFitToData)
{
	CPotterDrawView	*pView = theApp.GetMainFrame()->GetActiveMDIView();
	CDblRange	range(0, 0);
	if (m_bShowAllModulations) {	// if showing all modulations
		if (pView != NULL) {	// if active view exists
			CPotterDrawDoc	*pDoc = pView->GetDocument();
			CBoundArray<int, CPotProperties::PROPERTIES>	arrModIdx;
			pView->GetModulations(arrModIdx);
			int	iModTarget = pDoc->m_iModTarget;
			// if current target isn't modulated, plot it anyway
			if (iModTarget >= 0 && !pDoc->IsModulated(iModTarget))
				arrModIdx.Add(iModTarget);	// add target to modulation list
			int	nMods = arrModIdx.GetSize();
			m_Plot.SetSeriesCount(nMods);
			CDblRange	rangeMod;
			for (int iMod = 0; iMod < nMods; iMod++) {	// for each modulation
				int	iProp = arrModIdx[iMod];
				CDblRange	*pRange;
				if (bFitToData)	// if computing range
					pRange = &rangeMod;
				else
					pRange = NULL;
				pView->PlotProperty(iProp, m_Plot.GetSeries(iMod).m_Point, pRange);
				if (bFitToData) {	// if active view exists
					if (range.IsNull())
						range = rangeMod;
					else
						range.Include(rangeMod);
				}
			}
		} else {	// no active view
			m_Plot.SetSeriesCount(1);
			m_Plot.GetSeries(0).m_Point.SetSize(0);	// empty series
		}
	} else {	// showing current modulation target only; optimized case
		if (pView != NULL) {	// if active view exists
			CPotterDrawDoc	*pDoc = pView->GetDocument();
			int	iModTarget = pDoc->m_iModTarget;
			if (iModTarget >= 0) {	// if valid modulation target
				CDblRange	*pRange;
				if (bFitToData)	// if computing range
					pRange = &range;
				else
					pRange = NULL;
				int	iModObj = CPotProperties::MakeModulationIdx(iModTarget, pDoc->m_iModType);
				pView->PlotProperty(iModObj, m_Plot.GetSeries(0).m_Point, pRange);
			}
		} else {	// no active view
			m_Plot.GetSeries(0).m_Point.SetSize(0);	// empty series
		}
	}
	if (bFitToData) {	// if computing range
		double	fMargin, fMarginFrac = 0.05;
		if (range.IsEmpty())
			fMargin = 1;
		else
			fMargin = range.Length() * fMarginFrac;
		range.Start -= fMargin;
		range.End += fMargin;
		m_Plot.SetRange(CPlotCtrl::RULER_BOTTOM, range);
		m_Plot.Update();
	} else	// using existing range
		m_Plot.UpdateAndInvalidatePlot();	// avoids ruler flicker
}

void COscilloscopeBar::ShowPane(BOOL bShow, BOOL bDelay, BOOL bActivate)
{
	if (bShow)
		Update();
	CDockablePane::ShowPane(bShow, bDelay, bActivate);
}

BEGIN_MESSAGE_MAP(COscilloscopeBar, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_SHOWWINDOW()
	ON_WM_SETFOCUS()
	ON_WM_PARENTNOTIFY()
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COscilloscopeBar message handlers

int COscilloscopeBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	UINT	dwStyle = CPlotCtrl::DEFAULT_STYLE;
	UINT	nPlotID = 2100;
	if (!m_Plot.Create(dwStyle, CRect(0, 0, 0, 0), this, nPlotID))
		return -1;
	m_Plot.SendMessage(WM_SETFONT, WPARAM(GetStockObject(DEFAULT_GUI_FONT)));
	m_Plot.SetRange(CPlotCtrl::RULER_LEFT, CDblRange(0, 1));
	m_Plot.SetVisibleRulers(CPlotCtrl::RM_LEFT | CPlotCtrl::RM_BOTTOM);
	m_Plot.SetSeriesCount(1);

	return 0;
}

BOOL COscilloscopeBar::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void COscilloscopeBar::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	if (m_Plot.m_hWnd)
		m_Plot.MoveWindow(0, 0, cx, cy);
}

void COscilloscopeBar::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDockablePane::OnShowWindow(bShow, nStatus);
	m_bIsVisible = bShow;
}

void COscilloscopeBar::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_Plot.SetFocus();
}

void COscilloscopeBar::OnParentNotify(UINT message, LPARAM lParam)
{
	CDockablePane::OnParentNotify(message, lParam);
	switch (message) {
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		m_Plot.SetFocus();	// so clicking plot control activates bar
	}
}

LRESULT COscilloscopeBar::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
	theApp.WinHelp(GetDlgCtrlID());
	return TRUE;
}
