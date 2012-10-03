#pragma once
#ifndef __DEM_L1_HASH_TABLE_H__
#define __DEM_L1_HASH_TABLE_H__

#include <util/nfixedarray.h>
#include <util/narray.h>
#include <util/nkeyvaluepair.h>
#include <util/Hash.h>

// The Util::String class implements HashCode for strings, see it!

//(C) 2006 Radon Labs GmbH
// Backported from Nebula 3

//???write Grow()?

//namespace Util
//{

#define NEBULA3_BOUNDSCHECKS 1

template<class KEYTYPE, class VALUETYPE>
class HashTable
{
public:

	HashTable(): hashArray(128), size(0) {}
    HashTable(int capacity): hashArray(capacity), size(0) {}
    /// copy constructor
	HashTable(const HashTable<KEYTYPE, VALUETYPE>& rhs): hashArray(rhs.hashArray), size(rhs.size) {}
    /// assignment operator
    void operator=(const HashTable<KEYTYPE, VALUETYPE>& rhs);
    /// read/write [] operator, assertion if key not found
    VALUETYPE& operator[](const KEYTYPE& key) const;
    /// return current number of values in the hashtable
	int Size() const { return size; }
    /// return fixed-size capacity of the hash table
	int Capacity() const { return hashArray.Size(); }
    /// clear the hashtable
    void Clear();
    /// return true if empty
	bool IsEmpty() const { return !size; }
    /// add a key/value pair object to the hash table
    void Add(const nKeyValuePair<KEYTYPE, VALUETYPE>& kvp);
    /// add a key and associated value
    void Add(const KEYTYPE& key, const VALUETYPE& value);
    /// erase an entry
    void Erase(const KEYTYPE& key);
    /// return true if key exists in the nArray
    bool Contains(const KEYTYPE& key) const;

	bool		Get(const KEYTYPE& key, VALUETYPE& Value) const;
	VALUETYPE*	Get(const KEYTYPE& key) const;

	/// return nArray of all key/value pairs in the table (slow)
    nArray<nKeyValuePair<KEYTYPE, VALUETYPE> > Content() const;

private:
    nFixedArray<nArray<nKeyValuePair<KEYTYPE, VALUETYPE> > > hashArray;
    int size;
};

template<class KEYTYPE, class VALUETYPE>
void HashTable<KEYTYPE, VALUETYPE>::operator =(const HashTable<KEYTYPE, VALUETYPE>& rhs)
{
	if (this != &rhs)
	{
		hashArray = rhs.hashArray;
		size = rhs.size;
	}
}
//---------------------------------------------------------------------

/**
*/
template<class KEYTYPE, class VALUETYPE>
VALUETYPE&
HashTable<KEYTYPE, VALUETYPE>::operator[](const KEYTYPE& key) const
{
    // get hash code from key, trim to capacity
    int hashIndex = Hash(key) % this->hashArray.Size();
    const nArray<nKeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
    int numHashElements = hashElements.Size();
    #if NEBULA3_BOUNDSCHECKS    
    n_assert(0 != numHashElements); // element with key doesn't exist
    #endif
    if (1 == numHashElements)
    {
        // no hash collisions, just return the only existing element
		n_assert(hashElements[0].Key() == key);
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
/**
*/
template<class KEYTYPE, class VALUETYPE>
void HashTable<KEYTYPE, VALUETYPE>::Clear()
{
	for (int i = 0; i < hashArray.Size(); i++)
		hashArray[i].Clear();
	size = 0;
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Add(const nKeyValuePair<KEYTYPE, VALUETYPE>& kvp)
{
    #if NEBULA3_BOUNDSCHECKS
    n_assert(!this->Contains(kvp.Key()));
    #endif
    int hashIndex = Hash(kvp.Key()) % this->hashArray.Size();
    this->hashArray[hashIndex].InsertSorted(kvp);
    this->size++;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Add(const KEYTYPE& key, const VALUETYPE& value)
{
    nKeyValuePair<KEYTYPE, VALUETYPE> kvp(key, value);
    this->Add(kvp);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Erase(const KEYTYPE& key)
{
    if (this->size > 0)
	{
		int hashIndex = Hash(key) % this->hashArray.Size();
		nArray<nKeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
		int hashElementIndex = hashElements.BinarySearchIndex(key);
		if (-1 != hashElementIndex) // key exists
		{
			hashElements.Erase(hashElementIndex);
			this->size--;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
HashTable<KEYTYPE, VALUETYPE>::Contains(const KEYTYPE& key) const
{
    if (this->size > 0)
    {
        int hashIndex = Hash(key) % this->hashArray.Size();
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
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
HashTable<KEYTYPE, VALUETYPE>::Get(const KEYTYPE& key, VALUETYPE& Value) const
{
	if (this->size > 0)
	{
		int hashIndex = Hash(key) % this->hashArray.Size();
		nArray<nKeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
		if (hashElements.Size() == 0) return false;
		else if (hashElements.Size() == 1)
		{
			if (hashElements[0].Key() == key)
			{
				Value = hashElements[0].Value();
				return true;
			}
		}
		else
		{
			int hashElementIndex = hashElements.BinarySearchIndex(key);
			if (-1 != hashElementIndex)
			{
				Value = hashElements[hashElementIndex].Value();
				return true;
			}
		}
	}
	return false;
}
//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
VALUETYPE*
HashTable<KEYTYPE, VALUETYPE>::Get(const KEYTYPE& key) const
{
	if (this->size > 0)
	{
		int hashIndex = Hash(key) % this->hashArray.Size();
		nArray<nKeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
		if (hashElements.Size() == 0) return false;
		else if (hashElements.Size() == 1)
		{
			if (hashElements[0].Key() == key) return &hashElements[0].Value();
		}
		else
		{
			int hashElementIndex = hashElements.BinarySearchIndex(key);
			if (-1 != hashElementIndex) return &hashElements[hashElementIndex].Value();
		}
	}
	return NULL;
}
//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
nArray<nKeyValuePair<KEYTYPE, VALUETYPE>> HashTable<KEYTYPE, VALUETYPE>::Content() const
{
	nArray<nKeyValuePair<KEYTYPE, VALUETYPE>> result;
	for (int i = 0; i < hashArray.Size(); i++)
		if (hashArray[i].Size() > 0)
			result.AppendArray(hashArray[i]);
	return result;
}
//
//} // namespace Util
//------------------------------------------------------------------------------

#endif
