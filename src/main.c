#include <stdio.h>

#include "raylib.h"
#include "raygui.h"
#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#include "include/global_vars.h"
#include "include/menus.h"
#include "include/game.h"



void DrawPause(void) {

}

void DrawGameOver(void) {

}



int main ()
{
    // Set window config flags
    SetConfigFlags(
            FLAG_VSYNC_HINT          // Set to try enabling V-Sync on GPU
            | FLAG_WINDOW_HIGHDPI    // Set to support HighDPI
            | FLAG_MSAA_4X_HINT      // Set to try enabling MSAA 4x (will smooth image, but may cause issues)
            // | FLAG_FULLSCREEN_MODE   // Set to run program in fullscreen
            // | FLAG_WINDOW_RESIZABLE  // Set to allow resizable window
            // | FLAG_WINDOW_ALWAYS_RUN // Set to allow windows running while minimized
            // | FLAG_WINDOW_MINIMIZED  // Set to minimize window (iconify)
            // | FLAG_WINDOW_MAXIMIZED  // Set to maximize window (expanded to monitor)
    );

    // Create the window and OpenGL context
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello World from Raylib");

    // Set text size for raygui elements
    GuiSetStyle(DEFAULT, TEXT_SIZE, GUI_TEXT_SIZE);

    // Loading assets
    if (!LoadMainMenu() | !LoadGame()) {
        printf("Failed loading textures.\n");
        return 0;
    }


    // Main loop
#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(DrawMainMenu, FPS, 1);
#else
    SetTargetFPS(FPS);

    while (!WindowShouldClose()) // Detect window close button
    {
        DrawMainMenu();
    }
#endif


    // Cleanup
    UnloadMainMenu();

    // Destroy the window and cleanup the OpenGL context
    CloseWindow();
    return 0;
}
