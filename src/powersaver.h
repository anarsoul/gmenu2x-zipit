#ifndef POWERSAVER_H
#define POWERSAVER_H

#include <SDL.h>
	

enum POWERSTATE {
		AC_POWER,
		DC_POWER
			};
	
class PowerSaver {
public:


	
	static POWERSTATE getPwrState();

	static PowerSaver *getInstance();
	static bool isRunning();
	~PowerSaver();
	void resetScreenTimer();

	void enableScreen();
	void disableScreen();

	void setScreenTimeout(unsigned int seconds);
	void setScreenBlanking(bool state);
	
private:
	PowerSaver();
	void addScreenTimer();
	
	static PowerSaver *instance;
	bool screenState;
	unsigned int screenTimeout;
	unsigned int timeout_startms;
	SDL_TimerID screenTimer;
};

#endif
