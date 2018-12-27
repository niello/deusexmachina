#include "Data.h"

#include <Data/StringID.h>
#include <Data/StringUtils.h>

namespace Data
{

//!!!int, float - rewrite better if possible!
//???always use CTypeImpl<T>::ToString() { return Data::ValueToString<T>(); } and specialize that function?
template<> inline const char* CTypeImpl<bool>::ToString(const void* pObj) const { return pObj ? "true" : "false"; }
template<> inline const char* CTypeImpl<int>::ToString(const void* pObj) const { return StringUtils::FromInt(*((int*)&pObj)); }
template<> inline const char* CTypeImpl<float>::ToString(const void* pObj) const { return StringUtils::FromFloat(*((float*)&pObj)); }
template<> inline const char* CTypeImpl<CString>::ToString(const void* pObj) const { return ((CString*)pObj)->CStr(); }
template<> inline const char* CTypeImpl<CStrID>::ToString(const void* pObj) const { return (const char*)pObj; }

//DEFINE_TYPE(void)
DEFINE_TYPE(bool, false)
DEFINE_TYPE(int, 0)
DEFINE_TYPE(float, 0.f)
DEFINE_TYPE(CString, CString::Empty)
DEFINE_TYPE(CStrID, CStrID::Empty)
DEFINE_TYPE(PVOID, NULL)

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
