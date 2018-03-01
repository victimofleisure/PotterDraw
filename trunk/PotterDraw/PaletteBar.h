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

class CPaletteBar : public CDockablePane
{
// Construction
public:
	CPaletteBar();

// Attributes
public:
	void	GetPalette(CArray<COLORREF, COLORREF>& arrColor) const;
	void	SetPalette(const CArray<COLORREF, COLORREF>& arrColor, int iColor = -1);
	int		GetColorCount() const;
	void	SetColorCount(int nColors);
	int		GetCurSel() const;
	void	SetCurSel(int iColor);
	bool	ColorBarHasFocus() const;
	LRESULT	SendColorBarCommand(WPARAM nCmdID);

// Operations
	static	BOOL	CreatePalette(const CArray<COLORREF, COLORREF> & arrColor, CPalette& palette);
	void	OnApply();

protected:
// Types
	class CMyColorBar : public CMFCColorBar {
	public:
// Construction
		CMyColorBar(CPaletteBar *pPaletteBar);

// Attributes
		int		GetColorCount() const;
		void	SetColorCount(int nColors);
		int		GetCurSel() const;
		void	SetCurSel(int iColor);
		COLORREF	GetColor(int iColor) const;
		void	SetColor(int iColor, COLORREF c);
		void	GetPalette(CArray<COLORREF, COLORREF>& arrColor) const;
		void	SetPalette(const CArray<COLORREF, COLORREF>& arrColor);
		CMFCToolBarButton*	GetButton(int iColor);
		void	GetContextMenuSelection();

// Operations
		void	OnColorChange();
		void	Insert(COLORREF c);
		void	Insert();
		void	Delete();
		void	Copy();
		void	Cut();
		void	Paste();

// Overrides
		virtual void Serialize(CArchive& ar);
		virtual	BOOL PreTranslateMessage(MSG *pMsg);
		virtual	BOOL OnSendCommand(const CMFCToolBarButton* pButton);

protected:
// Constants
		enum {	// drag states
			DS_NONE,	// default state
			DS_TRACK,	// tracking mouse for motion exceeding drag threshold
			DS_DRAG,	// dragging
			DRAG_STATES
		};
		enum {	// user messages
			UWM_CONTEXT_MENU_DONE = WM_USER + 2017,
		};

// Data members
		CPaletteBar	*m_pPaletteBar;	// pointer to parent bar
		int		m_iCurSel;			// index of currently selected color
		int		m_nDragState;		// see drag state enum above
		CPoint	m_ptDragOrigin;		// point at which drag began, in client coords
		CPoint	m_ptContextMenu;	// point at which context menu was created, in screen coords
		bool	m_bInContextMenu;	// true if handling context menu command

// Message handlers
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
		afx_msg LRESULT OnContextMenuDone(WPARAM wParam, LPARAM lParam);
		DECLARE_MESSAGE_MAP()
	};
	class CMyColorDlg : public CMFCColorDialog {
	public:
// Construction
		CMyColorDlg(CPaletteBar *pPaletteBar);

// Attributes
		void	SetOriginalColor(COLORREF c);
		void	SetColor(COLORREF c);

protected:
// Constants
		enum {
			IDC_DELETE = 1974,
		};

// Data members
		CPaletteBar	*m_pPaletteBar;	// pointer to parent bar
		CMFCButton	m_btnDelete;	// delete button
		bool	m_bIsDirty;			// true if color change is unapplied

// Overrides
		virtual BOOL OnInitDialog();
		virtual	BOOL PreTranslateMessage(MSG *pMsg);
		virtual	void OnOK();
		virtual	void OnCancel();

// Helpers
		static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam);
		static LRESULT CALLBACK HookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

		afx_msg void OnDelete();
		DECLARE_MESSAGE_MAP()
	};

// Data members
	CSize		m_szPicker;
	CPalette	m_Palette;
	CMyColorBar	m_ColorBar;
	CMyColorDlg	m_PickerDlg;
	static	COLORREF	m_cClipboard;

// Helpers
	static	void	NotifyPaletteChange(int iColor = -1, COLORREF c = 0);

// Implementation
public:
	virtual ~CPaletteBar();

protected:
// Generated message map functions
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
};

inline int CPaletteBar::CMyColorBar::GetColorCount() const
{
	return INT64TO32(m_colors.GetSize());
}

inline int CPaletteBar::CMyColorBar::GetCurSel() const
{
	return m_iCurSel;
}

inline COLORREF CPaletteBar::CMyColorBar::GetColor(int iColor) const
{
	return m_colors[iColor]; 
}

inline int CPaletteBar::GetCurSel() const
{
	return m_ColorBar.GetCurSel();
}

inline int CPaletteBar::GetColorCount() const
{
	return m_ColorBar.GetColorCount();
}

inline bool CPaletteBar::ColorBarHasFocus() const
{
	return ::GetFocus() == m_ColorBar.m_hWnd;
}
