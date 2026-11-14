#ifndef GAME_H
#define GAME_H

#include <stdbool.h>



typedef enum {
    PAUSE
   ,CONTINUE
   ,RESTART
   ,MAIN_MENU
   ,EXIT_TO_DESKTOP
} PauseCode;



bool LoadGame(void);

void UnloadGame(void);

void DrawGame(void);

PauseCode DrawPause(void);



#endif // GAME_H
