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

#include "stdafx.h"
#include "resource.h"
#include "PotGraphics.h"
#include "Benchmark.h"
#include "PathStr.h"
#include "DPoint.h"	// for plotting
#include "Range.h"	// for plotting
typedef CRange<double> CDblRange;

#define _USE_MATH_DEFINES	// for trig constants
#include <math.h>

#define CHECK(x) if (FAILED(m_hLastError = x)) { OnError(); return false; }

bool CPotGraphics::m_bShowingErrorMsg;

const D3DMATERIAL9 CPotGraphics::m_materialPot = {{0.8f, 0.8f, 0.8f}, {0.2f, 0.2f, 0.2f}, {1, 1, 1}, {0}, 1000};
const D3DMATERIAL9 CPotGraphics::m_materialBounds = {{0}, {0}, {0}, {0, 1, 0}};

#define CALC_NORMALS 1	// non-zero to calculate normals; zero to use D3DXComputeNormals (very slow)

CPotGraphics::CPotGraphics()
{
	m_nStyle |= ST_TEXTURE;
	m_nFaces = 0;
	m_fColorPos = 0;
	m_fColorDelta = 1 / m_fFrameRate;
	m_nAdjRings = 0;
	m_nAdjSides = 0;
	m_bModulatingMesh = false;
}

CPotGraphics::~CPotGraphics()
{
}

bool CPotGraphics::OnCreate(bool bResizing)
{
	return MakeMesh(bResizing);
}

void CPotGraphics::OnDestroy()
{
	m_pMesh = NULL;
	m_pTexture = NULL;
}

bool CPotGraphics::OnRender()
{
	if (m_pMesh != NULL) {
		CHECK(m_pMesh->DrawSubset(0));
	}
	if (m_pBounds != NULL) {	// if bounding box exists
		DrawBoundingBox();
	}
	return true;
}

void CPotGraphics::OnError()
{
	CD3DGraphics::OnError();	// let base class handle error first
	if (!m_bShowingErrorMsg) {	// if not already showing error message
		HRESULT	hLastError = m_hLastError;
		m_bShowingErrorMsg = true;	// set reentry guard
		CString	sMsg;
		sMsg.Format(_T("Direct3d error: %s\n%s (0x%x)"), GetLastErrorDescription(), 
			GetLastErrorName(), m_hLastError);
		AfxMessageBox(sMsg);	// display error message
		if (hLastError == D3DERR_DEVICELOST)	// if device was lost
			Resize(m_szClient);	// try to reset device
		m_bShowingErrorMsg = false;	// reset reentry guard
	}
}

bool CPotGraphics::SetStyle(UINT nStyle)
{
	if ((nStyle & ST_BOUNDS) ^ (m_nStyle & ST_BOUNDS)) {	// if show bounds state changed
		if (nStyle & ST_BOUNDS) {	// if showing bounds
			ComputeBounds(m_vBounds[0], m_vBounds[1]);
			CreateBoundingBox();
		} else
			m_pBounds = NULL;
	}
	if (!CD3DGraphics::SetStyle(nStyle))
		return false;
	CHECK(m_pDevice->SetTexture(0, (nStyle & ST_TEXTURE) != 0 ? m_pTexture : NULL));
	return true;
}

void CPotGraphics::SetProperties(const CPotProperties& Props)
{
	CPotProperties::operator=(Props);
	SetBkgndColor(CvtFromColorRef(m_clrBackground));
	m_bModulatingMesh = GetModulations(m_arrModIdx);
}

void CPotGraphics::GetViewState(CPotProperties& Props) const
{
	Props.m_nRenderStyle = m_nStyle;
	Props.m_vRotation = CD3DGraphics::m_vRotation;
	Props.m_vPan = CD3DGraphics::m_vPan;
	Props.m_fZoom = CD3DGraphics::m_fZoom;
}

void CPotGraphics::SetViewState(const CPotProperties& Props)
{
	m_nStyle = Props.m_nRenderStyle;
	CD3DGraphics::m_vRotation = Props.m_vRotation;
	CD3DGraphics::m_vPan = Props.m_vPan;
	CD3DGraphics::m_fZoom = Props.m_fZoom;
}

bool CPotGraphics::MakeVertexBuffer(C3DVertexBuffer& Dst, const void *pSrc, int nBufSize, int nPrimitives, DWORD dwFVF)
{
	Dst.m_pVertBuf = NULL;	// release previous buffer if any
	CHECK(m_pDevice->CreateVertexBuffer(nBufSize, 0, dwFVF, D3DPOOL_MANAGED, &Dst.m_pVertBuf, NULL));
	LPVOID	pBuf;
	CHECK(Dst.m_pVertBuf->Lock(0, 0, &pBuf, 0));
	CopyMemory(pBuf, pSrc, nBufSize);
	CHECK(Dst.m_pVertBuf->Unlock());
	Dst.m_nPrimitives = nPrimitives;
	return true;
}

bool CPotGraphics::MakeMesh(bool bResizing)
{
//	printf("CPotGraphics::MakeMesh bResizing=%d\n", bResizing);
	if (!bResizing) {	// if not resizing
		CalcPotMesh();
	}
	m_pMesh = NULL;
	m_pBounds = NULL;
	m_pTexture = NULL;
	int	nVerts = m_arrVert.GetSize();
	int	nIdxs = m_arrIdx.GetSize();
	int	nFaces = m_nFaces;
	UINT	nFVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;
	CHECK(D3DXCreateMeshFVF(nFaces, nVerts, D3DXMESH_32BIT, nFVF, m_pDevice, &m_pMesh));
	CVertex	*pVert;
	CHECK(m_pMesh->LockVertexBuffer(0, (void **)&pVert));
	for (int iVert = 0; iVert < nVerts; iVert++)
		pVert[iVert] = m_arrVert[iVert];
	CHECK(m_pMesh->UnlockVertexBuffer());
	DWORD	*pIdx;
	CHECK(m_pMesh->LockIndexBuffer(0, (void **)&pIdx));
	for (int iIdx = 0; iIdx < nIdxs; iIdx++)
		pIdx[iIdx] = m_arrIdx[iIdx];
	CHECK(m_pMesh->UnlockIndexBuffer());
	DWORD	*pAttr;
	CHECK(m_pMesh->LockAttributeBuffer(0, &pAttr));
	for (int iAttr = 0; iAttr < nFaces; iAttr++)
		pAttr[iAttr] = 0;
	CHECK(m_pMesh->UnlockAttributeBuffer());
#if !CALC_NORMALS	// if not calculating normals
//	CBenchmark b;
	// use Direct3D to compute normals; for reference only, very slow
	if (!bResizing) {	// if not resizing
		CDWordArray	adj;
		adj.SetSize(nIdxs);
		printf("generating adjaceny\n");
		CHECK(m_pMesh->GenerateAdjacency(0.001f, adj.GetData()));
		printf("computing normals\n");
		CHECK(D3DXComputeNormals(m_pMesh, adj.GetData()));	// compute normals
//		printf("validating mesh\n");
//		CHECK(D3DXValidMesh(m_pMesh, adj.GetData(), NULL));	// validate mesh
		printf("storing normals\n");
		CHECK(m_pMesh->LockVertexBuffer(0, (void **)&pVert));	// store normals
		for (int iVert = 0; iVert < nVerts; iVert++)
			m_arrVert[iVert].n = pVert[iVert].n;
		CHECK(m_pMesh->UnlockVertexBuffer());
	}
//	printf("%f\n", b.Elapsed());
#endif	// !CALC_NORMALS
//	printf("verts=%d faces=%d \n", m_pMesh->GetNumVertices(), m_pMesh->GetNumFaces());
	CHECK(m_pDevice->SetMaterial(&m_materialPot));
	if (!MakeTexture())
		return false;
	if (m_nStyle & ST_BOUNDS)
		CreateBoundingBox();
//m_pMesh = NULL;
//CHECK(D3DXCreateCylinder(m_pDevice, 130, 100, 200, 50, 10, &m_pMesh, NULL));
	return true;
}

void CPotGraphics::CalcPotMesh()
{
	int	nRings = m_nRings;
	int	nSides = m_nSides;
	double	fHeight = m_rSplineBounds.Height();
	double	fZStep = fHeight / nRings;
	int	nStride = nSides + 1;
	int	nWallStride = nRings * nStride;
	int	nWallVerts = WALLS * nWallStride;
	int	nBottomVerts = WALLS;	// two center points
	int	nVerts = nWallVerts + nBottomVerts;
	int	iFirstInnerRing = round(m_fWallThickness / fZStep);
	iFirstInnerRing = CLAMP(iFirstInnerRing, 1, nRings - 1);
	int	nOuterFaces = nSides * (nRings - 1) * 2;	// two triangles per quad
	int	nInnerFaces = nSides * (nRings - 1 - iFirstInnerRing) * 2;
	int	nWallFaces = nOuterFaces + nInnerFaces;
	int nBottomFaces = WALLS * nSides;
	int	nNeckFaces = nSides * 2;
	int	nFaces = nWallFaces + nBottomFaces + nNeckFaces;
	m_nFaces = nFaces;
	int	nIdxs = nFaces * 3;	// three indices per triangle
	D3DXMATRIX	matRot;
	D3DXMatrixRotationYawPitchRoll(&matRot, 0, -float(M_PI) / 2, 0);
	m_arrVert.SetSize(nVerts);
	m_arrIdx.SetSize(nIdxs);
	int	iWall, iRing, iSide;
	int	iVert = 0;
	double	fDelta = M_PI * 2 / nSides;
	bool	bScallops = HasScallops();
	bool	bRipples = HasRipples();
	bool	bBends = HasBends();
	bool	bHelix = HasHelix();
	double	fRingBend = 0;
	D3DXVECTOR2	vOrigin(0, 0);
	CArrayEx<CDoubleArray, CDoubleArray&> faOuterRad;
	faOuterRad.SetSize(nRings);
	for (iRing = 0; iRing < nRings; iRing++)
		faOuterRad[iRing].SetSize(nStride);
	int	nMods = m_arrModIdx.GetSize();
	if (nMods)
		SaveModulatedProperties();
//	printf("generating vertices\n");
	// vertices for inner and outer walls
	double	fRingRad = 0;	// init to avoid warning
	for (iWall = 0; iWall < WALLS; iWall++) {	// for each wall
		for (iRing = 0; iRing < nRings; iRing++) {	// for each ring
			double	fRing = double(iRing) / (nRings - 1);	// normalized ring
			if (nMods)
				ApplyModulations(fRing);
			if (iWall == WALL_OUTER) {	// if outer wall
				double	fRad = m_fRadius * m_arrSpline[iRing];
				if (bRipples) {
					double	r = sin((fRing * m_fRipples + m_fRipplePhase) * M_PI * 2) * m_fRippleDepth;
					ApplyMotif(m_iRippleMotif, r);
					fRad += r;
				}
				if (bBends) {
					fRingBend = cos((fRing * m_fBends + m_fBendPhase) * M_PI * 2) * m_fBendDepth;
					ApplyMotif(m_iBendMotif, fRingBend);
				}
				fRingRad = fRad;
			}
			double	fRingTwist = m_fTwist * M_PI * 2 * fRing;	// optimization
			if (bHelix) {
				double	fHelixTheta = fRing * m_fHelixFrequency * M_PI * 2;
				vOrigin.x = float(sin(fHelixTheta) * m_fHelixAmplitude);
				vOrigin.y = float(cos(fHelixTheta) * m_fHelixAmplitude);
			}
			for (iSide = 0; iSide < nSides; iSide++) {	// for each side
				double	fSide = double(iSide) / nSides;	// normalized side
				double	fRad;
				if (iWall == WALL_OUTER) {	// if outer wall
					fRad = fRingRad;
					if (bScallops) {
						double	r = cos(fSide * M_PI * 2 * m_fScallops) * m_fScallopDepth;
						ApplyMotif(m_iScallopMotif, r);
						fRad += r;
					}
					if (bBends) {
						double	r = fRingBend * cos((fSide * m_fBendPoles + m_fBendPolePhase) * M_PI * 2);
						ApplyMotif(m_iBendPoleMotif, r);
						fRad += r;
					}
					faOuterRad[iRing][iSide] = fRad;	// store outer radius for 2nd pass
				} else {	// inner wall
					double	fSlope;
					if (iRing && iRing < nRings - 1)
						fSlope = (faOuterRad[iRing + 1][iSide] - faOuterRad[iRing - 1][iSide]) / (fZStep * 2);
					else
						fSlope = 0;
					double	a = atan(fSlope);
					fRad = faOuterRad[iRing][iSide] - m_fWallThickness / cos(a);
					fRad = max(fRad, 0);	// avoid negative radius
				}
				CVertex&	v = m_arrVert[iVert];
				double	fTheta = fDelta * iSide + fRingTwist;
				double	x = sin(fTheta) * m_fAspectRatio;
				double	y = cos(fTheta);
				double	z = (fRing - 0.5) * fHeight;
				v.pt = D3DXVECTOR3(float(x * fRad) + vOrigin.x, float(y * fRad) + vOrigin.y, float(z));
				D3DXVec3TransformCoord(&v.pt, &v.pt, &matRot);
				GetTexture(fRing, fSide, v.t);
				iVert++;
			}
			CVertex&	v = m_arrVert[iVert];	// ring's final vertex
			v.pt = m_arrVert[iVert - nSides].pt;	// point is same as first vertex
			GetTexture(fRing, 1, v.t);	// but texture differs
			iVert++;
		}
	}
	// vertices for outer and inner bottoms
	if (bHelix) {
		if (nMods)
			ApplyModulations(0);	// in case helix amplitude is modulated
		vOrigin.x = 0;
		vOrigin.y = float(m_fHelixAmplitude);
	}
	if (nMods)
		RestoreModulatedProperties();
	for (int iWall = 0; iWall < WALLS; iWall++) {
		CVertex&	v = m_arrVert[iVert + iWall];
		double	z = -fHeight / 2 + m_fWallThickness * iWall;
		v.pt = D3DXVECTOR3(vOrigin.x, vOrigin.y, float(z));
		D3DXVec3TransformCoord(&v.pt, &v.pt, &matRot);
	}
//	printf("generating indices\n");
	// indices for outer wall
	int	iIdx = 0;
	int	iStart = 0;
	for (iRing = 0; iRing < nRings - 1; iRing++) {
		for (iSide = 0; iSide < nSides; iSide++) {
			int	iPos1 = iStart + iSide;
			int	iPos2 = iPos1 + 1;
			AddQuad(m_arrIdx, iIdx, iPos1, iPos1 + nStride, iPos2, // CCW
				iPos2, iPos1 + nStride, iPos2 + nStride);
		}
		iStart += nStride;
	}
	// indices for inner wall
	iStart = (nRings + iFirstInnerRing) * nStride;
	for (iRing = iFirstInnerRing; iRing < nRings - 1; iRing++) {
		for (iSide = 0; iSide < nSides; iSide++) {
			int	iPos1 = iStart + iSide;
			int	iPos2 = iPos1 + 1;
			AddQuad(m_arrIdx, iIdx, iPos1, iPos2, iPos1 + nStride,	// CW
				iPos2, iPos2 + nStride, iPos1 + nStride);
		}
		iStart += nStride;
	}
	// indices for outer bottom
	for (iSide = 0; iSide < nSides; iSide++) {
		int	iPos1 = iSide;
		int	iPos2 = iPos1 + 1;
		AddTri(m_arrIdx, iIdx, iVert, iPos1, iPos2);	// CCW
	}
	// indices for inner bottom
	iStart = (nRings + iFirstInnerRing) * nStride;
	for (iSide = 0; iSide < nSides; iSide++) {
		int	iPos1 = iStart + iSide;
		int	iPos2 = iPos1 + 1;
		AddTri(m_arrIdx, iIdx, iVert + 1, iPos2, iPos1);	// CW
	}
	// indices for lip
	iStart = (nRings - 1) * nStride;
	for (iSide = 0; iSide < nSides; iSide++) {
		int	iPos1 = iStart + iSide;
		int	iPos2 = iPos1 + 1;
		AddQuad(m_arrIdx, iIdx, iPos1, iPos1 + nWallStride, iPos2,	// CCW
			iPos2, iPos1 + nWallStride, iPos2 + nWallStride);
	}
#if CALC_NORMALS	// if calculating normals 
//	CBenchmark b;
	if (nRings != m_nAdjRings || nSides != m_nAdjSides	// if adjacency is stale
	|| nFaces != m_arrFaceNormal.GetSize()) {	// detects wall thickness changes
		m_nAdjRings = nRings;	// update cached values
		m_nAdjSides = nSides;
		m_arrAdjaceny.SetSize(nVerts);	// resize adjacency array
		m_arrFaceNormal.SetSize(nFaces);	// resize face normal array
		for (iVert = 0; iVert < nVerts; iVert++)	// for each vertex
			m_arrAdjaceny[iVert].m_nFaces = 0;	// reset adjacent face counts
		iIdx = 0;
		for (int iFace = 0; iFace < nFaces; iFace++) {	// for each face
			int	iA = m_arrIdx[iIdx + 0];	// get face's vertex indices
			int	iB = m_arrIdx[iIdx + 1];
			int	iC = m_arrIdx[iIdx + 2];
			iIdx += 3;
			m_arrAdjaceny[iA].Add(iFace);	// add face to adjaceny lists
			m_arrAdjaceny[iB].Add(iFace);
			m_arrAdjaceny[iC].Add(iFace);
		}
		int	iVertInner = 0;	// fix duplicate vertex at end of each ring
		for (iRing = 0; iRing < nRings; iRing++) {	// for each ring
			m_arrAdjaceny[iVertInner].Weld(m_arrAdjaceny[iVertInner + nSides]);
			int	iVertOuter = iVertInner + nWallStride;
			m_arrAdjaceny[iVertOuter].Weld(m_arrAdjaceny[iVertOuter + nSides]);
			iVertInner += nStride;
		}
	}
	iIdx = 0;
	for (int iFace = 0; iFace < nFaces; iFace++) {	// for each face
		int	iA = m_arrIdx[iIdx + 0];	// get face's vertex indices
		int	iB = m_arrIdx[iIdx + 1];
		int	iC = m_arrIdx[iIdx + 2];
		iIdx += 3;
		D3DXVECTOR3 vAB, vBC;
		D3DXVec3Subtract(&vAB, &m_arrVert[iA].pt, &m_arrVert[iB].pt);
		D3DXVec3Subtract(&vBC, &m_arrVert[iB].pt, &m_arrVert[iC].pt);
		D3DXVECTOR3& n = m_arrFaceNormal[iFace];
		D3DXVec3Cross(&n, &vAB, &vBC);	// compute face normal
		D3DXVec3Normalize(&n, &n);	// required to match D3DXComputeNormals; softens sharp edges
	}
	for (iVert = 0; iVert < nWallVerts; iVert++) {	// for each wall vertex
		const CAdjacency&	adj = m_arrAdjaceny[iVert];
		if (adj.m_nFaces > 0) {
			D3DXVECTOR3	sum(m_arrFaceNormal[adj.m_arrFaceIdx[0]]);
			for (int iAdjFace = 1; iAdjFace < adj.m_nFaces; iAdjFace++)
				sum += m_arrFaceNormal[adj.m_arrFaceIdx[iAdjFace]];
			D3DXVec3Normalize(&m_arrVert[iVert].n, &sum);
		}
	}
	m_arrVert[nVerts - 1].n = D3DXVECTOR3(0, 1, 0);
	m_arrVert[nVerts - 2].n = D3DXVECTOR3(0, -1, 0);
//	printf("%f\n", b.Elapsed());
#endif	// CALC_NORMALS
	if (m_nStyle & ST_BOUNDS)	// if showing bounds
		ComputeBounds(m_vBounds[0], m_vBounds[1]);
}

void CPotGraphics::GetTexture(double fRing, double fSide, D3DXVECTOR2& t) const
{
	switch (m_iColorPattern) {
	case COLORPAT_STRIPES:
		t.x = float((fSide + sin((fRing * m_fStripeFrequency + m_fStripePhase) * M_PI * 2) * m_fStripeAmplitude) 
			* m_fColorCycles + m_fOffsetU);
		t.y = float(fRing * m_fCyclesV + m_fOffsetV);
		break;
	case COLORPAT_RINGS:
		t.x = float((fRing + cos((fSide * m_fStripeFrequency + m_fStripePhase) * M_PI * 2) * m_fStripeAmplitude) 
			* m_fColorCycles + m_fOffsetU);
		t.y = float(fRing * m_fCyclesV + m_fOffsetV);
		break;
	case COLORPAT_PETALS:
//				u = (fRing + cos(fSide * M_PI * 2 * 6) / 2 + sin(fRing * M_PI * 2 * 3) / 16) * 2;
//				double	u = (fRing + cos(fSide * M_PI * 2 * 6) / 2 //* cos(theta / 3) 
//					+ sin(fRing * M_PI * 2 * (3 /*+ cos(theta / 5)) * 2*/)) / 16) * 2 - theta;
		{
			double	b = 0.5; // amplitude of radial term
			t.x = float((fRing + cos(fSide * M_PI * 2 * m_fPetals) * b 
				+ sin((fRing * m_fStripeFrequency + m_fStripePhase) * M_PI * 2) * m_fStripeAmplitude) 
				* m_fColorCycles + m_fOffsetU);
			t.y = float(fRing * m_fCyclesV + m_fOffsetV);
		}
		break;
	case COLORPAT_POLAR:
		{
			double	a = (fSide * m_fStripeFrequency + m_fStripePhase) * M_PI * 2;
			// minimize discontinuity by wrapping in pairs and flipping second instance
			double	r = Wrap(fRing + m_fOffsetV, 2);
			if (r > 1)	// if second instance
				r = 2 - r;	// flip and normalize radius
			r = r * 0.5 * m_fStripeAmplitude;
			t.x = float(sin(a) * r + 0.5);
			t.y = float(cos(a) * r + 0.5);
		}
		break;
	}
}

void CPotGraphics::UpdateAnimation()
{
	int	nMods = m_arrModIdx.GetSize();
	if (nMods) {	// if at least one modulation
		for (int iMod = 0; iMod < nMods; iMod++) {	// for each modulation
			int	iProp = m_arrModIdx[iMod];
			m_Mod[iProp].m_fPhase += m_Mod[iProp].m_fPhaseSpeed / m_fFrameRate;
		}
		if (m_bModulatingMesh)	// if at least one mesh modulation
			MakeMesh();
		else	// texture modulations only
			UpdateTexture();
	}
}

void CPotGraphics::GetAnimationState(CPotProperties& Props) const
{
	int	nMods = m_arrModIdx.GetSize();
	for (int iMod = 0; iMod < nMods; iMod++) {	// for each modulation
		int	iProp = m_arrModIdx[iMod];
		Props.m_Mod[iProp].m_fPhase = m_Mod[iProp].m_fPhase;
	}
}

bool CPotGraphics::UpdateTexture()
{
//	CBenchmark	b;
	int	nMods = m_arrModIdx.GetSize();
	if (nMods)
		SaveModulatedProperties();
	CVertex	*pVert;
	CHECK(m_pMesh->LockVertexBuffer(0, (void **)&pVert));
	int	iVert = 0;
	int	nStride = m_nSides + 1;
	for (int iWall = 0; iWall < WALLS; iWall++) {	// for each wall
		for (int iRing = 0; iRing < m_nRings; iRing++) {
			double	fRing = double(iRing) / (m_nRings - 1);	// normalized ring
			if (nMods)
				ApplyModulations(fRing);
			for (int iSide = 0; iSide < nStride; iSide++) {	// for each side
				double	fSide = double(iSide) / m_nSides;	// normalized side
				CVertex&	vert = pVert[iVert];
				GetTexture(fRing, fSide, vert.t);
				m_arrVert[iVert].t = vert.t;
				iVert++;
			}
		}
	}
	CHECK(m_pMesh->UnlockVertexBuffer());
	if (nMods)
		RestoreModulatedProperties();
//	printf("T=%f\n", b.Elapsed());
	return true;
}

bool CPotGraphics::MakeTexture()
{
//	printf("CPotGraphics::MakeTexture\n");
	if (!m_sTexturePath.IsEmpty()) {
		if (!PathFileExists(m_sTexturePath)) {
			AfxMessageBox(LDS(IDS_ERR_TEXTURE_FILE_NOT_FOUND) + '\n' + m_sTexturePath);
			return false;
		}
		m_pTexture = NULL;
		CHECK(D3DXCreateTextureFromFile(m_pDevice, m_sTexturePath, &m_pTexture));
	} else {
		int	iPaletteType = m_iPaletteType;
		int	nColors = m_arrPalette.GetSize();
		if (iPaletteType == PALTYPE_SPLIT) {
			if (nColors >= 2)	// if enough colors
				nColors /= 2;	// split palette into two rows
			else	// palette too small
				iPaletteType = PALTYPE_LINEAR;	// avoid divide by zero
		}
		int	nWidth = nColors * m_nColorSharpness;
		int	nHeight = (iPaletteType > PALTYPE_LINEAR) + 1;
		if (!m_dibTexture.Create(nWidth, nHeight, 24)) {
			AfxMessageBox(IDS_ERR_CANT_CREATE_TEXTURE_BITMAP);
			return false;
		}
		m_pTexture = NULL;
		CHECK(D3DXCreateTexture(m_pDevice, nWidth, nHeight, 0, 0, D3DFMT_A8B8G8R8, D3DPOOL_MANAGED, &m_pTexture));
		D3DLOCKED_RECT	rect;
		CHECK(m_pTexture->LockRect(0, &rect, 0, 0));
		DWORD	*pBits = (DWORD *)rect.pBits;
		for (int y = 0; y < nHeight; y++) {
			for (int x = 0; x < nWidth; x++) {
				int	iColor = x / m_nColorSharpness % nColors;
				COLORREF	c;
				switch (iPaletteType) {
				case PALTYPE_SPLIT:
					c = m_arrPalette[y * nColors + iColor];
					break;
				default:
					if (y & 1)
						c = 0xffffff - m_arrPalette[nColors - 1 - iColor];
					else
						c = m_arrPalette[iColor];
				}
				*pBits++ = D3DCOLOR_RGBA(GetRValue(c), GetGValue(c), GetBValue(c), 0);
				m_dibTexture.SetPixel(x, y, c);
			}
		}
		CHECK(m_pTexture->UnlockRect(0));
//		printf("w=%d h=%d, levels=%d\n", sz.cx, sz.cy, m_pTexture->GetLevelCount());
	}
	CHECK(m_pDevice->SetTexture(0, (m_nStyle & ST_TEXTURE) != 0 ? m_pTexture : NULL));
	return true;
}

__forceinline void CPotGraphics::AddTri(CDWordArrayEx& arrIdx, int& iIdx, int i0, int i1, int i2)
{
	DWORD	*pIdx = &arrIdx[iIdx];
	pIdx[0] = i0;
	pIdx[1] = i1;
	pIdx[2] = i2;
	iIdx += 3;
}

__forceinline void CPotGraphics::AddQuad(CDWordArrayEx& arrIdx, int& iIdx, int iA0, int iA1, int iA2, int iB0, int iB1, int iB2)
{
	DWORD	*pIdx = &arrIdx[iIdx];
	pIdx[0] = iA0;
	pIdx[1] = iA1;
	pIdx[2] = iA2;
	pIdx[3] = iB0;
	pIdx[4] = iB1;
	pIdx[5] = iB2;
	iIdx += 6;
}

__forceinline double CPotGraphics::Wrap(double x, double y)
{
	x = fmod(x, y);
	if (x < 0)
		x += y;
	return x;
}

__forceinline void CPotGraphics::CAdjacency::Add(int iFace)
{
	if (m_nFaces < _countof(m_arrFaceIdx))
		m_arrFaceIdx[m_nFaces++] = iFace;
}

void CPotGraphics::CAdjacency::Weld(CAdjacency& adj)
{
	int	nOtherFaces = adj.m_nFaces;
	int	iFace;
	for (iFace = 0; iFace < m_nFaces; iFace++)
		adj.Add(m_arrFaceIdx[iFace]);
	for (iFace = 0; iFace < nOtherFaces; iFace++)
		Add(adj.m_arrFaceIdx[iFace]);
}

__forceinline void CPotGraphics::ApplyMotif(int iMotif, double& r)
{
	switch (iMotif) {
	case MOTIF_REEDS:
		r = fabs(r);
		break;
	case MOTIF_FLUTES:
		r = -fabs(r);
		break;
	case MOTIF_PARTED_REEDS:
		r = max(r, 0);
		break;
	case MOTIF_PARTED_FLUTES:
		r = min(r, 0);
		break;
	}
}

__forceinline double CPotGraphics::GetWave(int iWaveform, double fPhase)
{
	switch (iWaveform) {
	case CModulationProps::WAVE_SINE:
		return sin(fPhase * M_PI * 2);
	case CModulationProps::WAVE_TRIANGLE:
		{
			double	r = Wrap(fPhase + 0.25, 1) * 4;
			return r < 2 ? r - 1 : 3 - r;
		}
	case CModulationProps::WAVE_RAMP_UP:
		return Wrap(fPhase, 1) * 2 - 1;
	case CModulationProps::WAVE_RAMP_DOWN:
		return 1 - Wrap(fPhase, 1) * 2;
	case CModulationProps::WAVE_SQUARE:
		return Wrap(fPhase, 1) < 0.5 ? 1 : -1;
	}
	return 0;
}

void CPotGraphics::ApplyModulations(double fRing)
{
	const double	fEpsilon = 1e-9;
	double	fTheta = min(fRing, 1.0 - fEpsilon);
	int	nMods = m_arrModIdx.GetSize();
	for (int iMod = 0; iMod < nMods; iMod++) {	// for each modulation
		int	iProp = m_arrModIdx[iMod];
		const PROPERTY_INFO&	info = m_Info[iProp];
		const CModulationProps&	mod = m_Mod[iProp];
		double	r = GetWave(mod.m_iWaveform, fTheta * mod.m_fFrequency + mod.m_fPhase);
		ApplyMotif(mod.m_iMotif, r);
		switch (mod.m_iRange) {
		case CModulationProps::RANGE_UNIPOLAR:
			r = (r + 1) / 2;	// convert from bipolar to unipolar
			if (mod.m_fPower > 0) {	// if exponential
				double	fScale = mod.m_fPower - 1;
				if (fScale)	// avoid divide by zero
					r = (pow(mod.m_fPower, r) - 1) / fScale;
			}
			break;
		default:	// bipolar
			if (mod.m_fPower > 0) {	// if exponential
				double	fScale = mod.m_fPower - 1;
				if (fScale)	// avoid divide by zero
					r = (pow(mod.m_fPower, (r + 1) / 2) - 1) / fScale * 2 - 1;
			}
		}
		r = (r + mod.m_fBias) * mod.m_fAmplitude;
		if (info.pType == &typeid(double)) {	// if property type is double
			double	*pSource = static_cast<double *>(m_arrSrcProps.GetPropertyAddress(iProp));
			double	*pTarget = static_cast<double *>(GetPropertyAddress(iProp));
			switch (mod.m_iOperation) {
			case CModulationProps::OPER_ADD:
				*pTarget = *pSource + r;
				break;
			case CModulationProps::OPER_SUBTRACT:
				*pTarget = *pSource - r;
				break;
			case CModulationProps::OPER_MULTIPLY:
				*pTarget = *pSource * r;
				break;
			case CModulationProps::OPER_DIVIDE:
				if (r)	// avoid divide by zero
					*pTarget = *pSource / r;
				break;
			}
		} else if (info.pType == &typeid(int)) {	// else if property type is int
			int	*pSource = static_cast<int *>(m_arrSrcProps.GetPropertyAddress(iProp));
			int	*pTarget = static_cast<int *>(GetPropertyAddress(iProp));
			switch (mod.m_iOperation) {
			case CModulationProps::OPER_ADD:
				*pTarget = round(*pSource + r);
				break;
			case CModulationProps::OPER_SUBTRACT:
				*pTarget = round(*pSource - r);
				break;
			case CModulationProps::OPER_MULTIPLY:
				*pTarget = round(*pSource * r);
				break;
			case CModulationProps::OPER_DIVIDE:
				if (r)	// avoid divide by zero
					*pTarget = round(*pSource / r);
				break;
			}
		}
	}
}

void CPotGraphics::SaveModulatedProperties()
{
	int	nMods = m_arrModIdx.GetSize();
	for (int iMod = 0; iMod < nMods; iMod++) {	// for each modulation
		int	iProp = m_arrModIdx[iMod];
		memcpy(m_arrSrcProps.GetPropertyAddress(iProp), GetPropertyAddress(iProp), m_Info[iProp].nLen);
	}
}

void CPotGraphics::RestoreModulatedProperties()
{
	int	nMods = m_arrModIdx.GetSize();
	for (int iMod = 0; iMod < nMods; iMod++) {	// for each modulation
		int	iProp = m_arrModIdx[iMod];
		memcpy(GetPropertyAddress(iProp), m_arrSrcProps.GetPropertyAddress(iProp), m_Info[iProp].nLen);
	}
}

void CPotGraphics::PlotProperty(int iProp, CArrayEx<DPoint, DPoint&>& arrPoint, CRange<double> *pRange)
{
	if (IsModulated(iProp)) {	// if desired property is modulated
		int	nMods = m_arrModIdx.GetSize();
		ASSERT(nMods > 0);	// else logic error
		int	iFirstMod = m_arrModIdx[0];	// save first modulation
		m_arrModIdx.SetSize(1);	// only one modulation
		m_arrModIdx[0] = iProp;	// set modulation index array to desired property
		int	nRings = m_nRings;
		arrPoint.SetSize(nRings);
		const type_info *pType = m_Info[iProp].pType;
		int	nPropLen = m_Info[iProp].nLen;
		LPVOID	pSrcProp = m_arrSrcProps.GetPropertyAddress(iProp);
		LPVOID	pDstProp = GetPropertyAddress(iProp);
		memcpy(pSrcProp, pDstProp, nPropLen);	// save target property
		for (int iRing = 0; iRing < nRings; iRing++) {	// for each ring
			double	fRing = double(iRing) / (nRings - 1);	// normalized ring
			ApplyModulations(fRing);
			arrPoint[iRing].x = ConvertPropertyToDouble(pDstProp, pType);
			arrPoint[iRing].y = fRing;
		}
		if (pRange != NULL) {	// if caller requested range
			CModulationProps&	mod = m_Mod[iProp];
			int	iWaveform = mod.m_iWaveform;	// save modulation properties
			double	fPhase = mod.m_fPhase;
			double	fFrequency = mod.m_fFrequency;
			mod.m_iWaveform = CModulationProps::WAVE_SQUARE;
			mod.m_fFrequency = 1;	// normal frequency range
			mod.m_fPhase = 0;	// no phase offset
			CDblRange	range;
			ApplyModulations(0);	// sample at negative rail
			range.Start = ConvertPropertyToDouble(pDstProp, pType);
			ApplyModulations(0.5);	// sample at positive rail
			range.End = ConvertPropertyToDouble(pDstProp, pType);
			range.Normalize();	// normalize range
			mod.m_iWaveform = CModulationProps::WAVE_RAMP_UP;
			ApplyModulations(0.5);	// sample at origin too; handles motifs
			double	fOrg = ConvertPropertyToDouble(pDstProp, pType);
			if (fOrg < range.Start)	// if origin sample below range
				range.Start = fOrg;	// adjust range
			else if (fOrg > range.End)	// if origin sample above range
				range.End = fOrg;	// adjust range
			double	fMargin, fMarginFrac = 0.05;
			if (range.IsEmpty())
				fMargin = 1;
			else
				fMargin = range.Length() * fMarginFrac;
			range.Start -= fMargin;
			range.End += fMargin;
			*pRange = range;	// pass range back to caller
			mod.m_iWaveform = iWaveform;	// restore modulation properties
			mod.m_fPhase = fPhase;
			mod.m_fFrequency = fFrequency;
		}
		memcpy(pDstProp, pSrcProp, nPropLen);	// restore target property
		m_arrModIdx.SetSize(nMods);	// restore modulation index array's size
		m_arrModIdx[0] = iFirstMod;	// restore first modulation
	} else {	// desired property not modulated; trivial case
		const type_info *pType = m_Info[iProp].pType;
		LPVOID	pDstProp = GetPropertyAddress(iProp);
		double	fVal = ConvertPropertyToDouble(pDstProp, pType);
		arrPoint.SetSize(2);
		arrPoint[0] = DPoint(fVal, 0);
		arrPoint[1] = DPoint(fVal, 1);
		if (pRange != NULL) {	// if caller requested range
			CDblRange	range(fVal - 1, fVal + 1);
			*pRange = range;	// pass range back to caller
		}
	}
}

__forceinline double CPotGraphics::ConvertPropertyToDouble(LPCVOID pSrc, const type_info *pType)
{
	if (pType == &typeid(double))	// if property type is double (try most likely case first)
		return *static_cast<const double *>(pSrc);
	if (pType == &typeid(int))	// if property type is int
		return *static_cast<const int *>(pSrc);
	return 0;	// unsupported type
}

COLORREF CPotGraphics::Interpolate(const CDibEx& dib, double x, double y)
{
	CSize	sz(dib.GetSize());
	x = Wrap(x - 0.5 / sz.cx, 1);	// offset by half a pixel
	int	ix = trunc(x * sz.cx);
	int	ix1 = ix < sz.cx ? ix : 0;
	int	ix2 = ix + 1 < sz.cx ? ix + 1 : 0;
	y = Wrap(y - 0.5 / sz.cy, 1);	// offset by half a pixel
	int	iy = trunc(y * sz.cy);
	int	iy1 = iy < sz.cy ? iy : 0;
	int	iy2 = iy + 1 < sz.cy ? iy + 1 : 0;
	COLORREF	c11 = dib.GetPixel(ix1, iy1);
	COLORREF	c21 = dib.GetPixel(ix2, iy1);
	COLORREF	c12 = dib.GetPixel(ix1, iy2);
	COLORREF	c22 = dib.GetPixel(ix2, iy2);
	double	sx2 = (x - double(ix) / sz.cx) * sz.cx / 255.0;
	double	sx1 = 1 / 255.0 - sx2;
	double	sy2 = (y - double(iy) / sz.cy) * sz.cy;
	double	sy1 = 1 - sy2;
	double	r = (GetRValue(c11) * sx1 + GetRValue(c21) * sx2) * sy1 +
				(GetRValue(c12) * sx1 + GetRValue(c22) * sx2) * sy2;
	double	g = (GetGValue(c11) * sx1 + GetGValue(c21) * sx2) * sy1 +
				(GetGValue(c12) * sx1 + GetGValue(c22) * sx2) * sy2;
	double	b = (GetBValue(c11) * sx1 + GetBValue(c21) * sx2) * sy1 +
				(GetBValue(c12) * sx1 + GetBValue(c22) * sx2) * sy2;
	return RGB(round(r * 255), round(g * 255), round(b * 255));  	
}

#define PLY_RGBA(r,g,b,a) ((DWORD)((((a)&0xff)<<24)|(((b)&0xff)<<16)|(((g)&0xff)<<8)|((r)&0xff)))

bool CPotGraphics::ExportPLY(LPCTSTR szPath, bool bVertexColor)
{
	if (bVertexColor) {	// if exporting vertex color
		static const LPCSTR szHdr =
			"ply\n"
			"format binary_little_endian 1.0\n"
			"comment PotterDraw generated\n"
			"element vertex %d\n"
			"property float x\n"
			"property float y\n"
			"property float z\n"
			"property float nx\n"
			"property float ny\n"
			"property float nz\n"
			"property uchar red\n"
			"property uchar green\n"
			"property uchar blue\n"
			"property uchar alpha\n"
			"element face %d\n"
			"property list uchar int vertex_indices\n"
			"end_header\n";
		int	nVerts = m_arrVert.GetSize();
		int	nFaces = m_nFaces;
		CStringA	sHdr;
		sHdr.Format(szHdr, nVerts, nFaces);
		CFile	fOut(szPath, CFile::modeCreate | CFile::modeWrite);
		fOut.Write(sHdr, sHdr.GetLength());
		for (int iVert = 0; iVert < nVerts; iVert++) {	// for each vertex
			const CVertex&	vert = m_arrVert[iVert];
			fOut.Write(&vert, sizeof(C3DVertexNormal));	// omit texture coords
			COLORREF	c = Interpolate(m_dibTexture, vert.t.x, vert.t.y);
			DWORD	pc = PLY_RGBA(GetRValue(c), GetGValue(c), GetBValue(c), 255);
			fOut.Write(&pc, sizeof(pc));
		}
		for (int iFace = 0; iFace < nFaces; iFace++) {	// for each face
			char	n = 3;
			fOut.Write(&n, 1);
			for (int iPt = 0; iPt < 3; iPt++) {	// for each point
				int	iVert = m_arrIdx[iFace * 3 + iPt];
				fOut.Write(&iVert, sizeof(DWORD));
			}
		}
	} else {	// exporting texture coords
		static const LPCSTR szHdr =
			"ply\n"
			"format binary_little_endian 1.0\n"
			"comment PotterDraw generated\n"
			"comment TextureFile %s\n"
			"element vertex %d\n"
			"property float x\n"
			"property float y\n"
			"property float z\n"
			"property float nx\n"
			"property float ny\n"
			"property float nz\n"
			"element face %d\n"
			"property list uchar int vertex_indices\n"
			"property list uchar float texcoord\n"
			"end_header\n";
		int	nVerts = m_arrVert.GetSize();
		int	nFaces = m_nFaces;
		CString	sTexturePath;
		if (!ExportTexture(szPath, sTexturePath))
			return false;
		CStringA	sTextureFileName(PathFindFileNameA(CStringA(sTexturePath)));
		CStringA	sHdr;
		sHdr.Format(szHdr, sTextureFileName, nVerts, nFaces);
		CFile	fOut(szPath, CFile::modeCreate | CFile::modeWrite);
		fOut.Write(sHdr, sHdr.GetLength());
		for (int iVert = 0; iVert < nVerts; iVert++) {	// for each vertex
			fOut.Write(&m_arrVert[iVert], sizeof(C3DVertexNormal));	// omit texture coords
		}
		for (int iFace = 0; iFace < nFaces; iFace++) {	// for each face
			int	iPt;
			char	n = 3;
			fOut.Write(&n, 1);
			for (iPt = 0; iPt < 3; iPt++) {	// for each point
				int	iVert = m_arrIdx[iFace * 3 + iPt];
				fOut.Write(&iVert, sizeof(DWORD));
			}
			n = 6;
			fOut.Write(&n, 1);
			for (iPt = 0; iPt < 3; iPt++) {	// for each point
				int	iVert = m_arrIdx[iFace * 3 + iPt];
				fOut.Write(&m_arrVert[iVert].t, sizeof(D3DXVECTOR2));
			}
		}
	}
	return true;
}

bool CPotGraphics::ExportTexture(LPCTSTR szPath, CString& sNewPath)
{
	CPathStr	sTexturePath(szPath);
	CString	sTextureFileName(PathFindFileName(szPath));
	sTextureFileName.Replace(' ', '_');	// replace any spaces in file name with underscores
	sTexturePath.RemoveFileSpec();
	sTexturePath.Append(sTextureFileName);
	if (m_sTexturePath.IsEmpty()) {	// if we don't have a texture file
		sTexturePath.RenameExtension(_T(".bmp"));
		if (!m_dibTexture.Write(sTexturePath)) {	// use bitmap as texture
			AfxMessageBox(IDS_ERR_CANT_WRITE_TEXTURE_BITMAP);
			return false;
		}
	} else {	// we have a texture file
		CString	sExt(PathFindExtension(m_sTexturePath));
		sTexturePath.RenameExtension(sExt);
		if (sTexturePath != m_sTexturePath) {	// if texture paths differ
			if (!CopyFile(m_sTexturePath, sTexturePath, FALSE)) {	// copy texture file
				AfxMessageBox(IDS_ERR_CANT_COPY_TEXTURE_FILE);
				return false;
			}
		}
	}
	sNewPath = sTexturePath;
	return true;
}

bool CPotGraphics::ExportSTL(LPCTSTR szPath)
{
	#pragma pack(push, 2)	// necessary because attribute is 16-bit
	struct FACE {
		D3DXVECTOR3	n;		// normal
		D3DXVECTOR3	pt[3];	// triangle vertices
		short	attr;		// attribute (unused)
	};
	#pragma pack(pop)
	CFile	fOut(szPath, CFile::modeCreate | CFile::modeWrite);
	BYTE	hdr[80];
	ZeroMemory(hdr, sizeof(hdr));
	fOut.Write(hdr, sizeof(hdr));
	int	nFaces = m_nFaces;
	fOut.Write(&nFaces, sizeof(m_nFaces));
	FACE	face;
	face.attr = 0;
	for (int iFace = 0; iFace < nFaces; iFace++) {	// for each face
		for (int iPt = 0; iPt < 3; iPt++) {	// for each point
			int	iVert = m_arrIdx[iFace * 3 + iPt];
			face.pt[iPt] = m_arrVert[iVert].pt;
		}
		D3DXVECTOR3	a, b;
		D3DXVec3Subtract(&a, &face.pt[0], &face.pt[1]); 
		D3DXVec3Subtract(&b, &face.pt[1], &face.pt[2]); 
		D3DXVec3Cross(&face.n, &a, &b);
		fOut.Write(&face, sizeof(face));
	}
	return true;
}

bool CPotGraphics::ExportOBJ(LPCTSTR szPath, int nPrecision)
{
	CPathStr	sMaterialPath(szPath);
	CPathStr	sMaterialFileName(PathFindFileName(szPath));
	sMaterialFileName.RenameExtension(_T(".mtl"));	// use material file extension
	sMaterialFileName.Replace(' ', '_');	// replace any spaces in file name with underscores
	sMaterialPath.RemoveFileSpec();
	sMaterialPath.Append(sMaterialFileName);
	CString	sTexturePath;
	if (m_nStyle & ST_TEXTURE) {	// if showing texture
		if (!ExportTexture(szPath, sTexturePath))	// export texture
			return false;
	}
	CStdioFile	fOut(szPath, CFile::modeCreate | CFile::modeWrite);
	CStdioFile	fMtl(sMaterialPath, CFile::modeCreate | CFile::modeWrite);
	fOut.WriteString(_T("mtllib ") + sMaterialFileName + '\n');	// link material file
	int	nVerts = m_arrVert.GetSize();
	CString	s, t;
	CString	sFmt;
	int	np = nPrecision;
	sFmt.Format(_T("v %%.%df %%.%df %%.%df\nvn %%.%df %%.%df %%.%df\nvt %%.%df %%.%df\n"),
		np, np, np, np, np, np, np, np);	// make format for desired floating-point precision
	for (int iVert = 0; iVert < nVerts; iVert++) {	// for each vertex
		const C3DVertexNormalTexture&	v = m_arrVert[iVert];
		s.Format(sFmt, v.pt.x, v.pt.y, v.pt.z, v.n.x, v.n.y, v.n.z, v.t.x, v.t.y);
		fOut.WriteString(s);
	}
	fOut.WriteString(_T("usemtl material_0\n"));	// apply material definition to faces
	int	nFaces = m_nFaces;
	for (int iFace = 0; iFace < nFaces; iFace++) {	// for each face
		t = 'f';	// face element
		for (int iPt = 0; iPt < 3; iPt++) {	// for each point
			s.Format(_T("%d"), m_arrIdx[iFace * 3 + iPt] + 1);	// one-based indices
			t += ' ' + s + '/' + s + '/' + s;	// same index for vertex, normal and texture coords
		}
		fOut.WriteString(t + '\n');
	}
	fMtl.WriteString(_T("newmtl material_0\n"));	// define material
	CString	sTextureFileName(PathFindFileName(sTexturePath));
	fMtl.WriteString(_T("Ka 1.0 1.0 1.0\nKd 1.0 1.0 1.0\n"));	// ambient and diffuse lighting
	if (m_nStyle & ST_TEXTURE)	// if showing texture
		fMtl.WriteString(_T("map_Kd ") + sTextureFileName + '\n');	// diffuse texture filename
	return true;
}

bool CPotGraphics::ComputeBounds(D3DXVECTOR3& p1, D3DXVECTOR3& p2)
{
	CHECK(D3DXComputeBoundingBox((D3DXVECTOR3 *)m_arrVert.GetData(), m_arrVert.GetSize(), 
		sizeof(CVertex), &p1, &p2));
	return true;
}

bool CPotGraphics::CreateBoundingBox()
{
	D3DXVECTOR3	vBoundsSize = m_vBounds[1] - m_vBounds[0];
	CHECK(D3DXCreateBox(m_pDevice, vBoundsSize.x, vBoundsSize.y, vBoundsSize.z, &m_pBounds, NULL));
	return true;
}

bool CPotGraphics::DrawBoundingBox()
{
	D3DXVECTOR3	vBoundsSize = m_vBounds[1] - m_vBounds[0];
	D3DXVECTOR3	vBoundsOrg(m_vBounds[0] + vBoundsSize / 2);
	D3DXVec3TransformCoord(&vBoundsOrg, &vBoundsOrg, &m_matRotation);
	D3DXMATRIX	matWorld;
	CHECK(m_pDevice->GetTransform(D3DTS_WORLD, &matWorld));	// save world transform
	D3DXMATRIX	matBounds(matWorld), matPan;
	D3DXMatrixTranslation(&matPan, vBoundsOrg.x, vBoundsOrg.y, vBoundsOrg.z);
	D3DXMatrixMultiply(&matBounds, &matBounds, &matPan);
	CHECK(m_pDevice->SetTransform(D3DTS_WORLD, &matBounds));	// set bounding box world transform
	DWORD	nFillMode;
	CHECK(m_pDevice->GetRenderState(D3DRS_FILLMODE, &nFillMode));	// save fill mode
	CHECK(m_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));	// set wireframe mode
	D3DMATERIAL9	material;
	CHECK(m_pDevice->GetMaterial(&material));	// save material
	CHECK(m_pDevice->SetMaterial(&m_materialBounds));	// set bounds material
	IDirect3DBaseTexture9	*pTexture;
	CHECK(m_pDevice->GetTexture(0, &pTexture));	// save texture
	CHECK(m_pDevice->SetTexture(0, NULL));	// disable texture
	CHECK(m_pBounds->DrawSubset(0));	// draw bounding box
	CHECK(m_pDevice->SetTransform(D3DTS_WORLD, &matWorld));	// restore world transform
	CHECK(m_pDevice->SetRenderState(D3DRS_FILLMODE, nFillMode));	// restore fill mode
	CHECK(m_pDevice->SetMaterial(&material));	// restore material
	CHECK(m_pDevice->SetTexture(0, pTexture));	// restore texture
	return true;
}

void CPotGraphics::CalcSpline(const CSplineArray& arrSpline)
{
	arrSpline.SampleX(m_nRings, m_arrSpline, m_rSplineBounds);
}

bool CPotGraphics::IsSplinePositiveInX() const
{
	int	nRings = m_arrSpline.GetSize();
	for (int iRing = 0; iRing < nRings; iRing++) {
		if (m_arrSpline[iRing] < 0)
			return false;
	}
	return true;
}

