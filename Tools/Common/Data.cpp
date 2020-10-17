#include "Data.h"
#include <StringID.h>
#include <cassert>

namespace Data
{

//!!!int, float - rewrite better if possible!
//???always use CTypeImpl<T>::ToString() { return Data::ValueToString<T>(); } and specialize that function?
template<> inline const char* CTypeImpl<bool>::ToString(const void* pObj) const { return pObj ? "true" : "false"; }
template<> inline const char* CTypeImpl<int>::ToString(const void* pObj) const { return std::to_string(*((int*)&pObj)).c_str(); }
template<> inline const char* CTypeImpl<float>::ToString(const void* pObj) const { return std::to_string(*((float*)&pObj)).c_str(); }
template<> inline const char* CTypeImpl<std::string>::ToString(const void* pObj) const { return ((std::string*)pObj)->c_str(); }
template<> inline const char* CTypeImpl<CStrID>::ToString(const void* pObj) const { return (const char*)pObj; }

//DEFINE_TYPE(void)
DEFINE_TYPE(bool, false)
DEFINE_TYPE(int, 0)
DEFINE_TYPE(float, 0.f)
DEFINE_TYPE_EX(std::string, string, std::string{})
DEFINE_TYPE(CStrID, CStrID::Empty)
DEFINE_TYPE(float3, {})
DEFINE_TYPE(float4, {})
DEFINE_TYPE(CBuffer, {})
DEFINE_TYPE(CDataArray, {})
DEFINE_TYPE(CParams, {})

CData::CData(CData&& Data)
	: Type(Data.Type)
	, Value(Data.Value)
{
	Data.Type = nullptr;
	Data.Value = nullptr;
}
//---------------------------------------------------------------------

CData& CData::operator =(CData&& Src)
{
	Type = Src.Type;
	Value = Src.Value;
	Src.Type = nullptr;
	Src.Value = nullptr;
	return *this;
}
//---------------------------------------------------------------------

void CData::SetType(const CType* SrcType)
{
	if (Type) Type->Delete(&Value);
	Type = SrcType;
	if (Type) Type->New(&Value);
	else Value = nullptr;
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
	else Value = nullptr;
}
//---------------------------------------------------------------------

void CData::SetValue(const CData& Src)
{
	assert(Type && Type == Src.GetType()); //!!!need conversion!
	Type->Copy(&Value, &Src.Value);
}
//---------------------------------------------------------------------

};
