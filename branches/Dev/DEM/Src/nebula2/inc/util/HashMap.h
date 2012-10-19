#ifndef N_HASHMAP2_H
#define N_HASHMAP2_H
//------------------------------------------------------------------------------
/**
    @class CHashMap
    @ingroup Util
    @brief A simple associative array, mapping strings (byte blocks)
    to arbitrary values.

    (C) 2005 RadonLabs GmbH
*/

#include "kernel/ntypes.h"
#include <util/Hash.h>

//------------------------------------------------------------------------------
template<class TVal>
class CHashMap
{
protected:
    // Hashmap configuration values
    static const int DEFAULT_SIZE = 32;
    // someone tell me, why the above does not work with floats ...
#define GROW_FACTOR    2.0f
#define GROW_THRESHOLD 1.5f

	/// Hashmap key-value-pair
    /// Contains key and value, the keys hash, value and prev/next links for overflowing buckets.
    struct Node
    {
        Node(unsigned int hash, const void* key, size_t len, const TVal& val) :
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

        ~Node() { n_free(key); }

        // the keys hash value
        unsigned int hash;

        // key and value
        void* key;
        size_t len;
        TVal value;

        // bucket list prev/next links
        Node* prev;
        Node* next;
    };

    /// Lookup an entry in the hashmap.
    Node* Find(unsigned int hash, const void* key, size_t len) const;
    /// Insert a new entry into the hashmap.
    void Insert(Node* node);
    /// Grow the hashmap.
    void Grow(size_t capacity);

    /// Default value for new entries
    const TVal defaultValue;

    /// Current number of buckets
    size_t capacity;
    /// Current number of key-value-pairs
    size_t count;

    /// Bucket array
    Node** buckets; //???lists or sorted arrays?

public:
    /// default constructor
    //CHashMap(const TVal& defaultValue = TVal());
    /// constructor with initial hash capacity
    CHashMap(const TVal& defaultValue = TVal(), int capacity = DEFAULT_SIZE);
    /// destructor
    ~CHashMap();

	void		Add(const char* key, const TVal& value) { Add(key, strlen(key), value); }
	const TVal&	At(const char* key) const { return At(key, strlen(key)); } // Entry must exist
	TVal&		At(const char* key) { return At(key, strlen(key)); } // Entry will be added
	void		Remove(const char* key) { Remove(key, strlen(key)); }
	bool		Contains(const char* key) const { return Contains(key, strlen(key)); }
	bool		Get(const char* key, TVal& Value) const;
	TVal*		Get(const char* key) const;

    void		Add(const void* key, size_t len, const TVal& value);
    const TVal&	At(const void* key, size_t len) const;
    TVal&		At(const void* key, size_t len);
    void		Remove(const void* key, size_t len);
	bool		Contains(const void* key, size_t len) const { return !!Find(Hash(key, len), key, len); }

	int			Size() const { return count; }

    const TVal&	operator[](const char* key) const { return At(key); }
	TVal&		operator[](const char* key) { return At(key); }
};


//------------------------------------------------------------------------------
/**
*/
template<class TVal>
CHashMap<TVal>::CHashMap(const TVal& defaultVal, int capacity) :
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
template<class TVal>
CHashMap<TVal>::~CHashMap()
{
    for (size_t b = 0; b < this->capacity; ++b)
    {
        Node* node = this->buckets[b];
        Node* next;

        while (node)
        {
            next = node->next;
            n_delete(node);
            node = next;
        }
    }

    n_free(this->buckets);
}

//------------------------------------------------------------------------------
/**
*/
template<class TVal>
typename CHashMap<TVal>::Node*
CHashMap<TVal>::Find(unsigned int hash, const void* key, size_t len) const
{
    Node* node = this->buckets[hash % this->capacity];

    while (node)
    {
        if (hash == node->hash && len == node->len && !memcmp(key, node->key, len))
            return node;
        node = node->next;
    }

    return 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TVal>
void
CHashMap<TVal>::Insert(typename CHashMap<TVal>::Node* node)
{
    int bucket = node->hash % this->capacity;

    node->next = this->buckets[bucket];
    if (node->next)
        node->next->prev = node;

    node->prev = 0;
    this->buckets[bucket] = node;
	this->count++;
}

//------------------------------------------------------------------------------
/**
*/
template<class TVal>
void
CHashMap<TVal>::Grow(size_t capacity)
{
    if (capacity > this->capacity)
    {
        Node** old_buckets = this->buckets;
        size_t old_capacity = this->capacity;

        this->buckets = (CHashMap<TVal>::Node**)n_calloc(capacity, sizeof(Node*));
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
template<class TVal>
void
CHashMap<TVal>::Add(const void* key, size_t len, const TVal& value)
{
    unsigned int hash = Hash(key, len);

    n_assert(!Find(hash, key, len));

    if (this->count > (size_t)(this->capacity * GROW_THRESHOLD))
        this->Grow((size_t)(this->capacity * GROW_FACTOR));

    Node* node = n_new(Node(hash, key, len, value));
    this->Insert(node);
}

//------------------------------------------------------------------------------

template<class TVal>
const TVal& CHashMap<TVal>::At(const void* key, size_t len) const
{
	unsigned int hash = Hash(key, len);
	Node* node = Find(hash, key, len);
	n_assert(node);
	return node->value;
}
//---------------------------------------------------------------------

/**
*/
template<class TVal>
TVal&
CHashMap<TVal>::At(const void* key, size_t len)
{
    unsigned int hash = Hash(key, len);

    Node* node = this->Find(hash, key, len);

    if (!node)
    {
        if (this->count > (size_t)(this->capacity * GROW_THRESHOLD))
        {
            this->Grow((size_t)(this->capacity * GROW_FACTOR));
        }

        node = n_new(Node(hash, key, len, this->defaultValue));
        this->Insert(node);
    }

    return node->value;
}

//------------------------------------------------------------------------------
/**
*/
template<class TVal>
void
CHashMap<TVal>::Remove(const void* key, size_t len)
{
	Node* node = this->Find(Hash(key, len), key, len);
	if (node)
	{
		if (node->prev) node->prev->next = node->next;
		if (node->next) node->next->prev = node->prev;
		n_delete(node);
	}
}
//------------------------------------------------------------------------------

template<class TVal>
bool CHashMap<TVal>::Get(const char* key, TVal& Value) const
{
	size_t len = strlen(key);
	Node* node = this->Find(Hash(key, len), key, len);
	if (!node) return false;
	Value = node->value;
	return true;
}
//---------------------------------------------------------------------

/**
*/
template<class TVal>
TVal*
CHashMap<TVal>::Get(const char* key) const
{
	size_t len = strlen(key);
    unsigned int hash = Hash(key, len);

    Node* node = this->Find(hash, key, len);
	if (node) return &node->value;
	return NULL;
}


//------------------------------------------------------------------------------
#endif
