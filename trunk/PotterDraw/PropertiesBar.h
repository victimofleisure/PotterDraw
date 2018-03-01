// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      23mar17	initial version
		
*/

#pragma once

#include "PropertiesGrid.h"
#include "PotProperties.h"

class CPropertiesBar : public CDockablePane
{
// Construction
public:
	CPropertiesBar();

// Attributes
public:
	void	SetInitialProperties(const CPotProperties& Props);
	void	GetProperties(CPotProperties& Props) const;
	void	SetProperties(const CPotProperties& Props);
	void	EnableDescriptionArea(bool bEnable = true);
	int		GetCurSel() const;
	void	SetCurSel(int iProp);

protected:
// Types
	class CMyPropertiesGridCtrl : public CPropertiesGridCtrl {
	public:
		CMyPropertiesGridCtrl();
		virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;
		virtual void OnChangeSelection(CMFCPropertyGridProperty* pNewSel, CMFCPropertyGridProperty* pOldSel);
		virtual int OnDrawProperty(CDC* pDC, CMFCPropertyGridProperty* pProp) const;
		void	UpdateModulationIndicators();

	protected:
		enum {	// modulation indicators
			MI_NONE,
			MI_MODULATED,
			MI_PAUSED,
			MI_ANIMATED,
			MODULATION_STATES
		};
		BYTE	m_iModInd[CPotProperties::PROPERTIES];	// modulation indicator shadows
		void	DrawModulationIndicator(CDC* pDC, int iProp, int iModInd) const;
	};
	typedef CArrayEx<CMFCPropertyGridProperty *, CMFCPropertyGridProperty *> CPropertyPtrArray;

// Data members
	const CProperties	*m_pInitialProps;	// pointer to initial properties
	CMyPropertiesGridCtrl m_Grid;			// derived properties grid control

// Implementation
public:
	virtual ~CPropertiesBar();

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

inline void CPropertiesBar::EnableDescriptionArea(bool bEnable)
{
	m_Grid.EnableDescriptionArea(bEnable);
}

inline int CPropertiesBar::GetCurSel() const
{
	return m_Grid.GetCurSelIdx();
}

inline void CPropertiesBar::SetCurSel(int iProp)
{
	m_Grid.SetCurSelIdx(iProp);
}
