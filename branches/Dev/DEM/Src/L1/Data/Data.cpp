#include "Data.h"

#include <Data/StringID.h>
#include <Data/StringUtils.h>

namespace Data
{

//!!!int, float - rewrite better if possible!
//???always use CTypeImpl<T>::ToString() { return Data::ValueToString<T>(); } and specialize that function?
template<> inline LPCSTR CTypeImpl<bool>::ToString(const void* pObj) const { return pObj ? "true" : "false"; }
template<> inline LPCSTR CTypeImpl<int>::ToString(const void* pObj) const { return StringUtils::FromInt(*((int*)&pObj)); }
template<> inline LPCSTR CTypeImpl<float>::ToString(const void* pObj) const { return StringUtils::FromFloat(*((float*)&pObj)); }
template<> inline LPCSTR CTypeImpl<CString>::ToString(const void* pObj) const { return ((CString*)pObj)->CStr(); }
template<> inline LPCSTR CTypeImpl<CStrID>::ToString(const void* pObj) const { return (LPCSTR)pObj; }

//DEFINE_TYPE(void)
DEFINE_TYPE(bool)
DEFINE_TYPE(int)
DEFINE_TYPE(float)
DEFINE_TYPE(CString)
DEFINE_TYPE(CStrID)
DEFINE_TYPE(PVOID)

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
