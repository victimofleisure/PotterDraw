// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      05apr17	initial version
        01      01sep17	add property help
		
*/

#pragma once

#include "PropertiesGrid.h"
#include "Options.h"

// COptionsDlg dialog

class COptionsDlg : public CDialog
{
	DECLARE_DYNAMIC(COptionsDlg)

public:
	COptionsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~COptionsDlg();

protected:
// Dialog Data
	enum { IDD = IDD_OPTIONS };
	CPropertiesGridCtrl	m_Grid;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnClickedResetAll();
	afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
};
