// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda

		revision history:
		rev		date	comments
        00      12mar17	initial version

*/

#include "stdafx.h"
#include "D3DGraphics.h"
#include "Benchmark.h"
#include "dxerr9.h"

#define _USE_MATH_DEFINES	// for trig constants
#include <math.h>

#define CHECK(x) if (FAILED(m_hLastError = x)) { OnError(); return false; }

#define GRAPHICS_NATTER 0
#if GRAPHICS_NATTER
#define START_BENCHMARK	CBenchmark b
#define END_BENCHMARK printf("T=%f\n", b.Elapsed());
#else
#define START_BENCHMARK
#define END_BENCHMARK
#define printf sizeof
#endif

#define	Z_RANGE				10000.0f	// z-axis positive and negative range
#define BASELINE_SCALE		150.0f		// baseline scaling without zoom
#define EYE_Z				-800.0f		// z-axis position of eye in all presets
#define ABOVE_EYE_Y			250.0f		// y-axis position of eye in above preset

const D3DVECTOR CD3DGraphics::m_vStandardView[STANDARD_VIEWS] = {
	{atan(ABOVE_EYE_Y / EYE_Z), 0, 0},	// above
	{0, 0, 0},					// front
	{0, float(M_PI), 0},		// back
	{-float(M_PI_2), 0, 0},		// top
	{float(M_PI_2), 0, 0},		// bottom
	{0, -float(M_PI_2), 0},		// left
	{0, float(M_PI_2), 0},		// right
};

CD3DGraphics::CD3DGraphics()
{
	m_szClient = CSize(0, 0);
	m_nStyle = ST_GOURAUD | ST_HIGHLIGHTS | ST_CULLING;
	m_iStdView = INITIAL_VIEW;
	m_hLastError = 0;
	m_fZoom = 1;
	m_vPan = D3DXVECTOR3(0, 0, 0);
	m_vRotation = m_vStandardView[INITIAL_VIEW];
	m_BkgndColor = D3DCOLOR_RGBA(255, 255, 255, 0);
}

CD3DGraphics::~CD3DGraphics()
{
	DestroyDevice();
}

bool CD3DGraphics::OnCreate(bool bResizing)
{
	return true;
}

void CD3DGraphics::OnDestroy()
{
}

bool CD3DGraphics::OnRender()
{
	return true;
}

void CD3DGraphics::OnError()
{
	_tprintf(_T("%s (0x%x)\n"), GetLastErrorName(), m_hLastError);
}

CString	CD3DGraphics::GetLastErrorName() const
{
	return DXGetErrorString9(m_hLastError);
}

CString	CD3DGraphics::GetLastErrorDescription() const
{
	return DXGetErrorDescription9(m_hLastError);
}

bool CD3DGraphics::Create()
{
	printf("CD3DGraphics::Create\n");
	m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!m_pD3D)
		return false;
	return true;
}

void CD3DGraphics::DestroyDevice()
{
	m_pDevice = NULL;
}

bool CD3DGraphics::CreateDevice(HWND hWnd, CSize szClient)
{
	printf("CD3DGraphics::CreateDevice %x\n", hWnd);
	START_BENCHMARK;
	DestroyDevice();
	if (!m_pD3D)
		return false;
	DWORD	nFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	D3DPRESENT_PARAMETERS	pp;
	GetPresentationParameters(pp);
	CHECK(m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, nFlags, &pp, &m_pDevice));
	m_szClient = szClient;
	if (!InitDevice(false))	// not resizing
		return false;
	END_BENCHMARK;
	return true;
}

void CD3DGraphics::GetPresentationParameters(D3DPRESENT_PARAMETERS& pp)
{
	DWORD	nMSQualityLevels;
	HRESULT	MultiSampleResult = m_pD3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL, D3DFMT_D16, TRUE, D3DMULTISAMPLE_NONMASKABLE, &nMSQualityLevels);
	ZeroMemory(&pp, sizeof(pp));
	pp.Windowed = true;
	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pp.EnableAutoDepthStencil = true;
	pp.AutoDepthStencilFormat = D3DFMT_D16;
	if (SUCCEEDED(MultiSampleResult)) {	// if device supports multisampling
		pp.MultiSampleType = D3DMULTISAMPLE_NONMASKABLE;
		pp.MultiSampleQuality = nMSQualityLevels - 1;	// maximum quality
	}
}

bool CD3DGraphics::InitDevice(bool bResizing)
{
	D3DXVECTOR3 vEye(0.0f, 0.0f, EYE_Z), vAt(0.0f, 0.0f, 0.0f), vLook(0.0f, 1.0f, 0.0f);
	D3DXMATRIX	matView;
	D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vLook);
	CHECK(m_pDevice->SetTransform(D3DTS_VIEW, &matView));
	CHECK(m_pDevice->SetRenderState(D3DRS_LIGHTING, true));
	int	nAmbient = 50;
	CHECK(m_pDevice->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(nAmbient, nAmbient, nAmbient)));
	CHECK(m_pDevice->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL));
	CHECK(m_pDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, true));
	CHECK(m_pDevice->SetRenderState(D3DRS_NORMALIZENORMALS, true));
	CHECK(m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));	// antialias textures
	CHECK(m_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
	SetStyle(m_nStyle);
	// set up light source
	D3DLIGHT9	light;
	ZeroMemory(&light, sizeof(light));
	D3DXVECTOR3  vLightDir(10.0f, EYE_Z / ABOVE_EYE_Y, 20.0f);
	D3DXVec3Normalize((D3DXVECTOR3*)&light.Direction, &vLightDir);
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	light.Specular = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	light.Range = sqrtf(FLT_MAX);
	CHECK(m_pDevice->SetLight(0, &light));
	CHECK(m_pDevice->LightEnable(0, true));
	if (!UpdateProjectionTransform())
		return false;
	return OnCreate(bResizing);
}

bool CD3DGraphics::SetStyle(UINT nStyle)
{
	m_nStyle = nStyle;
	CHECK(m_pDevice->SetRenderState(D3DRS_FILLMODE, (m_nStyle & ST_WIREFRAME) != 0 ? D3DFILL_WIREFRAME : D3DFILL_SOLID));
	CHECK(m_pDevice->SetRenderState(D3DRS_SHADEMODE, (m_nStyle & ST_GOURAUD) != 0 ? D3DSHADE_GOURAUD : D3DSHADE_FLAT));
	CHECK(m_pDevice->SetRenderState(D3DRS_SPECULARENABLE, (m_nStyle & ST_HIGHLIGHTS) != 0));
	CHECK(m_pDevice->SetRenderState(D3DRS_CULLMODE, (m_nStyle & ST_CULLING) != 0 ? D3DCULL_CCW : D3DCULL_NONE));
	return true;
}

bool CD3DGraphics::ModifyStyle(UINT nRemove, UINT nAdd)
{
	UINT	nStyle = m_nStyle;
	nStyle &= ~nRemove;
	nStyle |= nAdd;
	return SetStyle(nStyle);
}

bool CD3DGraphics::Resize(CSize szClient)
{
	printf("CD3DGraphics::Resize %d %d\n", szClient.cx, szClient.cy);
	START_BENCHMARK;
	m_szClient = szClient;
	OnDestroy();
	D3DPRESENT_PARAMETERS	pp;
	GetPresentationParameters(pp);
	CHECK(m_pDevice->Reset(&pp));
	if (!InitDevice(true))	// resizing
		return false;
	END_BENCHMARK;
	return true;
}

bool CD3DGraphics::UpdateProjectionTransform()
{
	if (!m_pDevice)
		return false;
	D3DXMATRIX m;
	float	fAspect = float(m_szClient.cx) / m_szClient.cy;
	float	fScale = float(BASELINE_SCALE / m_fZoom);
	D3DXMatrixOrthoLH(&m, fScale * fAspect, fScale, -Z_RANGE, Z_RANGE);
	CHECK(m_pDevice->SetTransform(D3DTS_PROJECTION, &m));
	return UpdateWorldTransform();
}

bool CD3DGraphics::UpdateWorldTransform()
{
	if (!m_pDevice)
		return false;
	D3DXMatrixRotationYawPitchRoll(&m_matRotation, m_vRotation.y, m_vRotation.x, m_vRotation.z);
	D3DXVECTOR3	vTranslate(m_vPan);
	D3DXMATRIX	matPan;
	D3DXMatrixTranslation(&matPan, vTranslate.x, vTranslate.y, vTranslate.z);
	D3DXMatrixMultiply(&matPan, &m_matRotation, &matPan);
	CHECK(m_pDevice->SetTransform(D3DTS_WORLD, &matPan));
	return true;
}

bool CD3DGraphics::Render()
{
	printf("CD3DGraphics::Render\n");
	if (!m_pDevice)
		return false;
	START_BENCHMARK;
	CHECK(m_pDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, m_BkgndColor, 1.0f, 0));
	CHECK(m_pDevice->BeginScene());
	if (!OnRender())
		return false;
	CHECK(m_pDevice->EndScene());
	CHECK(m_pDevice->Present(NULL, NULL, NULL, NULL));
	END_BENCHMARK;
	return true;
}

void CD3DGraphics::SetStandardView(int iView)
{
	ASSERT(iView >= 0 && iView < STANDARD_VIEWS);
	m_iStdView = iView;
	m_vRotation = m_vStandardView[iView];
	VERIFY(UpdateWorldTransform());
}

void CD3DGraphics::SetZoom(double fZoom)
{
	m_fZoom = fZoom;
	VERIFY(UpdateProjectionTransform());
}

void CD3DGraphics::SetRotation(const D3DXVECTOR3& vRotation)
{
	m_vRotation = vRotation;
	VERIFY(UpdateWorldTransform());
}

void CD3DGraphics::SetPan(const D3DXVECTOR3& vPan)
{
	m_vPan = vPan;
	VERIFY(UpdateWorldTransform());
}

void CD3DGraphics::SetZoom(CPoint ptOrigin, double fZoom)
{
	if (!(m_szClient.cy && m_szClient.cx))	// if invalid size
		return;	// avoid divide by zero
	ASSERT(fZoom > 0);
	D3DXVECTOR2	vOrigin(ptOrigin.x - m_szClient.cx / 2.0f, ptOrigin.y - m_szClient.cy / 2.0f);
	vOrigin *= BASELINE_SCALE / float(m_szClient.cy);
	vOrigin = vOrigin / float(m_fZoom) - vOrigin / float(fZoom);
	D3DXVECTOR3	vPan(-vOrigin.x, vOrigin.y, 0);
	m_fZoom = fZoom;
	m_vPan += vPan;
	VERIFY(UpdateProjectionTransform());
}

void CD3DGraphics::SetPan(const D3DXVECTOR3& vPanOrigin, CPoint ptStart, CPoint ptEnd)
{
	if (!(m_szClient.cy && m_szClient.cx))	// if invalid size
		return;	// avoid divide by zero
	D3DXVECTOR2	vOrigin(float(ptStart.x - ptEnd.x), float(ptStart.y - ptEnd.y));
	vOrigin *= BASELINE_SCALE / float(m_szClient.cy);
	vOrigin = vOrigin / float(m_fZoom);
	D3DXVECTOR3	vPan(-vOrigin.x, vOrigin.y, 0);
	m_vPan = vPanOrigin + vPan;
	VERIFY(UpdateWorldTransform());
}

void CD3DGraphics::GetRay(CPoint point, D3DXVECTOR3& rayPos, D3DXVECTOR3& rayDir) const
{
	D3DVIEWPORT9	viewport;
	D3DXMATRIX	matWorld, matView, matProjection;
	m_pDevice->GetViewport(&viewport);
	m_pDevice->GetTransform(D3DTS_PROJECTION, &matProjection);
	m_pDevice->GetTransform(D3DTS_VIEW, &matView);
	m_pDevice->GetTransform(D3DTS_WORLD, &matWorld);
	D3DXVECTOR3	vCursor(float(point.x), float(point.y), 0);
	D3DXVec3Unproject(&rayPos, &vCursor, &viewport, &matProjection, &matView, &matWorld);
	vCursor.z = 1;
	D3DXVECTOR3	p2;
	D3DXVec3Unproject(&p2, &vCursor, &viewport, &matProjection, &matView, &matWorld);
//	printf("(%f %f %f)(%f %f %f)\n", rayPos.x, rayPos.y, rayPos.z, p2.x, p2.y, p2.z);
	rayDir = p2 - rayPos;
}

void CD3DGraphics::ZoomRect(const CRect& rect)
{
	if (!(m_szClient.cy && m_szClient.cx) || rect.IsRectEmpty())	// if invalid size
		return;	// avoid divide by zero
	CPoint	ptOrigin(rect.CenterPoint());
	D3DXVECTOR2	vOrigin(ptOrigin.x - m_szClient.cx / 2.0f, ptOrigin.y - m_szClient.cy / 2.0f);
	vOrigin *= BASELINE_SCALE / float(m_szClient.cy);
	vOrigin /= float(m_fZoom);
	D3DXVECTOR3	vPan(-vOrigin.x, vOrigin.y, 0);
	double	fZoomDeltaX = double(m_szClient.cx) / rect.Width();
	double	fZoomDeltaY = double(m_szClient.cy) / rect.Height();
	double	fDeltaZoom = min(fZoomDeltaX, fZoomDeltaY);
	m_fZoom *= fDeltaZoom;
	m_vPan += vPan;
	VERIFY(UpdateProjectionTransform());
}

void CD3DGraphics::SetBkgndColor(D3DCOLOR Color)
{
	m_BkgndColor = Color;
}

bool CD3DGraphics::ExportImage(LPCTSTR szPath, D3DXIMAGE_FILEFORMAT nFormat)
{
	if (m_pDevice == NULL)
		return false;
	CComPtr<IDirect3DSurface9> pBackbuffer;
	CHECK(m_pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackbuffer));
	CHECK(D3DXSaveSurfaceToFile(szPath, nFormat, pBackbuffer, NULL, NULL));
	return true;
}

bool CD3DGraphics::ExportImage(LPCTSTR szPath, CSize szImage, D3DXIMAGE_FILEFORMAT nFormat)
{
	CComPtr<IDirect3DSurface9> pPrevTarget, pTarget, pPrevStencil, pStencil;
	D3DVIEWPORT9	viewportPrev, viewport;
	CHECK(m_pDevice->GetRenderTarget(0, &pPrevTarget));	// save render target
	CHECK(m_pDevice->GetDepthStencilSurface(&pPrevStencil));	// save depth stencil
	m_pDevice->GetViewport(&viewportPrev);	// save viewport
	D3DPRESENT_PARAMETERS	pp;
	GetPresentationParameters(pp);	// for antialiasing type and quality
	CHECK(m_pDevice->CreateRenderTarget(szImage.cx, szImage.cy, D3DFMT_X8R8G8B8, 
		pp.MultiSampleType, pp.MultiSampleQuality, FALSE, &pTarget, NULL));
	CHECK(m_pDevice->CreateDepthStencilSurface(szImage.cx, szImage.cy, D3DFMT_D16, 
		pp.MultiSampleType, pp.MultiSampleQuality, FALSE, &pStencil, NULL));
	CHECK(m_pDevice->SetRenderTarget(0, pTarget));	// set offscreen render target
	CHECK(m_pDevice->SetDepthStencilSurface(pStencil));	// set matching depth stencil
	viewport = viewportPrev;
	viewport.Width = szImage.cx;
	viewport.Height = szImage.cy;
	m_pDevice->SetViewport(&viewport);	// set viewport for image size
	CSize	szPrevClient(m_szClient);	// save client size
	m_szClient = szImage;	// override client size with image size
	UpdateProjectionTransform();	// compute projection transform for image size
	m_szClient = szPrevClient;	// restore client size
	Render();	// render image to offscreen surface
	CHECK(D3DXSaveSurfaceToFile(szPath, nFormat, pTarget, NULL, NULL));	// export image
	CHECK(m_pDevice->SetRenderTarget(0, pPrevTarget));	// restore render target
	CHECK(m_pDevice->SetDepthStencilSurface(pPrevStencil));	// restore depth stencil
	m_pDevice->SetViewport(&viewportPrev);	// restore viewport
	UpdateProjectionTransform();	// restore projection transform
	return true;
}
