#include "screen_manager.h"
#include "screen_gameplay.h"
#include "screen_menu.h"
#include <stdbool.h>


static GameScreen currentScreen = SCREEN_MENU;
static bool screenChangeRequested = false;
static GameScreen nextScreen;

static void (*Screen_Init[])(void) = {  ScreenMenu_Init, ScreenGameplay_Init };
static void (*Screen_Update[])(void) = {  ScreenMenu_Update, ScreenGameplay_Update };
static void (*Screen_Draw[])(void) = {  ScreenMenu_Draw, ScreenGameplay_Draw };
static void (*Screen_Unload[])(void) = {  ScreenMenu_Unload, ScreenGameplay_Unload };

void ChangeToScreen(GameScreen screen)
{
    screenChangeRequested = true;
    nextScreen = screen;
}

void ProcessScreenChange(void)
{
    if (screenChangeRequested)
    {
        Screen_Unload[currentScreen]();
        currentScreen = nextScreen;
        Screen_Init[currentScreen]();
        screenChangeRequested = false;
    }
}

void Update(void) {
    Screen_Update[currentScreen]();
}
void Draw(void) {
    Screen_Draw[currentScreen]();
}
void Unload(void) {
    Screen_Unload[currentScreen]();
}