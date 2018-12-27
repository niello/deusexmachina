#pragma once
#ifndef __DEM_L1_PARAM_H__
#define __DEM_L1_PARAM_H__

#include "Data.h"
#include <Data/StringID.h>

// Named variant variable

namespace Data
{

class CParam
{
protected:

	CStrID   Name;	//???what about case-sensitivity for HRDs?
	CData    Value;

public:

	CParam() {}
	CParam(const CParam& Src) { Clone(Src); }
	CParam(CStrID name, const CData& value): Name(name), Value(value) {} 
	
	template<class T> bool		IsA() const { return Value.GetType() == CType::GetType<T>(); }

	CStrID						GetName() const { return Name; }
	CData&						GetRawValue() { return Value; }
	const CData&				GetRawValue() const { return Value; }
	template<class T> T&		GetValue() { return Value.GetValue<T>(); }
	template<class T> const T&	GetValue() const { return Value.GetValue<T>(); }
	//template<class T> T*		GetValuePtr() { return Value.GetValuePtr<T>(); }
	//template<class T> const T*	GetValuePtr() const { return Value.GetValuePtr<T>(); }

	void						SetName(CStrID name) { Name = name; }
	void						SetValue(const CData& value) { Value = value; }
	template<class T> void		SetValue(const T& value) { Value.SetTypeValue(value); } //!!!SetValue!
	template<class T> void		Set(CStrID name, const T& value) { Name = name; Value.SetTypeValue(value); } //!!!SetValue!
	void						Clone(const CParam& Src) { Name = Src.Name; Value.SetTypeValue(Src.Value); } //!!!SetValue!
	inline void					Clear() { Name = CStrID(); Value.Clear(); }

	inline CParam&				operator =(const CParam& Src) { Clone(Src); return *this; }
	//inline bool				operator ==(const CParam& Other) const { return Name == Other.Name && Value == Other.Value; }
	//inline bool				operator !=(const CParam& Other) const { return !(*this == Other); }
};

};

#endif
