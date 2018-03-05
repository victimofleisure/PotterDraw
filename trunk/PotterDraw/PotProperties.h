// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      23mar17	initial version
		01		01nov17	add IsPolygon
		02		03nov17	add property subgroup
		03		06nov17	add lighting
		04		23nov17	add modulation type flags
		05		24nov17	add animated modulation methods
		06		12dec17	add operations (subset of modulation operations)
		07		02jan18	add ruffle properties
		08		15jan18	add view subgroup for auto rotate
		09		20feb18	add secondary modulation
		
*/

#pragma once

#include "Properties.h"
#include "d3d9types.h"	// for D3DVECTOR
#include "ModulationProps.h"
#include "FixedArray.h"
#include "BoundArray.h"
#include "SplineWnd.h"	// for CSplineState

class CPotProperties : public CProperties {
public:
// Construction
	CPotProperties();

// Types
	struct STYLE_INFO {
		LPCTSTR	szName;		// name
		UINT	nMask;		// bitmask
		bool	bInitVal;	// initial value
	};

// Constants
	enum {	// groups
		#define GROUPDEF(name) GROUP_##name,
		#include "PotPropsDef.h"
		GROUPS
	};
	enum {	// mesh subgroups
		#define MESHSUBGROUPDEF(name) SUBGROUP_##name,
		#include "PotPropsDef.h"
		MESH_SUBGROUPS
	};
	enum {	// view subgroups
		#define VIEWSUBGROUPDEF(name) SUBGROUP_##name,
		#include "PotPropsDef.h"
		VIEW_SUBGROUPS
	};
	enum {	// color patterns
		#define PATTERNDEF(name) COLORPAT_##name,
		#include "PotPropsDef.h"
		COLOR_PATTERNS
	};
	enum {	// palette type
		#define PALTYPEDEF(name) PALTYPE_##name,
		#include "PotPropsDef.h"
		PALETTE_TYPES
	};
	enum {	// motifs
		#define MOTIFDEF(name) MOTIF_##name,
		#include "PotPropsDef.h"
		MOTIFS
	};
	enum {	// operations
		#define OPERATIONDEF(name) OPER_##name,
		#include "PotPropsDef.h"
		OPERATIONS
	};
	enum {	// render styles
		#define RENDERSTYLEDEF(name, initval) RENDSTYLE_##name,
		#include "PotPropsDef.h"
		RENDER_STYLES
	};
	static const OPTION_INFO	m_Group[GROUPS];	// group names
	static const OPTION_INFO	m_MeshSubgroup[MESH_SUBGROUPS];	// mesh subgroup names
	static const OPTION_INFO	m_ViewSubgroup[VIEW_SUBGROUPS];	// view subgroup names
	static const OPTION_INFO	m_ColorPattern[COLOR_PATTERNS];	// pattern names
	static const OPTION_INFO	m_PaletteType[PALETTE_TYPES];	// palette types
	static const OPTION_INFO	m_Motif[MOTIFS];	// motifs
	static const OPTION_INFO	m_Operation[OPERATIONS];	// operations
	static const COLORREF		m_cDefaultPalette[];	// default palette
	static const STYLE_INFO		m_RenderStyleInfo[RENDER_STYLES];	// render styles
	static const D3DMATERIAL9	m_mtrlPotDefault;	// default pot material properties
	enum {	// properties
		#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) PROP_##name,
		#include "PotPropsDef.h"
		PROPERTIES
	};
	static const PROPERTY_INFO	m_Info[PROPERTIES];	// fixed info for each property
	enum {	// modulation state flags
		MOD_ANIMATED		= 0x01,	// at least one property is animated
		MOD_ANIMATED_MESH	= 0x02,	// at least one mesh property is animated
	};
	enum {
		MODULATIONS = PROPERTIES * CModulationProps::MOD_TYPES,	// total including secondary modulations
	};

// Data members
	int		m_nFileVersion;
	#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) type m_##name;
	#include "PotPropsDef.h"
	CArrayEx<COLORREF, COLORREF>	m_arrPalette;	// array of palette color values
	int		m_iPaletteCurSel;	// index of currently selected color within palette
	UINT	m_nRenderStyle;		// render style bitmask
	D3DVECTOR	m_vRotation;	// rotation vector
	D3DVECTOR	m_vPan;			// panning vector
	D3DVECTOR	m_vLightDir;	// light direction vector
	D3DMATERIAL9	m_mtrlPot;	// pot material properties
	double	m_fZoom;			// zoom factor
	CFixedArray<CModulationProps, MODULATIONS>	m_Mod;	// array of modulations
	int		m_iModTarget;		// property index of current modulation target
	int		m_iModType;			// current modulation type; zero for target property, else modulation property index plus one
	CSplineState	m_arrSpline;	// array of spline segments
	bool	m_bIsPlotAnimated;	// true if modulations require animated plot

// Overrides
	virtual	int		GetGroupCount() const;
	virtual	int		GetPropertyCount() const;
	virtual	const PROPERTY_INFO&	GetPropertyInfo(int iProp) const;
	virtual	void	GetVariants(CVariantArray& Var) const;
	virtual	void	SetVariants(const CVariantArray& Var);
	virtual	CString	GetGroupName(int iGroup) const;
	virtual	int		GetSubgroupCount(int iGroup) const;
	virtual	CString	GetSubgroupName(int iGroup, int iSubgroup) const;

// Attributes
	void	SetDefaultPalette();
	static	bool	CanModulate(int iProp);
	bool	IsModulated(int iProp) const;
	bool	IsAnimated(int iProp) const;
	bool	IsAnimatedModulation(int iProp) const;
	bool	HasScallops() const;
	bool	HasRipples() const;
	bool	HasBends() const;
	bool	HasRuffles() const;
	bool	HasHelix() const;
	bool	HasModulations() const;
	bool	HasAnimatedModulations() const;
	int		GetModulationCount() const;
	int		GetAnimatedModulationCount() const;
	int		GetAnimatedModulationCountEx() const;
	int		GetSecondaryModulationCount(int iProp) const;
	bool	IsPolygon() const;
	LPVOID	GetPropertyAddress(int iProp);
	LPCVOID	GetPropertyAddress(int iProp) const;
	static	int		MakeModulationIdx(int iModTarget, int iModType);
	static	int		GetModulationType(int iModObj, int& iModTarget);
	int		IsModulated(int iModTarget, int iModType) const;
	bool	IsAnimatedModulationEx(int iProp) const;

// Operations
	void	ReadProperties(LPCTSTR szPath);
	void	WriteProperties(LPCTSTR szPath) const;
	static	int		FindProperty(LPCTSTR szInternalName);
	static	int		FindRenderStyle(UINT nStyle);
	void	UpdatePlotAnimationState(int iProp);
};

inline bool CPotProperties::IsModulated(int iProp) const
{
	return m_Mod[iProp].IsModulated();
}

inline bool CPotProperties::IsAnimated(int iProp) const
{
	return m_Mod[iProp].IsAnimated();
}

inline bool CPotProperties::IsAnimatedModulation(int iProp) const
{
	return m_Mod[iProp].IsAnimatedModulation();
}

inline bool CPotProperties::HasScallops() const
{
	return (m_fScallops && m_fScallopDepth) || IsModulated(PROP_fScallops) || IsModulated(PROP_fScallopDepth);
}

inline bool CPotProperties::HasRipples() const
{
	return (m_fRipples && m_fRippleDepth) || IsModulated(PROP_fRipples) || IsModulated(PROP_fRippleDepth);
}

inline bool CPotProperties::HasBends() const
{
	return (m_fBends && m_fBendDepth) || IsModulated(PROP_fBends) || IsModulated(PROP_fBendDepth);
}

inline bool CPotProperties::HasRuffles() const
{
	return (m_fRuffles && m_fRuffleDepth) || IsModulated(PROP_fRuffles) || IsModulated(PROP_fRuffleDepth);
}

inline bool CPotProperties::HasHelix() const
{
	return (m_fHelixFrequency && m_fHelixAmplitude) || IsModulated(PROP_fHelixFrequency) || IsModulated(PROP_fHelixAmplitude);
}

inline bool CPotProperties::IsPolygon() const
{
	return m_fPolygonSides >= 2 || IsModulated(PROP_fPolygonSides);
}

inline LPVOID CPotProperties::GetPropertyAddress(int iProp)
{
	ASSERT(IsValidProperty(iProp));
	return LPBYTE(this) + m_Info[iProp].nOffset;
}

inline LPCVOID CPotProperties::GetPropertyAddress(int iProp) const
{
	ASSERT(IsValidProperty(iProp));
	return LPBYTE(this) + m_Info[iProp].nOffset;
}

inline int CPotProperties::MakeModulationIdx(int iModTarget, int iModType)
{
	return iModType * PROPERTIES + iModTarget;	// return index of modulation object
}

inline int CPotProperties::GetModulationType(int iModObj, int& iModTarget)
{
	iModTarget = iModObj % PROPERTIES;	// pass back index of modulation target property
	return iModObj / PROPERTIES;	// and return index of modulation type
}

inline int CPotProperties::IsModulated(int iModTarget, int iModType) const
{
	return IsModulated(MakeModulationIdx(iModTarget, iModType));
}
