// Based on SFont by Karl Bartel.
// Adapted to C++ by Massimiliano Torromeo.
// Refactored by Maarten ter Huurne and several others (see git log).
// License: GPL version 2 or later.

#ifndef ASFONT_H
#define ASFONT_H

#include <SDL.h>
#include <string>
#include <vector>

class Surface;

class SFontPlus {
public:
	SFontPlus(const std::string &fontImagePath, const std::string &characters);
	~SFontPlus();

	bool utf8Code(unsigned char c);

	void write(SDL_Surface *s, const std::string &text, int x, int y);

	unsigned getTextWidth(const char *text);
	unsigned getHeight();
	unsigned getLineHeight();

private:
	Uint32 getPixel(Sint32 x, Sint32 y);

	SDL_Surface *surface;
	std::vector<unsigned> charpos;
	std::string characters;
	unsigned height, lineHeight;
};

class ASFont {
public:
	enum HAlign { HAlignLeft, HAlignRight,  HAlignCenter };
	enum VAlign { VAlignTop,  VAlignBottom, VAlignMiddle };

	ASFont(const std::string &font);
	~ASFont();

	bool utf8Code(unsigned char c);

	int getHeight();
	int getLineHeight();
	int getTextWidth(const char* text);
	int getTextWidth(const std::string& text);
	void write(Surface* surface, const std::string& text, int x, int y, HAlign halign = HAlignLeft, VAlign valign = VAlignTop);

private:
	void write(SDL_Surface* surface, const std::string& text, int x, int y, HAlign halign, VAlign valign);
	void write(SDL_Surface* surface, const std::vector<std::string> &text, int x, int y, HAlign halign, VAlign valign);

	SFontPlus font;
};

#endif /* ASFONT_H */
