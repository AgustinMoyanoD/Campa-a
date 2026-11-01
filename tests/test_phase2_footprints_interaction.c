#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "models/map_generator.h"
#include "models/balance_config.h"
#include "models/city.h"
#include "controllers/game_master.h"

int main() {
    printf("============================================================\n");
    printf("PRUEBAS UNITARIAS: FASE 2 - HUELLAS, INTERACCION Y ASEDIO\n");
    printf("============================================================\n\n");

    // 0. Balance
    BalanceConfig bCfg;
    Balance_InitDefaults(&bCfg);
    Balance_LoadFromFile(&bCfg, "assets/balance.cfg");

    // 1. Generar Mapa
    srand(42);
    MapConfig mapCfg;
    MapConfig_InitDefault(&mapCfg, 1);
    Map map = Map_GenerateProcedural(mapCfg.radius, mapCfg.landProbabilityPercent, mapCfg.exitCount, mapCfg.provinceLevel);

    HexCoord bCoord = Map_GetBastionCoord(&map);
    HexTile* bTile = Map_GetBastionTile(&map);
    assert(bTile != NULL);

    printf("[TEST 1] Verificando roles de la huella del Bastion...\n");
    assert(bTile->footprintRole == FOOTPRINT_ROLE_ANCHOR);
    assert(bTile->structure.isBastion == true);
    assert(bTile->structure.settlementType == SETTLEMENT_BASTION);

    HexTile* wallTile = NULL;
    for (int d = 0; d < 6; d++) {
        HexTile* nb = bTile->neighbors[d];
        assert(nb != NULL);
        assert(nb->footprintRole == FOOTPRINT_ROLE_WALL || nb->footprintRole == FOOTPRINT_ROLE_GATE);
        assert(nb->footprintAnchor.q == bCoord.q && nb->footprintAnchor.r == bCoord.r);
        assert(nb->structure.isBastion == true);
        if (nb->footprintRole == FOOTPRINT_ROLE_WALL) wallTile = nb;
    }
    printf("  -> OK: Ancla y 6 murallas de huella 7 configuradas y referenciando la misma estructura.\n");

    // 2. Movimiento: Murallas son infranqueables
    printf("[TEST 2] Verificando que las murallas sean infranqueables en A* y GameMaster...\n");
    GameMaster gm;
    GameMaster_Init(&gm);
    Army playerArmy;
    ArmySetupConfig setupCfg;
    ArmySetup_InitDefault(&setupCfg);

    // Colocar jugador adyacente a una casilla de muralla
    HexTile* outsideTile = NULL;
    HexTile* stepFromTile = NULL;
    for (int d = 0; d < 6; d++) {
        HexTile* cand = wallTile->neighbors[d];
        if (cand && cand != bTile && cand->footprintRole != FOOTPRINT_ROLE_WALL && cand->terrain == TERRAIN_LAND) {
            outsideTile = cand;
            for (int d2 = 0; d2 < 6; d2++) {
                HexTile* c2 = outsideTile->neighbors[d2];
                if (c2 && c2 != wallTile && c2->footprintRole != FOOTPRINT_ROLE_WALL && c2->terrain == TERRAIN_LAND) {
                    stepFromTile = c2;
                    break;
                }
            }
            if (stepFromTile) break;
        }
    }
    assert(outsideTile != NULL);
    assert(stepFromTile != NULL);

    Army_Init(&playerArmy, &setupCfg, stepFromTile->coord);
    gm.currentActionPoints = 20;

    // Moverse hacia outsideTile (adyacente a la muralla)
    bool stepped = GameMaster_AttemptMove(&gm, &map, &playerArmy, outsideTile);
    assert(stepped);
    assert(playerArmy.position.q == outsideTile->coord.q && playerArmy.position.r == outsideTile->coord.r);

    // Intentar mover hacia la muralla: GameMaster_AttemptMove debe fallar
    bool moved = GameMaster_AttemptMove(&gm, &map, &playerArmy, wallTile);
    assert(!moved);
    assert(playerArmy.position.q == outsideTile->coord.q && playerArmy.position.r == outsideTile->coord.r);
    printf("  -> OK: GameMaster_AttemptMove rechazo el paso hacia la muralla.\n");

    // Búsqueda de caminos (A*): camino hacia la muralla debe ser de longitud 0
    HexTile* path[128];
    int cost = 0;
    int pathLen = Map_FindPath(&map, outsideTile, wallTile, &cost, path);
    assert(pathLen == 0);
    printf("  -> OK: Map_FindPath no trazo ninguna ruta hacia la muralla.\n");

    // 3. Interacción con la huella
    printf("[TEST 3] Verificando interaccion con la huella del Bastion...\n");
    // Al estar en outsideTile (adyacente a la muralla del bastión)
    assert(gm.canInteractStructure == true);
    assert(gm.interactStructureCoord.q == bCoord.q && gm.interactStructureCoord.r == bCoord.r);
    printf("  -> OK: interactStructureCoord apunta al ancla central del Bastion (%d, %d).\n",
           gm.interactStructureCoord.q, gm.interactStructureCoord.r);

    // 4. Menú de la Ciudad en el Bastión (solo asedio, sin servicios)
    printf("[TEST 4] Verificando menu de ciudad para SETTLEMENT_BASTION...\n");
    GameMaster_OpenCity(&gm, &map, &playerArmy);
    assert(gm.phase == PHASE_CITY);
    assert(strcmp(gm.activeCity.name, "Gran Bastion Central") == 0);
    assert(gm.activeCity.stockFood == 0);
    assert(gm.activeCity.stockWater == 0);
    assert(gm.activeCity.stockWine == 0);
    assert(gm.activeCity.stockMeat == 0);
    assert(!City_HasMarket(&gm.activeCity));
    assert(!City_HasArmory(&gm.activeCity));
    assert(!City_HasHospital(&gm.activeCity));
    assert(!City_HasTemple(&gm.activeCity));
    assert(!City_HasInn(&gm.activeCity));
    assert(City_CanSiege(&gm.activeCity) == true);
    printf("  -> OK: Menu del Bastion no ofrece servicios comerciales/civiles y permite asedio.\n");

    // 5. Asedio del Bastión: Guarnición calculada en 375 HP
    printf("[TEST 5] Probando inicio de asedio del Bastion y vida de guarnicion...\n");
    GameMaster_StartSiege(&gm, &map, &playerArmy, gm.interactStructureCoord);
    assert(gm.inSiegeCombat == true);
    assert(gm.inCombat == true);
    // 150 base * 2.0 bastión * 1.25 muralla = 375 HP
    assert((int)gm.activeCombat.enemyHealth == 375);
    assert((int)gm.activeCombat.enemyInitialHealth == 375);
    printf("  -> OK: Guarnicion del Bastion calculada exactamente en %.0f HP (150 * 2.0 * 1.25).\n",
           gm.activeCombat.enemyHealth);

    // 6. Victoria de Asedio: toda la huella queda marcada como asediada
    printf("[TEST 6] Probando resolucion de victoria de asedio sobre la huella completa...\n");
    GameMaster_ResolveSiegeVictory(&gm, &map, &playerArmy, gm.interactStructureCoord);
    assert(bTile->structure.isBesieged == true);
    for (int d = 0; d < 6; d++) {
        HexTile* nb = bTile->neighbors[d];
        assert(nb->structure.isBesieged == true);
    }
    printf("  -> OK: Las 7 casillas de la huella del Bastion quedaron marcadas con isBesieged = true.\n");

    Map_Free(&map);

    printf("\n============================================================\n");
    printf("¡TODAS LAS PRUEBAS DE LA FASE 2 PASARON CON EXITO!\n");
    printf("============================================================\n");
    return 0;
}
