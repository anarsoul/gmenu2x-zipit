#ifndef POWERSAVER_H
#define POWERSAVER_H
#include <SDL.h>
class PowerSaver {

    public:
        static PowerSaver* getInstance();
		static bool isRunning();
        ~PowerSaver();
        void addScreenTimer();
        void resetScreenTimer();

        void enableScreen();
        void disableScreen();

        void setScreenTimeout( unsigned int seconds );
    private:
        PowerSaver( );
        static PowerSaver* instance;
        bool screenState;
        unsigned int screenTimeout;
        SDL_TimerID screenTimer;
    
        void setScreenBlanking( bool state ); 
};
#endif
