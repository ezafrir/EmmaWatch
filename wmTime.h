#ifndef WTTIME
#define WTTIME

#define WT_IDLE_CNT  180 // timeout in seconds
#define WT_DIM_CNT 30 

#include "watMode.h"
#include "time.h"

extern struct tm timeinfo;

namespace emmawatch
{
    class wmTime:  public watMode
    {    
        private: 
            int m_aggTimeout;
            void refreshTime();

        public:
            wmTime(watMode ** wmp);

            ~wmTime();

            //void exeMode(long timeDif); // not virtual
            
            void click ();

            void dblClick ();

            void timeout();
    };
}

#endif
