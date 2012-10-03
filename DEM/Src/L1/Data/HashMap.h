//#pragma once
//#ifndef __DEM_L1_HASH_MAP_H__
//#define __DEM_L1_HASH_MAP_H__
//
//#include "util/nfixedarray.h"
//#include "util/narray.h"
//#include "util/nkeyvaluepair.h"

// I know it's quite strange to have HashMap & HashTable classes.
// HashTable uses sorted arrays for buckets and this HashMap uses lists.
// Each suits for its purposes, like Array & List themselves.

//namespace Util
//{
/*
#define NEBULA3_BOUNDSCHECKS 1

template<class KEYTYPE, class VALUETYPE> class HashTable
{
public:

	HashTable();
    HashTable(int capacity);
    /// copy constructor
    HashTable(const HashTable<KEYTYPE, VALUETYPE>& rhs);
    /// assignment operator
    void operator=(const HashTable<KEYTYPE, VALUETYPE>& rhs);
    /// read/write [] operator, assertion if key not found
    VALUETYPE& operator[](const KEYTYPE& key) const;
    /// return current number of values in the hashtable
    int Size() const;
    /// return fixed-size capacity of the hash table
    int Capacity() const;
    /// clear the hashtable
    void Clear();
    /// return true if empty
    bool IsEmpty() const;
    /// add a key/value pair object to the hash table
    void Add(const nKeyValuePair<KEYTYPE, VALUETYPE>& kvp);
    /// add a key and associated value
    void Add(const KEYTYPE& key, const VALUETYPE& value);
    /// erase an entry
    void Erase(const KEYTYPE& key);
    /// return true if key exists in the nArray
    bool Contains(const KEYTYPE& key) const;
    /// return nArray of all key/value pairs in the table (slow)
    nArray<nKeyValuePair<KEYTYPE, VALUETYPE> > Content() const;

private:
    nFixedArray<nArray<nKeyValuePair<KEYTYPE, VALUETYPE> > > hashArray;
    int size;
};

//------------------------------------------------------------------------------

template<class KEYTYPE, class VALUETYPE>
HashTable<KEYTYPE, VALUETYPE>::HashTable() :
    hashArray(128),
    size(0)
{
    // empty
}

//------------------------------------------------------------------------------
template<class KEYTYPE, class VALUETYPE>
HashTable<KEYTYPE, VALUETYPE>::HashTable(int capacity) :
    hashArray(capacity),
    size(0)
{
    // empty
}

//------------------------------------------------------------------------------
template<class KEYTYPE, class VALUETYPE>
HashTable<KEYTYPE, VALUETYPE>::HashTable(const HashTable<KEYTYPE, VALUETYPE>& rhs) :
    hashArray(rhs.hashArray),
    size(rhs.size)
{
    // empty
}

//------------------------------------------------------------------------------
template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::operator=(const HashTable<KEYTYPE, VALUETYPE>& rhs)
{
    if (this != &rhs)
    {
        this->hashArray = rhs.hashArray;
        this->size = rhs.size;
    }
}

//------------------------------------------------------------------------------

template<class KEYTYPE, class VALUETYPE>
VALUETYPE&
HashTable<KEYTYPE, VALUETYPE>::operator[](const KEYTYPE& key) const
{
    // get hash code from key, trim to capacity
    int hashIndex = key.HashCode() % this->hashArray.Size();
    const nArray<nKeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
    int numHashElements = hashElements.Size();
    #if NEBULA3_BOUNDSCHECKS    
    n_assert(0 != numHashElements); // element with key doesn't exist
    #endif
    if (1 == numHashElements)
    {
        // no hash collissions, just return the only existing element
        return hashElements[0].Value();
    }
    else
    {
        // here's a hash collision, find the right key
        // with a binary search
        int hashElementIndex = hashElements.BinarySearchIndex(key);
        #if NEBULA3_BOUNDSCHECKS
        n_assert(-1 != hashElementIndex);
        #endif
        return hashElements[hashElementIndex].Value();
    }
}

//------------------------------------------------------------------------------

template<class KEYTYPE, class VALUETYPE>
int
HashTable<KEYTYPE, VALUETYPE>::Size() const
{
    return this->size;
}

//------------------------------------------------------------------------------

template<class KEYTYPE, class VALUETYPE>
int
HashTable<KEYTYPE, VALUETYPE>::Capacity() const
{
    return this->hashArray.Size();
}

//------------------------------------------------------------------------------

template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Clear()
{
    int i;
    int num = this->hashArray.Size();
    for (i = 0; i < num; i++)
    {
        this->hashArray[i].Clear();
    }
    this->size = 0;
}

//------------------------------------------------------------------------------

template<class KEYTYPE, class VALUETYPE>
bool
HashTable<KEYTYPE, VALUETYPE>::IsEmpty() const
{
    return (0 == this->size);
}

//------------------------------------------------------------------------------

template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Add(const nKeyValuePair<KEYTYPE, VALUETYPE>& kvp)
{
    #if NEBULA3_BOUNDSCHECKS
    n_assert(!this->Contains(kvp.Key()));
    #endif
    int hashIndex = kvp.Key().HashCode() % this->hashArray.Size();
    this->hashArray[hashIndex].InsertSorted(kvp);
    this->size++;
}

//------------------------------------------------------------------------------

template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Add(const KEYTYPE& key, const VALUETYPE& value)
{
    nKeyValuePair<KEYTYPE, VALUETYPE> kvp(key, value);
    this->Add(kvp);
}

//------------------------------------------------------------------------------

template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Erase(const KEYTYPE& key)
{
    #if NEBULA3_BOUNDSCHECKS
    n_assert(this->size > 0);
    #endif
    int hashIndex = key.HashCode() % this->hashArray.Size();
    nArray<nKeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
    int hashElementIndex = hashElements.BinarySearchIndex(key);
    #if NEBULA3_BOUNDSCHECKS
    n_assert(-1 != hashElementIndex); // key doesn't exist
    #endif
    hashElements.EraseIndex(hashElementIndex);
    this->size--;
}

//------------------------------------------------------------------------------

template<class KEYTYPE, class VALUETYPE>
bool
HashTable<KEYTYPE, VALUETYPE>::Contains(const KEYTYPE& key) const
{
    if (this->size > 0)
    {
        int hashIndex = key.HashCode() % this->hashArray.Size();
        nArray<nKeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
        int hashElementIndex = hashElements.BinarySearchIndex(key);
        return (-1 != hashElementIndex);
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------

template<class KEYTYPE, class VALUETYPE>
nArray<nKeyValuePair<KEYTYPE, VALUETYPE> >
HashTable<KEYTYPE, VALUETYPE>::Content() const
{
    nArray<nKeyValuePair<KEYTYPE, VALUETYPE> > result;
    int i;
    int num = this->hashArray.Size();
    for (i = 0; i < num; i++)
    {
        if (this->hashArray[i].Size() > 0)
        {
            result.AppendArray(this->hashArray[i]);
        }
    }
    return result;
}
//
//} // namespace Util
//------------------------------------------------------------------------------
*/
//
//#endif
