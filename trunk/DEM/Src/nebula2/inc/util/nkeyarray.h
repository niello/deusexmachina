#ifndef N_KEYARRAY_H
#define N_KEYARRAY_H
//------------------------------------------------------------------------------
/**
    @class nKeyArray
    @ingroup NebulaDataTypes

    @brief Implements growable array of key-pointer pairs. The array
    is kept sorted for fast bsearch() by key.

    (C) 2002 - 2005 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "util/narray.h"

//------------------------------------------------------------------------------
template<class TYPE> class nKeyArray
{
public:
    /// default constructor
    nKeyArray();
    /// constructor for non-growable array
    nKeyArray(int num);
    /// constructor for growable array
    nKeyArray(int num, int grow);
    /// destructor
    ~nKeyArray();
    /// add key/element pair to array
    void Add(int key, const TYPE& e);
    /// find element associated with given key
    bool Find(int key, TYPE& e) const;
    /// return true if there exist a element with this key
    bool HasKey(int key) const;
    /// remove element defined by key
    void Rem(int key);
    /// remove element defined by key index
    void RemByIndex(int index);
    /// return number of elements
    int Size() const;
    /// element at index
    TYPE& operator[](int index) const;
    /// get element at index
    TYPE& GetElementAt(int index) const;
    /// get element of entry with key
    TYPE& GetElement(int key) const;
    /// get key at index
    int GetKeyAt(int index) const;
    /// clear the array without deallocating memory!
    void Clear();

private:
    struct nKAElement
    {
        int key;
        TYPE elm;
    };

    /// binary search - returns index of element or -1 on error
    int bsearch(int key) const;

    nArray<nKAElement> elements;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
int
nKeyArray<TYPE>::bsearch(int key) const
{
    n_assert(this->elements.Size() > 0);

    int num = this->elements.Size();
    int half;

    int lo = 0;
    int hi = num - 1;
    int mid;

    while (lo <= hi)
    {
        if ((half = num/2))
        {
            mid = lo + ((num & 1) ? half : (half - 1));
            int diff = key - this->elements[mid].key;
            if (diff < 0)
            {
                hi = mid - 1;
                num = num & 1 ? half : half - 1;
            }
            else if (diff > 0)
            {
                lo = mid + 1;
                num = half;
            }
            else
            {
                return mid;
            }
        }
        else if (num)
        {
            int diff = key - this->elements[lo].key;
            if (diff)
            {
                return -1;
            }
            else
            {
                return lo;
            }
        }
        else
        {
            break;
        }
    }
    return -1;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nKeyArray<TYPE>::nKeyArray()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nKeyArray<TYPE>::nKeyArray(int num) :
    elements(num, 0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nKeyArray<TYPE>::nKeyArray(int num, int grow) :
    elements(num, grow)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nKeyArray<TYPE>::~nKeyArray()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nKeyArray<TYPE>::Add(int key, const TYPE& e)
{
    // create new entry
    nKAElement newElement;
    newElement.elm = e;
    newElement.key = key;

    // insert key into array, keep array sorted by key
    int i;
    for (i = 0; i < this->elements.Size(); i++)
    {
        const nKAElement& kae = this->elements[i];
        if (key < kae.key)
        {
            // insert in front of
            this->elements.Insert(i, newElement);
            return;
        }
    }

    // fallthrough: add element to end of array
    this->elements.Append(newElement);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
nKeyArray<TYPE>::Find(int key, TYPE& e) const
{
    if (this->elements.Size() == 0)
    {
        return false;
    }

    // search
    int i = this->bsearch(key);
    if (i >= 0)
    {
        e = this->elements[i].elm;
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
nKeyArray<TYPE>::GetElement(int key) const
{
    n_assert(this->HasKey(key));

    // search
    int i = this->bsearch(key);
    return this->elements[i].elm;
}


//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
nKeyArray<TYPE>::HasKey(int key) const
{
    if (this->elements.Size() == 0)
    {
        return false;
    }

    // search
    int i = this->bsearch(key);
    if (i >= 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nKeyArray<TYPE>::Rem(int key)
{
    int i = this->bsearch(key);
    if (i >= 0)
    {
        this->elements.Erase(i);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nKeyArray<TYPE>::RemByIndex(int index)
{
    n_assert((index >= 0) && (index < this->elements.Size()));
    this->elements.Erase(index);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
int
nKeyArray<TYPE>::Size() const
{
    return this->elements.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
nKeyArray<TYPE>::operator[](int index) const
{
    return this->GetElementAt(index);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
nKeyArray<TYPE>::GetElementAt(int index) const
{
    n_assert((index >= 0) && (index < this->elements.Size()));
    return this->elements[index].elm;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
int
nKeyArray<TYPE>::GetKeyAt(int index) const
{
    n_assert((index >= 0) && (index < this->elements.Size()));
    return this->elements[index].key;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nKeyArray<TYPE>::Clear()
{
    this->elements.Clear();
}

//------------------------------------------------------------------------------
#endif
