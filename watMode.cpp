#include "watMode.h"
#include "wmIdle.h"
#include "wmTime.h"
#include "wmDST.h"
#include "wmMenu.h"

namespace emmawatch
{

//protected

    void watMode::modeChange(modeType newMode) {

        watMode *newHdl;
        bool cngMode = true;

        switch (newMode)
        {
            case w_menu:
                newHdl = new wmMenu(modeHandler);
                break;
                
            case w_idle:
                newHdl = new wmIdle(modeHandler);
                break;

            case w_time:
                newHdl = new wmTime(modeHandler);
                break;

            case w_DST:
                newHdl = new wmDST(modeHandler);
                break;
            
            // case w_init:
            //     newHdl = new wmInit(modeHandler);
            //     break;

            default:
                cngMode = false; //safety
                Serial.println("modeChangeError"); //debug
                break;
        }

        if (cngMode) {
            *modeHandler = newHdl;
            Serial.println("modeChange"); //debug
            delete this;
        }

    }

//public

    watMode::watMode(watMode ** wmp):
    modeHandler(wmp)
    {}

     
    watMode::~watMode() {}
    
   void watMode::exeMode(long timeDif) {}
   
   void watMode::click () {}

   void watMode::dblClick () {}

   void watMode::timeout() {}

}
