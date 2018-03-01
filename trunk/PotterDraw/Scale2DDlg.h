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

#include "DPoint.h"

// CScale2DDlg dialog

class CScale2DDlg : public CDialog
{
	DECLARE_DYNAMIC(CScale2DDlg)

public:
	CScale2DDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CScale2DDlg();

// Dialog Data
	enum { IDD = IDD_SCALE_2D };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	DPoint m_ptScale;
	int m_iRelativeTo;
	BOOL m_bProportionally;
	afx_msg LRESULT OnKickIdle(WPARAM wParam, LPARAM lParam);
	afx_msg void OnUpdateWidth(CCmdUI *pCmdUI);
	afx_msg void OnUpdateHeight(CCmdUI *pCmdUI);
};
