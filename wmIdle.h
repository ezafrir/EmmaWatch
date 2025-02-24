#ifndef WTIDLE
#define WTIDLE

#include "watMode.h"

namespace emmawatch
{
    class wmIdle:  public watMode
    {    
        private: 

        public:
            wmIdle(watMode ** wmp);

            ~wmIdle();

            //void exeMode(long timeDif); // not virtual
            
            void click ();

            void dblClick ();

            //void timeout();
    };
}

#endif
