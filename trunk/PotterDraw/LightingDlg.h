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

#pragma once

#include "D3DX9Math.h"

// CLightingDlg dialog

class CLightingDlg : public CDialog
{
	DECLARE_DYNAMIC(CLightingDlg)

public:
	CLightingDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLightingDlg();

// Dialog Data
	enum { IDD = IDD_LIGHTING };

	D3DXVECTOR3	m_vDir;
	float	m_fDiffuse;
	float	m_fAmbient;
	float	m_fSpecular;
	float	m_fPower;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnReset();
};
