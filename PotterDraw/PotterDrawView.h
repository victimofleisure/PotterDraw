// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      12mar17	initial version
		01		09oct17	add pre-record auto-rotate and animation states
		02		06nov17	add lighting dialog
		03		15nov17	add palette import/export
		04		23nov17	add step animation
		05		12dec17	add transparent render style
		
*/

// PotterDrawView.h : interface of the CPotterDrawView class
//

#pragma once

#include "PotGraphics.h"
#include "MidiWrap.h"
#include "Benchmark.h"

class CPotterDrawView : public CView
{
protected: // create from serialization only
	CPotterDrawView();
	DECLARE_DYNCREATE(CPotterDrawView)

// Attributes
public:
	CPotterDrawDoc* GetDocument() const;
	CSize	GetClientSize() const;
	UINT	GetRenderStyle() const;
	const D3DXVECTOR3&	GetRotation() const;
	const D3DXVECTOR3&	GetPan() const;
	double	GetZoom() const;
	void	GetViewState(CPotProperties& Props) const;
	void	SetViewState(const CPotProperties& Props);
	bool	IsAnimating() const;
	bool	IsRecording() const;
	int		GetRecordDuration() const;
	int		GetRecordFramesDone() const;
	void	PlotProperty(int iProp, CArrayEx<DPoint, DPoint&>& arrPoint, CRange<double> *pRange);
	void	GetAnimationState(CPotProperties& Props) const;

// Operations
public:
	void	SetAnimation(bool bEnable);
	double	MeasureFrameRate();
	bool	Record(bool bEnable);
	void	StepAnimation(bool bForward);

// Overrides
protected:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnInitialUpdate();
	virtual void OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);

// Implementation
public:
	virtual ~CPotterDrawView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
// Constants
	enum {	// drag states
		DS_NONE,	// default state
		DS_ROTATE,	// rotating view
		DS_PAN,		// panning view
		DRAG_STATES
	};

// Types
	struct FILE_TYPE {
		LPCTSTR	szName;
		LPCTSTR	szExt;
	};

// Member data
	CPotGraphics	m_Graphics;		// Direct3D graphics instance
	bool	m_bAutoRotate;			// true if auto-rotating
	bool	m_bDragRoll;			// true if drag rolling view
	bool	m_bInFrameTimer;		// true if in frame timer handler
	int		m_nDragState;			// see drag state enum above
	CPoint	m_ptDragOrigin;			// point at which drag began, in client coords
	D3DXVECTOR3	m_vDragPan;			// panning at start of drag, in view coords
	D3DXVECTOR3	m_vDragRotation;	// rotation at start of drag, in radians
	D3DXVECTOR3	m_vAutoRotateSpeed;	// auto-rotate speed, in radians per timer tick
	CMMTimerPeriod	m_TimerPeriod;	// multimedia timer's precision
	CMMTimer	m_FrameTimer;		// multimedia frame timer
	UINT	m_nBenchFrames;			// number of frames rendered per benchmark
	CBenchmark	m_tBenchFrame;		// frame rate benchmark elapsed time
	CString	m_sRecordFolder;		// path to record folder
	int		m_iRecordFileFormat;	// record file format; see enum in CRecordDlg
	int		m_nRecordDuration;		// record duration in frames
	int		m_nRecordFramesDone;	// number of frames recorded
	CSize	m_szRecordFrame;		// record frame size
	CDib	m_dibPrint;				// device-independent bitmap for printing
	bool	m_bPreRecordAutoRotate;	// true if auto-rotate was enabled before recording
	bool	m_bPreRecordAnimation;	// true if animation was enabled before recording

// Helpers
	void	SetStandardView(int iView);
	bool	CreateDevice();
	bool	IsActive() const;
	float	CalcPanStep() const;
	double	CalcZoomStep() const;
	static	double	CalcAutoRotateSpeed(double fDegreesPerSec, double fFrameRate);
	void	UpdateAutoRotateSpeed();
	void	StartDrag(UINT nFlags, CPoint point, bool bPan);
	void	EndDrag();
	void	UpdateAnimationEnable();
	bool	ExportImage(LPCTSTR szPath, D3DXIMAGE_FILEFORMAT nFormat = D3DXIFF_BMP);
	void	DumpFace(int iFace);
	static	void	CALLBACK	TimerCallback(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
	static	CString	MakeFileFilter(const FILE_TYPE *pType, int nTypes);
	static	int		FindFileType(const FILE_TYPE *pType, int nTypes, LPCTSTR szExt);

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnViewRotateAbove();
	afx_msg void OnUpdateViewRotateAbove(CCmdUI *pCmdUI);
	afx_msg void OnViewRotateFront();
	afx_msg void OnUpdateViewRotateFront(CCmdUI *pCmdUI);
	afx_msg void OnViewRotateTop();
	afx_msg void OnUpdateViewRotateTop(CCmdUI *pCmdUI);
	afx_msg void OnViewRotateLeft();
	afx_msg void OnUpdateViewRotateLeft(CCmdUI *pCmdUI);
	afx_msg void OnViewRotateBack();
	afx_msg void OnUpdateViewRotateBack(CCmdUI *pCmdUI);
	afx_msg void OnViewRotateBottom();
	afx_msg void OnUpdateViewRotateBottom(CCmdUI *pCmdUI);
	afx_msg void OnViewRotateRight();
	afx_msg void OnUpdateViewRotateRight(CCmdUI *pCmdUI);
	afx_msg void OnViewRotateAuto();
	afx_msg void OnUpdateViewRotateAuto(CCmdUI *pCmdUI);
	afx_msg void OnViewRotateReset();
	afx_msg void OnViewRotateEdit();
	afx_msg void OnViewAnimation();
	afx_msg void OnUpdateViewAnimation(CCmdUI *pCmdUI);
	afx_msg void OnRenderWireframe();
	afx_msg void OnUpdateRenderWireframe(CCmdUI *pCmdUI);
	afx_msg void OnRenderGouraud();
	afx_msg void OnUpdateRenderGouraud(CCmdUI *pCmdUI);
	afx_msg void OnRenderHighlights();
	afx_msg void OnUpdateRenderHighlights(CCmdUI *pCmdUI);
	afx_msg void OnRenderCulling();
	afx_msg void OnUpdateRenderCulling(CCmdUI *pCmdUI);
	afx_msg void OnRenderTexture();
	afx_msg void OnUpdateRenderTexture(CCmdUI *pCmdUI);
	afx_msg void OnRenderTransparent();
	afx_msg void OnUpdateRenderTransparent(CCmdUI *pCmdUI);
	afx_msg void OnRenderBounds();
	afx_msg void OnUpdateRenderBounds(CCmdUI *pCmdUI);
	afx_msg void OnViewZoomIn();
	afx_msg void OnViewZoomOut();
	afx_msg void OnViewZoomReset();
	afx_msg void OnViewPanUp();
	afx_msg void OnViewPanDown();
	afx_msg void OnViewPanLeft();
	afx_msg void OnViewPanRight();
	afx_msg void OnViewPanEdit();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg LRESULT OnDeferredUpdate(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFrameTimer(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetRecord(WPARAM wParam, LPARAM lParam);
	afx_msg void OnFileExport();
	afx_msg void OnFileRecord();
	afx_msg void OnUpdateFileRecord(CCmdUI *pCmdUI);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	afx_msg void OnUpdateNextPane(CCmdUI* pCmdUI);
	afx_msg void OnRenderLighting();
	afx_msg void OnToolsMeshInfo();
	afx_msg void OnPaletteImport();
	afx_msg void OnPaletteExport();
	afx_msg void OnViewStepForward();
	afx_msg void OnUpdateViewStepForward(CCmdUI *pCmdUI);
	afx_msg void OnViewStepBackward();
	afx_msg void OnUpdateViewStepBackward(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewRandomPhase(CCmdUI *pCmdUI);
};

#ifndef _DEBUG  // debug version in PotterDrawView.cpp
inline CPotterDrawDoc* CPotterDrawView::GetDocument() const
{
	return reinterpret_cast<CPotterDrawDoc*>(m_pDocument);
}
#endif

inline CSize CPotterDrawView::GetClientSize() const
{
	return m_Graphics.GetSize();
}

inline UINT CPotterDrawView::GetRenderStyle() const
{
	return m_Graphics.GetStyle();
}

inline const D3DXVECTOR3& CPotterDrawView::GetRotation() const
{
	return m_Graphics.GetRotation();
}

inline const D3DXVECTOR3& CPotterDrawView::GetPan() const
{
	return m_Graphics.GetPan();
}

inline double CPotterDrawView::GetZoom() const
{
	return m_Graphics.GetZoom();
}

inline void CPotterDrawView::GetViewState(CPotProperties& Props) const
{
	m_Graphics.GetViewState(Props);
}

inline void CPotterDrawView::SetViewState(const CPotProperties& Props)
{
	m_Graphics.SetViewState(Props);
}

inline bool CPotterDrawView::IsAnimating() const
{
	return m_FrameTimer.IsCreated();
}

inline bool CPotterDrawView::IsRecording() const
{
	return !m_sRecordFolder.IsEmpty();
}

inline int CPotterDrawView::GetRecordDuration() const
{
	return m_nRecordDuration;
}

inline int CPotterDrawView::GetRecordFramesDone() const
{
	return m_nRecordFramesDone;
}

inline void CPotterDrawView::PlotProperty(int iProp, CArrayEx<DPoint, DPoint&>& arrPoint, CRange<double> *pRange)
{
	m_Graphics.PlotProperty(iProp, arrPoint, pRange);
}

inline void CPotterDrawView::GetAnimationState(CPotProperties& Props) const
{
	m_Graphics.GetAnimationState(Props);
}
