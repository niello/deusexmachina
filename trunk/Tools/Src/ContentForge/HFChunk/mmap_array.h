// mmap_array.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Datatype for making a memory-mapped 2D array.  The data is stored
// on disk, and mapped into memory using mmap (Posix) or MapViewOfFile
// (Win32).  Use this for processing giant heightfield or texture
// datasets "out-of-core".


#ifndef MMAP_ARRAY_H
#define MMAP_ARRAY_H


#include "mmap_util.h"


template<class data_type>
class mmap_array {
// Use this class for dealing with huge 2D arrays.
public:
	mmap_array(int width, int height, bool writeable, const char* filename = NULL) :
		m_width(width),
		m_height(height),
		m_writeable(writeable)
	{
		m_data = mmap_util::map(total_bytes(), m_writeable, filename);
		if (m_data == NULL) {
			throw "mmap_array: can't map memory!";
		}
	}

	~mmap_array()
	{
		mmap_util::unmap(m_data, total_bytes());
	}


	data_type&	get(int x, int z)
	// Get a writable reference to an element.
	{
		assert(m_writeable == true);
		return const_cast<data_type&>(const_cast<const mmap_array<data_type>*>(this)->get(x, z));
	}


	const data_type&	get(int x, int z) const
	// Get a const reference to an element.
	{
		assert(x >= 0 && x < m_width);
		assert(z >= 0 && z < m_height);

		// @@ could do clever indexing here, for better paging locality...

		return ((data_type*) m_data)[x + z * m_width];
	}

private:

	int	total_bytes() { return m_height * m_width * sizeof(data_type); }

	void*	m_data;
	int	m_width;
	int	m_height;
	bool	m_writeable;
};


#endif // MMAP_ARRAY_H
