#ifndef MENU_H
#define MENU_H

#include <stdbool.h>

typedef enum {
    MENU_STATE_MAIN,
    MENU_STATE_OPTIONS,
    MENU_STATE_CREDITS
} MenuState;

typedef struct {
    MenuState state;
    bool startGame;
    bool quitGame;
} MainMenu;

void Menu_Init(MainMenu* menu);
void Menu_Update(MainMenu* menu);

#endif
