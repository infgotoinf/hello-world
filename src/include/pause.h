#ifndef PAUSE_H
#define PAUSE_H

#include <stdbool.h>



typedef enum {
    PAUSE
   ,CONTINUE
   ,RESTART
   ,MAIN_MENU
   ,EXIT_TO_DESKTOP
} PauseCode;



PauseCode DrawPause(void);



#endif // PAUSE_H
