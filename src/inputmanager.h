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
#ifndef NEWINPUT_H
#define NEWINPUT_H

#include <SDL/SDL.h>
#include <sstream>

using namespace std;

typedef enum buttontype_e {
    UP, DOWN, LEFT, RIGHT,
    ACCEPT, CANCEL,
    CLEAR, MANUAL,
    ALTLEFT, ALTRIGHT,
    MENU, SETTINGS,
    VOLUP, VOLDOWN,
    POWER, LOCK
} buttontype_t;
#define BUTTONTYPE_T_SIZE 16

enum source_type_e {KEYBOARD, JOYSTICK};
enum state_e {PRESSED, RELEASED};

typedef struct {
    source_type_e source;
    Uint32 code;
} input_t;

typedef struct {
    buttontype_t button;
    state_e state;
} bevent_t;


class InputManager {
private:
    input_t ButtonMap[BUTTONTYPE_T_SIZE];

    bool readConfFile(const string &conffile);
    void initJoystick();
    bool getEvent(bevent_t *bevent, bool wait);
    buttontype_t waitForButton(enum state_e state);

public:
    InputManager();
    ~InputManager();

    void init(const string &conffile);
    void waitForEvent(bevent_t *event);
    buttontype_t waitForPressedButton();
    buttontype_t waitForReleasedButton();
    bool pollEvent(bevent_t *event);
};

#endif
