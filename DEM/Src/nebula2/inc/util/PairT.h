#ifndef N_KEYVALUEPAIR_H
#define N_KEYVALUEPAIR_H

#include "kernel/ntypes.h"

// Key-value pair objects are used by most associative container classes,
// like Dictionary or HashTable. This has been "backported" from
// the Nebula3 Core Layer.
// (C) 2006 Radon Labs GmbH

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
