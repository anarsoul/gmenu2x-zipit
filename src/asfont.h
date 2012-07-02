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

class ASFont {
public:
	enum HAlign { HAlignLeft, HAlignRight,  HAlignCenter };
	enum VAlign { VAlignTop,  VAlignBottom, VAlignMiddle };

	ASFont(const std::string &font);
	~ASFont();

	bool utf8Code(unsigned char c);

	int getTextWidth(const std::string& text);

	int getHeight() {
		return surface->h - 1;
	}
	int getLineHeight() {
		return lineHeight;
	}

	void write(Surface* surface, const std::string& text, int x, int y, HAlign halign = HAlignLeft, VAlign valign = VAlignTop);

private:
	void writeLine(Surface *surface, const std::string &text, int x, int y);
	void writeLine(Surface *surface, const std::string &text, int x, int y, HAlign halign);
	void writeLine(Surface *surface, const std::string &text, int x, int y, HAlign halign, VAlign valign);
	void writeLine(Surface *surface, const std::vector<std::string> &text, int x, int y, HAlign halign, VAlign valign);

	SDL_Surface *surface;
	std::vector<unsigned> charpos;
	std::string characters;
	int lineHeight;
};

#endif /* ASFONT_H */
