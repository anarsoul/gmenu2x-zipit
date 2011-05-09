#ifndef SFONTPLUS_H
#define SFONTPLUS_H

#include <SDL.h>
#include <string>
#include <vector>

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

#endif /* SFONTPLUS_H */
