
#include "wmIdle.h"

namespace emmawatch
{

//public

    wmIdle::wmIdle(watMode ** wmp):
    watMode::watMode(wmp)
    { 
        timeoutMS = 10000;
        wLed.allBlack();
        wLed.refreshAll();
    }

     
    wmIdle::~wmIdle() {}

    void wmIdle::click () 
    {
        wLed.testLights();
        modeChange(w_time);
    }

    void wmIdle::dblClick () 
    {
        modeChange(w_menu);
    }

}
