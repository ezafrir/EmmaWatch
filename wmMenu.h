#ifndef WTMENU
#define WTMENU

#include "watMode.h"


namespace emmawatch
{
    class wmMenu:  public watMode
    {     
        //private:
        
        public:
        
            wmMenu(watMode ** wmp);

            ~wmMenu();

            void exeMode(long timeDif); // not virtual
            
            void click ();

            //void dblClick ();

            void timeout();


    };
}

#endif
