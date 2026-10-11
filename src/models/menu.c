#include "models/menu.h"

void Menu_Init(MainMenu* menu) {
    if (!menu) return;
    menu->state = MENU_STATE_MAIN;
    menu->startGame = false;
    menu->quitGame = false;
}

void Menu_Update(MainMenu* menu) {
    // Background logic
}
