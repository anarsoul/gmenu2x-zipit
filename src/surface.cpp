/***************************************************************************
 *   Copyright (C) 2006 by Massimiliano Torromeo   *
 *   massimiliano.torromeo@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "surface.h"
#include "imageio.h"
#include "utilities.h"
#include "debug.h"

#include <SDL_gfxPrimitives.h>

#include <iostream>
using namespace std;

RGBAColor strtorgba(const string &strColor) {
	RGBAColor c = {0,0,0,255};
	c.r = constrain( strtol( strColor.substr(0,2).c_str(), NULL, 16 ), 0, 255 );
	c.g = constrain( strtol( strColor.substr(2,2).c_str(), NULL, 16 ), 0, 255 );
	c.b = constrain( strtol( strColor.substr(4,2).c_str(), NULL, 16 ), 0, 255 );
	c.a = constrain( strtol( strColor.substr(6,2).c_str(), NULL, 16 ), 0, 255 );
	return c;
}

Surface::Surface() {
	raw = NULL;
	dblbuffer = NULL;
}

Surface::Surface(Surface *s) {
	dblbuffer = NULL;
	raw = SDL_DisplayFormat(s->raw);
	halfW = raw->w/2;
	halfH = raw->h/2;
}

Surface::Surface(const string &img, const string &skin) {
	raw = NULL;
	dblbuffer = NULL;
	load(img, skin);
	halfW = raw->w/2;
	halfH = raw->h/2;
}

Surface::~Surface() {
	free();
}

void Surface::enableVirtualDoubleBuffer(SDL_Surface *surface) {
	dblbuffer = surface;
	raw = SDL_DisplayFormat(dblbuffer);
}

void Surface::free() {
	if (raw!=NULL) SDL_FreeSurface( raw );
	if (dblbuffer!=NULL) SDL_FreeSurface( dblbuffer );
}

SDL_PixelFormat *Surface::format() {
	if (raw==NULL)
		return NULL;
	else
		return raw->format;
}

void Surface::load(const string &img, const string &skin) {
	free();

	string skinpath;
	if (!skin.empty() && !img.empty() && img[0]!='/') {
		skinpath = "skins/"+skin+"/"+img;
		if (!fileExists(skinpath))
			skinpath = "skins/Default/"+img;
	} else {
		skinpath = img;
	}

	SDL_Surface *buf = loadPNG(skinpath);
	if (buf!=NULL) {
		raw = SDL_DisplayFormatAlpha(buf);
		SDL_FreeSurface(buf);
	} else {
		ERROR("Couldn't load surface '%s'\n", img.c_str());
	}
}

void Surface::flip() {
	if (dblbuffer!=NULL) {
		this->blit(dblbuffer,0,0);
		SDL_Flip(dblbuffer);
	} else {
		SDL_Flip(raw);
	}
}

bool Surface::blit(SDL_Surface *destination, int x, int y, int w, int h, int a) {
	if (destination == NULL || a==0) return false;

	SDL_Rect src = {0,0,w,h};
	SDL_Rect dest;
	dest.x = x;
	dest.y = y;
	if (a>0 && a!=raw->format->alpha)
		SDL_SetAlpha(raw, SDL_SRCALPHA|SDL_RLEACCEL, a);
	return SDL_BlitSurface(raw, (w==0 || h==0) ? NULL : &src, destination, &dest);
}
bool Surface::blit(Surface *destination, int x, int y, int w, int h, int a) {
	return blit(destination->raw,x,y,w,h,a);
}

bool Surface::blitCenter(SDL_Surface *destination, int x, int y, int w, int h, int a) {
	int oh, ow;
	if (w==0) ow = halfW; else ow = min(halfW,w/2);
	if (h==0) oh = halfH; else oh = min(halfH,h/2);
	return blit(destination,x-ow,y-oh,w,h,a);
}
bool Surface::blitCenter(Surface *destination, int x, int y, int w, int h, int a) {
	return blitCenter(destination->raw,x,y,w,h,a);
}

bool Surface::blitRight(SDL_Surface *destination, int x, int y, int w, int h, int a) {
	if (!w) w = raw->w;
	return blit(destination,x-min(raw->w,w),y,w,h,a);
}
bool Surface::blitRight(Surface *destination, int x, int y, int w, int h, int a) {
	if (!w) w = raw->w;
	return blitRight(destination->raw,x,y,w,h,a);
}

int Surface::box(Sint16 x, Sint16 y, Sint16 w, Sint16 h, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	return boxRGBA(raw,x,y,x+w-1,y+h-1,r,g,b,a);
}
int Surface::box(Sint16 x, Sint16 y, Sint16 w, Sint16 h, Uint8 r, Uint8 g, Uint8 b) {
	SDL_Rect re = {x,y,w,h};
	return SDL_FillRect(raw, &re, SDL_MapRGBA(format(),r,g,b,255));
}
int Surface::box(Sint16 x, Sint16 y, Sint16 w, Sint16 h, RGBAColor c) {
	return box(x,y,w,h,c.r,c.g,c.b,c.a);
}
int Surface::box(SDL_Rect re, RGBAColor c) {
	return boxRGBA(
		raw, re.x, re.y, re.x + re.w - 1, re.y + re.h - 1, c.r, c.g, c.b, c.a
		);
}

int Surface::rectangle(Sint16 x, Sint16 y, Sint16 w, Sint16 h, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	return rectangleRGBA(raw,x,y,x+w-1,y+h-1,r,g,b,a);
}
int Surface::rectangle(Sint16 x, Sint16 y, Sint16 w, Sint16 h, RGBAColor c) {
	return rectangle(x,y,w,h,c.r,c.g,c.b,c.a);
}
int Surface::rectangle(SDL_Rect re, RGBAColor c) {
	return rectangle(re.x,re.y,re.w,re.h,c.r,c.g,c.b,c.a);
}

int Surface::hline(Sint16 x, Sint16 y, Sint16 w, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	return hlineRGBA(raw,x,x+w-1,y,r,g,b,a);
}

void Surface::clearClipRect() {
	SDL_SetClipRect(raw,NULL);
}

void Surface::setClipRect(int x, int y, int w, int h) {
	SDL_Rect rect = {x,y,w,h};
	setClipRect(rect);
}

void Surface::setClipRect(SDL_Rect rect) {
	SDL_SetClipRect(raw,&rect);
}

bool Surface::blit(Surface *destination, SDL_Rect container, ASFont::HAlign halign, ASFont::VAlign valign) {
	switch (halign) {
	case ASFont::HAlignLeft:
		break;
	case ASFont::HAlignCenter:
		container.x += container.w/2-halfW;
		break;
	case ASFont::HAlignRight:
		container.x += container.w-raw->w;
		break;
	}

	switch (valign) {
	case ASFont::VAlignTop:
		break;
	case ASFont::VAlignMiddle:
		container.y += container.h/2-halfH;
		break;
	case ASFont::VAlignBottom:
		container.y += container.h-raw->h;
		break;
	}

	return blit(destination,container.x,container.y);
}
