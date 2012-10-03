#ifndef N_DICTIONARY_H
#define N_DICTIONARY_H
//------------------------------------------------------------------------------
/**
    @class nDictionary

    A collection of key/value pairs with quick value retrieval
    by key at roughly O(log n). Insertion is O(n), or faster
    if the BeginAdd()/EndAdd() methods are used.
    Find-by-key behavior is undefined if several identical
    keys are added to the array.

    Internally the dictionary is implemented as a sorted array.

    On insertion performance:
    Key/value pairs can be added at any time with the Add() methods.
    This uses Array::InsertSorted() which is roughly O(n). If keys
    are added between BeginAdd()/EndAdd() the keys will be added
    unsorted, and only one Array::Sort() happens in End(). This may
    be faster depending on how many keys are added to the dictionary,
    and how big the dictionary is.

    This class has been backported from the Nebula3 Core Layer.

    (C) 2006 Radon Labs GmbH
*/
#include "util/narray.h"
#include "util/nkeyvaluepair.h"

//------------------------------------------------------------------------------
template<class KEYTYPE, class VALUETYPE> class nDictionary
{
public:
    /// default constructor
    nDictionary();
    /// copy constructor
    nDictionary(const nDictionary<KEYTYPE, VALUETYPE>& rhs);
    /// assignment operator
    void operator=(const nDictionary<KEYTYPE, VALUETYPE>& rhs);
    /// read/write [] operator
    VALUETYPE& operator[](const KEYTYPE& key);
    /// read-only [] operator
    const VALUETYPE& operator[](const KEYTYPE& key) const;
    /// return number of key/value pairs in the nDictionary
    int Size() const;
    /// clear the nDictionary
    void Clear();
    /// return true if empty
    bool IsEmpty() const;
    /// optional: begin adding a batch of key/value pairs to the array
    void BeginAdd();
    /// optional: begin adding a batch of key/value pairs to the array
    void BeginAdd(int num);
    /// optional: end adding a batch of key/value pairs to the array
    void EndAdd();
    /// add a key/value pair
    void Add(const nKeyValuePair<KEYTYPE, VALUETYPE>& kvp);
    /// add a key and associated value
    void Add(const KEYTYPE& key, const VALUETYPE& value);
    /// erase a key and its associated value
    void Erase(const KEYTYPE& key);
    /// erase a key and its associated value at index
    void EraseAt(int index);
    /// find index of key/value pair (-1 if doesn't exist)
    int FindIndex(const KEYTYPE& key) const;
    /// find value by key, returns NULL if not found
	VALUETYPE* Find(const KEYTYPE& key) const;
   /// return true if key exists in the array
    bool Contains(const KEYTYPE& key) const;
    /// get a key at given index
    const KEYTYPE& KeyAtIndex(int index) const;
    /// get a value at given index
    VALUETYPE& ValueAtIndex(int index);
    /// get a value at given index
    const VALUETYPE& ValueAtIndex(int index) const;
    /// get key/value pair at index
    nKeyValuePair<KEYTYPE, VALUETYPE>& nKeyValuePairAtIndex(int index);
    /// get all values as array
    nArray<VALUETYPE> AsArray() const;

private:
    nArray<nKeyValuePair<KEYTYPE, VALUETYPE> > keyValuePairs;
    bool inBeginAdd;
};

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
nDictionary<KEYTYPE, VALUETYPE>::nDictionary() :
    inBeginAdd(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
nDictionary<KEYTYPE, VALUETYPE>::nDictionary(const nDictionary<KEYTYPE, VALUETYPE>& rhs) :
    keyValuePairs(rhs.keyValuePairs),
    inBeginAdd(rhs.inBeginAdd)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
nDictionary<KEYTYPE, VALUETYPE>::operator=(const nDictionary<KEYTYPE, VALUETYPE>& rhs)
{
    this->keyValuePairs = rhs.keyValuePairs;
    this->inBeginAdd = rhs.inBeginAdd;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
nDictionary<KEYTYPE, VALUETYPE>::Clear()
{
    this->keyValuePairs.Clear();
    this->inBeginAdd = false;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
int
nDictionary<KEYTYPE, VALUETYPE>::Size() const
{
    return this->keyValuePairs.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
nDictionary<KEYTYPE, VALUETYPE>::IsEmpty() const
{
    return (0 == this->keyValuePairs.Size());
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
nDictionary<KEYTYPE, VALUETYPE>::BeginAdd()
{
    n_assert(!this->inBeginAdd);
    this->inBeginAdd = true;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
nDictionary<KEYTYPE, VALUETYPE>::BeginAdd(int num)
{
    n_assert(!this->inBeginAdd);
	//!!!keyValuePairs.GrowTo .Reserve(num);
    this->inBeginAdd = true;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
nDictionary<KEYTYPE, VALUETYPE>::EndAdd()
{
    n_assert(this->inBeginAdd);
    this->inBeginAdd = false;
    this->keyValuePairs.Sort();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
nDictionary<KEYTYPE, VALUETYPE>::Add(const nKeyValuePair<KEYTYPE, VALUETYPE>& kvp)
{
    if (this->inBeginAdd)
    {
        // add unsorted, EndAdd() will sort the array
        this->keyValuePairs.Append(kvp);
    }
    else
    {
        // add sorted
        this->keyValuePairs.InsertSorted(kvp);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
nDictionary<KEYTYPE, VALUETYPE>::Add(const KEYTYPE& key, const VALUETYPE& value)
{
    nKeyValuePair<KEYTYPE, VALUETYPE> kvp(key, value);
    this->Add(kvp);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
nDictionary<KEYTYPE, VALUETYPE>::Erase(const KEYTYPE& key)
{
    n_assert(!this->inBeginAdd);
    int eraseIndex = this->keyValuePairs.BinarySearchIndex(key);
    n_assert(-1 != eraseIndex);
    EraseAt(eraseIndex);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
nDictionary<KEYTYPE, VALUETYPE>::EraseAt(int index)
{
    n_assert(!this->inBeginAdd);
    this->keyValuePairs.Erase(index);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
int
nDictionary<KEYTYPE, VALUETYPE>::FindIndex(const KEYTYPE& key) const
{
    n_assert(!this->inBeginAdd);
    return this->keyValuePairs.BinarySearchIndex(key);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
VALUETYPE*
nDictionary<KEYTYPE, VALUETYPE>::Find(const KEYTYPE& key) const
{
    n_assert(!this->inBeginAdd);
	int Idx = keyValuePairs.BinarySearchIndex(key);
	return (Idx == -1) ? NULL : &keyValuePairs[Idx].Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
nDictionary<KEYTYPE, VALUETYPE>::Contains(const KEYTYPE& key) const
{
    return (-1 != this->keyValuePairs.BinarySearchIndex(key));
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline const KEYTYPE&
nDictionary<KEYTYPE, VALUETYPE>::KeyAtIndex(int index) const
{
    return this->keyValuePairs[index].Key();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
VALUETYPE&
nDictionary<KEYTYPE, VALUETYPE>::ValueAtIndex(int index)
{
    return this->keyValuePairs[index].Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
const VALUETYPE&
nDictionary<KEYTYPE, VALUETYPE>::ValueAtIndex(int index) const
{
    return this->keyValuePairs[index].Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
nKeyValuePair<KEYTYPE, VALUETYPE>&
nDictionary<KEYTYPE, VALUETYPE>::nKeyValuePairAtIndex(int index)
{
    return this->keyValuePairs[index];
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
VALUETYPE&
nDictionary<KEYTYPE, VALUETYPE>::operator[](const KEYTYPE& key)
{
    int keyValuePairIndex = this->FindIndex(key);
    n_assert(-1 != keyValuePairIndex);
    return this->keyValuePairs[keyValuePairIndex].Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
const VALUETYPE&
nDictionary<KEYTYPE, VALUETYPE>::operator[](const KEYTYPE& key) const
{
    int keyValuePairIndex = this->FindIndex(key);
    n_assert(-1 != keyValuePairIndex);
    return this->keyValuePairs[keyValuePairIndex].Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
nArray<VALUETYPE>
nDictionary<KEYTYPE, VALUETYPE>::AsArray() const
{
    nArray<VALUETYPE> result;
    int i;
    for (i = 0; i < this->keyValuePairs.Size(); i++)
    {
        result.Append(this->keyValuePairs[i].Value());
    }
    return result;
}

//------------------------------------------------------------------------------
#endif