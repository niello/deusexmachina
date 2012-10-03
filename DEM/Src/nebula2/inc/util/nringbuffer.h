#ifndef N_RINGBUFFER_H
#define N_RINGBUFFER_H
//------------------------------------------------------------------------------
/**
    @class nRingBuffer
    @ingroup NebulaDataTypes

    @brief A ring buffer class.

    (C) 2003 RadonLabs GmbH
*/
#include <memory.h>
#include "kernel/ntypes.h"

//------------------------------------------------------------------------------
template<class TYPE> class nRingBuffer
{
public:
    /// constructor 1
    nRingBuffer(int capacity);
    /// default constructor
    nRingBuffer();
    /// destructor
    ~nRingBuffer();
    /// assignment operator
    nRingBuffer<TYPE>& operator=(const nRingBuffer<TYPE>& src);
    /// initialize, only use when default constructor has been used
    void Initialize(int capacity);
    /// returns true if ringbuffer is valid
    bool IsValid() const;
    /// return true if ringbuffer is empty
    bool IsEmpty() const;
    /// return true if ringbuffer is full
    bool IsFull() const;
    /// add uninitialized element to buffer
    TYPE* Add();
    /// deletes the oldest element
    void DeleteTail();
    /// return pointer to head element
    TYPE* GetHead() const;
    /// return pointer to tail element
    TYPE* GetTail() const;
    /// return pointer to next element
    TYPE* GetNext(TYPE* e) const;
    /// return pointer to previous element
    TYPE* GetPrev(TYPE* e) const;
    /// return pointer to start of ringbuffer array
    TYPE* GetStart() const;
    /// return pointer to end of ringbuffer array
    TYPE *GetEnd() const;

private:
    /// copy content
    void Copy(const nRingBuffer<TYPE>& src);
    /// delete all content
    void Delete();

    TYPE *start;                        // start of ring buffer array
    TYPE *end;                          // last element of ring buffer array
    TYPE *tail;                         // oldest valid element
    TYPE *head;                         // youngest element+1
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nRingBuffer<TYPE>::nRingBuffer(int capacity)
{
    //_num++; // there is always 1 empty element in buffer
    this->start = n_new_array(TYPE, capacity+1);
    this->end   = this->start + capacity;
    this->tail  = this->start;
    this->head  = this->start;
}

//------------------------------------------------------------------------------
/**
    NOTE: you must call Initialize() when using the default constructor!
*/
template<class TYPE>
nRingBuffer<TYPE>::nRingBuffer() :
    start(0),
    end(0),
    tail(0),
    head(0)
{
    // empty
}

//---------------------------------------------------------------
/**
*/
template<class TYPE>
nRingBuffer<TYPE>::~nRingBuffer()
{
    this->Delete();
}

//---------------------------------------------------------------
/**
*/
template<class TYPE>
void
nRingBuffer<TYPE>::Delete()
{
    if (this->start)
    {
        n_delete_array(this->start);
    }
    start = 0;
    end = 0;
    tail = 0;
    head = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nRingBuffer<TYPE>::Copy(const nRingBuffer<TYPE>& src)
{
    if (0 != this->start)
    {
        int capacity = src.end - src.start;
        this->start = n_new_array(TYPE, capacity+1);
        this->end   = this->start + capacity;
        this->tail  = this->start;
        this->head  = this->start;

        memcpy(src.start, this->start, capacity+1);
    }
}

//------------------------------------------------------------------------------
/**
    Initialize with n elements, may only be called when
    default constructor has been used.
*/
template<class TYPE>
void
nRingBuffer<TYPE>::Initialize(int capacity)
{
    n_assert(!this->start);
    //_num++; // there is always 1 empty element in buffer
    this->start = n_new_array(TYPE, capacity+1);
    this->end   = this->start + capacity;
    this->tail  = this->start;
    this->head  = this->start;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nRingBuffer<TYPE>&
nRingBuffer<TYPE>::operator=(const nRingBuffer<TYPE>& src)
{
    this->Delete();
    this->Copy(src);
    return *this;
}

//------------------------------------------------------------------------------
/**
    Return true if ring buffer is valid.
*/
template<class TYPE>
bool
nRingBuffer<TYPE>::IsValid() const
{
    return (0 != this->start);
}

//------------------------------------------------------------------------------
/**
    Checks if ring buffer is empty

    Returns true if head and tail are in the same position otherwise
    false.
*/
template<class TYPE>
bool
nRingBuffer<TYPE>::IsEmpty() const
{
    return (this->head == this->tail);
}

//------------------------------------------------------------------------------
/**
    Checks if ring buffer is full
*/
template<class TYPE>
bool
nRingBuffer<TYPE>::IsFull() const
{
    TYPE* e = this->head;
    if (e == this->end)
    {
        e = this->start;
    }
    else
    {
        e++;
    }

    return (e == this->tail);
}

//------------------------------------------------------------------------------
/**
    Add new unitialized head element to ringbuffer.
*/
template<class TYPE>
TYPE*
nRingBuffer<TYPE>::Add()
{
    n_assert(this->start);
    n_assert(!this->IsFull());
    TYPE *e = this->head;
    if (this->head == this->end)
    {
        this->head = this->start;
    }
    else
    {
        this->head++;
    }
    return e;
}

//------------------------------------------------------------------------------
/**
    Delete the oldest element
*/
template<class TYPE>
void
nRingBuffer<TYPE>::DeleteTail()
{
    n_assert(this->start);
    n_assert(!this->IsEmpty());
    if (this->tail == this->end)
    {
        this->tail = this->start;
    } else
    {
        this->tail++;
    }
}

//------------------------------------------------------------------------------
/**
    Return head element (youngest element).
*/
template<class TYPE>
TYPE*
nRingBuffer<TYPE>::GetHead() const
{
    if (this->head == this->tail)
    {
        // empty ringbuffer
        return 0;
    }
    TYPE *e = this->head - 1;
    if (e < this->start)
    {
        e = this->end;
    }
    return e;
}

//------------------------------------------------------------------------------
/**
    Return tail element (oldest element).
*/
template<class TYPE>
TYPE*
nRingBuffer<TYPE>::GetTail() const
{
    if (this->head == this->tail)
    {
        // empty ringbuffer
        return 0;
    }
    return this->tail;
};

//------------------------------------------------------------------------------
/**
    Get next element (from tail to head).
*/
template<class TYPE>
TYPE*
nRingBuffer<TYPE>::GetNext(TYPE* e) const
{
    n_assert(e);
    n_assert(this->start);
    if (e == this->end)
    {
        e = this->start;
    } else
    {
        e++;
    }
    if (e == this->head)
    {
        return 0;
    } else
    {
        return e;
    }
}

//------------------------------------------------------------------------------
/**
    Get previous element (from head to tail).
*/
template<class TYPE>
TYPE*
nRingBuffer<TYPE>::GetPrev(TYPE* e) const
{
    n_assert(e);
    if (e == tail)
    {
        return 0;
    }

    if (e == start)
    {
        return end;
    }
    else
    {
        return e-1;
    }
}

//------------------------------------------------------------------------------
/**
    Return physical start of ringbuffer array.
    Only useful for accessing the ringbuffer array elements directly.
*/
template<class TYPE>
TYPE*
nRingBuffer<TYPE>::GetStart() const
{
    return this->start;
}

//------------------------------------------------------------------------------
/**
    Return physical end of ringbuffer array.
    Only useful for accessing the ringbuffer array elements directly.
*/
template<class TYPE>
TYPE*
nRingBuffer<TYPE>::GetEnd() const
{
    return this->end;
}

//------------------------------------------------------------------------------
#endif
