// Copyleft 2008 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      22oct15	initial version
		01		28oct15	in SetPixel1, add missing 1's complement
		02		01jun16	in assignment operator, copy format
        03      06jun16	add Clear

        extended device-independent bitmap
 
*/

#include "stdafx.h"
#include "DibEx.h"

const CDibEx::PIXEL_FORMAT CDibEx::m_Driver[PIXEL_FORMATS] = {
	{&CDibEx::GetPixel1,	&CDibEx::SetPixel1},
	{&CDibEx::GetPixel4,	&CDibEx::SetPixel4},
	{&CDibEx::GetPixel8,	&CDibEx::SetPixel8},
	{&CDibEx::GetPixel16,	&CDibEx::SetPixel16},
	{&CDibEx::GetPixel24,	&CDibEx::SetPixel24},
	{&CDibEx::GetPixel32,	&CDibEx::SetPixel32},
};

CDibEx::CDibEx()
{
	m_Size = CSize(0, 0);
	m_Stride = 0;
	m_Format = 0;
}

CDibEx::CDibEx(const CDibEx& Dib) : CDib(Dib)
{
	m_Size = Dib.m_Size;
	m_Stride = Dib.m_Stride;
	m_Format = Dib.m_Format;
}

CDibEx& CDibEx::operator=(const CDibEx& Dib)
{
	CDib::operator=(Dib);
	m_Size = Dib.m_Size;
	m_Stride = Dib.m_Stride;
	m_Format = Dib.m_Format;
	return *this;
}

bool CDibEx::Create(int Width, int Height, WORD BitCount)
{
	if (!CDib::Create(Width, Height, BitCount))
		return(FALSE);
	OnFormatChange();
	return(TRUE);
}

bool CDibEx::Create(const CBitmap& Bitmap)
{
	if (!CDib::Create(Bitmap))
		return(FALSE);
	OnFormatChange();
	return(TRUE);
}

void CDibEx::Clear()
{
	ZeroMemory(m_pBits, m_Stride * m_Size.cy);
}

bool CDibEx::Read(LPCTSTR Path)
{
	if (!CDib::Read(Path))
		return(FALSE);
	OnFormatChange();
	return(TRUE);
}

void CDibEx::Serialize(CArchive& ar)
{
	CDib::Serialize(ar);
	if (ar.IsLoading())
		OnFormatChange();
}

void CDibEx::Swap(CDibEx& Dib)
{
	CDib::Swap(Dib);
	OnFormatChange();
	Dib.OnFormatChange();
}

void CDibEx::Attach(HBITMAP Bitmap, PVOID Bits)
{
	CDib::Attach(Bitmap, Bits);
	OnFormatChange();
}

HBITMAP CDibEx::Detach(PVOID& Bits)
{
	HBITMAP	hDib = CDib::Detach(Bits);
	m_Size = CSize(0, 0);
	m_Stride = 0;
	m_Format = 0;
	return(hDib);
}

void CDibEx::OnFormatChange()
{
	BITMAP	bmp;
	GetBitmap(&bmp);
	m_Size = CSize(bmp.bmWidth, bmp.bmHeight);
	m_Stride = CDib::GetStride(bmp);
	switch (bmp.bmBitsPixel) {
	case 1:
		m_Format = PF_BPP1;
		break;
	case 4:
		m_Format = PF_BPP4;
		break;
	case 8:
		m_Format = PF_BPP8;
		break;
	case 16:
		m_Format = PF_BPP16;
		break;
	case 24:
		m_Format = PF_BPP24;
		break;
	case 32:
		m_Format = PF_BPP32;
		break;
	default:
		ASSERT(0);
		m_Format = 0;
	}
}

UINT CDibEx::GetColorTableSize() const
{
	static const UINT ColorTableSize[PIXEL_FORMATS] = {2, 16, 256};
	return(ColorTableSize[m_Format]);
}

DWORD CDibEx::GetPixel(int x, int y) const
{
	if (x >= 0 && x < m_Size.cx && y >= 0 && y < m_Size.cy)
		return((this->*m_Driver[m_Format].GetPixel)(x, y));
	return(0);
}

void CDibEx::SetPixel(int x, int y, DWORD Color)
{
	if (x >= 0 && x < m_Size.cx && y >= 0 && y < m_Size.cy)
		(this->*m_Driver[m_Format].SetPixel)(x, y, Color);
}

#define ROWOFS(y) ((m_Size.cy - 1 - y) * m_Stride)

DWORD CDibEx::GetPixel1(int x, int y) const
{
	return((((BYTE *)m_pBits)[ROWOFS(y) + (x >> 3)] & (0x80 >> (x & 7))) != 0);
}

void CDibEx::SetPixel1(int x, int y, DWORD Color)
{
	BYTE	*pb = &((BYTE *)m_pBits)[ROWOFS(y) + (x >> 3)];
	BYTE	mask = 0x80 >> (x & 7);
	if (Color)
		*pb |= mask;
	else
		*pb &= ~mask;
}

DWORD CDibEx::GetPixel4(int x, int y) const
{
	return((((BYTE *)m_pBits)[ROWOFS(y) + (x >> 1)] >> ((x & 1) ? 0 : 4)) & 0x0f);
}

void CDibEx::SetPixel4(int x, int y, DWORD Color)
{
	BYTE	*pb = &((BYTE *)m_pBits)[ROWOFS(y) + (x >> 1)];
	*pb = BYTE((*pb & ((x & 1) ? 0xf0 : 0x0f)) | ((Color & 0xf) << ((x & 1) ? 0 : 4)));
}

DWORD CDibEx::GetPixel8(int x, int y) const
{
	return(((BYTE *)m_pBits)[ROWOFS(y) + x]);
}

void CDibEx::SetPixel8(int x, int y, DWORD Color)
{
	((BYTE *)m_pBits)[ROWOFS(y) + x] = BYTE(Color);
}

DWORD CDibEx::GetPixel16(int x, int y) const
{
	return(((SHORT *)&((BYTE *)m_pBits)[ROWOFS(y)])[x]);
}

void CDibEx::SetPixel16(int x, int y, DWORD Color)
{
	((SHORT *)&((BYTE *)m_pBits)[ROWOFS(y)])[x] = SHORT(Color);
}

DWORD CDibEx::GetPixel24(int x, int y) const
{
	DWORD	c = 0;
	BYTE	*pb = &((BYTE *)m_pBits)[ROWOFS(y) + x * 3];
	((BYTE *)&c)[0] = pb[2];
	((BYTE *)&c)[1] = pb[1];
	((BYTE *)&c)[2] = pb[0];
	return(c);
}

void CDibEx::SetPixel24(int x, int y, DWORD Color)
{
	BYTE	*pb = &((BYTE *)m_pBits)[ROWOFS(y) + x * 3];
	pb[2] = ((BYTE *)&Color)[0];
	pb[1] = ((BYTE *)&Color)[1];
	pb[0] = ((BYTE *)&Color)[2];
}

DWORD CDibEx::GetPixel32(int x, int y) const
{
	return(((DWORD *)&((BYTE *)m_pBits)[ROWOFS(y)])[x]);
}

void CDibEx::SetPixel32(int x, int y, DWORD Color)
{
	((DWORD *)&((BYTE *)m_pBits)[ROWOFS(y)])[x] = Color;
}
