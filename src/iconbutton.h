#ifndef ICONBUTTON_H
#define ICONBUTTON_H

#include "button.h"

#include <string>

class GMenu2X;
class Surface;

class IconButton : public Button {
public:
	IconButton(GMenu2X *gmenu2x, const std::string &icon,
			const std::string &label="");
	virtual ~IconButton() {};

	virtual void paint();
	virtual bool paintHover();

	virtual void setPosition(int x, int y);

	void setAction(ButtonAction action);

private:
	void updateSurfaces();

	GMenu2X *gmenu2x;
	std::string icon, label;
	void recalcSize();
	SDL_Rect iconRect, labelRect;

	Surface *iconSurface;
};

#endif
