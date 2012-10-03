// image.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Handy image utilities for RGB surfaces.


#include "engine/image.h"
#include "engine/utility.h"
//#include "engine/jpeg.h"
#include "engine/dlmalloc.h"
#include <SDL.h>
#include <stdlib.h>


namespace image
{
	rgb::rgb(int width, int height)
		: m_data(0),
		  m_width(width),
		  m_height(height),
		  m_pitch((m_width * 3 + 3) & ~3)	// round pitch up to nearest 4-byte boundary
	{
		assert(width > 0);
		assert(height > 0);
		assert(m_pitch >= m_width * 3);
		assert((m_pitch & 3) == 0);

		m_data = (Uint8*) dlmalloc(m_pitch * m_height);
	}

	rgb::~rgb()
	{
		if (m_data) {
			dlfree(m_data);
			m_data = 0;
		}
	}


	rgb*	create_rgb(int width, int height)
	// Create an system-memory rgb surface.  The data order is
	// packed 24-bit, RGBRGB..., regardless of the endian-ness of
	// the CPU.
	{
		rgb*	s = new rgb(width, height);

		return s;
	}


#if 0
	void	resample(SDL_Surface* dest, int out_x0, int out_y0, int out_x1, int out_y1,
			 SDL_Surface* src, float in_x0, float in_y0, float in_x1, float in_y1)
	// Resample the specified rectangle of the src surface into the
	// specified rectangle of the destination surface.  Output coords
	// are inclusive.
	{
		// Make sure output is within bounds.
		assert(out_x0 >= 0 && out_x0 < dest->w);
		assert(out_x1 > out_x0 && out_x1 < dest->w);
		assert(out_y0 >= 0 && out_y0 < dest->h);
		assert(out_y1 > out_y0 && out_y1 < dest->h);

		int	dxo = (out_x1 - out_x0);
		int	dyo = (out_y1 - out_y0);

		// @@ check input...

		float	dxi = in_x1 - in_x0;
		float	dyi = in_y1 - in_y0;
		assert(dxi > 0.001f);
		assert(dyi > 0.001f);

		float	x_factor = dxi / dxo;
		float	y_factor = dyi / dyo;

		// @@ not optimized.

		for (int j = 0; j <= dyo; j++) {
			for (int i = 0; i <= dxo; i++) {
				// @@ simple nearest-neighbor point-sample.
				float	x = i * x_factor + in_x0;
				float	y = j * y_factor + in_y0;
				x = fclamp(x, 0.f, float(src->w - 1));
				y = fclamp(y, 0.f, float(src->h - 1));

				Uint8*	p = scanline(src, frnd(y)) + 3 * frnd(x);
				Uint8*	q = scanline(dest, out_y0 + j) + 3 * (out_x0 + i);

				*q++ = *p++;	// red
				*q++ = *p++;	// green
				*q++ = *p++;	// blue
			}
		}
	}
#endif // 0

	/*void	write_jpeg(SDL_RWops* out, rgb* image, int quality)
	// Write the given image to the given out stream, in jpeg format.
	{
		jpeg::output*	j_out = jpeg::output::create(out, image->m_width, image->m_height, quality);

		for (int y = 0; y < image->m_height; y++) {
			j_out->write_scanline(scanline(image, y));
		}

		delete j_out;
	}


	rgb*	read_jpeg(const char* filename)
	// Create and read a new image from the given filename, if possible.
	{
		SDL_RWops*	in = SDL_RWFromFile(filename, "rb");
		if (in)
		{
			rgb*	im = read_jpeg(in);
			SDL_RWclose(in);
			return im;
		}
		else {
			return NULL;
		}
	}


	rgb*	read_jpeg(SDL_RWops* in)
	// Create and read a new image from the stream.
	{
		jpeg::input*	j_in = jpeg::input::create(in);
		if (j_in == NULL) return NULL;
		
		rgb*	im = image::create_rgb(j_in->get_width(), j_in->get_height());

		for (int y = 0; y < j_in->get_height(); y++) {
			j_in->read_scanline(scanline(im, y));
		}

		delete j_in;

		return im;
	}*/


	SDL_Surface*	create_SDL_Surface(rgb* image)
	// Steal *image's data to create an SDL_Surface.
	//
	// DELETES image!!!
	{
		assert(image->m_pitch < 65536);	// SDL_Surface only uses Uint16 for pitch!!!

		SDL_Surface*	s = SDL_CreateRGBSurfaceFrom(image->m_data,
							     image->m_width, image->m_height, 24, image->m_pitch,
							     SDL_SwapLE32(0x0FF),
							     SDL_SwapLE32(0x0FF00),
							     SDL_SwapLE32(0x0FF0000),
							     0);

		// s owns *image's data now -- invalidate *image.
		image->m_data = 0;
		image->m_height = 0;
		image->m_width = 0;
		image->m_pitch = 0;
		delete image;

		assert(s->pixels);
		assert(s->format->BytesPerPixel == 3);
		assert(s->format->BitsPerPixel == 24);

		return s;
	}


	void	make_next_miplevel(rgb* image)
	// Fast, in-place resample.  For making mip-maps.  Munges the
	// input image to produce the output image.
	{
		assert(image->m_data);

		int	new_w = image->m_width >> 1;
		int	new_h = image->m_height >> 1;
		if (new_w < 1) new_w = 1;
		if (new_h < 1) new_h = 1;

		int	new_pitch = new_w * 3;
		// Round pitch up to the nearest 4-byte boundary.
		new_pitch = (new_pitch + 3) & ~3;

		if (new_w * 2 != image->m_width  || new_h * 2 != image->m_height)
		{
			// Image can't be shrunk along (at least) one
			// of its dimensions, so don't bother
			// resampling.  Technically we should, but
			// it's pretty useless at this point.  Just
			// change the image dimensions and leave the
			// existing pixels.
		}
		else
		{
			// Resample.  Simple average 2x2 --> 1, in-place.
			int	pitch = image->m_pitch;
			for (int j = 0; j < new_h; j++) {
				Uint8*	out = ((Uint8*) image->m_data) + j * new_pitch;
				Uint8*	in = ((Uint8*) image->m_data) + (j << 1) * pitch;
				for (int i = 0; i < new_w; i++) {
					int	r, g, b;
					r = (*(in + 0) + *(in + 3) + *(in + 0 + pitch) + *(in + 3 + pitch));
					g = (*(in + 1) + *(in + 4) + *(in + 1 + pitch) + *(in + 4 + pitch));
					b = (*(in + 2) + *(in + 5) + *(in + 2 + pitch) + *(in + 5 + pitch));
					*(out + 0) = r >> 2;
					*(out + 1) = g >> 2;
					*(out + 2) = b >> 2;
					out += 3;
					in += 6;
				}
			}
		}

		// Munge image's members to reflect the shrunken image.
		image->m_width = new_w;
		image->m_height = new_h;
		image->m_pitch = new_pitch;
	}
};


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
