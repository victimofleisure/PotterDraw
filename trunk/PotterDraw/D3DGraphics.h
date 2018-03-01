// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      12mar17	initial version
		01		06nov17	add get/set light
		02		12dec17	add transparent style
		
*/

#pragma once

#include "d3d9.h"
#include "D3DX9Math.h"

#define IDirect3D IDirect3D9
#define IDirect3DDevice IDirect3DDevice9

class CD3DGraphics : public WObject
{
public:
// Construction
	CD3DGraphics();
	virtual	~CD3DGraphics();

// Constants
	enum {	// axes
		AXIS_X,
		AXIS_Y,
		AXIS_Z,
		AXES,
	};
	enum {	// styles
		ST_WIREFRAME	= 0x0001,		// show wireframe
		ST_GOURAUD		= 0x0002,		// use Gouraud shading
		ST_HIGHLIGHTS	= 0x0004,		// show specular highlights
		ST_CULLING		= 0x0008,		// do back-face culling
		ST_TRANSPARENT	= 0x0010,		// transparent material 
	};
	enum {	// standard views
		SV_ABOVE,
		SV_FRONT,
		SV_BACK,
		SV_TOP,
		SV_BOTTOM,
		SV_LEFT,
		SV_RIGHT,
		STANDARD_VIEWS,
		INITIAL_VIEW = SV_ABOVE,
	};

// Attributes
	bool	IsCreated() const;
	bool	IsDeviceCreated() const;
	CSize	GetSize() const;
	UINT	GetStyle() const;
	virtual	bool	SetStyle(UINT nStyle);
	bool	ModifyStyle(UINT nRemove, UINT nAdd);
	int		GetStandardView() const;
	void	SetStandardView(int iView);
	double	GetZoom() const;
	void	SetZoom(double fZoom);
	void	SetZoom(CPoint ptOrigin, double fZoom);
	const D3DXVECTOR3&	GetRotation() const;
	void	SetRotation(const D3DXVECTOR3& vRotation);
	const D3DXVECTOR3&	GetPan() const;
	void	SetPan(const D3DXVECTOR3& vPan);
	void	SetPan(const D3DXVECTOR3& vPanOrigin, CPoint ptStart, CPoint ptEnd);
	D3DCOLOR	GetBkgndColor() const;
	void	SetBkgndColor(D3DCOLOR Color);
	HRESULT	GetLastError() const;
	CString	GetLastErrorName() const;
	CString	GetLastErrorDescription() const;
	void	GetLight(D3DLIGHT9& light) const;
	bool	SetLight(const D3DLIGHT9& light);

// Operations
	bool	Create(); 
	bool	CreateDevice(HWND hWnd, CSize szClient);
	bool	Resize(CSize szClient);
	bool	Render();
	void	DestroyDevice();
	void	GetRay(CPoint point, D3DXVECTOR3& rayPos, D3DXVECTOR3& rayDir) const;
	void	ZoomRect(const CRect& rect);
	static	D3DCOLOR	CvtFromColorRef(COLORREF Color);
	bool	ExportImage(LPCTSTR szPath, D3DXIMAGE_FILEFORMAT nFormat = D3DXIFF_BMP);
	bool	ExportImage(LPCTSTR szPath, CSize szImage, D3DXIMAGE_FILEFORMAT nFormat = D3DXIFF_BMP);
	static	int		FindStandardView(const D3DXVECTOR3& vRotation);

protected:
// Constants
	static const D3DVECTOR	m_vStandardView[STANDARD_VIEWS];	// standard view rotations
	static const D3DVECTOR	m_vDefaultLightDir;	// default light direction

// Data members
	CComPtr<IDirect3D>	m_pD3D;	// Direct3D interface
	CComPtr<IDirect3DDevice>	m_pDevice;	// Direct3D device interface
	CSize	m_szClient;			// size of client window, in pixels
	UINT	m_nStyle;			// render style; see enum
	int		m_iStdView;			// standard view; see enum
	HRESULT	m_hLastError;		// most recent Direct3D error code
	double	m_fZoom;			// current zoom, as a scaling factor
	D3DXVECTOR3	m_vPan;			// current panning, in world coordinates
	D3DXVECTOR3	m_vRotation;	// yaw, pitch, and roll, in radians
	D3DXMATRIX	m_matRotation;	// world rotation matrix
	D3DCOLOR	m_clrBkgnd;		// background color
	D3DLIGHT9	m_light;		// light source

// Overrideables
	virtual	bool	OnCreate(bool bResizing);
	virtual	void	OnDestroy();
	virtual	bool	OnRender();
	virtual	void	OnError();

// Helpers
	void	GetPresentationParameters(D3DPRESENT_PARAMETERS& pp);
	bool	InitDevice(bool bResizing);
	bool	UpdateWorldTransform();
	bool	UpdateProjectionTransform();
};

inline bool CD3DGraphics::IsCreated() const
{
	return m_pD3D != NULL;
}

inline bool CD3DGraphics::IsDeviceCreated() const
{
	return m_pDevice != NULL;
}

inline CSize CD3DGraphics::GetSize() const
{
	return m_szClient;
}

inline UINT CD3DGraphics::GetStyle() const
{
	return m_nStyle;
}

inline int CD3DGraphics::GetStandardView() const
{
	return m_iStdView;
}

inline double CD3DGraphics::GetZoom() const
{
	return m_fZoom;
}

inline const D3DXVECTOR3& CD3DGraphics::GetRotation() const
{
	return m_vRotation;
}

inline const D3DXVECTOR3& CD3DGraphics::GetPan() const
{
	return m_vPan;
}

inline D3DCOLOR CD3DGraphics::GetBkgndColor() const
{
	return m_clrBkgnd;
}

inline D3DCOLOR CD3DGraphics::CvtFromColorRef(COLORREF Color)
{
	return D3DCOLOR_ARGB(0, GetRValue(Color), GetGValue(Color), GetBValue(Color));
}

inline HRESULT CD3DGraphics::GetLastError() const
{
	return m_hLastError;
}

inline void CD3DGraphics::GetLight(D3DLIGHT9& light) const
{
	light = m_light;
}
