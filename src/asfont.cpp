#include "asfont.h"
#include "imageio.h"
#include "surface.h"
#include "utilities.h"

#include <cassert>

#define SFONTPLUS_CHARSET "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~¡¿ÀÁÈÉÌÍÒÓÙÚÝÄËÏÖÜŸÂÊÎÔÛÅÃÕÑÆÇČĎĚĽĹŇÔŘŔŠŤŮŽàáèéìíòóùúýäëïöüÿâêîôûåãõñæçčďěľĺňôřŕšťžůðßÐÞþАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюяØøąćęłńśżźĄĆĘŁŃŚŻŹ"

Uint32 SFontPlus::getPixel(Sint32 x, Sint32 y) {
	assert(x>=0);
	assert(x<surface->w);
	assert(y>=0);
	assert(y<surface->h);

	Uint32 Bpp = surface->format->BytesPerPixel;

	// Get the pixel
	switch(Bpp) {
		case 1:
			return *((Uint8 *)surface->pixels + y * surface->pitch + x);
		break;
		case 2:
			return *((Uint16 *)surface->pixels + y * surface->pitch/2 + x);
		break;
		case 3: { // Format/endian independent
			Uint8 *bits = ((Uint8 *)surface->pixels)+y*surface->pitch+x*Bpp;
			Uint8 r, g, b;
			r = *((bits)+surface->format->Rshift/8);
			g = *((bits)+surface->format->Gshift/8);
			b = *((bits)+surface->format->Bshift/8);
			return SDL_MapRGB(surface->format, r, g, b);
		}
		break;
		case 4:
			return *((Uint32 *)surface->pixels + y * surface->pitch/4 + x);
		break;
	}

	return 0;
}

SFontPlus::SFontPlus(const std::string &fontImagePath, const std::string &characters) {
	this->characters = characters;

	SDL_Surface *buf = loadPNG(fontImagePath);
	if (!buf) {
		surface = NULL;
		return;
	}
	surface = SDL_DisplayFormatAlpha(buf);
	SDL_FreeSurface(buf);

	Uint32 pink = SDL_MapRGB(surface->format, 255,0,255);
#ifdef DEBUG
	bool utf8 = false;
	for (unsigned x=0; x<characters.length(); x++) {
		if (!utf8) utf8 = (unsigned char)characters[x]>128;
		if (utf8) DEBUG("%d\n", (unsigned char)characters[x]);
	}
#endif

	unsigned c = 0;

	SDL_LockSurface(surface);
	for (unsigned x=0; x<(unsigned)surface->w && c<characters.length(); x++) {
		if (getPixel(x,0) == pink) {
			unsigned startx = x;
			charpos.push_back(x);

			x++;
			while (x<(unsigned)surface->w && getPixel(x,0) == pink) x++;
			charpos.push_back(x);

			//utf8 characters
			if (c>0 && utf8Code(characters[c-1])) {
				charpos.push_back(startx);
				charpos.push_back(x);
				c++;
			}

			c++;
		}
	}
	SDL_UnlockSurface(surface);
	Uint32 colKey = getPixel(0,surface->h-1);
	SDL_SetColorKey(surface, SDL_SRCCOLORKEY, colKey);
	std::string::size_type pos = characters.find("0")*2;
	SDL_Rect srcrect = {charpos[pos], 1, charpos[pos+2] - charpos[pos], surface->h - 1};
	unsigned y = srcrect.h;
	bool nonKeyFound = false;
	while (y-- > 0 && !nonKeyFound) {
		unsigned x = srcrect.w;
		while (x-- > 0 && !nonKeyFound)
			nonKeyFound = getPixel(x+srcrect.x,y+srcrect.y) != colKey;
	}
	lineHeight = y+1;
}

SFontPlus::~SFontPlus() {
	if (surface) {
		SDL_FreeSurface(surface);
	}
}

bool SFontPlus::utf8Code(unsigned char c) {
	return (c>=194 && c<=198) || c==208 || c==209;
	//return c>=194;
}

void SFontPlus::write(SDL_Surface *s, const std::string &text, int x, int y) {
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

unsigned SFontPlus::getTextWidth(const std::string &text) {
	std::string::size_type pos;
	int width = 0;

	for(unsigned x=0; x<text.length(); x++) {
		//Utf8 characters
		if (utf8Code(text[x]) && x+1<text.length()) {
			pos = characters.find(text.substr(x,2));
			x++;
		} else
			pos = characters.find(text[x]);
		if (pos == std::string::npos) {
			width += charpos[2]-charpos[1];
			continue;
		}

		pos *= 2;
		width += charpos[pos+2] - charpos[pos+1];
	}

	return width;
}

unsigned SFontPlus::getHeight() {
	return surface->h - 1;
}

unsigned SFontPlus::getLineHeight() {
	return lineHeight;
}

ASFont::ASFont(const std::string &fontImagePath)
	: font(fontImagePath, SFONTPLUS_CHARSET)
{
	halfHeight = getHeight()/2;
	halfLineHeight = getLineHeight()/2;
}

ASFont::~ASFont() {
}

bool ASFont::utf8Code(unsigned char c) {
	return font.utf8Code(c);
}

void ASFont::write(SDL_Surface* surface, const char* text, int x, int y) {
	font.write(surface, text, x, y);
}

void ASFont::write(SDL_Surface* surface, const std::string& text, int x, int y, HAlign halign, VAlign valign) {
	switch (halign) {
	case HAlignLeft:
		break;
	case HAlignCenter:
		x -= getTextWidth(text)/2;
		break;
	case HAlignRight:
		x -= getTextWidth(text);
		break;
	}

	switch (valign) {
	case VAlignTop:
		break;
	case VAlignMiddle:
		y -= getHalfHeight();
		break;
	case VAlignBottom:
		y -= getHeight();
		break;
	}

	font.write(surface, text, x, y);
}
void ASFont::write(SDL_Surface* surface, std::vector<std::string> *text, int x, int y, HAlign halign, VAlign valign) {
	switch (valign) {
	case VAlignTop:
		break;
	case VAlignMiddle:
		y -= getHalfHeight()*text->size();
		break;
	case VAlignBottom:
		y -= getHeight()*text->size();
		break;
	}

	for (unsigned i=0; i<text->size(); i++) {
		int ix = x;
		switch (halign) {
		case HAlignLeft:
			break;
		case HAlignCenter:
			ix -= getTextWidth(text->at(i))/2;
			break;
		case HAlignRight:
			ix -= getTextWidth(text->at(i));
			break;
		}

		font.write(surface, text->at(i), x, y+getHeight()*i);
	}
}

void ASFont::write(Surface* surface, const std::string& text, int x, int y, HAlign halign, VAlign valign) {
	if (text.find("\n", 0) != std::string::npos) {
		std::vector<std::string> textArr;
		split(textArr,text,"\n");
		write(surface->raw, &textArr, x, y, halign, valign);
	} else
		write(surface->raw, text, x, y, halign, valign);
}

int ASFont::getHeight() {
	return font.getHeight();
}
int ASFont::getHalfHeight() {
	return halfHeight;
}

int ASFont::getLineHeight() {
	return font.getLineHeight();
}
int ASFont::getHalfLineHeight() {
	return halfLineHeight;
}

int ASFont::getTextWidth(const char* text) {
	return font.getTextWidth(text);
}
int ASFont::getTextWidth(const std::string& text) {
	if (text.find("\n", 0) != std::string::npos) {
		std::vector<std::string> textArr;
		split(textArr,text,"\n");
		return getTextWidth(&textArr);
	} else
		return getTextWidth(text.c_str());
}
int ASFont::getTextWidth(std::vector<std::string> *text) {
	int w = 0;
	for (unsigned i=0; i<text->size(); i++)
		w = max( getTextWidth(text->at(i).c_str()), w );
	return w;
}
