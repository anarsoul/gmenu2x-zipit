#include "asfont.h"
#include "surface.h"
#include "utilities.h"

ASFont::ASFont(SDL_Surface* font) {
	this->font.initFont(font);
	halfHeight = getHeight()/2;
	halfLineHeight = getLineHeight()/2;
}

ASFont::ASFont(Surface* font) {
	this->font.initFont(font->raw);
	halfHeight = getHeight()/2;
	halfLineHeight = getLineHeight()/2;
}

ASFont::ASFont(const std::string &font) {
	this->font.initFont(font);
	halfHeight = getHeight()/2;
	halfLineHeight = getLineHeight()/2;
}

ASFont::~ASFont() {
	font.freeFont();
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
