#pragma once
#include <Data/RefCounted.h>
#include <Data/Array.h>
#include <Data/Param.h>

// Array of named variant variables

namespace Data
{

// Can be OR'ed
enum EMergeMethod
{
	Merge_AddNew      = 0x01, // Adds new fields
	Merge_Replace     = 0x02, // Overrides existing fields
	Merge_Deep        = 0x04, // Merges nested param lists
	Merge_DeleteNulls = 0x08, // Deletes all null fields after merge

	// Deep/TopLevel, only unique array elements
};

typedef Ptr<class CParams> PParams;

class CParams: public CRefCounted
{
private:

	CArray<CParam> Params; //!!!order is important at least for HRDs!

public:

	using value_type = CParam;

	CParams() = default;
	CParams(const CParams& Other) : Params(Other.Params) {}
	CParams(CParams&& Other) noexcept : Params(std::move(Other.Params)) {}
	CParams(int InitialSize, int Grow = 4) : Params(InitialSize, Grow) {}

	//???const char*/CString version?
	IPTR						IndexOf(CStrID Name) const;
	bool						Has(CStrID Name) const { return IndexOf(Name) != INVALID_INDEX; }
	UPTR						GetCount() const { return Params.GetCount(); }
	
	CParam&						Get(IPTR Idx) { return Params[Idx]; }
	const CParam&				Get(IPTR Idx) const { return Params[Idx]; }
	template<class T> const T&	Get(IPTR Idx) const { return Params[Idx].GetValue<T>(); }
	CParam*						Find(CStrID Name);
	const CParam*				Find(CStrID Name) const;
	CParam&						Get(CStrID Name); //???explicit specialization?
	const CParam&				Get(CStrID Name) const; //???explicit specialization?
	template<class T> const T&	Get(CStrID Name) const;
	template<class T> const T&	Get(CStrID Name, const T& Default) const; //???what about nonconst?
	bool						TryGet(CParam*& Dest, CStrID Name) const;
	template<class T> bool		TryGet(T& Dest, CStrID Name) const;
	void						Set(const CParam& Param);
	void						Set(CStrID Name, const CData& Value);
	void						Set(CStrID Name, CData&& Value);
	template<class T> void		Set(CStrID Name, const T& Value) { Set(Name, CData(Value)); }
	template<class T> void		Set(CStrID Name, T&& Value) { Set(Name, CData(std::move(Value))); }
	bool						Remove(CStrID Name);
	void						Clear() { Params.Clear(); }

	void						FromDataDict(const CDataDict& Dict);
	void						ToDataDict(CDataDict& Dict) const;

	void						Merge(const CParams& Other, int Method);
	void						MergeDiff(CParams& OutChangedData, const CParams& Diff) const;
	void						GetDiff(CParams& OutDiff, const CParams& ChangedData) const;
	void						GetDiff(CParams& OutDiff, const CDataDict& ChangedData) const;

	// Range-based loop support
	CParam* begin() const { return Params.begin(); }
	CParam* end() const { return Params.end(); }

	//???also/instead of this: return CData? or even template class
	const CParam&				operator [](CStrID Name) const { return Get(Name); }
	const CParam&				operator [](IPTR Idx) const { return Params[Idx]; }
	//CParam&				        operator [](CStrID Name) { return Get(Name); }
	//CParam&				        operator [](IPTR Idx) { return Params[Idx]; }

	CParams& operator =(const CParams& Other) { Params = Other.Params; return *this; }
	CParams& operator =(CParams&& Other) noexcept { Params = std::move(Other.Params); return *this; }
};
//---------------------------------------------------------------------

inline IPTR CParams::IndexOf(CStrID Name) const
{
	for (UPTR i = 0; i < Params.GetCount(); ++i)
		if (Params[i].GetName() == Name) return i;
	return INVALID_INDEX;
}
//---------------------------------------------------------------------

inline CParam* CParams::Find(CStrID Name)
{
	for (UPTR i = 0; i < Params.GetCount(); ++i)
		if (Params[i].GetName() == Name) return &Params[i];
	return nullptr;
}
//---------------------------------------------------------------------

inline const CParam* CParams::Find(CStrID Name) const
{
	for (UPTR i = 0; i < Params.GetCount(); ++i)
		if (Params[i].GetName() == Name) return &Params[i];
	return nullptr;
}
//---------------------------------------------------------------------

inline CParam& CParams::Get(CStrID Name)
{
	for (UPTR i = 0; i < Params.GetCount(); ++i)
		if (Params[i].GetName() == Name) return Params[i];
	Sys::Error("Param \"%s\" does not exist", Name.CStr());
	return *(n_new(CParam()));
}
//---------------------------------------------------------------------

inline const CParam& CParams::Get(CStrID Name) const
{
	for (UPTR i = 0; i < Params.GetCount(); ++i)
		if (Params[i].GetName() == Name) return Params[i];
	Sys::Error("Param \"%s\" does not exist", Name.CStr());
	return *(n_new(CParam()));
}
//---------------------------------------------------------------------

//???what about nonconst?
template<class T> inline const T& CParams::Get(CStrID Name) const
{
	for (UPTR i = 0; i < Params.GetCount(); ++i)
		if (Params[i].GetName() == Name)
			return ((const CData&)Params[i].GetRawValue()).GetValue<T>();
	Sys::Error("Param \"%s\" does not exist", Name.CStr());
	return *(n_new(T()));
}
//---------------------------------------------------------------------

//???what about nonconst?
template<class T> inline const T& CParams::Get(CStrID Name, const T& Default) const
{
	for (UPTR i = 0; i < Params.GetCount(); ++i)
		if (Params[i].GetName() == Name)
			return ((const CData&)Params[i].GetRawValue()).GetValue<T>();
	return Default;
}
//---------------------------------------------------------------------

inline bool CParams::TryGet(CParam*& Dest, CStrID Name) const
{
	for (UPTR i = 0; i < Params.GetCount(); ++i)
		if (Params[i].GetName() == Name)
		{
			Dest = &Params[i];
			OK;
		}
	FAIL;
}
//---------------------------------------------------------------------

template<class T> inline bool CParams::TryGet(T& Dest, CStrID Name) const
{
	for (UPTR i = 0; i < Params.GetCount(); ++i)
	{
		const CParam& Param = Params[i];
		if (Param.GetName() == Name)
			return Param.GetRawValue().GetValue<T>(Dest);
	}
	FAIL;
}
//---------------------------------------------------------------------

template<>
inline bool CParams::TryGet<const CData*>(const CData*& Dest, CStrID Name) const
{
	for (UPTR i = 0; i < Params.GetCount(); ++i)
		if (Params[i].GetName() == Name)
		{
			Dest = &Params[i].GetRawValue();
			OK;
		}
	FAIL;
}
//---------------------------------------------------------------------

template<>
inline bool CParams::TryGet<CData*>(CData*& Dest, CStrID Name) const
{
	for (UPTR i = 0; i < Params.GetCount(); ++i)
		if (Params[i].GetName() == Name)
		{
			Dest = &Params[i].GetRawValue();
			OK;
		}
	FAIL;
}
//---------------------------------------------------------------------

template<>
inline bool CParams::TryGet<CData>(CData& Dest, CStrID Name) const
{
	for (UPTR i = 0; i < Params.GetCount(); ++i)
		if (Params[i].GetName() == Name)
		{
			Dest = Params[i].GetRawValue();
			OK;
		}
	FAIL;
}
//---------------------------------------------------------------------

inline void CParams::Set(const CParam& Param)
{
	IPTR Idx = IndexOf(Param.GetName());
	if (Idx != INVALID_INDEX) Params[Idx].SetValue(Param.GetRawValue());
	else Params.Add(Param);
}
//---------------------------------------------------------------------

inline void CParams::Set(CStrID Name, const CData& Value)
{
	IPTR Idx = IndexOf(Name);
	if (Idx != INVALID_INDEX) Params[Idx].SetValue(Value);
	else Params.Add(CParam(Name, Value)); //???can avoid tmp obj creation?
}
//---------------------------------------------------------------------

inline void CParams::Set(CStrID Name, CData&& Value)
{
	IPTR Idx = IndexOf(Name);
	if (Idx != INVALID_INDEX) Params[Idx].SetValue(std::move(Value));
	else Params.Add(CParam(Name, std::move(Value))); //???can avoid tmp obj creation?
}
//---------------------------------------------------------------------

inline bool CParams::Remove(CStrID Name)
{
	const IPTR Idx = IndexOf(Name);
	if (Idx == INVALID_INDEX) FAIL;
	Params.RemoveAt(Idx);
	OK;
}
//---------------------------------------------------------------------

};

//!!!Tmp while there are no conversions!
inline float GetFloat(const Data::CParams& P, CStrID Name, float Default = 0.f)
{
	Data::CParam* pPrm;
	if (P.TryGet(pPrm, Name))
		return pPrm->IsA<float>() ? pPrm->GetValue<float>() : (float)pPrm->GetValue<int>();
	return Default;
}
//---------------------------------------------------------------------

DECLARE_TYPE(PParams, 11)

namespace Data
{

// Type conversion for saving in HRD format
template<typename T, typename Enable = void>
struct hrd_type { using type = void; };

template<typename T>
struct hrd_type<T, typename std::enable_if_t<CTypeID<T>::IsDeclared>> { using type = T; };

// Lossy conversion, remove when extend HRD types to 64 bits
template<typename T>
struct hrd_type<T, typename std::enable_if_t<std::is_integral_v<T> && (sizeof(T) > sizeof(int))>> { using type = int; };

// Lossy conversion, remove when extend HRD types to 64 bits
template<>
struct hrd_type<double> { using type = float; };

template<>
struct hrd_type<std::string> { using type = CString; };

template <typename T>
using THRDType = typename hrd_type<T>::type;

}
