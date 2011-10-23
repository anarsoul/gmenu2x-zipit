#include "iconbutton.h"
#include "gmenu2x.h"
#include "surface.h"

using namespace std;
using namespace fastdelegate;

IconButton::IconButton(GMenu2X *gmenu2x_, const string &icon,
					   const string &label)
	: Button(gmenu2x_->ts)
	, gmenu2x(gmenu2x_)
{
	this->icon = icon;
	this->label = label;
	updateSurfaces();
}

void IconButton::updateSurfaces() {
	iconSurface = gmenu2x->sc[icon];
	recalcSize();
}

void IconButton::setPosition(int x, int y) {
	if (rect.x != x || rect.y != y) {
		Button::setPosition(x,y);
		recalcSize();
	}
}

void IconButton::paint() {
	if (iconSurface) {
		iconSurface->blit(gmenu2x->s, iconRect);
	}
	if (label != "") {
		gmenu2x->s->write(gmenu2x->font, label, labelRect.x, labelRect.y,
				ASFont::HAlignLeft, ASFont::VAlignMiddle);
	}
}

bool IconButton::paintHover() {
	return true;
}

void IconButton::recalcSize() {
	uint h = 0, w = 0;
	if (iconSurface) {
		w += iconSurface->width();
		h += iconSurface->height();
		iconRect = (SDL_Rect) { rect.x, rect.y, w, h };
	} else {
		iconRect = (SDL_Rect) { 0, 0, 0, 0 };
	}

	if (label != "") {
		uint margin = iconSurface ? 2 : 0;
		labelRect = (SDL_Rect) {
			iconRect.x + iconRect.w + margin,
			rect.y + h / 2,
			gmenu2x->font->getTextWidth(label),
			gmenu2x->font->getHeight()
		};
		w += margin + labelRect.w;
	}

	setSize(w, h);
}

void IconButton::setAction(ButtonAction action) {
	this->action = action;
}
