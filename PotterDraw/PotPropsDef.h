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
		
*/

#ifdef GROUPDEF

GROUPDEF(	MESH	)
GROUPDEF(	TEXTURE	)
GROUPDEF(	VIEW	)

#undef GROUPDEF
#endif

#ifdef PROPDEF

//			group		proptype	type		name				initval			minval		maxval		itemname	items
PROPDEF(	MESH,		VAR,		int,		nRings,				100,			2,			SHRT_MAX,	NULL,		0)
PROPDEF(	MESH,		VAR,		int,		nSides,				100,			3,			SHRT_MAX,	NULL,		0)
PROPDEF(	MESH,		VAR,		double,		fRadius,			1,				1e-6,		1e6,		NULL,		0)
PROPDEF(	MESH,		VAR,		double,		fWallThickness,		5,				1e-6,		1e6,		NULL,		0)
PROPDEF(	MESH,		VAR,		double,		fScallops,			0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		VAR,		double,		fScallopDepth,		0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		VAR,		double,		fScallopPhase,		0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		ENUM,		int,		iScallopWaveform,	0,				0,			0,			CModulationProps::m_Waveform + 1,	_countof(CModulationProps::m_Waveform) - 1)
PROPDEF(	MESH,		ENUM,		int,		iScallopOperation,	0,				0,			0,			CModulationProps::m_Operation,	_countof(CModulationProps::m_Operation))
PROPDEF(	MESH,		ENUM,		int,		iScallopRange,		0,				0,			0,			CModulationProps::m_Range,	_countof(CModulationProps::m_Range))
PROPDEF(	MESH,		ENUM,		int,		iScallopMotif,		0,				0,			0,			m_Motif,	_countof(m_Motif))
PROPDEF(	MESH,		VAR,		double,		fScallopPower,		0,				0,			DBL_MAX,	NULL,		0)
PROPDEF(	MESH,		VAR,		double,		fScallopPulseWidth,	0.5,			0,			0,			NULL,		0)
PROPDEF(	MESH,		VAR,		double,		fScallopSlew,		0.5,			0,			0,			NULL,		0)
PROPDEF(	MESH,		VAR,		double,		fRipples,			0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		VAR,		double,		fRippleDepth,		0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		VAR,		double,		fRipplePhase,		0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		ENUM,		int,		iRippleMotif,		0,				0,			0,			m_Motif,	_countof(m_Motif))
PROPDEF(	MESH,		VAR,		double,		fTwist,				0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		VAR,		double,		fBends,				0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		VAR,		double,		fBendDepth,			0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		VAR,		double,		fBendPhase,			0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		ENUM,		int,		iBendMotif,			0,				0,			0,			m_Motif,	_countof(m_Motif))
PROPDEF(	MESH,		VAR,		double,		fBendPoles,			0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		VAR,		double,		fBendPolePhase,		0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		ENUM,		int,		iBendPoleMotif,		0,				0,			0,			m_Motif,	_countof(m_Motif))
PROPDEF(	MESH,		VAR,		double,		fHelixFrequency,	0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		VAR,		double,		fHelixAmplitude,	0,				0,			0,			NULL,		0)
PROPDEF(	MESH,		VAR,		double,		fAspectRatio,		1,				0.1,		10.0,		NULL,		0)
PROPDEF(	TEXTURE,	FILE,		CString,	sTexturePath,		_T(""),			0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	VAR,		int,		nColors,			DEFAULT_COLORS,	1,			256,		NULL,		0)
PROPDEF(	TEXTURE,	VAR,		int,		nColorSharpness,	100,			1,			100,		NULL,		0)
PROPDEF(	TEXTURE,	VAR,		double,		fColorCycles,		5.0,			0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	VAR,		double,		fStripeFrequency,	1,				0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	VAR,		double,		fStripeAmplitude,	0.125,			0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	VAR,		double,		fStripePhase,		0,				0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	VAR,		double,		fPetals,			6.0,			0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	ENUM,		int,		iColorPattern,		0,				0,			0,			m_ColorPattern,	_countof(m_ColorPattern))
PROPDEF(	TEXTURE,	ENUM,		int,		iPaletteType,		0,				0,			0,			m_PaletteType,	_countof(m_PaletteType))
PROPDEF(	TEXTURE,	VAR,		double,		fOffsetU,			0,				0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	VAR,		double,		fOffsetV,			0,				0,			0,			NULL,		0)
PROPDEF(	TEXTURE,	VAR,		double,		fCyclesV,			1,				0,			0,			NULL,		0)
PROPDEF(	VIEW,		VAR,		bool,		bAnimation,			0,				0,			0,			NULL,		0)
PROPDEF(	VIEW,		VAR,		double,		fAutoRotateYaw,		0,				0,			0,			NULL,		0)
PROPDEF(	VIEW,		VAR,		double,		fAutoRotatePitch,	12,				0,			0,			NULL,		0)
PROPDEF(	VIEW,		VAR,		double,		fAutoRotateRoll,	0,				0,			0,			NULL,		0)
PROPDEF(	VIEW,		VAR,		double,		fFrameRate,			30,				0.01,		100.0,		NULL,		0)
PROPDEF(	VIEW,		COLOR,		COLORREF,	clrBackground,		0xffffff,		0,			0,			NULL,		0)

#undef PROPDEF
#endif

#ifdef PATTERNDEF

// append only, to maintain compatibility with previously saved documents
PATTERNDEF(	STRIPES)
PATTERNDEF(	RINGS)
PATTERNDEF(	PETALS)
PATTERNDEF(	POLAR)

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
