// Copyleft 2008 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      22oct15	initial version
        01      06jun16	add Clear and GetStride

        extended device-independent bitmap
 
*/

#ifndef CDIBEX_INCLUDED
#define CDIBEX_INCLUDED

#include "Dib.h"

class CDibEx : public CDib {
public:
// Construction
	CDibEx();
	CDibEx(const CDibEx& Dib);
	CDibEx& operator=(const CDibEx& Dib);
	bool	Create(int Width, int Height, WORD BitCount);
	bool	Create(const CBitmap& Bitmap);

// Attributes
	CSize	GetSize() const;
	UINT	GetPixelFormat() const;
	UINT	GetColorTableSize() const;
	DWORD	GetPixel(int x, int y) const;
	void	SetPixel(int x, int y, DWORD Color);
	UINT	GetStride() const;

// Operations
	void	Clear();
	bool	Read(LPCTSTR Path);
	void	Serialize(CArchive& ar);
	void	Swap(CDibEx& Dib);
	void	Attach(HBITMAP Bitmap, PVOID Bits);
	HBITMAP Detach(PVOID& Bits);

// Constants
	enum {
		PF_BPP1,
		PF_BPP4,
		PF_BPP8,
		PF_BPP16,
		PF_BPP24,
		PF_BPP32,
		PIXEL_FORMATS
	};

protected:
// Types
	struct PIXEL_FORMAT {
		DWORD	(CDibEx::*GetPixel)(int x, int y) const;
		void	(CDibEx::*SetPixel)(int x, int y, DWORD Color);
	};

// Constants
	static const PIXEL_FORMAT m_Driver[PIXEL_FORMATS];

// Data members

	CSize	m_Size;			// dimensions in pixels
	UINT	m_Stride;		// stride in bytes
	UINT	m_Format;		// pixel format

// Helpers
	void	OnFormatChange();
	DWORD	GetPixel1(int x, int y) const;
	void	SetPixel1(int x, int y, DWORD Color);
	DWORD	GetPixel4(int x, int y) const;
	void	SetPixel4(int x, int y, DWORD Color);
	DWORD	GetPixel8(int x, int y) const;
	void	SetPixel8(int x, int y, DWORD Color);
	DWORD	GetPixel16(int x, int y) const;
	void	SetPixel16(int x, int y, DWORD Color);
	DWORD	GetPixel24(int x, int y) const;
	void	SetPixel24(int x, int y, DWORD Color);
	DWORD	GetPixel32(int x, int y) const;
	void	SetPixel32(int x, int y, DWORD Color);
};

inline CSize CDibEx::GetSize() const
{
	return(m_Size);
}

inline UINT CDibEx::GetPixelFormat() const
{
	return(m_Format);
}

inline UINT CDibEx::GetStride() const
{
	return(m_Stride);
}

#endif
