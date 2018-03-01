// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      30mar17	initial version
		01		05sep17	in Delete and Insert, add OnColorChange
		
*/

#include "stdafx.h"
#include "Resource.h"
#include "PaletteBar.h"
#include "afxcolorpropertysheet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CPaletteBar

// the definition of this MFC class can't be included because it's
// in afxcolorbar.cpp, so copy it here and pray it doesn't change
class CMFCToolBarColorButton : public CMFCToolBarButton {
public:	
	COLORREF m_Color;
	BOOL m_bHighlight;	// this is the only member we care about
	BOOL m_bIsAutomatic;
	BOOL m_bIsOther;
	BOOL m_bIsLabel;
	BOOL m_bIsDocument;
	BOOL m_bIsOtherColor;
	CMFCColorBar* m_pParentBar;
};

COLORREF CPaletteBar::m_cClipboard;

CPaletteBar::CPaletteBar() : m_ColorBar(this), m_PickerDlg(this)
{
}

CPaletteBar::~CPaletteBar()
{
}

void CPaletteBar::GetPalette(CArray<COLORREF, COLORREF>& arrColor) const
{
	m_ColorBar.GetPalette(arrColor);
}

void CPaletteBar::SetPalette(const CArray<COLORREF, COLORREF>& arrColor, int iColor)
{
	m_ColorBar.SetPalette(arrColor);
	SetCurSel(iColor);
}

void CPaletteBar::SetColorCount(int nColors)
{
	m_ColorBar.SetColorCount(nColors);
}

void CPaletteBar::SetCurSel(int iColor)
{
	iColor = min(iColor, GetColorCount() - 1);
	m_ColorBar.SetCurSel(iColor);
	if (iColor >= 0) {
		COLORREF	c = m_ColorBar.GetColor(iColor);
		m_PickerDlg.SetColor(c);
	}
}

void CPaletteBar::NotifyPaletteChange(int iColor, COLORREF c)
{
	AfxGetMainWnd()->SendMessage(UWM_PALETTE_CHANGE, iColor, c);
}

CPaletteBar::CMyColorBar::CMyColorBar(CPaletteBar *pPaletteBar)
{
	m_pPaletteBar = pPaletteBar;
	m_iCurSel = 0;
	m_nDragState = DS_NONE;
	m_ptDragOrigin = CPoint(0, 0);
	m_ptContextMenu = CPoint(0, 0);
	m_bInContextMenu = false;
}

void CPaletteBar::CMyColorBar::Serialize(CArchive& ar)
{
	// don't serialize or stupid stuff happens
}

BOOL CPaletteBar::CMyColorBar::OnSendCommand(const CMFCToolBarButton* pButton)
{
	// base class implementation interferes with selection so skip it
	return true;
}

void CPaletteBar::CMyColorBar::SetColorCount(int nColors)
{
	m_colors.SetSize(nColors);
	OnColorChange();
}

void CPaletteBar::CMyColorBar::SetCurSel(int iColor)
{
	m_iCurSel = iColor;
	int iButton = 0;
	for (POSITION pos = m_Buttons.GetHeadPosition(); pos != NULL; iButton++) {
		CObject* pObj = m_Buttons.GetNext(pos);
		if (pObj != NULL) {
			CMFCToolBarColorButton* pButton = reinterpret_cast<CMFCToolBarColorButton*>(pObj);
			pButton->m_bHighlight = (iButton == iColor);
		}
	}
	m_iHighlighted = iColor;
	Invalidate();
	AfxGetMainWnd()->SendMessage(UWM_PALETTE_SELECTION, iColor);
}

void CPaletteBar::CMyColorBar::SetColor(int iColor, COLORREF c)
{
	m_colors[iColor] = c;
	OnColorChange();
}

void CPaletteBar::CMyColorBar::GetPalette(CArray<COLORREF, COLORREF>& arrColor) const
{
	arrColor.Copy(m_colors);
}

void CPaletteBar::CMyColorBar::SetPalette(const CArray<COLORREF, COLORREF>& arrColor)
{
	m_colors.Copy(arrColor);
	OnColorChange();
}

CMFCToolBarButton* CPaletteBar::CMyColorBar::GetButton(int iColor)
{
	int	iButton = 0;
	for (POSITION pos = m_Buttons.GetHeadPosition(); pos != NULL; iButton++) {
		CMFCToolBarButton* pButton = reinterpret_cast<CMFCToolBarButton*>(m_Buttons.GetNext(pos));
		if (iButton == iColor)
			return pButton;
	}
	return NULL;
}

void CPaletteBar::CMyColorBar::OnColorChange()
{
	CreatePalette(m_colors, m_Palette);
	Rebuild();
	AdjustLayout();
	if (m_iCurSel >= GetColorCount())
		m_iCurSel = -1;
	SetCurSel(m_iCurSel);
}

void CPaletteBar::CMyColorBar::Insert(COLORREF c)
{
	int	iSel = GetCurSel();
	if (iSel < 0)
		iSel = GetColorCount();
	m_colors.InsertAt(iSel, c);
	SetCurSel(iSel);
	OnColorChange();
	NotifyPaletteChange();
}

void CPaletteBar::CMyColorBar::Copy()
{
	int	iSel = GetCurSel();
	if (iSel >= 0)
		m_cClipboard = GetColor(iSel);
}

void CPaletteBar::CMyColorBar::Cut()
{
	Copy();
	Delete();
}

void CPaletteBar::CMyColorBar::Paste()
{
	Insert(m_cClipboard);
}

void CPaletteBar::CMyColorBar::Insert()
{
	Insert(0);
}

void CPaletteBar::CMyColorBar::Delete()
{
	int	iSel = GetCurSel();
	if (iSel >= 0)  {
		m_colors.RemoveAt(iSel);
		OnColorChange();
		NotifyPaletteChange();
	}
}

void CPaletteBar::CMyColorBar::GetContextMenuSelection()
{
	if (m_bInContextMenu) {
		CPoint	pt(m_ptContextMenu);
		ScreenToClient(&pt);
		m_iCurSel = HitTest(pt);
	}
}

BOOL CPaletteBar::CMyColorBar::PreTranslateMessage(MSG *pMsg)
{
	switch (pMsg->message) {
	case WM_KEYDOWN:
		switch (pMsg->wParam) {
		case VK_LEFT:
		case VK_UP:
			if (m_iCurSel >= 0) {
				if (m_iCurSel > 0)
					m_iCurSel--;
				else
					m_iCurSel = GetColorCount() - 1;
			} else {
				if (GetColorCount())
					m_iCurSel = 0;
			}
			m_pPaletteBar->SetCurSel(m_iCurSel);
			return TRUE;	// don't dispatch
		case VK_RIGHT:
		case VK_DOWN:
			if (m_iCurSel >= 0) {
				if (m_iCurSel < GetColorCount() - 1)
					m_iCurSel++;
				else
					m_iCurSel = 0;
			} else {
				if (GetColorCount())
					m_iCurSel = 0;
			}
			m_pPaletteBar->SetCurSel(m_iCurSel);
			return TRUE;	// don't dispatch
		case VK_ESCAPE:
			if (m_nDragState == DS_DRAG) {
				ReleaseCapture();
				m_nDragState = DS_NONE;
			}
			return TRUE;	// don't dispatch
		case VK_INSERT:
			Insert();
			return TRUE;	// don't dispatch
		case VK_DELETE:
			Delete();
			return TRUE;	// don't dispatch
		case 'C':
			if (GetKeyState(VK_CONTROL) & GKS_DOWN) {
				Copy();
				return TRUE;	// don't dispatch
			}
			break;
		case 'X':
			if (GetKeyState(VK_CONTROL) & GKS_DOWN) {
				Cut();
				return TRUE;	// don't dispatch
			}
			break;
		case 'V':
			if (GetKeyState(VK_CONTROL) & GKS_DOWN) {
				Paste();
				return TRUE;	// don't dispatch
			}
			break;
		}
	case WM_COMMAND:
		switch (pMsg->wParam) {
		case ID_EDIT_CUT:
			GetContextMenuSelection();	// assume command came from context menu
			Cut();
			return TRUE;	// don't dispatch
		case ID_EDIT_COPY:
			GetContextMenuSelection();
			Copy();
			return TRUE;	// don't dispatch
		case ID_EDIT_PASTE:
			GetContextMenuSelection();
			Paste();
			return TRUE;	// don't dispatch
		case ID_EDIT_INSERT:
			GetContextMenuSelection();
			Insert();
			return TRUE;	// don't dispatch
		case ID_EDIT_DELETE:
			GetContextMenuSelection();
			Delete();
			return TRUE;	// don't dispatch
		}
		break;
	}
	return __super::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CPaletteBar::CMyColorBar, CMFCColorBar)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(UWM_CONTEXT_MENU_DONE, OnContextMenuDone)
END_MESSAGE_MAP()

void CPaletteBar::CMyColorBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();	// not sure why this is necessary
	int	iColor = HitTest(point);
	if (iColor >= 0) {	// if valid selection
		m_pPaletteBar->SetCurSel(iColor);
		m_ptDragOrigin = point;
		m_nDragState = DS_TRACK;	// start tracking mouse for drag
		SetCapture();
	}
	// base class implementation interferes with selection so skip it
}

void CPaletteBar::CMyColorBar::OnLButtonUp(UINT nFlags, CPoint point)
{
	switch (m_nDragState) {
	case DS_TRACK:
		{
			ReleaseCapture();
			m_nDragState = DS_NONE;
		}
		break;
	case DS_DRAG:
		{
			ReleaseCapture();
			m_nDragState = DS_NONE;
			int	iSrc = HitTest(m_ptDragOrigin);
			int	iDst = HitTest(point);
			if (iDst != iSrc && iDst >= 0) {
				COLORREF	c = m_colors[iSrc];
				m_colors.RemoveAt(iSrc);
				m_colors.InsertAt(iDst, c);
				m_iCurSel = iDst;
				OnColorChange();
				NotifyPaletteChange();
			}
		}
		break;
	}
	__super::OnLButtonUp(nFlags, point);
}

void CPaletteBar::CMyColorBar::OnMouseMove(UINT nFlags, CPoint point)
{
	switch (m_nDragState) {
	case DS_TRACK:
		// if cursor has moved far enough from drag origin in either axis
		if (abs(m_ptDragOrigin.x - point.x) > GetSystemMetrics(SM_CXDRAG)
		|| abs(m_ptDragOrigin.y - point.y) > GetSystemMetrics(SM_CYDRAG)) {
			m_nDragState = DS_DRAG;	// enter drag mode
			SetCursor(LoadCursor(AfxGetApp()->m_hInstance, (LPCTSTR)IDC_DRAG_SINGLE));
		}
		break;
	case DS_DRAG:
		SetCursor(LoadCursor(AfxGetApp()->m_hInstance, (LPCTSTR)IDC_DRAG_SINGLE));
		break;
	}
	__super::OnMouseMove(nFlags, point);
}

void CPaletteBar::CMyColorBar::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (point.x == -1 && point.y == -1) {	// if menu triggered via keyboard
		CRect	rc;
		CMFCToolBarButton* pButton = GetButton(GetCurSel());
		if (pButton != NULL)
			rc = pButton->Rect();
		else
			GetClientRect(rc);
		point = rc.CenterPoint();	// center of rectangle
		ClientToScreen(&point);	// convert to screen coords
	}
	CMenu	menu;
	menu.LoadMenu(IDR_PALETTE_BAR);
	CMenu	*pPopup = menu.GetSubMenu(0);
	m_ptContextMenu = point;
	m_bInContextMenu = true;
	pPopup->TrackPopupMenu(0, point.x, point.y, this);
	PostMessage(UWM_CONTEXT_MENU_DONE);	// post message to reset context menu state
}

LRESULT CPaletteBar::CMyColorBar::OnContextMenuDone(WPARAM wParam, LPARAM lParam)
{
	m_bInContextMenu = false;
	return 0;
}

CPaletteBar::CMyColorDlg::CMyColorDlg(CPaletteBar *pPaletteBar)
{
	m_pPaletteBar = pPaletteBar;
	m_bIsDirty = true;
}

BOOL CPaletteBar::CMyColorDlg::OnInitDialog()
{
	BOOL	bResult = __super::OnInitDialog();
	// reuse OK and Cancel buttons as Apply and Insert, and add Delete button
	CWnd*	pApplyBtn = GetDlgItem(IDOK);
	CWnd*	pInsertBtn = GetDlgItem(IDCANCEL);
	ASSERT(pApplyBtn != NULL && pInsertBtn != NULL);
	pApplyBtn->SetWindowText(LDS(IDS_PALETTE_BAR_APPLY));
	pInsertBtn->SetWindowText(LDS(IDS_PALETTE_BAR_INSERT));
	CRect	rApply, rInsert;
	pApplyBtn->GetWindowRect(rApply);
	ScreenToClient(rApply);
	pInsertBtn->GetWindowRect(rInsert);
	ScreenToClient(rInsert);
	CSize	szBtn(rApply.Size());
	int	nBtnGutter = rInsert.top - rApply.bottom;	// space between buttons
	CRect	rDelete(CPoint(rApply.left, rInsert.bottom + nBtnGutter), szBtn);
	DWORD	dwStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
	if (!m_btnDelete.Create(LDS(IDS_PALETTE_BAR_DELETE), dwStyle, rDelete, this, IDC_DELETE))
		AfxThrowResourceException();
	m_btnDelete.SetWindowPos(pInsertBtn, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	CRect	rSelect(CPoint(rApply.left, rDelete.bottom + nBtnGutter), szBtn);
	m_btnColorSelect.MoveWindow(rSelect);
	CString	s;
	m_btnColorSelect.GetWindowText(s);
	s.Remove('&');	// remove mnemonic from color select button
	m_btnColorSelect.SetWindowText(s);
	EnumChildWindows(m_pColourSheetOne->m_hWnd,  EnumChildProc, LPARAM(this));
	m_pPropSheet->SetActivePage(1);	// force creation of sheet two's child controls
	EnumChildWindows(m_pColourSheetTwo->m_hWnd,  EnumChildProc, LPARAM(this));
	m_pPropSheet->SetActivePage(0);	// make sheet one active again
	return bResult;
}

BOOL CALLBACK CPaletteBar::CMyColorDlg::EnumChildProc(HWND hwnd, LPARAM lParam)
{
	SetWindowSubclass(hwnd, HookProc, 0, lParam);
	return TRUE;
}

LRESULT CALLBACK CPaletteBar::CMyColorDlg::HookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	// apply button should only be enabled while new and current colors differ
	CMyColorDlg	*pDlg = reinterpret_cast<CMyColorDlg *>(dwRefData);
	bool	bIsDirty = pDlg->m_NewColor != pDlg->m_CurrentColor;
	if (bIsDirty != pDlg->m_bIsDirty) {	// if dirty state transitioned
		pDlg->GetDlgItem(IDOK)->EnableWindow(bIsDirty);	// enable or disable apply button
		pDlg->m_bIsDirty = bIsDirty;	// update shadow
	}
	if (pDlg->m_btnDelete.m_hWnd)
		pDlg->m_btnDelete.EnableWindow(pDlg->m_pPaletteBar->GetColorCount());
	if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST) {	// if keyboard message
		MSG	msg = {hWnd, uMsg, wParam, lParam};	// give main frame a try
		if (AfxGetMainWnd()->SendMessage(UWM_HANDLEDLGKEY, reinterpret_cast<WPARAM>(&msg)))
			return 0;	// key was handled so don't process further
	} else if (uMsg == WM_LBUTTONDBLCLK) {	// if left double-click
		if (::IsChild(pDlg->m_pColourSheetOne->m_hWnd, hWnd))	// if standard color sheet
			pDlg->m_pPaletteBar->OnApply();	// apply change, imitating modal behavior
		return 0;	// base class closes dialog which is unhelpful in tool bar
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

BOOL CPaletteBar::CMyColorDlg::PreTranslateMessage(MSG *pMsg)
{
	if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST) {
		//  give main frame a try
		if (AfxGetMainWnd()->SendMessage(UWM_HANDLEDLGKEY, reinterpret_cast<WPARAM>(pMsg)))
			return TRUE;	// key was handled so don't process further
	}
	return __super::PreTranslateMessage(pMsg);
}

void CPaletteBar::CMyColorDlg::SetOriginalColor(COLORREF c)
{
	m_wndColors.SetOriginalColor(c); 
	m_wndColors.Invalidate();
}

void CPaletteBar::CMyColorDlg::SetColor(COLORREF c)
{
	SetNewColor(c);
	SetCurrentColor(c);
	SetOriginalColor(c);
	BYTE	r = GetRValue(c);
	BYTE	g = GetGValue(c);
	BYTE	b = GetBValue(c);
	SetPageOne(r, g, b);
	SetPageTwo(r, g, b);
}

BEGIN_MESSAGE_MAP(CPaletteBar::CMyColorDlg, CMFCColorDialog)
	ON_COMMAND(IDC_DELETE, OnDelete)
END_MESSAGE_MAP()

void CPaletteBar::CMyColorDlg::OnOK()
{
	m_pPaletteBar->OnApply();
}

void CPaletteBar::CMyColorDlg::OnCancel()
{
	m_pPaletteBar->m_ColorBar.Insert();
}

void CPaletteBar::CMyColorDlg::OnDelete()
{
	m_pPaletteBar->m_ColorBar.Delete();
}

BOOL CPaletteBar::CreatePalette(const CArray<COLORREF, COLORREF>& arrColor, CPalette& palette)
{
	if (palette.GetSafeHandle() != NULL) {
		::DeleteObject(palette.Detach());
		ENSURE(palette.GetSafeHandle() == NULL);
	}
	int	nColors = INT64TO32(arrColor.GetSize());
	CByteArrayEx	ba;
	// LOGPALETTE struct's palette entry array contains only one element
	ba.SetSize(sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * (nColors - 1));
	LOGPALETTE	*pLogPalette = reinterpret_cast<LOGPALETTE *>(ba.GetData());
	pLogPalette->palVersion = 0x300;	// magic number
	pLogPalette->palNumEntries = static_cast<WORD>(nColors);
	for (int iColor = 0; iColor < nColors; iColor++) {	// for each color
		PALETTEENTRY	*pEntry  = &pLogPalette->palPalEntry[iColor];
		pEntry->peRed = GetRValue(arrColor[iColor]);
		pEntry->peGreen = GetGValue(arrColor[iColor]);
		pEntry->peBlue = GetBValue(arrColor[iColor]);
		pEntry->peFlags = 0;
	}
	return palette.CreatePalette(pLogPalette);
}

void CPaletteBar::OnApply()
{
	COLORREF	c = m_PickerDlg.GetColor();
	int	iColor = GetCurSel();
	if (iColor >= 0) {	// if valid selection
		m_PickerDlg.SetOriginalColor(c);
		if (c != m_ColorBar.GetColor(iColor)) {	// if color changed
			m_ColorBar.SetColor(iColor, c);
			NotifyPaletteChange(iColor, c);
		}
	}
}

LRESULT CPaletteBar::SendColorBarCommand(WPARAM nCmdID)
{
	MSG	msg = {m_hWnd, WM_COMMAND, nCmdID};
	return m_ColorBar.PreTranslateMessage(&msg);
}

BEGIN_MESSAGE_MAP(CPaletteBar, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPaletteBar message handlers

int CPaletteBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_PickerDlg.Create(CMFCColorDialog::IDD, this))
		return -1;

	// order matters here to avoid flicker: change style first, then set parent
	// remove caption and change from popup to child; also remove dialog frame
	m_PickerDlg.ModifyStyle(WS_CAPTION | WS_POPUP, WS_CHILD);
	m_PickerDlg.ModifyStyleEx(WS_EX_DLGMODALFRAME, 0);
	m_PickerDlg.SetParent(this);

	CRect	rPicker;
	m_PickerDlg.GetClientRect(rPicker);
	m_szPicker = rPicker.Size();

	int	nInitColors = 5;
	int	nSwatchSize = 50;
	CArray<COLORREF, COLORREF>	arrColor;
	arrColor.SetSize(nInitColors);
	CreatePalette(arrColor, m_Palette);
	CRect	rPalette(0, 0, nInitColors * nSwatchSize, nSwatchSize);
	if (!m_ColorBar.CreateControl(this, rPalette, UINT(-1), -1, &m_Palette))
		return -1;
	m_ColorBar.ModifyStyle(0, WS_TABSTOP);	// works from color bar to picker but not vice versa
	m_ColorBar.ContextToSize(TRUE, FALSE);	// square buttons, don't center

	return 0;
}

BOOL CPaletteBar::OnEraseBkgnd(CDC* pDC)
{
	return CWnd::OnEraseBkgnd(pDC);
}

void CPaletteBar::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	if (m_PickerDlg.m_hWnd)
		m_PickerDlg.MoveWindow(CRect(CPoint(0, 0), m_szPicker));
	if (m_ColorBar.m_hWnd)
		m_ColorBar.MoveWindow(CRect(0, m_szPicker.cy, cx, cy));
}

LRESULT CPaletteBar::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
	AfxGetApp()->WinHelp(GetDlgCtrlID());
	return TRUE;
}
