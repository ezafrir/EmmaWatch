#ifndef WATMODE
#define WATMODE

#include "watchled.h"
#include "sharedProgs.h"
#include <EEPROM.h>



namespace emmawatch
{
    class watMode
    {

        private:

        protected:
            
            enum modeType {w_init, w_idle, w_menu, w_time, w_brightness, w_weather, w_DST} curMode;
            int stage; // mode stage

            void modeChange(modeType newMode); // replace the handler in the pointer


        public:

            watMode ** modeHandler; // the mode handler will call the virtual methods ->
            
            watMode(watMode ** wmp);

            virtual ~watMode();

            virtual void exeMode(long timeDif); // not virtual
            
            virtual void click ();

            virtual void dblClick ();

            virtual void timeout();

            long timeoutMS; // mode timeout in ms
            
            int maxMenu; // mode steps

            bool execState; //can be executed
            

            

    };
}
#endif
