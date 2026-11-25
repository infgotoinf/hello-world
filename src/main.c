#include "raylib.h"



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

    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Hello, World!");

    SetExitKey(KEY_NULL);

    int currentFps = 60;

    SetTargetFPS(currentFps);

    // Main loop
    while (!WindowShouldClose())
    {
        float mouseWheel = GetMouseWheelMove();
        if (mouseWheel != 0)
        {
            currentFps += (int)mouseWheel;
            if (currentFps < 0) currentFps = 0;
            SetTargetFPS(currentFps);
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);

            // Draw the help text
            // Determine what help text to show depending on the current FPS target
            const char *fpsText = 0;
            if (currentFps <= 0) fpsText = TextFormat("FPS: unlimited (%i)", GetFPS());
            else fpsText = TextFormat("FPS: %i (target: %i)", GetFPS(), currentFps);
            DrawText(fpsText, 10, 10, 20, DARKGRAY);
            DrawText(TextFormat("Frame time: %02.02f ms", GetFrameTime()*1000.0f), 10, 30, 20, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
