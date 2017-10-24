// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      04may17	initial version
		01		05oct17	add exponentiate operation
		02		09oct17	add pulse and rounded pulse waves
		03		17oct17	add sine cubed and flame waves
		04		20oct17	add triangular pulse wave
		
*/

#ifdef GROUPDEF

GROUPDEF(	MAIN	)

#undef GROUPDEF
#endif

#ifdef PROPDEF

//			group		proptype	type		name				initval			minval		maxval		itemname	items
#ifndef EXCLUDETARGETPROP
PROPDEF(	MAIN,		ENUM,		int,		iTarget,			0,				0,			0,			m_Target,	_countof(m_Target))
#endif
PROPDEF(	MAIN,		ENUM,		int,		iWaveform,			0,				0,			0,			m_Waveform,	_countof(m_Waveform))
PROPDEF(	MAIN,		ENUM,		int,		iOperation,			0,				0,			0,			m_Operation,	_countof(m_Operation))
PROPDEF(	MAIN,		ENUM,		int,		iRange,				0,				0,			0,			m_Range,	_countof(m_Range))
PROPDEF(	MAIN,		ENUM,		int,		iMotif,				0,				0,			0,			CPotProperties::m_Motif,	_countof(CPotProperties::m_Motif))
PROPDEF(	MAIN,		VAR,		double,		fFrequency,			1,				0,			0,			NULL,		0)
PROPDEF(	MAIN,		VAR,		double,		fAmplitude,			1,				0,			0,			NULL,		0)
PROPDEF(	MAIN,		VAR,		double,		fPhase,				0,				0,			0,			NULL,		0)
PROPDEF(	MAIN,		VAR,		double,		fBias,				0,				0,			0,			NULL,		0)
PROPDEF(	MAIN,		VAR,		double,		fPower,				0,				0,			DBL_MAX,	NULL,		0)
PROPDEF(	MAIN,		VAR,		double,		fPhaseSpeed,		0,				0,			0,			NULL,		0)
PROPDEF(	MAIN,		VAR,		double,		fPulseWidth,		0.5,			0,			0,			NULL,		0)
PROPDEF(	MAIN,		VAR,		double,		fSlew,				0.5,			0,			0,			NULL,		0)

#undef PROPDEF
#undef EXCLUDETARGETPROP
#endif

#ifdef WAVEFORMDEF

// append only, to maintain compatibility with previously saved documents
WAVEFORMDEF(	NONE)
WAVEFORMDEF(	SINE)
WAVEFORMDEF(	TRIANGLE)
WAVEFORMDEF(	RAMP_UP)
WAVEFORMDEF(	RAMP_DOWN)
WAVEFORMDEF(	SQUARE)
WAVEFORMDEF(	PULSE)
WAVEFORMDEF(	ROUNDED_PULSE)
WAVEFORMDEF(	TRIANGULAR_PULSE)
WAVEFORMDEF(	SINE_CUBED)
WAVEFORMDEF(	FLAME)

#undef WAVEFORMDEF
#endif

#ifdef RANGEDEF

// append only, to maintain compatibility with previously saved documents
RANGEDEF(	BIPOLAR)
RANGEDEF(	UNIPOLAR)

#undef RANGEDEF
#endif

#ifdef OPERATIONDEF

// append only, to maintain compatibility with previously saved documents
OPERATIONDEF(	ADD)
OPERATIONDEF(	SUBTRACT)
OPERATIONDEF(	MULTIPLY)
OPERATIONDEF(	DIVIDE)
OPERATIONDEF(	EXPONENTIATE)

#undef OPERATIONDEF
#endif
