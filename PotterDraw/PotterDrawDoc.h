// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda

		revision history:
		rev		date	comments
        00      12mar17	initial version
		01		05sep17	add spline drag hint
		02		06nov17	add lighting

*/

// PotterDrawDoc.h : interface of the CPotterDrawDoc class
//

#pragma once

#include "PotProperties.h"
#include "Undoable.h"

class CPotterDrawDoc : public CDocument, public CPotProperties, public CUndoable
{
// Construction
protected: // create from serialization only
	CPotterDrawDoc();
	DECLARE_DYNCREATE(CPotterDrawDoc)

// Constants
	enum {	// update hints
		HINT_NONE,			// no hint
		HINT_PROPERTY,		// property edit; pHint points to CPropertyHint object
		HINT_PALETTE,		// palette edit
		HINT_MODULATION,	// modulation edit; pHint points to CModulationHint object
		HINT_SPLINE,		// spline edit
		HINT_SPLINE_DRAG,	// spline drag
		HINT_LIGHTING,		// lighting edit
		HINTS
	};
	static const LPCTSTR	m_arrTextureFileExt[];	// array of texture file extensions
	static const int	m_nTextureFileExts;	// number of texture file extensions

// Types
	class CPropertyHint : public CObject {
	public:
		CPropertyHint() : m_iProp(0) {}
		CPropertyHint(int iProp) : m_iProp(iProp) {}
		int		m_iProp;		// property index
	};
	class CModulationHint : public CObject {
	public:
		CModulationHint() : m_iTarget(0), m_iModProp(0) {}
		CModulationHint(int iTarget, int iModProp) : m_iTarget(iTarget), m_iModProp(iModProp) {}
		int		m_iTarget;		// index of target property
		int		m_iModProp;		// modulation property index
	};

// Attributes
public:

// Operations
public:
	void	OnPropertyEdit(int iProp);
	void	LoadTexture(LPCTSTR szPath);

// Overrides
public:
	BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CPotterDrawDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Member data
	CUndoManager	m_UndoMgr;	// undo manager instance

protected:
// Overrides
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual	void	SaveUndoState(CUndoState& State);
	virtual	void	RestoreUndoState(const CUndoState& State);
	virtual	CString	GetUndoTitle(const CUndoState& State);

// Types
	class CUndoPalette : public CRefObj {
	public:
		CArrayEx<COLORREF, COLORREF>	m_arrPalette;	// array of colors
		int		m_iCurSel;		// index of currently selected color
	};
	class CUndoString : public CRefObj {
	public:
		CString	m_str;
	};
	class CUndoSpline : public CRefObj {
	public:
		CSplineState	m_arrSpline;
	};
	class CUndoLighting : public CRefObj {
	public:
		D3DVECTOR	m_vLightDir;
		D3DMATERIAL9	m_mtrlPot;
	};

// Helpers
	void	SavePalette(CUndoState& State, int iColor = -1) const;
	void	RestorePalette(const CUndoState& State, int iColor = -1);
	void	SaveString(CUndoState& State, const CString& str) const;
	void	RestoreString(const CUndoState& State, CString& str);
	void	RelaySplineCmd(WPARAM nCmdID);
	void	RelaySplineUpdateCmdUI(CCmdUI *pCmdUI);
	void	RelayEditCmd(WPARAM nCmdID);
	void	RelayEditUpdateCmdUI(CCmdUI *pCmdUI);

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnUpdateEditDelete(CCmdUI *pCmdUI);
	afx_msg void OnEditSelectAll();
	afx_msg void OnEditDeselect();
	afx_msg void OnEditDelete();
	afx_msg void OnUpdateEditSelectAll(CCmdUI *pCmdUI);
	afx_msg void OnEditCut();
	afx_msg void OnUpdateEditCut(CCmdUI *pCmdUI);
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI *pCmdUI);
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateEditPaste(CCmdUI *pCmdUI);
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateEditUndo(CCmdUI *pCmdUI);
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateEditRedo(CCmdUI *pCmdUI);
	afx_msg	void OnSplineCmd(UINT nID);
	afx_msg void OnUpdateSplineCmd(CCmdUI *pCmdUI);
	afx_msg void OnFileLoadTexture();
};
