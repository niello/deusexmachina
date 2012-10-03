#include "Data.h"

#include "StringID.h"

namespace Data
{
//DEFINE_TYPE(void)
DEFINE_TYPE(bool)
DEFINE_TYPE(int)
DEFINE_TYPE(float)
DEFINE_TYPE(nString)
DEFINE_TYPE(CStrID)
DEFINE_TYPE(PVOID)
DEFINE_TYPE(vector4)
DEFINE_TYPE(matrix44)

void CData::SetType(const CType* SrcType)
{
	if (Type) Type->Delete(&Value);
	Type = SrcType;
	if (Type) Type->New(&Value);
}
//---------------------------------------------------------------------

// Be careful, method is not type-safe
void CData::SetTypeValue(const CType* SrcType, void* const* pSrcObj)
{
	if (Type)
		if (Type == SrcType)
		{
			Type->Copy(&Value, pSrcObj);
			return;
		}
		else Type->Delete(&Value);

	Type = SrcType;
	if (Type) Type->New(&Value, pSrcObj);
}
//---------------------------------------------------------------------

void CData::SetValue(const CData& Src)
{
	n_assert(Type && Type == Src.GetType()); //!!!need conversion!
	Type->Copy(&Value, &Src.Value);
}
//---------------------------------------------------------------------

};
