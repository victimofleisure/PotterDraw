// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      23mar17	initial version
		01		24aug17	add scallop phase
		02		05oct17	add scallop range, power, operation
		03		19oct17	add scallop waveform, pulse width, and slew
		04		01nov17	add polygon properties
		05		03nov17	add property subgroup
		06		10nov17	add scallop power type
		07		15nov17	add radius color pattern
		08		10dec17	add azimuth and incline color patterns
		09		12dec17	add operations (subset of modulation operations)
		10		12dec17	for ripple and bend, add operation, power and power type
		11		12dec17	add transparent render style
		12		15dec17	add edges color pattern
		13		02jan18	add ruffle properties
		14		03jan18	add ring phase
		15		15jan18	add view subgroup for auto rotate; add auto zoom
		
*/

#ifdef GROUPDEF

GROUPDEF(	MESH	)
GROUPDEF(	TEXTURE	)
GROUPDEF(	VIEW	)

#undef GROUPDEF
#endif

#ifdef MESHSUBGROUPDEF

MESHSUBGROUPDEF(	POLYGON		)
MESHSUBGROUPDEF(	SCALLOP		)
MESHSUBGROUPDEF(	RIPPLE		)
MESHSUBGROUPDEF(	BEND		)
MESHSUBGROUPDEF(	RUFFLE		)
MESHSUBGROUPDEF(	HELIX		)

#undef MESHSUBGROUPDEF
#endif

#ifdef VIEWSUBGROUPDEF

VIEWSUBGROUPDEF(	AUTOROTATE	)

#undef VIEWSUBGROUPDEF
#endif

#ifdef PROPDEF

//			group		subgroup	proptype	type		name				initval			minval		maxval		itemname	items
PROPDEF(	MESH,		NONE,		VAR,		int,		nRings,				100,			2,			SHRT_MAX,	NULL,		0)
PROPDEF(	MESH,		NONE,		VAR,		int,		nSides,				100,			3,			SHRT_MAX,	NULL,		0)
PROPDEF(	MESH,		NONE,		VAR,		double,		fRadius,			1,				1e-6,		1e6,		NULL,		0)
PROPDEF(	MESH,		NONE,		VAR,		double,		fWallThickness,		5,				1e-6,		1e6,		NULL,		0)
PROPDEF(	MESH,		NONE,		VAR,		double,		fTwist,				0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		NONE,		VAR,		double,		fRingPhase,			0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		NONE,		VAR,		double,		fAspectRatio,		1,				0.1,		10.0,		NULL,		0)
PROPDEF(	MESH,		POLYGON,	VAR,		double,		fPolygonSides,		0,				0,			1e6,		NULL,		0)
PROPDEF(	MESH,		POLYGON,	VAR,		double,		fPolygonRoundness,	0,				-1.0,		1.0,		NULL,		0)
PROPDEF(	MESH,		POLYGON,	VAR,		double,		fPolygonBulge,		0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		POLYGON,	VAR,		double,		fPolygonPhase,		0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		SCALLOP,	VAR,		double,		fScallops,			0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		SCALLOP,	VAR,		double,		fScallopDepth,		0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		SCALLOP,	VAR,		double,		fScallopPhase,		0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		SCALLOP,	ENUM,		int,		iScallopWaveform,	0,				0,			0,			CModulationProps::m_Waveform + 1,	_countof(CModulationProps::m_Waveform) - 1)
PROPDEF(	MESH,		SCALLOP,	ENUM,		int,		iScallopOperation,	0,				0,			0,			CModulationProps::m_Operation,	_countof(CModulationProps::m_Operation))
PROPDEF(	MESH,		SCALLOP,	ENUM,		int,		iScallopRange,		0,				0,			0,			CModulationProps::m_Range,	_countof(CModulationProps::m_Range))
PROPDEF(	MESH,		SCALLOP,	ENUM,		int,		iScallopMotif,		0,				0,			0,			m_Motif,	_countof(m_Motif))
PROPDEF(	MESH,		SCALLOP,	VAR,		double,		fScallopPower,		0,				0,			DBL_MAX,	NULL,		0)
PROPDEF(	MESH,		SCALLOP,	ENUM,		int,		iScallopPowerType,	0,				0,			0,			CModulationProps::m_PowerType,	_countof(CModulationProps::m_PowerType))
PROPDEF(	MESH,		SCALLOP,	VAR,		double,		fScallopPulseWidth,	0.5,			0,			0,			NULL,		0)
PROPDEF(	MESH,		SCALLOP,	VAR,		double,		fScallopSlew,		0.5,			0,			0,			NULL,		0)
PROPDEF(	MESH,		RIPPLE,		VAR,		double,		fRipples,			0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		RIPPLE,		VAR,		double,		fRippleDepth,		0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		RIPPLE,		VAR,		double,		fRipplePhase,		0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		RIPPLE,		ENUM,		int,		iRippleMotif,		0,				0,			0,			m_Motif,	_countof(m_Motif))
PROPDEF(	MESH,		RIPPLE,		ENUM,		int,		iRippleOperation,	0,				0,			0,			m_Operation,	_countof(m_Operation))
PROPDEF(	MESH,		RIPPLE,		VAR,		double,		fRipplePower,		0,				0,			DBL_MAX,	NULL,		0)
PROPDEF(	MESH,		RIPPLE,		ENUM,		int,		iRipplePowerType,	0,				0,			0,			CModulationProps::m_PowerType,	_countof(CModulationProps::m_PowerType))
PROPDEF(	MESH,		BEND,		VAR,		double,		fBends,				0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		BEND,		VAR,		double,		fBendDepth,			0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		BEND,		VAR,		double,		fBendPhase,			0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		BEND,		ENUM,		int,		iBendMotif,			0,				0,			0,			m_Motif,	_countof(m_Motif))
PROPDEF(	MESH,		BEND,		VAR,		double,		fBendPoles,			0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		BEND,		VAR,		double,		fBendPolePhase,		0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		BEND,		ENUM,		int,		iBendPoleMotif,		0,				0,			0,			m_Motif,	_countof(m_Motif))
PROPDEF(	MESH,		BEND,		ENUM,		int,		iBendOperation,		0,				0,			0,			m_Operation,	_countof(m_Operation))
PROPDEF(	MESH,		BEND,		VAR,		double,		fBendPower,			0,				0,			DBL_MAX,	NULL,		0)
PROPDEF(	MESH,		BEND,		ENUM,		int,		iBendPowerType,		0,				0,			0,			CModulationProps::m_PowerType,	_countof(CModulationProps::m_PowerType))
PROPDEF(	MESH,		RUFFLE,		VAR,		double,		fRuffles,			0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		RUFFLE,		VAR,		double,		fRuffleDepth,		0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		RUFFLE,		VAR,		double,		fRufflePhase,		0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		RUFFLE,		ENUM,		int,		iRuffleWaveform,	0,				0,			0,			CModulationProps::m_Waveform + 1,	_countof(CModulationProps::m_Waveform) - 1)
PROPDEF(	MESH,		RUFFLE,		ENUM,		int,		iRuffleMotif,		0,				0,			0,			m_Motif,	_countof(m_Motif))
PROPDEF(	MESH,		RUFFLE,		VAR,		double,		fRufflePower,		0,				0,			DBL_MAX,	NULL,		0)
PROPDEF(	MESH,		RUFFLE,		ENUM,		int,		iRufflePowerType,	0,				0,			0,			CModulationProps::m_PowerType,	_countof(CModulationProps::m_PowerType))
PROPDEF(	MESH,		RUFFLE,		VAR,		double,		fRufflePulseWidth,	0.5,			0,			0,			NULL,		0)
PROPDEF(	MESH,		RUFFLE,		VAR,		double,		fRuffleSlew,		0.5,			0,			0,			NULL,		0)
PROPDEF(	MESH,		HELIX,		VAR,		double,		fHelixFrequency,	0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		HELIX,		VAR,		double,		fHelixAmplitude,	0,				0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	NONE,		FILE,		CString,	sTexturePath,		_T(""),			0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	NONE,		VAR,		int,		nColors,			DEFAULT_COLORS,	1,			256,		NULL,		0)
PROPDEF(	TEXTURE,	NONE,		VAR,		int,		nColorSharpness,	100,			1,			100,		NULL,		0)
PROPDEF(	TEXTURE,	NONE,		VAR,		double,		fColorCycles,		5.0,			0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	NONE,		VAR,		double,		fStripeFrequency,	1,				0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	NONE,		VAR,		double,		fStripeAmplitude,	0.125,			0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	NONE,		VAR,		double,		fStripePhase,		0,				0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	NONE,		VAR,		double,		fPetals,			6.0,			0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	NONE,		ENUM,		int,		iColorPattern,		0,				0,			0,			m_ColorPattern,	_countof(m_ColorPattern))
PROPDEF(	TEXTURE,	NONE,		ENUM,		int,		iPaletteType,		0,				0,			0,			m_PaletteType,	_countof(m_PaletteType))
PROPDEF(	TEXTURE,	NONE,		VAR,		double,		fOffsetU,			0,				0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	NONE,		VAR,		double,		fOffsetV,			0,				0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	NONE,		VAR,		double,		fCyclesV,			1,				0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	NONE,		VAR,		double,		fEdgeGain,			6.0,			0,			0,			NULL,		0)
PROPDEF(	VIEW,		NONE,		VAR,		bool,		bAnimation,			0,				0,			0,			NULL,		0)
PROPDEF(	VIEW,		NONE,		VAR,		double,		fFrameRate,			30,				0.01,		100.0,		NULL,		0)
PROPDEF(	VIEW,		NONE,		COLOR,		COLORREF,	clrBackground,		0xffffff,		0,			0,			NULL,		0)
PROPDEF(	VIEW,		AUTOROTATE,	VAR,		double,		fAutoRotateYaw,		0,				0,			0,			NULL,		0)
PROPDEF(	VIEW,		AUTOROTATE,	VAR,		double,		fAutoRotatePitch,	12,				0,			0,			NULL,		0)
PROPDEF(	VIEW,		AUTOROTATE,	VAR,		double,		fAutoRotateRoll,	0,				0,			0,			NULL,		0)
PROPDEF(	VIEW,		AUTOROTATE,	VAR,		double,		fAutoRotateZoom,	0,				0,			0,			NULL,		0)

#undef PROPDEF
#endif

#ifdef PATTERNDEF

// append only, to maintain compatibility with previously saved documents
PATTERNDEF(	STRIPES)
PATTERNDEF(	RINGS)
PATTERNDEF(	PETALS)
PATTERNDEF(	POLAR)
PATTERNDEF(	RADIUS)
PATTERNDEF(	AZIMUTH)
PATTERNDEF(	INCLINE)
PATTERNDEF(	AZI_INC)
PATTERNDEF(	EDGES)

#undef PATTERNDEF
#endif

#ifdef PALTYPEDEF

// append only, to maintain compatibility with previously saved documents
PALTYPEDEF(	LINEAR)
PALTYPEDEF(	COMPLEMENT)
PALTYPEDEF(	SPLIT)

#undef PALTYPEDEF
#endif

#ifdef RENDERSTYLEDEF
	
RENDERSTYLEDEF(	WIREFRAME,		0)
RENDERSTYLEDEF(	GOURAUD,		1)
RENDERSTYLEDEF(	HIGHLIGHTS,		1)
RENDERSTYLEDEF(	CULLING,		1)
RENDERSTYLEDEF(	TEXTURE,		1)
RENDERSTYLEDEF(	BOUNDS,			0)
RENDERSTYLEDEF(	TRANSPARENT,	0)

#undef RENDERSTYLEDEF
#endif

#ifdef MOTIFDEF

// append only, to maintain compatibility with previously saved documents
MOTIFDEF(	NONE)
MOTIFDEF(	REEDS)
MOTIFDEF(	FLUTES)
MOTIFDEF(	PARTED_REEDS)
MOTIFDEF(	PARTED_FLUTES)

#undef MOTIFDEF
#endif

#ifdef OPERATIONDEF

// append only, to maintain compatibility with previously saved documents
OPERATIONDEF(	ADD)
OPERATIONDEF(	EXPONENTIATE)

#undef OPERATIONDEF
#endif
