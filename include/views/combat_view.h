#ifndef COMBAT_VIEW_H
#define COMBAT_VIEW_H

#include "models/combat.h"
#include <raylib.h>

// Dibuja la pantalla de combate (layout centrado en pantalla completa)
void CombatView_Draw(CombatContext* ctx, int screenWidth, int screenHeight);

// Maneja los inputs del combate (espacio para tirar dados, clicks para seleccionar planes).
// Retorna true si el combate finalizo y debe cerrarse.
bool CombatView_Update(CombatContext* ctx, int screenWidth, int screenHeight);

#endif
