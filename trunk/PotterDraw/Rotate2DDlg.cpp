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

// Rotate2DDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "Rotate2DDlg.h"

// CRotate2DDlg dialog

IMPLEMENT_DYNAMIC(CRotate2DDlg, CDialog)

CRotate2DDlg::CRotate2DDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD, pParent)
	, m_fAngle(0)
	, m_iRelativeTo(0)
{

}

CRotate2DDlg::~CRotate2DDlg()
{
}

void CRotate2DDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_ROTATE_ANGLE, m_fAngle);
	DDX_Radio(pDX, IDC_SCALE_RELATIVE_TO, m_iRelativeTo);
	if (pDX->m_bSaveAndValidate) {
		if (m_fAngle == 0) {
			AfxMessageBox(IDS_DDV_VAL_CANT_BE_ZERO);
			DDV_Fail(pDX, IDC_ROTATE_ANGLE);
		}
	}
}


BEGIN_MESSAGE_MAP(CRotate2DDlg, CDialog)
END_MESSAGE_MAP()


// CRotate2DDlg message handlers

