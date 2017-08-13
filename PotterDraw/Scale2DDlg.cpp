// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda

		revision history:
		rev		date	comments
        00      01jul17	initial version

*/

// Scale2DDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "Scale2DDlg.h"

// CScale2DDlg dialog

IMPLEMENT_DYNAMIC(CScale2DDlg, CDialog)

CScale2DDlg::CScale2DDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD, pParent)
	, m_ptScale(100, 100)
	, m_iRelativeTo(0)
	, m_bProportionally(TRUE)
{

}

CScale2DDlg::~CScale2DDlg()
{
}

void CScale2DDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_SCALE_WIDTH, m_ptScale.x);
	DDX_Text(pDX, IDC_SCALE_HEIGHT, m_ptScale.y);
	DDX_Radio(pDX, IDC_SCALE_RELATIVE_TO, m_iRelativeTo);
	DDX_Check(pDX, IDC_SCALE_PROPORTIONALLY, m_bProportionally);
	if (pDX->m_bSaveAndValidate) {
		if (m_ptScale.x == 100 && (m_bProportionally || m_ptScale.y == 100)) {
			AfxMessageBox(IDS_DDV_PCT_CANT_BE_100);
			DDV_Fail(pDX, IDC_SCALE_WIDTH);
		}
		if (m_ptScale.x <= 0 || m_ptScale.x <= 0) {
			AfxMessageBox(IDS_DDV_PCT_MUST_BE_GT_ZERO);
			DDV_Fail(pDX, m_ptScale.x <= 0 ? IDC_SCALE_WIDTH : IDC_SCALE_HEIGHT);
		}
	}
}

BEGIN_MESSAGE_MAP(CScale2DDlg, CDialog)
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_UPDATE_COMMAND_UI(IDC_SCALE_WIDTH_CAPTION, OnUpdateWidth)
	ON_UPDATE_COMMAND_UI(IDC_SCALE_HEIGHT_CAPTION, OnUpdateHeight)
	ON_UPDATE_COMMAND_UI(IDC_SCALE_HEIGHT_UNIT, OnUpdateHeight)
	ON_UPDATE_COMMAND_UI(IDC_SCALE_HEIGHT, OnUpdateHeight)
END_MESSAGE_MAP()

// CScale2DDlg message handlers

LRESULT CScale2DDlg::OnKickIdle(WPARAM wParam, LPARAM lParam)
{
	UpdateDialogControls(this, TRUE);
	return 0;
}

void CScale2DDlg::OnUpdateWidth(CCmdUI *pCmdUI)
{
	int	nID;
	if (!IsDlgButtonChecked(IDC_SCALE_PROPORTIONALLY))
		nID = IDS_SCALE_2D_WIDTH;
	else
		nID = IDS_SCALE_2D_SCALE;
	pCmdUI->SetText(LDS(nID));
}

void CScale2DDlg::OnUpdateHeight(CCmdUI *pCmdUI)
{
	BOOL	bProportionally = IsDlgButtonChecked(IDC_SCALE_PROPORTIONALLY);
	pCmdUI->Enable(!bProportionally);
	if (pCmdUI->m_nID == IDC_SCALE_HEIGHT && bProportionally) {
		CString	s;
		GetDlgItem(IDC_SCALE_WIDTH)->GetWindowText(s);
		pCmdUI->SetText(s);
	}
}
