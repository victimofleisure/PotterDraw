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

// Translate2DDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "Translate2DDlg.h"

// CTranslate2DDlg dialog

IMPLEMENT_DYNAMIC(CTranslate2DDlg, CDialog)

CTranslate2DDlg::CTranslate2DDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD, pParent)
{
	m_pt = DPoint(0, 0);
}

CTranslate2DDlg::~CTranslate2DDlg()
{
}

void CTranslate2DDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_TRANSLATE_X, m_pt.x);
	DDX_Text(pDX, IDC_TRANSLATE_Y, m_pt.y);
	if (pDX->m_bSaveAndValidate) {
		if (m_pt == DPoint(0, 0)) {
			AfxMessageBox(IDS_DDV_VAL_CANT_BE_ZERO);
			DDV_Fail(pDX, IDC_TRANSLATE_X);
		}
	}
}


BEGIN_MESSAGE_MAP(CTranslate2DDlg, CDialog)
END_MESSAGE_MAP()


// CTranslate2DDlg message handlers
