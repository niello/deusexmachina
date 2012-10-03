// tqt.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Texture quadtree utility class.


//#include "engine/ogl.h"
#include "engine/tqt.h"
#include "engine/image.h"


static const int	TQT_VERSION = 1;


struct tqt_header_info {
	int	m_version;
	int	m_tree_depth;
	int	m_tile_size;

	tqt_header_info()
		: m_version(0),
		  m_tree_depth(0),
		  m_tile_size(0)
	{
	}
};


static tqt_header_info	read_tqt_header_info(SDL_RWops* in)
// Read .tqt header file info from the given stream, and return a
// struct containing the info.  If the file doesn't look like .tqt,
// then set m_version in the return value to 0.
{
	tqt_header_info	info;

	int	tag = SDL_ReadLE32(in);
	if (tag != 0x00747174 /* "tqt\0" */) {
		// Wrong header tag.
		info.m_version = 0;	// signal invalid header.
		return info;
	}

	info.m_version = SDL_ReadLE32(in);
	info.m_tree_depth = SDL_ReadLE32(in);
	info.m_tile_size = SDL_ReadLE32(in);

	return info;
}


tqt::tqt(const char* filename)
// Constructor.  Open the file and read the table of contents.  Keep
// the stream open in order to load textures on demand.
{
	m_source = SDL_RWFromFile(filename, "rb");
	if (m_source == NULL) {
		throw "tqt::tqt() can't open file.";
	}

	// Read header.
	tqt_header_info	info = read_tqt_header_info(m_source);
	if (info.m_version != TQT_VERSION) {
		m_source = NULL;
		throw "tqt::tqt() incorrect file version.";
		return; // Hm.
	}

	m_depth = info.m_tree_depth;
	m_tile_size = info.m_tile_size;

	// Read table of contents.  Each entry is the offset of the
	// index'ed tile's JPEG data, relative to the start of the file.
	m_toc.resize(node_count(m_depth));
	for (int i = 0; i < node_count(m_depth); i++) {
		m_toc[i] = SDL_ReadLE32(m_source);
	}
}


tqt::~tqt()
// Destructor.  Close input file and release resources.
{
	SDL_RWclose(m_source);
}


//unsigned int	tqt::get_texture_id(int level, int col, int row) const
//// Returns the OpenGL texture id for a texture tile corresponding to
//// the specified node in the quadtree.
//{
//	return make_texture_id(load_image(level, col, row));
//}


/*
image::rgb*	tqt::load_image(int level, int col, int row) const
// Create a new image with data from the specified texture tile.
// Caller becomes the owner of the returned object.
{
	if (is_valid() == false) {
		return NULL;
	}
	assert(level < m_depth);

	int	index = node_index(level, col, row);
	assert(index < m_toc.size());

	// Load the .jpg and make a texture from it.
	SDL_RWseek(m_source, m_toc[index], SEEK_SET);
	image::rgb*	im = image::read_jpeg(m_source);

	return im;
}
*/

/*static*/ int tqt::node_count(int depth)
// Return the number of nodes in a fully populated quadtree of the specified depth.
{
	return 0x55555555 & ((1 << depth*2) - 1);
}


/*static*/ int tqt::node_index(int level, int col, int row)
// Given a tree level and the indices of a node within that tree
// level, this function returns a node index.
{
	assert(col >= 0 && col < (1 << level));
	assert(row >= 0 && row < (1 << level));

	return tqt::node_count(level) + (row << level) + col;
}


/*static*/ bool	tqt::is_tqt_file(const char* filename)
// Return true if the given file looks like a .tqt file of our
// appropriate version.  Do this by attempting to read the header.
{
	SDL_RWops*	in = SDL_RWFromFile(filename, "rb");
	if (in == NULL) {
		return false;
	}

	// Read header.
	tqt_header_info	info = read_tqt_header_info(in);
	SDL_RWclose(in);

	if (info.m_version != TQT_VERSION) {
		return false;
	}
	
	return true;
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
