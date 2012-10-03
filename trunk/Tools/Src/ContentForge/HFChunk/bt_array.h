// bt_array.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Wrapper for accessing a .bt format disk file.  Uses memory-mapped
// file access to a (potentially giant) data file directly without
// loading it all into RAM.

// Modified to support BT 1.3 by Vladimir "Niello" Orlov
// http://vterrain.org/Implementation/Formats/BT.html


#ifndef BT_ARRAY_H
#define BT_ARRAY_H


#include <engine/container.h>


class bt_array
// Class for wrapping access to a .bt terrain file.
{
	bt_array();
public:
	static bt_array*	create(const char* filename);

	~bt_array();

	int	get_width() const { return m_width; }
	int	get_height() const { return m_height; }
	bool	get_utm_flag() const { return m_utm_flag; }
	int	get_utm_zone() const { return m_utm_zone; }
	int	get_datum() const { return m_datum; }

	double	get_left() const { return m_left; }
	double	get_right() const { return m_right; }
	double	get_bottom() const { return m_bottom; }
	double	get_top() const { return m_top; }
	
	float	get_elev_diff() const { return m_max_elev - m_min_elev; }

	// out-of-bounds access is clamped.
	float	get_sample(int x, int z) const;

private:
	// We're going to cache short vertical strips of data, because
	// our processing tools often want to scan horizontally, but
	// the data is stored vertically.  For giant .bt files,
	// scanning horizontally is brutal on the mmap subsystem.
	struct cache_line {
		void*	m_data;
		int	m_v0;

		cache_line() : m_data(NULL), m_v0(0) {}
		~cache_line() {
			if (m_data) delete [] (Uint8*) m_data;
			m_data = 0;
		}
	};
	mutable array<cache_line>	m_cache;
	int	m_cache_height;
	int	m_sizeof_element;

	enum
	{
		BT11 = 1, // BT 1.2 is processed as BT 1.1 too
		BT13 = 2
	};

	int m_file_ver;

	bool	m_float_data;	// true if the array data is in floating-point format.
	int		m_width;
	int		m_height;
	union
	{
		bool	m_utm_flag;		// BT 1.1
		short	m_horz_units;	// BT 1.3: 0: Degrees 1: Meters 2: Feet (international .3048 m) 3: Feet (U.S. survey = 1200/3937 meters)
	};
	int		m_utm_zone;
	int		m_datum;
	double	m_left, m_right, m_bottom, m_top;
	float	m_vertical_scale;

	float	m_max_elev, m_min_elev;

	void*	m_data;
	int	m_data_size;

};


#endif // BT_ARRAY_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
