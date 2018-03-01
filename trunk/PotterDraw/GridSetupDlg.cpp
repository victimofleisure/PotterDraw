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

// GridSetupDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "GridSetupDlg.h"

// CGridSetupDlg dialog

IMPLEMENT_DYNAMIC(CGridSetupDlg, CDialog)

CGridSetupDlg::CGridSetupDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGridSetupDlg::IDD, pParent)
	, m_fGridSpacing(1)
{
}

CGridSetupDlg::~CGridSetupDlg()
{
}

void CGridSetupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_GRID_SPACING, m_fGridSpacing);
	DDV_MinMaxDouble(pDX, m_fGridSpacing, 1e-4, 1e4);
}


BEGIN_MESSAGE_MAP(CGridSetupDlg, CDialog)
END_MESSAGE_MAP()


// CGridSetupDlg message handlers
