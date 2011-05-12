#include "asfont.h"
#include "imageio.h"
#include "surface.h"
#include "utilities.h"

#include <cassert>
#include <cstring>

#define SFONTPLUS_CHARSET "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~¡¿ÀÁÈÉÌÍÒÓÙÚÝÄËÏÖÜŸÂÊÎÔÛÅÃÕÑÆÇČĎĚĽĹŇÔŘŔŠŤŮŽàáèéìíòóùúýäëïöüÿâêîôûåãõñæçčďěľĺňôřŕšťžůðßÐÞþАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюяØøąćęłńśżźĄĆĘŁŃŚŻŹ"

ASFont::ASFont(const std::string &fontImagePath)
	: characters(SFONTPLUS_CHARSET)
{
	// Load PNG file into an SDL surface.
	SDL_Surface *buf = loadPNG(fontImagePath);
	if (!buf) {
		surface = NULL;
		return;
	}

	// Make sure we have a surface that can be blitted using alpha blending.
	if (buf->format->BytesPerPixel == 4) {
		surface = buf;
		SDL_SetAlpha(surface, SDL_SRCALPHA, 255);
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
		surface = SDL_ConvertSurface(buf, &format32, SDL_SRCALPHA);
		SDL_FreeSurface(buf);
	}
	assert(surface->format->BytesPerPixel == 4);

	SDL_LockSurface(surface);

	// Determine character widths.
	Uint32 pink = SDL_MapRGB(surface->format, 255, 0, 255);
	Uint32 *topLine = static_cast<Uint32 *>(surface->pixels);
	const unsigned width = surface->w;
	unsigned x = 0;
	unsigned c = 0;
	while (c < characters.length()) {
		while (x < width && topLine[x] != pink) x++;
		unsigned startx = x;
		x++;
		while (x < width && topLine[x] == pink) x++;

		charpos.push_back(startx);
		charpos.push_back(x);
		if (c > 0 && utf8Code(characters[c - 1])) {
			// UTF8 character
			charpos.push_back(startx);
			charpos.push_back(x);
			c++;
		}
		c++;
	}

	// Scan height of "0" glyph.
	std::string::size_type pos = characters.find("0") * 2;
	SDL_Rect srcrect = {
		charpos[pos], 1, charpos[pos + 2] - charpos[pos], surface->h - 1
	};
	const unsigned alphaMask = surface->format->Amask;
	unsigned y = srcrect.h;
	bool nonTransparentFound = false;
	while (!nonTransparentFound && y-- > 0) {
		Uint32 *line = reinterpret_cast<Uint32 *>(
			reinterpret_cast<Uint8 *>(surface->pixels)
				+ (srcrect.y + y) * surface->pitch
			);
		for (unsigned x = 0; !nonTransparentFound && x < srcrect.w; x++) {
			nonTransparentFound = (line[srcrect.x + x] & alphaMask) != 0;
		}
	}
	lineHeight = y + 1;

	SDL_UnlockSurface(surface);
}

ASFont::~ASFont() {
	if (surface) {
		SDL_FreeSurface(surface);
	}
}

bool ASFont::utf8Code(unsigned char c) {
	return (c>=194 && c<=198) || c==208 || c==209;
	//return c>=194;
}

void ASFont::write(SDL_Surface *s, const std::string &text, int x, int y) {
	if (text.empty()) return;

	std::string::size_type pos;
	SDL_Rect srcrect, dstrect;

	// these values won't change in the loop
	srcrect.y = 1;
	dstrect.y = y;
	srcrect.h = dstrect.h = surface->h-1;

	for(unsigned i=0; i<text.length() && x<surface->w; i++) {
		//Utf8 characters
		if (utf8Code(text[i]) && i+1<text.length()) {
			pos = characters.find(text.substr(i,2));
			i++;
		} else
			pos = characters.find(text[i]);
		if (pos == std::string::npos) {
			x += charpos[2]-charpos[1];
			continue;
		}

		pos *= 2;

		srcrect.x = charpos[pos];
		srcrect.w = charpos[pos+2] - charpos[pos];
		dstrect.x = x - charpos[pos+1] + charpos[pos];

		SDL_BlitSurface(surface, &srcrect, s, &dstrect);

		x += charpos[pos+2] - charpos[pos+1];
	}
}

int ASFont::getTextWidth(const char *text) {
	int maxWidth = 0, width = 0;
	while (char ch = *text++) {
		if (ch == '\n') {
			// New line.
			maxWidth = max(width, maxWidth);
			width = 0;
		} else {
			std::string::size_type pos;
			if (utf8Code(ch) && *text) {
				// 2-byte character.
				pos = characters.find(std::string(&text[-1], 2));
				text++;
			} else {
				// 1-byte character.
				pos = characters.find(ch);
			}
			if (pos == std::string::npos) {
				pos = 0;
			}
			width += charpos[pos * 2 + 2] - charpos[pos * 2 + 1];
		}
	}
	return max(width, maxWidth);
}

int ASFont::getTextWidth(const std::string& text) {
	return getTextWidth(text.c_str());
}

void ASFont::write(SDL_Surface* surface, const std::string& text, int x, int y, HAlign halign) {
	switch (halign) {
	case HAlignLeft:
		break;
	case HAlignCenter:
		x -= getTextWidth(text) / 2;
		break;
	case HAlignRight:
		x -= getTextWidth(text);
		break;
	}
	write(surface, text, x, y);
}

void ASFont::write(SDL_Surface* surface, const std::string& text, int x, int y, HAlign halign, VAlign valign) {
	switch (valign) {
	case VAlignTop:
		break;
	case VAlignMiddle:
		y -= getHeight() / 2;
		break;
	case VAlignBottom:
		y -= getHeight();
		break;
	}
	write(surface, text, x, y, halign);
}

void ASFont::write(SDL_Surface* surface, const std::vector<std::string> &text, int x, int y, HAlign halign, VAlign valign) {
	switch (valign) {
	case VAlignTop:
		break;
	case VAlignMiddle:
		y -= (getHeight() / 2) * text.size();
		break;
	case VAlignBottom:
		y -= getHeight() * text.size();
		break;
	}

	for (std::vector<std::string>::const_iterator it = text.begin(); it != text.end(); ++it) {
		write(surface, *it, x, y, halign);
		y += getHeight();
	}
}

void ASFont::write(Surface* surface, const std::string& text, int x, int y, HAlign halign, VAlign valign) {
	if (text.find("\n", 0) != std::string::npos) {
		std::vector<std::string> textArr;
		split(textArr, text, "\n");
		write(surface->raw, textArr, x, y, halign, valign);
	} else
		write(surface->raw, text, x, y, halign, valign);
}
