// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      13jun17	initial version
		01		05sep17	add changing notification
		02		26oct17	in read/write profile, add grid setup
 
*/

#include "stdafx.h"
#include "resource.h"
#include "SplineWnd.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "GridSetupDlg.h"
#include "Translate2DDlg.h"
#include "Scale2DDlg.h"
#include "Rotate2DDlg.h"
#include "RegTempl.h"

IMPLEMENT_DYNAMIC(CSplineWnd, CWnd);

const COLORREF CSplineWnd::m_arrDefaultColor[COLORS] = {
	RGB(0, 0, 0),			// line
	RGB(0, 0, 255),			// selection
	RGB(255, 160, 160),		// axis
	RGB(128, 255, 255),		// grid
	RGB(0, 160, 0),			// mirror
};

// spline dialog keys
#define RK_SPLINE_DLG_SECTION		_T("Spline\\")
#define RK_SPLINE_DLG_TRANSLATE		RK_SPLINE_DLG_SECTION _T("Translate")
#define RK_SPLINE_DLG_SCALE			RK_SPLINE_DLG_SECTION _T("Scale")
#define RK_SPLINE_DLG_ROTATE		RK_SPLINE_DLG_SECTION _T("Rotate")
#define RK_SPLINE_DLG_RELATIVE_TO	_T("iRelativeTo")
#define RK_SPLINE_DLG_TRANSLATE_PT	_T("ptTranslation")
#define RK_SPLINE_DLG_SCALE_PT		_T("ptScale")
#define RK_SPLINE_DLG_SCALE_PROPORTIONALLY	_T("bProportionally")
#define RK_SPLINE_DLG_ROTATE_ANGLE	_T("fAngle")

// read/write profile keys
#define RK_SPLINE_SECTION	_T("Spline")
#define RK_SPLINE_COUNT		_T("nSegs")
#define RK_SPLINE_START_PT	_T("ptStart")
#define RK_SPLINE_PAN		_T("ptPan")
#define RK_SPLINE_ZOOM		_T("fZoom")
#define RK_SPLINE_GRID_SPACING	_T("fGridSpacing")
#define RK_SPLINE_STYLE		_T("nStyle")
#define RK_SPLINE_CTRL_PT_0	_T("ptCtrl[0]")
#define RK_SPLINE_CTRL_PT_1	_T("ptCtrl[1]")
#define RK_SPLINE_END_PT	_T("ptEnd")
#define RK_SPLINE_NODE_TYPE	_T("iNodeType")
#define RK_SPLINE_SEGMENT_TYPE	_T("iSegType")

CSplineProperties::CSplineProperties()
{
	m_fZoom = 0.02;
	m_fGridSpacing = 5;
	m_sel.m_iSeg = -1;
	m_sel.m_nSegs = 0;
	m_nStyle = CSplineWnd::ST_SHOW_AXES;
}

CSplineWnd::CSplineWnd() :
	m_Clipboard(NULL, _T("PotterDrawSpline"))
{
	m_iDragState = DS_NONE;
	m_nHitFlags = 0;
	m_ptHit = CPoint(0, 0);
	m_iHitSeg = -1;
	m_iHitPointType = -1;
	m_nRulerWidth = 0;
	m_fZoomStep = 1.5;
	memcpy(m_arrColor, m_arrDefaultColor, sizeof(m_arrColor));
}

void CSplineWnd::GetState(CSplineState& arrSpline) const
{
	arrSpline.CSplineArray::operator=(m_arrSpline);	// copy spline array
	arrSpline.CSplineProperties::operator=(*this);	// copy spline properties
}

void CSplineWnd::SetState(const CSplineState& arrSpline)
{
	m_arrSpline = arrSpline;	// copy spline array
	// SetSplineStyle calls ShowRulers, which uses current value of m_nStyle for change detection
	SetSplineStyle(arrSpline.m_nStyle);	// apply style change if any, BEFORE copying properties
	CSplineProperties::operator=(arrSpline);	// copy spline properties
	CRect	rc;
	GetClientRect(rc);
	UpdateSize(rc.Size());
}

void CSplineWnd::UpdateSize(CSize szClient)
{
	DPoint	ptOrigin(DPoint(szClient) / 2);
	double	fScale = min(ptOrigin.x, ptOrigin.y);
	m_arrSpline.SetScale(DPoint(fScale, -fScale) * m_fZoom);	// GDI flips y-axis
	m_arrSpline.SetOrigin(ptOrigin);
	Invalidate();
	if (m_arrRuler[RULER_HORZ].m_hWnd)
		UpdateRulers();
}

void CSplineWnd::SetSelection(int iCurSel, int nCurSels)
{
	ASSERT(iCurSel >= 0 || !nCurSels);
	ASSERT(iCurSel + nCurSels <= GetSegmentCount());
	m_sel.m_iSeg = iCurSel;
	m_sel.m_nSegs = nCurSels;
	Invalidate();
	Notify(NM_SPLINE_SELECTION);
}

void CSplineWnd::MergeSelection(int iCurSel, int nCurSels)
{
	if (HasSelection()) {
		if (nCurSels) {
			int	iFirstSel = min(iCurSel, m_sel.m_iSeg);
			int	iLastSel = max(iCurSel + nCurSels - 1, m_sel.m_iSeg + m_sel.m_nSegs - 1);
			SetSelection(iFirstSel, iLastSel - iFirstSel + 1);
		}
	} else
		SetSelection(iCurSel, nCurSels);
}

void CSplineWnd::SelectAll()
{
	if (GetSegmentCount())
		SetSelection(0, GetSegmentCount());
}

void CSplineWnd::Deselect()
{
	SetSelection(-1, 0);
}

void CSplineWnd::MakeSquare(POINT ptCenter, int nSize, CRect& rSquare)
{
	rSquare = CRect(CPoint(ptCenter.x - nSize / 2, ptCenter.y - nSize / 2), CSize(nSize, nSize));
}

bool CSplineWnd::NodeHitTest(CPoint ptCursor, DPoint ptNode, int nSize) const
{
	CRect	r;
	MakeSquare(m_arrSpline.Denormalize(ptNode), nSize + HIT_TOLERANCE, r);
	return r.PtInRect(ptCursor) != 0;
}

int CSplineWnd::SplineHitTest(CPoint point, int& iPointType, UINT nFlags)
{
	if (m_sel.m_nSegs) {	// if segments selected
		int	iEndSeg = m_sel.m_iSeg + m_sel.m_nSegs;
		if (!(nFlags & HTF_NODES_ONLY)) {	// if not restricted to nodes, try control points
			for (int iSeg = m_sel.m_iSeg; iSeg < iEndSeg; iSeg++) {	// for each selected segment
				const CSegment&	seg = m_arrSpline[iSeg];
				if (seg.m_iSegType != ST_LINE) {	// if segment isn't line
					if (NodeHitTest(point, seg.m_ptCtrl[0], CONTROL_POINT_SIZE)) {	// if hit first control point
						iPointType = PT_CTRL_1;
						return iSeg;
					}
					if (NodeHitTest(point, seg.m_ptCtrl[1], CONTROL_POINT_SIZE)) {	// if hit second control point
						iPointType = PT_CTRL_2;
						return iSeg;
					}
				}
			}
		}
		for (int iSeg = m_sel.m_iSeg; iSeg < iEndSeg; iSeg++) {	// for each selected segment
			if (NodeHitTest(point, m_arrSpline.GetStartNode(iSeg), NODE_POINT_SIZE)) {	// if hit start node
				iPointType = PT_START;
				return iSeg;
			}
			if (NodeHitTest(point, m_arrSpline[iSeg].m_ptEnd, NODE_POINT_SIZE)) {	// if hit end node
				iPointType = PT_END;
				return iSeg;
			}
		}
	}
	iPointType = -1;
	CClientDC	dc(this);
	CPointArray	arrSplinePt;
	CPointArray	arrPathPt;
	CByteArray	arrVertexType;
	int	nSegs = GetSegmentCount();
	for (int iSeg = 0; iSeg < nSegs; iSeg++) {	// for each segment
		m_arrSpline.GetPath(dc, iSeg, arrSplinePt, arrPathPt, arrVertexType);
		int	nPts = arrPathPt.GetSize();
		for (int iPt = 0; iPt < nPts - 1; iPt++) {	// for each of path's line segments
			CPoint	p1(arrPathPt[iPt]);
			CPoint	p2(arrPathPt[iPt + 1]);
			if (p1 != p2) {	// if line segment has non-zero length
				CRect	rBounds(p1, p2);	// construct bounding rectangle of line segment
				rBounds.NormalizeRect();
				rBounds.BottomRight() += CPoint(1, 1);	// make bottom and right edges exclusive
				rBounds.InflateRect(HIT_TOLERANCE, HIT_TOLERANCE);	// sloppy but cheap; gives false hits but no false misses
				if (rBounds.PtInRect(point)) {	// if point is within inflated line segment bounds, do expensive precise test
					DPoint	vL(p2 - p1);	// normalize line segment
					double	fLen = vL.Length();	// length of line segment
					double	fArea = abs(vL.y * point.x - vL.x * point.y + p2.x * p1.y - p2.y * p1.x);	// area of triangle
					double	fDist = fArea / fLen;	// distance is area of triangle divided by length of line segment
					if (fDist <= HIT_TOLERANCE) {	// if point's distance from line is within tolerance
						m_rHitLineSeg = CRect(p1, p2);
						const DPoint&	ptStart = m_arrSpline.GetStartNode(iSeg);
						const CSegment&	seg = m_arrSpline[iSeg];
						if (NodeHitTest(point, ptStart, NODE_POINT_SIZE))	// if start node
							iPointType = PT_START;
						if (NodeHitTest(point, seg.m_ptEnd, NODE_POINT_SIZE))	// if end node
							iPointType = PT_END;
						return iSeg;
					}
				}
			}
		}
	}
	return -1;	// background
}

int CSplineWnd::SplineHitTest(const CRect& rBounds, int& nHitSegs)
{
	CClientDC	dc(this);
	CPointArray	arrSplinePt;
	CPointArray	arrPathPt;
	CByteArray	arrVertexType;
	int	nSegs = GetSegmentCount();
	int	iMinHit = INT_MAX;
	int	iMaxHit = -1;
	for (int iSeg = 0; iSeg < nSegs; iSeg++) {	// for each segment
		m_arrSpline.GetPath(dc, iSeg, arrSplinePt, arrPathPt, arrVertexType);
		int	nPts = arrPathPt.GetSize();
		for (int iPt = 0; iPt < nPts; iPt++) {	// for each of path's points
			if (rBounds.PtInRect(arrPathPt[iPt])) {	// if point within bounds
				if (iSeg < iMinHit)
					iMinHit = iSeg;
				if (iSeg > iMaxHit)
					iMaxHit = iSeg;
				break;
			}
		}
	}
	if (iMaxHit < 0) {
		nHitSegs = 0;
		return -1;
	}
	nHitSegs = iMaxHit - iMinHit + 1;
	return iMinHit;
}

void CSplineWnd::ResetHitState()
{
	m_iHitSeg = -1;
	m_iHitPointType = -1;
}

void CSplineWnd::SetZoom(double fZoom)
{
	ASSERT(fZoom > 0);
	m_arrSpline.SetScale(m_arrSpline.GetScale() / m_fZoom * fZoom);
	m_fZoom = fZoom;
	Invalidate();
	Notify(NM_SPLINE_ZOOM);
}

void CSplineWnd::SetZoom(CPoint ptOrigin, double fZoom)
{
	ASSERT(fZoom > 0);
	DPoint	ptScale(m_arrSpline.GetScale());
	double	fDeltaZoom = fZoom / m_fZoom;
	m_arrSpline.SetScale(ptScale * fDeltaZoom);
	DPoint	ptDeltaOrigin(DPoint(ptOrigin) - m_arrSpline.GetOrigin());
	DPoint	ptDeltaPan;
	if (fDeltaZoom == 1)	// if zoom unchanged
		ptDeltaPan = DPoint(0, 0);	// avoid divide by zero below
	else	// correct panning for scaling
		ptDeltaPan = ptDeltaOrigin / (ptScale * (fDeltaZoom / (fDeltaZoom - 1)));
	m_arrSpline.SetPan(m_arrSpline.GetPan() - ptDeltaPan);
	m_fZoom = fZoom;
	Invalidate();
	Notify(NM_SPLINE_ZOOM);
	UpdateRulers();
};

void CSplineWnd::SetPan(DPoint ptPan)
{
	m_arrSpline.SetPan(ptPan);
	Invalidate();
	UpdateRulers();
}

void CSplineWnd::Notify(int nCode)
{
	NMHDR	nmhdr;
	nmhdr.code = nCode;
	nmhdr.hwndFrom = m_hWnd;
	nmhdr.idFrom = GetDlgCtrlID();
	GetParent()->SendMessage(WM_NOTIFY, nmhdr.idFrom, LPARAM(&nmhdr));
}

void CSplineWnd::SetSplineStyle(UINT nStyle)
{
	ShowRulers((nStyle & ST_SHOW_RULERS) != 0);
	m_nStyle = nStyle;
	Invalidate();
}

bool CSplineWnd::CreateRulers()
{
	CFont	font;
	if (!font.CreateStockObject(DEFAULT_GUI_FONT))
		return false;
	LOGFONT	lf;
	font.GetLogFont(&lf);
	lf.lfEscapement = 900;	//  rotate font 90 degrees counter-clockwise
	if (!m_fontVertical.CreateFontIndirect(&lf))
		return false;
	static const DWORD	nPerRulerStyle[RULERS] = {
		CBRS_ALIGN_TOP,
		CBRS_ALIGN_LEFT | CRulerCtrl::VERTICAL_FONT,
	};
	DWORD	nCommonStyle = WS_CHILD | CRulerCtrl::HIDE_CLIPPED_VALS;
	if (m_nStyle & ST_SHOW_RULERS)
		nCommonStyle |= WS_VISIBLE;
	for (int iRuler = 0; iRuler < RULERS; iRuler++) {	// for each ruler
		CRulerCtrl&	ruler = m_arrRuler[iRuler];
		DWORD	nStyle = nCommonStyle | nPerRulerStyle[iRuler];
		if (!ruler.Create(nStyle, CRect(0, 0, 0, 0), GetParent(), iRuler + 1))
			return false;
		ruler.ModifyStyleEx(0, WS_EX_COMPOSITED);
		if (iRuler == RULER_VERT)
			ruler.SetFont(&m_fontVertical);
		else
			ruler.SendMessage(WM_SETFONT, WPARAM(GetStockObject(DEFAULT_GUI_FONT)));
	}
	m_nRulerWidth = m_arrRuler[RULER_HORZ].CalcMinHeight();
	UpdateRulers();
	return true;
}

void CSplineWnd::MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint)
{
	if (m_nStyle & ST_SHOW_RULERS) {	// if showing rulers
		CPoint	ptSpline(x + m_nRulerWidth, y + m_nRulerWidth);
		CSize	szSpline(nWidth - m_nRulerWidth, nHeight - m_nRulerWidth);
		CWnd::MoveWindow(ptSpline.x, ptSpline.y, szSpline.cx, szSpline.cy, bRepaint);
		m_arrRuler[RULER_HORZ].MoveWindow(ptSpline.x, y, szSpline.cx, m_nRulerWidth);
		m_arrRuler[RULER_VERT].MoveWindow(x, ptSpline.y, m_nRulerWidth, szSpline.cy);
	} else {	// not showing rulers
		CWnd::MoveWindow(x, y, nWidth, nHeight, bRepaint);
	}
}

void CSplineWnd::UpdateRulers()
{
	if (m_nStyle & ST_SHOW_RULERS) {	// if showing rulers
		DPoint	ptZoom(DPoint(1, 1) / m_arrSpline.GetScale());
		DPoint	ptPan(m_arrSpline.Denormalize(DPoint(0, 0)));
		for (int iRuler = 0; iRuler < RULERS; iRuler++) {
			CRulerCtrl&	ruler = m_arrRuler[iRuler];
			ruler.SetScrollPosition(-ptPan[iRuler]);
			ruler.SetZoom(ptZoom[iRuler]);
			ruler.UpdateSpacing();
		}
	}
}

void CSplineWnd::ShowRulers(bool bShow)
{
	bool	bShowing = (m_nStyle & ST_SHOW_RULERS) != 0;
	if (bShow == bShowing)	// if ruler visibility unchanged
		return;	// nothing to do
	UINT	nShowCmd = bShow ? SW_SHOW : SW_HIDE;
	for (int iRuler = 0; iRuler < RULERS; iRuler++)	// for each ruler
		m_arrRuler[iRuler].ShowWindow(nShowCmd);	// show or hide
	CRect	rc;
	GetClientRect(rc);
	MapWindowPoints(GetParent(), rc);	// MoveWindow expects parent window coords
	if (!bShow)	// if hiding rulers
		rc.TopLeft() -= CSize(m_nRulerWidth, m_nRulerWidth);	// remove rulers from client area
	m_nStyle ^= ST_SHOW_RULERS;	// toggle show rulers style
	MoveWindow(rc.left, rc.top, rc.Width(), rc.Height());	//  order matters; uses style
}

void CSplineWnd::Translate(DPoint ptTranslation)
{
	if (HasSelection()) {
		m_arrSpline.Translate(m_sel, ptTranslation, TF_TRANSLATE_ADJACENT_CTRLS);
		Notify(NM_SPLINE_TRANSLATE);
		Invalidate();
	}
}

void CSplineWnd::Scale(DPoint ptScaling, int iRelativeTo)
{
	if (HasSelection()) {
		m_arrSpline.Scale(m_sel, ptScaling, iRelativeTo);
		Notify(NM_SPLINE_SCALE);
		Invalidate();
	}
}

void CSplineWnd::Rotate(double fRotation, int iRelativeTo)
{
	if (HasSelection()) {
		m_arrSpline.Rotate(m_sel, fRotation, iRelativeTo);
		Notify(NM_SPLINE_ROTATE);
		Invalidate();
	}
}

void CSplineState::ReadProfile()
{
	int	nSegs = 0;
	RdReg(RK_SPLINE_SECTION, RK_SPLINE_COUNT, nSegs);	// read segment count
	SetSize(nSegs);
	RdReg(RK_SPLINE_SECTION, RK_SPLINE_START_PT, m_ptStart);
	RdReg(RK_SPLINE_SECTION, RK_SPLINE_PAN, m_ptPan);
	RdReg(RK_SPLINE_SECTION, RK_SPLINE_ZOOM, m_fZoom);
	RdReg(RK_SPLINE_SECTION, RK_SPLINE_GRID_SPACING, m_fGridSpacing);
	RdReg(RK_SPLINE_SECTION, RK_SPLINE_STYLE, m_nStyle);
	RdReg(RK_SPLINE_SECTION, RK_SPLINE_NODE_TYPE, m_iNodeType);
	CString	sIdx;
	for (int iSeg = 0; iSeg < nSegs; iSeg++) {	// for each segment
		sIdx.Format(_T("%d"), iSeg);	// format segment index
		CString	sSection(RK_SPLINE_SECTION _T("\\") + sIdx);
		CSegment&	seg = GetAt(iSeg);
		RdReg(sSection, RK_SPLINE_CTRL_PT_0, seg.m_ptCtrl[0]);
		RdReg(sSection, RK_SPLINE_CTRL_PT_1, seg.m_ptCtrl[1]);
		RdReg(sSection, RK_SPLINE_END_PT, seg.m_ptEnd);
		RdReg(sSection, RK_SPLINE_NODE_TYPE, seg.m_iNodeType);
		RdReg(sSection, RK_SPLINE_SEGMENT_TYPE, seg.m_iSegType);
	}
}

void CSplineState::WriteProfile() const
{
	int	nSegs = GetSize();
	WrReg(RK_SPLINE_SECTION, RK_SPLINE_COUNT, nSegs);	// write segment count
	WrReg(RK_SPLINE_SECTION, RK_SPLINE_START_PT, m_ptStart);
	WrReg(RK_SPLINE_SECTION, RK_SPLINE_PAN, m_ptPan);
	WrReg(RK_SPLINE_SECTION, RK_SPLINE_ZOOM, m_fZoom);
	WrReg(RK_SPLINE_SECTION, RK_SPLINE_GRID_SPACING, m_fGridSpacing);
	WrReg(RK_SPLINE_SECTION, RK_SPLINE_STYLE, m_nStyle);
	WrReg(RK_SPLINE_SECTION, RK_SPLINE_NODE_TYPE, m_iNodeType);
	CString	sIdx;
	for (int iSeg = 0; iSeg < nSegs; iSeg++) {	// for each segment
		sIdx.Format(_T("%d"), iSeg);	// format segment index
		CString	sSection(RK_SPLINE_SECTION _T("\\") + sIdx);
		const CSegment&	seg = GetAt(iSeg);
		WrReg(sSection, RK_SPLINE_CTRL_PT_0, seg.m_ptCtrl[0]);
		WrReg(sSection, RK_SPLINE_CTRL_PT_1, seg.m_ptCtrl[1]);
		WrReg(sSection, RK_SPLINE_END_PT, seg.m_ptEnd);
		WrReg(sSection, RK_SPLINE_NODE_TYPE, seg.m_iNodeType);
		WrReg(sSection, RK_SPLINE_SEGMENT_TYPE, seg.m_iSegType);
	}
}

void CSplineWnd::SetRadioEx(CCmdUI *pCmdUI, UINT nIDFirst, UINT nIDLast, UINT nIDItem)
{
	if (m_iDragState == DS_CONTEXT) {	// if tracking context menu
		// SetRadio draws button as miniscule dot, so use CheckMenuRadioItem instead
		if (pCmdUI->m_pMenu != NULL && pCmdUI->m_nID == nIDFirst) {
			pCmdUI->m_pMenu->CheckMenuRadioItem(nIDFirst, nIDLast, nIDItem, MF_BYCOMMAND);
		}
	} else	// not tracking context menu; SetRadio draws reasonable button on app menu
		pCmdUI->SetRadio(nIDItem == pCmdUI->m_nID);
}

void CSplineWnd::HorzMirror(CPoint ptOrigin, CPointArray& arrPt)
{
	for (int iPt = 0; iPt < arrPt.GetSize(); iPt++)
		arrPt[iPt].x = ptOrigin.x - (arrPt[iPt].x - ptOrigin.x);
}

BEGIN_MESSAGE_MAP(CSplineWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_SIZE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(UWM_RESET_HIT_STATE, OnResetHitState)
	ON_COMMAND(ID_SPLINE_ADD_NODE, OnAddNode)
	ON_UPDATE_COMMAND_UI(ID_SPLINE_ADD_NODE, OnUpdateAddNode)
	ON_COMMAND(ID_SPLINE_DELETE_NODE, OnDeleteNode)
	ON_UPDATE_COMMAND_UI(ID_SPLINE_DELETE_NODE, OnUpdateDeleteNode)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnSelectAll)
	ON_COMMAND(ID_EDIT_COPY, OnCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateCopy)
	ON_COMMAND(ID_EDIT_CUT, OnCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdatePaste)
	ON_COMMAND(ID_EDIT_DELETE, OnDelete)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateDelete)
	ON_COMMAND_RANGE(ID_SEGMENT_TYPE_FIRST, ID_SEGMENT_TYPE_LAST, OnSegmentType)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SEGMENT_TYPE_FIRST, ID_SEGMENT_TYPE_LAST, OnUpdateSegmentType)
	ON_COMMAND_RANGE(ID_NODE_TYPE_FIRST, ID_NODE_TYPE_LAST, OnNodeType)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NODE_TYPE_FIRST, ID_NODE_TYPE_LAST, OnUpdateNodeType)
	ON_WM_MOUSEWHEEL()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_COMMAND(ID_SPLINE_SHOW_GRID, OnShowGrid)
	ON_UPDATE_COMMAND_UI(ID_SPLINE_SHOW_GRID, OnUpdateShowGrid)
	ON_COMMAND(ID_SPLINE_SNAP_TO_GRID, OnSnapToGrid)
	ON_UPDATE_COMMAND_UI(ID_SPLINE_SNAP_TO_GRID, OnUpdateSnapToGrid)
	ON_COMMAND(ID_SPLINE_SHOW_RULERS, OnShowRulers)
	ON_UPDATE_COMMAND_UI(ID_SPLINE_SHOW_RULERS, OnUpdateShowRulers)
	ON_COMMAND(ID_SPLINE_SHOW_HORZ_MIRROR, OnShowHorzMirror)
	ON_UPDATE_COMMAND_UI(ID_SPLINE_SHOW_HORZ_MIRROR, OnUpdateShowHorzMirror)
	ON_COMMAND(ID_SPLINE_IMPORT, OnImport)
	ON_COMMAND(ID_SPLINE_EXPORT, OnExport)
	ON_COMMAND(ID_SPLINE_GRID_SETUP, OnGridSetup)
	ON_COMMAND(ID_SPLINE_TRANSLATE, OnTranslate)
	ON_UPDATE_COMMAND_UI(ID_SPLINE_TRANSLATE, OnUpdateTranslate)
	ON_COMMAND(ID_SPLINE_SCALE, OnScale)
	ON_UPDATE_COMMAND_UI(ID_SPLINE_SCALE, OnUpdateTranslate)
	ON_COMMAND(ID_SPLINE_ROTATE, OnRotate)
	ON_UPDATE_COMMAND_UI(ID_SPLINE_ROTATE, OnUpdateTranslate)
	ON_COMMAND(ID_SPLINE_FLIP_VERTICAL, OnFlipVertical)
	ON_UPDATE_COMMAND_UI(ID_SPLINE_FLIP_VERTICAL, OnUpdateTranslate)
	ON_COMMAND(ID_SPLINE_FLIP_HORIZONTAL, OnFlipHorizontal)
	ON_UPDATE_COMMAND_UI(ID_SPLINE_FLIP_HORIZONTAL, OnUpdateTranslate)
	ON_COMMAND(ID_SPLINE_ZOOM_IN, OnZoomIn)
	ON_COMMAND(ID_SPLINE_ZOOM_OUT, OnZoomOut)
	ON_COMMAND(ID_SPLINE_ZOOM_RESET, OnZoomReset)
END_MESSAGE_MAP()

int CSplineWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	if (!CreateRulers())
		return -1;
	return 0;
}

void CSplineWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	CRect	rc;
	GetClientRect(rc);
	dc.FillSolidRect(rc, GetSysColor(COLOR_WINDOW));	// erase background
	CSize	szClient = rc.Size();
	CPoint	ptOrigin = m_arrSpline.Denormalize(DPoint(0, 0));
	if (m_nStyle & ST_SHOW_AXES) {
		dc.FillSolidRect(round(ptOrigin.x), 0, 1, szClient.cy, m_arrColor[CLR_AXIS]);	// draw x-axis
		dc.FillSolidRect(0, round(ptOrigin.y), szClient.cx, 1, m_arrColor[CLR_AXIS]);	// draw y-axis
	}
	if ((m_nStyle & ST_SHOW_GRID) && m_fGridSpacing > 0) {	// if showing grid and valid spacing
		double	fGridSpacing = m_fGridSpacing * m_arrSpline.GetScale().x;	// denormalize grid step
		int	nMinGridSpacing = 10;
		if (fGridSpacing < nMinGridSpacing)	// if grid spacing below threshold, double it as needed
			fGridSpacing *= pow(2.0, ceil(log(double(nMinGridSpacing / fGridSpacing)) / log(2.0)));
		int	iFirst = (m_nStyle & ST_SHOW_AXES) != 0;
		int	x, y;
		for (int iLine = iFirst; (x = round(ptOrigin.x + iLine * fGridSpacing)) < szClient.cx; iLine++)
			dc.FillSolidRect(x, 0, 1, szClient.cy, m_arrColor[CLR_GRID]);
		for (int iLine = iFirst; (x = round(ptOrigin.x - iLine * fGridSpacing)) >= 0; iLine++)
			dc.FillSolidRect(x, 0, 1, szClient.cy, m_arrColor[CLR_GRID]);
		for (int iLine = iFirst; (y = round(ptOrigin.y + iLine * fGridSpacing)) < szClient.cy; iLine++)
			dc.FillSolidRect(0, y, szClient.cx, 1, m_arrColor[CLR_GRID]);
		for (int iLine = iFirst; (y = round(ptOrigin.y - iLine * fGridSpacing)) >= 0; iLine++)
			dc.FillSolidRect(0, y, szClient.cx, 1, m_arrColor[CLR_GRID]);
	}
	int	nSegs = GetSegmentCount();
	if (nSegs) {	// if at least one segment
		HGDIOBJ	penPrev = dc.SelectObject(GetStockObject(DC_PEN));	// save GDI objects
		HGDIOBJ	brushPrev = dc.SelectObject(GetStockObject(NULL_BRUSH));
		dc.SetDCPenColor(m_arrColor[CLR_LINE]);
		CPointArray	pa;
		int	iFirstRemain;
		if (m_sel.m_nSegs) {	// if segments selected
			if (m_sel.m_iSeg > 0) {	// if first segment isn't selected
				// draw segments before selection
				m_arrSpline.GetPoints(CSelection(0, m_sel.m_iSeg), pa);
				dc.PolyBezier(pa.GetData(), pa.GetSize());
				if (m_nStyle & ST_SHOW_HORZ_MIRROR) {
					dc.SetDCPenColor(m_arrColor[CLR_MIRROR]);
					HorzMirror(ptOrigin, pa);
					dc.PolyBezier(pa.GetData(), pa.GetSize());
				}
			}
			// draw selected segments
			dc.SetDCPenColor(m_arrColor[CLR_SELECTION]);
			m_arrSpline.GetPoints(m_sel, pa);
			dc.PolyBezier(pa.GetData(), pa.GetSize());
			CRect	r;
			int	iPt = 0;
			int	iEndSeg = m_sel.m_iSeg + m_sel.m_nSegs;
			for (int iSeg = m_sel.m_iSeg; iSeg < iEndSeg; iSeg++) {	// for each selected segment
				if (m_arrSpline[iSeg].m_iSegType != ST_LINE) {	// if not line
					// draw control point vectors
					dc.MoveTo(pa[iPt + PT_START]);
					dc.LineTo(pa[iPt + PT_CTRL_1]);
					dc.MoveTo(pa[iPt + PT_END]);
					dc.LineTo(pa[iPt + PT_CTRL_2]);
					// draw control points
					MakeSquare(pa[iPt + PT_CTRL_1], CONTROL_POINT_SIZE, r);
					dc.FillSolidRect(r, m_arrColor[CLR_SELECTION]);
					MakeSquare(pa[iPt + PT_CTRL_2], CONTROL_POINT_SIZE, r);
					dc.FillSolidRect(r, m_arrColor[CLR_SELECTION]);
				}
				// draw nodes
				MakeSquare(pa[iPt + PT_START], NODE_POINT_SIZE, r);
				dc.Rectangle(r);
				MakeSquare(pa[iPt + PT_END], NODE_POINT_SIZE, r);
				dc.Rectangle(r);
				iPt += POINT_TYPES - 1;
			}
			if (m_nStyle & ST_SHOW_HORZ_MIRROR) {
				dc.SetDCPenColor(m_arrColor[CLR_MIRROR]);
				HorzMirror(ptOrigin, pa);
				dc.PolyBezier(pa.GetData(), pa.GetSize());
			}
			dc.SetDCPenColor(m_arrColor[CLR_LINE]);
			iFirstRemain = iEndSeg;
		} else	// no selection
			iFirstRemain = 0;
		int	nRemains = nSegs - iFirstRemain;
		if (nRemains) {	// if at least one remaining segment
			m_arrSpline.GetPoints(CSelection(iFirstRemain, nRemains), pa);
			dc.PolyBezier(pa.GetData(), pa.GetSize());
			if (m_nStyle & ST_SHOW_HORZ_MIRROR) {
				dc.SetDCPenColor(m_arrColor[CLR_MIRROR]);
				HorzMirror(ptOrigin, pa);
				dc.PolyBezier(pa.GetData(), pa.GetSize());
			}
		}
		dc.SelectObject(penPrev);	// restore GDI objects
		dc.SelectObject(brushPrev);
	}
}

BOOL CSplineWnd::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CSplineWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	UpdateSize(CSize(cx, cy));
}

void CSplineWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_iDragState == DS_NONE) {
		m_nHitFlags = nFlags;
		m_ptHit = point;
		m_iHitSeg = SplineHitTest(point, m_iHitPointType);
		if (m_iHitSeg >= 0) {	// if segment was hit
			if (!IsSelected(m_iHitSeg)) {	// if segment not selected
				if ((nFlags & MK_SHIFT) != 0)	// if multiple selection
					MergeSelection(m_iHitSeg, 1);
				else	// single selection
					SetSelection(m_iHitSeg, 1);
			}
			m_iDragState = DS_TRACK;	// wait for drag to start
			SetCapture();
		} else {	// segment wasn't hit
			// compositing makes tracker invisible, so temporarily disable it
			bool	bCompositing = (GetExStyle() & WS_EX_COMPOSITED) != 0;
			if (bCompositing)
				ModifyStyleEx(WS_EX_COMPOSITED, 0);	// disable compositing
			CRectTracker	trk;
			if (trk.TrackRubberBand(this, point)) {	// if tracker succeeds
				// find segments that intersect tracker rect
				trk.m_rect.NormalizeRect();
				int	nSegs;
				int	iSeg = SplineHitTest(trk.m_rect, nSegs);
				if ((nFlags & MK_SHIFT) != 0)	// if multiple selection
					MergeSelection(iSeg, nSegs);
				else	// single selection
					SetSelection(iSeg, nSegs);
			} else {	// empty selection or tracking was canceled
				if (!(nFlags & MK_SHIFT))	// if single selection
					Deselect();	// remove selection
			}
			if (bCompositing)
				ModifyStyleEx(0, WS_EX_COMPOSITED);	// restore compositing
		}
	}
	CWnd::OnLButtonDown(nFlags, point);
}

void CSplineWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_iDragState > DS_NONE && m_iDragState < DS_PAN) {
		ReleaseCapture();
		switch (m_iDragState) {
		case DS_TRACK:	// drag didn't start
			if (m_iHitPointType < 0 && !(m_nHitFlags & MK_SHIFT)) {	// if node wasn't hit and single selection
				SetSelection(m_iHitSeg, 1);
				Invalidate();
			}
			break;
		case DS_DRAG:
			Notify(NM_SPLINE_CHANGED);
			break;
		}
		m_iDragState = DS_NONE;
	}
	ResetHitState();
	CWnd::OnLButtonUp(nFlags, point);
}

void CSplineWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_iDragState == DS_NONE) {
		m_iHitSeg = SplineHitTest(point, m_iHitPointType);
		if (m_iHitSeg >= 0 && m_iHitPointType >= 0)	// if hit point
			OnDeleteNode();
		else	// didn't hit point
			OnAddNode();
		ResetHitState();
	}
	CWnd::OnLButtonDblClk(nFlags, point);
}

void CSplineWnd::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_iDragState == DS_NONE) {
		int	iPointType;
		int	iSeg = SplineHitTest(point, iPointType);
		if (iSeg >= 0 && !IsSelected(iSeg))	// if segment was hit and it's not already selected
			SetSelection(iSeg, 1);	// select hit segment
	}
	CWnd::OnRButtonUp(nFlags, point);
}

void CSplineWnd::OnMButtonDown(UINT nFlags, CPoint point)
{
	if (m_iDragState == DS_NONE) {
		m_nHitFlags = nFlags;
		m_ptHit = point;
		SetCapture();
		m_iDragState = DS_PAN;
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEALL));
	}
	CWnd::OnMButtonDown(nFlags, point);
}

void CSplineWnd::OnMButtonUp(UINT nFlags, CPoint point)
{
	if (m_iDragState == DS_PAN) {
		ReleaseCapture();
		m_iDragState = DS_NONE;
		Notify(NM_SPLINE_PAN);
	}
	CWnd::OnMButtonUp(nFlags, point);
}

void CSplineWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_iDragState == DS_TRACK) {
		CPoint	ptDelta(point - m_ptHit);
		if (ptDelta.x || ptDelta.y)
			m_iDragState = DS_DRAG;
	}
	switch (m_iDragState) {
	case DS_DRAG:
		{
			DPoint	ptCurNorm(m_arrSpline.Normalize(point));	// normalize cursor point
			if (m_nStyle & ST_SNAP_TO_GRID)	// if snapping to grid
				CSplineArray::SnapToGrid(ptCurNorm, m_fGridSpacing);	// quantize cursor point
			if (m_iHitPointType >= 0)	// if point was hit
				m_arrSpline.SetPoint(m_iHitSeg, m_iHitPointType, ptCurNorm);
			else {	// segment was hit; drag selected segments
				DPoint	ptHitNorm = m_arrSpline.Normalize(m_ptHit);	// normalize hit point
				if (m_nStyle & ST_SNAP_TO_GRID) {	// if snapping to grid
					CSplineArray::SnapToGrid(ptHitNorm, m_fGridSpacing);	// quantize hit point
					const UINT	DRAG_INITIAL_SNAP = 0x80000000;	// bit hopefully never used by virtual key flags
					if (!(m_nHitFlags & DRAG_INITIAL_SNAP)) {	// if initial snap not done yet
						DPoint	ptNearestNode(m_arrSpline.GetNearestNode(m_iHitSeg, ptHitNorm));	// find nearest node
						DPoint	ptSnapNode(ptNearestNode);
						CSplineArray::SnapToGrid(ptSnapNode, m_fGridSpacing);	// quantize nearest node
						ptHitNorm += ptNearestNode - ptSnapNode;	// compensate hit point for initial snap
						m_nHitFlags |= DRAG_INITIAL_SNAP;	// mark initial snap done by setting a hit flag
					}
				}
				DPoint	ptOffset(ptCurNorm - ptHitNorm);
				m_ptHit = point;
				m_arrSpline.Translate(m_sel, ptOffset, TF_TRANSLATE_ADJACENT_CTRLS);
			}
			Invalidate();
			Notify(NM_SPLINE_CHANGING);
		}
		break;
	case DS_PAN:
		SetPan(GetPan() + DPoint(point - m_ptHit) / m_arrSpline.GetScale());
		m_ptHit = point;
		break;
	}
	CWnd::OnMouseMove(nFlags, point);
}

BOOL CSplineWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	CPoint	ptClient(pt);
	ScreenToClient(&ptClient);
	CRect	rc;
	GetClientRect(rc);
	if (rc.PtInRect(ptClient)) {	// if cursor within client area
		double	fDeltaZoom = pow(m_fZoomStep, double(zDelta) / WHEEL_DELTA);
		double	fNewZoom = m_fZoom * fDeltaZoom;
		SetZoom(ptClient, fNewZoom);
	}
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CSplineWnd::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (m_iDragState != DS_NONE)
		return;
	if (point.x == -1 && point.y == -1) {
		CRect	rc;
		GetClientRect(rc);
		ClientToScreen(rc);
		CPoint	ptCursor;
		GetCursorPos(&ptCursor);
		if (rc.PtInRect(ptCursor))
			point = ptCursor;
		else
			point = rc.CenterPoint();
	}
	m_ptHit = point;
	ScreenToClient(&m_ptHit);
	m_iHitSeg = SplineHitTest(m_ptHit, m_iHitPointType, HTF_NODES_ONLY);
	CMenu	menu;
	menu.LoadMenu(IDR_SPLINE);
	CMenu	*pMenu = menu.GetSubMenu(0);
	m_iDragState = DS_CONTEXT;
	pMenu->TrackPopupMenu(0, point.x, point.y, AfxGetMainWnd());
	m_iDragState = DS_NONE;
	PostMessage(UWM_RESET_HIT_STATE);	// post message to reset hit state after menu command is handled
}

LRESULT CSplineWnd::OnResetHitState(WPARAM wParam, LPARAM lParam)
{
	ResetHitState();
	return 0;
}

void CSplineWnd::OnAddNode()
{
	const int	nCtrlVecLen = NODE_POINT_SIZE * 3;	// reasonable distance from node
	DPoint	ptNode(m_arrSpline.Normalize(m_ptHit));
	DPoint	p1(m_arrSpline.Normalize(m_rHitLineSeg.TopLeft()));
	DPoint	p2(m_arrSpline.Normalize(m_rHitLineSeg.BottomRight()));
	double	fCtrlVecLen = nCtrlVecLen / m_arrSpline.GetScale().x;	// normalize control vector length
	double	fGridSpacing;
	if (m_nStyle & ST_SNAP_TO_GRID)
		fGridSpacing = m_fGridSpacing;
	else
		fGridSpacing = 0;
	int	iSeg = m_arrSpline.AddNode(m_iHitSeg, ptNode, p1, p2, fCtrlVecLen, fGridSpacing);
	Notify(NM_SPLINE_ADD_NODE);	// notify before modifying selection
	int	nSels = (m_iHitSeg >= 0) + 1;	// if segment hit, select both segments at new node
	SetSelection(iSeg, nSels);
}

void CSplineWnd::OnUpdateAddNode(CCmdUI *pCmdUI)
{
	// disallow adding node on top of existing node, to avoid math instability
	pCmdUI->Enable(!IsNode(m_iHitPointType));
}

void CSplineWnd::OnDeleteNode()
{
	m_arrSpline.DeleteNode(m_iHitSeg, m_iHitPointType);	// delete node
	Notify(NM_SPLINE_DELETE_NODE);	// notify before modifying selection
	if (GetSegmentCount())	// if not empty
		SetSelection(min(m_iHitSeg, GetSegmentCount() - 1), 1);
	else	// empty
		Deselect();
}

void CSplineWnd::OnUpdateDeleteNode(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(IsNode(m_iHitPointType));
}

void CSplineWnd::OnSelectAll()
{
	SelectAll();
}

void CSplineWnd::OnCopy()
{
	if (HasSelection()) {	// if selection exists
		CSplineArray	arrSpline;
		m_arrSpline.GetSegments(m_sel, arrSpline);
		CByteArrayEx	ba;
		int	nSegArraySize = arrSpline.GetSize() * sizeof(CSegment);
		ba.SetSize(offsetof(CLIPBOARD_SPLINE, arrSeg) + nSegArraySize);	// allocate buffer
		CLIPBOARD_SPLINE	*pSpline = reinterpret_cast<CLIPBOARD_SPLINE *>(ba.GetData());	
		pSpline->ptStart = arrSpline.GetStartNode();	// copy start node
		memcpy(pSpline->arrSeg, arrSpline.GetData(), nSegArraySize);	// copy segment array
		if (!m_Clipboard.Write(ba.GetData(), ba.GetSize()))	// if can't write data to clipboard
			AfxThrowResourceException();
	}
}

void CSplineWnd::OnUpdateCopy(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(HasSelection());
}

void CSplineWnd::OnCut()
{
	if (HasSelection()) {	// if selection exists
		OnCopy();	// copy selected segments to clipboard
		m_arrSpline.DeleteSegments(m_sel);	// delete selected segments
		Notify(NM_SPLINE_CUT);	// notify before modifying selection
		Deselect();
	}
}

void CSplineWnd::OnPaste()
{
	if (m_Clipboard.HasData()) {	// if clipboard has our kind of data
		DWORD	nCBSize;
		LPVOID	pCBData = m_Clipboard.Read(nCBSize);	// read data from clipboard
		if (pCBData == NULL)	// if can't read clipboard data
			AfxThrowResourceException();
		CSplineArray	arrSpline;
		CLIPBOARD_SPLINE	*pSpline = static_cast<CLIPBOARD_SPLINE *>(pCBData);	
		int	nSegArraySize = nCBSize - offsetof(CLIPBOARD_SPLINE, arrSeg);
		if (nSegArraySize % sizeof(CSegment))	// if invalid segment array size
			AfxThrowResourceException();
		int	nSegs = nSegArraySize / sizeof(CSegment);
		if (nSegs > 0) {	// if at least one segment
			arrSpline.SetStartNode(pSpline->ptStart);	// copy start node
			arrSpline.SetSize(nSegs);
			memcpy(arrSpline.GetData(), pSpline->arrSeg, nSegArraySize);	// copy segment array
			int	iSeg = m_sel.m_iSeg;
			if (iSeg < 0)	// if empty selection
				iSeg = GetSegmentCount();	// insert at end of array
			m_arrSpline.InsertSegments(iSeg, arrSpline);
			Notify(NM_SPLINE_PASTE);	// notify before modifying selection
			SetSelection(iSeg, nSegs);	// select pasted segments
		}
		delete [] pCBData;	// delete clipboard data to avoid memory leak
	}
}

void CSplineWnd::OnUpdatePaste(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_Clipboard.HasData());
}

void CSplineWnd::OnDelete()
{
	if (HasSelection()) {	// if selection exists
		m_arrSpline.DeleteSegments(m_sel);	// delete selected segments
		Notify(NM_SPLINE_DELETE);	// notify before modifying selection
		Deselect();
	} else {	// no selection
		if (m_iHitSeg >= 0) {	// if segment was hit
			m_arrSpline.DeleteSegments(CSelection(m_iHitSeg, 1));	// delete hit segment
			Notify(NM_SPLINE_DELETE);	// notify before modifying selection
			Deselect();
		}
	}
}

void CSplineWnd::OnUpdateDelete(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iHitSeg >= 0 || HasSelection());
}

void CSplineWnd::OnSegmentType(UINT nID)
{
	int	iSegType = nID - ID_SEGMENT_TYPE_FIRST;
	ASSERT(IsValidSegmentType(iSegType));
	if (m_iHitSeg >= 0) {	// if segment was hit
		if (iSegType != m_arrSpline.GetSegmentType(m_iHitSeg)) {	// if segment type changed
			m_arrSpline.SetSegmentType(m_iHitSeg, iSegType);
			Notify(NM_SPLINE_SEGMENT_TYPE);
			Invalidate();
		}
	} else {	// segment wasn't hit
		if (HasSelection()) {	// if selection exists
			m_arrSpline.SetSegmentType(m_sel, iSegType);
			Notify(NM_SPLINE_SEGMENT_TYPE);
			Invalidate();
		}
	}
}

void CSplineWnd::OnUpdateSegmentType(CCmdUI *pCmdUI)
{
	bool	bEnable = false;
	int	iSegType = -1;
	if (m_iHitSeg >= 0) {	// if segment was hit
		iSegType = m_arrSpline.GetSegmentType(m_iHitSeg);
		bEnable = true;
	}
	if (!bEnable && HasSelection()) {	// if segment wasn't hit and selection exists
		bEnable = true;
		iSegType = m_arrSpline.GetSegmentType(m_sel);
	}
	pCmdUI->Enable(bEnable);
	SetRadioEx(pCmdUI, ID_SEGMENT_TYPE_FIRST, ID_SEGMENT_TYPE_LAST, ID_SEGMENT_TYPE_FIRST + iSegType);
}

void CSplineWnd::OnNodeType(UINT nID)
{
	int	iNodeType = nID - ID_NODE_TYPE_FIRST;
	ASSERT(IsValidNodeType(iNodeType));
	if (IsNode(m_iHitPointType)) {	// if node was hit
		if (iNodeType != m_arrSpline.GetNodeType(m_iHitSeg, m_iHitPointType)) {	// if node type changed
			m_arrSpline.SetNodeType(m_iHitSeg, m_iHitPointType, iNodeType);
			Notify(NM_SPLINE_NODE_TYPE);
			Invalidate();
		}
	} else {	// node wasn't hit
		if (HasSelection()) {	// if selection exists
			m_arrSpline.SetNodeType(m_sel, iNodeType);
			Notify(NM_SPLINE_NODE_TYPE);
			Invalidate();
		}
	}
}

void CSplineWnd::OnUpdateNodeType(CCmdUI *pCmdUI)
{
	bool	bEnable = false;
	int	iNodeType = -1;
	if (IsNode(m_iHitPointType)) {	// if node was hit
		iNodeType = m_arrSpline.GetNodeType(m_iHitSeg, m_iHitPointType);
		bEnable = true;
	}
	if (!bEnable && HasSelection()) {	// if node wasn't hit and selection exists
		bEnable = true;
		iNodeType = m_arrSpline.GetNodeType(m_sel);
	}
	pCmdUI->Enable(bEnable);
	SetRadioEx(pCmdUI, ID_NODE_TYPE_FIRST, ID_NODE_TYPE_LAST, ID_NODE_TYPE_FIRST + iNodeType);
}

void CSplineWnd::OnShowGrid()
{
	SetSplineStyle(m_nStyle ^ ST_SHOW_GRID);	// toggle show grid style
	Notify(NM_SPLINE_STYLE);
}

void CSplineWnd::OnUpdateShowGrid(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_nStyle & ST_SHOW_GRID) != 0);
}

void CSplineWnd::OnSnapToGrid()
{
	SetSplineStyle(m_nStyle ^ ST_SNAP_TO_GRID);	// toggle snap to grid style
	Notify(NM_SPLINE_STYLE);
}

void CSplineWnd::OnUpdateSnapToGrid(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_nStyle & ST_SNAP_TO_GRID) != 0);
}

void CSplineWnd::OnShowRulers()
{
	SetSplineStyle(m_nStyle ^ ST_SHOW_RULERS);	// toggle show rulers style
	Notify(NM_SPLINE_STYLE);
}

void CSplineWnd::OnUpdateShowRulers(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_nStyle & ST_SHOW_RULERS) != 0);
}

void CSplineWnd::OnShowHorzMirror()
{
	SetSplineStyle(m_nStyle ^ ST_SHOW_HORZ_MIRROR);	// toggle show rulers style
	Notify(NM_SPLINE_STYLE);
}

void CSplineWnd::OnUpdateShowHorzMirror(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_nStyle & ST_SHOW_HORZ_MIRROR) != 0);
}

void CSplineWnd::OnImport()
{
	CString	sFilter(LPCTSTR(IDS_TEXT_FILES_FILTER));
	CFileDialog	fd(true, _T(".txt"), NULL, OFN_HIDEREADONLY, sFilter);
	if (fd.DoModal() == IDOK) {
		if (m_arrSpline.Import(fd.GetPathName())) {
			Notify(NM_SPLINE_IMPORT);	// notify before modifying selection
			Deselect();
		}
	}
}

void CSplineWnd::OnExport()
{
	CString	sFilter(LPCTSTR(IDS_TEXT_FILES_FILTER));
	CFileDialog	fd(false, _T(".txt"), NULL, OFN_OVERWRITEPROMPT, sFilter);
	if (fd.DoModal() == IDOK) {
		m_arrSpline.Export(fd.GetPathName());
	}
}

void CSplineWnd::OnGridSetup()
{
	HWND	hFocusWnd = ::GetFocus();
	CGridSetupDlg	dlg;
	dlg.m_fGridSpacing = m_fGridSpacing;
	if (dlg.DoModal() == IDOK) {
		m_fGridSpacing = dlg.m_fGridSpacing;
		Notify(NM_SPLINE_GRID_SETUP);
		Invalidate();
		::SetFocus(hFocusWnd);
	}
}

void CSplineWnd::OnTranslate()
{
	HWND	hFocusWnd = ::GetFocus();
	CTranslate2DDlg	dlg;
	RdReg(RK_SPLINE_DLG_TRANSLATE, RK_SPLINE_DLG_TRANSLATE_PT, dlg.m_pt);
	if (dlg.DoModal() == IDOK) {
		Translate(dlg.m_pt);
		WrReg(RK_SPLINE_DLG_TRANSLATE, RK_SPLINE_DLG_TRANSLATE_PT, dlg.m_pt);
		::SetFocus(hFocusWnd);
	}
}

void CSplineWnd::OnUpdateTranslate(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(HasSelection());
}

void CSplineWnd::OnScale()
{
	HWND	hFocusWnd = ::GetFocus();
	CScale2DDlg	dlg;
	RdReg(RK_SPLINE_DLG_SCALE, RK_SPLINE_DLG_SCALE_PT, dlg.m_ptScale);
	RdReg(RK_SPLINE_DLG_SCALE, RK_SPLINE_DLG_RELATIVE_TO, dlg.m_iRelativeTo);
	RdReg(RK_SPLINE_DLG_SCALE, RK_SPLINE_DLG_SCALE_PROPORTIONALLY, dlg.m_bProportionally);
	if (dlg.DoModal() == IDOK) {
		Scale(dlg.m_ptScale / 100, dlg.m_iRelativeTo);
		WrReg(RK_SPLINE_DLG_SCALE, RK_SPLINE_DLG_RELATIVE_TO, dlg.m_iRelativeTo);
		WrReg(RK_SPLINE_DLG_SCALE, RK_SPLINE_DLG_SCALE_PT, dlg.m_ptScale);
		WrReg(RK_SPLINE_DLG_SCALE, RK_SPLINE_DLG_SCALE_PROPORTIONALLY, dlg.m_bProportionally);
		::SetFocus(hFocusWnd);
	}
}

void CSplineWnd::OnRotate()
{
	HWND	hFocusWnd = ::GetFocus();
	CRotate2DDlg	dlg;
	RdReg(RK_SPLINE_DLG_ROTATE, RK_SPLINE_DLG_ROTATE_ANGLE, dlg.m_fAngle);
	RdReg(RK_SPLINE_DLG_ROTATE, RK_SPLINE_DLG_RELATIVE_TO, dlg.m_iRelativeTo);
	if (dlg.DoModal() == IDOK) {
		Rotate(dlg.m_fAngle / 180 * M_PI, dlg.m_iRelativeTo);
		WrReg(RK_SPLINE_DLG_ROTATE, RK_SPLINE_DLG_ROTATE_ANGLE, dlg.m_fAngle);
		WrReg(RK_SPLINE_DLG_ROTATE, RK_SPLINE_DLG_RELATIVE_TO, dlg.m_iRelativeTo);
		::SetFocus(hFocusWnd);
	}
}

void CSplineWnd::OnFlipVertical()
{
	m_arrSpline.Flip(m_sel, true);	// bVertical
	Notify(NM_SPLINE_FLIP_VERTICAL);
	Invalidate();
}

void CSplineWnd::OnFlipHorizontal()
{
	m_arrSpline.Flip(m_sel, false);	// bVertical
	Notify(NM_SPLINE_FLIP_HORIZONTAL);
	Invalidate();
}

void CSplineWnd::OnZoomIn()
{
	SetZoom(m_arrSpline.GetOrigin(), m_fZoom * m_fZoomStep);
}

void CSplineWnd::OnZoomOut()
{
	SetZoom(m_arrSpline.GetOrigin(), m_fZoom / m_fZoomStep);
}

void CSplineWnd::OnZoomReset()
{
	CSplineState	state;
	m_arrSpline.SetPan(DPoint(0, 0));
	SetZoom(state.m_fZoom);
}
