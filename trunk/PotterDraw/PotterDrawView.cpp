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
		02		05oct17	add scallop phase, range, power, operation
		03		09oct17	in Record, save and restore auto-rotate and animation
		04		01nov17	add scallop waveform, pulse width, slew
		05		01nov17	add polygon properties
		06		06nov17	add lighting dialog
		07		13nov17	in OnUpdate, use mesh subgroups
		08		15nov17	add palette import/export
		09		16nov17	in OnUpdate, if radius color pattern selected, remake mesh
		10		22nov17	in OnUpdate, for modulation hint, add mesh group check
		11		23nov17	add step animation
		12		24nov17	don't sync phases during undo/redo animation
		13		06dec17	in OnLButtonDown, add optional face selection
		14		12dec17	add transparent render style
		15		02jan18	add ruffle properties
		16		15jan18	add auto zoom
		17		20feb18	in OnUpdate, add OnModulationChange

*/

// PotterDrawView.cpp : implementation of the CPotterDrawView class
//

#include "stdafx.h"
#include "PotterDraw.h"
#include "PotterDrawDoc.h"
#include "PotterDrawView.h"
#include "MainFrm.h"
#include "FolderDialog.h"
#include "PathStr.h"
#include "RecordDlg.h"
#include "RecordStatusDlg.h"
#include "UndoCodes.h"
#include "RotateDlg.h"
#include "LightingDlg.h"

#define _USE_MATH_DEFINES	// for trig constants
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define VIEW_NATTER 0
#if !VIEW_NATTER
#define printf sizeof
#endif

// CPotterDrawView

IMPLEMENT_DYNCREATE(CPotterDrawView, CView)

// CPotterDrawView construction/destruction

#define RK_RECORD_FOLDER _T("RecordFolder")
#define RK_EXPORT_TYPE _T("ExportType")

CPotterDrawView::CPotterDrawView()
{
	m_bAutoRotate = false;
	m_bDragRoll = false;
	m_bInFrameTimer = false;
	m_nDragState = DS_NONE;
	m_ptDragOrigin = CPoint(0, 0);
	m_vDragPan = D3DXVECTOR3(0, 0, 0);
	m_vDragRotation = D3DXVECTOR3(0, 0, 0);
	m_vAutoRotateSpeed = D3DXVECTOR3(0, 0, 0);
	m_fAutoZoomScale = 0;
	m_nBenchFrames = 0;
	m_iRecordFileFormat = 0;
	m_nRecordDuration = 0;
	m_nRecordFramesDone = 0;
	m_szRecordFrame = CSize(0, 0);
	m_bPreRecordAutoRotate = false;
	m_bPreRecordAnimation = false;
}

CPotterDrawView::~CPotterDrawView()
{
}

BOOL CPotterDrawView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

int CPotterDrawView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (m_TimerPeriod.Create(1)) {
		AfxMessageBox(IDS_ERR_CANT_CREATE_MM_TIMER_PERIOD);
		return -1;
	}

	return 0;
}

bool CPotterDrawView::CreateDevice()
{
	CRect	rc;
	GetClientRect(rc);
	CSize	szClient(rc.Size());
	printf("view %Ix CreateDevice cy=%d cx=%d\n", this, szClient.cx, szClient.cy);
	if (!(szClient.cx && szClient.cy))	// if window has a zero dimension
		return false;
	CWaitCursor	wc;
	if (!m_Graphics.Create()) {
		AfxMessageBox(IDS_ERR_CANT_CREATE_DIRECT3D);
		return false;
	}
	if (!m_Graphics.CreateDevice(m_hWnd, szClient)) {
		AfxMessageBox(IDS_ERR_CANT_CREATE_DIRECT3D_DEVICE);
		return false;
	}
	return true;
}

void CPotterDrawView::SetStandardView(int iView)
{
	m_Graphics.SetStandardView(iView);
	Invalidate();
}

bool CPotterDrawView::IsActive() const
{
	return(theApp.GetMainFrame()->GetActiveMDIView() == this);
}

void CPotterDrawView::OnInitialUpdate()
{
	printf("view %Ix OnInitialUpdate\n", this);
	theApp.GetMainFrame()->SetDeferredUpdate(true);
	CPotterDrawDoc	*pDoc = GetDocument();
	if (!pDoc->GetPathName().IsEmpty())	// if not new document
		m_Graphics.SetViewState(*pDoc);
	UpdateAutoRotateSpeed();
	PostMessage(UWM_DEFERRED_UPDATE);	// avoids flicker due to repeated resizing
//	CView::OnInitialUpdate();
}

void CPotterDrawView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	printf("view %Ix OnUpdate %Ix %Id %Ix\n", this, pSender, lHint, pHint);
	CPotterDrawDoc	*pDoc = GetDocument();
	if (m_Graphics.m_bAnimation) {	// if currently animating
		bool	bSync = true;
		switch (lHint) {
		case CPotterDrawDoc::HINT_PROPERTY:
			{
				ASSERT(pHint != NULL);
				CPotterDrawDoc::CPropertyHint	*pPropHint = static_cast<CPotterDrawDoc::CPropertyHint *>(pHint);
				if (pPropHint->m_iProp == CPotProperties::PROP_bAnimation) {	// if animation edit
					if (pDoc->IsUndoing() || pDoc->IsRedoing())	// if undo/redo in progress
						bSync = false; // don't synchronize
				}
			}
			break;
		case CPotterDrawDoc::HINT_MODULATION:
			{
				ASSERT(pHint != NULL);
				CPotterDrawDoc::CModulationHint	*pModHint = static_cast<CPotterDrawDoc::CModulationHint *>(pHint);
				if (pModHint->m_iModProp == CModulationProps::PROP_fPhase)	// if phase edit
					bSync = false; // don't synchronize
			}
			break;
		case CPotterDrawDoc::HINT_MOD_PHASE:
			bSync = false;	// don't synchronize
			break;
		}
		if (bSync) {	// if synchronization needed
			UpdateWindow();	// synchronize window with animation state
			m_Graphics.GetAnimationState(*pDoc);	// update document's animation state
		}
	}
	if (!m_Graphics.IsDeviceCreated()) {	// if device not created
		m_Graphics.SetProperties(*pDoc);	// update graphics properties from document
		m_Graphics.OnModulationChange();
		theApp.GetMainFrame()->OnUpdate(pSender, lHint, pHint);	// relay update to main frame
		m_Graphics.CalcSpline(pDoc->m_arrSpline);
		CreateDevice();
	} else {	// normal case: device created
		bool	bMakeMesh = false;
		bool	bMakeTexture = false;
		bool	bMakeSpline = false;
		bool	bResizing = false;
		bool	bModulationChange = false;
		switch (lHint) {
		case CPotterDrawDoc::HINT_PROPERTY:
			{
				ASSERT(pHint != NULL);
				CPotterDrawDoc::CPropertyHint	*pPropHint = static_cast<CPotterDrawDoc::CPropertyHint *>(pHint);
				int	iProp = pPropHint->m_iProp;
				int	iGroup = CPotProperties::m_Info[iProp].iGroup;
				switch (iGroup) {
				case CPotProperties::GROUP_MESH:
					switch (CPotProperties::m_Info[iProp].iSubgroup) {	// mesh subgroups
					case CPotProperties::SUBGROUP_POLYGON:
						if (pDoc->IsPolygon() || m_Graphics.IsPolygon())
							bMakeMesh = true;
						break;
					case CPotProperties::SUBGROUP_SCALLOP:
						if (pDoc->HasScallops() || m_Graphics.HasScallops())
							bMakeMesh = true;
						break;
					case CPotProperties::SUBGROUP_RIPPLE:
						if (pDoc->HasRipples() || m_Graphics.HasRipples())
							bMakeMesh = true;
						break;
					case CPotProperties::SUBGROUP_BEND:
						if (pDoc->HasBends() || m_Graphics.HasBends())
							bMakeMesh = true;
						break;
					case CPotProperties::SUBGROUP_RUFFLE:
						if ((pDoc->HasBends() || m_Graphics.HasBends()) 
						&& (pDoc->HasRuffles() || m_Graphics.HasRuffles()))
							bMakeMesh = true;
						break;
					case CPotProperties::SUBGROUP_HELIX:
						if (pDoc->HasHelix() || m_Graphics.HasHelix())
							bMakeMesh = true;
						break;
					default:
						// mesh properties in subgroups should be handled above
						ASSERT(CPotProperties::m_Info[iProp].iSubgroup < 0);
						switch (iProp) {
						case CPotProperties::PROP_nRings:
							bMakeMesh = true;
							bMakeSpline = true;
							break;
						default:
							bMakeMesh = true;
						}
					}
					break;
				case CPotProperties::GROUP_TEXTURE:
					switch (iProp) {
					case CPotProperties::PROP_nColors:
					case CPotProperties::PROP_nColorSharpness:
					case CPotProperties::PROP_iPaletteType:
					case CPotProperties::PROP_sTexturePath:
						bMakeTexture = true;
						break;
					case CPotProperties::PROP_iColorPattern:
						if (pDoc->m_iColorPattern == CPotProperties::COLORPAT_RADIUS) {	// if radius color pattern
							bMakeMesh = true;	// need to compute radius range and update texure coords
							bResizing = true;	// but don't need to recalculate vertices and faces
						}
						break;
					}
					break;
				case CPotProperties::GROUP_VIEW:
					switch (iProp) {
					case CPotProperties::PROP_fAutoRotateYaw:
					case CPotProperties::PROP_fAutoRotatePitch:
					case CPotProperties::PROP_fAutoRotateRoll:
					case CPotProperties::PROP_fAutoRotateZoom:
						UpdateAutoRotateSpeed();
						break;
					case CPotProperties::PROP_fFrameRate:
						UpdateAutoRotateSpeed();
						if (IsAnimating()) {
							SetAnimation(false);
							SetAnimation(true);
						}
						break;
					case CPotProperties::PROP_bAnimation:
						if ((m_Graphics.GetModulationState() & CPotProperties::MOD_ANIMATED_MESH)	// if animating mesh
						&& (pDoc->IsUndoing() || pDoc->IsRedoing()))	// and undo/redo in progress
							bMakeMesh = true;	// mesh animation phases were restored above
						break;
					}
					break;
				}
			}
			break;
		case CPotterDrawDoc::HINT_PALETTE:
			bMakeTexture = true;
			break;
		case CPotterDrawDoc::HINT_MODULATION:
			{
				ASSERT(pHint != NULL);
				CPotterDrawDoc::CModulationHint	*pModHint = static_cast<CPotterDrawDoc::CModulationHint *>(pHint);
				int	iTarget = pModHint->m_iTarget;
				if ((pDoc->IsModulated(iTarget) || m_Graphics.IsModulated(iTarget))	// if target will be or is modulated
				&& CPotProperties::m_Info[iTarget].iGroup == CPotProperties::GROUP_MESH)	// and target is mesh property
					bMakeMesh = true;
				switch (pModHint->m_iModProp) {
				case CModulationProps::PROP_iWaveform:
				case CModulationProps::PROP_fPhaseSpeed:
					bModulationChange = true;
					break;
				}
			}
			break;
		case CPotterDrawDoc::HINT_SPLINE:
			bMakeSpline = true;
			bMakeMesh = true;
			break;
		case CPotterDrawDoc::HINT_SPLINE_DRAG:
			m_Graphics.CalcSpline(theApp.GetMainFrame()->GetSplineWnd().GetSpline());
			m_Graphics.MakeMesh(false);
			RedrawWindow();	// redraw view explicitly to avoid paint lag
			return;	// early out, don't update document
		case CPotterDrawDoc::HINT_LIGHTING:
			{
				D3DLIGHT9	light;
				m_Graphics.GetLight(light);
				light.Direction = pDoc->m_vLightDir;
				m_Graphics.SetLight(light);
				m_Graphics.SetPotMaterial(pDoc->m_mtrlPot);
				Invalidate();	// necessary due to early out below
			}
			return;	// early out, to avoid needlessly updating texture
		case CPotterDrawDoc::HINT_MOD_PHASE:
			if (m_Graphics.GetModulationState() & CPotProperties::MOD_ANIMATED_MESH)
				bMakeMesh = true;
			break;
		default:	// no hint
			bMakeMesh = true;
			bMakeSpline = true;
			bModulationChange = true;
			UpdateAutoRotateSpeed();
		}
		printf("view %Ix OnUpdate mesh=%d texture=%d spline=%d resize=%d\n", 
			this, bMakeMesh, bMakeTexture, bMakeSpline, bResizing);
		m_Graphics.SetProperties(*pDoc);	// update graphics properties from document
		if (bModulationChange)
			m_Graphics.OnModulationChange();
		theApp.GetMainFrame()->OnUpdate(pSender, lHint, pHint);	// relay update to main frame
		if (bMakeSpline)
			m_Graphics.CalcSpline(pDoc->m_arrSpline);
		if (bMakeMesh) {	// if mesh is stale
			CWaitCursor wc;	// mesh generation can be slow
			m_Graphics.MakeMesh(bResizing);
		} else {	// reusing mesh
			if (bMakeTexture)	// if texture is stale
				m_Graphics.MakeTexture();
			m_Graphics.UpdateTextureCoords();
		}
	}
	SetAnimation(m_bAutoRotate || pDoc->m_bAnimation);
	Invalidate();
}

void CALLBACK CPotterDrawView::TimerCallback(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	CPotterDrawView*	pView = reinterpret_cast<CPotterDrawView*>(dwUser);
	// if drawing takes too long and falls behind frame rate, drop ticks instead 
	// of posting more timer messages, else message queue backs up and hangs app
	if (!pView->m_bInFrameTimer)	// if not already in frame timer handler
		::PostMessage(pView->m_hWnd, UWM_FRAME_TIMER, 0, 0);
}

void CPotterDrawView::SetAnimation(bool bEnable)
{
	if (bEnable == IsAnimating())	// if already in requested state
		return;	// nothing to do
	if (bEnable) {
		UINT	fuEvent = TIME_PERIODIC | TIME_CALLBACK_FUNCTION;
		DWORD_PTR	dwUser = reinterpret_cast<DWORD_PTR>(this);
		DWORD	nPeriod = round(1000 / GetDocument()->m_fFrameRate);
		if (!m_FrameTimer.Create(nPeriod, 0, TimerCallback, dwUser, fuEvent)) {
			AfxMessageBox(IDS_ERR_CANT_CREATE_MM_TIMER);
			return;
		}
		m_nBenchFrames = 0;
		m_tBenchFrame.Reset();
	} else {
		m_FrameTimer.Destroy();
	}
}

void CPotterDrawView::UpdateAnimationEnable()
{
	SetAnimation((m_bAutoRotate || GetDocument()->m_bAnimation) && !IsRecording());
}

float CPotterDrawView::CalcPanStep() const
{
	return static_cast<float>(theApp.m_Options.m_fPanStep / m_Graphics.GetZoom());
}

double CPotterDrawView::CalcZoomStep() const
{
	return theApp.m_Options.m_fZoomStep / 100 + 1;
}

double CPotterDrawView::CalcAutoRotateSpeed(double fDegreesPerSec, double fFrameRate)
{
	return fDegreesPerSec / 180 * M_PI / fFrameRate;
}

void CPotterDrawView::UpdateAutoRotateSpeed()
{
	CPotterDrawDoc	*pDoc = GetDocument();
	double	fFrameRate = pDoc->m_fFrameRate;
	m_vAutoRotateSpeed.x = float(CalcAutoRotateSpeed(pDoc->m_fAutoRotateYaw, fFrameRate));
	m_vAutoRotateSpeed.y = float(CalcAutoRotateSpeed(pDoc->m_fAutoRotatePitch, fFrameRate));
	m_vAutoRotateSpeed.z = float(CalcAutoRotateSpeed(pDoc->m_fAutoRotateRoll, fFrameRate));
	if (pDoc->m_fAutoRotateZoom) {	// if auto zooming
		bool	bIsNeg = pDoc->m_fAutoRotateZoom < 0;
		double	fScale = fabs(pDoc->m_fAutoRotateZoom) / 100 + 1;
		fScale = exp(log(fScale) / fFrameRate);	// compensate for frame rate
		if (bIsNeg)	// if zoom percentage was negative
			fScale = 1 / fScale;	// take reciprocal
		m_fAutoZoomScale = fScale;
	} else	// not auto zooming
		m_fAutoZoomScale = 0;
}

double CPotterDrawView::MeasureFrameRate()
{
	double	tElapsed = m_tBenchFrame.Elapsed();	// get elapsed time since last measurement
	m_tBenchFrame.Reset();	// reset timer ASAP
	double	fFrameRate = m_nBenchFrames / tElapsed;	// compute actual frame rate
	m_nBenchFrames = 0;	// reset frame counter
	return fFrameRate;
}

void CPotterDrawView::StartDrag(UINT nFlags, CPoint point, bool bPan)
{
	if (m_nDragState == DS_NONE) {
		if (bPan)	// if control key down
			m_nDragState = DS_PAN;
		else	// no modifiers
			m_nDragState = DS_ROTATE;
		m_ptDragOrigin = point;
		m_vDragPan = m_Graphics.GetPan();
		m_vDragRotation = m_Graphics.GetRotation();
		m_bDragRoll = (nFlags & MK_SHIFT) != 0;
		SetCapture();
	}
}

void CPotterDrawView::EndDrag()
{
	switch (m_nDragState) {
	case DS_ROTATE:
	case DS_PAN:
		m_nDragState = DS_NONE;
		ReleaseCapture();
		break;
	}
}

CString	CPotterDrawView::MakeFileFilter(const FILE_TYPE *pType, int nTypes)
{
	CString	sFilter, s;
	for (int iType = 0; iType < nTypes; iType++) {
		const FILE_TYPE&	type = pType[iType];
		s.Format(_T("%s (*.%s)|*.%s|"), type.szName, type.szExt, type.szExt);
		sFilter += s;
	}
	sFilter += LDS(AFX_IDS_ALLFILTER) + _T("|*.*||");
	return sFilter;
}

int	CPotterDrawView::FindFileType(const FILE_TYPE *pType, int nTypes, LPCTSTR szExt)
{
	for (int iType = 0; iType < nTypes; iType++) {
		if (!_tcscmp(szExt, pType[iType].szExt))
			return iType;
	}
	return -1;
}

bool CPotterDrawView::ExportImage(LPCTSTR szPath, D3DXIMAGE_FILEFORMAT nFormat)
{
	if (theApp.m_Options.m_bCustomImageSize) {
		CSize	szImage(theApp.m_Options.m_nCustomImageWidth, theApp.m_Options.m_nCustomImageHeight);
		return m_Graphics.ExportImage(szPath, szImage, nFormat); 		
	} else
		return m_Graphics.ExportImage(szPath, nFormat);
}

bool CPotterDrawView::Record(bool bEnable)
{
	if (bEnable == IsRecording())	// if already in requested state
		return true;	// nothing to do
	CPotterDrawDoc	*pDoc = GetDocument();
	if (bEnable) {	// if starting
		CString	sFolder(theApp.GetProfileString(REG_SETTINGS, RK_RECORD_FOLDER));
		CString	sTitle(LPCTSTR(IDS_RECORD_FOLDER));
		if (!CFolderDialog::BrowseFolder(sTitle, sFolder, NULL, BIF_USENEWUI, sFolder, m_hWnd))
			return false;
		theApp.WriteProfileString(REG_SETTINGS, RK_RECORD_FOLDER, sFolder);
		CRecordDlg	dlg;
		dlg.m_fFrameRate = pDoc->m_fFrameRate;
		if (dlg.DoModal() != IDOK)
			return false;
		if (dlg.m_fFrameRate != pDoc->m_fFrameRate) {	// if frame rate changed
			int	iProp = CPotterDrawDoc::PROP_fFrameRate;
			pDoc->NotifyUndoableEdit(iProp, UCODE_PROPERTY);
			pDoc->m_fFrameRate = dlg.m_fFrameRate;
			pDoc->OnPropertyEdit(iProp);
		}
		m_sRecordFolder = sFolder;
		m_nRecordFramesDone = 0;
		m_nRecordDuration = dlg.m_nDurationFrames;
		m_iRecordFileFormat = dlg.m_iFileFormat;
		m_szRecordFrame = CSize(dlg.m_nFrameWidth, dlg.m_nFrameHeight);
		m_bPreRecordAutoRotate = m_bAutoRotate;	// save pre-record state of auto-rotate
		m_bPreRecordAnimation = pDoc->m_bAnimation;	// save pre-record state of animation
		m_bAutoRotate = dlg.m_bAutoRotate != 0;
		pDoc->m_bAnimation = dlg.m_bAnimation != 0;
	} else {	// stopping
		m_sRecordFolder.Empty();
		m_bAutoRotate = m_bPreRecordAutoRotate;	// restore pre-record state of auto-rotate
		pDoc->m_bAnimation = m_bPreRecordAnimation;	// restore pre-record state of animation
	}
	CMainFrame	*pMain = theApp.GetMainFrame();
	pMain->OnRecord(bEnable);
	theApp.GetMainFrame()->OnUpdate(NULL);	// update animation property
	UpdateAnimationEnable();
	return true;
}

void CPotterDrawView::StepAnimation(bool bForward)
{
	CPotterDrawDoc	*pDoc = GetDocument();
	pDoc->NotifyUndoableEdit(0, UCODE_STEP_ANIMATION, CUndoable::UE_COALESCE);
	double	fFrameRate = m_Graphics.m_fFrameRate;	// save frame rate
	if (!bForward)	// if stepping backward
		m_Graphics.m_fFrameRate = -fFrameRate;	// negate frame rate
	m_Graphics.UpdateAnimation();
	m_Graphics.m_fFrameRate = fFrameRate;	// restore frame rate
	RedrawWindow();	// redraw immediately, no lagging
	m_Graphics.GetAnimationState(*pDoc);	// update document's animation state
	theApp.GetMainFrame()->UpdateModulationBar(pDoc);	// update modulation bar
}

void CPotterDrawView::DumpFace(int iFace)
{
	fprintf(stdout, "iFace=%d\n", iFace);
	if (iFace >= 0) {
		CPotGraphics::CFace	face;
		m_Graphics.GetFace(iFace, face);
		for (int iFV = 0; iFV < 3; iFV++) {	// for each of face's vertices
			CPotGraphics::CVertex	vert;
			int	iVert = face.arrIdx[iFV];
			m_Graphics.GetVertex(iVert, vert);
			int	iWall, iRing, iSide;
			m_Graphics.GetVertexCoords(iVert, iWall, iRing, iSide);
			fprintf(stdout, "iVert=%d iWall=%d iRing=%d iSide=%d  t=%g, %g\n", 
				iVert, iWall, iRing, iSide, vert.t.x, vert.t.y);
			fprintf(stdout, "v=%g, %g, %g  n=%g, %g, %g\n", 
				vert.pt.x, vert.pt.y, vert.pt.z, vert.n.x, vert.n.y, vert.n.z);
		}
	}
}

// CPotterDrawView drawing

void CPotterDrawView::OnDraw(CDC *pDC)
{
	printf("view %Ix OnDraw\n", this);
//	CPotterDrawDoc* pDoc = GetDocument();
//	ASSERT_VALID(pDoc);
	if (m_Graphics.IsDeviceCreated()) {
		m_Graphics.Render();
	} else {
/*			
		CRect	cb;
		pDC->GetClipBox(cb);
		pDC->FillSolidRect(cb, GetSysColor(COLOR_WINDOW));
*/
	}
}

void CPotterDrawView::OnFilePrintPreview()
{
	AFXPrintPreview(this);
}

BOOL CPotterDrawView::OnPreparePrinting(CPrintInfo* pInfo)
{
	pInfo->SetMaxPage(1);	// one page only
	SetAnimation(false);	// suspend animation while printing
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CPotterDrawView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* pInfo)
{
}

void CPotterDrawView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* pInfo)
{
	m_dibPrint.Destroy();
	UpdateAnimationEnable();	// restore animation state
}

void CPotterDrawView::OnPrint(CDC* pDC, CPrintInfo* pInfo)
{
	printf("view %Ix OnPrint\n", this);
	CSize	szPrint(pInfo->m_rectDraw.Size());	// size of printable area
	CSize	szImage;
	if (m_dibPrint.IsEmpty()) {	// if print bitmap not created yet
		if (pInfo->m_bPreview) {	// if previewing
			CClientDC	dc(this);	// render at screen resolution to keep preview reasonably fast
			double	fScale = double(dc.GetDeviceCaps(LOGPIXELSX)) / pDC->GetDeviceCaps(LOGPIXELSX);
			szImage.cx = round(szPrint.cx * fScale);	// scale printable area to screen resolution
			szImage.cy = round(szPrint.cy * fScale);
		} else	// not previewing
			szImage = szPrint;	// render at printer's full resolution; likely to be slow
		CString	sImagePath;
		if (!theApp.GetTempFileName(sImagePath, _T("pdr")))
			AfxThrowResourceException();
		CWaitCursor	wc;
		if (!m_Graphics.ExportImage(sImagePath, szImage))	// export bitmap to temp file
			return;	// error already reported
		bool	bResult = m_dibPrint.Read(sImagePath);	// read bitmap into memory
		DeleteFile(sImagePath);	// clean up temp file
		if (!bResult)
			AfxThrowResourceException();
	} else {	// bitmap already created
		BITMAP	bmp;
		if (!m_dibPrint.GetBitmap(&bmp))
			AfxThrowResourceException();
		szImage.cx = bmp.bmWidth;	// get image size from bitmap
		szImage.cy = bmp.bmHeight;
	}
	CDC	dc;
	if (!dc.CreateCompatibleDC(pDC))
		AfxThrowResourceException();
	HGDIOBJ	hPrevBmp = dc.SelectObject(m_dibPrint);
	CPoint	ptPrint(pInfo->m_rectDraw.TopLeft());
	if (pInfo->m_bPreview) {	// if previewing
		pDC->SetStretchBltMode(HALFTONE);
		pDC->StretchBlt(ptPrint.x, ptPrint.y, szPrint.cx, szPrint.cy, 
			&dc, 0, 0, szImage.cx, szImage.cy, SRCCOPY);
	} else {	// not previewing
		ASSERT(szImage == szPrint);	// image size should match printable area
		pDC->BitBlt(ptPrint.x, ptPrint.y, szPrint.cx, szPrint.cy, &dc, 0, 0, SRCCOPY);
	}
	dc.SelectObject(hPrevBmp);	// restore DC's default bitmap
}

void CPotterDrawView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CPotterDrawView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
}

// CPotterDrawView diagnostics

#ifdef _DEBUG
void CPotterDrawView::AssertValid() const
{
	CView::AssertValid();
}

void CPotterDrawView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CPotterDrawDoc* CPotterDrawView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CPotterDrawDoc)));
	return (CPotterDrawDoc*)m_pDocument;
}
#endif //_DEBUG

// CPotterDrawView message map

BEGIN_MESSAGE_MAP(CPotterDrawView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_WM_TIMER()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_VIEW_ROTATE_ABOVE, OnViewRotateAbove)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ROTATE_ABOVE, OnUpdateViewRotateAbove)
	ON_COMMAND(ID_VIEW_ROTATE_FRONT, OnViewRotateFront)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ROTATE_FRONT, OnUpdateViewRotateFront)
	ON_COMMAND(ID_VIEW_ROTATE_TOP, OnViewRotateTop)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ROTATE_TOP, OnUpdateViewRotateTop)
	ON_COMMAND(ID_VIEW_ROTATE_LEFT, OnViewRotateLeft)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ROTATE_LEFT, OnUpdateViewRotateLeft)
	ON_COMMAND(ID_VIEW_ROTATE_BACK, OnViewRotateBack)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ROTATE_BACK, OnUpdateViewRotateBack)
	ON_COMMAND(ID_VIEW_ROTATE_BOTTOM, OnViewRotateBottom)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ROTATE_BOTTOM, OnUpdateViewRotateBottom)
	ON_COMMAND(ID_VIEW_ROTATE_RIGHT, OnViewRotateRight)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ROTATE_RIGHT, OnUpdateViewRotateRight)
	ON_COMMAND(ID_VIEW_ROTATE_AUTO, OnViewRotateAuto)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ROTATE_AUTO, OnUpdateViewRotateAuto)
	ON_COMMAND(ID_VIEW_ROTATE_RESET, OnViewRotateReset)
	ON_COMMAND(ID_VIEW_ROTATE_EDIT, OnViewRotateEdit)
	ON_COMMAND(ID_VIEW_ZOOM_IN, OnViewZoomIn)
	ON_COMMAND(ID_VIEW_ZOOM_OUT, OnViewZoomOut)
	ON_COMMAND(ID_VIEW_ZOOM_RESET, OnViewZoomReset)
	ON_COMMAND(ID_VIEW_PAN_UP, OnViewPanUp)
	ON_COMMAND(ID_VIEW_PAN_DOWN, OnViewPanDown)
	ON_COMMAND(ID_VIEW_PAN_LEFT, OnViewPanLeft)
	ON_COMMAND(ID_VIEW_PAN_RIGHT, OnViewPanRight)
	ON_COMMAND(ID_VIEW_PAN_EDIT, OnViewPanEdit)
	ON_COMMAND(ID_VIEW_ANIMATION, OnViewAnimation)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ANIMATION, OnUpdateViewAnimation)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_COMMAND(ID_RENDER_WIREFRAME, OnRenderWireframe)
	ON_UPDATE_COMMAND_UI(ID_RENDER_WIREFRAME, OnUpdateRenderWireframe)
	ON_COMMAND(ID_RENDER_GOURAUD, OnRenderGouraud)
	ON_UPDATE_COMMAND_UI(ID_RENDER_GOURAUD, OnUpdateRenderGouraud)
	ON_COMMAND(ID_RENDER_HIGHLIGHTS, OnRenderHighlights)
	ON_UPDATE_COMMAND_UI(ID_RENDER_HIGHLIGHTS, OnUpdateRenderHighlights)
	ON_COMMAND(ID_RENDER_CULLING, OnRenderCulling)
	ON_UPDATE_COMMAND_UI(ID_RENDER_CULLING, OnUpdateRenderCulling)
	ON_COMMAND(ID_RENDER_TEXTURE, OnRenderTexture)
	ON_UPDATE_COMMAND_UI(ID_RENDER_TEXTURE, OnUpdateRenderTexture)
	ON_COMMAND(ID_RENDER_TRANSPARENT, OnRenderTransparent)
	ON_UPDATE_COMMAND_UI(ID_RENDER_TRANSPARENT, OnUpdateRenderTransparent)
	ON_COMMAND(ID_RENDER_BOUNDS, OnRenderBounds)
	ON_UPDATE_COMMAND_UI(ID_RENDER_BOUNDS, OnUpdateRenderBounds)
	ON_MESSAGE(UWM_DEFERRED_UPDATE, OnDeferredUpdate)
	ON_MESSAGE(UWM_FRAME_TIMER, OnFrameTimer)
	ON_MESSAGE(UWM_SET_RECORD, OnSetRecord)
	ON_COMMAND(ID_FILE_EXPORT, OnFileExport)
	ON_COMMAND(ID_FILE_RECORD, OnFileRecord)
	ON_UPDATE_COMMAND_UI(ID_FILE_RECORD, OnUpdateFileRecord)
	ON_UPDATE_COMMAND_UI(ID_NEXT_PANE, OnUpdateNextPane)
	ON_UPDATE_COMMAND_UI(ID_PREV_PANE, OnUpdateNextPane)
	ON_COMMAND(ID_RENDER_LIGHTING, OnRenderLighting)
	ON_COMMAND(ID_TOOLS_MESH_INFO, OnToolsMeshInfo)
	ON_COMMAND(ID_PALETTE_IMPORT, OnPaletteImport)
	ON_COMMAND(ID_PALETTE_EXPORT, OnPaletteExport)
	ON_COMMAND(ID_VIEW_STEP_FORWARD, OnViewStepForward)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STEP_FORWARD, OnUpdateViewStepForward)
	ON_COMMAND(ID_VIEW_STEP_BACKWARD, OnViewStepBackward)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STEP_BACKWARD, OnUpdateViewStepBackward)
	ON_UPDATE_COMMAND_UI(ID_VIEW_RANDOM_PHASE, OnUpdateViewRandomPhase)
END_MESSAGE_MAP()

// CPotterDrawView message handlers

LRESULT CPotterDrawView::OnDeferredUpdate(WPARAM wParam, LPARAM lParam)
{
	printf("view %Ix OnDeferredUpdate\n", this);
	theApp.GetMainFrame()->SetDeferredUpdate(false);
	OnUpdate(NULL, 0, NULL);
	return 0;
}

void CPotterDrawView::OnSize(UINT nType, int cx, int cy)
{
	printf("view %Ix OnSize nType=%d cx=%d cy=%d\n", this, nType, cx, cy);
	CView::OnSize(nType, cx, cy);
	if (!theApp.GetMainFrame()->GetDeferredSizing()) {
		if (m_Graphics.IsDeviceCreated() && cx && cy)
			m_Graphics.Resize(CSize(cx, cy));
	}
}

void CPotterDrawView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	printf("view %Ix OnActivateView\n", this);
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
	if (bActivate)	// if activating
		Invalidate();	// force render
}

BOOL CPotterDrawView::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;	// don't erase background else redraw flickers
//	return CView::OnEraseBkgnd(pDC);
}

void CPotterDrawView::OnUpdateNextPane(CCmdUI* pCmdUI)
{
	// override view's splitter implementation; main frame handles next/prev pane commands
}

void CPotterDrawView::OnLButtonDown(UINT nFlags, CPoint point)
{
#if POT_GFX_SHOW_FACE_SELECTION	// if face selection enabled
	if (GetKeyState(VK_MENU) & GKS_DOWN) {	// if menu key down
		AfxGetMainWnd()->PostMessage(WM_SYSKEYUP, VK_MENU);	// cancel menu state
		int	iFace = m_Graphics.HitTest(point);
		CIntArrayEx	arrFaceIdx;
		if (iFace >= 0) {	// if face hit
			m_Graphics.GetSelection(arrFaceIdx);	// get current selection
			int	nSels = arrFaceIdx.GetSize();
			int iSel;
			for (iSel = 0; iSel < nSels; iSel++) {	// for each selected face
				if (arrFaceIdx[iSel] == iFace)	// if hit face found
					break;
			}
			if (nFlags & MK_CONTROL) {	// if control key also down
				if (iSel < nSels)	// if hit face found in selection
					arrFaceIdx.RemoveAt(iSel);	// remove hit face from selection
				else	// hit face not found
					arrFaceIdx.Add(iFace);	// add hit face to selection
			} else {	// control key up
				arrFaceIdx.SetSize(1);	// set selection to hit face
				arrFaceIdx[0] = iFace;
			}
		}
		m_Graphics.SetSelection(arrFaceIdx);
		Invalidate();
		DumpFace(iFace);
		return;
	}
#endif	// POT_GFX_SHOW_FACE_SELECTION
	if (nFlags & MK_CONTROL) {	// if control key down
		HCURSOR hCur = theApp.LoadCursor(IDC_CURSOR_MAGNIFIER);
		SetCursor(hCur);
		CRectTracker	trk;
		if (trk.TrackRubberBand(this, point)) {	// if tracker rectangle created
			trk.m_rect.NormalizeRect();
			m_Graphics.ZoomRect(trk.m_rect);	// zoom into specified rectangle
			Invalidate();
		}
	} else {	// normal case
		StartDrag(nFlags, point, false);	// rotate
	}
	CView::OnLButtonDown(nFlags, point);
}

void CPotterDrawView::OnLButtonUp(UINT nFlags, CPoint point)
{
	EndDrag();
	CView::OnLButtonUp(nFlags, point);
}

void CPotterDrawView::OnMButtonDown(UINT nFlags, CPoint point)
{
	StartDrag(nFlags, point, true);	// pan
	CView::OnMButtonDown(nFlags, point);
}

void CPotterDrawView::OnMButtonUp(UINT nFlags, CPoint point)
{
	EndDrag();
	CView::OnMButtonUp(nFlags, point);
}

void CPotterDrawView::OnMouseMove(UINT nFlags, CPoint point)
{
	switch (m_nDragState) {
	case DS_ROTATE:
		{
			bool	bRoll = (nFlags & MK_SHIFT) != 0;
			if (bRoll != m_bDragRoll) {	// if drag roll state changed
				m_bDragRoll = bRoll;
				m_ptDragOrigin = point;	// update origin
				m_vDragRotation = m_Graphics.GetRotation(); // update rotation
			}
			CPoint	ptDelta(m_ptDragOrigin - point);
			D3DXVECTOR3	vRotation(m_vDragRotation);
			double	fScale = theApp.m_Options.m_fDragRotateStep / 180 * M_PI;	// degrees to radians
			if (bRoll) {	// if rolling
				vRotation.z += float(ptDelta.x * fScale);	// mouse x rotates z; mouse y unused
			} else {	// not rolling
				vRotation.y += float(ptDelta.x * fScale);	// mouse x rotates y
				vRotation.x += float(ptDelta.y * fScale);	// mouse y rotates x
			}
			m_Graphics.SetRotation(vRotation);
			Invalidate();
		}
		break;
	case DS_PAN:
		m_Graphics.SetPan(m_vDragPan, m_ptDragOrigin, point);
		Invalidate();
		break;
	}
	CView::OnMouseMove(nFlags, point);
}

BOOL CPotterDrawView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	CPoint	ptClient(pt);
	ScreenToClient(&ptClient);
	CRect	rc(CPoint(0, 0), m_Graphics.GetSize());
	if (rc.PtInRect(ptClient)) {	// if cursor within view
		double	fStep = abs(zDelta) * CalcZoomStep() / WHEEL_DELTA;
		double	fZoom;
		if (zDelta > 0)	// if zooming in
			fZoom = m_Graphics.GetZoom() * fStep;
		else	// zooming out
			fZoom = m_Graphics.GetZoom() / fStep;
		m_Graphics.SetZoom(ptClient, fZoom);
		Invalidate();
	}
	return 0;
//	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

LRESULT CPotterDrawView::OnFrameTimer(WPARAM wParam, LPARAM lParam)
{
	m_bInFrameTimer = true;
	if (GetDocument()->m_bAnimation) {
		m_Graphics.UpdateAnimation();
		COscilloscopeBar&	wndOscBar = theApp.GetMainFrame()->GetOscilloscopeBar();
		if (wndOscBar.FastIsVisible()) {	// if oscilloscope bar is visible
			if (theApp.GetMainFrame()->GetActiveMDIView() == this) {	// if we're active view
				CPotterDrawDoc	*pDoc = GetDocument();
				if (wndOscBar.GetShowAllModulations()) {	// if showing all modulations
					// if at least one modulated property is animated
					if (m_Graphics.GetModulationState() & CPotProperties::MOD_ANIMATED)
						wndOscBar.Update(false);	// don't refit to data to avoid ruler flicker
				} else {	// showing current modulation target only
					int	iProp = pDoc->m_iModTarget;
					if (iProp >= 0) {	// if modulation target is valid property
						if (pDoc->m_bIsPlotAnimated)	// if target property requires animated plot
							wndOscBar.Update(false);	// don't refit to data to avoid ruler flicker
					}
				}
			}
		}
	}
	if (m_bAutoRotate) {
		m_Graphics.SetRotation(m_Graphics.GetRotation() + m_vAutoRotateSpeed);
		if (m_fAutoZoomScale)	// if auto zooming
			m_Graphics.SetZoom(m_Graphics.GetZoom() * m_fAutoZoomScale);
	}
	Invalidate();
	m_nBenchFrames++;
	if (IsRecording()) {
		struct FILE_FORMAT {
			LPCTSTR	szExt;
			D3DXIMAGE_FILEFORMAT	iFormat;
		};
		static const FILE_FORMAT	arrFileFormat[] = {
			{_T("bmp"),		D3DXIFF_BMP},
			{_T("png"),		D3DXIFF_PNG},
		};
		ASSERT(m_iRecordFileFormat >= 0 && m_iRecordFileFormat < _countof(arrFileFormat));
		const FILE_FORMAT&	FileFormat = arrFileFormat[m_iRecordFileFormat];
		CString	sFileName;
		sFileName.Format(_T("img%04d.%s"), m_nRecordFramesDone, FileFormat.szExt);
		CPathStr	sPath(m_sRecordFolder);
		sPath.Append(sFileName);
		UpdateWindow();
		m_Graphics.ExportImage(sPath, m_szRecordFrame, FileFormat.iFormat);
		m_nRecordFramesDone++;
		if (m_nRecordFramesDone >= m_nRecordDuration)
			Record(false);
	}
	m_bInFrameTimer = false;
	return 0;
}

LRESULT CPotterDrawView::OnSetRecord(WPARAM wParam, LPARAM lParam)
{
	Record(wParam != 0);
	return 0;
}

void CPotterDrawView::OnFileExport()
{
	enum {	// export types
		ET_BITMAP,
		ET_OBJ,
		ET_PLY,
		ET_PNG,
		ET_STL,
		EXPORT_TYPES
	};
	static const FILE_TYPE	arrExportType[EXPORT_TYPES] = {
		{_T("Bitmap"),	_T("bmp")},
		{_T("OBJ"),		_T("obj")},
		{_T("PLY"),		_T("ply")},
		{_T("PNG"),		_T("png")},
		{_T("STL"),		_T("stl")},
	};
	const int nExportTypes = _countof(arrExportType);
	CString	sPrevExt(theApp.GetProfileString(REG_SETTINGS, RK_EXPORT_TYPE, _T("")));
	int	iPrevType = ET_BITMAP;	// default export type
	if (!sPrevExt.IsEmpty()) {
		int	iType = FindFileType(arrExportType, nExportTypes, sPrevExt);
		if (iType >= 0)	// if previous extension found
			iPrevType = iType;
	}
	CString	sFilter(MakeFileFilter(arrExportType, nExportTypes));
	CPathStr	sDefFileName;
	if (!GetDocument()->GetPathName().IsEmpty()) {	// if document was saved
		sDefFileName = GetDocument()->GetTitle();	// use document title as default filename
		sDefFileName.RemoveExtension();	// without file extension
	}
	CFileDialog	fd(false, arrExportType[iPrevType].szExt, sDefFileName, OFN_OVERWRITEPROMPT, sFilter);
	fd.m_ofn.nFilterIndex = iPrevType + 1;	// zero is for custom filter
	CString	sTitle(LPCTSTR(IDS_EXPORT));
	fd.m_ofn.lpstrTitle = sTitle;
	CWaitCursor	wc;
	if (fd.DoModal() == IDOK) {
		wc.Restore();
		CString	sExt(fd.GetFileExt());
		CString	sPath(fd.GetPathName());
		int	iType = FindFileType(arrExportType, nExportTypes, sExt);
		bool	bResult = false;
		switch (iType) {
		case ET_BITMAP:
			bResult = ExportImage(sPath);
			break;
		case ET_OBJ:
			bResult = m_Graphics.ExportOBJ(sPath, theApp.m_Options.m_nFloatPrecision);
			break;
		case ET_PLY:
			bResult = m_Graphics.ExportPLY(sPath, theApp.m_Options.m_bVertexColor);
			break;
		case ET_PNG:
			bResult = ExportImage(sPath, D3DXIFF_PNG);
			break;
		case ET_STL:
			bResult = m_Graphics.ExportSTL(sPath);
			break;
		default:
			AfxMessageBox(IDS_ERR_BAD_EXPORT_TYPE);
			sExt.Empty();
		}
		theApp.WriteProfileString(REG_SETTINGS, RK_EXPORT_TYPE, sExt);
		if (!bResult) {
			CString	sMsg(LPCTSTR(IDS_ERR_EXPORT_FAILED));
			if (m_Graphics.GetLastError())
				sMsg += '\n' + m_Graphics.GetLastErrorName();
			if (GetLastError())
				sMsg += '\n' + GetLastErrorString();
			AfxMessageBox(sMsg);
		}
	}
}

void CPotterDrawView::OnFileRecord()
{
	Record(!IsRecording());
}

void CPotterDrawView::OnUpdateFileRecord(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(!m_sRecordFolder.IsEmpty());
}

void CPotterDrawView::OnViewRotateAbove()
{
	SetStandardView(CD3DGraphics::SV_ABOVE);
}

void CPotterDrawView::OnUpdateViewRotateAbove(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(m_Graphics.GetStandardView() == CD3DGraphics::SV_ABOVE);
}

void CPotterDrawView::OnViewRotateFront()
{
	SetStandardView(CD3DGraphics::SV_FRONT);
}

void CPotterDrawView::OnUpdateViewRotateFront(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(m_Graphics.GetStandardView() == CD3DGraphics::SV_FRONT);
}

void CPotterDrawView::OnViewRotateTop()
{
	SetStandardView(CD3DGraphics::SV_TOP);
}

void CPotterDrawView::OnUpdateViewRotateTop(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(m_Graphics.GetStandardView() == CD3DGraphics::SV_TOP);
}

void CPotterDrawView::OnViewRotateLeft()
{
	SetStandardView(CD3DGraphics::SV_LEFT);
}

void CPotterDrawView::OnUpdateViewRotateLeft(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(m_Graphics.GetStandardView() == CD3DGraphics::SV_LEFT);
}

void CPotterDrawView::OnViewRotateBack()
{
	SetStandardView(CD3DGraphics::SV_BACK);
}

void CPotterDrawView::OnUpdateViewRotateBack(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(m_Graphics.GetStandardView() == CD3DGraphics::SV_BACK);
}

void CPotterDrawView::OnViewRotateBottom()
{
	SetStandardView(CD3DGraphics::SV_BOTTOM);
}

void CPotterDrawView::OnUpdateViewRotateBottom(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(m_Graphics.GetStandardView() == CD3DGraphics::SV_BOTTOM);
}

void CPotterDrawView::OnViewRotateRight()
{
	SetStandardView(CD3DGraphics::SV_RIGHT);
}

void CPotterDrawView::OnUpdateViewRotateRight(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(m_Graphics.GetStandardView() == CD3DGraphics::SV_RIGHT);
}

void CPotterDrawView::OnViewRotateAuto()
{
	m_bAutoRotate ^= 1;
	UpdateAnimationEnable();
}

void CPotterDrawView::OnUpdateViewRotateAuto(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bAutoRotate);
}

void CPotterDrawView::OnViewRotateReset()
{
	m_Graphics.SetStandardView(m_Graphics.GetStandardView());	// apply initial rotation for current view type
	m_Graphics.SetPan(D3DXVECTOR3(0, 0, 0));
	Invalidate();
}

void CPotterDrawView::OnViewRotateEdit()
{
	CRotateDlg	dlg;
	dlg.m_vRotation = m_Graphics.GetRotation() / float(M_PI / 180);
	if (dlg.DoModal() == IDOK)
		m_Graphics.SetRotation(dlg.m_vRotation * float(M_PI / 180));
}

void CPotterDrawView::OnViewZoomIn()
{
	m_Graphics.SetZoom(m_Graphics.GetZoom() * CalcZoomStep());
	Invalidate();
}

void CPotterDrawView::OnViewZoomOut()
{
	m_Graphics.SetZoom(m_Graphics.GetZoom() / CalcZoomStep());
	Invalidate();
}

void CPotterDrawView::OnViewZoomReset()
{
	m_Graphics.SetZoom(1);
	Invalidate();
}

void CPotterDrawView::OnViewPanUp()
{
	D3DXVECTOR3	org(m_Graphics.GetPan());
	org.y -= CalcPanStep();
	m_Graphics.SetPan(org);
	Invalidate();
}

void CPotterDrawView::OnViewPanDown()
{
	D3DXVECTOR3	org(m_Graphics.GetPan());
	org.y += CalcPanStep();
	m_Graphics.SetPan(org);
	Invalidate();
}

void CPotterDrawView::OnViewPanLeft()
{
	D3DXVECTOR3	org(m_Graphics.GetPan());
	org.x += CalcPanStep();
	m_Graphics.SetPan(org);
	Invalidate();
}

void CPotterDrawView::OnViewPanRight()
{
	D3DXVECTOR3	org(m_Graphics.GetPan());
	org.x -= CalcPanStep();
	m_Graphics.SetPan(org);
	Invalidate();
}

void CPotterDrawView::OnViewPanEdit()
{
	CRotateDlg	dlg;
	dlg.m_vRotation = m_Graphics.GetPan() / float(M_PI / 180);
	dlg.m_sCaption.LoadString(IDS_VIEW_PAN);
	if (dlg.DoModal() == IDOK)
		m_Graphics.SetPan(dlg.m_vRotation * float(M_PI / 180));
}

void CPotterDrawView::OnViewAnimation()
{
	CPotterDrawDoc	*pDoc = GetDocument();
	pDoc->NotifyUndoableEdit(CPotterDrawDoc::PROP_bAnimation, UCODE_PROPERTY);
	pDoc->m_bAnimation ^= 1;
	pDoc->OnPropertyEdit(CPotterDrawDoc::PROP_bAnimation);
}

void CPotterDrawView::OnUpdateViewAnimation(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(GetDocument()->m_bAnimation);
}

void CPotterDrawView::OnViewStepForward()
{
	StepAnimation(true);	// forward
}

void CPotterDrawView::OnUpdateViewStepForward(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!IsAnimating() && (m_Graphics.GetModulationState() & CPotProperties::MOD_ANIMATED));
}

void CPotterDrawView::OnViewStepBackward()
{
	StepAnimation(false);	// backward
}

void CPotterDrawView::OnUpdateViewStepBackward(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!IsAnimating() && (m_Graphics.GetModulationState() & CPotProperties::MOD_ANIMATED));
}

void CPotterDrawView::OnUpdateViewRandomPhase(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_Graphics.GetModulationState() & CPotProperties::MOD_ANIMATED);
}

void CPotterDrawView::OnRenderWireframe()
{
	m_Graphics.SetStyle(m_Graphics.GetStyle() ^ CD3DGraphics::ST_WIREFRAME);
	Invalidate();
}

void CPotterDrawView::OnUpdateRenderWireframe(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_Graphics.GetStyle() & CD3DGraphics::ST_WIREFRAME) != 0);
}

void CPotterDrawView::OnRenderGouraud()
{
	m_Graphics.SetStyle(m_Graphics.GetStyle() ^ CD3DGraphics::ST_GOURAUD);
	Invalidate();
}

void CPotterDrawView::OnUpdateRenderGouraud(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_Graphics.GetStyle() & CD3DGraphics::ST_GOURAUD) != 0);
}

void CPotterDrawView::OnRenderHighlights()
{
	m_Graphics.SetStyle(m_Graphics.GetStyle() ^ CD3DGraphics::ST_HIGHLIGHTS);
	Invalidate();
}

void CPotterDrawView::OnUpdateRenderHighlights(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_Graphics.GetStyle() & CD3DGraphics::ST_HIGHLIGHTS) != 0);
}

void CPotterDrawView::OnRenderCulling()
{
	m_Graphics.SetStyle(m_Graphics.GetStyle() ^ CD3DGraphics::ST_CULLING);
	Invalidate();
}

void CPotterDrawView::OnUpdateRenderCulling(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_Graphics.GetStyle() & CD3DGraphics::ST_CULLING) != 0);
}

void CPotterDrawView::OnRenderTexture()
{
	m_Graphics.SetStyle(m_Graphics.GetStyle() ^ CPotGraphics::ST_TEXTURE);
	Invalidate();
}

void CPotterDrawView::OnUpdateRenderTexture(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_Graphics.GetStyle() & CPotGraphics::ST_TEXTURE) != 0);
}

void CPotterDrawView::OnRenderTransparent()
{
	m_Graphics.SetStyle(m_Graphics.GetStyle() ^ CPotGraphics::ST_TRANSPARENT);
	Invalidate();
}

void CPotterDrawView::OnUpdateRenderTransparent(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_Graphics.GetStyle() & CPotGraphics::ST_TRANSPARENT) != 0);
}

void CPotterDrawView::OnRenderBounds()
{
	m_Graphics.SetStyle(m_Graphics.GetStyle() ^ CPotGraphics::ST_BOUNDS);
	Invalidate();
}

void CPotterDrawView::OnUpdateRenderBounds(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_Graphics.GetStyle() & CPotGraphics::ST_BOUNDS) != 0);
}

void CPotterDrawView::OnRenderLighting()
{
	CPotterDrawDoc	*pDoc = GetDocument();
	CLightingDlg	dlg;
	dlg.m_vDir = pDoc->m_vLightDir;
	dlg.m_fDiffuse = pDoc->m_mtrlPot.Diffuse.r;
	dlg.m_fAmbient = pDoc->m_mtrlPot.Ambient.r;
	dlg.m_fSpecular = pDoc->m_mtrlPot.Specular.r;
	dlg.m_fPower = pDoc->m_mtrlPot.Power;
	if (dlg.DoModal() == IDOK) {
		pDoc->NotifyUndoableEdit(0, UCODE_LIGHTING);
		pDoc->m_vLightDir = dlg.m_vDir;
		pDoc->m_mtrlPot.Diffuse = D3DXCOLOR(dlg.m_fDiffuse, dlg.m_fDiffuse, dlg.m_fDiffuse, 1.0f);
		pDoc->m_mtrlPot.Ambient = D3DXCOLOR(dlg.m_fAmbient, dlg.m_fAmbient, dlg.m_fAmbient, 1.0f);
		pDoc->m_mtrlPot.Specular = D3DXCOLOR(dlg.m_fSpecular, dlg.m_fSpecular, dlg.m_fSpecular, 1.0f);
		pDoc->m_mtrlPot.Power = dlg.m_fPower;
		pDoc->UpdateAllViews(NULL, CPotterDrawDoc::HINT_LIGHTING);
		pDoc->SetModifiedFlag();
	}
}

void CPotterDrawView::OnToolsMeshInfo()
{
	D3DXVECTOR3	p1, p2;
	m_Graphics.ComputeBounds(p1, p2);
	D3DXVec3Subtract(&p1, &p1, &p2);
	D3DXVECTOR3	vSize(fabs(p1.x), fabs(p1.y), fabs(p1.z));
	CString	sMsg;
	sMsg.Format(IDS_MESH_INFO_FORMAT, m_Graphics.GetVertexCount(), 
		m_Graphics.GetFaceCount(), vSize.x, vSize.y, vSize.z);
	CPotterDrawDoc	*pDoc = GetDocument();
	CString	sWarn;
	if (pDoc->m_arrSpline.GetSize()) {	// if spline has at least one segment
		if (!m_Graphics.IsSplinePositiveInX())	// if spline has negative x-coords
			sWarn += LDS(IDS_ERR_SPLINE_NEGATIVE_X) + '\n';
		if (!pDoc->m_arrSpline.HasDuplicateY())	// if spline has duplicate y-coords
			sWarn += LDS(IDS_ERR_SPLINE_DUPLICATE_Y) + '\n';
	} else	// spline has no segments
		sWarn += LDS(IDS_ERR_SPLINE_EMPTY) + '\n';
	UINT	nType;
	if (!sWarn.IsEmpty()) {	// if at least one warning
		sMsg.Insert(0, sWarn + '\n');
		nType = MB_ICONWARNING;
	} else	// no warnings
		nType = MB_ICONINFORMATION;
	AfxMessageBox(sMsg, nType);	// display message box
}

void CPotterDrawView::OnPaletteImport()
{
	CString	sFilter(LPCTSTR(IDS_PALETTE_FILES_FILTER));
	CFileDialog	fd(true, _T(".pal"), NULL, OFN_HIDEREADONLY, sFilter);
	if (fd.DoModal() == IDOK) {
		CStdioFile	fp(fd.GetPathName(), CFile::modeRead);
		CString	s;
		CArrayEx<COLORREF, COLORREF>	arrPalette;
		while (fp.ReadString(s)) {
			int	r, g, b;
			if (_stscanf_s(s, _T("%d %d %d"), &r, &g, &b) == 3) {
				arrPalette.Add(RGB(r, g, b));
			}
		}
		if (arrPalette.GetSize()) {	// if palette non-empty
			CPotterDrawDoc	*pDoc = GetDocument();
			pDoc->NotifyUndoableEdit(-1, UCODE_PALETTE);	// entire palette changed; size may have changed
			pDoc->m_arrPalette = arrPalette;
			pDoc->m_nColors = arrPalette.GetSize();
			pDoc->UpdateAllViews(NULL, CPotterDrawDoc::HINT_PALETTE);
			pDoc->SetModifiedFlag();
		}
	}
}

void CPotterDrawView::OnPaletteExport()
{
	CString	sFilter(LPCTSTR(IDS_PALETTE_FILES_FILTER));
	CFileDialog	fd(false, _T(".pal"), NULL, OFN_OVERWRITEPROMPT, sFilter);
	if (fd.DoModal() == IDOK) {
		CStdioFile	fp(fd.GetPathName(), CFile::modeCreate | CFile::modeWrite);
		const CPotterDrawDoc	*pDoc = GetDocument();
		CString	s;
		int	nColors = pDoc->m_arrPalette.GetSize();
		for (int iColor = 0; iColor < nColors; iColor++) {	// for each palette color
			COLORREF	c = pDoc->m_arrPalette[iColor];
			s.Format(_T("%d\t%d\t%d\n"), GetRValue(c), GetGValue(c), GetBValue(c));
			fp.WriteString(s);
		}
	}
}
