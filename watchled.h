#ifndef WATCHLED
#define WATCHLED

#include <FastLED.h>
#include "sharedProgs.h"

#define LED_PIN 13 
#define NUM_LEDS 18  
#define LED_DELAY 30

#define WT_DIM_LEVEL 20 
#define WT_MAX_LEVEL 200 


namespace emmawatch
{
    class watchLed
    {
        private:
            int m_delayMS;
            CRGB m_leds[NUM_LEDS];
            CRGB m_color;

        public:
        
            enum wlbrightness {wlbright_max = WT_MAX_LEVEL, wlbright_dim = WT_DIM_LEVEL};
            enum { Rows = 4, Cols = 4 };
            
            watchLed(int showDelay, CRGB initColor);

            virtual ~watchLed();

            CRGB * init();

            CRGB & operator () ( int c, int r);
            CRGB operator () ( int c, int r) const;

            void lInd(CRGB col);

            void rInd(CRGB col);

            int delayMS();

            void setrow(int rn, int val, CRGB col);
            void setcol(int cn, int val, CRGB col);
            CRGB watchColor();
            void watchBrightness(wlbrightness wlb);
            void allBlack();
            void refreshAll(unsigned long delayTime=0);

            //void ColArr(int * lArr, int lAsize, CRGB col);



            //programs
            void testLights();
            void exitL(CRGB colL);

    };
}

extern emmawatch::watchLed wLed;
extern CRGB * leds;

#endif
