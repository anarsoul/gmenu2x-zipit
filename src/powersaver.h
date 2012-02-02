#ifndef POWERSAVER_H
#define POWERSAVER_H

#include <SDL.h>

class PowerSaver {
public:
	static PowerSaver *getInstance();
	static bool isRunning();
	~PowerSaver();
	void resetScreenTimer();

	void enableScreen();
	void disableScreen();

	void setScreenTimeout(unsigned int seconds);

private:
	PowerSaver();
	void addScreenTimer();
	void setScreenBlanking(bool state);

	static PowerSaver *instance;
	bool screenState;
	unsigned int screenTimeout;
	unsigned int timeout_startms;
	SDL_TimerID screenTimer;
};

#endif
