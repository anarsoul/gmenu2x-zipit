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

#include "debug.h"
#include "inputmanager.h"
#include "utilities.h"
#include "powersaver.h"

#include <iostream>
#include <fstream>

using namespace std;

void InputManager::init(const string &conffile) {
	for (int i = 0; i < BUTTON_TYPE_SIZE; i++) {
		buttonMap[i].source = UNMAPPED;
	}
	readConfFile(conffile);
}

InputManager::InputManager()
{
#ifndef SDL_JOYSTICK_DISABLED
	if (SDL_NumJoysticks() > 0) {
		joystick = SDL_JoystickOpen(0);
	} else {
		joystick = NULL;
	}
#endif
}

InputManager::~InputManager() {
#ifndef SDL_JOYSTICK_DISABLED
	if (joystick) {
		SDL_JoystickClose(joystick);
	}
#endif
}

void InputManager::readConfFile(const string &conffile) {
	ifstream inf(conffile.c_str(), ios_base::in);
	if (inf.fail()) {
		ERROR("InputManager: failed to open config file\n");
		return;
	}

	string line;
	while (getline(inf, line, '\n')) {
		string::size_type pos = line.find("=");
		string name = trim(line.substr(0,pos));
		line = trim(line.substr(pos+1,line.length()));

		Button button;
		if (name == "up")            button = UP;
		else if (name == "down")     button = DOWN;
		else if (name == "left")     button = LEFT;
		else if (name == "right")    button = RIGHT;
		else if (name == "accept")   button = ACCEPT;
		else if (name == "cancel")   button = CANCEL;
		else if (name == "altleft")  button = ALTLEFT;
		else if (name == "altright") button = ALTRIGHT;
		else if (name == "menu")     button = MENU;
		else if (name == "settings") button = SETTINGS;
		else if (name == "volup")    button = VOLUP;
		else if (name == "voldown")  button = VOLDOWN;
		else if (name == "power")    button = POWER;
		else if (name == "lock")     button = LOCK;
		else {
			WARNING("InputManager: Ignoring unknown button name \"%s\"\n",
					name.c_str());
			continue;
		}

		pos = line.find(",");
		string sourceStr = trim(line.substr(0,pos));
		line = trim(line.substr(pos+1, line.length()));

		ButtonSource source;
		if (sourceStr == "keyboard") {
			source = KEYBOARD;
#ifndef SDL_JOYSTICK_DISABLED
		} else if (sourceStr == "joystick") {
			source = JOYSTICK;
#endif
		} else {
			WARNING("InputManager: Ignoring unknown button source \"%s\"\n",
					sourceStr.c_str());
			continue;
		}
		buttonMap[button].source = source;
		buttonMap[button].code = atoi(line.c_str());
	}

	inf.close();
}

InputManager::Button InputManager::waitForPressedButton() {
	return waitForButton(PRESSED);
}

InputManager::Button InputManager::waitForReleasedButton() {
	return waitForButton(RELEASED);
}

InputManager::Button InputManager::waitForButton(ButtonState state) {
	ButtonEvent event;
	do {
		waitForEvent(&event);
	} while (event.state != state);
	return event.button;
}

void InputManager::waitForEvent(ButtonEvent *event) {
	getEvent(event, true);
}

bool InputManager::pollEvent(ButtonEvent *event) {
	return getEvent(event, false);
}

bool InputManager::getEvent(ButtonEvent *bevent, bool wait) {
	//TODO: when an event is processed, program a new event
	//in some time, and when it occurs, do a key repeat

#ifndef SDL_JOYSTICK_DISABLED
	if (joystick) {
		SDL_JoystickUpdate();
	}
#endif
	SDL_Event event;
	if (wait) {
		SDL_WaitEvent(&event);
	} else {
		bevent->state = RELEASED;
		if (!SDL_PollEvent(&event)) {
			return false;
		}
	}

	ButtonSource source;
	switch(event.type) {
		case SDL_KEYDOWN:
			bevent->state = PRESSED;
			source = KEYBOARD;
			break;
		case SDL_KEYUP:
			bevent->state = RELEASED;
			source = KEYBOARD;
			break;
#ifndef SDL_JOYSTICK_DISABLED
		case SDL_JOYBUTTONDOWN:
			bevent->state = PRESSED;
			source = JOYSTICK;
			break;
		case SDL_JOYBUTTONUP:
			bevent->state = RELEASED;
			source = JOYSTICK;
			break;
#endif
		default:
			return false;
	}

	if (source == KEYBOARD) {
		for (int i = 0; i < BUTTON_TYPE_SIZE; i++) {
			if (buttonMap[i].source == KEYBOARD
					&& (unsigned int)event.key.keysym.sym == buttonMap[i].code) {
				bevent->button = static_cast<Button>(i);
				break;
			}
		}
#ifndef SDL_JOYSTICK_DISABLED
	} else if (source == JOYSTICK) {
		for (int i = 0; i < BUTTON_TYPE_SIZE; i++) {
			if (buttonMap[i].source == JOYSTICK
					&& (unsigned int)event.jbutton.button == buttonMap[i].code) {
				bevent->button = static_cast<Button>(i);
				break;
			}
		}
#endif
	}
	if (wait && PowerSaver::isRunning()) {
		PowerSaver::getInstance()->resetScreenTimer();
	}

	return true;
}
