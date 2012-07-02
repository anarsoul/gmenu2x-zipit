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

#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include "inputmanager.h"
#include "FastDelegate.h"

#include <SDL.h>
#include <string>

class MessageBox;

typedef fastdelegate::FastDelegate2<MessageBox*, int&, void> BlockingAction;

class GMenu2X;

class MessageBox {
public:
	MessageBox(GMenu2X *gmenu2x, const std::string &text,
			const std::string &icon="", BlockingAction act=0);
	void setButton(InputManager::Button button, const std::string &label);
	void setText(const std::string &str);
	
	int exec();

private:
	void Draw();
	
	GMenu2X *gmenu2x;
	std::string text, icon;
	std::string buttons[BUTTON_TYPE_SIZE];
	std::string buttonLabels[BUTTON_TYPE_SIZE];
	SDL_Rect buttonPositions[BUTTON_TYPE_SIZE];
	
	BlockingAction action;
};

#endif // MESSAGEBOX_H
