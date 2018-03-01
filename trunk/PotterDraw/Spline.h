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

#include "ArrayEx.h"
#include "DRect.h"

#ifndef CDoubleArray
typedef CArrayEx<double, double> CDoubleArray;
#endif

class CSplineBase {
public:
// Constants
	enum {	// point types, in GDI order
		PT_START,		// start node
		PT_CTRL_1,		// first control point
		PT_CTRL_2,		// second control point
		PT_END,			// end node
		POINT_TYPES
	};
	enum {	// segment types
		ST_CURVE,		// Bezier curve
		ST_LINE,		// straight line; control vectors have zero length
		SEGMENT_TYPES
	};
	enum {	// node types
		NT_CUSP,		// control vectors are unconstrained
		NT_SMOOTH,		// control vectors are colinear
		NT_SYMMETRICAL,	// control vectors are colinear and have same length
		NODE_TYPES
	};
	enum {	// transform relative to
		RT_ORIGIN,				// coordinate space origin
		RT_SELECTION_CENTER,	// center of selected segments' bounding rectangle
		RELATIVE_TOS
	};
	enum {	// translate flags
		TF_SKIP_FIRST_NODE			= 0x01,	// skip first node in range of segments
		TF_SKIP_LAST_NODE			= 0x02,	// skip last node in range of segments
		TF_TRANSLATE_ADJACENT_CTRLS	= 0x04,	// translate control points adjacent to range
		TF_RECALC_ADJACENT_CTRLS	= 0x08,	// recalculate control points adjacent to range
	};

// Attributes
	static	bool	IsNode(int iPointType);
	static	bool	IsValidPointType(int iPointType);
	static	bool	IsValidSegmentType(int iSegType);
	static	bool	IsValidNodeType(int iNodeType);

// Types
	typedef CArrayEx<CPoint, CPoint&> CPointArray;

// Helpers

// Nested classes
	class CSegment {
	public:
	// Data members
		CSegment();
		CSegment(DPoint ptCtrl0, DPoint ptCtrl1, DPoint ptEnd, int SegType, int iNodeType);
		DPoint	m_ptCtrl[2];	// pair of control points
		DPoint	m_ptEnd;		// end node
		int		m_iSegType;		// segment type; see enum above
		int		m_iNodeType;	// node type; see enum above

	// Operations
		double	FindX(DPoint ptStart, double fTargetY) const;
		static	double	Eval(double t, double x1, double x2, double x3, double x4);
		void	operator+=(const DPoint& pt);
		void	operator-=(const DPoint& pt);
		void	Scale(DPoint ptOrigin, DPoint ptScaling);
	};
	class CSelection {
	public:
		CSelection();
		CSelection(int iSeg, int nSegs);
		int		End() const;
		bool	IsEmpty() const;
		void	SetEmpty();
		int		m_iSeg;			// index of first segment in range, or -1 if empty
		int		m_nSegs;		// number of segments in range, or zero if empty
	};
};

inline bool CSplineBase::IsNode(int iPointType)
{
	return iPointType == PT_START || iPointType == PT_END;
}

inline bool CSplineBase::IsValidPointType(int iPointType)
{
	return iPointType >= 0 && iPointType < POINT_TYPES;
}

inline bool CSplineBase::IsValidSegmentType(int iSegType)
{
	return iSegType >= 0 && iSegType < SEGMENT_TYPES;
}

inline bool CSplineBase::IsValidNodeType(int iNodeType)
{
	return iNodeType >= 0 && iNodeType < NODE_TYPES;
}

inline CSplineBase::CSegment::CSegment()
{
}

inline CSplineBase::CSegment::CSegment(DPoint ptCtrl0, DPoint ptCtrl1, DPoint ptEnd, int iSegType, int iNodeType)
{
	m_ptCtrl[0] = ptCtrl0;
	m_ptCtrl[1] = ptCtrl1;
	m_ptEnd = ptEnd;
	m_iSegType = iSegType;
	m_iNodeType = iNodeType;
}

inline CSplineBase::CSelection::CSelection()
{
}

inline CSplineBase::CSelection::CSelection(int iSeg, int nSegs)
{
	m_iSeg = iSeg;
	m_nSegs = nSegs;
}

inline int CSplineBase::CSelection::End() const
{
	return m_iSeg + m_nSegs;
}

inline bool CSplineBase::CSelection::IsEmpty() const
{
	return m_nSegs <= 0;
}

inline void CSplineBase::CSelection::SetEmpty()
{
	m_iSeg = -1;
	m_nSegs = 0;
}

class CSplineArray : public CArrayEx<CSplineBase::CSegment, CSplineBase::CSegment&>, public CSplineBase {
public:
// Construction
	CSplineArray();
	CSplineArray(const CSplineArray& arr);
	CSplineArray&	CSplineArray::operator=(const CSplineArray& arr);
	void	Copy(const CSplineArray& src);

// Attributes
	const DPoint&	GetStartNode() const;
	DPoint&	GetStartNode();
	const DPoint&	GetStartNode(int iSeg) const;
	DPoint&		GetStartNode(int iSeg);
	void	SetStartNode(DPoint pt);
	bool	IsValid(CSelection sel) const;
	void	GetSegments(CSelection sel, CSplineArray& arrSpline) const;
	void	GetPoints(CSelection sel, CPointArray& arrPt) const;
	void	GetPath(CDC& dc, int iSeg, CPointArray& arrSplinePt, CPointArray& arrPathPt, CByteArray& arrVertexType) const;
	DPoint	GetPan() const;
	void	SetPan(DPoint ptPan);
	DPoint	GetScale() const;
	void	SetScale(DPoint ptScale);
	DPoint	GetOrigin() const;
	void	SetOrigin(DPoint ptOrigin);
	int		GetSegmentType(int iSeg) const;
	void	SetSegmentType(int iSeg, int iSegType);
	int		GetNodeType(int iSeg, int iPointType) const;
	int&	GetNodeType(int iSeg);
	void	SetNodeType(int iSeg, int iPointType, int iNodeType);
	int		GetSegmentType(CSelection sel) const;
	void	SetSegmentType(CSelection sel, int iSegType);
	int		GetNodeType(CSelection sel) const;
	void	SetNodeType(CSelection sel, int iNodeType);
	DPoint	GetPoint(int iSeg, int iPointType) const;
	void	SetPoint(int iSeg, int iPointType, DPoint ptNew);
	DPoint	GetNearestNode(int iSeg, DPoint pt) const;
	bool	HasDuplicateY() const;

// Operations
	void	DumpSegments() const;
	DPoint	Normalize(DPoint pt) const;
	DPoint	Denormalize(DPoint pt) const;
	int		AddNode(int iSeg, DPoint ptNode, DPoint p1, DPoint p2, double fCtrlVecLen, double fGridStep = 0);
	void	DeleteNode(int iSeg, int iPointType);
	void	DeleteSegments(CSelection sel, bool bMerge = false);
	void	InsertSegments(int iSeg, const CSplineArray& arrSpline);
	void	CalcBounds(CSelection sel, DRect& rBounds) const;
	DPoint	CalcCenter(CSelection sel, DRect& rBounds) const;
	void	Translate(CSelection sel, DPoint ptTranslation, UINT nFlags = 0);
	void	Scale(CSelection sel, DPoint ptScaling, int iRelativeTo);
	void	Rotate(CSelection sel, double fRotation, int iRelativeTo);
	void	Flip(CSelection sel, bool bVertical = false);
	void	Reverse();
	void	SampleX(int nSamps, CDoubleArray& arrSample, DRect& rBounds) const;
	bool	Import(LPCTSTR szPath);
	void	Export(LPCTSTR szPath) const;
	static	double	Wrap(double x, double y);
	static	void	SnapToGrid(DPoint& pt, double fGridStep);

protected:
// Data members
	DPoint	m_ptStart;		// first segment's start node
	DPoint	m_ptPan;		// pre-scaling pan translation in normalized coordinates
	DPoint	m_ptOrigin;		// post-scaling origin translation in display coordinates
	DPoint	m_ptScale;		// scaling ratio between normalized and display coordinates
	int		m_iNodeType;	// first node's type

// Helpers
	void	AdjustControlPt(int iNodeType, DPoint ptNode, DPoint ptControl1, DPoint& ptControl2) const;
	void	AdjustPrevControlPt(int iSeg);
	void	AdjustNextControlPt(int iSeg);
};

inline CSplineArray::CSplineArray(const CSplineArray& arr)
{
	*this = arr;
}

inline CSplineArray& CSplineArray::operator=(const CSplineArray& arr)
{
	Copy(arr);
	return *this;
}

inline const DPoint& CSplineArray::GetStartNode() const
{
	return m_ptStart;
}

inline DPoint& CSplineArray::GetStartNode()
{
	return m_ptStart;
}

inline void CSplineArray::SetStartNode(DPoint pt)
{
	m_ptStart = pt;
}

inline const DPoint& CSplineArray::GetStartNode(int iSeg) const
{
	if (iSeg > 0)	// if not first segment
		return GetAt(iSeg - 1).m_ptEnd;	// return previous segment's end point
	return m_ptStart;	// return first segment's start node
}

inline DPoint& CSplineArray::GetStartNode(int iSeg)
{
	if (iSeg > 0)	// if not first segment
		return GetAt(iSeg - 1).m_ptEnd;	// return previous segment's end point
	return m_ptStart;	// return first segment's start node
}

inline bool CSplineArray::IsValid(CSelection sel) const
{
	return sel.m_iSeg >= 0 && sel.m_iSeg + sel.m_nSegs <= GetSize();
}

inline int CSplineArray::GetSegmentType(int iSeg) const
{
	return GetAt(iSeg).m_iSegType;
}

inline int& CSplineArray::GetNodeType(int iSeg)
{
	if (iSeg > 0)	// if not first segment
		return GetAt(iSeg - 1).m_iNodeType;	// return previous segment's node type
	return m_iNodeType;	// return first segment's node type
}

inline DPoint CSplineArray::GetPan() const
{
	return m_ptPan;
}

inline void CSplineArray::SetPan(DPoint ptPan)
{
	m_ptPan = ptPan;
}

inline DPoint CSplineArray::GetScale() const
{
	return m_ptScale;
}

inline void CSplineArray::SetScale(DPoint ptScale)
{
	m_ptScale = ptScale;
}

inline DPoint CSplineArray::GetOrigin() const
{
	return m_ptOrigin;
}

inline void CSplineArray::SetOrigin(DPoint ptOrigin)
{
	m_ptOrigin = ptOrigin;
}

inline DPoint CSplineArray::Normalize(DPoint pt) const
{
	return (pt - m_ptOrigin) / m_ptScale - m_ptPan;
}

inline DPoint CSplineArray::Denormalize(DPoint pt) const
{
	return (pt + m_ptPan) * m_ptScale + m_ptOrigin;
}
