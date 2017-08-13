// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      02may17	initial version

*/

// RotateDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Resource.h"
#include "RotateDlg.h"

// CRotateDlg dialog

IMPLEMENT_DYNAMIC(CRotateDlg, CDialog)

CRotateDlg::CRotateDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRotateDlg::IDD, pParent)
{
}

CRotateDlg::~CRotateDlg()
{
}

void CRotateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_ROTATE_X, m_vRotation.x);
	DDX_Text(pDX, IDC_ROTATE_Y, m_vRotation.y);
	DDX_Text(pDX, IDC_ROTATE_Z, m_vRotation.z);
}

BEGIN_MESSAGE_MAP(CRotateDlg, CDialog)
END_MESSAGE_MAP()

// CRotateDlg message handlers

BOOL CRotateDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if (!m_sCaption.IsEmpty())
		SetWindowText(m_sCaption);

	return TRUE;  // return TRUE unless you set the focus to a control
}
