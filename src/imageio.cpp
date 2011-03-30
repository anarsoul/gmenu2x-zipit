#include "imageio.h"

#include <SDL_image.h>

SDL_Surface *loadPNG(const std::string &path)
{
	return IMG_Load(path.c_str());
}
