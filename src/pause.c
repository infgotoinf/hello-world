#include "include/pause.h"

#include "raylib.h"
#include "raygui.h"

#include "include/global_vars.h"


bool DrawPause(void) {
    BeginDrawing();

        if (GuiButton((Rectangle){ WINDOW_WIDTH * 0.5, WINDOW_HEIGHT * 0.5, 200, GUI_TEXT_SIZE + GUI_TEXT_MARGIN }, "continue"))
        {
            return false;
        }
        // else if (customButton(textures["buttons_restart"], screen_width / 2, screen_height / 2, &pause_restart_status))
        // {

        // }
        // else if (customButton(textures["buttons_end"], screen_width / 2, screen_height / 2 + 50, &pause_end_status))
        // {

        // }

    EndDrawing();
    return true;
}
