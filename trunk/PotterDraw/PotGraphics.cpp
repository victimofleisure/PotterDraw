// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda

		revision history:
		rev		date	comments
        00      12mar17	initial version
		01		24aug17	in CalcPotMesh, add scallop phase
		02		25aug17	copy outer wall's texture coords to inner wall
		03		05oct17	add scallop range, power, operation
		04		09oct17	in GetWave, add pulse and rounded pulse waves
		05		11oct17	in CalcPotMesh, make origin double instead of float
		06		12oct17	in MakeTexture, check locked rectangle's pitch
		07		17oct17	in GetWave, add sine cubed and flame waves
		08		19oct17	add scallop waveform, pulse width, and slew
		09		20oct17	in GetWave, add triangular pulse wave
		10		01nov17	add polygon properties
		11		06nov17	add get/set pot material
		12		10nov17	add ramp pulse wave
		13		10nov17	add power types
		14		14nov17	flatten outer radius array and make it a member
		15		15nov17	add radius color pattern
		16		15nov17	optimize modulo one wrapping
		17		16nov17	in UpdateTextureCoords, avoid reading from vertex buffer
		18		17nov17	in ExportPLY, handle texture file
		19		23nov17	add modulation type flags
		20		06dec17	add hit test and face and vertex accessors
		21		06dec17	add optional showing of normals and face selection
		22		07dec17	skip calculating unused inner wall vertices
		23		10dec17	add azimuth and incline color patterns
		24		12dec17	add operation, power and power type to ripple and bend
		25		12dec17	in SetViewState, set standard view if applicable
		26		15dec17	add edges color pattern
		27		02jan18	add ruffle properties
		28		03jan18	add ring phase
		29		20feb18	add secondary modulation
		30		22feb18	in CalcPotMesh, pitch mesh upright by swapping y and z
		31		22feb18	in DrawBoundingBox, don't need to set world transform
		32		27feb18	add semicircle and circular pulse waves
		33		27feb18	add invert motif

*/

#include "stdafx.h"
#include "resource.h"
#include "PotGraphics.h"
#include "Benchmark.h"
#include "PathStr.h"
#include "DPoint.h"
#include "DPoint3.h"

#define _USE_MATH_DEFINES	// for trig constants
#include <math.h>

#define CHECK(x) if (FAILED(m_hLastError = x)) { OnError(); return false; }

bool CPotGraphics::m_bShowingErrorMsg;

const D3DMATERIAL9 CPotGraphics::m_mtrlBounds = {{0}, {0}, {0}, {0, 1, 0}};

#define CALC_NORMALS 1	// non-zero to calculate normals; zero to use D3DXComputeNormals (very slow)

CPotGraphics::CPotGraphics()
{
	m_nStyle |= ST_TEXTURE;
	m_nFaces = 0;
	m_fColorPos = 0;
	m_fColorDelta = 1 / m_fFrameRate;
	m_mtrlPot = m_mtrlPotDefault;
	m_nAdjRings = 0;
	m_nAdjSides = 0;
	m_nModState = 0;
	m_iCurPlotProp = -1;
	m_fMinOuterRadius = 0;
	m_fMaxOuterRadius = 0;
	m_fOuterRadiusScale = 0;
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
#if POT_GFX_SHOW_FACE_NORMALS
	if (m_vbFaceNormalLine.m_pVertBuf != NULL) {	// if drawing face normal lines
		DrawFaceNormalLines();
	}
#endif	// POT_GFX_SHOW_FACE_NORMALS
#if POT_GFX_SHOW_VERTEX_NORMALS
	if (m_vbVertexNormalLine.m_pVertBuf != NULL) {	// if drawing vertex normal lines
		DrawVertexNormalLines();
	}
#endif	// POT_GFX_SHOW_VERTEX_NORMALS
#if POT_GFX_SHOW_FACE_SELECTION
	if (m_vbSelectedFace.m_pVertBuf != NULL) {	// if drawing selected faces
		DrawSelectedFaces();
	}
#endif	// POT_GFX_SHOW_FACE_SELECTION
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
#if POT_GFX_SHOW_FACE_NORMALS
	if ((nStyle & ST_FACE_NORMALS) ^ (m_nStyle & ST_FACE_NORMALS)) {	// if face normals state changed
		if (nStyle & ST_FACE_NORMALS) {	// if showing face normals
			CreateFaceNormalLines();
		} else
			m_vbFaceNormalLine.m_pVertBuf = NULL;
	}
#endif	// POT_GFX_SHOW_FACE_NORMALS
#if POT_GFX_SHOW_VERTEX_NORMALS
	if ((nStyle & ST_VERTEX_NORMALS) ^ (m_nStyle & ST_VERTEX_NORMALS)) {	// if vertex normals state changed
		if (nStyle & ST_VERTEX_NORMALS) {	// if showing vertex normals
			CreateVertexNormalLines();
		} else
			m_vbVertexNormalLine.m_pVertBuf = NULL;
	}
#endif	// POT_GFX_SHOW_VERTEX_NORMALS
	if (!CD3DGraphics::SetStyle(nStyle))
		return false;
	CHECK(m_pDevice->SetTexture(0, (nStyle & ST_TEXTURE) != 0 ? m_pTexture : NULL));
	return true;
}

void CPotGraphics::SetProperties(const CPotProperties& Props)
{
	CPotProperties::operator=(Props);
	SetBkgndColor(CvtFromColorRef(m_clrBackground));
}

void CPotGraphics::GetViewState(CPotProperties& Props) const
{
	Props.m_nRenderStyle = m_nStyle;
	Props.m_vRotation = CD3DGraphics::m_vRotation;
	Props.m_vPan = CD3DGraphics::m_vPan;
	Props.m_fZoom = CD3DGraphics::m_fZoom;
	Props.m_vLightDir = m_light.Direction;
	Props.m_mtrlPot = m_mtrlPot;
}

void CPotGraphics::SetViewState(const CPotProperties& Props)
{
	m_nStyle = Props.m_nRenderStyle;
	CD3DGraphics::m_vRotation = Props.m_vRotation;
	CD3DGraphics::m_vPan = Props.m_vPan;
	CD3DGraphics::m_fZoom = Props.m_fZoom;
	m_light.Direction = Props.m_vLightDir;
	m_mtrlPot = Props.m_mtrlPot;
	int	iStdView = FindStandardView(Props.m_vRotation);
	if (iStdView >= 0)	// if rotation is standard view
		m_iStdView = iStdView;
}

bool CPotGraphics::SetPotMaterial(const D3DMATERIAL9& mtrlPot)
{
	m_mtrlPot = mtrlPot;
	CHECK(m_pDevice->SetMaterial(&mtrlPot));
	return true;
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
		switch (m_iColorPattern) {
		case COLORPAT_RADIUS:
		case COLORPAT_AZIMUTH:
		case COLORPAT_INCLINE:
		case COLORPAT_AZI_INC:
		case COLORPAT_EDGES:
			// color depends on mesh, so CalcPotMesh's GetTextureCoords calls are
			// useless at best; save time by making GetTextureCoords do nothing
			m_iColorPattern -= COLOR_PATTERNS;	// force color pattern out of range
			break;
		}
		CalcPotMesh();	// calculate pot mesh
		if (m_iColorPattern < 0)	// if color pattern was overridden above
			m_iColorPattern += COLOR_PATTERNS;	// restore original color pattern
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
	CHECK(m_pDevice->SetMaterial(&m_mtrlPot));
	if (!MakeTexture())
		return false;
	if (m_nStyle & ST_BOUNDS)
		CreateBoundingBox();
#if POT_GFX_SHOW_FACE_NORMALS
	if (m_nStyle & ST_FACE_NORMALS)
		CreateFaceNormalLines();
#endif	// POT_GFX_SHOW_FACE_NORMALS
#if POT_GFX_SHOW_VERTEX_NORMALS
	if (m_nStyle & ST_VERTEX_NORMALS)
		CreateVertexNormalLines();
#endif	// POT_GFX_SHOW_VERTEX_NORMALS
#if POT_GFX_SHOW_FACE_SELECTION
	if (m_arrSelectedFaceIdx.GetSize())
		CreateSelectedFaces();
#endif	// POT_GFX_SHOW_FACE_SELECTION
	switch (m_iColorPattern) {
	case COLORPAT_RADIUS:	// if radius color pattern
		{
			ComputeOuterRadiusRange(m_fMinOuterRadius, m_fMaxOuterRadius);
			double	fDeltaRad = m_fMaxOuterRadius - m_fMinOuterRadius;
			if (fDeltaRad > 1e-6)	// if radius range is reasonable 
				m_fOuterRadiusScale = 1 / fDeltaRad;
			else	// radius range is infinitesimal
				m_fOuterRadiusScale = 0;	// avoid excessive scale
			UpdateTextureCoords();
		}
		break;
	case COLORPAT_AZIMUTH:
	case COLORPAT_INCLINE:
	case COLORPAT_AZI_INC:
	case COLORPAT_EDGES:
		UpdateTextureCoords();
		break;
	}
//m_pMesh = NULL;
//CHECK(D3DXCreateCylinder(m_pDevice, 130, 100, 200, 50, 10, &m_pMesh, NULL));
	return true;
}

#define OUTER_RADIUS(ring, side) m_faOuterRadius[(ring) * nSides + (side)]

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
	m_arrVert.SetSize(nVerts);
	m_arrIdx.SetSize(nIdxs);
	int	iWall, iRing, iSide;
	int	iVert = 0;
	double	fDelta = M_PI * 2 / nSides;
	bool	bHasScallops = HasScallops();
	bool	bHasRipples = HasRipples();
	bool	bHasBends = HasBends();
	bool	bHasRuffles = HasRuffles();
	bool	bHasHelix = HasHelix();
	bool	bIsPolygon = IsPolygon();
	double	fRingBend = 0;
	double	fPolyRoundnessOffset = 0;
	double	fPolyRoundness = 0;
	DPoint	vOrigin(0, 0);
	m_faOuterRadius.SetSize(nRings * nSides);
	int	nMods = m_arrModIdx.GetSize();
	if (nMods)
		SaveModulatedProperties();
//	printf("generating vertices\n");
	// vertices for inner and outer walls
	double	fRingRad = 0;	// init to avoid warning
	for (iWall = 0; iWall < WALLS; iWall++) {	// for each wall
		int iFirstRing;
		if (iWall == WALL_OUTER) {	// if outer wall
			iFirstRing = 0;
		} else {	// inner wall
			int	nUnusedVerts = iFirstInnerRing * nStride;
			ZeroMemory(&m_arrVert[iVert], sizeof(CVertex) * nUnusedVerts);
			iVert += nUnusedVerts;
			iFirstRing = iFirstInnerRing;	// skip calculating unused vertices
		}
		for (int iRing = iFirstRing; iRing < nRings; iRing++) {	// for each ring
			double	fRing = double(iRing) / (nRings - 1);	// normalized ring
			if (nMods)
				ApplyModulations(fRing);
			if (iWall == WALL_OUTER) {	// if outer wall
				double	fRad = m_fRadius * m_arrSpline[iRing];
				if (bHasRipples) {		// if ripples enabled
					double	r = sin((fRing * m_fRipples + m_fRipplePhase) * M_PI * 2);
					ApplyMotif(m_iRippleMotif, r);
					ApplyPower(CModulationProps::RANGE_BIPOLAR, m_iRipplePowerType, m_fRipplePower, r);
					r *= m_fRippleDepth;	// apply amplitude
					if (m_iRippleOperation == OPER_ADD)	// if operation is add
						fRad += r;
					else	// operation is exponentiate
						fRad *= pow(2, r);
				}
				if (bHasBends && !bHasRuffles) {	// if bends enabled, but not ruffles
					fRingBend = cos((fRing * m_fBends + m_fBendPhase) * M_PI * 2);
					ApplyMotif(m_iBendMotif, fRingBend);
				}
				fRingRad = fRad;
			}
			double	fRingPhase = (m_fTwist * fRing + m_fRingPhase) * M_PI * 2;	// optimization
			if (bHasHelix) {	// if helix enabled
				double	fHelixTheta = fRing * m_fHelixFrequency * M_PI * 2;
				vOrigin.x = sin(fHelixTheta) * m_fHelixAmplitude;
				vOrigin.y = -cos(fHelixTheta) * m_fHelixAmplitude;	// negate for swapped y and z
			}
			if (bIsPolygon && m_fPolygonRoundness)	{	// if polygon roundness enabled
				double	fMin = 1 - cos(M_PI / m_fPolygonSides);
				double	b = min(fabs(m_fPolygonRoundness), 1) * fMin;
				double	fExp = fMin - b;
				if (fExp)	// if less than maximum roundness
					fPolyRoundness = log(fExp) / log(fMin);
				else	// maximum roundness; avoid log of zero
					fPolyRoundness = DBL_MAX;
				if (m_fPolygonRoundness > 0)
					fPolyRoundnessOffset = 0;
				else
					fPolyRoundnessOffset = b;
			}
			for (iSide = 0; iSide < nSides; iSide++) {	// for each side
				CVertex&	v = m_arrVert[iVert];
				double	fSide = double(iSide) / nSides;	// normalized side
				double	fRad;
				if (iWall == WALL_OUTER) {	// if outer wall
					fRad = fRingRad;
					if (bIsPolygon) {	// if polygon enabled
						double	n = m_fPolygonSides;
						double	r = cos(M_PI / n) / cos(Wrap1(fSide * n + m_fPolygonPhase) * M_PI * 2 / n - M_PI / n);
						if (m_fPolygonRoundness)
							r = 1 - pow(1 - r, fPolyRoundness) - fPolyRoundnessOffset;
						if (m_fPolygonBulge)
							r = pow(r, 1 - m_fPolygonBulge);
						fRad *= r;
					}
					if (bHasScallops) {	// if scallops enabled
						double	r;
						if (!m_iScallopWaveform)	// if sine, handle specially for performance
							r = cos((fSide * m_fScallops + m_fScallopPhase) * M_PI * 2);	// actually cosine
						else {	// waveform other than sine; offset phase by 90 degrees to eumlate cosine
							r = GetWave(m_iScallopWaveform + 1, fSide * m_fScallops + m_fScallopPhase + 0.25, 
								m_fScallopPulseWidth, m_fScallopSlew);	// for pulse waveforms
						}
						ApplyMotif(m_iScallopMotif, r);
						ApplyPower(m_iScallopRange, m_iScallopPowerType, m_fScallopPower, r);
						r *= m_fScallopDepth;	// apply amplitude
						switch (m_iScallopOperation) {
						case CModulationProps::OPER_ADD:
							fRad += r;
							break;
						case CModulationProps::OPER_SUBTRACT:
							fRad -= r;
							break;
						case CModulationProps::OPER_MULTIPLY:
							if (r >= 0)
								fRad *= r + 1;
							else
								fRad /= 1 - r;
							break;
						case CModulationProps::OPER_DIVIDE:
							if (r >= 0)
								fRad /= r + 1;
							else
								fRad *= 1 - r;
							break;
						case CModulationProps::OPER_EXPONENTIATE:
							fRad *= pow(2, r);
							break;
						}
					}
					if (bHasBends) {	// if bends enabled
						if (bHasRuffles) {	// if ruffles enabled, fRingBend can vary per side
							double	fPhase = fSide * m_fRuffles + m_fBendPolePhase + m_fRufflePhase;
							double	fRuffle = GetWave(m_iRuffleWaveform + 1, fPhase, m_fRufflePulseWidth, m_fRuffleSlew);
							ApplyMotif(m_iRuffleMotif, fRuffle);
							ApplyPower(CModulationProps::RANGE_BIPOLAR, m_iRufflePowerType, m_fRufflePower, fRuffle);
							fRuffle = fRing + fRuffle * m_fRuffleDepth;
							fRingBend = cos((fRuffle * m_fBends + m_fBendPhase) * M_PI * 2);
							ApplyMotif(m_iBendMotif, fRingBend);
						}
						double	r = fRingBend * cos((fSide * m_fBendPoles + m_fBendPolePhase) * M_PI * 2);
						ApplyMotif(m_iBendPoleMotif, r);
						ApplyPower(CModulationProps::RANGE_BIPOLAR, m_iBendPowerType, m_fBendPower, r);
						r *= m_fBendDepth;	// apply amplitude
						if (m_iBendOperation == OPER_ADD)	// if operation is add
							fRad += r;
						else	// operation is exponentiate
							fRad *= pow(2, r);
					}
					OUTER_RADIUS(iRing, iSide) = fRad;	// store outer radius for 2nd pass
					GetTextureCoords(fRing, fSide, v.t);	// get texture coords
				} else {	// inner wall
					double	fSlope;
					if (iRing && iRing < nRings - 1)
						fSlope = (OUTER_RADIUS(iRing + 1, iSide) - OUTER_RADIUS(iRing - 1, iSide)) / (fZStep * 2);
					else
						fSlope = 0;
					double	a = atan(fSlope);
					fRad = OUTER_RADIUS(iRing, iSide) - m_fWallThickness / cos(a);
					fRad = max(fRad, 0);	// avoid negative radius
					v.t = m_arrVert[iVert - nWallStride].t;	// copy outer wall's texture coords
				}
				double	fTheta = fDelta * iSide + fRingPhase;
				double	x = sin(fTheta) * m_fAspectRatio;
				double	y = cos(fTheta);
				double	z = (fRing - 0.5) * fHeight;
				v.pt = D3DXVECTOR3(float(x * fRad + vOrigin.x), float(z), float(vOrigin.y - y * fRad));
				iVert++;
			}
			CVertex&	v = m_arrVert[iVert];	// ring's final vertex
			v.pt = m_arrVert[iVert - nSides].pt;	// point is same as first vertex
			GetTextureCoords(fRing, 1, v.t);	// but texture differs
			iVert++;
		}
	}
	// vertices for outer and inner bottoms
	if (bHasHelix) {
		if (nMods)
			ApplyModulations(0);	// in case helix amplitude is modulated
		vOrigin.x = 0;
		vOrigin.y = -m_fHelixAmplitude;	// invert for swapped y and z
	}
	if (nMods)
		RestoreModulatedProperties();
	for (int iWall = 0; iWall < WALLS; iWall++) {
		CVertex&	v = m_arrVert[iVert + iWall];
		double	z = m_fWallThickness * iWall - fHeight / 2;
		v.pt = D3DXVECTOR3(float(vOrigin.x), float(z), float(vOrigin.y));
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

void CPotGraphics::GetTextureCoords(double fRing, double fSide, D3DXVECTOR2& t) const
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
	case COLORPAT_RADIUS:
		{
			int	nSides = m_nSides;
			int iRing = round(fRing * (m_nRings - 1));
			int iSide = round(fSide * nSides);
			if (iSide >= nSides)
				iSide = 0;	// wrap around
			t.x = float((OUTER_RADIUS(iRing, iSide) - m_fMinOuterRadius) * m_fOuterRadiusScale 
				* m_fColorCycles + m_fOffsetU);
			t.y = float(fRing * m_fCyclesV + m_fOffsetV);
		}
		break;
	case COLORPAT_AZIMUTH:
		{
			int	iVert = GetVertexIdx(fRing, fSide);
			D3DXVECTOR3	vNormal(m_arrVert[iVert].n);
			t.x = float(GetAzimuth(vNormal, fSide) * m_fColorCycles + m_fOffsetU);
			t.y = float(fRing * m_fCyclesV + m_fOffsetV);
		}
		break;
	case COLORPAT_INCLINE:
		{
			int	iVert = GetVertexIdx(fRing, fSide);
			D3DXVECTOR3	vNormal(m_arrVert[iVert].n);
			t.x = float(GetIncline(vNormal) * m_fColorCycles + m_fOffsetU);
			t.y = float(fRing * m_fCyclesV + m_fOffsetV);
		}
		break;
	case COLORPAT_AZI_INC:
		{
			int	iVert = GetVertexIdx(fRing, fSide);
			D3DXVECTOR3	vNormal(m_arrVert[iVert].n);
			t.x = float(GetAzimuth(vNormal, fSide) * m_fColorCycles + m_fOffsetU);
			t.y = float(GetIncline(vNormal) * m_fCyclesV + m_fOffsetV);
		}
		break;
	case COLORPAT_EDGES:
		{
			int	iVert = GetVertexIdx(fRing, fSide);
			if (iVert < m_arrAdjaceny.GetSize()) {	// if vertex index in range
				DPoint3	vMean(m_arrVert[iVert].n);	// vertex normal is mean of adjacent face normals
				DPoint3	vStdDev(0, 0, 0);
				const CAdjacency&	adj = m_arrAdjaceny[iVert];
				for (int iAdj = 0; iAdj < adj.m_nFaces; iAdj++) {	// for each adjacent face
					int	iFace = adj.m_arrFaceIdx[iAdj];
					DPoint3	vFaceNormal(m_arrFaceNormal[iFace]);
					vStdDev += (vFaceNormal - vMean).Square();	// add squared difference to sum
				}
				vStdDev = (vStdDev / adj.m_nFaces).SquareRoot();	// 3D standard deviation
				double	m = vStdDev.Length();	// geometric mean of 3D standard deviation
				double	fBase = pow(10, -m_fEdgeGain);	// smaller bases are more sensitive
				double	fScale = fBase - 1;
				if (fScale)	// avoid divide by zero
					m = (pow(fBase, m) - 1) / fScale;
				t.x = float(m * m_fColorCycles + m_fOffsetU);
			} else	// vertex index out of range
				t.x = 0;
			t.y = float(fRing * m_fCyclesV + m_fOffsetV);
		}
		break;
	}
}

__forceinline int CPotGraphics::GetVertexIdx(double fRing, double fSide) const
{
	int	nSides = m_nSides;
	int iRing = round(fRing * (m_nRings - 1));
	int iSide = round(fSide * nSides);
	return iRing * (nSides + 1) + iSide;
}

__forceinline double CPotGraphics::GetAzimuth(const D3DXVECTOR3& vNormal, double fAzimuth)
{
	DPoint	vNormal2(vNormal.x, vNormal.z);
	vNormal2.Normalize();	// convert 2D normal on ring plane to unit vector
	double	fTheta = fAzimuth * M_PI * 2;	// convert azimuth to radians
	DPoint	vRef(sin(fTheta), -cos(fTheta));	// minus to match 3D normal's handedness
	double	fDP = vRef.Dot(vNormal2);	// dot product
	return acos(CLAMP(fDP, -1, 1)) / M_PI_2;	// deviation from circular, as normalized angle
}

__forceinline double CPotGraphics::GetIncline(const D3DXVECTOR3& vNormal)
{
	return asin(fabs(vNormal.y)) / M_PI_2;	// deviation from vertical, as normalized angle
}

void CPotGraphics::UpdateAnimation()
{
	if (m_nModState) {	// if animation required
		int	nMods = m_arrModIdx.GetSize();
		for (int iMod = 0; iMod < nMods; iMod++) {	// for each modulation
			int	iProp = m_arrModIdx[iMod];
			m_Mod[iProp].m_fPhase += m_Mod[iProp].m_fPhaseSpeed / m_fFrameRate;
		}
		int	nMod2s = m_arrMod2Idx.GetSize();
		for (int iMod2 = 0; iMod2 < nMod2s; iMod2++) {	// for each secondary modulation
			int	iProp = m_arrMod2Idx[iMod2];
			m_Mod[iProp].m_fPhase += m_Mod[iProp].m_fPhaseSpeed / m_fFrameRate;
		}
		if (m_nModState & MOD_ANIMATED_MESH)	// if at least one mesh modulation is animated
			MakeMesh();
		else	// texture modulations only
			UpdateTextureCoords();
	}
}

void CPotGraphics::GetAnimationState(CPotProperties& Props) const
{
	int	nMods = m_arrModIdx.GetSize();
	for (int iMod = 0; iMod < nMods; iMod++) {	// for each modulation
		int	iProp = m_arrModIdx[iMod];
		Props.m_Mod[iProp].m_fPhase = m_Mod[iProp].m_fPhase;
	}
	int	nMod2s = m_arrMod2Idx.GetSize();
	for (int iMod2 = 0; iMod2 < nMod2s; iMod2++) {	// for each secondary modulation
		int	iProp = m_arrMod2Idx[iMod2];
		Props.m_Mod[iProp].m_fPhase = m_Mod[iProp].m_fPhase;
	}
}

bool CPotGraphics::UpdateTextureCoords()
{
//	CBenchmark	b;
	int	nMods = m_arrModIdx.GetSize();
	if (nMods)
		SaveModulatedProperties();
	CVertex	*pVert;
	CHECK(m_pMesh->LockVertexBuffer(0, (void **)&pVert));
	int	iVert = 0;
	int	nSides = m_nSides;
	int	nStride = nSides + 1;
	int	iLastRing = m_nRings - 1;
	for (int iRing = 0; iRing < m_nRings; iRing++) {	// for each ring
		double	fRing = double(iRing) / iLastRing;	// normalized ring
		if (nMods)
			ApplyModulations(fRing);
		for (int iSide = 0; iSide < nStride; iSide++) {	// for each side
			double	fSide = double(iSide) / nSides;	// normalized side
			D3DXVECTOR2	t;	// texture coords; local variable performs best
			GetTextureCoords(fRing, fSide, t);	// get texture coords for this vertex
			m_arrVert[iVert].t = t;	// write texture coords to vertex array
			pVert[iVert].t = t;	// write texture coords to mesh vertex buffer
			iVert++;
		}
	}
	int	nWallStride = m_nRings * nStride;
	int	iInnerVert = nWallStride;
	for (iVert = 0; iVert < nWallStride; iVert++) {	// for outer wall's vertices
		const CVertex&	vert = m_arrVert[iVert];
		pVert[iInnerVert].t = vert.t;	// copy texture coords to inner wall
		m_arrVert[iInnerVert].t = vert.t;
		iInnerVert++;
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
		if (rect.Pitch < nWidth * 4) {	// if texture's pitch is less than a row
			AfxMessageBox(IDS_ERR_TEXTURE_TOO_WIDE);
			CHECK(m_pTexture->UnlockRect(0));
			return false;	// avoid access outside texture buffer
		}
		BYTE	*pRow = (BYTE *)rect.pBits;
		for (int y = 0; y < nHeight; y++) {
			DWORD	*pBits = (DWORD *)pRow;
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
			pRow += rect.Pitch;	// advance to texture's next row
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

__forceinline double CPotGraphics::Wrap1(double x)
{
	return x - floor(x);
}

__forceinline double CPotGraphics::Square(double x)
{
	return x * x;
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
	case MOTIF_INVERT:
		r = -r;
		break;
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

__forceinline void CPotGraphics::ApplyPower(int iRange, int iPowerType, double fPower, double& r)
{
	// assume input is bipolar; output is unipolar or bipolar, depending on iRange
	if (iRange == CModulationProps::RANGE_UNIPOLAR) {
		if (fPower > 0) {	// if exponential
			double	fScale = fPower - 1;
			if (fScale) {	// avoid divide by zero
				if (iPowerType == CModulationProps::POWER_TYPE_ASYMMETRIC) {	// if asymmetric
					r = (r + 1) / 2;	// convert from bipolar to unipolar
					r = (pow(fPower, r) - 1) / fScale;
				} else {	// symmetric
					if (r >= 0)	// if positive input
						r = (pow(fPower, r) - 1) / fScale;
					else	// negative input
						r = -(pow(fPower, -r) - 1) / fScale;
					r = (r + 1) / 2;	 // convert from bipolar to unipolar
				}
			} else	// scale is zero
				r = (r + 1) / 2;	// convert from bipolar to unipolar
		} else	// not exponential
			r = (r + 1) / 2;	// convert from bipolar to unipolar
	} else {	// bipolar
		if (fPower > 0) {	// if exponential
			double	fScale = fPower - 1;
			if (fScale) {	// avoid divide by zero
				if (iPowerType == CModulationProps::POWER_TYPE_ASYMMETRIC) {	// if asymmetric
					r = (r + 1) / 2;	// convert from bipolar to unipolar
					r = (pow(fPower, r) - 1) / fScale;
					r = r * 2 - 1;	// convert from unipolar back to bipolar
				} else {	// symmetric
					if (r >= 0)	// if positive input
						r = (pow(fPower, r) - 1) / fScale;
					else	// negative input
						r = -(pow(fPower, -r) - 1) / fScale;
				}
			}
		}
	}
}

__forceinline double CPotGraphics::GetWave(int iWaveform, double fPhase, double fPulseWidth, double fSlew)
{
	switch (iWaveform) {
	case CModulationProps::WAVE_SINE:
		return sin(fPhase * M_PI * 2);
	case CModulationProps::WAVE_TRIANGLE:
		{
			double	r = Wrap1(fPhase + 0.25) * 4;
			return r < 2 ? r - 1 : 3 - r;
		}
	case CModulationProps::WAVE_RAMP_UP:
		return Wrap1(fPhase) * 2 - 1;
	case CModulationProps::WAVE_RAMP_DOWN:
		return 1 - Wrap1(fPhase) * 2;
	case CModulationProps::WAVE_SQUARE:
		return Wrap1(fPhase) < 0.5 ? 1 : -1;
	case CModulationProps::WAVE_PULSE:
		{
			double	r = Wrap1(fPhase);
			double	a = fPulseWidth / 2 * fSlew;
			if (r < a)	// if rising
				return r / a * 2 - 1;
			else if (r < fPulseWidth - a)	// if high
				return 1;
			else if (r < fPulseWidth)	// if falling
				return 1 - (r - (fPulseWidth - a)) / a * 2;
			else	// low
				return -1;
		}
	case CModulationProps::WAVE_ROUNDED_PULSE:
		{
			double	r = Wrap1(fPhase);
			double	a = fPulseWidth / 2 * fSlew;
			if (r < a)	// if rising
				return cos((r / a + 1) * M_PI);
			else if (r < fPulseWidth - a)	// if high
				return 1;
			else if (r < fPulseWidth)	// if falling
				return cos((r - (fPulseWidth - a)) / a * M_PI);
			else	// low
				return -1;
		}
	case CModulationProps::WAVE_CIRCULAR_PULSE:
		{
			double	r = Wrap1(fPhase);
			double	a = fPulseWidth / 2 * fSlew;
			if (r < a)	// if rising
				return sqrt(1 - Square(1 - r / a)) * 2 - 1;
			else if (r < fPulseWidth - a)	// if high
				return 1;
			else if (r < fPulseWidth)	// if falling
				return sqrt(1 - Square((r - (fPulseWidth - a)) / a)) * 2 - 1;
			else	// low
				return -1;
		}
	case CModulationProps::WAVE_TRIANGULAR_PULSE:
		{
			double	r = Wrap1(fPhase);
			if (r < fPulseWidth) {	// if high
				if (r < fPulseWidth * fSlew)	// if rising
					r = r / (fPulseWidth * fSlew);
				else	// falling
					r = (r - fPulseWidth) / (fPulseWidth * (fSlew - 1));
				return r * 2 - 1;	// make bipolar
			} else	// low
				return -1;
		}
	case CModulationProps::WAVE_RAMP_PULSE:
		{
			double	r = Wrap1(fPhase);
			double	a = fPulseWidth / 2 * fSlew;
			if (r < a)	// if rising
				return r / a;
			else if (r < fPulseWidth - a)	// if high
				return 1 - (r - a) / (fPulseWidth - a * 2) * 2;
			else if (r < fPulseWidth)	// if falling
				return (r - (fPulseWidth - a)) / a - 1;
			else	// low
				return 0;
		}
	case CModulationProps::WAVE_SINE_CUBED:
		return pow(sin(fPhase * M_PI * 2), 3);
	case CModulationProps::WAVE_FLAME:
		{
			double	r = Wrap1(fPhase + 0.25);
			double	s = sin(r * M_PI * 2);
			if (r < 0.25)
				return s - 1;
			else if (r < 0.5)
				return 1 - s;
			else if (r < 0.75)
				return s + 1;
			else
				return -1 - s;
		}
	case CModulationProps::WAVE_SEMICIRCLE:
		{
			double	r = Wrap1(fPhase);
			double	fSign;
			if (r < 0.5) {
				fSign = 1;
			} else {
				r = r - 0.5;
				fSign = -1;
			}
			return sqrt(1 - Square(r * 4 - 1)) * fSign;
		}
	}
	return 0;
}

__forceinline void CPotGraphics::Modulate(double fTheta, const PROPERTY_INFO& info, const CModulationProps& mod, LPCVOID pvSource, LPVOID pvTarget)
{
	double	r = GetWave(mod.m_iWaveform, fTheta * mod.m_fFrequency + mod.m_fPhase, 
		mod.m_fPulseWidth, mod.m_fSlew);	// for pulse waveforms only
	ApplyMotif(mod.m_iMotif, r);
	ApplyPower(mod.m_iRange, mod.m_iPowerType, mod.m_fPower, r);
	r = (r + mod.m_fBias) * mod.m_fAmplitude;	// apply bias and amplitude
	if (info.pType == &typeid(double)) {	// if property type is double
		const double	*pSource = static_cast<const double *>(pvSource);
		double	*pTarget = static_cast<double *>(pvTarget);
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
		case CModulationProps::OPER_EXPONENTIATE:
			*pTarget = *pSource * pow(2, r);
			break;
		}
	} else {	// assume property type is int
		ASSERT(info.pType == &typeid(int)); 
		const int	*pSource = static_cast<const int *>(pvSource);
		int	*pTarget = static_cast<int *>(pvTarget);
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
		case CModulationProps::OPER_EXPONENTIATE:
			*pTarget = round(*pSource * pow(2, r));
			break;
		}
	}
}

void CPotGraphics::OnModulationChange()
{
	UINT	nModState = 0;
	m_arrModIdx.SetSize(PROPERTIES);	// allocate enough for worst case
	int	nMods = 0;
	int	nMod2s = 0;
	for (int iProp = 0; iProp < PROPERTIES; iProp++) {	// for each property
		if (IsModulated(iProp)) {	// if property is modulated
			m_arrModIdx[nMods] = iProp;
			nMods++;
			if (IsAnimated(iProp)) {	// if property is animated
				nModState |= MOD_ANIMATED;
				if (m_Info[iProp].iGroup == GROUP_MESH)	// if mesh property
					nModState |= MOD_ANIMATED_MESH;
			}
		}
		nMod2s += GetSecondaryModulationCount(iProp);
	}
	m_arrModIdx.SetSize(nMods);	// resize primary modulation array
	m_arrMod2Idx.SetSize(nMod2s);	// resize secondary modulation array
	if (nMod2s) {	// if at least one secondary modulation
		int	iMod2 = 0;
		for (int iProp = 0; iProp < PROPERTIES; iProp++) {	// for each property
			int	iModTarget = iProp;	// get index of target property
			int	iModObj = iModTarget;
			for (int iModType = 1; iModType < CModulationProps::MOD_TYPES; iModType++) {	// for each secondary modulation type
				iModObj += PROPERTIES;	// skip to target property's next potential secondary modulation
				if (IsModulated(iModObj)) {	// if property is modulated
					m_arrMod2Idx[iMod2] = iModObj;
					iMod2++;
					if (IsAnimated(iModObj)) {	// if property is animated
						nModState |= MOD_ANIMATED;
						if (m_Info[iModTarget].iGroup == GROUP_MESH)	// if mesh property
							nModState |= MOD_ANIMATED_MESH;
					}
				}
			}
		}
	}
	m_nModState = nModState;
	m_iCurPlotProp = -1;	// invalidate current plot property
}

void CPotGraphics::ApplyModulations(double fRing)
{
	const double	fEpsilon = 1e-9;
	double	fTheta = min(fRing, 1.0 - fEpsilon);
	int	nMod2s = m_arrMod2Idx.GetSize();
	for (int iMod2 = 0; iMod2 < nMod2s; iMod2++) {	// for each secondary modulation
		int	iModObj = m_arrMod2Idx[iMod2];
		int	iModTarget, iModProp = GetModulationType(iModObj, iModTarget) + 1;
		const PROPERTY_INFO&	info = CModulationProps::m_Info[iModProp];
		const CModulationProps&	mod = m_Mod[iModObj];
		LPCVOID	pvSource = &m_arrSrcMods[iModObj];
		LPVOID	pvTarget = m_Mod[iModTarget].GetPropertyAddress(iModProp);
		Modulate(fTheta, info, mod, pvSource, pvTarget);
	}
	int	nMods = m_arrModIdx.GetSize();
	for (int iMod = 0; iMod < nMods; iMod++) {	// for each primary modulation
		int	iProp = m_arrModIdx[iMod];
		const PROPERTY_INFO&	info = m_Info[iProp];
		const CModulationProps&	mod = m_Mod[iProp];
		LPCVOID	pvSource = &m_arrSrcProps[iProp];
		LPVOID	pvTarget = GetPropertyAddress(iProp);
		Modulate(fTheta, info, mod, pvSource, pvTarget);
	}
}

__forceinline void CPotGraphics::PropCopy(LPVOID pDst, LPCVOID pSrc, size_t nLen) 
{
	if (nLen == 8)
		*static_cast<double*>(pDst) = *static_cast<const double*>(pSrc);
	else
		*static_cast<int*>(pDst) = *static_cast<const int*>(pSrc);
}

__forceinline void CPotGraphics::SaveModulatedProperties()
{
	int	nMods = m_arrModIdx.GetSize();
	for (int iMod = 0; iMod < nMods; iMod++) {	// for each primary modulation
		int	iProp = m_arrModIdx[iMod];
		PropCopy(&m_arrSrcProps[iProp], GetPropertyAddress(iProp), m_Info[iProp].nLen);
	}
	int	nMod2s = m_arrMod2Idx.GetSize();
	for (int iMod2 = 0; iMod2 < nMod2s; iMod2++) {	// for each secondary modulation
		int	iModObj = m_arrMod2Idx[iMod2];
		int	iModTarget, iModProp = GetModulationType(iModObj, iModTarget) + 1;
		const CModulationProps&	mod = m_Mod[iModTarget];
		PropCopy(&m_arrSrcMods[iModObj], mod.GetPropertyAddress(iModProp), mod.m_Info[iModProp].nLen);
	}
}

__forceinline void CPotGraphics::RestoreModulatedProperties()
{
	int	nMods = m_arrModIdx.GetSize();
	for (int iMod = 0; iMod < nMods; iMod++) {	// for each primary modulation
		int	iProp = m_arrModIdx[iMod];
		PropCopy(GetPropertyAddress(iProp), &m_arrSrcProps[iProp], m_Info[iProp].nLen);
	}
	int	nMod2s = m_arrMod2Idx.GetSize();
	for (int iMod2 = 0; iMod2 < nMod2s; iMod2++) {	// for each secondary modulation
		int	iModObj = m_arrMod2Idx[iMod2];
		int	iModTarget, iModProp = GetModulationType(iModObj, iModTarget) + 1;
		CModulationProps&	mod = m_Mod[iModTarget];
		PropCopy(mod.GetPropertyAddress(iModProp), &m_arrSrcMods[iModObj], mod.m_Info[iModProp].nLen);
	}
}

CPotGraphics::CDblRange CPotGraphics::CalcModulationRange(int iProp, UINT nFlags, const double *pfAmplitude) const
{
	const type_info *pType;
	LPCVOID	pSrcProp;	// assume source properties already saved
	if (iProp < PROPERTIES) {	// if primary modulation
		pType = m_Info[iProp].pType;
		pSrcProp = &m_arrSrcProps[iProp];
	} else {	// secondary modulation
		int	iModTarget, iModProp = GetModulationType(iProp, iModTarget) + 1;
		pType = CModulationProps::m_Info[iModProp].pType;
		pSrcProp = &m_arrSrcMods[iProp];
	}
	DPoint	pt;
	const CModulationProps&	mod = m_Mod[iProp];
	if (nFlags & CMR_FORCE_BIPOLAR) {
		pt = DPoint(-1, 1);
	} else {
		switch (mod.m_iMotif) {
		case CPotProperties::MOTIF_REEDS:
		case CPotProperties::MOTIF_PARTED_REEDS:
			pt = DPoint(0, 1);
			break;
		case CPotProperties::MOTIF_FLUTES:
		case CPotProperties::MOTIF_PARTED_FLUTES:
			pt = DPoint(-1, 0);
			break;
		default:
			pt = DPoint(-1, 1);
		}
		ApplyPower(mod.m_iRange, mod.m_iPowerType, mod.m_fPower, pt.x);
		ApplyPower(mod.m_iRange, mod.m_iPowerType, mod.m_fPower, pt.y);
	}
	double	fAmplitude;
	if (pfAmplitude != NULL) 
		fAmplitude = *pfAmplitude;
	else
		fAmplitude = mod.m_fAmplitude;
	pt = (pt + mod.m_fBias) * fAmplitude;
	double	fVal;
	if (pType == &typeid(double))	// if property type is double
		fVal = *(double *)pSrcProp;
	else
		fVal = *(int *)pSrcProp;
	DPoint	ptDst(fVal, fVal);
	switch (mod.m_iOperation) {
	case CModulationProps::OPER_ADD:
		ptDst += pt;
		break;
	case CModulationProps::OPER_SUBTRACT:
		ptDst -= pt;
		break;
	case CModulationProps::OPER_MULTIPLY:
		ptDst *= pt;
		break;
	case CModulationProps::OPER_DIVIDE:
		ptDst /= pt;
		break;
	case CModulationProps::OPER_EXPONENTIATE:
		ptDst *= DPoint(pow(2, pt.x), pow(2, pt.y));
		break;
	}
	CDblRange	range(ptDst.x, ptDst.y);
	range.Normalize();
	return range;
}

void CPotGraphics::PlotProperty(int iProp, CArrayEx<DPoint, DPoint&>& arrPoint, CRange<double> *pRange)
{
	int	iModTarget;
	const type_info *pType;
	LPVOID	pvProp;
	if (iProp < PROPERTIES) {	// if plotting primary modulation
		iModTarget = iProp;
		pType = m_Info[iProp].pType;
		pvProp = GetPropertyAddress(iProp);
	} else {	// plotting secondary modulation
		int	iModProp = GetModulationType(iProp, iModTarget) + 1;
		pType = CModulationProps::m_Info[iModProp].pType;
		pvProp = m_Mod[iModTarget].GetPropertyAddress(iModProp);
	}
	if (IsModulated(iProp)) {	// if desired property is modulated
		LPVOID	pvTarget = GetPropertyAddress(iModTarget);
		LPVOID	pvSource = &m_arrSrcProps[iModTarget];
		int	nTargetLen = m_Info[iModTarget].nLen;
		PropCopy(pvSource, pvTarget, nTargetLen);	// save target property
		int	nMods = m_arrModIdx.GetSize();
		int	iFirstMod;
		if (nMods) {	// if at least one primary modulation
			iFirstMod = m_arrModIdx[0];	// save first modulation
			m_arrModIdx.SetSize(1);	// only one modulation
			m_arrModIdx[0] = iModTarget;	// set modulation index array to target property
		} else {	// no primary modulations
			iFirstMod = 0;	// avoid compiler warnings
		}
		if (iModTarget != m_iCurPlotProp) {	// if plotting different property than before
			m_iCurPlotProp = iModTarget;	// update shadow
			int	nMod2s = GetSecondaryModulationCount(iModTarget);	// count target's secondary modulations
			m_arrPlotMod2Idx.FastSetSize(nMod2s);	// size array without zeroing or freeing memory
			if (nMod2s) {	// if any secondary modulations for this target
				int	iModObj = iModTarget;
				int	iMod2 = 0;
				for (int iModType = 1; iModType < CModulationProps::MOD_TYPES; iModType++) {	// for each secondary modulation type
					iModObj += PROPERTIES;	// skip to specified property's next potential secondary modulation
					if (IsModulated(iModObj)) {
						m_arrPlotMod2Idx[iMod2] = iModObj;
						iMod2++;
					}
				}
			}
		}
		int	nMod2s = m_arrPlotMod2Idx.GetSize();
		for (int iMod2 = 0; iMod2 < nMod2s; iMod2++) {	// for each of target's secondary modulations
			int	iModObj = m_arrPlotMod2Idx[iMod2];
			int	iModTarget, iModProp = GetModulationType(iModObj, iModTarget) + 1;
			const CModulationProps&	mod = m_Mod[iModTarget];
			PropCopy(&m_arrSrcMods[iModObj], mod.GetPropertyAddress(iModProp), mod.m_Info[iModProp].nLen);
		}
		m_arrMod2Idx.Swap(m_arrPlotMod2Idx);	// swap main secondary modulations array with plot's
		int	nRings = m_nRings;
		arrPoint.SetSize(nRings);
		for (int iRing = 0; iRing < nRings; iRing++) {	// for each ring
			double	fRing = double(iRing) / (nRings - 1);	// normalized ring
			ApplyModulations(fRing);
			arrPoint[iRing].x = ConvertPropertyToDouble(pvProp, pType);
			arrPoint[iRing].y = fRing;
		}
		m_arrMod2Idx.Swap(m_arrPlotMod2Idx);	// swap again to restore secondary modulation arrays
		for (int iMod2 = 0; iMod2 < nMod2s; iMod2++) {	// for each of target's secondary modulations
			int	iModObj = m_arrPlotMod2Idx[iMod2];
			int	iModTarget, iModProp = GetModulationType(iModObj, iModTarget) + 1;
			CModulationProps&	mod = m_Mod[iModTarget];
			PropCopy(mod.GetPropertyAddress(iModProp), &m_arrSrcMods[iModObj], mod.m_Info[iModProp].nLen);
		}
		if (nMods) {	// if at least one primary modulation
			m_arrModIdx.SetSize(nMods);	// restore modulation index array's size
			m_arrModIdx[0] = iFirstMod;	// restore first modulation
		}
		PropCopy(pvTarget, pvSource, nTargetLen);	// restore target property
		if (pRange != NULL) {	// if caller requested range
			CDblRange	range;
			if (iProp < PROPERTIES) {	// if plotting primary modulation
				UINT	nFlags;
				if (IsModulated(iProp, CModulationProps::MOD_TYPE_iRange)
				|| IsModulated(iProp, CModulationProps::MOD_TYPE_iMotif)
				|| IsModulated(iProp, CModulationProps::MOD_TYPE_fPower)
				|| IsModulated(iProp, CModulationProps::MOD_TYPE_iPowerType))
					nFlags |= CMR_FORCE_BIPOLAR;
				else	// plotting secondary modulation
					nFlags = 0;
				int	iAmpMod = MakeModulationIdx(iProp, CModulationProps::MOD_TYPE_fAmplitude);
				if (IsModulated(iAmpMod)) {	// if amplitude modulated
					CDblRange	rangeAmp(CalcModulationRange(iAmpMod));
					range = CalcModulationRange(iProp, nFlags, &rangeAmp.Start);
					CDblRange	range2(CalcModulationRange(iProp, nFlags, &rangeAmp.End));
					range.Include(range2);
				} else	// not amplitude modulated
					range = CalcModulationRange(iProp, nFlags);
			} else	// plotting secondary modulation
				range = CalcModulationRange(iProp);
			*pRange = range;	// pass range back to caller
		}
	} else {	// desired property not modulated; trivial case
		double	fVal = ConvertPropertyToDouble(pvProp, pType);
		arrPoint.SetSize(2);
		arrPoint[0] = DPoint(fVal, 0);
		arrPoint[1] = DPoint(fVal, 1);
		if (pRange != NULL) {	// if caller requested range
			*pRange = CDblRange(fVal, fVal);	// pass range back to caller
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
	x = Wrap1(x - 0.5 / sz.cx);	// offset by half a pixel
	int	ix = trunc(x * sz.cx);
	int	ix1 = ix < sz.cx ? ix : 0;
	int	ix2 = ix + 1 < sz.cx ? ix + 1 : 0;
	y = Wrap1(y - 0.5 / sz.cy);	// offset by half a pixel
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
		CDibEx	dibTextureFile, *pDibTexture;
		if (!m_sTexturePath.IsEmpty()) {	// if we have a texture file
			CImage	image;
			if (FAILED(image.Load(m_sTexturePath)))	// load image from texture file
				return false;
			if (!dibTextureFile.Create(image.GetWidth(), image.GetHeight(), 24))	// create texture bitmap
				return false;
			CDC	dc;
			if (!dc.CreateCompatibleDC(NULL))	// create memory device context
				return false;
			HGDIOBJ	hPrevBmp = dc.SelectObject(dibTextureFile);	// select texture bitmap
			if (hPrevBmp == NULL)
				return false;
			if (!image.BitBlt(dc, 0, 0, SRCCOPY))	// blit image into texture bitmap
				return false;
			dc.SelectObject(hPrevBmp);	// reselect default bitmap
			pDibTexture = &dibTextureFile;	// use bitmap created from texture file
		} else	// no texture file
			pDibTexture = &m_dibTexture;	// use synthesized texture bitmap
		for (int iVert = 0; iVert < nVerts; iVert++) {	// for each vertex
			const CVertex&	vert = m_arrVert[iVert];
			fOut.Write(&vert, sizeof(C3DVertexNormal));	// omit texture coords
			COLORREF	c = Interpolate(*pDibTexture, vert.t.x, vert.t.y);
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
	DWORD	nFillMode;
	CHECK(m_pDevice->GetRenderState(D3DRS_FILLMODE, &nFillMode));	// save fill mode
	CHECK(m_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));	// set wireframe mode
	D3DMATERIAL9	material;
	CHECK(m_pDevice->GetMaterial(&material));	// save material
	CHECK(m_pDevice->SetMaterial(&m_mtrlBounds));	// set bounds material
	IDirect3DBaseTexture9	*pTexture;
	CHECK(m_pDevice->GetTexture(0, &pTexture));	// save texture
	CHECK(m_pDevice->SetTexture(0, NULL));	// disable texture
	CHECK(m_pBounds->DrawSubset(0));	// draw bounding box
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

void CPotGraphics::ComputeOuterRadiusRange(double& fMinRadius, double& fMaxRadius) const
{
	int	nRads = m_nSides * m_nRings;
	if (!nRads) {	// if empty
		fMinRadius = 0;
		fMaxRadius = 0;
		return;
	}
	double	fMin = m_faOuterRadius[0];	// init to first radius
	double	fMax = m_faOuterRadius[0];
	for (int iRad = 1; iRad < nRads; iRad++) {	// for each subsequent radius
		double	r = m_faOuterRadius[iRad];
		if (r < fMin)
			fMin = r;
		else if (r > fMax)
			fMax = r;
	}
	fMinRadius = fMin;
	fMaxRadius = fMax;
}

int CPotGraphics::HitTest(CPoint point) const
{
	D3DXVECTOR3	rayPos, rayDir;
	GetRay(point, rayPos, rayDir);
	int	nFaces = m_nFaces;
	int	iHitFace = -1;
	float	fHitDist = 1;
	int	iIdx = 0;
	for (int iFace = 0; iFace < nFaces; iFace++) {	// for each face
		float	fDist;
		BOOL	bHit = D3DXIntersectTri(&m_arrVert[m_arrIdx[iIdx + 0]].pt, &m_arrVert[m_arrIdx[iIdx + 1]].pt, 
			&m_arrVert[m_arrIdx[iIdx + 2]].pt, &rayPos, &rayDir, NULL, NULL, &fDist);
		// if ray intersects face and intersection is nearer than current contender
		if (bHit && fDist < fHitDist) {
			iHitFace = iFace;	// store index of intersecting face
			fHitDist = fDist;	// update intersection distance
		}
		iIdx += 3;
	}
	return iHitFace;
}

void CPotGraphics::GetVertexCoords(int iVert, int& iWall, int& iRing, int& iSide) const
{
	int	nStride = m_nSides + 1;
	int	nWallStride = m_nRings * nStride;
	iWall = iVert / nWallStride;
	int	iWallVert = iVert % nWallStride;
	iRing = iWallVert / nStride;
	iSide = iWallVert % nStride;
}

#if POT_GFX_SHOW_FACE_NORMALS
bool CPotGraphics::CreateFaceNormalLines()
{
	const float	fNormalLineLength = 5.0f;
	m_vbFaceNormalLine.m_pVertBuf = NULL;	// release previous vertex buffer if any
	int	nFaces = m_nFaces;
	if (!nFaces)
		return false;
	int	nBufSize = sizeof(C3DLine) * nFaces;
	CHECK(m_pDevice->CreateVertexBuffer(nBufSize, 0, D3DFVF_XYZ, D3DPOOL_MANAGED, &m_vbFaceNormalLine.m_pVertBuf, NULL));
	C3DLine	*pLine;
	CHECK(m_vbFaceNormalLine.m_pVertBuf->Lock(0, 0, reinterpret_cast<LPVOID *>(&pLine), 0));	// lock vertex buffer
	int	iIdx = 0;
	for (int iFace = 0; iFace < nFaces; iFace++) {	// for each face
		D3DXVECTOR3	vCtr = (m_arrVert[m_arrIdx[iIdx + 0]].pt + m_arrVert[m_arrIdx[iIdx + 1]].pt 
			+ m_arrVert[m_arrIdx[iIdx + 2]].pt) / 3;	// compute triangle's centroid
		pLine[iFace].v[0] = vCtr;
		pLine[iFace].v[1] = vCtr + m_arrFaceNormal[iFace] * fNormalLineLength;
		iIdx += 3;	// three indices per face
	}
	CHECK(m_vbFaceNormalLine.m_pVertBuf->Unlock());	// unlock vertex buffer
	m_vbFaceNormalLine.m_nPrimitives = nFaces;
	return true;
}

bool CPotGraphics::DrawFaceNormalLines()
{
	D3DMATERIAL9	material;
	CHECK(m_pDevice->GetMaterial(&material));	// save material
	CHECK(m_pDevice->SetMaterial(&m_mtrlBounds));
	IDirect3DBaseTexture9	*pTexture;
	CHECK(m_pDevice->GetTexture(0, &pTexture));	// save texture
	CHECK(m_pDevice->SetTexture(0, NULL));	// disable texture
	CHECK(m_pDevice->SetStreamSource(0, m_vbFaceNormalLine.m_pVertBuf, 0, sizeof(D3DXVECTOR3)));
	CHECK(m_pDevice->DrawPrimitive(D3DPT_LINELIST, 0, m_vbFaceNormalLine.m_nPrimitives));
	CHECK(m_pDevice->SetMaterial(&material));	// restore material
	CHECK(m_pDevice->SetTexture(0, pTexture));	// restore texture
	return true;
}
#endif	// POT_GFX_SHOW_FACE_NORMALS

#if POT_GFX_SHOW_VERTEX_NORMALS
bool CPotGraphics::CreateVertexNormalLines()
{
	const float	fNormalLineLength = 5.0f;
	m_vbVertexNormalLine.m_pVertBuf = NULL;	// release previous vertex buffer if any
	int	nVerts = m_arrVert.GetSize();
	if (!nVerts)
		return false;
	int	nBufSize = sizeof(C3DLine) * nVerts;
	CHECK(m_pDevice->CreateVertexBuffer(nBufSize, 0, D3DFVF_XYZ, D3DPOOL_MANAGED, &m_vbVertexNormalLine.m_pVertBuf, NULL));
	C3DLine	*pLine;
	CHECK(m_vbVertexNormalLine.m_pVertBuf->Lock(0, 0, reinterpret_cast<LPVOID *>(&pLine), 0));	// lock vertex buffer
	for (int iVert = 0; iVert < nVerts; iVert++) {	// for each vertex
		D3DXVECTOR3	v = m_arrVert[iVert].pt;
		pLine[iVert].v[0] = v;
		pLine[iVert].v[1] = v + m_arrVert[iVert].n * fNormalLineLength;
	}
	CHECK(m_vbVertexNormalLine.m_pVertBuf->Unlock());	// unlock vertex buffer
	m_vbVertexNormalLine.m_nPrimitives = nVerts;
	return true;
}

bool CPotGraphics::DrawVertexNormalLines()
{
	D3DMATERIAL9	material;
	CHECK(m_pDevice->GetMaterial(&material));	// save material
	CHECK(m_pDevice->SetMaterial(&m_mtrlBounds));
	IDirect3DBaseTexture9	*pTexture;
	CHECK(m_pDevice->GetTexture(0, &pTexture));	// save texture
	CHECK(m_pDevice->SetTexture(0, NULL));	// disable texture
	CHECK(m_pDevice->SetStreamSource(0, m_vbVertexNormalLine.m_pVertBuf, 0, sizeof(D3DXVECTOR3)));
	CHECK(m_pDevice->DrawPrimitive(D3DPT_LINELIST, 0, m_vbVertexNormalLine.m_nPrimitives));
	CHECK(m_pDevice->SetMaterial(&material));	// restore material
	CHECK(m_pDevice->SetTexture(0, pTexture));	// restore texture
	return true;
}
#endif	// POT_GFX_SHOW_VERTEX_NORMALS

#if POT_GFX_SHOW_FACE_SELECTION
bool CPotGraphics::CreateSelectedFaces()
{
	m_vbSelectedFace.m_pVertBuf = NULL;	// release previous vertex buffer if any
	int	nSelFaces = m_arrSelectedFaceIdx.GetSize();
	if (!nSelFaces)
		return false;
	int	nBufSize = sizeof(C3DTriangle) * nSelFaces;
	CHECK(m_pDevice->CreateVertexBuffer(nBufSize, 0, D3DFVF_XYZ, D3DPOOL_MANAGED, &m_vbSelectedFace.m_pVertBuf, NULL));
	C3DTriangle	*pTri;
	CHECK(m_vbSelectedFace.m_pVertBuf->Lock(0, 0, reinterpret_cast<LPVOID *>(&pTri), 0));	// lock vertex buffer
	for (int iSelFace = 0; iSelFace < nSelFaces; iSelFace++) {
		int	iIdx = m_arrSelectedFaceIdx[iSelFace] * 3;
		if (iIdx < m_arrIdx.GetSize()) {	// if index within range
			pTri[iSelFace].v[0] = m_arrVert[m_arrIdx[iIdx + 0]].pt;
			pTri[iSelFace].v[1] = m_arrVert[m_arrIdx[iIdx + 1]].pt;
			pTri[iSelFace].v[2] = m_arrVert[m_arrIdx[iIdx + 2]].pt;
		}
	}
	CHECK(m_vbSelectedFace.m_pVertBuf->Unlock());	// unlock vertex buffer
	m_vbSelectedFace.m_nPrimitives = nSelFaces;
	return true;
}

bool CPotGraphics::DrawSelectedFaces()
{
	DWORD	nFillMode;
	CHECK(m_pDevice->GetRenderState(D3DRS_FILLMODE, &nFillMode));	// save fill mode
	CHECK(m_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));	// set wireframe mode
	D3DMATERIAL9	material;
	CHECK(m_pDevice->GetMaterial(&material));	// save material
	CHECK(m_pDevice->SetMaterial(&m_mtrlBounds));
	IDirect3DBaseTexture9	*pTexture;
	CHECK(m_pDevice->GetTexture(0, &pTexture));	// save texture
	CHECK(m_pDevice->SetTexture(0, NULL));	// disable texture
	CHECK(m_pDevice->SetStreamSource(0, m_vbSelectedFace.m_pVertBuf, 0, sizeof(D3DXVECTOR3)));
	CHECK(m_pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, m_vbSelectedFace.m_nPrimitives));
	CHECK(m_pDevice->SetRenderState(D3DRS_FILLMODE, nFillMode));	// restore fill mode
	CHECK(m_pDevice->SetMaterial(&material));	// restore material
	CHECK(m_pDevice->SetTexture(0, pTexture));	// restore texture
	return true;
}
#endif	// POT_GFX_SHOW_FACE_SELECTION

void CPotGraphics::GetSelection(CIntArrayEx& arrFaceIdx) const
{
#if POT_GFX_SHOW_FACE_SELECTION
	arrFaceIdx = m_arrSelectedFaceIdx;
#else
	ASSERT(0);	// optional feature disabled
#endif	// POT_GFX_SHOW_FACE_SELECTION
}

bool CPotGraphics::SetSelection(const CIntArrayEx& arrFaceIdx)
{
#if POT_GFX_SHOW_FACE_SELECTION
	m_arrSelectedFaceIdx = arrFaceIdx;
	return CreateSelectedFaces();
#else
	ASSERT(0);	// optional feature disabled
	return false;
#endif	// POT_GFX_SHOW_FACE_SELECTION
}
