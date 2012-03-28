/***************************************************************************
 *   Copyright (C) 2006 by Massimiliano Torromeo                           *
 *   massimiliano.torromeo@gmail.com                                       *
 *                                                                         *
	 Copyright 2012 Mark Majeres (slug_)  mark@engine12.com		 

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

#include "inputdialog.h"
#include <stdlib.h>

#include "gmenu2x.h"
#include "iconbutton.h"
#include "utilities.h"

#include <SDL_gfxPrimitives.h>

using namespace std;
using namespace fastdelegate;

InputDialog::InputDialog(GMenu2X *gmenu2x, InputManager &inputMgr_,
		Touchscreen &ts_, const string &text,
		const string &startvalue, const string &title, const string &icon)
	: Dialog(gmenu2x)
	, inputMgr(inputMgr_)
	, ts(ts_)
{
	if (title == "") {
		this->title = text;
		this->text = "";
	} else {
		this->title = title;
		this->text = text;
	}
	this->icon = "";
	if (icon != "" && gmenu2x->sc[icon] != NULL) {
		this->icon = icon;
	}

	Surface bg(gmenu2x->s);
	//Darken background
	bg.box(0, 0, gmenu2x->resX, gmenu2x->resY, 0,0,0,200);
	
	input = startvalue;


//	buttonbox = new ButtonBox(gmenu2x);
	IconButton *btnConfirm = new IconButton(gmenu2x, ts,
			"skin:imgs/buttons/accept.png", gmenu2x->tr["Confirm"]);
//	btnConfirm->setAction(MakeDelegate(this, &InputDialog::confirm));
//	buttonbox->add(btnConfirm);

}


bool InputDialog::exec() {
	SDL_Rect box = { 0, 60, 0, gmenu2x->font->getHeight() + 4 };

	Uint32 caretTick = 0, curTick;
	bool caretOn = true;

	SDL_EnableUNICODE(SDL_ENABLE);

	bool close = false;
	bool ok = true;
	while (!close) {
		gmenu2x->bg->blit(gmenu2x->s,0,0);
		writeTitle(title);
		writeSubTitle(text);
		drawTitleIcon(icon);

//		buttonbox->paint(5);

		box.w = gmenu2x->font->getTextWidth(input) + 18;
		box.x = 160 - box.w / 2;
		gmenu2x->s->box(box.x, box.y, box.w, box.h,
		gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
		gmenu2x->s->rectangle(box.x, box.y, box.w, box.h,
				gmenu2x->skinConfColors[COLOR_SELECTION_BG]);

		gmenu2x->s->write(gmenu2x->font, input, box.x + 5, box.y + box.h - 2,
				ASFont::HAlignLeft, ASFont::VAlignBottom);

		curTick = SDL_GetTicks();
		if (curTick - caretTick >= 600) {
			caretOn = !caretOn;
			caretTick = curTick;
		}

		if (caretOn) {
			gmenu2x->s->box(box.x + box.w - 12, box.y + 3, 8, box.h - 6,
					gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
		}

		if (ts.available()) ts.poll();
		
		gmenu2x->s->flip();


		SDL_Event event;
	
		if (SDL_PollEvent(&event))
		{
			
			if(event.type == SDL_KEYDOWN){
				
			//	SDLMod modifier = event.key.keysym.mod;
			//	if( modifier & (KMOD_LCTRL | KMOD_LSHIFT | KMOD_LALT) )
			//		continue;
					
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
							ok = false;
							close = true;
							break;
							
					case SDLK_RETURN:
							close = true;
							break;

					case SDLK_SPACE:
							input += ' '; 
							break;
				   
					case SDLK_BACKSPACE:
							backspace();  
							break;
				   
					case SDLK_LSHIFT:
					case SDLK_LCTRL:
					case SDLK_LALT:
					case SDLK_HOME:
					case SDLK_END:
							break;
							
					default:
							input += (wchar_t)event.key.keysym.unicode;
						//	input += SDL_GetKeyName( event.key.keysym.sym );
							break;

				}
			}
		}
		
		usleep(LOOP_DELAY);
		
	}
				
	SDL_EnableUNICODE(SDL_DISABLE);

	return ok;
}

void InputDialog::backspace() {
	// Check for UTF8 characters.
	input = input.substr(0, input.length()
		- (gmenu2x->font->utf8Code(input[input.length() - 2]) ? 2 : 1));
}

/*
void InputDialog::confirm() {
	if (selRow == (int)kb->size()) {
		if (selCol == 0) {
			ok = false;
		}
		close = true;
	} else {
		int xc = 0;
		for (uint x = 0; x < kb->at(selRow).length(); x++) {
			bool utf8 = gmenu2x->font->utf8Code(kb->at(selRow)[x]);
			if (xc == selCol) input += kb->at(selRow).substr(x, utf8 ? 2 : 1);
			if (utf8) x++;
			xc++;
		}
	}
}

void InputDialog::drawVirtualKeyboard() {
	//keyboard border
	gmenu2x->s->rectangle(kbRect, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);

	if (selCol<0) selCol = selRow==(int)kb->size() ? 1 : kbLength-1;
	if (selCol>=(int)kbLength) selCol = 0;
	if (selRow<0) selRow = kb->size()-1;
	if (selRow>(int)kb->size()) selRow = 0;

	//selection
	if (selRow<(int)kb->size())
		gmenu2x->s->box(kbLeft + selCol * KEY_WIDTH - 1,
				KB_TOP + selRow * KEY_HEIGHT, KEY_WIDTH - 1, KEY_HEIGHT - 2,
				gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
	else {
		if (selCol > 1) selCol = 0;
		if (selCol < 0) selCol = 1;
		gmenu2x->s->box(kbLeft + selCol * kbLength * KEY_WIDTH / 2 - 1,
				KB_TOP + kb->size() * KEY_HEIGHT, kbLength * KEY_WIDTH / 2 - 1,
				KEY_HEIGHT - 1, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
	}

	//keys
	for (uint l=0; l<kb->size(); l++) {
		string line = kb->at(l);
		for (uint x=0, xc=0; x<line.length(); x++) {
			string charX;
			//utf8 characters
			if (gmenu2x->font->utf8Code(line[x])) {
				charX = line.substr(x,2);
				x++;
			} else
				charX = line[x];

			SDL_Rect re = {
				kbLeft + xc * KEY_WIDTH - 1, KB_TOP + l * KEY_HEIGHT,
				KEY_WIDTH - 1, KEY_HEIGHT - 2
			};

			//if ts on rect, change selection
			if (ts.available() && ts.pressed() && ts.inRect(re)) {
				selCol = xc;
				selRow = l;
			}

			gmenu2x->s->rectangle(re,
					gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
			gmenu2x->s->write(gmenu2x->font, charX,
					kbLeft + xc * KEY_WIDTH + KEY_WIDTH / 2 - 1,
					KB_TOP + l * KEY_HEIGHT + KEY_HEIGHT / 2,
					ASFont::HAlignCenter, ASFont::VAlignMiddle);
			xc++;
		}
	}

	//Ok/Cancel
	SDL_Rect re = {kbLeft-1, KB_TOP+kb->size()*KEY_HEIGHT, kbLength*KEY_WIDTH/2-1, KEY_HEIGHT-1};
	gmenu2x->s->rectangle(re, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
	if (ts.available() && ts.pressed() && ts.inRect(re)) {
		selCol = 0;
		selRow = kb->size();
	}
	gmenu2x->s->write(gmenu2x->font, gmenu2x->tr["Cancel"],
			(int)(160 - kbLength * KEY_WIDTH / 4),
			KB_TOP + kb->size() * KEY_HEIGHT + KEY_HEIGHT / 2,
			ASFont::HAlignCenter, ASFont::VAlignMiddle);

	re.x = kbLeft + kbLength * KEY_WIDTH / 2 - 1;
	gmenu2x->s->rectangle(re, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
	if (ts.available() && ts.pressed() && ts.inRect(re)) {
		selCol = 1;
		selRow = kb->size();
	}
	gmenu2x->s->write(gmenu2x->font, gmenu2x->tr["OK"],
			(int)(160 + kbLength * KEY_WIDTH / 4),
			KB_TOP + kb->size() * KEY_HEIGHT + KEY_HEIGHT / 2,
			ASFont::HAlignCenter, ASFont::VAlignMiddle);
}
*/