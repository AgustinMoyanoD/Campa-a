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

/* Tooltips y Glosario (Sec B1) */
void UI_RequestTooltip(const char* title, const char* text, Vector2 anchorPos);
void UI_RequestGlossaryTooltip(const char* termId, Vector2 anchorPos);
void UI_DrawPendingTooltips(void);

/* Chips de termino con explicacion bajo demanda */
Rectangle UI_DrawChip(const char* termId, Rectangle bounds);
Rectangle UI_DrawChipEx(const char* label, const char* termId, Rectangle bounds, Color bg, Color fg);

#endif
