#pragma once
#ifndef __DEM_L1_PAIR_T_H__
#define __DEM_L1_PAIR_T_H__

#include <StdDEM.h>

// Template key-value pair for use with associative containers

template<class TKey, class TVal>
class CPairT
{
protected:

	TKey Key;
	TVal Value;

public:

	CPairT() {}
	CPairT(const TKey& _Key, const TVal& _Value):  Key(_Key), Value(_Value) {}
	CPairT(const TKey& _Key): Key(_Key) {}
	CPairT(const CPairT<TKey, TVal>& Other): Key(Other.Key), Value(Other.Value) {}

	const TKey&	GetKey() const { return Key; }
	const TVal&	GetValue() const { return Value; }
	TVal&		GetValue() { return Value; }

	void operator =(const CPairT<TKey, TVal>& Other) { Key = Other.Key; Value = Other.Value; }
	bool operator ==(const CPairT<TKey, TVal>& Other) const { return Key == Other.Key; }
	bool operator !=(const CPairT<TKey, TVal>& Other) const { return Key != Other.Key; }
	bool operator >(const CPairT<TKey, TVal>& Other) const { return Key > Other.Key; }
	bool operator >=(const CPairT<TKey, TVal>& Other) const { return Key >= Other.Key; }
	bool operator <(const CPairT<TKey, TVal>& Other) const { return Key < Other.Key; }
	bool operator <=(const CPairT<TKey, TVal>& Other) const { return Key <= Other.Key; }
};

#endif
