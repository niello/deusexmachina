// image.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Handy image utilities for RGB surfaces.

#ifndef IMAGE_H
#define IMAGE_H


#include "engine/utility.h"
#include <SDL.h>
struct SDL_RWops;


namespace image
{
	// 24-bit RGB image.  Packed data, red byte first (RGBRGB...)
	//
	// We need this class because SDL_Surface chokes on any image
	// that has more than 64KB per row, due to a Uint16 pitch
	// member.
	struct rgb {
		rgb(int width, int height);
		~rgb();

		Uint8*	m_data;
		int	m_width;
		int	m_height;
		int	m_pitch;	// byte offset from one row to the next
	};


	// Make a system-memory 24-bit bitmap surface.  24-bit packed
	// data, red byte first.
	rgb*	create_rgb(int width, int height);
	
	inline Uint8*	scanline(rgb* surf, int y)
	{
		assert(surf);
		assert(y < surf->m_height);
		return ((Uint8*) surf->m_data) + surf->m_pitch * y;
	}

	void	resample(rgb* out, int out_x0, int out_y0, int out_x1, int out_y1,
			 rgb* in, float in_x0, float in_y0, float in_x1, float in_y1);

	//void	write_jpeg(SDL_RWops* out, rgb* image, int quality);

	//rgb*	read_jpeg(const char* filename);
	//rgb*	read_jpeg(SDL_RWops* in);

//	// Makes an SDL_Surface from the given image data.
//	//
//	// DELETES image!!!
//	SDL_Surface*	create_SDL_Surface(rgb* image);

	// Fast, in-place, DESTRUCTIVE resample.  For making mip-maps.
	// Munges the input image to produce the output image.
	void	make_next_miplevel(rgb* image);
};


#endif // IMAGE_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
