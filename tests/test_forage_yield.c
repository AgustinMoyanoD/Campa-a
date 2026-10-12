#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#include "models/army.h"
#include "models/general.h"
#include "models/hex_tile.h"
#include "models/map_generator.h"
#include "models/balance_config.h"
#include "controllers/game_master.h"
#include "models/day_clock.h"

int main(void) {
    printf("====================================================\n");
    printf("PRUEBA A2: VERIFICAR RENDIMIENTO NETO DEL FORRAJEO\n");
    printf("====================================================\n");

    BalanceConfig cfg;
    Balance_InitDefaults(&cfg);

    Map map;
    map.tileCount = 3;
    map.tiles = (HexTile*)calloc(map.tileCount, sizeof(HexTile));
    map.tiles[0].coord = (HexCoord){0, 0};
    map.tiles[0].terrain = TERRAIN_LAND;
    map.tiles[0].biome = BIOME_PLAINS;

    map.tiles[1].coord = (HexCoord){1, 0};
    map.tiles[1].terrain = TERRAIN_LAND;
    map.tiles[1].biome = BIOME_FOREST;

    General gen = General_Create("Comandante Test", 30, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 0}, gen, 5, 20, 10, 500.0f, 500.0f, 100, 0);

    int totalSoldiers = Army_GetTotalSoldiers(&army);
    float dailyFood = Army_CalculateFoodUsage(totalSoldiers, army.ganado);
    float dailyWater = Army_CalculateWaterUsage(totalSoldiers, army.ganado);

    printf("Ejercito: %d soldados. Consumo diario: %.1f com / %.1f agua\n\n", totalSoldiers, dailyFood, dailyWater);

    /* TEST LLANURA (PLAINS): Recurso objetivo = COMIDA */
    printf("[1] Forrajeo en Llanura desde la Manana (Morning -> Afternoon = -0.5 dias de consumo)...\n");
    GameMaster gm;
    GameMaster_Init(&gm);
    gm.clock.timeOfDay = TIME_MORNING;
    gm.timeOfDay = TIME_MORNING;

    float foodBefore = army.comida_disponible;
    float waterBefore = army.agua_disponible;

    /* Forzar semilla para tirada normal (7) */
    gm.rng.state = 12345678ULL;

    bool res = GameMaster_Forage(&gm, &army, &map);
    assert(res);

    float netFoodDays = (army.comida_disponible - foodBefore) / dailyFood;
    float netWaterDays = (army.agua_disponible - waterBefore) / dailyWater;

    printf("    Comida neta: %+.2f dias (Esperado >= +0.25d)\n", netFoodDays);
    printf("    Agua neta:   %+.2f dias\n", netWaterDays);
    assert(netFoodDays >= 0.25f);
    assert(gm.timeOfDay == TIME_AFTERNOON);
    printf("-> [OK] Llanura rindio neto positivo en comida\n\n");

    /* TEST BOSQUE (FOREST): Recurso objetivo = COMIDA */
    printf("[2] Forrajeo en Bosque desde la Manana...\n");
    army.position = (HexCoord){1, 0};
    GameMaster gm2;
    GameMaster_Init(&gm2);
    gm2.clock.timeOfDay = TIME_MORNING;
    gm2.timeOfDay = TIME_MORNING;

    foodBefore = army.comida_disponible;
    waterBefore = army.agua_disponible;

    res = GameMaster_Forage(&gm2, &army, &map);
    assert(res);

    netFoodDays = (army.comida_disponible - foodBefore) / dailyFood;
    netWaterDays = (army.agua_disponible - waterBefore) / dailyWater;

    printf("    Comida neta: %+.2f dias (Esperado >= +0.25d)\n", netFoodDays);
    printf("    Agua neta:   %+.2f dias\n", netWaterDays);
    assert(netFoodDays >= 0.25f);
    printf("-> [OK] Bosque rindio neto positivo en comida\n\n");

    printf(">>> PRUEBA A2 PASO AL 100%% [OK] <<<\n");

    free(map.tiles);
    return 0;
}
