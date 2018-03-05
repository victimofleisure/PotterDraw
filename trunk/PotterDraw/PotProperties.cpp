// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      23mar17	initial version
		01		06oct17	bump file version to 2
		02		03nov17	add property subgroup
		03		06nov17	add lighting
		04		23nov17	add modulation type flags
		05		24nov17	add animated modulation methods
		06		12dec17	add operations (subset of modulation operations)
		07		12dec17	bump file version to 3 for app version 1.0.5
		08		02jan18	bump file version to 4 for app version 1.0.6
		09		15jan18	add view subgroup for auto rotate
		10		20feb18	bump file version to 5 for app version 1.0.7
		11		20feb18	add secondary modulation

*/

#include "stdafx.h"
#include "PotterDraw.h"
#include "PotProperties.h"
#include "VariantHelper.h"
#include "IniFile.h"
#include "RegTempl.h"
#include "PotGraphics.h"	// for style bits

#define FILE_ID			_T("PotterDraw")
#define	FILE_VERSION	5

#define RK_FILE_ID		_T("sFileID")
#define RK_FILE_VERSION	_T("nFileVersion")
#define RK_PALETTE		_T("Palette")

#define RK_RENDER_STYLE	_T("RenderStyle")
#define RK_ROTATION		_T("vRotation")
#define RK_PAN			_T("vPan")
#define RK_ZOOM			_T("fZoom")
#define RK_LIGHT_DIR	_T("vLightDir")
#define RK_POT_MATERIAL	_T("mtrlPot")
#define RK_CUR_MOD_TARGET	_T("sModTarget")
#define RK_CUR_MOD_TYPE	_T("sModType")

#define	RK_MOD_SECTION	_T("Modulation\\")
#define	RK_MOD_COUNT	_T("nModulations")
#define	RK_MOD_TARGET	_T("sTarget")
#define	RK_MOD_TYPE		_T("sType")

#define SUBGROUP_NONE	-1

const CProperties::OPTION_INFO CPotProperties::m_Group[GROUPS] = {
	#define GROUPDEF(name) {_T(#name), IDS_PDR_GROUP_##name}, 
	#include "PotPropsDef.h"
};

const CProperties::OPTION_INFO CPotProperties::m_MeshSubgroup[MESH_SUBGROUPS] = {
#define MESHSUBGROUPDEF(name) {_T(#name), IDS_PDR_SUBGROUP_MESH_##name},
	#include "PotPropsDef.h"
};

const CProperties::OPTION_INFO CPotProperties::m_ViewSubgroup[VIEW_SUBGROUPS] = {
#define VIEWSUBGROUPDEF(name) {_T(#name), IDS_PDR_SUBGROUP_VIEW_##name},
	#include "PotPropsDef.h"
};

const CProperties::OPTION_INFO CPotProperties::m_ColorPattern[COLOR_PATTERNS] = {
	#define PATTERNDEF(name) {_T(#name), IDS_PDR_OPT_COLPAT_##name},
	#include "PotPropsDef.h"
};

const CProperties::OPTION_INFO CPotProperties::m_PaletteType[PALETTE_TYPES] = {
	#define PALTYPEDEF(name) {_T(#name), IDS_PDR_OPT_PALTYPE_##name},
	#include "PotPropsDef.h"
};

const CProperties::OPTION_INFO CPotProperties::m_Motif[MOTIFS] = {
	#define MOTIFDEF(name) {_T(#name), IDS_PDR_OPT_MOTIF_##name},
	#include "PotPropsDef.h"
};

const CProperties::OPTION_INFO CPotProperties::m_Operation[OPERATIONS] = {
	#define OPERATIONDEF(name) {_T(#name), IDS_MOD_OPT_OPER_##name},
	#include "PotPropsDef.h"
};

const CProperties::PROPERTY_INFO CPotProperties::m_Info[PROPERTIES] = {
	#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) \
		{_T(#name), IDS_PDR_NAME_##name, IDS_PDR_DESC_##name, offsetof(CPotProperties, m_##name), \
		sizeof(type), &typeid(type), GROUP_##group, SUBGROUP_##subgroup, PT_##proptype, items, itemname, minval, maxval},
	#include "PotPropsDef.h"
};

const COLORREF CPotProperties::m_cDefaultPalette[] = {
	RGB(255, 255, 0),
	RGB(255, 128, 0),
	RGB(255, 0, 0),
	RGB(0, 128, 0),
	RGB(128, 0, 128),
};

#define DEFAULT_COLORS _countof(m_cDefaultPalette)

const CPotProperties::STYLE_INFO CPotProperties::m_RenderStyleInfo[] = {
	#define RENDERSTYLEDEF(name, initval) {_T(#name), CPotGraphics::ST_##name, initval},
	#include "PotPropsDef.h"
};

const D3DMATERIAL9 CPotProperties::m_mtrlPotDefault = {{0.8f, 0.8f, 0.8f}, {0.2f, 0.2f, 0.2f}, {1, 1, 1}, {0}, 1000};

CPotProperties::CPotProperties()
{
	#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) m_##name = initval;
	#include "PotPropsDef.h"
	m_nFileVersion = FILE_VERSION;
	m_iPaletteCurSel = -1;
	m_nRenderStyle = 0;
	ZeroMemory(&m_vRotation, sizeof(m_vRotation));
	ZeroMemory(&m_vPan, sizeof(m_vPan));
	m_vLightDir = CPotGraphics::GetDefaultLightDir();
	m_mtrlPot = m_mtrlPotDefault;
	m_fZoom = 1;
	m_iModTarget = 0;
	m_iModType = 0;
	m_bIsPlotAnimated = false;
	for (int iModObj = 0; iModObj < MODULATIONS; iModObj++)	// for each modulation object
		m_Mod[iModObj].SetDefault();	// set modulation to default values
}

void CPotProperties::SetDefaultPalette()
{
	m_arrPalette.SetSize(DEFAULT_COLORS);
	for (int iColor = 0; iColor < DEFAULT_COLORS; iColor++)
		m_arrPalette[iColor] = m_cDefaultPalette[iColor];
	if (DEFAULT_COLORS)
		m_iPaletteCurSel = 0;
}

int CPotProperties::GetGroupCount() const
{
	return GROUPS;
}

int CPotProperties::GetPropertyCount() const
{
	return PROPERTIES;
}

const CPotProperties::PROPERTY_INFO& CPotProperties::GetPropertyInfo(int iProp) const
{
	ASSERT(IsValidProperty(iProp));
	return m_Info[iProp];
}

void CPotProperties::GetVariants(CVariantArray& Var) const
{
	Var.SetSize(PROPERTIES);
	#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) \
		Var[PROP_##name] = CComVariant(m_##name);
	#include "PotPropsDef.h"
}

void CPotProperties::SetVariants(const CVariantArray& Var)
{
	#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) \
		GetVariant(Var[PROP_##name], m_##name);
	#include "PotPropsDef.h"
}

CString CPotProperties::GetGroupName(int iGroup) const
{
	ASSERT(IsValidGroup(iGroup));
	return LDS(m_Group[iGroup].nNameID);
}

int CPotProperties::GetSubgroupCount(int iGroup) const
{
	ASSERT(IsValidGroup(iGroup));
	if (iGroup == GROUP_MESH)
		return MESH_SUBGROUPS;
	else
		return 0;
}

CString	CPotProperties::GetSubgroupName(int iGroup, int iSubgroup) const
{
	ASSERT(IsValidGroup(iGroup));
	CString	sName;
	int	nNameID;
	switch (iGroup) {
	case GROUP_MESH:
		nNameID = m_MeshSubgroup[iSubgroup].nNameID;
		break;
	case GROUP_VIEW:
		nNameID = m_ViewSubgroup[iSubgroup].nNameID;
		break;
	default:
		nNameID = 0;
	}
	if (nNameID)
		sName.LoadString(nNameID);
	return sName;
}

void CPotProperties::ReadProperties(LPCTSTR szPath)
{
	CIniFile	f;
	f.Open(szPath, CFile::modeRead);
	CString	sFileID;
	RdReg(RK_FILE_ID, sFileID);
	if (sFileID != FILE_ID) {
		CString	msg;
		AfxFormatString1(msg, IDS_DOC_BAD_FORMAT, szPath);
		AfxMessageBox(msg);
		AfxThrowUserException();
	}
	RdReg(RK_FILE_VERSION, m_nFileVersion);
	if (m_nFileVersion > FILE_VERSION) {
		CString	msg;
		AfxFormatString1(msg, IDS_DOC_NEWER_VERSION, szPath);
		AfxMessageBox(msg);
	}
	#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) \
		if (PT_##proptype == PT_ENUM) \
			ReadEnum(REG_SETTINGS, _T(#name), m_##name, itemname, items); \
		else \
			RdReg(_T(#name), m_##name);
	#include "PotPropsDef.h"
	// read palette
	CString	sKey;
	m_arrPalette.SetSize(m_nColors);
	for (int iColor = 0; iColor < m_nColors; iColor++) {
		sKey.Format(_T("%d"), iColor);
		RdReg(RK_PALETTE, sKey, m_arrPalette[iColor]);
	}
	ASSERT(m_nColors);
	if (m_nColors)
		m_iPaletteCurSel = 0;	// select first color
	m_nRenderStyle = 0;
	int	nStyles = _countof(m_RenderStyleInfo);
	for (int iStyle = 0; iStyle < nStyles; iStyle++) {
		const STYLE_INFO&	info = m_RenderStyleInfo[iStyle];
		m_nRenderStyle |= CPersist::GetInt(RK_RENDER_STYLE, info.szName, info.bInitVal) ? info.nMask : 0;
	}
	RdReg(RK_ROTATION, m_vRotation);
	RdReg(RK_PAN, m_vPan);
	RdReg(RK_ZOOM, m_fZoom);
	RdReg(RK_LIGHT_DIR, m_vLightDir);
	RdReg(RK_POT_MATERIAL, m_mtrlPot);
	// read current modulation target and type
	CString	sModTarget, sModType;
	RdReg(RK_CUR_MOD_TARGET, sModTarget);
	RdReg(RK_CUR_MOD_TYPE, sModType);
	m_iModTarget = FindProperty(sModTarget);
	if (!sModType.IsEmpty())	// if modulation type specified
		m_iModType = max(CModulationProps::FindProperty(sModType) - 1, 0);
	else	// default to primary modulation
		m_iModType = 0;
	// read modulations
	CString	sModIdx;
	int	nMods = CPersist::GetInt(REG_SETTINGS, RK_MOD_COUNT, 0);
	for (int iMod = 0; iMod < nMods; iMod++) {	// for each modulation
		sModIdx.Format(_T("%d"), iMod);
		CString	sSection(RK_MOD_SECTION + sModIdx);
		CString	sTarget(CPersist::GetString(sSection, RK_MOD_TARGET));	// read target name
		int	iModTarget = FindProperty(sTarget);
		if (iModTarget >= 0) {	// if target property found
			CString	sModType(CPersist::GetString(sSection, RK_MOD_TYPE));	// read modulation type
			int	iModObj;
			if (sModType.IsEmpty()) {	// if modulation type not specified
				iModObj = iModTarget;	// default to primary modulation
			} else {	// modulation type specified
				int	iModType = max(CModulationProps::FindProperty(sModType) - 1, 0);
				iModObj = MakeModulationIdx(iModTarget, iModType);
			}
			m_Mod[iModObj].ReadProperties(sSection);
		}
	}
	m_arrSpline.ReadProfile();	// read spline
}

void CPotProperties::WriteProperties(LPCTSTR szPath) const
{
	CIniFile	f;
	f.Open(szPath, CFile::modeCreate | CFile::modeWrite);
	WrReg(RK_FILE_ID, CString(FILE_ID));
	WrReg(RK_FILE_VERSION, FILE_VERSION);
	#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) \
		if (PT_##proptype == PT_ENUM) \
			WriteEnum(REG_SETTINGS, _T(#name), m_##name, itemname, items); \
		else \
			WrReg(_T(#name), m_##name);
	#include "PotPropsDef.h"
	// write palette
	CString	sKey;
	for (int iColor = 0; iColor < m_nColors; iColor++) {
		sKey.Format(_T("%d"), iColor);
		WrReg(RK_PALETTE, sKey, m_arrPalette[iColor]);
	}
	int	nStyles = _countof(m_RenderStyleInfo);
	for (int iStyle = 0; iStyle < nStyles; iStyle++) {
		const STYLE_INFO&	info = m_RenderStyleInfo[iStyle];
		CPersist::WriteInt(RK_RENDER_STYLE, info.szName, (m_nRenderStyle & info.nMask) != 0);
	}
	WrReg(RK_ROTATION, m_vRotation);
	WrReg(RK_PAN, m_vPan);
	WrReg(RK_ZOOM, m_fZoom);
	WrReg(RK_LIGHT_DIR, m_vLightDir);
	WrReg(RK_POT_MATERIAL, m_mtrlPot);
	// write current modulation target and type
	CString	sModTarget, sModType;
	if (m_iModTarget >= 0)
		sModTarget = GetPropertyInternalName(m_iModTarget);
	if (m_iModType > 0)	// if secondary modulation selected
		sModType = m_Mod[0].GetPropertyInternalName(m_iModType + 1);
	WrReg(RK_CUR_MOD_TARGET, sModTarget);
	WrReg(RK_CUR_MOD_TYPE, sModType);
	// write modulations
	CString	sModIdx;
	int	nMods = 0;	// count of non-default modulations
	for (int iModObj = 0; iModObj < MODULATIONS; iModObj++) {	// for each modulation object
		const CModulationProps&	mod = m_Mod[iModObj];
		if (!mod.IsDefault()) {	// if property's modulation has non-default values
			sModIdx.Format(_T("%d"), nMods);
			CString	sSection(RK_MOD_SECTION + sModIdx);
			int	iModTarget, iModType = GetModulationType(iModObj, iModTarget);
			CPersist::WriteString(sSection, RK_MOD_TARGET, GetPropertyInternalName(iModTarget));
			if (iModType)	// if secondary modulation, write modulation type
				CPersist::WriteString(sSection, RK_MOD_TYPE, mod.GetPropertyInternalName(iModType + 1));
			mod.WriteProperties(sSection);
			nMods++;
		}
	}
	CPersist::WriteInt(REG_SETTINGS, RK_MOD_COUNT, nMods);
	m_arrSpline.WriteProfile();	// write spline
}

int CPotProperties::FindProperty(LPCTSTR szInternalName)
{
	for (int iProp = 0; iProp < PROPERTIES; iProp++) {
		if (!_tcscmp(m_Info[iProp].pszName, szInternalName))
			return iProp;
	}
	return -1;
}

int CPotProperties::FindRenderStyle(UINT nStyle)
{
	int	nStyles = _countof(m_RenderStyleInfo);
	for (int iStyle = 0; iStyle < nStyles; iStyle++) {
		if (m_RenderStyleInfo[iStyle].nMask == nStyle)
			return iStyle;
	}
	return -1;
}

bool CPotProperties::HasModulations() const
{
	for (int iProp = 0; iProp < PROPERTIES; iProp++) {
		if (IsModulated(iProp))
			return true;
	}
	return false;
}

bool CPotProperties::HasAnimatedModulations() const
{
	for (int iProp = 0; iProp < PROPERTIES; iProp++) {
		if (IsAnimatedModulation(iProp))
			return true;
	}
	return false;
}

int CPotProperties::GetModulationCount() const
{
	int	nMods = 0;
	for (int iProp = 0; iProp < PROPERTIES; iProp++) {
		if (IsModulated(iProp))
			nMods++;
	}
	return nMods;
}

int CPotProperties::GetAnimatedModulationCount() const
{
	int	nAnimMods = 0;
	for (int iProp = 0; iProp < PROPERTIES; iProp++) {
		if (IsAnimatedModulation(iProp))
			nAnimMods++;
	}
	return nAnimMods;
}

int CPotProperties::GetAnimatedModulationCountEx() const
{
	int	nAnimMods = 0;
	for (int iProp = 0; iProp < MODULATIONS; iProp++) {	// include secondary modulations
		if (IsAnimatedModulation(iProp))
			nAnimMods++;
	}
	return nAnimMods;
}

int CPotProperties::GetSecondaryModulationCount(int iProp) const
{
	ASSERT(IsValidProperty(iProp));
	int	nMod2s = 0;
	for (int iModType = 1; iModType < CModulationProps::MOD_TYPES; iModType++) {	// for each secondary modulation type
		iProp += PROPERTIES;	// skip to specified property's next potential secondary modulation
		if (IsModulated(iProp))
			nMod2s++;
	}
	return nMod2s;
}

bool CPotProperties::IsAnimatedModulationEx(int iProp) const
{
	if (iProp < 0)	// if no property selected
		return false;
	ASSERT(iProp < PROPERTIES);	// must be property index
	if (!IsModulated(iProp))	// if property not modulated
		return false;
	if (IsAnimated(iProp))	// if property is animated
		return true;
	for (int iModType = 1; iModType < CModulationProps::MOD_TYPES; iModType++) {	// for each secondary modulation type
		iProp += PROPERTIES;	// skip to target property's next potential secondary modulation
		if (IsAnimatedModulation(iProp))	// if secondary modulation exists and is animated
			return true;
	}
	return false;
}

void CPotProperties::UpdatePlotAnimationState(int iProp)
{
	if (m_iModType)	// if showing secondary modulation
		m_bIsPlotAnimated = IsAnimatedModulation(MakeModulationIdx(iProp, m_iModType));
	else	// showing primary modulation
		m_bIsPlotAnimated = IsAnimatedModulationEx(iProp);	// accounts for secondary modulations
}

bool CPotProperties::CanModulate(int iProp)
{
	if (iProp < 0 || iProp >= PROPERTIES)	// if index out of range
		return false;	// improper argument
	if (m_Info[iProp].iGroup == GROUP_VIEW)	// if property in view group
		return false;	// modulation unsupported
	switch (iProp) {	// if property in this list
	case PROP_nRings:
	case PROP_nSides:
	case PROP_sTexturePath:
	case PROP_nColors:
	case PROP_nColorSharpness:
	case PROP_iPaletteType:
		return false;	// modulation unsupported
	}
	return true;
}
