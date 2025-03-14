#pragma once
#include "Type.h"
#include <Vectors.h>
#include <StringID.h>
#include <string>
#include <vector>
#include <map>
#include <cassert>

// Variant data type with compile-time extendable type list

typedef std::vector<uint8_t> CBuffer;

namespace Data
{
// CParamsSorted is for faster search, CParams is for source file order preservation
typedef std::pair<CStringID, class CData> CParam;
typedef std::vector<std::pair<CStringID, class CData>> CParams;
typedef std::map<CStringID, class CData> CParamsSorted;
typedef std::vector<class CData> CDataArray;

class CData
{
protected:

	const CType* Type = nullptr;

#ifdef _DEBUG
	union
	{
		void*							Value;
		bool							As_bool;
		int								As_int;
		float							As_float;
		const std::string*				As_string;
		const char*						As_CStrID;
		CBuffer*						As_CBuffer;
		CParams*						As_CParams;
		CDataArray*						As_CDataArray;
		float3*							As_vector3;
		float4*							As_vector4;
		struct { float m[4][4]; }*		As_matrix44;
	};
#else
	void* Value = nullptr;
#endif

public:

	CData() {}
	CData(const CData& Data) { SetTypeValue(Data); }
	CData(CData&& Data);
	template<class T> CData(const T& Val) { Type = DATA_TYPE(T); DATA_TYPE_NV(T)::NewT(&Value, &Val); }
	explicit CData(const CType* type): Type(type) { if (Type) Type->New(&Value); }
	~CData() { if (Type) Type->Delete(&Value); }

	template<class T> T&		New();
	inline void					Clear();

	const CType*				GetType() const { return Type; }
	int							GetTypeID() const { return Type ? Type->GetID() : INVALID_TYPE_ID; }
	bool						IsA(const CType* Sample) const { return Type == Sample; }
	template<class T> bool		IsA() const { return Type == CType::GetType<T>(); }
	bool						IsValid() const { return Type != nullptr; }
	bool						IsVoid() const { return !Type /*|| Type->GetID() == INVALID_TYPE_ID*/; }
	bool						IsNull() const { return IsVoid() || (!Value && Type != DATA_TYPE(bool) && Type != DATA_TYPE(int)); }

	// Overwrites type
	void						SetType(const CType* SrcType);
	template<class T> void		SetTypeValue(const T& Src);		
	void						SetTypeValue(const CData& Src) { SetTypeValue(Src.Type, &Src.Value); }
	void						SetTypeValue(const CType* SrcType, void* const* pSrcObj);
	
	// Keeps type, uses conversion
	template<class T> void		SetValue(const T& Src);			
	void						SetValue(const CData& Src);
	
	template<class T> bool		GetValue(T& Dest) const;
	template<class T> T&		GetValue();
	template<class T> const T&	GetValue() const;
	template<class T> T*		GetValuePtr();
	template<class T> const T*	GetValuePtr() const;
	void* const*				GetValueObjectPtr() const { return &Value; }

	const char*					ToString() const { return Type ? Type->ToString(Value) : nullptr; }

	CData&						operator =(const CData& Src) { SetTypeValue(Src); return *this; }
	CData&						operator =(CData&& Src);
	template<class T> CData&	operator =(const T& Src) { SetTypeValue(Src); return *this; }
	template<class T> CData&	operator =(T&& Src);

	bool						operator ==(const CData& Other) const;
	template<class T> bool		operator ==(const T& Other) const;
	bool						operator !=(const CData& Other) const { return !(*this == Other); }
	template<class T> bool		operator !=(const T& Other) const { return !(*this == Other); }

	template<class T>			operator T&() { return GetValue<T>(); }
	template<class T>			operator const T&() { return GetValue<T>(); }
	template<class T>			operator const T&() const { return GetValue<T>(); }
	template<class T>			operator T*() { return GetValuePtr<T>(); }
	template<class T>			operator const T*() { return GetValuePtr<T>(); }
	template<class T>			operator const T*() const { return GetValuePtr<T>(); }

	//!!!save, load!
};
//---------------------------------------------------------------------

template<class T> inline T& CData::New()
{
	if (Type) Type->Delete(&Value);
	Type = DATA_TYPE(T);
	DATA_TYPE_NV(T)::New(&Value);
	return GetValue<T>();
}
//---------------------------------------------------------------------

inline void CData::Clear()
{
	if (Type)
	{
		Type->Delete(&Value);
		Type = nullptr;
	}
}
//---------------------------------------------------------------------

// TODO: move instead of copying!
template<class T> CData& CData::operator =(T&& Src)
{
	SetTypeValue(Src);
	return *this;
}
//---------------------------------------------------------------------

template<class T> void CData::SetTypeValue(const T& Src)
{
	if (Type)
		if (Type == DATA_TYPE(T))
		{
			DATA_TYPE_NV(T)::CopyT(&Value, &Src);
			return;
		}
		else Type->Delete(&Value);

	Type = DATA_TYPE(T);
	DATA_TYPE_NV(T)::NewT(&Value, &Src);
}
//---------------------------------------------------------------------

template<class T> void CData::SetValue(const T& Src)
{
	if (Type)
	{
		assert(Type == DATA_TYPE(T)); //!!!need conversion!
		DATA_TYPE_NV(T)::CopyT(&Value, &Src);
	}
	else SetTypeValue(Src);
}
//---------------------------------------------------------------------

template<class T> inline bool CData::GetValue(T& Dest) const
{
	if (!IsVoid() && Type == DATA_TYPE(T)) // no conversions now
	{
		Dest = *(const T*)DATA_TYPE_NV(T)::GetPtr(&Value);
		return true;
	}

	//!!!return converted if !void!
	return false;
}
//---------------------------------------------------------------------

template<class T> inline T& CData::GetValue()
{
	assert(!IsVoid() && Type == DATA_TYPE(T)); // no conversions now
	return *(T*)DATA_TYPE_NV(T)::GetPtr(&Value);
}
//---------------------------------------------------------------------

template<class T> inline const T& CData::GetValue() const
{
	assert(!IsVoid() && Type == DATA_TYPE(T)); // no conversions now
	return *(const T*)DATA_TYPE_NV(T)::GetPtr(&Value);
}
//---------------------------------------------------------------------

template<class T> inline T* CData::GetValuePtr()
{
	assert(Type == DATA_TYPE(T)); // no conversions now
	return (IsNull()) ? nullptr : (T*)DATA_TYPE_NV(T)::GetPtr(&Value);
}
//---------------------------------------------------------------------

template<class T> inline const T* CData::GetValuePtr() const
{
	assert(Type == DATA_TYPE(T)); // no conversions now
	return (IsNull()) ? nullptr : (const T*)DATA_TYPE_NV(T)::GetPtr(&Value);
}
//---------------------------------------------------------------------

inline bool CData::operator ==(const CData& Other) const
{
	//!!!compare by comparator & return IsEqual!
	if (Type != Other.Type)	return false;
	return (IsVoid() || Type->IsEqual(&Value, &Other.Value));
}
//---------------------------------------------------------------------

template<class T> inline bool CData::operator ==(const T& Other) const
{
	//!!!compare by comparator & return IsEqual!
	if (Type != DATA_TYPE(T)) return false;
	return (IsVoid() || Type->IsEqualT(&Value, &Other));
}
//---------------------------------------------------------------------

};

// Std types
//DECLARE_TYPE(void, 0) // Can use struct CVoid {}; CType* == nullptr for void now.
DECLARE_TYPE(bool, 1)
DECLARE_TYPE(int, 2)
DECLARE_TYPE(float, 3)
DECLARE_TYPE(std::string, 4)
DECLARE_TYPE(CStrID, 5)
DECLARE_TYPE(float3, 16)
DECLARE_TYPE(float4, 7)
DECLARE_TYPE(CBuffer, 9)
DECLARE_TYPE(CDataArray, 10)
DECLARE_TYPE(CParams, 11)
