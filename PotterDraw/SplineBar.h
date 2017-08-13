// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      20jun17	initial version
		
*/

#pragma once

#include "SplineWnd.h"

class CSplineBar : public CDockablePane
{
// Construction
public:
	CSplineBar();

// Attributes
	CSplineWnd	m_wndSpline;	// spline window
	static int	GetUndoTitle(int iCode);

// Operations

// Implementation
public:
	virtual ~CSplineBar();

protected:
// Constants
	enum {
		ID_SPLINE	= 1974
	};
	static const int m_nUndoTitleID[];

// Generated message map functions
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
};
