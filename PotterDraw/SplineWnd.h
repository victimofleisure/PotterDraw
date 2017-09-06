// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      13jun17	initial version
 
*/

#pragma once

#include "Spline.h"
#include "RulerCtrl.h"
#include "Clipboard.h"

class CSplineProperties : public CSplineBase {
public:
	CSplineProperties();
	double	m_fZoom;			// zoom as a fraction of normal size
	double	m_fGridSpacing;		// grid spacing, in normalized coords
	CSelection	m_sel;			// current selecton
	UINT	m_nStyle;			// spline style
};

class CSplineState : public CSplineArray, public CSplineProperties {
public:
	void	ReadProfile();
	void	WriteProfile() const;
};

class CSplineWnd : public CWnd, public CSplineProperties {
	DECLARE_DYNAMIC(CSplineWnd);

public:
// Construction
	CSplineWnd();

// Constants
	enum {	// hit test flags
		HTF_NODES_ONLY		= 0x01,	// ignore control points
	};
	enum {	// notification codes
		NM_SPLINE_FIRST = WM_USER,
		#define SPLINEMSGDEF(name) NM_SPLINE_##name,
		#include "SplineMsgDef.h"
		NM_SPLINE_LAST,
	};
	enum {	// styles
		ST_SHOW_AXES		= 0x01,
		ST_SHOW_GRID		= 0x02,
		ST_SNAP_TO_GRID		= 0x04,
		ST_SHOW_RULERS		= 0x08,
		ST_SHOW_HORZ_MIRROR	= 0x10,
	};
	enum {	// color indices
		CLR_LINE,
		CLR_SELECTION,
		CLR_AXIS,
		CLR_GRID,
		CLR_MIRROR,
		COLORS
	};
	enum {	// drag states
		DS_NONE,		// not dragging
		DS_TRACK,		// waiting for drag to begin
		DS_DRAG,		// dragging
		DS_PAN,			// panning view
		DS_CONTEXT,		// tracking context menu
		DRAG_STATES
	};

// Attributes
	const CSplineArray&	GetSpline() const;
	int		GetSegmentCount() const;
	void	GetState(CSplineState& arrSpline) const;
	void	SetState(const CSplineState& arrSpline);
	bool	HasSelection() const;
	void	GetSelection(int& iCurSel, int& nCurSels) const;
	void	SetSelection(int iCurSel, int nCurSels);
	void	MergeSelection(int iCurSel, int nCurSels);
	bool	IsSelected(int iSeg) const;
	void	SelectAll();
	void	Deselect();
	double	GetZoom() const;
	void	SetZoom(double fZoom);
	void	SetZoom(CPoint ptOrigin, double fZoom);
	DPoint	GetPan() const;
	void	SetPan(DPoint ptPan);
	UINT	GetSplineStyle() const;
	void	SetSplineStyle(UINT nStyle);
	double	GetZoomStep() const;
	void	SetZoomStep(double fZoomStep);
	COLORREF	GetColor(int iColor) const;
	void	SetColor(int iColor, COLORREF color);
	int		GetDragState() const;

// Operations
	static	void	MakeSquare(POINT ptCenter, int nSize, CRect& rSquare);
	bool	NodeHitTest(CPoint ptCursor, DPoint ptNode, int nSize) const;
	int		SplineHitTest(CPoint point, int& iPointType, UINT nFlags = 0);
	int		SplineHitTest(const CRect& rBounds, int& nHitSplines);
	void	MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint = TRUE);
	void	ShowRulers(bool bShow);
	void	Translate(DPoint ptTranslation);
	void	Scale(DPoint ptScaling, int iRelativeTo);
	void	Rotate(double fRotation, int iRelativeTo);
	void	DumpSegments() const;

protected:
// Constants
	enum {
		CONTROL_POINT_SIZE = 5,	// size of control point marker, in pixels
		NODE_POINT_SIZE = 7,	// size of node point marker, in pixels
		HIT_TOLERANCE = 4,		// hit test tolerance, in pixels
	};
	enum {	// user messages
		UWM_RESET_HIT_STATE = WM_USER + 3010,
	};
	enum {	// segment type IDs; assumes resource IDs are alpha sorted
		ID_SEGMENT_TYPE_FIRST = ID_SPLINE_SEGMENT_TYPE1_CURVE,
		ID_SEGMENT_TYPE_LAST = ID_SPLINE_SEGMENT_TYPE2_LINE,
	};
	enum {	// node type IDs; assumes resource IDs are alpha sorted
		ID_NODE_TYPE_FIRST = ID_SPLINE_NODE_TYPE1_CUSP,
		ID_NODE_TYPE_LAST = ID_SPLINE_NODE_TYPE3_SYMMETRICAL,
	};
	enum {	// rulers
		RULER_HORZ,
		RULER_VERT,
		RULERS
	};
	static const COLORREF	m_arrDefaultColor[COLORS];

// Types
	struct CLIPBOARD_SPLINE {
		DPoint	ptStart;		// first segment's start node
		CSegment	arrSeg[1];	// array of segments
	};

// Data members
	CSplineArray	m_arrSpline;	// array of spline segments
	int		m_iDragState;		// drag state; see enum above
	UINT	m_nHitFlags;		// virtual key flags at hit
	CPoint	m_ptHit;			// hit point, in client coords
	CRect	m_rHitLineSeg;		// end points of hit line segment
	int		m_iHitSeg;			// index of hit spline segment, or -1 if none
	int		m_iHitPointType;	// index of hit point's type, or -1 if none
	int		m_nRulerWidth;		// ruler width in pixels
	CRulerCtrl	m_arrRuler[RULERS];	// array of rulers
	CFont	m_fontVertical;		// rotated font for vertical ruler
	double	m_fZoomStep;		// zoom step size, as a fraction
	CClipboard	m_Clipboard;	// clipboard instance
	COLORREF	m_arrColor[COLORS];	// array of colors

// Helpers
	void	UpdateSize(CSize szClient);
	void	Notify(int nCode);
	bool	CreateRulers();
	void	UpdateRulers();
	void	ResetHitState();
	void	SetRadioEx(CCmdUI *pCmdUI, UINT nIDFirst, UINT nIDLast, UINT nIDItem);
	static	void	HorzMirror(CPoint ptOrigin, CPointArray& arrPt);

// Message handlers
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnResetHitState(WPARAM wParam, LPARAM lParam);
	afx_msg void OnAddNode();
	afx_msg void OnUpdateAddNode(CCmdUI *pCmdUI);
	afx_msg void OnDeleteNode();
	afx_msg void OnUpdateDeleteNode(CCmdUI *pCmdUI);
	afx_msg void OnSelectAll();
	afx_msg void OnCopy();
	afx_msg void OnUpdateCopy(CCmdUI *pCmdUI);
	afx_msg void OnCut();
	afx_msg void OnPaste();
	afx_msg void OnUpdatePaste(CCmdUI *pCmdUI);
	afx_msg void OnDelete();
	afx_msg void OnUpdateDelete(CCmdUI *pCmdUI);
	afx_msg void OnSegmentType(UINT nID);
	afx_msg void OnUpdateSegmentType(CCmdUI *pCmdUI);
	afx_msg void OnNodeType(UINT nID);
	afx_msg void OnUpdateNodeType(CCmdUI *pCmdUI);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnShowGrid();
	afx_msg void OnUpdateShowGrid(CCmdUI *pCmdUI);
	afx_msg void OnSnapToGrid();
	afx_msg void OnUpdateSnapToGrid(CCmdUI *pCmdUI);
	afx_msg void OnShowRulers();
	afx_msg void OnUpdateShowRulers(CCmdUI *pCmdUI);
	afx_msg void OnShowHorzMirror();
	afx_msg void OnUpdateShowHorzMirror(CCmdUI *pCmdUI);
	afx_msg void OnImport();
	afx_msg void OnExport();
	afx_msg void OnGridSetup();
	afx_msg void OnTranslate();
	afx_msg void OnUpdateTranslate(CCmdUI *pCmdUI);
	afx_msg void OnScale();
	afx_msg void OnRotate();
	afx_msg void OnFlipVertical();
	afx_msg void OnFlipHorizontal();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnZoomIn();
	afx_msg void OnZoomOut();
	afx_msg void OnZoomReset();
};

inline const CSplineArray& CSplineWnd::GetSpline() const
{
	return m_arrSpline;
}

inline int CSplineWnd::GetSegmentCount() const
{
	return m_arrSpline.GetSize();
}

inline bool CSplineWnd::HasSelection() const
{
	return m_sel.m_nSegs > 0;

}
inline bool CSplineWnd::IsSelected(int iSeg) const
{
	return iSeg >= m_sel.m_iSeg && iSeg < m_sel.m_iSeg + m_sel.m_nSegs;
}

inline void CSplineWnd::GetSelection(int& iCurSel, int& nCurSels) const
{
	iCurSel = m_sel.m_iSeg;
	nCurSels = m_sel.m_nSegs;
}

inline double CSplineWnd::GetZoom() const
{
	return m_fZoom;
}

inline DPoint CSplineWnd::GetPan() const
{
	return m_arrSpline.GetPan();
}

inline UINT CSplineWnd::GetSplineStyle() const
{
	return m_nStyle;
}

inline double CSplineWnd::GetZoomStep() const
{
	return m_fZoomStep;
}

inline void CSplineWnd::SetZoomStep(double fZoomStep)
{
	m_fZoomStep = fZoomStep;
}

inline void CSplineWnd::DumpSegments() const
{
	m_arrSpline.DumpSegments();
}

inline COLORREF CSplineWnd::GetColor(int iColor) const
{
	ASSERT(iColor >= 0 && iColor < COLORS);
	return m_arrColor[iColor];
}

inline void CSplineWnd::SetColor(int iColor, COLORREF color)
{
	ASSERT(iColor >= 0 && iColor < COLORS);
	m_arrColor[iColor] = color;
}

inline int CSplineWnd::GetDragState() const
{
	return m_iDragState;
}
