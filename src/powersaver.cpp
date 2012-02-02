#include "powersaver.h"
#include "debug.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

PowerSaver *PowerSaver::instance = NULL;

Uint32 screenTimerCallback(Uint32 timeout, void *d) {
	unsigned int * old_ticks = (unsigned int *) d;
	unsigned int new_ticks = SDL_GetTicks();

	if (new_ticks > *old_ticks + timeout + 1000) {
		DEBUG("Suspend occured, restarting timer\n");
		*old_ticks = new_ticks;
		return timeout;
	}

	DEBUG("Disable Backlight Event\n");
	PowerSaver::getInstance()->disableScreen();
	return 0;
}

PowerSaver *PowerSaver::getInstance() {
	if (instance == NULL) {
		instance = new PowerSaver();
	}
	return instance;
}

bool PowerSaver::isRunning() {
	return instance != NULL;
}

PowerSaver::PowerSaver() {
	SDL_InitSubSystem(SDL_INIT_TIMER);
	setScreenTimeout(0);
	screenTimer = NULL;
}

PowerSaver::~PowerSaver() {
	SDL_RemoveTimer(screenTimer);
	SDL_QuitSubSystem(SDL_INIT_TIMER);
	instance = NULL;
}

void PowerSaver::setScreenTimeout(unsigned int seconds) {
	screenTimeout = seconds;
	resetScreenTimer();
}

void PowerSaver::resetScreenTimer() {
	if (screenTimer != NULL) {
		SDL_RemoveTimer(screenTimer);
	}

	addScreenTimer();
	// If display is off, turn on it.
	if (!screenState) {
		enableScreen();
	}
}

void PowerSaver::addScreenTimer() {
	// If timeout is zero, don't set timeout.
	if (screenTimeout == 0) {
		screenTimer = NULL;
		return;
	}

	timeout_startms = SDL_GetTicks();
	screenTimer = SDL_AddTimer(screenTimeout * 1000, screenTimerCallback, &timeout_startms);
	if (screenTimer == NULL) {
		ERROR("Could not initialize SDLTimer: %s\n", SDL_GetError());
	}
}

#define SCREEN_BLANK_PATH "/sys/class/graphics/fb0/blank"
void PowerSaver::setScreenBlanking(bool state) {
	const char *path = SCREEN_BLANK_PATH;
	const char *blank = state ? "0" : "1";

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
	if (!screenState) {
		setScreenBlanking(true);
	}
}

void PowerSaver::disableScreen() {
	if (screenState) {
		setScreenBlanking(false);
	}
}
