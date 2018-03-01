// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda

		revision history:
		rev		date	comments
        00      01jul17	initial version

*/

#pragma once

#include "DPoint.h"

// CTranslate2DDlg dialog

class CTranslate2DDlg : public CDialog
{
	DECLARE_DYNAMIC(CTranslate2DDlg)

public:
	CTranslate2DDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTranslate2DDlg();

// Dialog Data
	enum { IDD = IDD_TRANSLATE_2D };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	DPoint m_pt;
};
