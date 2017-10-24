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
	printf("CPotterDrawView::CreateDevice this=%Ix cy=%d cx=%d\n", this, szClient.cx, szClient.cy);
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
	printf("CPotterDrawView::OnInitialUpdate this=%Ix\n", this);
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
	printf("CPotterDrawView::OnUpdate %Ix %Id %Ix\n", pSender, lHint, pHint);
	CPotterDrawDoc	*pDoc = GetDocument();
	if (m_Graphics.m_bAnimation) {	// if currently animating texture
		bool	bSync;
		if (lHint == CPotterDrawDoc::HINT_MODULATION) {	// if modulation edit
			ASSERT(pHint != NULL);
			CPotterDrawDoc::CModulationHint	*pModHint = static_cast<CPotterDrawDoc::CModulationHint *>(pHint);
			bSync = pModHint->m_iModProp != CModulationProps::PROP_fPhase;	// don't synchronize for phase edit
		} else	// not modulation edit
			bSync = true;
		if (bSync) {	// if synchronization needed
			UpdateWindow();	// synchronize window with animation state
			CPotProperties&	props = *pDoc;
			m_Graphics.GetAnimationState(props);	// update document's animation state
		}
	}
	if (!m_Graphics.IsDeviceCreated()) {	// if device not created
		m_Graphics.SetProperties(*pDoc);	// update graphics properties from document
		theApp.GetMainFrame()->OnUpdate(pSender, lHint, pHint);	// relay update to main frame
		m_Graphics.CalcSpline(pDoc->m_arrSpline);
		CreateDevice();
	} else {	// normal case: device created
		bool	bMakeMesh = false;
		bool	bMakeTexture = false;
		bool	bMakeSpline = false;
		switch (lHint) {
		case CPotterDrawDoc::HINT_PROPERTY:
			{
				ASSERT(pHint != NULL);
				CPotterDrawDoc::CPropertyHint	*pPropHint = static_cast<CPotterDrawDoc::CPropertyHint *>(pHint);
				int	iProp = pPropHint->m_iProp;
				int	iGroup = CPotProperties::m_Info[iProp].iGroup;
				switch (iGroup) {
				case CPotProperties::GROUP_MESH:
					switch (iProp) {
					case CPotProperties::PROP_nRings:
						bMakeMesh = true;
						bMakeSpline = true;
						break;
					case CPotProperties::PROP_fScallops:
					case CPotProperties::PROP_fScallopDepth:
					case CPotProperties::PROP_iScallopMotif:
					case CPotProperties::PROP_fScallopPhase:
					case CPotProperties::PROP_iScallopRange:
					case CPotProperties::PROP_fScallopPower:
					case CPotProperties::PROP_iScallopOperation:
						bMakeMesh = pDoc->HasScallops() || m_Graphics.HasScallops();
						break;
					case CPotProperties::PROP_fRipples:
					case CPotProperties::PROP_fRippleDepth:
					case CPotProperties::PROP_fRipplePhase:
					case CPotProperties::PROP_iRippleMotif:
						bMakeMesh = pDoc->HasRipples() || m_Graphics.HasRipples();
						break;
					case CPotProperties::PROP_fBends:
					case CPotProperties::PROP_fBendDepth:
					case CPotProperties::PROP_fBendPhase:
					case CPotProperties::PROP_iBendMotif:
					case CPotProperties::PROP_fBendPoles:
					case CPotProperties::PROP_fBendPolePhase:
					case CPotProperties::PROP_iBendPoleMotif:
						bMakeMesh = pDoc->HasBends() || m_Graphics.HasBends();
						break;
					case CPotProperties::PROP_fHelixFrequency:
					case CPotProperties::PROP_fHelixAmplitude:
						bMakeMesh = pDoc->HasHelix() || m_Graphics.HasHelix();
						break;
					default:
						bMakeMesh = true;
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
					}
					break;
				case CPotProperties::GROUP_VIEW:
					switch (iProp) {
					case CPotProperties::PROP_fAutoRotateYaw:
					case CPotProperties::PROP_fAutoRotatePitch:
					case CPotProperties::PROP_fAutoRotateRoll:
						UpdateAutoRotateSpeed();
						break;
					case CPotProperties::PROP_fFrameRate:
						UpdateAutoRotateSpeed();
						if (IsAnimating()) {
							SetAnimation(false);
							SetAnimation(true);
						}
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
				bMakeMesh = pDoc->IsModulated(iTarget) || m_Graphics.IsModulated(iTarget);
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
		default:	// no hint
			bMakeMesh = true;
			bMakeSpline = true;
			UpdateAutoRotateSpeed();
		}
		m_Graphics.SetProperties(*pDoc);	// update graphics properties from document
		theApp.GetMainFrame()->OnUpdate(pSender, lHint, pHint);	// relay update to main frame
		if (bMakeSpline)
			m_Graphics.CalcSpline(pDoc->m_arrSpline);
		if (bMakeMesh) {	// if mesh is stale
			CWaitCursor wc;	// mesh generation can be slow
			m_Graphics.MakeMesh(false);
		} else {	// reusing mesh
			if (bMakeTexture)	// if texture is stale
				m_Graphics.MakeTexture();
			m_Graphics.UpdateTexture();
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

// CPotterDrawView drawing

void CPotterDrawView::OnDraw(CDC *pDC)
{
	printf("CPotterDrawView::OnDraw this=%Ix\n", this);
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
	printf("CPotterDrawView::OnPrint this=%Ix\n", this);
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
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_COMMAND(ID_VIEW_STYLE_WIREFRAME, OnViewStyleWireframe)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STYLE_WIREFRAME, OnUpdateViewStyleWireframe)
	ON_COMMAND(ID_VIEW_STYLE_GOURAUD, OnViewStyleGouraud)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STYLE_GOURAUD, OnUpdateViewStyleGouraud)
	ON_COMMAND(ID_VIEW_STYLE_HIGHLIGHTS, OnViewStyleHighlights)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STYLE_HIGHLIGHTS, OnUpdateViewStyleHighlights)
	ON_COMMAND(ID_VIEW_STYLE_CULLING, OnViewStyleCulling)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STYLE_CULLING, OnUpdateViewStyleCulling)
	ON_COMMAND(ID_VIEW_STYLE_TEXTURE, OnViewStyleTexture)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STYLE_TEXTURE, OnUpdateViewStyleTexture)
	ON_COMMAND(ID_VIEW_STYLE_ANIMATION, OnViewStyleAnimation)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STYLE_ANIMATION, OnUpdateViewStyleAnimation)
	ON_COMMAND(ID_VIEW_STYLE_BOUNDS, OnViewStyleBounds)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STYLE_BOUNDS, OnUpdateViewStyleBounds)
	ON_MESSAGE(UWM_DEFERRED_UPDATE, OnDeferredUpdate)
	ON_MESSAGE(UWM_FRAME_TIMER, OnFrameTimer)
	ON_MESSAGE(UWM_SET_RECORD, OnSetRecord)
	ON_COMMAND(ID_FILE_EXPORT, OnFileExport)
	ON_COMMAND(ID_FILE_RECORD, OnFileRecord)
	ON_UPDATE_COMMAND_UI(ID_FILE_RECORD, OnUpdateFileRecord)
	ON_UPDATE_COMMAND_UI(ID_NEXT_PANE, OnUpdateNextPane)
	ON_UPDATE_COMMAND_UI(ID_PREV_PANE, OnUpdateNextPane)
	ON_COMMAND(ID_TOOLS_MESH_INFO, OnToolsMeshInfo)
END_MESSAGE_MAP()

// CPotterDrawView message handlers

LRESULT CPotterDrawView::OnDeferredUpdate(WPARAM wParam, LPARAM lParam)
{
	printf("CPotterDrawView::OnDeferredUpdate\n");
	theApp.GetMainFrame()->SetDeferredUpdate(false);
	OnUpdate(NULL, 0, NULL);
	return 0;
}

void CPotterDrawView::OnSize(UINT nType, int cx, int cy)
{
	printf("CPotterDrawView::OnSize this=%Ix nType=%d cx=%d cy=%d\n", this, nType, cx, cy);
	CView::OnSize(nType, cx, cy);
	if (!theApp.GetMainFrame()->GetDeferredSizing()) {
		if (m_Graphics.IsDeviceCreated() && cx && cy)
			m_Graphics.Resize(CSize(cx, cy));
	}
}

void CPotterDrawView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	printf("CPotterDrawView::OnActivateView this=%Ix\n", this);
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
					if (pDoc->HasAnimatedModulations())
						wndOscBar.Update(false);	// don't refit to data to avoid ruler flicker
				} else {	// showing current modulation target only
					int	iProp = pDoc->m_iModTarget;
					if (iProp >= 0) {	// if modulation target is valid property
						const CModulationProps&	mod = pDoc->m_Mod[iProp];
						// if target property is modulated and modulation is animated
						if (mod.IsModulated() && mod.IsAnimated())
							wndOscBar.Update(false);	// don't refit to data to avoid ruler flicker
					}
				}
			}
		}
	}
	if (m_bAutoRotate) {
		m_Graphics.SetRotation(m_Graphics.GetRotation() + m_vAutoRotateSpeed);
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

void CPotterDrawView::OnViewStyleWireframe()
{
	m_Graphics.SetStyle(m_Graphics.GetStyle() ^ CD3DGraphics::ST_WIREFRAME);
	Invalidate();
}

void CPotterDrawView::OnUpdateViewStyleWireframe(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_Graphics.GetStyle() & CD3DGraphics::ST_WIREFRAME) != 0);
}

void CPotterDrawView::OnViewStyleGouraud()
{
	m_Graphics.SetStyle(m_Graphics.GetStyle() ^ CD3DGraphics::ST_GOURAUD);
	Invalidate();
}

void CPotterDrawView::OnUpdateViewStyleGouraud(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_Graphics.GetStyle() & CD3DGraphics::ST_GOURAUD) != 0);
}

void CPotterDrawView::OnViewStyleHighlights()
{
	m_Graphics.SetStyle(m_Graphics.GetStyle() ^ CD3DGraphics::ST_HIGHLIGHTS);
	Invalidate();
}

void CPotterDrawView::OnUpdateViewStyleHighlights(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_Graphics.GetStyle() & CD3DGraphics::ST_HIGHLIGHTS) != 0);
}

void CPotterDrawView::OnViewStyleCulling()
{
	m_Graphics.SetStyle(m_Graphics.GetStyle() ^ CD3DGraphics::ST_CULLING);
	Invalidate();
}

void CPotterDrawView::OnUpdateViewStyleCulling(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_Graphics.GetStyle() & CD3DGraphics::ST_CULLING) != 0);
}

void CPotterDrawView::OnViewStyleTexture()
{
	m_Graphics.SetStyle(m_Graphics.GetStyle() ^ CPotGraphics::ST_TEXTURE);
	Invalidate();
}

void CPotterDrawView::OnUpdateViewStyleTexture(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_Graphics.GetStyle() & CPotGraphics::ST_TEXTURE) != 0);
}

void CPotterDrawView::OnViewStyleAnimation()
{
	CPotterDrawDoc	*pDoc = GetDocument();
	pDoc->NotifyUndoableEdit(CPotterDrawDoc::PROP_bAnimation, UCODE_PROPERTY);
	pDoc->m_bAnimation ^= 1;
	pDoc->OnPropertyEdit(CPotterDrawDoc::PROP_bAnimation);
}

void CPotterDrawView::OnUpdateViewStyleAnimation(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(GetDocument()->m_bAnimation);
}

void CPotterDrawView::OnViewStyleBounds()
{
	m_Graphics.SetStyle(m_Graphics.GetStyle() ^ CPotGraphics::ST_BOUNDS);
	Invalidate();
}

void CPotterDrawView::OnUpdateViewStyleBounds(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_Graphics.GetStyle() & CPotGraphics::ST_BOUNDS) != 0);
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

void CPotterDrawView::OnToolsMeshInfo()
{
	D3DXVECTOR3	p1, p2;
	m_Graphics.ComputeBounds(p1, p2);
	D3DXVec3Subtract(&p1, &p1, &p2);
	D3DXVECTOR3	vSize(fabs(p1.x), fabs(p1.y), fabs(p1.z));
	CString	sMsg;
	sMsg.Format(_T("Vertices:\t%d\nFacets:\t%d\nWidth:\t%g\nHeight:\t%g\nDepth:\t%g"),
		m_Graphics.GetVertexCount(), m_Graphics.GetFaceCount(), vSize.x, vSize.y, vSize.z);
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
