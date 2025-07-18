// screen_menu.h
#ifndef SCREEN_MENU_H
#define SCREEN_MENU_H

#include "raylib.h"

// Forward declarations
void ScreenMenu_Init(void);
void ScreenMenu_Update(void);
void ScreenMenu_Draw(void);
void ScreenMenu_Unload(void);

#ifdef SCREEN_MENU_IMPLEMENTATION

// Menu screen implementation
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
    DrawText("FLYWRENCH", 300, 150, 40, DARKGRAY);
    DrawText("Press ENTER or SPACE to start", 250, 250, 20, GRAY);
    DrawText("ESC to quit", 350, 300, 20, GRAY);
    
    EndDrawing();
}

void ScreenMenu_Unload(void) {
    // Clean up menu resources
}

#endif // SCREEN_MENU_IMPLEMENTATION

#endif // SCREEN_MENU_H
