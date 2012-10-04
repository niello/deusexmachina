// tqt.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Texture quadtree utility class.

#ifndef TQT_H
#define TQT_H


#include "engine/container.h"
#include "engine/image.h"
struct SDL_RWops;


class tqt
// Manages a disk-based texture quadtree.
{
public:
	tqt(const char* filename);
	~tqt();
	bool	is_valid() const { return m_source != NULL; }
	int	get_depth() const { return m_depth; }
	int	get_tile_size() const { return m_tile_size; }

	unsigned int	get_texture_id(int level, int col, int row) const;
	//image::rgb*	load_image(int level, int col, int row) const;

	// Static utility functions.
	static bool	tqt::is_tqt_file(const char* filename);
	static int	tqt::node_count(int depth);
	static int	tqt::node_index(int level, int col, int row);
	
private:
	array<unsigned int>	m_toc;
	int	m_depth;
	int	m_tile_size;
	SDL_RWops*	m_source;
};


#endif // TQT_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
