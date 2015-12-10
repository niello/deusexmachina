#pragma once
#ifndef __DEM_L1_HASH_PAIR_T_H__
#define __DEM_L1_HASH_PAIR_T_H__

#include <StdDEM.h>
#include <Data/Hash.h>

// Template key-value pair for use with hashed associative containers, it uses (in)equality
// comparison optimization, which speeds up collision resolution

template<class TKey, class TVal>
class CHashPairT
{
protected:

	UPTR KeyHash;
	TKey Key;
	TVal Value;

public:

	CHashPairT() {}
	CHashPairT(const TKey& _Key, const TVal& _Value, UPTR _KeyHash):  Key(_Key), Value(_Value), KeyHash(_KeyHash) {}
	CHashPairT(const TKey& _Key, const TVal& _Value):  Key(_Key), Value(_Value), KeyHash(Hash(_Key)) {}
	CHashPairT(const TKey& _Key): Key(_Key), KeyHash(Hash(_Key)) {}
	CHashPairT(const CHashPairT<TKey, TVal>& Other): Key(Other.Key), Value(Other.Value), KeyHash(Other.KeyHash) {}
	//CHashPairT(const CPairT<TKey, TVal>& Other): Key(Other.Key), Value(Other.Value), KeyHash(Hash(Other.Key)) {}

	const TKey&	GetKey() const { return Key; }
	UPTR		GetKeyHash() const { return KeyHash; }
	const TVal&	GetValue() const { return Value; }
	TVal&		GetValue() { return Value; }

	void operator =(const CHashPairT<TKey, TVal>& Other) { Key = Other.Key; Value = Other.Value; KeyHash = Other.KeyHash; }
	//void operator =(const CPairT<TKey, TVal>& Other) { Key = Other.Key; Value = Other.Value; KeyHash = Hash(Other.Key); }
	bool operator ==(const CHashPairT<TKey, TVal>& Other) const { return KeyHash == Other.KeyHash && Key == Other.Key; }
	bool operator !=(const CHashPairT<TKey, TVal>& Other) const { return KeyHash != Other.KeyHash || Key != Other.Key; }
	bool operator >(const CHashPairT<TKey, TVal>& Other) const { return Key > Other.Key; }
	bool operator >=(const CHashPairT<TKey, TVal>& Other) const { return Key >= Other.Key; }
	bool operator <(const CHashPairT<TKey, TVal>& Other) const { return Key < Other.Key; }
	bool operator <=(const CHashPairT<TKey, TVal>& Other) const { return Key <= Other.Key; }
};

#endif
