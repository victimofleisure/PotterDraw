// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      12mar17	initial version
		01		09oct17	in GetWave, add pulse and rounded pulse waves
		02		06nov17	add get/set pot material
		03		14nov17	add outer radius array
		04		15nov17	optimize modulo one wrapping
		05		23nov17	add modulation type flags
		06		06dec17	add hit test and face and vertex accessors
		07		06dec17	add optional showing of normals and face selection
		08		10dec17	add azimuth and incline color patterns
		09		20feb18	add secondary modulation
		
*/

#pragma once

#include "D3DGraphics.h"
#include "DibEx.h"
#include "PotProperties.h"
#include "Spline.h"
#include "Range.h"

#define	POT_GFX_SHOW_FACE_NORMALS 0		// non-zero to enable showing face normals
#define	POT_GFX_SHOW_VERTEX_NORMALS 0	// non-zero to enable showing vertex normals
#define	POT_GFX_SHOW_FACE_SELECTION 0	// non-zero to enable showing face selection

class DPoint;	// for plotting
template<class T> class CRange;	// for plotting

class CPotGraphics : public CD3DGraphics, public CPotProperties
{
public:
// Construction
	CPotGraphics();
	virtual	~CPotGraphics();

// Constants
	enum {	// derived styles; low word reserved for base class
		ST_TEXTURE			= 0x00010000,	// show texture mapping
		ST_BOUNDS			= 0x00020000,	// show bounding box
		ST_FACE_NORMALS		= 0x00040000,	// show face normals (if supported)
		ST_VERTEX_NORMALS	= 0x00080000,	// show vertex normals (if supported)
	};

// Types
	struct CFace {
		int	arrIdx[3];	// array of vertex indices
	};
	class C3DVertexNormal {	// binary copy OK
	public:
		D3DXVECTOR3	pt;	// 3D point
		D3DXVECTOR3	n;	// normal at point
	};
	class C3DVertexNormalTexture : public C3DVertexNormal {
	public:
		D3DXVECTOR2	t;	// texture coords
	};
	typedef C3DVertexNormalTexture CVertex;

// Attributes
	virtual	bool	SetStyle(UINT nStyle);
	void	GetProperties(CPotProperties& Props) const;
	void	SetProperties(const CPotProperties& Props);
	void	GetAnimationState(CPotProperties& Props) const;
	void	GetViewState(CPotProperties& Props) const;
	void	SetViewState(const CPotProperties& Props);
	int		GetVertexCount() const;
	int		GetIndexCount() const;
	int		GetFaceCount() const;
	void	GetPotMaterial(D3DMATERIAL9& mtrlPot) const;
	bool	SetPotMaterial(const D3DMATERIAL9& mtrlPot);
	static const D3DVECTOR&	GetDefaultLightDir();
	void	GetOuterRadiusRange(double& fMinRadius, double& fMaxRadius) const;
	int		GetModulationCount() const;
	void	GetModulations(CBoundArray<int, PROPERTIES>& arrModIdx) const;
	UINT	GetModulationState() const;
	void	GetFace(int iFace, CFace& Face) const;
	void	GetFaceNormal(int iFace, D3DXVECTOR3& vNormal) const;
	void	GetVertex(int iVertex, CVertex& Vert) const;
	void	GetVertexCoords(int iVertex, int& iWall, int& iRing, int& iSide) const;
	void	GetSelection(CIntArrayEx& arrFaceIdx) const;
	bool	SetSelection(const CIntArrayEx& arrFaceIdx);

// Operations
	bool	MakeMesh(bool bResizing = false);
	bool	MakeTexture();
	bool	UpdateTextureCoords();
	bool	ExportPLY(LPCTSTR szPath, bool bVertexColor = false);
	bool	ExportSTL(LPCTSTR szPath);
	bool	ExportOBJ(LPCTSTR szPath, int nPrecision = 6);
	void	UpdateAnimation();
	void	PlotProperty(int iProp, CArrayEx<DPoint, DPoint&>& arrPoint, CRange<double> *pRange);
	void	CalcSpline(const CSplineArray& arrSpline);
	bool	IsSplinePositiveInX() const;
	bool	ComputeBounds(D3DXVECTOR3& p1, D3DXVECTOR3& p2);
	void	ComputeOuterRadiusRange(double& fMinRadius, double& fMaxRadius) const;
	static	COLORREF	Interpolate(const CDibEx& dib, double x, double y);
	int		HitTest(CPoint point) const;
	void	OnModulationChange();

protected:
// Constants
	enum {
		WALL_OUTER,
		WALL_INNER,
		WALLS
	};
	enum {	// CalcModulationRange flags
		CMR_FORCE_BIPOLAR	= 0x01,		// use bipolar regardless of modulation's properties
	};

// Types
	class C3DVertexBuffer : public WObject {
	public:
		CComPtr<IDirect3DVertexBuffer9>	m_pVertBuf;	// COM pointer to vertex buffer
		int		m_nPrimitives;	// number of primitives in vertex buffer
		int		m_iMaterial;	// index into array of material properties
	};
	struct C3DLine {
		D3DVECTOR	v[2];	// 3D line segment end points
	};
	struct C3DTriangle {
		D3DVECTOR	v[3];	// 3D triangle vertices
	};
	typedef CArrayEx<C3DVertexBuffer, C3DVertexBuffer&> C3DVertexBufferArray;
	typedef CArrayEx<CVertex, CVertex&> CVertexArray;
	typedef CArrayEx<C3DLine, C3DLine&> C3DLineArray;
	typedef CArrayEx<C3DTriangle, C3DTriangle&> C3DTriangleArray;
	class CAdjacency {
	public:
		int		m_nFaces;			// number of faces stored in array
		int		m_arrFaceIdx[6];	// array of adjacent face indices
		void	Add(int iFace);
		void	Weld(CAdjacency& adj);
	};
	typedef CRange<double> CDblRange;

// Data members
	CComPtr<ID3DXMesh>	m_pMesh;	// mesh
	CComPtr<ID3DXMesh>	m_pBounds;	// bounding box
	CVertexArray	m_arrVert;	// array of vertices
	CDWordArrayEx	m_arrIdx;	// array of vertex indices; three per face
	CComPtr<IDirect3DTexture9>	m_pTexture;	// texture
	CDibEx	m_dibTexture;		// texture bitmap
	int		m_nFaces;			// number of faces
	double	m_fColorPos;		// color position
	double	m_fColorDelta;		// color change per frame
	static	bool	m_bShowingErrorMsg;	// true while displaying error message box
	D3DXVECTOR3	m_vBounds[2];	// bounding box corners
	static const D3DMATERIAL9	m_mtrlBounds;	// bounds material properties
	CArrayEx<D3DXVECTOR3, D3DXVECTOR3&>	m_arrFaceNormal;	// array of face normals
	CArrayEx<CAdjacency, CAdjacency&>	m_arrAdjaceny;		// array of adjaceny lists
	int		m_nAdjRings;		// number of rings in adjacency array
	int		m_nAdjSides;		// number of sides in adjacency array
	CFixedArray<double, PROPERTIES>	m_arrSrcProps;	// source properties, used during modulation
	CFixedArray<double, MODULATIONS>	m_arrSrcMods;	// secondary modulation source properties
	CBoundArray<int, PROPERTIES>	m_arrModIdx;	// array of indices of properties to be modulated
	CIntArrayEx	m_arrMod2Idx;	// array of indices of secondary modulations
	CIntArrayEx	m_arrPlotMod2Idx;	// array of indices of plotted secondary modulations
	UINT	m_nModState;		// modulation state bitmask; flags defined in PotProperties.h
	int		m_iCurPlotProp;		// index of property that's currently plotted, or -1 if none
	CDoubleArray	m_arrSpline;	// spline as array of ring radii
	DRect	m_rSplineBounds;	// spline bounds
	CDoubleArray	m_faOuterRadius;	// 2D array of outer wall radii
	double	m_fMinOuterRadius;	// minimum radius of outer wall
	double	m_fMaxOuterRadius;	// maximum radius of outer wall
	double	m_fOuterRadiusScale;	// normalizes outer wall radii for radius color pattern

// Overrides
	virtual	bool	OnCreate(bool bResizing);
	virtual	void	OnDestroy();
	virtual	bool	OnRender();
	virtual	void	OnError();

// Helpers
	bool	MakeVertexBuffer(C3DVertexBuffer& Dst, const void *pSrc, int nBufSize, int nPrimitives, DWORD dwFVF);
	void	CalcPotMesh();
	void	GetTextureCoords(double fRing, double fSide, D3DXVECTOR2& t) const;
	static	void	AddTri(CDWordArrayEx& arrIdx, int& iIdx, int i0, int i1, int i2);
	static	void	AddQuad(CDWordArrayEx& arrIdx, int& iIdx, int iA0, int iA1, int iA2, int iB0, int iB1, int iB2);
	static	double	Wrap(double x, double y);
	static	double	Wrap1(double x);
	static	double	Square(double x);
	bool	CreateBoundingBox();
	bool	DrawBoundingBox();
	static	void	ApplyMotif(int iMotif, double& r);
	static	void	ApplyPower(int iRange, int iPowerType, double fPower, double& r);
	static	double	GetWave(int iWaveform, double fPhase, double fPulseWidth, double fSlew);
	static	void	Modulate(double fTheta, const PROPERTY_INFO& info, const CModulationProps& mod, LPCVOID pvSource, LPVOID pvTarget);
	CDblRange	CalcModulationRange(int iProp, UINT nFlags = 0, const double *pfAmplitude = NULL) const;
	void	ApplyModulations(double fRing);
	void	SaveModulatedProperties();
	void	RestoreModulatedProperties();
	static	void	PropCopy(LPVOID pDst, LPCVOID pSrc, size_t nLen);
	bool	ExportTexture(LPCTSTR szPath, CString& sNewPath);
	static	double	ConvertPropertyToDouble(LPCVOID pSrc, const type_info *pType);
	int		GetVertexIdx(double fRing, double fSide) const;
	static	double	GetAzimuth(const D3DXVECTOR3& vNormal, double fAzimuth);
	static	double	GetIncline(const D3DXVECTOR3& vNormal);

// Optional features
#if POT_GFX_SHOW_FACE_NORMALS
	C3DVertexBuffer	m_vbFaceNormalLine;	// vertex buffer for face normal line segments
	bool	CreateFaceNormalLines();
	bool	DrawFaceNormalLines();
#endif	// POT_GFX_SHOW_FACE_NORMALS
#if POT_GFX_SHOW_VERTEX_NORMALS
	C3DVertexBuffer	m_vbVertexNormalLine;	// vertex buffer for vertex normal line segments
	bool	CreateVertexNormalLines();
	bool	DrawVertexNormalLines();
#endif	// POT_GFX_SHOW_VERTEX_NORMALS
#if POT_GFX_SHOW_FACE_SELECTION
	CIntArrayEx	m_arrSelectedFaceIdx;	// array of selected face indices
	C3DVertexBuffer	m_vbSelectedFace;	// vertex buffer for selected faces
	bool	CreateSelectedFaces();
	bool	DrawSelectedFaces();
#endif	// POT_GFX_SHOW_FACE_SELECTION
};

inline void CPotGraphics::GetProperties(CPotProperties& Props) const
{
	Props = *this;
}

inline int CPotGraphics::GetVertexCount() const
{
	return m_arrVert.GetSize();
}

inline int CPotGraphics::GetIndexCount() const
{
	return m_arrIdx.GetSize();
}

inline int CPotGraphics::GetFaceCount() const
{
	return m_nFaces;
}

inline void CPotGraphics::GetPotMaterial(D3DMATERIAL9& mtrlPot) const
{
	mtrlPot = m_mtrlPot;
}

inline const D3DVECTOR& CPotGraphics::GetDefaultLightDir()
{
	return m_vDefaultLightDir;
}

inline void CPotGraphics::GetOuterRadiusRange(double& fMinRadius, double& fMaxRadius) const
{
	fMaxRadius = m_fMaxOuterRadius;
	fMinRadius = m_fMinOuterRadius;
}

inline int CPotGraphics::GetModulationCount() const
{
	return m_arrModIdx.GetSize();
}

inline void CPotGraphics::GetModulations(CBoundArray<int, PROPERTIES>& arrModIdx) const
{
	arrModIdx = m_arrModIdx;
}

inline UINT CPotGraphics::GetModulationState() const
{
	return m_nModState;
}

inline void CPotGraphics::GetFace(int iFace, CFace& Face) const
{
	int	iIdx = iFace * 3;
	Face.arrIdx[0] = m_arrIdx[iIdx + 0];
	Face.arrIdx[1] = m_arrIdx[iIdx + 1];
	Face.arrIdx[2] = m_arrIdx[iIdx + 2];
}

inline void CPotGraphics::GetFaceNormal(int iFace, D3DXVECTOR3& vNormal) const
{
	vNormal = m_arrFaceNormal[iFace];
}

inline void CPotGraphics::GetVertex(int iVertex, CVertex& Vert) const
{
	Vert = m_arrVert[iVertex];
}
