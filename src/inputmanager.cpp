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

#include <SDL.h>

#include <iostream>
#include <fstream>

using namespace std;

static SDL_Joystick *joystick;

void InputManager::init(const string &conffile) {
	if (!readConfFile(conffile)) {
		ERROR("InputManager initialization from config file failed.\n");
	}
}

InputManager::InputManager() {
	initJoystick();
}

InputManager::~InputManager() {
	if (SDL_NumJoysticks() > 0) {
		SDL_JoystickClose(joystick);
	}
}

void InputManager::initJoystick() {
	if (SDL_NumJoysticks() > 0) {
		joystick = SDL_JoystickOpen(0);
	}
}

bool InputManager::readConfFile(const string &conffile) {
	if (!fileExists(conffile)) {
		return false;
	}

	ifstream inf(conffile.c_str(), ios_base::in);
	if (!(inf.is_open())) {
		return false;
	}

	string line, name, source;
	string::size_type pos;
	buttontype_t button;

	while(getline(inf, line, '\n')) {
		pos = line.find("=");
		name = trim(line.substr(0,pos));
		line = trim(line.substr(pos+1,line.length()));

		if (name == "up")            button = UP;
		else if (name == "down")     button = DOWN;
		else if (name == "left")     button = LEFT;
		else if (name == "right")    button = RIGHT;
		else if (name == "accept")   button = ACCEPT;
		else if (name == "cancel")   button = CANCEL;
		else if (name == "clear")    button = CLEAR;
		else if (name == "manual")   button = MANUAL;
		else if (name == "altleft")  button = ALTLEFT;
		else if (name == "altright") button = ALTRIGHT;
		else if (name == "menu")     button = MENU;
		else if (name == "settings") button = SETTINGS;
		else if (name == "volup")    button = VOLUP;
		else if (name == "voldown")  button = VOLDOWN;
		else if (name == "power")    button = POWER;
		else if (name == "lock")     button = LOCK;
		else return false;

		pos = line.find(",");
		source = trim(line.substr(0,pos));
		line = trim(line.substr(pos+1, line.length()));

		if (source == "keyboard") ButtonMap[button].source = KEYBOARD;
		else if (source == "joystick") ButtonMap[button].source = JOYSTICK;
		else return false;

		ButtonMap[button].code = atoi(line.c_str());
	}

	inf.close();
	return true;
}

buttontype_t InputManager::waitForPressedButton() {
	return waitForButton(PRESSED);
}

buttontype_t InputManager::waitForReleasedButton() {
	return waitForButton(RELEASED);
}

buttontype_t InputManager::waitForButton(enum state_e state) {
	bevent_t event;
	do {
		waitForEvent(&event);
	} while (event.state != state);
	return event.button;
}

void InputManager::waitForEvent(bevent_t *event) {
	getEvent(event, true);
}

bool InputManager::pollEvent(bevent_t *event) {
	return getEvent(event, false);
}

bool InputManager::getEvent(bevent_t *bevent, bool wait) {
	//TODO: when an event is processed, program a new event
	//in some time, and when it occurs, do a key repeat

	SDL_JoystickUpdate();
	SDL_Event event;
	if (wait) {
		SDL_WaitEvent(&event);
	} else {
		bevent->state = RELEASED;
		if (!SDL_PollEvent(&event)) {
			return false;
		}
	}

	enum source_type_e source;
	switch(event.type) {
		case SDL_KEYDOWN:
			bevent->state = PRESSED;
			source = KEYBOARD;
			break;
		case SDL_KEYUP:
			bevent->state = RELEASED;
			source = KEYBOARD;
			break;
		case SDL_JOYBUTTONDOWN:
			bevent->state = PRESSED;
			source = JOYSTICK;
			break;
		case SDL_JOYBUTTONUP:
			bevent->state = RELEASED;
			source = JOYSTICK;
			break;
		default:
			return false;
	}

	if (source == KEYBOARD) {
		for (int i=0; i<BUTTONTYPE_T_SIZE; i++) {
			if (ButtonMap[i].source == KEYBOARD
					&& (unsigned int)event.key.keysym.sym == ButtonMap[i].code) {
				bevent->button = (buttontype_t)i;
				break;
			}
		}
	} else {
		for (int i=0; i<BUTTONTYPE_T_SIZE; i++) {
			if (ButtonMap[i].source == JOYSTICK
					&& (unsigned int)event.jbutton.button == ButtonMap[i].code) {
				bevent->button = (buttontype_t)i;
				break;
			}
		}
	}
	if (wait && PowerSaver::isRunning()) {
		PowerSaver::getInstance()->resetScreenTimer();
	}

	return true;
}
