#ifndef UI_CARDS_H
#define UI_CARDS_H

#include "raylib.h"
#include <stdbool.h>

struct ModifierDef;

/* Ciclo de vida de los marcos de cartas (card_frame_divine.png y card_frame_curse.png) */
void UICards_Init(void);
void UICards_Shutdown(void);

/* Dibuja una carta divina o maldición completa dentro del rectángulo indicado */
void UI_DrawModifierCard(const struct ModifierDef* def, Rectangle bounds);
void UI_DrawModifierCardEx(const struct ModifierDef* def, Rectangle bounds, bool isExhausted, int battlesUntilRecharge);

#endif /* UI_CARDS_H */
