#ifndef ICONBUTTON_H
#define ICONBUTTON_H

#include "button.h"
#include "asfont.h"

#include <string>

using std::string;

class GMenu2X;
class Surface;

class IconButton : public Button {
protected:
	GMenu2X *gmenu2x;
	string icon, label;
	void recalcSize();
	SDL_Rect iconRect, labelRect;

	Surface *iconSurface;

	void updateSurfaces();

public:
	IconButton(GMenu2X *gmenu2x, const string &icon, const string &label="");
	virtual ~IconButton() {};

	virtual void paint();
	virtual bool paintHover();

	virtual void setPosition(int x, int y);

	void setAction(ButtonAction action);
};

#endif
