#ifndef MENU_VIEW_H
#define MENU_VIEW_H

#include "models/menu.h"
#include <stdbool.h>

void MenuView_Init(void);
void MenuView_Draw(MainMenu* menu, int screenWidth, int screenHeight);
bool MenuView_Update(MainMenu* menu, int screenWidth, int screenHeight);

#endif
