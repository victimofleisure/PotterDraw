// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      04may17	initial version
		
*/

#pragma once

#include "PropertiesGrid.h"
#include "ModulationProps.h"

class CModulationBar : public CDockablePane
{
// Construction
public:
	CModulationBar();

// Attributes
public:
	void	SetInitialProperties(const CModulationProps& Props);
	void	GetProperties(CModulationProps& Props) const;
	void	SetProperties(const CModulationProps& Props);
	void	EnableDescriptionArea(bool bEnable = true);
	int		GetCurSel() const;
	void	SetCurSel(int iProp);
	void	EnableModulation(bool bEnable);

protected:
// Types
	class CMyPropertiesGridCtrl : public CPropertiesGridCtrl {
	public:
		virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;
	};
	typedef CArrayEx<CMFCPropertyGridProperty *, CMFCPropertyGridProperty *> CPropertyPtrArray;

// Data members
	const CProperties	*m_pInitialProps;	// pointer to initial properties
	CMyPropertiesGridCtrl m_Grid;			// derived properties grid control
	bool	m_bEnableModulation;			// true if modulation properties are enabled

// Implementation
public:
	virtual ~CModulationBar();

protected:
// Helpers
	void	InitPropList(const CProperties& Props);
	void	AdjustLayout();

// Generated message map functions
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnExpandAllProperties();
	afx_msg void OnUpdateExpandAllProperties(CCmdUI* pCmdUI);
	afx_msg void OnSortProperties();
	afx_msg void OnUpdateSortProperties(CCmdUI* pCmdUI);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnDestroy();
	afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
};

inline void CModulationBar::EnableDescriptionArea(bool bEnable)
{
	m_Grid.EnableDescriptionArea(bEnable);
}

inline int CModulationBar::GetCurSel() const
{
	return m_Grid.GetCurSelIdx();
}

inline void CModulationBar::SetCurSel(int iProp)
{
	m_Grid.SetCurSelIdx(iProp);
}
