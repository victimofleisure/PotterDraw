// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda
 
		revision history:
		rev		date	comments
		00		16apr17	initial version
 
*/

#pragma once

// CRecordStatusDlg dialog

#include "ModelessDlg.h"
#include "afxcmn.h"
#include "afxwin.h"

class CRecordStatusDlg : public CModelessDlg
{
	DECLARE_DYNAMIC(CRecordStatusDlg)

public:
	CRecordStatusDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRecordStatusDlg();

// Dialog Data
	enum { IDD = IDD_RECORD_STATUS };
	enum {
		TIMER_ID = 1917,
		TIMER_PERIOD = 1000,
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CProgressCtrl m_Progress;
	virtual void OnOK();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	CStatic m_Completed;
	CStatic m_Duration;
};
