// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      04may17	initial version
		01		03nov17	add property subgroup
		02		10nov17	add power types
		03		24nov17	add animated modulation accessor
		04		03jan18	change target enum's prefix to avoid conflicts
		
*/

#pragma once

#include "Properties.h"

class CModulationProps : public CProperties {
public:
// Construction
	CModulationProps();	// default ctor doesn't initialize
	CModulationProps(bool bInit);	// this overload initializes

// Types

// Constants
	enum {	// groups
		#define GROUPDEF(name) GROUP_##name,
		#include "ModulationPropsDef.h"
		GROUPS
	};
	enum {	// targets
		#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) TARG_##name,
		#include "PotPropsDef.h"
		TARGETS
	};
	enum {	// waveforms
		#define WAVEFORMDEF(name) WAVE_##name,
		#include "ModulationPropsDef.h"
		WAVEFORMS
	};
	enum {	// ranges
		#define RANGEDEF(name) RANGE_##name,
		#include "ModulationPropsDef.h"
		RANGES
	};
	enum {	// operations
		#define OPERATIONDEF(name) OPER_##name,
		#include "ModulationPropsDef.h"
		OPERATIONS
	};
	enum {	// operations
		#define POWERTYPEDEF(name) POWER_TYPE_##name,
		#include "ModulationPropsDef.h"
		POWER_TYPES
	};
	enum {	// properties
		#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) PROP_##name,
		#include "ModulationPropsDef.h"
		PROPERTIES
	};
	static const OPTION_INFO	m_Group[GROUPS];	// group names
	static const OPTION_INFO	m_Target[TARGETS];	// target names
	static const OPTION_INFO	m_Waveform[WAVEFORMS];	// waveform names
	static const OPTION_INFO	m_Range[RANGES];	// range names
	static const OPTION_INFO	m_Operation[OPERATIONS];	// operation names
	static const OPTION_INFO	m_PowerType[POWER_TYPES];	// power type names
	static const PROPERTY_INFO	m_Info[PROPERTIES];	// fixed info for each property
	static const CModulationProps	m_DefaultVals;	// default values

// Data members
	#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) type m_##name;
	#include "ModulationPropsDef.h"

// Overrides
	virtual	int		GetGroupCount() const;
	virtual	int		GetPropertyCount() const;
	virtual	const PROPERTY_INFO&	GetPropertyInfo(int iProp) const;
	virtual	void	GetVariants(CVariantArray& Var) const;
	virtual	void	SetVariants(const CVariantArray& Var);
	virtual	CString	GetGroupName(int iGroup) const;

// Attributes
	void	SetDefault();
	bool	IsDefault() const;
	bool	IsModulated() const;
	bool	IsAnimated() const;
	bool	IsAnimatedModulation() const;
	bool	operator==(const CModulationProps& props) const;
	bool	operator!=(const CModulationProps& props) const;

// Operations
	void	ReadProperties(LPCTSTR szSection);
	void	WriteProperties(LPCTSTR szSection) const;
};

inline CModulationProps::CModulationProps()
{
}

inline void CModulationProps::SetDefault()
{
	*this = m_DefaultVals;
}

inline bool CModulationProps::IsDefault() const
{
	return *this == m_DefaultVals;
}

inline bool CModulationProps::operator!=(const CModulationProps& props) const
{
	return !operator==(props);
}

inline bool CModulationProps::IsModulated() const
{
	return m_iWaveform != WAVE_NONE;
}

inline bool CModulationProps::IsAnimated() const
{
	return m_fPhaseSpeed != 0;
}

inline bool CModulationProps::IsAnimatedModulation() const
{
	return IsModulated() && IsAnimated();
}
