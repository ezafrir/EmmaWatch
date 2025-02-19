#include "watchled.h"

namespace emmawatch
{

//public

    watchLed::watchLed(int showDelay, CRGB  initColor):
    m_delayMS(showDelay),
    m_color(initColor)
    { }

     
    watchLed::~watchLed() {}

    CRGB & watchLed::operator () ( int c, int r) //set returns ref to the array
    {
        return m_leds[r+Rows*c];
        //CRGB * ledCell = m_leds + r + Rows*c; // one function for get and set with return type : CRGB * 
        //return ledCell;
        //*wLed(1,1) = CRGB::Blue; 
        // Serial.print(*wLed(1,1));  
        // vs
        // wLed(1,1) = CRGB::Blue;
        // Serial.print(wLed(1,1));
    }

    CRGB watchLed::operator () ( int c, int r) const //get (const at the end means member vars cannot be changed)
    {
        return m_leds[r+Rows*c];
    }

    
    CRGB * watchLed::init() 
    {
        FastLED.addLeds<WS2812, LED_PIN, GRB>(m_leds, NUM_LEDS);
        fill_solid(m_leds,NUM_LEDS,m_color);
        FastLED.delay(m_delayMS);
        delay(500);
        FastLED.clear();
        FastLED.delay(m_delayMS);

        return m_leds;
    }

    void watchLed::lInd(CRGB col)
    {
        m_leds[NUM_LEDS-2] = col;
        FastLED.delay(m_delayMS);
    }

    void watchLed::rInd(CRGB col)
    {
        m_leds[NUM_LEDS-1] = col;
        FastLED.delay(m_delayMS);
    }
    
    void watchLed::setrow(int rn, int val, CRGB col)
    {
        for (int i=0; i<Cols; i++) m_leds[rn+i*Rows] = (val >> i & 0x1)?col:CRGB::Black;
        FastLED.delay(m_delayMS);  
    }

    void watchLed::setcol(int cn, int val, CRGB col)
    {
        for (int i=0; i<Rows; i++) m_leds[cn*Rows+i] = (val >> i & 0x1)?col:CRGB::Black;
        FastLED.delay(m_delayMS);  
    }

    int watchLed::delayMS() {
        return m_delayMS;
    }

    CRGB watchLed::watchColor() {
        return m_color;
    }

    void watchLed::watchBrightness(wlbrightness wlb) {
        FastLED.setBrightness(wlb);
    }
    
    void watchLed:: allBlack() {
        FastLED.clear();
    }

    void watchLed:: refreshAll(unsigned long delayTime) {
        
        if (!delayTime) delayTime = m_delayMS;
        FastLED.delay(delayTime);  
    }

    //programs //
    //--------//
    void watchLed::testLights() {
    
        Serial.println("testlight");

        m_leds[16] = CRGB::Blue;
        m_leds[17] = CRGB::Red;
        m_leds[0] = CRGB(random(255),random(255),random(255));
        FastLED.delay(10);
        delay(15);
        for (int i=1; i< 16; i++) {
        m_leds[i] = CRGB(random(255),random(255),random(255));
        m_leds[i-1]=0;
        FastLED.delay(10);
        delay(15);
        }
        FastLED.clear();
        FastLED.delay(m_delayMS);

    }

    
    // void watchLed::ColArr(int * lArr, int lAsize, CRGB col) {
        
    //     FastLED.clear();
    //     for (int i=0; i < lAsize; i++) m_leds[*(lArr+i)] = col;
    //     FastLED.delay(m_delayMS);
    // }


    void watchLed::exitL(CRGB colL) {

        const int ledSide = (NUM_LEDS-2)/4;
        const int delayT = 580;

        int x = ledSide - 1;
        int y = x;

        fill_solid(m_leds,ledSide*ledSide,colL);
        FastLED.delay(10);
        for (int i=0;i<ledSide;i++) {
            (*this)(x,y--) = CRGB::Black;
            FastLED.delay(delayT);
        }
        int dxy = 1;
        y++;
        for (int j=1;j<ledSide;j++) {
            for (int i=0;i<ledSide*ledSide;i++) m_leds[i] /=3; //dim
            FastLED.delay(10);
            for (int i=0;i<ledSide-j;i++) {
                x -= dxy;
                (*this)(x,y) = CRGB::Black;
                FastLED.delay(delayT);
            }
            for (int i=0;i<ledSide*ledSide;i++) m_leds[i] /=3; //dim
            FastLED.delay(10);
            for (int i=0;i<ledSide-j;i++) {
                y += dxy;
                (*this)(x,y) = CRGB::Black;
                FastLED.delay(delayT);
            }
            dxy = -dxy;
        }

    }
}
