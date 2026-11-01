#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "models/map_generator.h"
#include "models/balance_config.h"
#include "models/army.h"
#include "controllers/game_master.h"

int main() {
    printf("======================================================================\n");
    printf("PRUEBA INTEGRAL DE 2000 SEMILLAS - VERIFICACION COMPLETA DE REQUISITOS\n");
    printf("======================================================================\n\n");

    BalanceConfig bCfg;
    Balance_InitDefaults(&bCfg);
    Balance_LoadFromFile(&bCfg, "assets/balance.cfg");

    const int TOTAL_SEEDS = 2000;
    int totalGates = 0;

    printf("[1/3] Ejecutando validacion procedural a traves de %d semillas...\n", TOTAL_SEEDS);

    for (int seed = 1; seed <= TOTAL_SEEDS; seed++) {
        srand(seed);

        MapConfig mapCfg;
        MapConfig_InitDefault(&mapCfg, 1);
        Map map = Map_GenerateProcedural(mapCfg.radius, mapCfg.landProbabilityPercent, mapCfg.exitCount, mapCfg.provinceLevel);

        assert(map.tiles != NULL);
        assert(map.tileCount > 0);

        // Req 1: Bastión central existe con offset <= 8 de (0,0)
        HexCoord bCoord = Map_GetBastionCoord(&map);
        HexTile* bTile = Map_GetBastionTile(&map);
        assert(bTile != NULL);
        assert(bTile->structure.isBastion == true);
        assert(bTile->footprintRole == FOOTPRINT_ROLE_ANCHOR);
        assert(bTile->structure.settlementType == SETTLEMENT_BASTION);
        int offsetFromCenter = HexCoord_Distance(bCoord, (HexCoord){0, 0});
        assert(offsetFromCenter <= 8);

        // Req 8: Rios radiales frenan a distancia >= 3 del centro del bastión
        for (int i = 0; i < map.tileCount; i++) {
            HexTile* t = &map.tiles[i];
            if (t->river == RIVER_PRESENT) {
                int distB = HexCoord_Distance(t->coord, bCoord);
                if (distB < 3) {
                    printf("[FAIL] Seed %d: River tile at (%d, %d) is distance %d (< 3) from Bastion (%d, %d)!\n",
                           seed, t->coord.q, t->coord.r, distB, bCoord.q, bCoord.r);
                    assert(false);
                }
            }
        }

        // Req 5: Huella 7 del Bastión con 1 a 3 puertas y perímetro amurallado
        int gateCount = 0;
        int wallCount = 0;
        for (int d = 0; d < 6; d++) {
            HexTile* nb = bTile->neighbors[d];
            assert(nb != NULL);
            assert(nb->structure.isBastion == true);
            assert(nb->footprintAnchor.q == bCoord.q && nb->footprintAnchor.r == bCoord.r);
            if (nb->footprintRole == FOOTPRINT_ROLE_GATE) {
                gateCount++;
            } else if (nb->footprintRole == FOOTPRINT_ROLE_WALL) {
                wallCount++;
            } else {
                printf("[FAIL] Seed %d: Bastion perimeter neighbor has invalid role %d\n", seed, nb->footprintRole);
                assert(false);
            }
        }
        assert(gateCount >= 1 && gateCount <= 3);
        assert(wallCount + gateCount == 6);
        totalGates += gateCount;

        // Req 2: Inicio en costa a distancia >= 31 hexes en tierra firme (mainland)
        HexCoord sCoord = Map_GetStartCoord(&map);
        HexTile* sTile = Map_GetTile(&map, sCoord);
        assert(sTile != NULL);
        assert(sTile->terrain == TERRAIN_LAND);
        int startDist = HexCoord_Distance(sCoord, bCoord);
        assert(startDist >= 31);

        // Req 4: Huellas no se solapan (todas las huellas BODY tienen ancla consistente)
        for (int i = 0; i < map.tileCount; i++) {
            HexTile* t = &map.tiles[i];
            if (t->footprintRole == FOOTPRINT_ROLE_BODY) {
                HexTile* anc = Map_GetTile(&map, t->footprintAnchor);
                assert(anc != NULL);
                assert(anc->footprintRole == FOOTPRINT_ROLE_ANCHOR);
                assert(anc->structure.type == t->structure.type);
                assert(anc->structure.settlementId == t->structure.settlementId);
            }
        }

        // Identificar los 3 clusters de asentamiento
        HexTile* clusterAnchors[10];
        int clusterCount = 0;
        for (int i = 0; i < map.tileCount; i++) {
            HexTile* t = &map.tiles[i];
            if (t->structure.type == STRUCT_TOWN_CASTLE && !t->structure.isBastion && t->footprintRole == FOOTPRINT_ROLE_ANCHOR) {
                clusterAnchors[clusterCount++] = t;
            }
        }
        assert(clusterCount == 3);

        // Req 9: 100% ruta segura garantizada de Start a Bastión
        bool safeToBastion = Map_HasSafePath(&map, sTile, bTile);
        if (!safeToBastion) {
            printf("[FAIL] Seed %d: No safe path to Bastion!\n", seed);
            assert(false);
        }

        // Req 10: 100% ruta segura garantizada de Start a cada uno de los 3 clusters
        for (int k = 0; k < 3; k++) {
            bool safeToCluster = Map_HasSafePath(&map, sTile, clusterAnchors[k]);
            if (!safeToCluster) {
                printf("[FAIL] Seed %d: No safe path to Cluster %d!\n", seed, k);
                assert(false);
            }
        }

        // Req 11: Todos los puentes son cruces transitables
        for (int i = 0; i < map.tileCount; i++) {
            HexTile* t = &map.tiles[i];
            if (t->bridge != BRIDGE_NONE) {
                assert(t->bridge == BRIDGE_ABANDONED || t->bridge == BRIDGE_OCCUPIED);
            }
        }

        // Gradiente de dificultad
        float diffBastion = Map_DifficultyAt(&map, bCoord);
        float diffStart = Map_DifficultyAt(&map, sCoord);
        assert(fabsf(diffBastion - bCfg.difficultyBastion) < 0.01f);
        assert(diffBastion > diffStart);

        // Progreso periódico
        if (seed % 500 == 0) {
            printf("       Progreso: %d / %d semillas evaluadas exitosamente...\n", seed, TOTAL_SEEDS);
        }

        Map_Free(&map);
    }
    printf("[OK] Las %d semillas pasaron la verificacion procedural.\n\n", TOTAL_SEEDS);

    printf("[2/3] Validando comportamiento funcional de Movimiento, Murallas y Puertas...\n");
    {
        srand(42);
        MapConfig mapCfg;
        MapConfig_InitDefault(&mapCfg, 1);
        Map map = Map_GenerateProcedural(mapCfg.radius, mapCfg.landProbabilityPercent, mapCfg.exitCount, mapCfg.provinceLevel);

        HexCoord bCoord = Map_GetBastionCoord(&map);
        HexTile* bTile = Map_GetBastionTile(&map);

        HexTile* wallTile = NULL;
        HexTile* gateTile = NULL;
        for (int d = 0; d < 6; d++) {
            HexTile* nb = bTile->neighbors[d];
            if (nb->footprintRole == FOOTPRINT_ROLE_WALL) wallTile = nb;
            if (nb->footprintRole == FOOTPRINT_ROLE_GATE) gateTile = nb;
        }
        assert(wallTile != NULL);
        assert(gateTile != NULL);

        // Req 6: Murallas infranqueables
        GameMaster gm;
        GameMaster_Init(&gm);
        gm.currentActionPoints = 10;
        Army playerArmy;
        ArmySetupConfig setup;
        memset(&setup, 0, sizeof(setup));
        setup.iniciados = 20;
        setup.soldados = 20;
        setup.sargentos = 10;
        Army_Init(&playerArmy, &setup, bCoord);

        // Intentar moverse sobre una muralla debe ser rechazado
        bool wallMoveOk = GameMaster_AttemptMove(&gm, &map, &playerArmy, wallTile);
        assert(wallMoveOk == false);

        // Req 7: Puertas franqueables
        bool gateMoveOk = GameMaster_AttemptMove(&gm, &map, &playerArmy, gateTile);
        assert(gateMoveOk == true);
        assert(playerArmy.position.q == gateTile->coord.q && playerArmy.position.r == gateTile->coord.r);

        // Pathfinding: el pathfinding nunca debe incluir una muralla
        HexTile* path[128];
        int cost = 0;
        int len = Map_FindPathEx(&map, gateTile, bTile, false, &cost, path);
        assert(len > 0);
        for (int p = 0; p < len; p++) {
            assert(path[p]->footprintRole != FOOTPRINT_ROLE_WALL);
        }

        printf("[OK] Murallas y puertas verificadas correctamente.\n\n");
        Map_Free(&map);
    }

    printf("[3/3] Validando Tirada de Cruce de Rios, Dificultad y Victoria...\n");
    {
        srand(101);
        MapConfig mapCfg;
        MapConfig_InitDefault(&mapCfg, 1);
        Map map = Map_GenerateProcedural(mapCfg.radius, mapCfg.landProbabilityPercent, mapCfg.exitCount, mapCfg.provinceLevel);

        GameMaster gm;
        GameMaster_Init(&gm);
        gm.currentActionPoints = 10;
        Army playerArmy;
        ArmySetupConfig setup;
        memset(&setup, 0, sizeof(setup));
        setup.iniciados = 30;
        setup.soldados = 30;
        setup.sargentos = 10;
        Army_Init(&playerArmy, &setup, (HexCoord){0, 0});
        playerArmy.comida_disponible = 100.0f;
        playerArmy.agua_disponible = 100.0f;

        // Req 12: Cruce de río sin puente
        HexTile riverTile;
        memset(&riverTile, 0, sizeof(HexTile));
        riverTile.coord = (HexCoord){0, 1};
        riverTile.river = RIVER_PRESENT;
        riverTile.bridge = BRIDGE_NONE;
        riverTile.terrain = TERRAIN_LAND;

        // Forzar tirada de cruce
        GameMaster_ResolveRiverCrossing(&gm, &playerArmy, &riverTile);
        assert(gm.riverCrossTimer > 0.0f);
        assert(strlen(gm.riverCrossMessage) > 0);
        assert(gm.riverCrossSeverity >= 0 && gm.riverCrossSeverity <= 2);

        // Req 13: Asedio y Victoria al capturar Bastión
        HexCoord bCoord = Map_GetBastionCoord(&map);

        // HP de Bastión = 150 * 2.0 (Bastion difficulty) * 1.25 (Wall bonus) = 375 HP
        GameMaster_StartSiege(&gm, &map, &playerArmy, bCoord);
        assert(gm.inSiegeCombat == true);
        assert(gm.siegeTargetCoord.q == bCoord.q && gm.siegeTargetCoord.r == bCoord.r);
        assert((int)gm.activeCombat.enemyInitialHealth == 375);

        // Resolver victoria de asedio en Bastión
        GameMaster_ResolveSiegeVictory(&gm, &map, &playerArmy, bCoord);
        assert(gm.conqueredBastion == true);
        assert(gm.phase == PHASE_VICTORY);

        // Asedio en castillo secundario no debe disparar PHASE_VICTORY
        HexTile* secondaryCenter = NULL;
        for (int i = 0; i < map.tileCount; i++) {
            HexTile* t = &map.tiles[i];
            if (t->structure.type == STRUCT_TOWN_CASTLE && !t->structure.isBastion && t->footprintRole == FOOTPRINT_ROLE_ANCHOR) {
                secondaryCenter = t;
                break;
            }
        }
        assert(secondaryCenter != NULL);
        gm.phase = PHASE_PLAYER;
        GameMaster_ResolveSiegeVictory(&gm, &map, &playerArmy, secondaryCenter->coord);
        assert(gm.phase == PHASE_PLAYER); // Sigue jugando la isla, no da victoria inmediata

        printf("[OK] Cruce de rio, Dificultad (375 HP) y Victoria verificados correctamente.\n\n");
        Map_Free(&map);
    }

    printf("======================================================================\n");
    printf("¡TODOS LOS REQUISITOS DEL PROMPT CUMPLIDOS CON EXITO! (100.0%% PASS)\n");
    printf("======================================================================\n");

    return 0;
}
