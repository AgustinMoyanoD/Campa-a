#ifndef ENDSCREEN_VIEW_H
#define ENDSCREEN_VIEW_H

#include "raylib.h"
#include <stdbool.h>
#include "game/controllers/game_master.h"

typedef enum {
    ENDSCREEN_ACTION_NONE = 0,
    ENDSCREEN_ACTION_CONTINUE,
    ENDSCREEN_ACTION_MENU
} EndscreenAction;

/**
 * Dibuja la pantalla de victoria insular y registro de campaña.
 * Retorna la acción elegida (Continuar a la siguiente provincia, Volver al menú, o Ninguna).
 */
EndscreenAction EndscreenView_DrawVictory(const CampaignEndStats* stats, int sw, int sh);

/**
 * Dibuja la pantalla de derrota total.
 * Retorna la acción elegida (Volver al menú, o Ninguna).
 */
EndscreenAction EndscreenView_DrawGameOver(int sw, int sh);

#endif /* ENDSCREEN_VIEW_H */
