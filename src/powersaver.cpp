#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "powersaver.h"
#include "debug.h"
PowerSaver* PowerSaver::instance = NULL;
Uint32 screenTimerCallback(Uint32 interval, void *param)
{
    DEBUG("Disable Backlight Event\n");
    PowerSaver::getInstance()->disableScreen();
    return 0;
}

PowerSaver* PowerSaver::getInstance() {
    if ( instance == NULL ) {
        instance = new PowerSaver();
    } 
    return instance;
}

PowerSaver::PowerSaver( ) {
    setScreenTimeout(0);
    screenTimer = NULL;
}

PowerSaver::~PowerSaver() {
    SDL_RemoveTimer(screenTimer);
}

void PowerSaver::setScreenTimeout( unsigned int seconds ) {
    screenTimeout = seconds;
    resetScreenTimer();
}

void PowerSaver::resetScreenTimer() {
    if ( screenTimer != NULL ) {
        SDL_RemoveTimer(screenTimer);
    }
    addScreenTimer();
    //If display is off, turn on it
    if ( !screenState ) {
        enableScreen();
    }
}

void PowerSaver::addScreenTimer() {
    //if timeout is zero, don't set timeout
    if ( screenTimeout == 0 ) {
        screenTimer = NULL;
        return;
    }
    screenTimer = SDL_AddTimer(screenTimeout*1000, screenTimerCallback,NULL);
    if ( screenTimer == NULL ) {
		ERROR("Could not initialize SDLTimer: %s\n", SDL_GetError());
    } 
}

#define SCREEN_BLANK_PATH "/sys/class/graphics/fb0/blank"
void PowerSaver::setScreenBlanking( bool state ) {
    const char* path = SCREEN_BLANK_PATH;
    const char* blank = state ? "0" : "1";

	int fd = open(path, O_RDWR);
	if (fd == -1) {
		WARNING("Failed to open '%s': %s\n", path, strerror(errno));
	} else {
		ssize_t written = write(fd, blank, strlen(blank));
		if (written == -1) {
			WARNING("Error writing '%s': %s\n", path, strerror(errno));
		}
		close(fd);
	}
    screenState = state;
}

void PowerSaver::enableScreen() {
    if ( !screenState ) {
        setScreenBlanking(true);
    }
}
void PowerSaver::disableScreen() {
    if ( screenState ) {
        setScreenBlanking(false);
    }
}


