// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      13jun17	initial version
		01		06oct17	in AddNode, if adding line segment, give control vectors zero length

*/

#include "stdafx.h"
#include "Spline.h"
#define _USE_MATH_DEFINES
#include <math.h>

double CSplineBase::CSegment::FindX(DPoint ptStart, double fTargetY) const
{
	const double	fEpsilon = 1e-6;
	if (fabs(fTargetY - ptStart.y) < fEpsilon)
		return ptStart.x;
	if (fabs(fTargetY - m_ptEnd.y) < fEpsilon)
		return m_ptEnd.x;
	double	t = 0.5;	// binary search starts in middle
	double	fStep;
	if (ptStart.y < m_ptEnd.y)	// if ascending in y-axis
		fStep = 0.5;
	else	// descending in y-axis
		fStep = -0.5;
	const int	nPasses = 30;	// tradeoff between accuracy and speed
	for (int iPass = 0; iPass < nPasses; iPass++) {
		double	y = Eval(t, ptStart.y, m_ptCtrl[0].y, m_ptCtrl[1].y, m_ptEnd.y);
		if (fabs(y - fTargetY) < fEpsilon)	// if close enough
			break;
		fStep /= 2;	// divide step size in half
		if (y > fTargetY)	// if overshoot
			t -= fStep;	// step backward
		else	// undershoot
			t += fStep;	// step forward
	}
	return Eval(t, ptStart.x, m_ptCtrl[0].x, m_ptCtrl[1].x, m_ptEnd.x);
}

__forceinline double CSplineBase::CSegment::Eval(double t, double x1, double x2, double x3, double x4)
{
	return pow(1 - t, 3) * x1 + 3 * pow(1 - t, 2) * t * x2 + 3 * (1 - t) * pow(t, 2) * x3 + pow(t, 3) * x4;
}

void CSplineBase::CSegment::operator+=(const DPoint& pt)
{
	m_ptEnd += pt;
	m_ptCtrl[0] += pt;
	m_ptCtrl[1] += pt;
}

void CSplineBase::CSegment::operator-=(const DPoint& pt)
{
	m_ptEnd -= pt;
	m_ptCtrl[0] -= pt;
	m_ptCtrl[1] -= pt;
}

void CSplineBase::CSegment::Scale(DPoint ptOrigin, DPoint ptScaling)
{
	m_ptEnd.Scale(ptOrigin, ptScaling);
	m_ptCtrl[0].Scale(ptOrigin, ptScaling);
	m_ptCtrl[1].Scale(ptOrigin, ptScaling);
}

CSplineArray::CSplineArray()
{
	m_ptStart = DPoint(0, 0);
	m_ptPan = DPoint(0, 0); 
	m_ptOrigin = DPoint(0, 0);
	m_ptScale = DPoint(0, 0);
	m_iNodeType = NT_SMOOTH;
}

void CSplineArray::Copy(const CSplineArray& src)
{
	CArrayEx::Copy(src);
	m_ptStart = src.m_ptStart;
	m_ptPan = src.m_ptPan;
	m_ptOrigin = src.m_ptOrigin;
	m_ptScale = src.m_ptScale;
	m_iNodeType = src.m_iNodeType;
}

void CSplineArray::GetSegments(CSelection sel, CSplineArray& arrSpline) const
{
	arrSpline.SetSize(sel.m_nSegs);
	if (!IsValid(sel))	// if invalid selection
		return;	// nothing else to do
	arrSpline.m_ptStart = GetStartNode(sel.m_iSeg);
	arrSpline.m_iNodeType = GetNodeType(sel.m_iSeg, PT_START);
	for (int iSeg = 0; iSeg < sel.m_nSegs; iSeg++)	// for each segment in range
		arrSpline[iSeg] = GetAt(sel.m_iSeg + iSeg);
}

void CSplineArray::GetPoints(CSelection sel, CPointArray& arrPt) const
{
	ASSERT(IsValid(sel));
	int	nPts = sel.m_nSegs * 3 + 1;
	arrPt.SetSize(nPts);
	arrPt[0] = Denormalize(GetStartNode(sel.m_iSeg));
	int	iPt = 0;
	int	iEndSeg = sel.End();
	for (int iSeg = sel.m_iSeg; iSeg < iEndSeg; iSeg++) {	// for each segment
		const CSegment& seg = GetAt(iSeg);
		arrPt[iPt + PT_CTRL_1] = Denormalize(seg.m_ptCtrl[0]);
		arrPt[iPt + PT_CTRL_2] = Denormalize(seg.m_ptCtrl[1]);
		arrPt[iPt + PT_END] = Denormalize(seg.m_ptEnd);
		iPt += POINT_TYPES - 1;
	}
}

void CSplineArray::GetPath(CDC& dc, int iSeg, CPointArray& arrSplinePt, CPointArray& arrPathPt, CByteArray& arrVertexType) const
{
	GetPoints(CSelection(iSeg, 1), arrSplinePt);
	VERIFY(dc.BeginPath());
	VERIFY(dc.PolyBezier(arrSplinePt.GetData(), arrSplinePt.GetSize()));
	VERIFY(dc.EndPath());
	VERIFY(dc.FlattenPath());
	int	nPts = dc.GetPath(NULL, NULL, 0);	// get path's point count
	arrPathPt.SetSize(nPts);
	arrVertexType.SetSize(nPts);
	VERIFY(dc.GetPath(arrPathPt.GetData(), arrVertexType.GetData(), nPts) >= 0);	// get path points
}

void CSplineArray::DumpSegments() const
{
	int	nSegs = GetSize();
	printf("nSegs=%d\n", nSegs);
	if (!nSegs)
		return;
	printf("start %.3f,%.3f NT=%d\n", m_ptStart.x, m_ptStart.y, m_iNodeType);
	for (int iSeg = 0; iSeg < nSegs; iSeg++) {
		const CSegment& s = GetAt(iSeg);
		printf("[%d] end=%.3f,%.3f c0=%.3f,%.3f c1=%.3f,%.3f NT=%d ST=%d\n", iSeg, s.m_ptEnd.x, s.m_ptEnd.y,
			s.m_ptCtrl[0].x, s.m_ptCtrl[0].y, s.m_ptCtrl[1].x, s.m_ptCtrl[1].y, s.m_iNodeType, s.m_iSegType);
	}
}

void CSplineArray::AdjustControlPt(int iNodeType, DPoint ptNode, DPoint ptControl1, DPoint& ptControl2) const
{
	switch (iNodeType) {
	case NT_SMOOTH:
		{
			// make second control vector colinear with first, on opposite side of node, without changing its length
			DPoint	v1 = ptControl1 - ptNode;	// normalize first control vector
			double	a = -M_PI / 2 - atan2(v1.y, v1.x);	// compute angle of first control vector and rotate 180 degrees
			DPoint	v2 = ptControl2 - ptNode;	// normalize second control vector
			double	r = v2.Length();	// compute length of second control vector; use as radius
			ptControl2 = ptNode + DPoint(sin(a) * r, cos(a) * r);	// compute rotated second control point
		}
		break;
	case NT_SYMMETRICAL:
		{
			// make second control vector a reflection of first
			DPoint	v1 = ptControl1 - ptNode;	// normalize first control vector
			ptControl2 = ptNode + v1 * -1;	// reflect first control vector around node
		}
		break;
	}
}

void CSplineArray::AdjustPrevControlPt(int iSeg)
{
	if (iSeg > 0) {	// if not first segment
		DPoint&	ptStart = GetStartNode(iSeg);
		CSegment&	seg = GetAt(iSeg);
		CSegment&	segPrev = GetAt(iSeg - 1);
		if (segPrev.m_iSegType != ST_LINE)	// if previous segment isn't line
			AdjustControlPt(segPrev.m_iNodeType, ptStart, seg.m_ptCtrl[0], segPrev.m_ptCtrl[1]);
	}
}

void CSplineArray::AdjustNextControlPt(int iSeg)
{
	if (iSeg < GetSize() - 1) {	// if not last segment
		CSegment&	seg = GetAt(iSeg);
		CSegment&	segNext = GetAt(iSeg + 1);
		if (segNext.m_iSegType != ST_LINE)	// if next segment isn't line
			AdjustControlPt(seg.m_iNodeType, seg.m_ptEnd, seg.m_ptCtrl[1], segNext.m_ptCtrl[0]);
	}
}

DPoint CSplineArray::GetPoint(int iSeg, int iPointType) const
{
	switch (iPointType) {
	case PT_START:
		return GetStartNode(iSeg);
	case PT_END:
		return GetAt(iSeg).m_ptEnd;
	case PT_CTRL_1:
		return GetAt(iSeg).m_ptCtrl[0];
	case PT_CTRL_2:
		return GetAt(iSeg).m_ptCtrl[1];
	}
	return DPoint(0, 0);
}

void CSplineArray::SetPoint(int iSeg, int iPointType, DPoint ptNew)
{
	CSegment&	seg = GetAt(iSeg);
	DPoint&	ptStart = GetStartNode(iSeg);
	switch (iPointType) {
	case PT_START:
		{
			DPoint	ptDelta(ptNew - ptStart);
			seg.m_ptCtrl[0] += ptDelta;	// translate first control point
			if (iSeg > 0) {	// if not first segment
				// translate previous segment's second control point
				GetAt(iSeg - 1).m_ptCtrl[1] += ptDelta;
			}
			ptStart = ptNew;	// update start node
		}
		break;
	case PT_END:
		{
			DPoint	ptDelta(ptNew - seg.m_ptEnd);
			seg.m_ptCtrl[1] += ptDelta;	// translate second control point
			if (iSeg < GetSize() - 1) {	// if not last segment
				// translate next segment's first control point
				GetAt(iSeg + 1).m_ptCtrl[0] += ptDelta;
			}
			seg.m_ptEnd = ptNew;	// update end node
		}
		break;
	case PT_CTRL_1:
		if (seg.m_iSegType != ST_LINE) {	// if not line
			seg.m_ptCtrl[0] = ptNew;	// update first control point
			AdjustPrevControlPt(iSeg);
		}
		break;
	case PT_CTRL_2:
		if (seg.m_iSegType != ST_LINE) {	// if not line
			seg.m_ptCtrl[1] = ptNew;	// update second control point
			AdjustNextControlPt(iSeg);
		}
		break;
	}
}

double CSplineArray::Wrap(double x, double y)
{
	x = fmod(x, y);
	if (x < 0)
		x += y;
	return x;
}

void CSplineArray::SnapToGrid(DPoint& pt, double fGridStep)
{
	if (fGridStep) {	// avoid divide by zero
		double	fHalfStep = fGridStep / 2;
		pt.x -= Wrap(pt.x - fHalfStep, fGridStep) - fHalfStep;
		pt.y -= Wrap(pt.y - fHalfStep, fGridStep) - fHalfStep;
	}
}

DPoint CSplineArray::GetNearestNode(int iSeg, DPoint pt) const
{
	DPoint	ptStart(GetStartNode(iSeg));
	DPoint	ptEnd(GetAt(iSeg).m_ptEnd);
	if (ptStart.Distance(pt) < ptEnd.Distance(pt))
		return ptStart;
	return ptEnd;
}

int CSplineArray::AddNode(int iSeg, DPoint ptNode, DPoint p1, DPoint p2, double fCtrlVecLen, double fGridStep)
{
	CSegment	segNew;
	int	iInsertPos;
	if (iSeg >= 0) {	// if segment specified
		DPoint	vIntersect;
		ptNode = ptNode.Intersect(p1, p2, vIntersect);	// find closest point to node on line
		if (fGridStep)	// if snapping to grid
			SnapToGrid(ptNode, fGridStep);	// quantize updated node point
		DPoint	ptCtrl[2];
		CSegment&	seg = GetAt(iSeg);
		if (seg.m_iSegType != ST_LINE) {	// if adding curve segment
			ptCtrl[0] = ptNode - vIntersect * fCtrlVecLen;
			ptCtrl[1] = vIntersect * fCtrlVecLen + ptNode;
		} else {	// adding line segment; give control vectors zero length
			ptCtrl[0] = ptNode;
			ptCtrl[1] = ptNode;
		}
		segNew = CSegment(seg.m_ptCtrl[0], ptCtrl[1], ptNode, seg.m_iSegType, seg.m_iNodeType);
		seg.m_ptCtrl[0] = ptCtrl[0];
		iInsertPos = iSeg;
	} else {	// segment unspecified
		if (fGridStep)	// if snapping to grid
			SnapToGrid(ptNode, fGridStep);	// quantize node point
		int	nSegs = GetSize();
		if (nSegs) {	// if at least one segment
			int	iLast = nSegs - 1;
			double	fDistStart = ptNode.Distance(m_ptStart);
			double	fDistEnd = ptNode.Distance(GetAt(iLast).m_ptEnd);
			if (fDistStart < fDistEnd) {	// if new node closer to start node
				// add segment at start of array, from new node to start node
				const CSegment&	seg = GetAt(0);
				segNew = CSegment(ptNode, m_ptStart, m_ptStart, seg.m_iSegType, m_iNodeType);
				if (seg.m_iSegType != ST_LINE) {	// if not adding line segment
					if (m_iNodeType == NT_SMOOTH)	// if adding at smooth node
						segNew.m_ptCtrl[1] += DPoint(fCtrlVecLen, fCtrlVecLen);
					AdjustControlPt(m_iNodeType, m_ptStart, seg.m_ptCtrl[0], segNew.m_ptCtrl[1]);
				}
				m_ptStart = ptNode;
				iInsertPos = 0;
			} else {	// new node closer to last segment's end node
				// add segment at end of array, from end node to new node
				const CSegment&	seg = GetAt(iLast);
				segNew = CSegment(seg.m_ptEnd, ptNode, ptNode, seg.m_iSegType, seg.m_iNodeType);
				if (seg.m_iSegType != ST_LINE) {	// if not adding line segment
					if (seg.m_iNodeType == NT_SMOOTH)	// if adding at smooth node
						segNew.m_ptCtrl[0] += DPoint(fCtrlVecLen, fCtrlVecLen);
					AdjustControlPt(seg.m_iNodeType, seg.m_ptEnd, seg.m_ptCtrl[1], segNew.m_ptCtrl[0]);
				}
				iInsertPos = nSegs;
			}
		} else {	// no segments
			// add segment from origin to new node
			m_ptStart = DPoint(0, 0);
			segNew = CSegment(m_ptStart, ptNode, ptNode, ST_CURVE, NT_SMOOTH);
			iInsertPos = 0;
		}
	}
	InsertAt(iInsertPos, segNew);
	return iInsertPos;
}

void CSplineArray::DeleteNode(int iSeg, int iPointType)
{
	if (!IsNode(iPointType))	// if point isn't node
		return;	// nothing to do
	if (iPointType == PT_START) {	// if deleting start node 
		if (!iSeg) {	// if first segment
			m_ptStart = GetAt(iSeg).m_ptEnd;	// update start node
			RemoveAt(iSeg);	// delete first segment
			return;
		}
		iSeg--;	// delete previous segment
	}
	if (iSeg < GetSize() - 1)	// if not deleting last segment
		GetAt(iSeg + 1).m_ptCtrl[0] = GetAt(iSeg).m_ptCtrl[0];	// copy first control point to next segment
	RemoveAt(iSeg);	// delete segment
}

void CSplineArray::DeleteSegments(CSelection sel, bool bMerge)
{
	ASSERT(IsValid(sel));
	int	iEndSeg = sel.End();
	if (bMerge) {	// if merging deleted segments into segment following selection (if any)
		if (!sel.m_iSeg)	// if deleting first segment
			m_ptStart = GetAt(sel.m_nSegs - 1).m_ptEnd;	// copy selection's last node to start node
		else {	// not deleting first segment
			if (iEndSeg < GetSize())	// if not deleting last segment either
				GetAt(iEndSeg).m_ptCtrl[0] = GetAt(sel.m_iSeg).m_ptCtrl[0];	// copy first control point to next segment
		}
	} else {	// not merging; translate segments as needed to account for deletion
		if (iEndSeg < GetSize()) {	// if segments after selection
			if (!sel.m_iSeg)	// if deleting first segment
				m_ptStart = GetAt(sel.m_nSegs - 1).m_ptEnd;	// copy selection's last node to start node
			else {	// not deleting first segment
				DPoint	ptDelta(GetStartNode(sel.m_iSeg) - GetAt(iEndSeg - 1).m_ptEnd);
				Translate(CSelection(iEndSeg, GetSize() - iEndSeg), ptDelta);
			}
		}
	}
	RemoveAt(sel.m_iSeg, sel.m_nSegs);	// delete selected segments
}

void CSplineArray::InsertSegments(int iSeg, const CSplineArray& arrSpline)
{
	int	nSrcSegs = arrSpline.GetSize();
	if (!nSrcSegs)
		return;
	DPoint	ptSrcStart(arrSpline.GetStartNode());
	DPoint	ptSrcEnd(arrSpline[nSrcSegs - 1].m_ptEnd - ptSrcStart);
	int	nDstSegs = GetSize();
	for (int iDst = iSeg; iDst < nDstSegs; iDst++) {	// for each segment after insert point
		GetAt(iDst) += ptSrcEnd;
	}
	DPoint	ptInsert(GetStartNode(iSeg));
	DPoint	ptOffset(ptInsert - ptSrcStart);
	for (int iSrc = 0; iSrc < nSrcSegs; iSrc++) {	// for each source segment in range
		CSegment	seg(arrSpline[iSrc]);
		seg += ptOffset;
		InsertAt(iSeg + iSrc, seg);
	}
}

void CSplineArray::SetSegmentType(int iSeg, int iSegType)
{
	ASSERT(IsValidSegmentType(iSegType));
	CSegment&	seg = GetAt(iSeg);
	seg.m_iSegType = iSegType;
	switch (iSegType) {
	case ST_LINE:
		seg.m_ptCtrl[0] = GetStartNode(iSeg);
		seg.m_ptCtrl[1] = seg.m_ptEnd;
		break;
	}
}

int CSplineArray::GetNodeType(int iSeg, int iPointType) const
{
	ASSERT(IsNode(iPointType));
	if (iPointType == PT_START) {	// if start node
		if (iSeg > 0)	// if not first segment
			iSeg--;	// get previous segment instead
		else	// first segment
			return m_iNodeType;	// return first node type
	}
	return GetAt(iSeg).m_iNodeType;
}

void CSplineArray::SetNodeType(int iSeg, int iPointType, int iNodeType)
{
	ASSERT(IsNode(iPointType));
	ASSERT(IsValidNodeType(iNodeType));
	if (iPointType == PT_START) {	// if start node
		if (iSeg > 0)	// if not first segment
			iSeg--;	// set previous segment instead
		else {	// first segment
			m_iNodeType = iNodeType;	// set first node type
			return;
		}
	}
	CSegment&	seg = GetAt(iSeg);
	seg.m_iNodeType = iNodeType;
	switch (iNodeType) {
	case NT_SMOOTH:
	case NT_SYMMETRICAL:
		if (iPointType == PT_START)	// if start node
			SetPoint(iSeg + 1, PT_CTRL_1, GetAt(iSeg + 1).m_ptCtrl[0]);	// adjust previous segment if any
		else	// end node
			SetPoint(iSeg, PT_CTRL_2, seg.m_ptCtrl[1]);	// adjust next segment if any
		break;
	}
}

int CSplineArray::GetSegmentType(CSelection sel) const
{
	if (!IsValid(sel))	// if invalid selection
		return -1;
	int	iSegType = GetSegmentType(sel.m_iSeg);	// get first selected segment's type
	int	iEndSeg = sel.End();
	for (int iSeg = sel.m_iSeg + 1; iSeg < iEndSeg; iSeg++) {	// for each remaining selected segment
		if (GetSegmentType(iSeg) != iSegType)	// if inconsistent segment type
			return -1;
	}
	return iSegType;
}

void CSplineArray::SetSegmentType(CSelection sel, int iSegType)
{
	ASSERT(IsValid(sel));
	int	iEndSeg = sel.End();
	for (int iSeg = sel.m_iSeg; iSeg < iEndSeg; iSeg++)	// for each selected segment
		SetSegmentType(iSeg, iSegType);
}

void CSplineArray::SetNodeType(CSelection sel, int iNodeType)
{
	ASSERT(IsValid(sel));
	SetNodeType(sel.m_iSeg, PT_START, iNodeType);	// set first selected node's type
	int	iEndSeg = sel.End();
	for (int iSeg = sel.m_iSeg; iSeg < iEndSeg; iSeg++)	// for each selected segment
		SetNodeType(iSeg, PT_END, iNodeType);	// set end node's type
}

int CSplineArray::GetNodeType(CSelection sel) const
{
	if (!IsValid(sel))	// if invalid selection
		return -1;
	int	iNodeType = GetNodeType(sel.m_iSeg, PT_START);	// get first selected node type
	int	iEndSeg = sel.End();
	for (int iSeg = sel.m_iSeg; iSeg < iEndSeg; iSeg++) {	// for each remaining selected node
		if (GetNodeType(iSeg, PT_END) != iNodeType)	// if inconsistent node type
			return -1;
	}
	return iNodeType;
}

void CSplineArray::CalcBounds(CSelection sel, DRect& rBounds) const
{
	ASSERT(IsValid(sel));
	DPoint	p1 = GetStartNode(sel.m_iSeg);
	DPoint	p2 = p1;
	int	iEndSeg = sel.End();
	for (int iSeg = sel.m_iSeg; iSeg < iEndSeg; iSeg++) {	// for each segment in range
		const CSegment&	seg = GetAt(iSeg);
		DPoint	pt = seg.m_ptEnd;
		if (pt.x < p1.x)
			p1.x = pt.x;
		if (pt.y < p1.y)
			p1.y = pt.y;
		if (pt.x > p2.x)
			p2.x = pt.x;
		if (pt.y > p2.y)
			p2.y = pt.y;
	}
	rBounds = DRect(p1, p2);
}

DPoint CSplineArray::CalcCenter(CSelection sel, DRect& rBounds) const
{
	CalcBounds(sel, rBounds);
	return (rBounds.TopLeft() + rBounds.BottomRight()) / 2;
}

void CSplineArray::Translate(CSelection sel, DPoint ptTranslation, UINT nFlags)
{
	ASSERT(IsValid(sel));
	if (!(nFlags & TF_SKIP_FIRST_NODE))	// if not skipping first node
		GetStartNode(sel.m_iSeg) += ptTranslation;	// translate first node
	int	iEndSeg = sel.End();
	for (int iSeg = sel.m_iSeg; iSeg < iEndSeg; iSeg++) {	// for each segment in range
		GetAt(iSeg) += ptTranslation;	// translate segment
	}
	if (nFlags & TF_SKIP_LAST_NODE)	// if skipping last node
		GetAt(iEndSeg - 1).m_ptEnd -= ptTranslation;	// undo last node's translation
	if (nFlags & TF_TRANSLATE_ADJACENT_CTRLS) {	// if translating adjacent control points
		if (sel.m_iSeg > 0)	// if segments before start of range
			GetAt(sel.m_iSeg - 1).m_ptCtrl[1] += ptTranslation;	// translate previous segment's second control point
		if (iEndSeg < GetSize())	// if segments after end of range
			GetAt(iEndSeg).m_ptCtrl[0] += ptTranslation;	// translate next segment's first control point
	} else if (nFlags & TF_RECALC_ADJACENT_CTRLS) {	// if recalculating adjacent control points
		AdjustPrevControlPt(sel.m_iSeg);
		AdjustNextControlPt(iEndSeg - 1);
	}
}

void CSplineArray::Scale(CSelection sel, DPoint ptScaling, int iRelativeTo)
{
	ASSERT(IsValid(sel));
	DPoint	ptCenter;
	if (iRelativeTo == RT_SELECTION_CENTER) {
		DRect	rBounds;
		ptCenter = CalcCenter(sel, rBounds);
	} else
		ptCenter = DPoint(0, 0);
	DPoint&	ptFirst = GetStartNode(sel.m_iSeg);
	DPoint	ptPrevFirst(ptFirst);
	ptFirst.Scale(ptCenter, ptScaling);
	int	iEndSeg = sel.End();
	DPoint&	ptLast = GetAt(iEndSeg - 1).m_ptEnd;
	DPoint	ptPrevLast(ptLast);
	for (int iSeg = sel.m_iSeg; iSeg < iEndSeg; iSeg++)	// for each segment in range
		GetAt(iSeg).Scale(ptCenter, ptScaling);
	if (sel.m_iSeg > 0)	// if segments before selection
		Translate(CSelection(0, sel.m_iSeg), ptFirst - ptPrevFirst, TF_SKIP_LAST_NODE);
	if (iEndSeg < GetSize())	// if segments after selection
		Translate(CSelection(iEndSeg, GetSize() - iEndSeg), ptLast - ptPrevLast, TF_SKIP_FIRST_NODE);
}

void CSplineArray::Rotate(CSelection sel, double fRotation, int iRelativeTo)
{
	ASSERT(IsValid(sel));
	DPoint	ptCenter;
	if (iRelativeTo == RT_SELECTION_CENTER) {
		DRect	rBounds;
		ptCenter = CalcCenter(sel, rBounds);
	} else
		ptCenter = DPoint(0, 0);
	DPoint&	ptFirst = GetStartNode(sel.m_iSeg);
	DPoint	ptPrevFirst(ptFirst);
	ptFirst.Rotate(ptCenter, fRotation);
	int	iEndSeg = sel.End();
	DPoint&	ptLast = GetAt(iEndSeg - 1).m_ptEnd;
	DPoint	ptPrevLast(ptLast);
	for (int iSeg = sel.m_iSeg; iSeg < iEndSeg; iSeg++) {	// for each segment in range
		CSegment&	seg = GetAt(iSeg);
		seg.m_ptEnd.Rotate(ptCenter, fRotation);
		seg.m_ptCtrl[0].Rotate(ptCenter, fRotation);
		seg.m_ptCtrl[1].Rotate(ptCenter, fRotation);
	}
	if (sel.m_iSeg > 0)	// if segments before selection
		Translate(CSelection(0, sel.m_iSeg), ptFirst - ptPrevFirst, TF_SKIP_LAST_NODE | TF_RECALC_ADJACENT_CTRLS);
	if (iEndSeg < GetSize())	// if segments after selection
		Translate(CSelection(iEndSeg, GetSize() - iEndSeg), ptLast - ptPrevLast, TF_SKIP_FIRST_NODE | TF_RECALC_ADJACENT_CTRLS);
}

void CSplineArray::Flip(CSelection sel, bool bVertical)
{
	ASSERT(IsValid(sel));
	DRect	rBounds;
	CalcBounds(sel, rBounds); 
	CSplineArray	arrSel;
	GetSegments(sel, arrSel);
	DPoint	ptScaling;
	if (bVertical)	// if vertical
		ptScaling = DPoint(1, -1);
	else	// horizontal
		ptScaling = DPoint(-1, 1);
	arrSel.Scale(CSelection(0, arrSel.GetSize()), ptScaling, RT_SELECTION_CENTER);
	if (bVertical)	// if vertical
		arrSel.Reverse();	// reverse order of selected segments
	DeleteSegments(sel);	// delete selected segments
	InsertSegments(sel.m_iSeg, arrSel);	// insert scaled segments
	DRect	rNewBounds;
	CalcBounds(sel, rNewBounds);	// translate so selection bounds stay the same
	Translate(CSelection(0, GetSize()), rBounds.TopLeft() - rNewBounds.TopLeft());
}

void CSplineArray::Reverse()
{
	int	nSegs = GetSize();
	if (!nSegs)	// if no segments
		return;	// nothing to do
	CSplineArray	arrSpline(*this);
	for (int iSeg = 0; iSeg < nSegs; iSeg++) {	// for each segment
		CSegment& dst = GetAt(iSeg);	
		const CSegment& src = arrSpline.GetAt(nSegs - 1 - iSeg);	
		GetStartNode(iSeg) = src.m_ptEnd;
		dst.m_ptCtrl[0] = src.m_ptCtrl[1];	// reverse order of control points
		dst.m_ptCtrl[1] = src.m_ptCtrl[0];
		dst.m_iSegType = src.m_iSegType;
		GetNodeType(iSeg) = src.m_iNodeType;
	}
	CSegment& last = GetAt(nSegs - 1);	
	last.m_ptEnd = arrSpline.GetStartNode();
	last.m_iNodeType = arrSpline.m_iNodeType;
}

void CSplineArray::SampleX(int nSamps, CDoubleArray& arrSample, DRect& rBounds) const
{
	arrSample.SetSize(nSamps);
	int	nSegs = GetSize();
	if (!nSegs) {	// if no segments, zero destination array
		ZeroMemory(arrSample.GetData(), nSamps * sizeof(double));
		return;
	}
	CalcBounds(CSelection(0, nSegs), rBounds);
	double	fHeight = rBounds.Height();
	double	fStep = fHeight / (nSamps - 1);
	double	fFillX;
	int	iSamp;
	if (m_ptStart.y < GetAt(nSegs - 1).m_ptEnd.y) {	// if ascending in y-axis
		int	iSeg = 0;	// forward iterate from first node
		const CSegment	*pSeg = &GetAt(0);
		DPoint	ptStart = m_ptStart;
		double	y1 = ptStart.y;
		for (iSamp = 0; iSamp < nSamps; iSamp++) {	// for each sample
			double	y = y1 + iSamp * fStep;	// compute y
			if (y < ptStart.y || y > pSeg->m_ptEnd.y) {	// if y exceeds segment's range
				iSeg++;	// next segment
				if (iSeg >= nSegs)	// if out of segments
					break;	// exit loop prematurely
				pSeg = &GetAt(iSeg);
				ptStart = GetStartNode(iSeg);
			}
			arrSample[iSamp] = pSeg->FindX(ptStart, y);	// compute segment's x for given y
		}
		fFillX = GetAt(nSegs - 1).m_ptEnd.x;	// fill any remaining samples with last node's x
	} else {	// descending in y-axis
		int	iSeg = nSegs - 1;	// reverse iterate from last node
		const CSegment	*pSeg = &GetAt(iSeg);
		DPoint	ptStart = GetStartNode(iSeg);
		double	y1 = pSeg->m_ptEnd.y;
		for (iSamp = 0; iSamp < nSamps; iSamp++) {	// for each sample
			double	y = y1 + iSamp * fStep;	// compute y
			if (y > ptStart.y || y < pSeg->m_ptEnd.y) {	// if y exceeds segment's range
				iSeg--;	// previous segment
				if (iSeg < 0)	// if out of segments
					break;	// exit loop prematurely
				pSeg = &GetAt(iSeg);
				ptStart = GetStartNode(iSeg);
			}
			arrSample[iSamp] = pSeg->FindX(ptStart, y);	// compute segment's x for given y
		}
		fFillX = m_ptStart.x;	// fill any remaining samples with first node's x
	}
	for (; iSamp < nSamps; iSamp++)	// for any remaining samples
		arrSample[iSamp] = fFillX;
}

bool CSplineArray::HasDuplicateY() const
{
	int	nSegs = GetSize();
	if (!nSegs)
		return false;
	if (m_ptStart.y < GetAt(nSegs - 1).m_ptEnd.y) {	// if ascending in y-axis
		for (int iSeg = 0; iSeg < nSegs; iSeg++) {
			const CSegment&	seg = GetAt(iSeg);
			double	StartY = GetStartNode(iSeg).y;
			if (seg.m_ptEnd.y < StartY || seg.m_ptCtrl[0].y < StartY || seg.m_ptCtrl[1].y > seg.m_ptEnd.y)
				return false;
		}
	} else {	// descending in y-axis
		for (int iSeg = 0; iSeg < nSegs; iSeg++) {
			const CSegment&	seg = GetAt(iSeg);
			double	StartY = GetStartNode(iSeg).y;
			if (seg.m_ptEnd.y > StartY || seg.m_ptCtrl[0].y > StartY || seg.m_ptCtrl[1].y < seg.m_ptEnd.y)
				return false;
		}
	}
	return true;
}

bool CSplineArray::Import(LPCTSTR szPath)
{
	CArrayEx<DPoint, DPoint&>	arrPt;
	{
		CStdioFile	fOut(szPath, CFile::modeRead);
		CString	sLine;
		DPoint	pt;
		while (fOut.ReadString(sLine)) {
			if (_stscanf_s(sLine, _T("%lf %lf"), &pt.x, &pt.y) == 2)
				arrPt.Add(pt);
		}
	}
	int	nPoints = arrPt.GetSize();
	if (!nPoints || (nPoints - 1) % 3) {
		AfxMessageBox(_T("Unexpected number of points."));
		return false;
	}
	int	nSegs = (nPoints - 1) / 3;
	SetSize(nSegs);
	m_ptStart = arrPt[0];
	m_iNodeType = ST_CURVE;
	int	iPt = 1;
	for (int iSeg = 0; iSeg < nSegs; iSeg++) {	// for each segment
		CSegment&	seg = GetAt(iSeg);
		seg.m_ptCtrl[0] = arrPt[iPt + 0]; 
		seg.m_ptCtrl[1] = arrPt[iPt + 1];
		seg.m_ptEnd = arrPt[iPt + 2];
		seg.m_iNodeType = NT_CUSP;	// can't assume smoothness
		seg.m_iSegType = ST_CURVE;
		iPt += 3;
	}
	return true;
}

void CSplineArray::Export(LPCTSTR szPath) const
{
	CStdioFile	fOut(szPath, CFile::modeCreate | CFile::modeWrite);
	CString	s;
	const DPoint&	ptStart = GetStartNode();
	s.Format(_T("%.17g\t%.17g\n"), ptStart.x, ptStart.y);
	fOut.WriteString(s);
	int	nSegs = GetSize();
	for (int iSeg = 0; iSeg < nSegs; iSeg++) {	// for each segment
		const CSegment&	seg = GetAt(iSeg);
		s.Format(_T("%.17g\t%.17g\n%.17g\t%.17g\n%.17g\t%.17g\n"), 
			seg.m_ptCtrl[0].x, seg.m_ptCtrl[0].y,
			seg.m_ptCtrl[1].x, seg.m_ptCtrl[1].y,
			seg.m_ptEnd.x, seg.m_ptEnd.y);
		fOut.WriteString(s);
	}
}
