// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      02may17	initial version

*/

#pragma once

#include "D3DX9Math.h"

// CRotateDlg dialog

class CRotateDlg : public CDialog
{
	DECLARE_DYNAMIC(CRotateDlg)

public:
	CRotateDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRotateDlg();

// Dialog Data
	enum { IDD = IDD_ROTATE };

	D3DXVECTOR3	m_vRotation;
	CString	m_sCaption;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
};
