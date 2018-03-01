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

#pragma once

// CGridSetupDlg dialog

class CGridSetupDlg : public CDialog
{
	DECLARE_DYNAMIC(CGridSetupDlg)

public:
	CGridSetupDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CGridSetupDlg();

// Dialog Data
	enum { IDD = IDD_GRID_SETUP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	double m_fGridSpacing;
};
