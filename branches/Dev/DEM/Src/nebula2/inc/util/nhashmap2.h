#ifndef N_HASHMAP2_H
#define N_HASHMAP2_H
//------------------------------------------------------------------------------
/**
    @class nHashMap2
    @ingroup Util
    @brief A simple associative array, mapping strings (byte blocks)
    to arbitrary values.

    (C) 2005 RadonLabs GmbH
*/

#include "kernel/ntypes.h"
#include <util/Hash.h>

//------------------------------------------------------------------------------
template<class TYPE>
class nHashMap2
{
protected:
    // Hashmap configuration values
    static const int DEFAULT_SIZE = 32;
    // someone tell me, why the above does not work with floats ...
#define GROW_FACTOR    2.0f
#define GROW_THRESHOLD 1.5f

public:
    /// default constructor
    //nHashMap2(const TYPE& defaultValue = TYPE());
    /// constructor with initial hash capacity
    nHashMap2(const TYPE& defaultValue = TYPE(), int capacity = DEFAULT_SIZE);
    /// destructor
    ~nHashMap2();

    /// Add a key-value-pair.
    void Add(const char* key, const TYPE& value);
    /// Return the reference to a stored value.  The key must already be present.
    const TYPE& At(const char* key) const;
    /// Return the reference to a stored value.  A new entry will be added if necessary.
    TYPE& At(const char* key);
    /// Remove an entry from the hashmap.
    void Remove(const char* key);
    /// Check whether the map contains an entry with a given name.
    bool Contains(const char* key) const;
	/// Get value to the output argument if it exists and returns was operation successful
	bool Get(const char* key, TYPE& Value) const;
	/// Get value ptr if exists, NULL otherwise
	TYPE* Get(const char* key) const;

    /// Add a key-value-pair
    void Add(const void* key, size_t len, const TYPE& value);
    /// Return the reference to a stored value.  The key must already be present.
    const TYPE& At(const void* key, size_t len) const;
    /// Return the reference to a stored value.  A new entry will be added if necessary.
    TYPE& At(const void* key, size_t len);
    /// Remove an entry from the hashmap.
    void Remove(const void* key, size_t len);
    /// Check whether the map contains an entry with a given name.
    bool Contains(const void* key, size_t len) const;

    /// Return the reference to a stored value.  The key must already be present.
    const TYPE& operator[](const char* key) const;
    /// Return the reference to a stored value.  A new entry will be added if necessary.
    TYPE& operator[](const char* key);

	int Size() const { return count; }

private:
    /// Hashmap key-value-pair
    /// Contains key and value, the keys hash, value and prev/next links for overflowing buckets.
    struct Node;
    struct Node
    {
        Node(unsigned int hash, const void* key, size_t len, const TYPE& val) :
            hash(hash),
            len(len),
            value(val),
            prev(0),
            next(0)
        {
            this->key = n_malloc(len);
            n_assert(0 != this->key);
            memcpy(this->key, key, len);
        }

        ~Node()
        {
            free(this->key);
        }

        // the keys hash value
        unsigned int hash;

        // key and value
        void* key;
        size_t len;
        TYPE value;

        // bucket list prev/next links
        Node* prev;
        Node* next;
    };

    /// Compute the hash value of a memory block.
    static unsigned int Hash(const void* key, size_t len);

    /// Lookup an entry in the hashmap.
    Node* Find(unsigned int hash, const void* key, size_t len) const;
    /// Insert a new entry into the hashmap.
    void Insert(Node* node);
    /// Grow the hashmap.
    void Grow(size_t capacity);

    /// Default value for new entries
    const TYPE defaultValue;

    /// Current number of buckets
    size_t capacity;
    /// Current number of key-value-pairs
    size_t count;

    /// Bucket array
    Node** buckets; //???lists or sorted arrays?
};

//------------------------------------------------------------------------------
/**
*/
//template<class TYPE>
//nHashMap2<TYPE>::nHashMap2(const TYPE& defaultVal) :
//    defaultValue(defaultVal),
//    capacity(0),
//    count(0),
//    buckets(0)
//{
//    Grow(DEFAULT_SIZE);
//}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nHashMap2<TYPE>::nHashMap2(const TYPE& defaultVal, int capacity) :
    defaultValue(defaultVal),
    capacity(0),
    count(0),
    buckets(0)
{
    Grow(capacity);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nHashMap2<TYPE>::~nHashMap2()
{
    for (size_t b = 0; b < this->capacity; ++b)
    {
        Node* node = this->buckets[b];
        Node* next;

        while (0 != node)
        {
            next = node->next;
            delete node;
            node = next;
        }
    }

    n_free(this->buckets);
}

//------------------------------------------------------------------------------

template<class TYPE>
inline unsigned int
nHashMap2<TYPE>::Hash(const void* key, size_t len)
{
    return OneAtATimeHash(key, len);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
typename nHashMap2<TYPE>::Node*
nHashMap2<TYPE>::Find(unsigned int hash, const void* key, size_t len) const
{
    Node* node = this->buckets[hash % this->capacity];

    while (0 != node)
    {
        if (hash == node->hash && len == node->len && 0 == memcmp(key, node->key, len))
        {
            return node;
        }

        node = node->next;
    }

    return 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nHashMap2<TYPE>::Insert(typename nHashMap2<TYPE>::Node* node)
{
    int bucket = node->hash % this->capacity;

    node->next = this->buckets[bucket];
    if (0 != node->next)
        node->next->prev = node;

    node->prev = 0;
    this->buckets[bucket] = node;
	this->count++;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nHashMap2<TYPE>::Grow(size_t capacity)
{
    if (capacity > this->capacity)
    {
        Node** old_buckets = this->buckets;
        size_t old_capacity = this->capacity;

        this->buckets = (nHashMap2<TYPE>::Node**)n_calloc(capacity, sizeof(Node*));
        this->capacity = capacity;

        // null pointers aren't all-bits-zero on all architectures
        //for (size_t i = 0; i < capacity; ++i)
        //{
        //    this->buckets[i] = 0;
        //}
		// Is better to call:
		//memset(this->buckets, 0, capacity * sizeof(Node*));

        // reinsert existing key-value-pairs
        for (size_t b = 0; b < old_capacity; ++b)
        {
            Node* node = old_buckets[b];

            while (0 != node)
            {
                Node* next = node->next;
                this->Insert(node);
                node = next;
            }
        }

        n_free(old_buckets);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nHashMap2<TYPE>::Add(const void* key, size_t len, const TYPE& value)
{
    unsigned int hash = this->Hash(key, len);

    n_assert(0 == this->Find(hash, key, len));

    if (this->count > (size_t)(this->capacity * GROW_THRESHOLD))
    {
        this->Grow((size_t)(this->capacity * GROW_FACTOR));
    }

    Node* node = new Node(hash, key, len, value);
    this->Insert(node);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
const TYPE&
nHashMap2<TYPE>::At(const void* key, size_t len) const
{
    unsigned int hash = this->Hash(key, len);

    Node* node = this->Find(hash, key, len);
    n_assert(0 != node);

    return node->value;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE&
nHashMap2<TYPE>::At(const void* key, size_t len)
{
    unsigned int hash = this->Hash(key, len);

    Node* node = this->Find(hash, key, len);

    if (0 == node)
    {
        if (this->count > (size_t)(this->capacity * GROW_THRESHOLD))
        {
            this->Grow((size_t)(this->capacity * GROW_FACTOR));
        }

        node = new Node(hash, key, len, this->defaultValue);
        this->Insert(node);
    }

    return node->value;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nHashMap2<TYPE>::Remove(const void* key, size_t len)
{
	Node* node = this->Find(Hash(key, len), key, len);
	if (node)
	{
		if (node->prev) node->prev->next = node->next;
		if (node->next) node->next->prev = node->prev;
		delete node;
	}
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline bool
nHashMap2<TYPE>::Contains(const void* key, size_t len) const
{
    unsigned int hash = this->Hash(key, len);

    return 0 != this->Find(hash, key, len);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void
nHashMap2<TYPE>::Add(const char* key, const TYPE& value)
{
    this->Add(key, strlen(key), value);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline const TYPE&
nHashMap2<TYPE>::At(const char* key) const
{
    return this->At(key, strlen(key));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline TYPE&
nHashMap2<TYPE>::At(const char* key)
{
    return this->At(key, strlen(key));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void
nHashMap2<TYPE>::Remove(const char* key)
{
    this->Remove(key, strlen(key));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline bool
nHashMap2<TYPE>::Contains(const char* key) const
{
    return this->Contains(key, strlen(key));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
nHashMap2<TYPE>::Get(const char* key, TYPE& Value) const
{
	size_t len = strlen(key);
    unsigned int hash = this->Hash(key, len);

    Node* node = this->Find(hash, key, len);
	if (node)
	{
		Value = node->value;
		return true;
	}
	return false;
}
//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE*
nHashMap2<TYPE>::Get(const char* key) const
{
	size_t len = strlen(key);
    unsigned int hash = this->Hash(key, len);

    Node* node = this->Find(hash, key, len);
	if (node) return &node->value;
	return NULL;
}
//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline TYPE&
nHashMap2<TYPE>::operator[](const char* key)
{
    return this->At(key);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline const TYPE&
nHashMap2<TYPE>::operator[](const char* key) const
{
    return this->At(key);
}

//------------------------------------------------------------------------------
#endif
