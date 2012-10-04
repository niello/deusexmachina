#ifndef N_ARRAY_H
#define N_ARRAY_H
//------------------------------------------------------------------------------
/**
    @class nArray
    @ingroup NebulaDataTypes

    @brief A dynamic array template class, similar to the stl vector class.

    Can also be set to a fixed size (SetFixedSize()) if the size of
    the array is known beforehand. This eliminates the memory overhead
    for pre-allocated elements if the array works in dynamic mode. To
    prevent the array from pre-allocating any memory on construction
    call the nArray(0, 0) constructor.

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/ntypes.h"

#include <algorithm> // std::sort

//------------------------------------------------------------------------------
template<class TYPE> class nArray
{
public:
    typedef TYPE* iterator;

    /// behavior flags
    enum
    {
        DoubleGrowSize = (1<<0),    // when set, grow size doubles each turn
    };

    /// constructor with default parameters
    nArray();
    /// constructor with initial size and grow size
    nArray(int initialSize, int initialGrow);
    /// constructor with initial size, grow size and initial values
    nArray(int initialSize, int initialGrow, const TYPE& initialValue);
    /// copy constructor
    nArray(const nArray<TYPE>& rhs);
    /// destructor
	~nArray() { Delete(); }
    /// assignment operator
    nArray<TYPE>& operator=(const nArray<TYPE>& rhs);
    /// [] operator
	TYPE& operator[](int Idx) const { n_assert(Idx >= 0 && Idx < numElements); return elements[Idx]; }
    /// equality operator
    bool operator==(const nArray<TYPE>& rhs) const;
    /// inequality operator
	bool operator!=(const nArray<TYPE>& rhs) const { return !(*this == rhs); }

    /// set behavior flags
	void SetFlags(int f) { flags = f; }
    /// get behavior flags
	int GetFlags() const { return flags; }
    /// clear contents and set a fixed size
    void SetFixedSize(int size);
    /// push element to back of array
    TYPE& PushBack(const TYPE& elm);
    /// append element to array (synonym for PushBack())
    void Append(const TYPE& elm);
    /// append the contents of an array to this array
    void AppendArray(const nArray<TYPE>& rhs);
    /// reserve 'num' elements at end of array and return pointer to first element
    iterator Reserve(int num, bool Grow = true);
    /// get number of elements in array
	int Size() const { return numElements; }
    /// get overall allocated size of array in number of elements
	int AllocSize() const { return allocSize; }
    /// set element at index, grow array if necessary
    TYPE& Set(int index, const TYPE& elm);
    /// return reference to nth element in array
    TYPE& At(int index);
    /// return reference to first element
	TYPE& Front() const { n_assert(elements && numElements > 0); return elements[0]; }
    /// return reference to last element
	TYPE& Back() const { n_assert(elements && numElements > 0); return elements[numElements - 1]; }
    /// return true if array empty
	bool Empty() const { return !numElements; }
    /// erase element equal to arg
    void EraseElement(const TYPE& elm);
    /// erase element at index
    void Erase(int index);
    /// quick erase, does not call operator= or destructor
    void EraseQuick(int index);
    /// erase element pointed to by iterator
    iterator Erase(iterator iter);
    /// quick erase, does not call operator= or destructor
    iterator EraseQuick(iterator iter);
    /// insert element at index
    void Insert(int index, const TYPE& elm);
    /// insert element into sorted array
    void InsertSorted(const TYPE& elm);
    /// grows/shrinks array (calls constructors/destructors)
    void Resize(int NewAllocSize);
    /// clear array (calls destructors)
    void Clear();
    /// reset array (does NOT call destructors)
	void Reset() { numElements = 0; }
    /// return iterator to beginning of array
	iterator Begin() const { return elements; }
    /// return iterator to end of array
	iterator End() const { return elements + numElements; }
    /// find identical element in array, return iterator
    iterator Find(const TYPE& elm) const;
    /// find identical element in array, return index
    int FindIndex(const TYPE& elm) const;
    /// find identical element in array, return index
	bool Contains(const TYPE& elm) const { return FindIndex(elm) != -1; }
    /// find array range with element
    void Fill(int first, int num, const TYPE& elm);
    /// clear contents and preallocate with new attributes
    void Reallocate(int initialSize, int grow);
    /// returns new array with elements which are not in rhs (slow!)
    nArray<TYPE> Difference(const nArray<TYPE>& rhs) const;
    /// sort the array
	void Sort() { std::sort(Begin(), End()); }
    /// do a binary search, requires a sorted array
    int BinarySearchIndex(const TYPE& elm) const;

private:
    /// check if index is in valid range, and grow array if necessary
    void CheckIndex(int);
    /// construct an element (call placement new)
	void Construct(TYPE* elm) { n_placement_new(elm, TYPE); }
    /// construct an element (call placement new)
    void Construct(TYPE* elm, const TYPE& val);
    /// destroy an element (call destructor without freeing memory)
	void Destroy(TYPE* elm) { elm->~TYPE(); }
    /// copy content
    void Copy(const nArray<TYPE>& src);
    /// delete content
    void Delete();
    /// grow array
    void Grow();
    /// move elements, grows array if needed
    void Move(int fromIndex, int toIndex);
    /// unsafe quick move, does not call operator= or destructor
    void MoveQuick(int fromIndex, int toIndex);

    int growSize;           // grow by this number of elements if array exhausted
    int allocSize;          // number of elements allocated
    int numElements;        // number of elements in array
    int flags;
    TYPE* elements;         // pointer to element array
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nArray<TYPE>::nArray() :
    growSize(16),
    allocSize(0),
    numElements(0),
    flags(0),
	elements(NULL)
{
}

//------------------------------------------------------------------------------
/**
    Note: 'grow' can be zero to create a static preallocated array.
*/
template<class TYPE>
nArray<TYPE>::nArray(int initialSize, int grow) :
    growSize(grow),
    allocSize(initialSize),
    numElements(0),
    flags(0)
{
	n_assert(initialSize >= 0);
	elements = (initialSize > 0) ? (TYPE*)n_malloc(sizeof(TYPE) * this->allocSize) : 0;
}

//------------------------------------------------------------------------------
/**
    Note: 'grow' can be zero to create a static preallocated array.
*/
template<class TYPE>
nArray<TYPE>::nArray(int initialSize, int grow, const TYPE& initialValue) :
	growSize(grow),
	allocSize(initialSize),
	numElements(initialSize),
	flags(0)
{
	n_assert(initialSize >= 0);
	if (initialSize > 0)
	{
		elements = (TYPE*)n_malloc(sizeof(TYPE) * allocSize);
		for (int i = 0; i < initialSize; i++)
			Construct(elements + i, initialValue);
	}
	else elements = NULL;
}
//---------------------------------------------------------------------

/**
*/
template<class TYPE>
void
nArray<TYPE>::Copy(const nArray<TYPE>& src)
{
    n_assert(0 == this->elements);

    this->growSize    = src.growSize;
    this->allocSize   = src.allocSize;
    this->numElements = src.numElements;
    this->flags       = src.flags;
    if (this->allocSize > 0)
    {
        this->elements = (TYPE*)n_malloc(sizeof(TYPE) * this->allocSize);
        for (int i = 0; i < this->numElements; i++)
            this->Construct(this->elements + i, src.elements[i]);
    }
}

//------------------------------------------------------------------------------

template<class TYPE>
void nArray<TYPE>::Delete()
{
	if (elements)
	{
		for (int i = 0; i < numElements; i++)
			Destroy(elements + i);
		n_free(elements);
		elements = NULL;
	}
	growSize = 0;
	allocSize = 0;
	numElements = 0;
	flags = 0;
}
//---------------------------------------------------------------------

/**
    construct an element (call placement new)
*/
template<class TYPE>
inline void
nArray<TYPE>::Construct(TYPE* elm, const TYPE &val)
{
    //!!!!!!
	// FIXME: since the nebula2 code have been assuming nArray to have demand on TYPE::operator =
    // it will be better to keep the assumption to avoid errors
    // copy constructor will better in efficient
	//n_placement_new(elm, TYPE(val));
    n_placement_new(elm, TYPE);
    *elm = val;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nArray<TYPE>::nArray(const nArray<TYPE>& rhs) :
    growSize(0),
    allocSize(0),
    numElements(0),
    elements(0),
    flags(0)
{
    this->Copy(rhs);
}

//------------------------------------------------------------------------------

template<class TYPE>
void nArray<TYPE>::Reallocate(int initialSize, int grow)
{
	Delete();
	growSize = grow;
	allocSize = initialSize;
	numElements = 0;
	elements = (initialSize > 0) ? (TYPE*)n_malloc(sizeof(TYPE) * initialSize) : 0;
}
//---------------------------------------------------------------------

// Set a new fixed size. This will throw away the current content, and
// create preallocate a new array which cannot grow. All elements in
// the array will be valid.
template<class TYPE>
void nArray<TYPE>::SetFixedSize(int size)
{
	Reallocate(size, 0);
	numElements = size;
	for (int i = 0; i < size; i++)
		Construct(this->elements + i);
}
//---------------------------------------------------------------------

template<class TYPE>
nArray<TYPE>& nArray<TYPE>::operator=(const nArray<TYPE>& rhs)
{
	if (this != &rhs)
	{
		Delete();
		Copy(rhs);
	}
	return *this;
}
//---------------------------------------------------------------------

template<class TYPE>
void nArray<TYPE>::Resize(int NewAllocSize)
{
	/*
	for (int i = NewAllocSize; i < numElements; ++i)
		Destroy(elements + i);

	elements = (TYPE*)n_realloc(elements, sizeof(TYPE) * NewAllocSize);
	n_assert(elements);

	allocSize = NewAllocSize;
	numElements = (NewAllocSize < numElements) ? NewAllocSize : numElements;
	*/

	TYPE* newArray = (TYPE*)n_malloc(sizeof(TYPE) * NewAllocSize);

	int NewSize = (NewAllocSize < numElements) ? NewAllocSize : numElements;

	if (elements)
	{
		for (int i = 0; i < NewSize; i++)
		{
			Construct(newArray + i, elements[i]);
			Destroy(elements + i);
		}
		n_free(elements);
	}

	elements = newArray;
	allocSize = NewAllocSize;
	numElements = NewSize;
}
//---------------------------------------------------------------------

template<class TYPE>
void nArray<TYPE>::Grow()
{
	n_assert(growSize > 0);
	Resize((DoubleGrowSize & flags) ? (allocSize ? (allocSize << 1) : growSize) : allocSize + growSize);
}
//---------------------------------------------------------------------

/**
     - 30-Jan-03   floh    serious bugfixes!
     - 07-Dec-04   jo      bugfix: neededSize >= this->allocSize becomes
                                   neededSize > allocSize
*/
template<class TYPE>
void
nArray<TYPE>::Move(int fromIndex, int toIndex)
{
    n_assert(this->elements);
    n_assert(fromIndex < this->numElements);

    // nothing to move?
    if (fromIndex == toIndex) return;

    // compute number of elements to move
    int num = this->numElements - fromIndex;

    // check if array needs to grow
    int neededSize = toIndex + num;
    while (neededSize > this->allocSize)
    {
        this->Grow();
    }

    if (fromIndex > toIndex)
    {
        // this is a backward move
        // create front elements first
        int createCount = fromIndex - toIndex;
        TYPE* from = this->elements + fromIndex;
        TYPE* to = this->elements + toIndex;
        for (int i = 0; i < createCount; i++)
        {
            this->Construct(to + i, from[i]);
        }

        // copy remaining elements
        int copyCount = num - createCount;
        from = this->elements + fromIndex + createCount;
        to = this->elements + toIndex + createCount;
        for (int i = 0; i < copyCount; i++) 
        {
            to[i] = from[i];
        }

        // destroy remaining elements
        for (int i = toIndex + num; i < this->numElements; i++)
        {
            this->Destroy(this->elements + i);
        }
    }
    else
    {
        // this is a forward move
        // create front elements first
        int createCount = toIndex - fromIndex;
        TYPE* from = this->elements + fromIndex + num - createCount;
        TYPE* to = this->elements + fromIndex + num;
        for (int i = 0; i < createCount; i++)
        {
            this->Construct(to + i, from[i]);
        }

        // copy remaining elements, this time backward copy
        int copyCount = num - createCount;
        from = (TYPE*)this->elements + fromIndex;
        to = (TYPE*)this->elements + toIndex;
        for (int i = copyCount - 1; i >= 0; i--) 
        {
            to[i] = from[i];
        }

        // destroy freed elements
        for (int i = fromIndex; i < toIndex; i++)
        {
            this->Destroy(this->elements + i);
        }

        // be aware of these uninitialized element slots
        // as far as I known, this part is only used in nArray::Insert
        // and it will fill in the blank element slot
    }

    // adjust array size
    this->numElements = toIndex + num;
}

//------------------------------------------------------------------------------
/**
    Very fast move which does not call assignment operators or destructors,
    so you better know what you do!
*/
template<class TYPE>
void nArray<TYPE>::MoveQuick(int fromIndex, int toIndex)
{
	if (fromIndex == toIndex) return;
	n_assert(elements);
	n_assert(fromIndex < numElements);
	int num = numElements - fromIndex;
	memmove(elements + toIndex, elements + fromIndex, sizeof(TYPE) * num);
	numElements = toIndex + num;
}
//---------------------------------------------------------------------

template<class TYPE>
TYPE& nArray<TYPE>::PushBack(const TYPE& elm)
{
	if (numElements == allocSize) Grow();
	n_assert(elements);
	Construct(elements + numElements, elm);
	return elements[numElements++];
}
//---------------------------------------------------------------------

template<class TYPE>
void nArray<TYPE>::Append(const TYPE& elm)
{
	if (numElements == allocSize) Grow();
	n_assert(elements);
	Construct(elements + numElements++, elm);
}
//---------------------------------------------------------------------

template<class TYPE>
void nArray<TYPE>::AppendArray(const nArray<TYPE>& rhs)
{
	if (numElements + rhs.numElements > allocSize)
		Resize(numElements + rhs.numElements);
	for (int i = 0; i < rhs.numElements; i++)
		Append(rhs[i]);
}
//---------------------------------------------------------------------

// Make room for N new elements at the end of the array, and return a pointer
// to the start of the reserved area. This can be (carefully!) used as a fast
// shortcut to fill the array directly with data.
template<class TYPE>
typename nArray<TYPE>::iterator nArray<TYPE>::Reserve(int num, bool Grow)
{
	n_assert(num > 0);
	int maxElement = numElements + num;
	if (maxElement > allocSize)
	{
		if (Grow && num < growSize) Resize(numElements + growSize);
		else Resize(maxElement);
	}
	n_assert(elements);
	iterator iter = elements + numElements;
	for (int i = numElements; i < maxElement; i++)
		Construct(elements + i);
	numElements = maxElement;
	return iter;
}
//---------------------------------------------------------------------

// This will check if the provided index is in the valid range. If it is
// not the array will be grown to that index.
template<class TYPE>
void nArray<TYPE>::CheckIndex(int Idx)
{
	if (Idx < numElements) return;
	if (Idx >= allocSize)
	{
		n_assert(growSize > 0);
		Resize(Idx + growSize);
	}
	n_assert(Idx < allocSize);
	for (int i = numElements; i <= Idx; i++) Construct(elements + i);
	numElements = Idx + 1;
}
//---------------------------------------------------------------------

template<class TYPE>
TYPE& nArray<TYPE>::Set(int Idx, const TYPE& elm)
{
	CheckIndex(Idx);
	elements[Idx] = elm;
	return elements[Idx];
}
//---------------------------------------------------------------------

// Access an element. This method may grow the array if the index is
// outside the array range.
template<class TYPE>
inline TYPE& nArray<TYPE>::At(int index)
{
	CheckIndex(index);
	return elements[index];
}
//---------------------------------------------------------------------

template<class TYPE>
bool nArray<TYPE>::operator ==(const nArray<TYPE>& rhs) const
{
	if (rhs.numElements != numElements) return false;
	for (int i = 0; i < numElements; i++)
		if (elements[i] != rhs.elements[i])
			return false;
	return true;
}
//---------------------------------------------------------------------

template<class TYPE>
void nArray<TYPE>::EraseElement(const TYPE& elm)
{
	int Idx = FindIndex(elm);
	if (Idx != -1) Erase(Idx);
}
//---------------------------------------------------------------------

template<class TYPE>
void nArray<TYPE>::Erase(int Idx)
{
	n_assert(elements && Idx >= 0 && Idx < numElements);
	Destroy(elements + Idx);
	if (Idx == numElements - 1) numElements--;
	else Move(Idx + 1, Idx);
}
//---------------------------------------------------------------------

// Quick erase, uses memmove() and does not call assignment operators
// or destructor, so be careful about that!
template<class TYPE>
inline void nArray<TYPE>::EraseQuick(int index)
{
	n_assert(elements && index >= 0 && index < numElements);
	if (index == numElements - 1) numElements--;
	else MoveQuick(index + 1, index);
}
//---------------------------------------------------------------------

template<class TYPE>
inline typename nArray<TYPE>::iterator
nArray<TYPE>::Erase(typename nArray<TYPE>::iterator iter)
{
	if (iter)
	{
		n_assert(elements && iter >= elements && iter < elements + numElements);
		Erase(int(iter - elements));
	}
	return iter;
}
//---------------------------------------------------------------------

// Quick erase, uses memmove() and does not call assignment operators
// or destructor, so be careful about that!
template<class TYPE>
inline typename nArray<TYPE>::iterator
nArray<TYPE>::EraseQuick(typename nArray<TYPE>::iterator iter)
{
	n_assert(elements && iter >= elements && iter < elements + numElements);
	EraseQuick(int(iter - elements));
	return iter;
}
//---------------------------------------------------------------------

template<class TYPE>
void nArray<TYPE>::Insert(int index, const TYPE& elm)
{
	n_assert(index >= 0 && index <= numElements);
	if (index == numElements) PushBack(elm);
	else
	{
		Move(index, index + 1);
		Construct(elements + index, elm);
	}
}
//---------------------------------------------------------------------

template<class TYPE>
void nArray<TYPE>::Clear()
{
	for (int i = 0; i < numElements; i++) Destroy(elements + i);
	numElements = 0;
}
//---------------------------------------------------------------------

template<class TYPE>
typename nArray<TYPE>::iterator nArray<TYPE>::Find(const TYPE& elm) const
{
	for (int i = 0; i < numElements; ++i)
		if (elements[i] == elm)
			return elements + i;
	return NULL;
}
//---------------------------------------------------------------------

template<class TYPE>
int nArray<TYPE>::FindIndex(const TYPE& elm) const
{
	for (int i = 0; i < numElements; ++i)
		if (elements[i] == elm)
			return i;
	return -1;
}
//------------------------------------------------------------------------------

template<class TYPE>
void nArray<TYPE>::Fill(int First, int Num, const TYPE& elm)
{
	n_assert(First >= 0 && First <= numElements && Num >= 0);

	int End = First + Num;
	if (End > numElements)
	{
		if (End > allocSize) Resize(End);
		// fill the tailing elements
		for (int i = numElements; i < End; i++)
			Construct(elements + i, elm);
		// the rest
		End = numElements;
		numElements = First + Num;
	}

	for (int i = First; i < End; i++)
	{
		Destroy(elements + i);
		Construct(elements + i, elm);
	}
}

//------------------------------------------------------------------------------

// Returns a new array with all element which are in rhs, but not in this.
// Be careful, this method may be very slow with large arrays!
template<class TYPE>
nArray<TYPE> nArray<TYPE>::Difference(const nArray<TYPE>& rhs) const
{
	nArray<TYPE> diff;
	for (int i = 0; i < rhs.numElements; i++)
		if (!Find(rhs[i])) diff.Append(rhs[i]);
	return diff;
}
//---------------------------------------------------------------------

/**
    Does a binary search on the array, returns the index of the identical
    element, or -1 if not found
*/
template<class TYPE>
int nArray<TYPE>::BinarySearchIndex(const TYPE& elm) const
{
	if (!numElements) return -1;

	int num = numElements, lo = 0, hi = num - 1;
	while (lo <= hi)
	{
		int half = (num >> 1);
		if (half)
		{
			int mid = lo + half - 1 + (num & 1);
			if (elm < elements[mid])
			{
				hi = mid - 1;
				num = mid - lo;
			}
			else if (elm > elements[mid])
			{
				lo = mid + 1;
				num = half;
			}
			else return mid;
		}
		else return (num && elm == elements[lo]) ? lo : -1;
	}

	return -1;
}

//------------------------------------------------------------------------------
/**
    This inserts the element into a sorted array. In the current
    implementation this is a slow operation O(n). This should be
    optimized to O(log n).
*/
template<class TYPE>
void nArray<TYPE>::InsertSorted(const TYPE& elm)
{
	for (int i = 0; i < numElements; i++)
		if (elm < elements[i])
		{
			Insert(i, elm);
			return;
		}
	Append(elm);
}
//---------------------------------------------------------------------

#endif
