// container.h	-- Thatcher Ulrich <tu@tulrich.com> 31 July 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Generic C++ containers.  Problem: STL is murder on compile times,
// and is hard to debug.  And also it's really easy to shoot yourself
// in the foot.  In general I don't like it very much.
//
// These are not as comprehensive or general, but I think might be
// more usable for day-to-day coding.
//
// Basic approach: array<> acts like a stripped-down version of
// std::vector<>, or a slightly beefed up native array.
//
// hash<> is a very simple take on std::hash_map, without the awful
// iterator-oriented interface.


#ifndef CONTAINER_H
#define CONTAINER_H


#include <stdlib.h>
#include <new>	// for placement new
#include "engine/utility.h"
#include "engine/dlmalloc.h"


template<class T>
class array {
// Resizable array.  Don't put anything in here that can't be moved
// around by bitwise copy.  Don't keep the address of an element; the
// array contents will move around as it gets resized.
//
// Default constructor and destructor get called on the elements as
// they are added or removed from the active part of the array.
public:
	array() : m_buffer(0), m_size(0), m_buffer_size(0) {}
	array(int size_hint) : m_buffer(0), m_size(0), m_buffer_size(0) { resize(size_hint); }
	~array() {
		clear();
	}

	// Basic array access.
	T&	operator[](int index) { assert(index >= 0 && index < m_size); return m_buffer[index]; }
	const T&	operator[](int index) const { assert(index >= 0 && index < m_size); return m_buffer[index]; }
	int	size() const { return m_size; }

	void	push_back(const T& val)
	// Insert the given element at the end of the array.
	{
		int	new_size = m_size + 1;
		resize(new_size);
		(*this)[new_size-1] = val;
	}

	// Access the first element.
	T&	front() { return (*this)[0]; }
	const T&	front() const { return (*this)[0]; }

	// Access the last element.
	T&	back() { return (*this)[m_size-1]; }
	const T&	back() const { return (*this)[m_size-1]; }

	void	clear()
	// Empty and destruct all elements.
	{
		resize(0);
	}

	// void	remove(int index);

	void	operator=(const array<T>& a)
	// Array copy.  Copies the contents of a into this array.
	{
		resize(a.size());
		for (int i = 0; i < m_size; i++) {
			*(m_buffer + i) = a[i];
		}
	}

	void	resize(int new_size)
	// Preserve existing elements.  Newly created elements are
	// initialized with default element of T.  Removed elements
	// are destructed.
	{
		assert(new_size >= 0);

		int	old_size = m_size;
		m_size = new_size;

		// Destruct old elements.
		{for (int i = new_size; i < old_size; i++) {
			(m_buffer + i)->~T();
		}}

		if (new_size == 0) {
			m_buffer_size = 0;
		} else if (m_size <= m_buffer_size && m_size > m_buffer_size >> 1) {
			// don't compact yet.
			return;
		} else {
			m_buffer_size = m_size + (m_size >> 2);
		}

		reserve(m_buffer_size);

		// Copy default T into new elements.
		{for (int i = old_size; i < new_size; i++) {
			new (m_buffer + i) T;	// placement new
		}}
	}

	void	reserve(int rsize)
	{
		assert(m_size >= 0);
		m_buffer_size = rsize;

		// Resize the buffer.
		if (m_buffer_size == 0) {
			if (m_buffer) {
				dlfree(m_buffer);
			}
			m_buffer = 0;
		} else {
			if (m_buffer) {
				m_buffer = (T*) dlrealloc(m_buffer, sizeof(T) * m_buffer_size);
			} else {
				m_buffer = (T*) dlmalloc(sizeof(T) * m_buffer_size);
			}
		}			
	}

	void	transfer_members(array<T>* a)
	// UNSAFE!  Low-level utility function: replace this array's
	// members with a's members.
	{
		m_buffer = a->m_buffer;
		m_size = a->m_size;
		m_buffer_size = a->m_buffer_size;

		a->m_buffer = 0;
		a->m_size = 0;
		a->m_buffer_size = 0;
	}

private:
	T*	m_buffer;
	int	m_size;
	int	m_buffer_size;
};


template<class T>
class fixed_size_hash
// Computes a hash of an object's representation.
{
public:
	static int	compute(const T& data)
	{
		unsigned char*	p = (unsigned char*) &data;
		int	size = sizeof(T);

		// Hash function suggested by http://www.cs.yorku.ca/~oz/hash.html
		// Due to Dan Bernstein.  Allegedly very good on strings.
		int	h = 5381;
		while (size > 0) {
			h = ((h << 5) + h) ^ *p;
			p++;
			size--;
		}

		// Alternative: "sdbm" hash function, suggested at same web page above.
		// h = 0;
		// for bytes { h = (h << 16) + (h << 6) - hash + *p; }

		return h;
	}
};


template<class T, class U, class hash_functor = fixed_size_hash<T> >
class hash {
// Fairly stupid hash table.
//
// Never shrinks, unless you explicitly clear() or resize() it.
// Expands on demand, though.  For best results, if you know roughly
// how big your table will be, default it to that size when you create
// it.
public:
	hash() { m_entry_count = 0;  m_size_mask = 0; }
	hash(int size_hint) { m_entry_count = 0; m_size_mask = 0; resize(size_hint); }
	~hash() {
		clear();
	}

	// @@ need a "remove()" or "set()" function, to replace/remove existing key.

	void	add(T key, U value)
	// Add a new value to the hash table, under the specified key.
	{
		assert(get(key, NULL) == false);

		m_entry_count++;
		check_expand();

		int	hash_value = hash_functor::compute(key);
		entry	e;
		e.key = key;
		e.value = value;

		int	index = hash_value & m_size_mask;	// % m_table.size();
		m_table[index].push_back(e);
	}

	void	clear()
	// Remove all entries from the hash table.
	{
		for (int i = 0; i < m_table.size(); i++) {
			m_table[i].clear();
		}
		m_table.clear();
		m_entry_count = 0;
		m_size_mask = 0;
	}


	bool	get(T key, U* value)
	// Retrieve the value under the given key.
	//
	// If there's no value under the key, then return false and leave
	// *value alone.
	//
	// If there is a value, return true, and set *value to the entry's
	// value.
	//
	// If value == NULL, return true or false according to the
	// presence of the key, but don't touch *value.
	{
		if (m_table.size() == 0) {
			return false;
		}

		int	hash_value = hash_functor::compute(key);
		int	index = hash_value % m_table.size();
		for (int i = 0; i < m_table[index].size(); i++) {
			if (m_table[index][i].key == key) {
				if (value) {
					*value = m_table[index][i].value;
				}
				return true;
			}
		}
		return false;
	}

	void	check_expand()
	// Resize the hash table, if it looks like the size is too small
	// for the current number of entries.
	{
		int	new_size = m_table.size();

		if (m_table.size() == 0 && m_entry_count > 0) {
			// Need to expand.  Let's make the base table size 256
			// elements.
			new_size = 256;
		} else if (m_table.size() * 2 < m_entry_count) {
			// Expand.
			new_size = (m_entry_count * 3) >> 1;
		}

		if (new_size != m_table.size()) {
			resize(new_size);
		}
	}


	void	resize(int new_size)
	// Resize the hash table to the given size (Rehash the contents of
	// the current table).
	{
		if (new_size <= 0) {
			// Special case.
			clear();
			return;
		}

		// Force new_size to be a power of two.
		int	bits = fchop(log2((float)(new_size-1)) + 1);
		assert((1 << bits) >= new_size);

		new_size = 1 << bits;
		m_size_mask = new_size - 1;

		array< array<entry> >	new_table(new_size);

		// Copy all entries to the new table.
		{for (int i = 0; i < m_table.size(); i++) {
			for (int j = 0; j < m_table[i].size(); j++) {
				entry&	e = m_table[i][j];
				int	hash_value = hash_functor::compute(e.key);
				
				int	index = hash_value & m_size_mask;	// % new_table.size();
				new_table[index].push_back(e);
			}
			m_table[i].clear();
		}}
		m_table.clear();

		// Replace old table data with new table's data.
		m_table.transfer_members(&new_table);
	}

private:
	struct entry {
		T	key;
		U	value;
	};
	
	int	m_entry_count;
	int	m_size_mask;
	array< array<entry> >	m_table;
};


#endif // CONTAINER_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
