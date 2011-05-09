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

#define SFONTPLUS_CHARSET "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~¡¿ÀÁÈÉÌÍÒÓÙÚÝÄËÏÖÜŸÂÊÎÔÛÅÃÕÑÆÇČĎĚĽĹŇÔŘŔŠŤŮŽàáèéìíòóùúýäëïöüÿâêîôûåãõñæçčďěľĺňôřŕšťžůðßÐÞþАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюяØøąćęłńśżźĄĆĘŁŃŚŻŹ"

class SFontPlus {
private:
	Uint32 getPixel(Sint32 x, Sint32 y);

	SDL_Surface *surface;
	std::vector<unsigned> charpos;
	std::string characters;
	unsigned height, lineHeight;

public:
	SFontPlus();
	SFontPlus(SDL_Surface *font);
	SFontPlus(const std::string &font);
	~SFontPlus();

	bool utf8Code(unsigned char c);

	void initFont(SDL_Surface *font, const std::string &characters = SFONTPLUS_CHARSET);
	void initFont(const std::string &font, const std::string &characters = SFONTPLUS_CHARSET);
	void freeFont();

	void write(SDL_Surface *s, const std::string &text, int x, int y);

	unsigned getTextWidth(const std::string &text);
	unsigned getHeight();
	unsigned getLineHeight();
};

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
