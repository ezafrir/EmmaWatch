
#include "wmDST.h"
//bool wDST;
namespace emmawatch
{
//private

    void wmDST::DSTShow() {
        
        wLed.allBlack();
        if (m_tmpDST) {
            int ledpos[6]  = {4,5,6,7,8,9}; //d
            for (int i=0; i<6;i++) leds[ledpos[i]] = CRGB(202,128,0);    
        } else {
            int ledpos[4]  = {5,7,8,10}; //s
            for (int i=0; i<4;i++) leds[ledpos[i]] = CRGB::Blue;       
        }
        wLed.refreshAll();
    }

//public

    wmDST::wmDST(watMode ** wmp):
    watMode::watMode(wmp)
    { 
        timeoutMS = 5000;
        m_tmpDST = wDST;
        DSTShow();
    }
     
    wmDST::~wmDST() {}

    void wmDST::click () 
    {
        m_tmpDST = !m_tmpDST;
        DSTShow();
    }

    void wmDST::exeMode (long timeDif) 
    {
        const char* ntpServer = NPTSERVER;
        const long  gmtOffset_sec = NYC_TIME; // ETDST = tmpDST;
        
        if (m_tmpDST != wDST) { //DST changed
            wDST = m_tmpDST;
            EEPROM.write(1, (int) (wDST?1:0));
            EEPROM.commit(); //Store on flash for next time
            const int daylightOffset_sec = (int) (wDST?1:0) * 3600;
            configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // apply change
        }
  
        timeout(); //exit
  
    }

    void wmDST::timeout() {

        wLed.allBlack();
        wLed.refreshAll();
        modeChange(w_idle);
    }

}
