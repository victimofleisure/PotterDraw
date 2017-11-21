// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda

		revision history:
		rev		date	comments
        00      06nov17	initial version

*/

// LightingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "LightingDlg.h"
#include "PotGraphics.h"

// CLightingDlg dialog

IMPLEMENT_DYNAMIC(CLightingDlg, CDialog)

CLightingDlg::CLightingDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD, pParent),
	m_vDir(0, 0, 0),
	m_fDiffuse(0),
	m_fAmbient(0),
	m_fSpecular(0),
	m_fPower(0)
{
}

CLightingDlg::~CLightingDlg()
{
}

void CLightingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_LIGHTING_DIR_X, m_vDir.x);
	DDX_Text(pDX, IDC_LIGHTING_DIR_Y, m_vDir.y);
	DDX_Text(pDX, IDC_LIGHTING_DIR_Z, m_vDir.z);
	DDX_Text(pDX, IDC_LIGHTING_DIFFUSE, m_fDiffuse);
	DDX_Text(pDX, IDC_LIGHTING_AMBIENT, m_fAmbient);
	DDX_Text(pDX, IDC_LIGHTING_SPECULAR, m_fSpecular);
	DDX_Text(pDX, IDC_LIGHTING_POWER, m_fPower);
}


BEGIN_MESSAGE_MAP(CLightingDlg, CDialog)
	ON_BN_CLICKED(IDC_LIGHTING_RESET, OnReset)
END_MESSAGE_MAP()


// CLightingDlg message handlers

void CLightingDlg::OnReset()
{
	m_vDir = CPotGraphics::GetDefaultLightDir();
	m_fDiffuse = CPotProperties::m_mtrlPotDefault.Diffuse.r;
	m_fAmbient = CPotProperties::m_mtrlPotDefault.Ambient.r;
	m_fSpecular = CPotProperties::m_mtrlPotDefault.Specular.r;
	m_fPower = CPotProperties::m_mtrlPotDefault.Power;
	UpdateData(FALSE);	// init dialog from data
}
