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

int main(void) {
    printf("====================================================\n");
    printf("PRUEBA A3: VERIFICAR PRONOSTICO UNIFICADO (FORECAST)\n");
    printf("====================================================\n");

    BalanceConfig cfg;
    Balance_InitDefaults(&cfg);

    Map map;
    map.tileCount = 1;
    map.tiles = (HexTile*)calloc(1, sizeof(HexTile));
    map.tiles[0].coord = (HexCoord){0, 0};
    map.tiles[0].terrain = TERRAIN_LAND;
    map.tiles[0].biome = BIOME_PLAINS;

    General gen = General_Create("Comandante Test", 30, 2, 2, 2, 2, 2);
    /* 50 soldados -> consumo diario = 50 comida, 50 agua (12.5 por turno)
       Con 250 comida y 250 agua -> exactamente 5 dias completos */
    Army army = Army_Create((HexCoord){0, 0}, gen, 5, 25, 20, 250.0f, 250.0f, 100, 0);

    SupplyForecast forecast = Army_ForecastSupply(&army, &map, (HexCoord){0, 0});

    printf("Pronostico calculado:\n");
    printf("  campDays:       %.2f dias (Esperado: 5.00d)\n", forecast.campDays);
    printf("  marchDays:      %.2f dias\n", forecast.marchDays);
    printf("  nightMarchDays: %.2f dias\n", forecast.nightMarchDays);

    assert(fabsf(forecast.campDays - 5.0f) < 0.1f);

    /* Simulacion real de consumo franja por franja acampando */
    Army realArmy = army;
    int realSlots = 0;
    while (realArmy.comida_disponible > 0.0f && realArmy.agua_disponible > 0.0f && realSlots < 120) {
        Army_ConsumeTurnResources(&realArmy);
        realSlots++;
    }
    float realDays = (float)realSlots / 4.0f;

    printf("\nSimulacion real ejecutada: %.2f dias consumidos hasta agotamiento\n", realDays);
    printf("Diferencia (|forecast - real|): %.4f dias\n", fabsf(forecast.campDays - realDays));

    assert(fabsf(forecast.campDays - realDays) <= 0.1f);

    /* Prueba con suministros asimetricos: 100 comida (2 dias), 300 agua (6 dias) -> Minimo = 2 dias */
    army.comida_disponible = 100.0f;
    army.agua_disponible = 300.0f;
    SupplyForecast fcAsym = Army_ForecastSupply(&army, &map, (HexCoord){0, 0});
    printf("\nPrueba Asimetrica (Comida 100=2d, Agua 300=6d):\n");
    printf("  campDays: %.2f dias (Esperado: 2.00d por comida limitante)\n", fcAsym.campDays);
    assert(fabsf(fcAsym.campDays - 2.0f) < 0.1f);

    printf(">>> PRUEBA A3 PASO AL 100%% [OK] <<<\n");

    free(map.tiles);
    return 0;
}
