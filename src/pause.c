#include "include/pause.h"

#include "raylib.h"
#include "raygui.h"

#include "include/global_vars.h"


PauseCode DrawPause(void) {
    BeginDrawing();

        static const int BUTTON_WIDTH = 200;
        static const int BUTTON_HEIGHT = GUI_TEXT_SIZE + GUI_TEXT_MARGIN;
        static const int BUTTON_MARGIN = GUI_TEXT_MARGIN;
        if (GuiButton((Rectangle){ (WINDOW_WIDTH - BUTTON_WIDTH) * 0.5, WINDOW_HEIGHT * 0.5 - BUTTON_HEIGHT - BUTTON_MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT }, "continue"))
        {
            return CONTINUE;
        }
        if (GuiButton((Rectangle){ (WINDOW_WIDTH - BUTTON_WIDTH) * 0.5, WINDOW_HEIGHT * 0.5, BUTTON_WIDTH, BUTTON_HEIGHT }, "restart"))
        {
            return RESTART;
        }
        if (GuiButton((Rectangle){ (WINDOW_WIDTH - BUTTON_WIDTH) * 0.5, WINDOW_HEIGHT * 0.5 + BUTTON_HEIGHT + BUTTON_MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT }, "main menu"))
        {
            return MAIN_MENU;
        }
        if (GuiButton((Rectangle){ (WINDOW_WIDTH - BUTTON_WIDTH) * 0.5, WINDOW_HEIGHT * 0.5 + (BUTTON_HEIGHT + BUTTON_MARGIN) * 2, BUTTON_WIDTH, BUTTON_HEIGHT }, "exit to desktop"))
        {
            return EXIT_TO_DESKTOP;
        }

    EndDrawing();
    return PAUSE;
}
