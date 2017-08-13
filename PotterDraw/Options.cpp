// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      05apr17	initial version
		
*/

#include "stdafx.h"
#include "resource.h"
#include "RegTempl.h"
#include "Options.h"
#include "VariantHelper.h"

const COptions::OPTION_INFO COptions::m_Group[GROUPS] = {
	#define GROUPDEF(name) {_T(#name), IDS_OPT_GROUP_##name},
	#include "OptionsDef.h"
};

const COptions::PROPERTY_INFO COptions::m_Info[PROPERTIES] = {
	#define PROPDEF(group, proptype, type, name, initval, minval, maxval, itemname, items) \
		{_T(#name), IDS_OPT_NAME_##name, IDS_OPT_DESC_##name, offsetof(COptions, m_##name), \
		sizeof(type), &typeid(type), GROUP_##group, PT_##proptype, items, itemname, minval, maxval},
	#include "OptionsDef.h"
};

COptions::COptions()
{
	#define PROPDEF(group, proptype, type, name, initval, minval, maxval, itemname, items) \
		m_##name = initval;
	#include "OptionsDef.h"
}

int COptions::GetGroupCount() const
{
	return GROUPS;
}

int COptions::GetPropertyCount() const
{
	return PROPERTIES;
}

const COptions::PROPERTY_INFO& COptions::GetPropertyInfo(int iProp) const
{
	ASSERT(IsValidProperty(iProp));
	return m_Info[iProp];
}

void COptions::GetVariants(CVariantArray& Var) const
{
	Var.SetSize(PROPERTIES);
	#define PROPDEF(group, proptype, type, name, initval, minval, maxval, itemname, items) \
		Var[PROP_##name] = CComVariant(m_##name);
	#include "OptionsDef.h"
}

void COptions::SetVariants(const CVariantArray& Var)
{
	#define PROPDEF(group, proptype, type, name, initval, minval, maxval, itemname, items) \
		GetVariant(Var[PROP_##name], m_##name);
	#include "OptionsDef.h"
}

CString COptions::GetGroupName(int iGroup) const
{
	ASSERT(IsValidGroup(iGroup));
	return LDS(m_Group[iGroup].nNameID);
}

void COptions::ReadProperties()
{
	#define PROPDEF(group, proptype, type, name, initval, minval, maxval, itemname, items) \
		RdReg(_T("Options\\")_T(#group), _T(#name), m_##name);
	#include "OptionsDef.h"
}

void COptions::WriteProperties() const
{
	#define PROPDEF(group, proptype, type, name, initval, minval, maxval, itemname, items) \
		WrReg(_T("Options\\")_T(#group), _T(#name), m_##name);
	#include "OptionsDef.h"
}
