// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      12mar17	initial version
		
*/

#pragma once

#include "D3DGraphics.h"
#include "DibEx.h"
#include "PotProperties.h"
#include "Spline.h"

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
		ST_TEXTURE	= 0x00010000,	// show texture mapping
		ST_BOUNDS	= 0x00020000,	// show bounding box
	};

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

// Operations
	bool	MakeMesh(bool bResizing = false);
	bool	MakeTexture();
	bool	UpdateTexture();
	bool	ExportPLY(LPCTSTR szPath, bool bVertexColor = false);
	bool	ExportSTL(LPCTSTR szPath);
	bool	ExportOBJ(LPCTSTR szPath, int nPrecision = 6);
	void	UpdateAnimation();
	void	PlotProperty(int iProp, CArrayEx<DPoint, DPoint&>& arrPoint, CRange<double> *pRange);
	void	CalcSpline(const CSplineArray& arrSpline);
	bool	IsSplinePositiveInX() const;
	bool	ComputeBounds(D3DXVECTOR3& p1, D3DXVECTOR3& p2);
	static	COLORREF	Interpolate(const CDibEx& dib, double x, double y);

protected:
// Constants
	enum {
		WALL_OUTER,
		WALL_INNER,
		WALLS
	};

// Types
	class C3DVertexBuffer : public WObject {
	public:
		CComPtr<IDirect3DVertexBuffer9>	m_pVertBuf;	// COM pointer to vertex buffer
		int		m_nPrimitives;	// number of primitives in vertex buffer
		int		m_iMaterial;	// index into array of material properties
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
	class C3DVertexNormalColor : public C3DVertexNormal {
	public:
		D3DCOLOR	c;	// vertex color
	};
	typedef CArrayEx<C3DVertexBuffer, C3DVertexBuffer&> C3DVertexBufferArray;
	typedef CArrayEx<C3DVertexNormal, C3DVertexNormal&> C3DVertexNormalArray;
	typedef CArrayEx<C3DVertexNormalTexture, C3DVertexNormalTexture&> C3DVertexNormalTextureArray;
	typedef CArrayEx<C3DVertexNormalColor, C3DVertexNormalColor&> C3DVertexNormalColorArray;
	typedef C3DVertexNormalTexture CVertex;
	typedef C3DVertexNormalTextureArray CVertexArray;
	class CAdjacency {
	public:
		int		m_nFaces;			// number of faces stored in array
		int		m_arrFaceIdx[6];	// array of adjacent face indices
		void	Add(int iFace);
		void	Weld(CAdjacency& adj);
	};

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
	static const D3DMATERIAL9	m_materialPot;	// pot material properties
	static const D3DMATERIAL9	m_materialBounds;	// bounds material properties
	CArrayEx<D3DXVECTOR3, D3DXVECTOR3&>	m_arrFaceNormal;	// array of face normals
	CArrayEx<CAdjacency, CAdjacency&>	m_arrAdjaceny;		// array of adjaceny lists
	int		m_nAdjRings;		// number of rings in adjacency array
	int		m_nAdjSides;		// number of sides in adjacency array
	CPotProperties	m_arrSrcProps;	// source properties, used during modulation
	CBoundArray<int, PROPERTIES>	m_arrModIdx;	// array of indices of properties to be modulated
	bool	m_bModulatingMesh;	// true if modulating mesh
	CDoubleArray	m_arrSpline;	// spline as array of ring radii
	DRect	m_rSplineBounds;	// spline bounds

// Overrides
	virtual	bool	OnCreate(bool bResizing);
	virtual	void	OnDestroy();
	virtual	bool	OnRender();
	virtual	void	OnError();

// Helpers
	bool	MakeVertexBuffer(C3DVertexBuffer& Dst, const void *pSrc, int nBufSize, int nPrimitives, DWORD dwFVF);
	void	CalcPotMesh();
	void	GetTexture(double fRing, double fSide, D3DXVECTOR2& t) const;
	static	void	AddTri(CDWordArrayEx& arrIdx, int& iIdx, int i0, int i1, int i2);
	static	void	AddQuad(CDWordArrayEx& arrIdx, int& iIdx, int iA0, int iA1, int iA2, int iB0, int iB1, int iB2);
	static	double	Wrap(double x, double y);
	bool	CreateBoundingBox();
	bool	DrawBoundingBox();
	static	void	ApplyMotif(int iMotif, double& r);
	static	double	GetWave(int iWaveform, double fPhase);
	void	ApplyModulations(double fRing);
	void	SaveModulatedProperties();
	void	RestoreModulatedProperties();
	bool	ExportTexture(LPCTSTR szPath, CString& sNewPath);
	static	double	ConvertPropertyToDouble(LPCVOID pSrc, const type_info *pType);
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
