#pragma once
#ifndef __DEM_L1_DB_ATTR_SET_H__
#define __DEM_L1_DB_ATTR_SET_H__

#include "AttrID.h"
#include <util/ndictionary.h>

// A simple container for attributes.
// Based on mangalore AttributeContainer_(C) 2007 Radon Labs GmbH

namespace DB
{

class CAttrSet
{
private:

	nDictionary<CAttrID, CData> Attrs;

	// find index for attribute (SLOW!)
	int FindAttrIndex(CAttrID AttrID) const;

public:

	void			AddAttr(CAttrID AttrID, const CData& Value) { Attrs.Add(AttrID, Value); }
	void			Clear() { Attrs.Clear(); }
	bool			HasAttr(CAttrID AttrID) const { return Attrs.Contains(AttrID); }
	void			SetAttr(CAttrID AttrID, const CData& Value);
	const CData&	GetAttr(CAttrID AttrID) const;
	//tpl Set<>(attrid) { SetAttr(AttrID, Value); }
	//tpl Get<>(attrid) { return Attrs[AttrID].Get<T>(); }
	//tpl Get<>(attrid, default) { return HasAttr() ? GetAttr() : default; }

	const nDictionary<CAttrID, CData>& GetAttrs() const { return Attrs; }
};

}

#endif
