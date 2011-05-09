//Advanced SFont by Massimiliano Torromeo (cpp wrapper around SFont)

#ifndef ASFONT_H
#define ASFONT_H

#include "sfontplus.h"

#include <string>
#include <vector>

struct SDL_Surface;
class Surface;


class ASFont {
public:
	enum HAlign { HAlignLeft, HAlignRight,  HAlignCenter };
	enum VAlign { VAlignTop,  VAlignBottom, VAlignMiddle };

	ASFont(SDL_Surface* font);
	ASFont(Surface* font);
	ASFont(const std::string &font);
	~ASFont();

	bool utf8Code(unsigned char c);

	int getHeight();
	int getHalfHeight();
	int getLineHeight();
	int getHalfLineHeight();
	int getTextWidth(const char* text);
	int getTextWidth(const std::string& text);
	int getTextWidth(std::vector<std::string> *text);
	void write(SDL_Surface* surface, const char* text, int x, int y);
	void write(SDL_Surface* surface, const std::string& text, int x, int y, HAlign halign = HAlignLeft, VAlign valign = VAlignTop);
	void write(SDL_Surface* surface, std::vector<std::string> *text, int x, int y, HAlign halign = HAlignLeft, VAlign valign = VAlignTop);
	void write(Surface* surface, const std::string& text, int x, int y, HAlign halign = HAlignLeft, VAlign valign = VAlignTop);

private:
	SFontPlus font;
	int halfHeight, halfLineHeight;
};

#endif /* ASFONT_H */
