#include "include/main_menu.h"

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
    static bool prevFramePlay = false;

    if (prevFramePlay != play) {
        if (prevFramePlay == false)
        {
            LoadGame();
            DisableCursor();
        }
        else
        {
            EnableCursor();
        }

        prevFramePlay = !prevFramePlay;
    }

    if (play) {
        DrawGame();
    }
    else {
        BeginDrawing();

            ClearBackground(DARKGRAY);

            Vector2 logo_pos = { (float)(WINDOW_WIDTH - logo.width) * 0.5f, 100 };
            DrawTexture(logo, (int)logo_pos.x, (int)logo_pos.y, WHITE);

            // Draw raygui button
            static const int BUTTON_WIDTH = 200;
            if (GuiButton((Rectangle){ (float)logo_pos.x + (float)(logo.width - BUTTON_WIDTH) * 0.5f
                                     , logo_pos.y + (float)logo.height + 100.f
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
