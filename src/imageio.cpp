#include "imageio.h"

#include <SDL_image.h>

SDL_Surface *loadPNG(const std::string &path) {
	// Load PNG file into an SDL surface.
	SDL_Surface *surface = IMG_Load(path.c_str());
	if (!surface) {
		return NULL;
	}

	// Make sure we have a surface that can be blitted using alpha blending.
	if (surface->format->BytesPerPixel == 4) {
		SDL_SetAlpha(surface, SDL_SRCALPHA, 255);
		return surface;
	} else {
		SDL_PixelFormat format32;
		memset(&format32, 0, sizeof(format32));
		format32.BitsPerPixel = 32;
		format32.BytesPerPixel = 4;
		format32.Rmask =
			SDL_BYTEORDER == SDL_BIG_ENDIAN ? 0x00FF0000 : 0x000000FF;
		format32.Gmask = 0x0000FF00;
		format32.Bmask =
			SDL_BYTEORDER == SDL_BIG_ENDIAN ? 0x000000FF : 0x00FF0000;
		format32.Amask = 0xFF000000;
		format32.Rshift = SDL_BYTEORDER == SDL_BIG_ENDIAN ? 16 : 0;
		format32.Gshift = 8;
		format32.Bshift = SDL_BYTEORDER == SDL_BIG_ENDIAN ? 0 : 16;
		format32.Ashift = 24;
		SDL_Surface *surface32 =
			SDL_ConvertSurface(surface, &format32, SDL_SRCALPHA);
		SDL_FreeSurface(surface);
		return surface32;
	}
}
