#include "wmMenu.h"

namespace emmawatch
{

//private
    

//public

    wmMenu::wmMenu(watMode ** wmp):
    watMode::watMode(wmp)
    { 
        timeoutMS = 5000;
        stage = 1;
        maxMenu = 6;
        //execState = true;
        wLed.allBlack();
        leds[0] = CRGB::Purple;
        wLed.refreshAll();
    }

     
    wmMenu::~wmMenu() {}

    
    void wmMenu::exeMode(long timeDif) {
        
        wLed.allBlack();
        wLed.refreshAll();
        Serial.print("exec stage");
        Serial.println(stage);
        switch (stage) {
        case 1: 
            modeChange(w_idle);
            break;

        case 2:
            wLed.testLights();
            modeChange(w_idle);
            break;

        case 3:
            hotspotConnect();
            modeChange(w_idle);
            break;


        case 4:
            modeChange(w_DST);
            break;

        case 5: // temp autokill toggle
            AutoKill = !AutoKill;
            if (AutoKill) leds[4] = CRGB::Red;
            else leds[4] = CRGB::Green;
            wLed.refreshAll();
            delay(500);
            wLed.allBlack();
            wLed.refreshAll();
            break;

        case 6:
            AutoSleep = !AutoSleep;
            if (AutoSleep) leds[5] = CRGB::Red;
            else leds[5] = CRGB::Green;
            wLed.refreshAll();
            delay(500);
            wLed.allBlack();
            wLed.refreshAll();
            break;

        default:
            modeChange(w_idle);
            break;
        }

    }
    
    void wmMenu::click () {

        if (stage < maxMenu) {
            leds[stage-1] = CRGB::Black;
            leds[stage++] = CRGB::Purple;
            Serial.print("cng stage");
            Serial.println(stage);
            wLed.refreshAll();
        }
        else timeout();
    }

    void wmMenu::timeout() {
        leds[stage-1] = CRGB::Black;
        wLed.refreshAll();
        modeChange(w_time);
    }

//    void watMode::dblClick () {

//         modeChange(w_menu);
//    }

}
