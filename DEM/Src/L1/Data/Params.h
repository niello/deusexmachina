#pragma once
#ifndef __DEM_L1_PARAMS_H__
#define __DEM_L1_PARAMS_H__

#include <Data/RefCounted.h>
#include <Data/Array.h>
#include <Data/Param.h>

// Array of named variant variables

namespace Data
{

// Can be OR'ed
enum EMergeMethod
{
	Merge_AddNew	= 0x01,	// Adds new fields
	Merge_Replace	= 0x02,	// Overrides existing fields
	Merge_Deep		= 0x04	// Merges nested param lists

	// Deep/TopLevel, only unique array elements
};

typedef Ptr<class CParams> PParams;

class CParams: public Data::CRefCounted
{
private:

	CArray<CParam> Params; //!!!order is important at least for HRDs!

public:

	CParams() {}
	CParams(const CParams& Other): Params(Other.Params) {}
	CParams(int InitialSize, int Grow = 4): Params(InitialSize, Grow) {}

	//???const char*/CString version?
	IPTR						IndexOf(CStrID Name) const;
	bool						Has(CStrID Name) const { return IndexOf(Name) != INVALID_INDEX; }
	UPTR						GetCount() const { return Params.GetCount(); }
	
	CParam&						Get(IPTR Idx) { return Params[Idx]; }
	const CParam&				Get(IPTR Idx) const { return Params[Idx]; }
	template<class T> const T&	Get(IPTR Idx) const { return Params[Idx].GetValue<T>(); }
	CParam&						Get(CStrID Name); //???explicit specialization?
	const CParam&				Get(CStrID Name) const; //???explicit specialization?
	template<class T> const T&	Get(CStrID Name) const;
	template<class T> const T&	Get(CStrID Name, const T& Default) const; //???what about nonconst?
	bool						Get(CParam*& Dest, CStrID Name) const;
	template<class T> bool		Get(T& Dest, CStrID Name) const;
	void						Set(const CParam& Param);
	void						Set(CStrID Name, const CData& Value);
	template<class T> void		Set(CStrID Name, const T& Value) { Set(Name, CData(Value)); }
	void						Clear() { Params.Clear(); }

	void						FromDataDict(const CDataDict& Dict);
	void						ToDataDict(CDataDict& Dict) const;

	void						Merge(const CParams& Other, int Method);
	void						MergeDiff(CParams& OutChangedData, const CParams& Diff) const;
	void						GetDiff(CParams& OutDiff, const CParams& ChangedData) const;
	void						GetDiff(CParams& OutDiff, const CDataDict& ChangedData) const;

	//???also/instead of this: return CData? or even template class
	const CParam&				operator [](CStrID Name) const { return Get(Name); }
	const CParam&				operator [](IPTR Idx) const { return Params[Idx]; }

	//Save Load binary
	//Save Load text
};
//---------------------------------------------------------------------

inline IPTR CParams::IndexOf(CStrID Name) const
{
	for (UPTR i = 0; i < Params.GetCount(); ++i)
		if (Params[i].GetName() == Name) return i;
	return INVALID_INDEX;
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

inline bool CParams::Get(CParam*& Dest, CStrID Name) const
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

template<class T> inline bool CParams::Get(T& Dest, CStrID Name) const
{
	for (UPTR i = 0; i < Params.GetCount(); ++i)
		if (Params[i].GetName() == Name)
			return ((const CData&)Params[i].GetRawValue()).GetValue<T>(Dest);
	FAIL;
}
//---------------------------------------------------------------------

template<>
inline bool CParams::Get<const CData*>(const CData*& Dest, CStrID Name) const
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
inline bool CParams::Get<CData*>(CData*& Dest, CStrID Name) const
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
inline bool CParams::Get<CData>(CData& Dest, CStrID Name) const
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

};

//!!!Tmp while there are no conversions!
inline float GetFloat(const Data::CParams& P, CStrID Name, float Default = 0.f)
{
	Data::CParam* pPrm;
	if (P.Get(pPrm, Name))
		return pPrm->IsA<float>() ? pPrm->GetValue<float>() : (float)pPrm->GetValue<int>();
	return Default;
}
//---------------------------------------------------------------------

//!!!Tmp while there are no conversions!
inline CStrID GetStrID(const Data::CData& Data)
{
	return Data.IsA<CStrID>() ? Data.GetValue<CStrID>() : CStrID(Data.GetValue<CString>().CStr());
}
//---------------------------------------------------------------------

//!!!Tmp while there are no conversions!
inline CStrID GetStrID(const Data::CParams& P, CStrID Name, CStrID Default = CStrID::Empty)
{
	Data::CParam* pPrm;
	if (P.Get(pPrm, Name))
		return pPrm->IsA<CStrID>() ? pPrm->GetValue<CStrID>() : CStrID(pPrm->GetValue<CString>().CStr());
	return Default;
}
//---------------------------------------------------------------------

DECLARE_TYPE(PParams, 11)
#define TParams DATA_TYPE(PParams)

#endif

/*
template<int Size>
class CStaticParams //???public CParams? or both from list?
{
protected:

CParam Params[Size];

public:

//???const char* / CString versions?
inline const CParam* Get(CStrID Name) const;
inline UPTR         IndexOf(CStrID Name) const;
inline bool          Exists(CStrID Name) const {return IndexOf(Name)!=INVALID_INDEX;}

const CParam& operator[](UPTR Index) const
{
assert((Index<Size)&&"Get invalid parameter!");
return Params[Index];
}

const CParam& operator[](CStrID Name) const
{
const CParam* Tmp=Get(Name);
assert(Tmp&&"Get invalid parameter!");
return Tmp;
}

//template<class T>
//void     Add(CStrID id, const T &val);
//???remove?
};
//---------------------------------------------------------------------

template<int Size> inline const CParam* CStaticParams<Size>::Get(CStrID Name) const
{
for (UPTR i=0; i<Size; ++i)
if (Params[i].GetName()==Name) return Params[i];
return NULL;
}
//---------------------------------------------------------------------

template<int Size> inline UPTR CStaticParams<Size>::IndexOf(CStrID Name) const
{
for (UPTR i=0; i<Size; ++i)
if (Params[i].GetName()==Name) return i;
return INVALID_INDEX;
}
//=====================================================================
*/