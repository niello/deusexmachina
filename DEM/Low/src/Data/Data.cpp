#include "Data.h"
#include <Data/StringID.h>
#include <Data/StringUtils.h>
#include <System/System.h>

namespace Data
{

//???always use CTypeImpl<T>::ToString() { return StringUtils::ToString(GetRef(pObj)); }?
template<> std::string CTypeImpl<bool>::ToString(const void* pObj) const { return StringUtils::ToString(*(bool*)GetPtr(&pObj)); }
template<> std::string CTypeImpl<int>::ToString(const void* pObj) const { return StringUtils::ToString(*(int*)GetPtr(&pObj)); }
template<> std::string CTypeImpl<float>::ToString(const void* pObj) const { return StringUtils::ToString(*(float*)GetPtr(&pObj)); }
template<> std::string CTypeImpl<std::string>::ToString(const void* pObj) const { return StringUtils::ToString(*(std::string*)GetPtr(&pObj)); }
template<> std::string CTypeImpl<CStrID>::ToString(const void* pObj) const { return StringUtils::ToString(*(CStrID*)GetPtr(&pObj)); }

//DEFINE_TYPE(void)
DEFINE_TYPE(bool, false)
DEFINE_TYPE(int, 0)
DEFINE_TYPE(float, 0.f)
static CTypeImpl<std::string> DataType_string; const CType* CTypeImpl<std::string>::Type = &DataType_string; const std::string CTypeImpl<std::string>::DefaultValue{};
DEFINE_TYPE(CStrID, CStrID::Empty)
DEFINE_TYPE(PVOID, nullptr)

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
