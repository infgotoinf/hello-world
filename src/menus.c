#include "include/menus.h"

#include <stdbool.h>
#include <stdio.h>

#include "raylib.h"
#include "raygui.h"

#include "include/global_vars.h"
#include "include/game.h"



static Texture logo;



bool LoadMainMenu(void) {
    logo = LoadTexture("assets/art/interface/logo.png");
    if (!IsTextureValid(logo)) {
        printf("Couldn't load Texture!\n");
        return false;
    }
    return true;
}


void UnloadMainMenu(void) {
    UnloadTexture(logo);
}


void DrawMainMenu(void) {
    static bool play = false;
    static bool prev_freme_play = false;

    if (prev_freme_play != play) {
        if (prev_freme_play == false)
            DisableCursor();
        else
            EnableCursor();
    }

    if (play) {
        DrawGame();
        prev_freme_play = true;
    }
    else {
        BeginDrawing();

            ClearBackground(DARKGRAY);

            Vector2 logo_pos = { (WINDOW_WIDTH - logo.width) * 0.5, 100 };
            DrawTexture(logo, logo_pos.x, logo_pos.y, WHITE);

            // Draw raygui button
            static const int BUTTON_WIDTH = 200;
            if (GuiButton((Rectangle){ logo_pos.x + logo.width * 0.5 - BUTTON_WIDTH * 0.5
                                     , logo_pos.y + logo.height + 100
                                     , BUTTON_WIDTH
                                     , GUI_TEXT_SIZE + GUI_TEXT_MARGIN }, "Play"))
            {
                play = true;
            }

            // DrawText("Hello, World!", 200, WINDOW_HEIGHT - 100, GUI_TEXT_SIZE, WHITE);
            // DrawText(TextFormat("Mouse Pos: %03.02f %03.02f", GetMouseDelta().x, GetMouseDelta().y), 200, WINDOW_HEIGHT - 100, GUI_TEXT_SIZE, WHITE);
        EndDrawing();
    }
}
