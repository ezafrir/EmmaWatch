
#include "wmTime.h"

namespace emmawatch
{
//Private

    void wmTime::refreshTime() {
        
        updateTimeN(); 
        int minD1 = timeinfo.tm_min % 10; // ones
        int hrD1 = timeinfo.tm_hour % 10; // ones
        wLed.setcol(0,minD1,wLed.watchColor());
        wLed.setcol(1,(timeinfo.tm_min-minD1)/10,wLed.watchColor());
        wLed.setcol(2,hrD1,wLed.watchColor());
        wLed.setcol(3,(timeinfo.tm_hour-hrD1)/10,wLed.watchColor());
        wLed.refreshAll();
    }

//public

    wmTime::wmTime(watMode ** wmp):
    watMode::watMode(wmp)
    { 
        timeoutMS = 5000; //10sec
        m_aggTimeout = 0;
        wLed.watchBrightness(wLed.wlbright_max);
        refreshTime();
    }

     
    wmTime::~wmTime() {}

    void wmTime::click () 
    {
        
        m_aggTimeout = 0;
        wLed.watchBrightness(wLed.wlbright_max);
        //wLed.testLights();
        //brighten
    }

    void wmTime::dblClick () 
    {
        modeChange(w_menu);
    }
    
    void wmTime::timeout () 
    {

        m_aggTimeout +=timeoutMS;

        if (m_aggTimeout < WT_IDLE_CNT * 1000) {
            if (m_aggTimeout > 1000 * WT_DIM_CNT) wLed.watchBrightness(wLed.wlbright_dim);
            refreshTime();
        }
        else modeChange(w_idle);

    }

}
