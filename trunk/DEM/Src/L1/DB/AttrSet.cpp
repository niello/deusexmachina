#include "AttrSet.h"

namespace DB
{

void CAttrSet::SetAttr(CAttrID AttrID, const CData& Value)
{
	int Idx = Attrs.FindIndex(AttrID);
	if (Idx == INVALID_INDEX) Attrs.Add(AttrID, Value);
	else Attrs.ValueAtIndex(Idx).SetValue(Value);
}
//---------------------------------------------------------------------

const CData& CAttrSet::GetAttr(CAttrID AttrID) const
{
	n_assert(AttrID);
	int Idx = Attrs.FindIndex(AttrID);
	if (Idx != INVALID_INDEX) return Attrs.ValueAtIndex(Idx);
	else return AttrID->GetDefaultValue();
	//else n_error("CAttrSet::GetAttr(): Attr '%s' not found!", AttrID->GetName().CStr());
	//return Attrs[0]; // silence the compiler
}
//---------------------------------------------------------------------

};
