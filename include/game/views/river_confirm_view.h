#ifndef RIVER_CONFIRM_VIEW_H
#define RIVER_CONFIRM_VIEW_H

#include "raylib.h"
#include <stdbool.h>
#include "game/models/hex_tile.h"
#include "game/models/balance_config.h"

typedef enum {
    RIVER_ACTION_NONE = 0,
    RIVER_ACTION_CONFIRM,
    RIVER_ACTION_REROUTE_SAFE,
    RIVER_ACTION_CANCEL
} RiverConfirmAction;

/**
 * Dibuja el modal de confirmación y advertencia previa de cruce de río sin puente.
 * Las probabilidades y porcentajes se leen dinámicamente de BalanceConfig (Hallazgo 1).
 * Retorna la acción seleccionada por el jugador.
 */
RiverConfirmAction RiverConfirmView_Draw(const HexTile* targetTile, const BalanceConfig* bcfg, int sw, int sh);

#endif /* RIVER_CONFIRM_VIEW_H */
