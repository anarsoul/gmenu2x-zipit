/***************************************************************************
 *   Copyright (C) 2006 by Massimiliano Torromeo                           *
 *   massimiliano.torromeo@gmail.com                                       *
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

#include "settingsdialog.h"

#include "gmenu2x.h"
#include "menusetting.h"

#include <SDL.h>
#include <SDL_gfxPrimitives.h>

using namespace std;

SettingsDialog::SettingsDialog(
		GMenu2X *gmenu2x_, InputManager &inputMgr_, Touchscreen &ts_,
		const string &text_, const string &icon)
	: Dialog(gmenu2x_)
	, inputMgr(inputMgr_)
	, ts(ts_)
	, text(text_)
{
	if (icon!="" && gmenu2x->sc[icon] != NULL)
		this->icon = icon;
	else
		this->icon = "icons/generic.png";
}

SettingsDialog::~SettingsDialog() {
	for (vector<MenuSetting *>::iterator it = voices.begin(); it != voices.end(); ++it)
		delete *it;
}

bool SettingsDialog::exec() {
	Surface bg(gmenu2x->bg);
	bg.convertToDisplayFormat();

	bool close = false, ts_pressed = false;
	uint i, sel = 0, iY, firstElement = 0;
	voices[sel]->adjustInput();

	SDL_Rect clipRect = {0, gmenu2x->skinConfInt["topBarHeight"]+1, gmenu2x->resX-9, gmenu2x->resY-gmenu2x->skinConfInt["topBarHeight"]-25};
	SDL_Rect touchRect = {2, gmenu2x->skinConfInt["topBarHeight"]+4, gmenu2x->resX-12, clipRect.h};
	uint rowHeight = gmenu2x->font->getHeight()+1; // gp2x=15+1 / pandora=19+1
	uint numRows = (gmenu2x->resY-gmenu2x->skinConfInt["topBarHeight"]-20)/rowHeight;

	while (!close) {
		if (ts.available()) ts.poll();

		bg.blit(gmenu2x->s,0,0);

		gmenu2x->drawTopBar(gmenu2x->s);
		//link icon
		drawTitleIcon(icon);
		writeTitle(text);

		gmenu2x->drawBottomBar(gmenu2x->s);

		if (sel>firstElement+numRows-1) firstElement=sel-numRows+1;
		if (sel<firstElement) firstElement=sel;

		//selection
		iY = sel-firstElement;
		iY = gmenu2x->skinConfInt["topBarHeight"]+2+(iY*rowHeight);
		gmenu2x->s->setClipRect(clipRect);
		if (sel<voices.size())
			gmenu2x->s->box(1, iY, 148, rowHeight-2, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
		gmenu2x->s->clearClipRect();

		//selected option
		voices[sel]->drawSelected(iY);

		gmenu2x->s->setClipRect(clipRect);
		if (ts_pressed && !ts.pressed()) ts_pressed = false;
		if (ts.available() && ts.pressed() && !ts.inRect(touchRect)) ts_pressed = false;
		for (i=firstElement; i<voices.size() && i<firstElement+numRows; i++) {
			iY = i-firstElement;
			voices[i]->draw(iY*rowHeight+gmenu2x->skinConfInt["topBarHeight"]+2);
			if (ts.available() && ts.pressed() && ts.inRect(touchRect.x, touchRect.y+(iY*rowHeight), touchRect.w, rowHeight)) {
				ts_pressed = true;
				sel = i;
			}
		}
		gmenu2x->s->clearClipRect();

		gmenu2x->drawScrollBar(numRows,voices.size(),firstElement,clipRect.y+1,clipRect.h);

		//description
		writeSubTitle(voices[sel]->getDescription());

		gmenu2x->s->flip();
		voices[sel]->handleTS();

		InputManager::Button button = inputMgr.waitForPressedButton();
		if (!voices[sel]->handleButtonPress(button)) {
			switch (button) {
				case InputManager::CANCEL:
				case InputManager::SETTINGS:
				case InputManager::MENU:
				
					close = true;
					break;
				case InputManager::UP:
					if (sel==0)
						sel = voices.size()-1;
					else
						sel -= 1;
					gmenu2x->setInputSpeed();
					voices[sel]->adjustInput();
					break;
				case InputManager::DOWN:
					sel += 1;
					if (sel>=voices.size()) sel = 0;
					gmenu2x->setInputSpeed();
					voices[sel]->adjustInput();
				default:
					break;
			}
		}
	}

	gmenu2x->setInputSpeed();
	return true;
}

void SettingsDialog::addSetting(MenuSetting* set) {
	voices.push_back(set);
}

bool SettingsDialog::edited() {
	for (uint i=0; i<voices.size(); i++)
		if (voices[i]->edited()) return true;
	return false;
}
