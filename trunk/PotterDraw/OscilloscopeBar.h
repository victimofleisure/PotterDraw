// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      30mar17	initial version
		
*/

#pragma once

#include "PlotCtrl.h"

class COscilloscopeBar : public CDockablePane
{
// Construction
public:
	COscilloscopeBar();

// Attributes
public:
	BOOL	FastIsVisible() const;
	bool	GetShowAllModulations() const;
	void	SetShowAllModulations(bool bEnable);

// Operations
	void	Update(bool bFitToData = true);

protected:
// Types

// Data members

// Helpers

// Implementation
public:
	virtual ~COscilloscopeBar();
	virtual void ShowPane(BOOL bShow, BOOL bDelay, BOOL bActivate = TRUE);

protected:
	CPlotCtrl	m_Plot;				// plot control
	BOOL	m_bIsVisible;			// true if bar is visible
	bool	m_bShowAllModulations;	// true if showing all modulations

// Generated message map functions
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
	afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
};

inline BOOL COscilloscopeBar::FastIsVisible() const
{
	return m_bIsVisible;	// fast as can be
}

inline bool COscilloscopeBar::GetShowAllModulations() const
{
	return m_bShowAllModulations;
}
