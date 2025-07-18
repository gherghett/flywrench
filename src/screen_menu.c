#include "screen_menu.h"
#include "screen_manager.h"
#include "raylib.h"

void ScreenMenu_Init(void) {
    // Initialize menu resources
}

void ScreenMenu_Update(void) {
    // Handle menu input and logic
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        ChangeToScreen(SCREEN_GAMEPLAY);
    }
}

void ScreenMenu_Draw(void) {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    
    // Draw menu elements
    DrawText("FLYWRENCH", GetScreenWidth() / 2 - 100, GetScreenHeight() / 2 - 100, 40, BLACK);
    DrawText("Press ENTER or SPACE to start", GetScreenWidth() / 2 - 150, GetScreenHeight() / 2 - 20, 20, DARKGRAY);
    DrawText("Use Arrow Keys to control", GetScreenWidth() / 2 - 120, GetScreenHeight() / 2 + 20, 16, GRAY);
    // DrawText("Press P to toggle edit mode", GetScreenWidth() / 2 - 120, GetScreenHeight() / 2 + 50, 16, GRAY);
    
    EndDrawing();
}

void ScreenMenu_Unload(void) {
    // Clean up menu resources
}
