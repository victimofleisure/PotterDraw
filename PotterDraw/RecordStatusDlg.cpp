// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda
 
		revision history:
		rev		date	comments
		00		16apr17	initial version
 
*/

// RecordStatusDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PotterDraw.h"
#include "RecordStatusDlg.h"
#include "MainFrm.h"
#include "PotterDrawDoc.h"
#include "PotterDrawView.h"
#include "RecordDlg.h"

// CRecordStatusDlg dialog

IMPLEMENT_DYNAMIC(CRecordStatusDlg, CModelessDlg)

CRecordStatusDlg::CRecordStatusDlg(CWnd* pParent /*=NULL*/)
	: CModelessDlg(IDD, 0, _T("RecordStatus"), pParent)
{
}

CRecordStatusDlg::~CRecordStatusDlg()
{
}

void CRecordStatusDlg::DoDataExchange(CDataExchange* pDX)
{
	CModelessDlg::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RECORD_STATUS_PROGRESS, m_Progress);
	DDX_Control(pDX, IDC_RECORD_STATUS_COMPLETED, m_Completed);
	DDX_Control(pDX, IDC_RECORD_STATUS_DURATION, m_Duration);
}

BEGIN_MESSAGE_MAP(CRecordStatusDlg, CModelessDlg)
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

// CRecordStatusDlg message handlers

BOOL CRecordStatusDlg::OnInitDialog()
{
	CModelessDlg::OnInitDialog();
	SetTimer(TIMER_ID, TIMER_PERIOD, NULL);
	SendMessage(WM_TIMER, TIMER_ID);
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CRecordStatusDlg::OnDestroy()
{
	KillTimer(TIMER_ID);
	CModelessDlg::OnDestroy();
}

void CRecordStatusDlg::OnOK()
{
	theApp.GetMainFrame()->OnRecord(false);
	CModelessDlg::OnOK();
}

void CRecordStatusDlg::OnTimer(UINT_PTR nIDEvent)
{
	CPotterDrawView	*pView = theApp.GetMainFrame()->GetActiveMDIView();
	CString	sDuration, sCompleted;
	int	nProgPos = 0;
	if (pView != NULL && pView->IsRecording()) {
		double	fFrameRate = pView->GetDocument()->m_fFrameRate;
		int	nDuration = pView->GetRecordDuration();
		int	nCompleted = pView->GetRecordFramesDone();
		CString	sTime;
		CRecordDlg::FrameToTime(nDuration, fFrameRate, sTime);
		sDuration.Format(_T(" %s (%d)"), sTime, nDuration);
		CRecordDlg::FrameToTime(nCompleted, fFrameRate, sTime);
		sCompleted.Format(_T(" %s (%d)"), sTime, nCompleted);
		if (nDuration)	// avoid divide by zero
			nProgPos = round(double(nCompleted) / nDuration * 100.0);
	}
	m_Duration.SetWindowText(sDuration);
	m_Completed.SetWindowText(sCompleted);
	m_Progress.SetPos(nProgPos);
	CModelessDlg::OnTimer(nIDEvent);
}
