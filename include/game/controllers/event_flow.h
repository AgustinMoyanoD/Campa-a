#ifndef EVENT_FLOW_H
#define EVENT_FLOW_H

#include "game/controllers/game_master.h"
#include "game/models/army.h"
#include "game/models/hex_tile.h"

typedef struct {
    void* lastAudioEvent;
    EnemySquad eventEnemy;
} EventFlowState;

void EventFlow_Init(EventFlowState* state);

/**
 * Actualiza la lógica de eventos: tiempo de lectura, selección de opciones,
 * sorteo ponderado con RNG determinista, aplicación de resultados, modificadores,
 * desencadenamiento de combate y reapertura de ciudades.
 */
void EventFlow_Update(EventFlowState* state, GameMaster* gm, Army* playerArmy, Map* map, float dt);

/**
 * Renderizado puro de la pantalla de evento o resultado activo.
 * Cero mutaciones de estado en el dibujo (Meta 3).
 */
void EventFlow_Draw(const GameMaster* gm, int sw, int sh);

#endif /* EVENT_FLOW_H */
