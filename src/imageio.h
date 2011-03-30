#ifndef IMAGEIO_H
#define IMAGEIO_H

#include <string>

struct SDL_Surface;

SDL_Surface *loadPNG(const std::string &path);

#endif
