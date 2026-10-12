#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include "raylib.h"
#include <stdbool.h>

void UI_Init(void);
void UI_Shutdown(void);
void UI_DrawPanel(Rectangle bounds);
bool UI_DrawButton(Rectangle bounds, const char* text, bool disabled);
bool UI_ButtonClicked(Rectangle bounds);
void UI_DrawBar(Rectangle bounds, float percentage, Color fg, Color bg);
void UI_DrawText(const char* text, Vector2 pos, float fontSize, Color color);

#endif
