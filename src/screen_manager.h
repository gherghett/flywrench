// screen_manager.h
#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

// #include "screens.h"  // optional: just for the GameScreen enum

typedef enum GameScreen { SCREEN_MENU, SCREEN_GAMEPLAY } GameScreen;

void ChangeToScreen(GameScreen screen);
void ProcessScreenChange(void);

void Update(void);
void Draw(void);
void Unload(void);


#endif
