#pragma once
#include <Data/Type.h>
#include <StdDEM.h>

// Variant data type with compile-time extendable type list

namespace Data
{
class CStringID;

#ifdef _DEBUG
	class CDataArray;
	class CParams;
	class IBuffer;
#endif

class CData
{
protected:

	const CType*	Type = nullptr;

#ifdef _DEBUG
	union
	{
		void*							Value = nullptr;
		bool							As_bool;
		int								As_int;
		float							As_float;
		std::string*					As_String;
		const char*						As_CStrID;
		const CDataArray*				As_CDataArray;
		const CParams*					As_CParams;
		const IBuffer*				    As_PBuffer;
		struct { float x, y, z; }*		As_vector3;
		struct { float x, y, z, w; }*	As_vector4;
		struct { float m[4][4]; }*		As_matrix44;
	};
#else
	void*			Value = nullptr;
#endif

public:

	CData() = default;
	CData(const CData& Src) { SetTypeValue(Src); }
	CData(CData&& Src) noexcept;
	template<class T> CData(const T& Val) { Type = DATA_TYPE(T); DATA_TYPE_NV(T)::NewT(&Value, &Val); }
	explicit CData(const CType* type) : Type(type) { if (Type) Type->New(&Value); }
	~CData() { if (Type) Type->Delete(&Value); }

	template<class T> T&		New();
	inline void					Clear();

	const CType*				GetType() const { return Type; }
	int							GetTypeID() const { return Type ? Type->GetID() : INVALID_TYPE_ID; }
	bool						IsA(const CType* Sample) const { return Type == Sample; }
	template<class T> bool		IsA() const { return Type == CType::GetType<T>(); }
	bool						IsValid() const { return Type != nullptr; }
	bool						IsVoid() const { return !Type /*|| Type->GetID() == INVALID_TYPE_ID*/; }
	bool						IsNull() const { return IsVoid() || (!Value && Type != DATA_TYPE(bool) && Type != DATA_TYPE(int) && Type != DATA_TYPE(float)); }

	// Overwrites type
	void						SetType(const CType* SrcType);
	template<class T> void		SetTypeValue(const T& Src);		
	void						SetTypeValue(const CData& Src) { SetTypeValue(Src.Type, &Src.Value); }
	void						SetTypeValue(const CType* SrcType, void* const* pSrcObj);
	
	// Keeps type, uses conversion
	template<class T> void		SetValue(const T& Src);			
	void						SetValue(const CData& Src);
	
	template<class T> const T*	As() const { return (Type == CType::GetType<T>()) ? GetValuePtr<T>() : nullptr; }
	template<class T> bool		GetValue(T& Dest) const;
	template<class T> T&		GetValue();
	template<class T> const T&	GetValue() const;
	template<class T> T*		GetValuePtr();
	template<class T> const T*	GetValuePtr() const;
	void* const*				GetValueObjectPtr() const { return &Value; }

	template<typename F>
	decltype(auto) Visit(F Visitor) const
	{
		switch (GetTypeID())
		{
			case Data::CTypeID<bool>::TypeID: return Visitor(GetValue<bool>());
			case Data::CTypeID<int>::TypeID: return Visitor(GetValue<int>());
			case Data::CTypeID<float>::TypeID: return Visitor(GetValue<float>());
			case Data::CTypeID<std::string>::TypeID: return Visitor(GetValue<std::string>());
			case Data::CTypeID<CStrID>::TypeID: return Visitor(GetValue<CStrID>());
			case Data::CTypeID<vector3>::TypeID: return Visitor(GetValue<vector3>());
			case Data::CTypeID<vector4>::TypeID: return Visitor(GetValue<vector4>());
			case Data::CTypeID<matrix44>::TypeID: return Visitor(GetValue<matrix44>());
			case Data::CTypeID<Data::PParams>::TypeID: return Visitor(GetValue<Data::PParams>());
			case Data::CTypeID<Data::PDataArray>::TypeID: return Visitor(GetValue<Data::PDataArray>());
			default: return Visitor(std::monostate{});
		}
	}

	std::string					ToString() const { return Type ? Type->ToString(Value) : std::string{}; }

	CData&                      operator =(const CData& Src) { SetTypeValue(Src); return *this; }
	CData&                      operator =(CData&& Src) noexcept;
	template<class T> CData&	operator =(const T& Src) { SetTypeValue(Src); return *this; }
	template<class T> CData&	operator =(T& Src) { SetTypeValue(Src); return *this; }
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

inline CData::CData(CData&& Src) noexcept
	: Type(Src.Type)
	, Value(Src.Value)

{
	Src.Type = nullptr;
	Src.Value = nullptr;
}
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
		n_assert(Type == DATA_TYPE(T)); //!!!need conversion!
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
	n_assert(!IsVoid() && Type == DATA_TYPE(T)); // no conversions now
	return *(T*)DATA_TYPE_NV(T)::GetPtr(&Value);
}
//---------------------------------------------------------------------

template<class T> inline const T& CData::GetValue() const
{
	n_assert(!IsVoid() && Type == DATA_TYPE(T)); // no conversions now
	return *(const T*)DATA_TYPE_NV(T)::GetPtr(&Value);
}
//---------------------------------------------------------------------

template<class T> inline T* CData::GetValuePtr()
{
	n_assert(Type == DATA_TYPE(T)); // no conversions now
	return (IsNull()) ? nullptr : (T*)DATA_TYPE_NV(T)::GetPtr(&Value);
}
//---------------------------------------------------------------------

template<class T> inline const T* CData::GetValuePtr() const
{
	n_assert(Type == DATA_TYPE(T)); // no conversions now
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

template<class T> inline CData& CData::operator =(T&& Src)
{
	if (Type)
	{
		if (Type == DATA_TYPE(T))
		{
			DATA_TYPE_NV(T)::MoveT(&Value, std::move(Src));
			return *this;
		}
		else Type->Delete(&Value);
	}

	Type = DATA_TYPE(T);
	DATA_TYPE_NV(T)::NewMoveT(&Value, std::move(Src));

	return *this;
}
//---------------------------------------------------------------------

//template<> inline CData& CData::operator =(const CData& Src)
//{
//	SetTypeValue(Src);
//	return *this;
//}
////---------------------------------------------------------------------

template<> inline CData& CData::operator =(CData& Src)
{
	SetTypeValue(Src);
	return *this;
}
//---------------------------------------------------------------------

inline CData& CData::operator =(CData&& Src) noexcept
{
	Clear();
	Type = Src.Type;
	Value = Src.Value;
	Src.Type = nullptr;
	Src.Value = nullptr;
	return *this;
}
//---------------------------------------------------------------------

};

namespace StringUtils
{

inline std::string ToString(const Data::CData& Value)
{
	return Value.ToString();
}
//---------------------------------------------------------------------

}

using CStrID = Data::CStringID;

// Std types
//DECLARE_TYPE(void) //!!!can use struct CVoid {};
DECLARE_TYPE(bool, 1)
DECLARE_TYPE(int, 2)
DECLARE_TYPE(float, 3)
DECLARE_TYPE(std::string, 4)
DECLARE_TYPE(CStrID, 5)
DECLARE_TYPE(PVOID, 6)
