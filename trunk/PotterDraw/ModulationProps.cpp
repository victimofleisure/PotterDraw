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
		
*/

#include "stdafx.h"
#include "PotterDraw.h"
#include "ModulationProps.h"
#include "VariantHelper.h"
#include "PotProperties.h"	// for motifs
#include "RegTempl.h"

const CProperties::OPTION_INFO CModulationProps::m_Group[GROUPS] = {
	#define GROUPDEF(name) {_T(#name), IDS_MOD_GROUP_##name}, 
	#include "ModulationPropsDef.h"
};

const CProperties::OPTION_INFO CModulationProps::m_Target[TARGETS] = {
	#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) {NULL, IDS_PDR_NAME_##name},
	#include "PotPropsDef.h"
};

const CProperties::OPTION_INFO CModulationProps::m_Waveform[WAVEFORMS] = {
	#define WAVEFORMDEF(name) {_T(#name), IDS_MOD_OPT_WAVE_##name},
	#include "ModulationPropsDef.h"
};

const CProperties::OPTION_INFO CModulationProps::m_Range[RANGES] = {
	#define RANGEDEF(name) {_T(#name), IDS_MOD_OPT_RANGE_##name},
	#include "ModulationPropsDef.h"
};

const CProperties::OPTION_INFO CModulationProps::m_Operation[OPERATIONS] = {
	#define OPERATIONDEF(name) _T(#name), {IDS_MOD_OPT_OPER_##name},
	#include "ModulationPropsDef.h"
};

const CProperties::OPTION_INFO CModulationProps::m_PowerType[POWER_TYPES] = {
	#define POWERTYPEDEF(name) _T(#name), {IDS_MOD_OPT_POWER_TYPE_##name},
	#include "ModulationPropsDef.h"
};

const CProperties::PROPERTY_INFO CModulationProps::m_Info[PROPERTIES] = {
	#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) \
		{_T(#name), IDS_MOD_NAME_##name, IDS_MOD_DESC_##name, offsetof(CModulationProps, m_##name), \
		sizeof(type), &typeid(type), GROUP_##group, -1, PT_##proptype, items, itemname, minval, maxval},
	#include "ModulationPropsDef.h"
};

const CModulationProps CModulationProps::m_DefaultVals(true);	// ctor overload that initializes

CModulationProps::CModulationProps(bool bInit)
{
	#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) m_##name = initval;
	#include "ModulationPropsDef.h"
}

int CModulationProps::GetGroupCount() const
{
	return GROUPS;
}

int CModulationProps::GetPropertyCount() const
{
	return PROPERTIES;
}

const CModulationProps::PROPERTY_INFO& CModulationProps::GetPropertyInfo(int iProp) const
{
	ASSERT(IsValidProperty(iProp));
	return m_Info[iProp];
}

void CModulationProps::GetVariants(CVariantArray& Var) const
{
	Var.SetSize(PROPERTIES);
	#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) \
		Var[PROP_##name] = CComVariant(m_##name);
	#include "ModulationPropsDef.h"
}

void CModulationProps::SetVariants(const CVariantArray& Var)
{
	#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) \
		GetVariant(Var[PROP_##name], m_##name);
	#include "ModulationPropsDef.h"
}

CString CModulationProps::GetGroupName(int iGroup) const
{
	ASSERT(IsValidGroup(iGroup));
	return LDS(m_Group[iGroup].nNameID);
}

bool CModulationProps::operator==(const CModulationProps& props) const
{
	#define EXCLUDETARGETPROP	// exclude target property
	#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) \
		if (m_##name != m_DefaultVals.m_##name) \
			return false;
	#include "ModulationPropsDef.h"
	return true;
}

void CModulationProps::ReadProperties(LPCTSTR szSection)
{
	#define EXCLUDETARGETPROP	// exclude target property
	#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) \
		if (PT_##proptype == PT_ENUM) \
			ReadEnum(szSection, _T(#name), m_##name, itemname, items); \
		else \
			RdReg(szSection, _T(#name), m_##name);
	#include "ModulationPropsDef.h"
}

void CModulationProps::WriteProperties(LPCTSTR szSection) const
{
	#define EXCLUDETARGETPROP	// exclude target property
	#define PROPDEF(group, subgroup, proptype, type, name, initval, minval, maxval, itemname, items) \
		if (PT_##proptype == PT_ENUM) \
			WriteEnum(szSection, _T(#name), m_##name, itemname, items); \
		else \
			WrReg(szSection, _T(#name), m_##name);
	#include "ModulationPropsDef.h"
}
