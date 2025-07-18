#include "raylib.h"
#include "screen_manager.c"
#include "screen_gameplay.c"
#include "screen_menu.c"

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 450;
    
    // Initialize window
    InitWindow(screenWidth, screenHeight, "Flywrench");
    
    // Set initial screen
    ChangeToScreen(SCREEN_MENU);
    
    SetTargetFPS(60);
    
    // Main game loop
    while (!WindowShouldClose()) {
        // Process any pending screen changes
        ProcessScreenChange();
        
        // Update current screen
        Update();
        
        // Draw current screen
        Draw();
    }
    
    // Clean up current screen
    Unload();
    
    // Close window
    CloseWindow();
    
    return 0;
}
