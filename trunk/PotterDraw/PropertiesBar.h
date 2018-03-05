// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      23mar17	initial version
		01		20feb18	make modulated properties grid control reusable
		
*/

#pragma once

#include "PropertiesGrid.h"
#include "PotProperties.h"

class CModulatedPropertiesGridCtrl : public CPropertiesGridCtrl {
public:
	CModulatedPropertiesGridCtrl();
	enum {	// modulation indicators
		MI_NONE,
		MI_MODULATED,
		MI_PAUSED,
		MI_ANIMATED,
		MODULATION_STATES
	};
	void	SetIndicatorCount(int nProps);
	void	SetIndicators(const BYTE *pbaModInd);
	void	SetHighlight(int iProp);

protected:
	CByteArrayEx	m_baModInd;	// array of modulation indicator shadows
	int		m_iHighlight;	// index of highlighted property, or -1 if none
	void	DrawModulationIndicator(CDC* pDC, int iProp, int iModInd) const;
	virtual int OnDrawProperty(CDC* pDC, CMFCPropertyGridProperty* pProp) const;
};

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
	class CMyPropertiesGridCtrl : public CModulatedPropertiesGridCtrl {
	public:
		virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;
		virtual void OnChangeSelection(CMFCPropertyGridProperty* pNewSel, CMFCPropertyGridProperty* pOldSel);
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
	void	UpdateModulationIndicators();

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
