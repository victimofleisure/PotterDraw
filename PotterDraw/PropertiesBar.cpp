// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      23mar17	initial version
		01		01sep17	call help directly
		
*/

#include "stdafx.h"
#include "PotterDraw.h"
#include "PropertiesBar.h"
#include "MainFrm.h"
#include "PotterDrawDoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar

#define RK_PROPERTIES_BAR _T("PropertiesBar\\")
#define RK_EXPAND _T("Expand")
#define RK_DESCRIPTION_ROWS _T("DescriptionRows")

CPropertiesBar::CPropertiesBar()
{
	m_pInitialProps = NULL;
}

CPropertiesBar::~CPropertiesBar()
{
}

void CPropertiesBar::SetInitialProperties(const CPotProperties& Props)
{
	m_pInitialProps = &Props;
}

void CPropertiesBar::GetProperties(CPotProperties& Props) const
{
	m_Grid.GetProperties(Props);
}

void CPropertiesBar::SetProperties(const CPotProperties& Props)
{
	m_Grid.SetProperties(Props);
	SetCurSel(Props.m_iModTarget);
	m_Grid.UpdateModulationIndicators();
}

void CPropertiesBar::InitPropList(const CProperties& Props)
{
	m_Grid.EnableHeaderCtrl(FALSE);
//	m_Grid.EnableDescriptionArea();
	m_Grid.SetVSDotNetLook();
//	m_Grid.MarkModifiedProperties();
	m_Grid.InitPropList(Props);
}

CPropertiesBar::CMyPropertiesGridCtrl::CMyPropertiesGridCtrl()
{
	ZeroMemory(m_iModInd, sizeof(m_iModInd));
}

void CPropertiesBar::CMyPropertiesGridCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pProp) const
{
	CValidPropertyGridCtrl::OnPropertyChanged(pProp);
	AfxGetMainWnd()->SendMessage(UWM_PROPERTY_CHANGE, pProp->GetData(), reinterpret_cast<LPARAM>(GetParent()));
}

void CPropertiesBar::CMyPropertiesGridCtrl::OnChangeSelection(CMFCPropertyGridProperty* pNewSel, CMFCPropertyGridProperty* pOldSel)
{
	CValidPropertyGridCtrl::OnChangeSelection(pNewSel, pOldSel);
	int	iProp;
	if (pNewSel != NULL)
		iProp = INT64TO32(pNewSel->GetData());
	else
		iProp = -1;
	AfxGetMainWnd()->SendMessage(UWM_PROPERTY_SELECT, iProp, reinterpret_cast<LPARAM>(GetParent()));
}

void CPropertiesBar::CMyPropertiesGridCtrl::UpdateModulationIndicators()
{
	CPotterDrawDoc	*pDoc = theApp.GetMainFrame()->GetActiveMDIDoc();
	CClientDC	dc(this);
	CRgn rgnList;
	rgnList.CreateRectRgnIndirect(&m_rectList);
	dc.SelectClipRgn(&rgnList);	// clip to current list rectangle to avoid overwriting description
	int	nProps = CPotProperties::PROPERTIES;
	for (int iProp = 0; iProp < nProps; iProp++) {	// for each property
		int	iModInd;
		if (pDoc != NULL && pDoc->IsModulated(iProp)) {
			if (pDoc->m_Mod[iProp].IsAnimated()) {
				if (pDoc->m_bAnimation)
					iModInd = MI_ANIMATED;
				else
					iModInd = MI_PAUSED;
			} else
				iModInd = MI_MODULATED;
		} else
			iModInd = MI_NONE;
		if (iModInd != m_iModInd[iProp]) {	// if modulation indicator changed
			DrawModulationIndicator(&dc, iProp, iModInd);	// draw or erase indicator
			m_iModInd[iProp] = static_cast<BYTE>(iModInd);	// update cached value
		}
	}
}

void CPropertiesBar::CMyPropertiesGridCtrl::DrawModulationIndicator(CDC* pDC, int iProp, int iModInd) const
{
	enum {
		TRI_PTS = 3,
		GUTTER = 2,
	};
	static const SIZE	szTri = {12, 10};	// roughly isosceles
	static const POINT	ptTri[TRI_PTS] = {{0, szTri.cy}, {szTri.cx / 2, 0}, {szTri.cx, szTri.cy}};
	static const COLORREF	clrIndicator[MODULATION_STATES] = {
		0,					// MI_NONE (unused)
		RGB(0, 0, 192),		// MI_MODULATED 
		RGB(192, 128, 0),	// MI_PAUSED
		RGB(0, 192, 0),		// MI_ANIMATED
	};
	const CMFCPropertyGridProperty	*pProp = m_arrProp[iProp];
	CRect	rProp(pProp->GetRect());
	CSize	szProp = rProp.Size();
	CPoint	ptModInd(rProp.left - szTri.cx - GUTTER, rProp.top + (szProp.cy - szTri.cy) / 2);
	if (iModInd) {	// if property is modulated
		CPoint	pt[TRI_PTS];
		for (int iPt = 0; iPt < TRI_PTS; iPt++)	// for each triangle point
			pt[iPt] = ptModInd + ptTri[iPt];
		HGDIOBJ	hPrevBrush = pDC->SelectObject(GetStockObject(DC_BRUSH));
		HGDIOBJ	hPrevPen = pDC->SelectObject(GetStockObject(NULL_PEN));
		pDC->SetDCBrushColor(clrIndicator[iModInd]);
		pDC->Polygon(pt, TRI_PTS);	// draw indicator
		pDC->SelectObject(hPrevBrush);
		pDC->SelectObject(hPrevPen);
	} else {	// property unmodulated
		pDC->FillSolidRect(CRect(ptModInd, szTri), m_clrGray);	// erase indicator
	}
}

int CPropertiesBar::CMyPropertiesGridCtrl::OnDrawProperty(CDC* pDC, CMFCPropertyGridProperty* pProp) const
{
	int	iResult = CValidPropertyGridCtrl::OnDrawProperty(pDC, pProp);
	if (iResult) {
		int	iProp = INT64TO32(pProp->GetData());
		if (iProp >= 0 && m_iModInd[iProp])
			DrawModulationIndicator(pDC, iProp, m_iModInd[iProp]);
	}
	return iResult;
}

BEGIN_MESSAGE_MAP(CPropertiesBar, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_EXPAND_ALL, OnExpandAllProperties)
	ON_UPDATE_COMMAND_UI(ID_EXPAND_ALL, OnUpdateExpandAllProperties)
	ON_COMMAND(ID_SORTPROPERTIES, OnSortProperties)
	ON_UPDATE_COMMAND_UI(ID_SORTPROPERTIES, OnUpdateSortProperties)
	ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar message handlers

void CPropertiesBar::AdjustLayout()
{
	if (GetSafeHwnd () == NULL || (AfxGetMainWnd() != NULL && AfxGetMainWnd()->IsIconic()))
	{
		return;
	}

	CRect	rc;
	GetClientRect(rc);

	m_Grid.SetWindowPos(NULL, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
}

int CPropertiesBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_Grid.Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 0, 0), this, 0))
	{
		TRACE0("Failed to create Properties Grid \n");
		return -1;      // fail to create
	}

	if (m_pInitialProps != NULL)
		InitPropList(*m_pInitialProps);

	m_Grid.RestoreGroupExpansion(RK_PROPERTIES_BAR RK_EXPAND);
	m_Grid.SetDescriptionRows(AfxGetApp()->GetProfileInt(RK_PROPERTIES_BAR, RK_DESCRIPTION_ROWS, 3));

	AdjustLayout();
	return 0;
}

void CPropertiesBar::OnDestroy()
{
	CDockablePane::OnDestroy();
	m_Grid.SaveGroupExpansion(RK_PROPERTIES_BAR RK_EXPAND);
	AfxGetApp()->WriteProfileInt(RK_PROPERTIES_BAR, RK_DESCRIPTION_ROWS, m_Grid.GetActualDescriptionRows());
}

void CPropertiesBar::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CPropertiesBar::OnExpandAllProperties()
{
	m_Grid.ExpandAll();
}

void CPropertiesBar::OnUpdateExpandAllProperties(CCmdUI* /* pCmdUI */)
{
}

void CPropertiesBar::OnSortProperties()
{
	m_Grid.SetAlphabeticMode(!m_Grid.IsAlphabeticMode());
}

void CPropertiesBar::OnUpdateSortProperties(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_Grid.IsAlphabeticMode());
}

void CPropertiesBar::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_Grid.SetFocus();
}

void CPropertiesBar::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
}

LRESULT CPropertiesBar::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
	int	iProp = m_Grid.GetCurSelIdx();
	int	nID = 0;
	if (iProp >= 0 && iProp < CPotProperties::PROPERTIES)	// if valid property index
		nID = CPotProperties::m_Info[iProp].nNameID;	// get property name resource ID
	else
		nID = IDS_PROPERTIES_BAR;
	theApp.WinHelp(nID);
	return TRUE;
}
