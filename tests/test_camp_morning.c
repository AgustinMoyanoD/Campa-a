#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "models/army.h"
#include "models/general.h"
#include "models/hex_tile.h"
#include "models/map_generator.h"
#include "models/balance_config.h"
#include "controllers/game_master.h"
#include "models/day_clock.h"

int main(void) {
    printf("====================================================\n");
    printf("PRUEBA A1: VERIFICAR MOVIMIENTO TRAS ACAMPAR (DAWN)\n");
    printf("====================================================\n");

    BalanceConfig cfg;
    Balance_InitDefaults(&cfg);

    Map map;
    map.tileCount = 7;
    map.tiles = (HexTile*)calloc(map.tileCount, sizeof(HexTile));
    for (int i = 0; i < map.tileCount; i++) {
        map.tiles[i].coord = (HexCoord){i, 0};
        map.tiles[i].terrain = TERRAIN_LAND;
        map.tiles[i].biome = BIOME_PLAINS;
        map.tiles[i].visible = true;
        map.tiles[i].visited = true;
    }

    General gen = General_Create("Comandante Test", 30, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 0}, gen, 2, 10, 5, 200.0f, 200.0f, 100, 0);

    GameMaster gm;
    GameMaster_Init(&gm);

    /* Avanzar reloj hasta el Atardecer (TIME_AFTERNOON) */
    gm.clock.timeOfDay = TIME_AFTERNOON;
    gm.timeOfDay = TIME_AFTERNOON;

    printf("[1] Simulando decision de acampar al atardecer...\n");
    GameMaster_ChooseCamp(&gm, &map, &army);

    /* Verificaciones post-acampada */
    printf("    isEncamped: %s (Esperado: false)\n", gm.isEncamped ? "true" : "false");
    assert(!gm.isEncamped);

    printf("    timeOfDay: %d (Esperado: TIME_MORNING = %d)\n", gm.timeOfDay, TIME_MORNING);
    assert(gm.timeOfDay == TIME_MORNING);

    printf("    hasRolledThisTurn: %s (Esperado: false)\n", gm.hasRolledThisTurn ? "true" : "false");
    assert(!gm.hasRolledThisTurn);

    printf("    phase: %d (Esperado: PHASE_PLAYER = %d)\n", gm.phase, PHASE_PLAYER);
    assert(gm.phase == PHASE_PLAYER);

    /* Tirar PA y mover */
    printf("[2] Tirando PA y moviendo a casilla adyacente (1, 0)...\n");
    gm.currentActionPoints = 5;
    gm.hasRolledThisTurn = true;

    bool moved = GameMaster_AttemptMove(&gm, &map, &army, &map.tiles[1]);
    printf("    GameMaster_AttemptMove retorno: %s\n", moved ? "true" : "false");
    assert(moved);

    printf("    Nueva posicion del ejercito: (%d, %d)\n", army.position.q, army.position.r);
    assert(army.position.q == 1 && army.position.r == 0);

    printf(">>> PRUEBA A1 PASO AL 100%% [OK] <<<\n");

    free(map.tiles);
    return 0;
}
