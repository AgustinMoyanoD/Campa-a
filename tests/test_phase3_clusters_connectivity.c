#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#include "models/map_generator.h"
#include "models/balance_config.h"

int main() {
    printf("============================================================\n");
    printf("PRUEBAS UNITARIAS: FASE 3, 4, 5 - CLUSTERS, CONECTIVIDAD Y PUERTAS\n");
    printf("============================================================\n\n");

    BalanceConfig bCfg;
    Balance_InitDefaults(&bCfg);
    Balance_LoadFromFile(&bCfg, "assets/balance.cfg");

    int testSeeds = 1000;
    int primaryFootprint3Count = 0;
    int totalGatesCount = 0;

    printf("[TEST] Evaluando generacion en %d semillas...\n", testSeeds);

    for (int seed = 1; seed <= testSeeds; seed++) {
        srand(seed);

        MapConfig mapCfg;
        MapConfig_InitDefault(&mapCfg, 1);
        Map map = Map_GenerateProcedural(mapCfg.radius, mapCfg.landProbabilityPercent, mapCfg.exitCount, mapCfg.provinceLevel);

        assert(map.tiles != NULL);
        assert(map.tileCount > 0);

        // 1. Bastión y Puertas (Fase 5)
        HexCoord bCoord = Map_GetBastionCoord(&map);
        HexTile* bTile = Map_GetBastionTile(&map);
        assert(bTile != NULL);
        assert(bTile->structure.isBastion == true);

        int gateCount = 0;
        int wallCount = 0;
        for (int d = 0; d < 6; d++) {
            HexTile* nb = bTile->neighbors[d];
            assert(nb != NULL);
            if (nb->footprintRole == FOOTPRINT_ROLE_GATE) gateCount++;
            if (nb->footprintRole == FOOTPRINT_ROLE_WALL) wallCount++;
        }
        assert(gateCount >= 1 && gateCount <= 3);
        assert(wallCount + gateCount == 6);
        totalGatesCount += gateCount;

        // 2. Inicio
        HexCoord sCoord = Map_GetStartCoord(&map);
        HexTile* sTile = Map_GetTile(&map, sCoord);
        assert(sTile != NULL);
        int startDist = HexCoord_Distance(sCoord, bCoord);
        if (startDist < 31) {
            printf("[FAIL] Seed %d: startDist = %d < 31!\n", seed, startDist);
            assert(false);
        }

        // 3. Identificar los 3 clusters de asentamiento
        HexTile* clusterAnchors[10];
        int clusterCount = 0;
        HexTile* primaryCastleAnchor = NULL;

        for (int i = 0; i < map.tileCount; i++) {
            HexTile* t = &map.tiles[i];
            if (t->structure.type == STRUCT_TOWN_CASTLE && !t->structure.isBastion && t->footprintRole == FOOTPRINT_ROLE_ANCHOR) {
                clusterAnchors[clusterCount++] = t;
                if (t->structure.settlementType == SETTLEMENT_PRIMARY) {
                    primaryCastleAnchor = t;
                }
            }
        }

        assert(clusterCount == 3);
        assert(primaryCastleAnchor != NULL);

        // 4. Distancias mínimas de clusters
        for (int k = 0; k < 3; k++) {
            HexTile* cTile = clusterAnchors[k];
            int distB = HexCoord_Distance(cTile->coord, bCoord);
            int distS = HexCoord_Distance(cTile->coord, sCoord);
            assert(distB >= 20); // Mínimo garantizado con relajación
            assert(distS >= 14);

            for (int j = k + 1; j < 3; j++) {
                int distCC = HexCoord_Distance(cTile->coord, clusterAnchors[j]->coord);
                assert(distCC >= 16);
            }
        }

        // 5. Huellas dentro de la ciudad principal
        int primaryCastleTiles = 0;
        for (int i = 0; i < map.tileCount; i++) {
            HexTile* t = &map.tiles[i];
            if (t->structure.type == STRUCT_TOWN_CASTLE && t->structure.settlementType == SETTLEMENT_PRIMARY && !t->structure.isBastion) {
                primaryCastleTiles++;
            }
        }
        assert(primaryCastleTiles == 3 || primaryCastleTiles == 1);
        if (primaryCastleTiles == 3) primaryFootprint3Count++;

        // 6. Huellas no se solapan
        for (int i = 0; i < map.tileCount; i++) {
            HexTile* t = &map.tiles[i];
            if (t->footprintRole == FOOTPRINT_ROLE_BODY) {
                // Su ancla debe ser válida y tener el mismo tipo
                HexTile* anc = Map_GetTile(&map, t->footprintAnchor);
                assert(anc != NULL);
                assert(anc->footprintRole == FOOTPRINT_ROLE_ANCHOR);
                assert(anc->structure.type == t->structure.type);
            }
        }

        // 7. Conectividad Segura (Fase 4)
        // Debe existir al menos una ruta 100% segura (sin ríos sin puente, sin murallas)
        // desde start a Bastión y a cada cluster
        for (int k = 0; k < 3; k++) {
            if (!Map_HasSafePath(&map, sTile, clusterAnchors[k])) {
                printf("[FAIL] Seed %d: Cluster %d (pos %d, %d, type %d) has NO safe path from start (%d, %d)!\n",
                       seed, k, clusterAnchors[k]->coord.q, clusterAnchors[k]->coord.r,
                       clusterAnchors[k]->structure.settlementType, sCoord.q, sCoord.r);
                
                // Diagnostico: buscar qué bloquea el camino
                bool* vis = calloc(map.tileCount, sizeof(bool));
                HexTile** qu = malloc(sizeof(HexTile*) * map.tileCount);
                int qs = 0, qe = 0;
                vis[(int)(sTile - map.tiles)] = true;
                qu[qe++] = sTile;
                int reachableTiles = 0;
                int riverBlocks = 0;
                int wallBlocks = 0;
                while (qs < qe) {
                    HexTile* curr = qu[qs++];
                    reachableTiles++;
                    for (int d = 0; d < 6; d++) {
                        HexTile* n = curr->neighbors[d];
                        if (!n) continue;
                        if (n->terrain == TERRAIN_WATER && n->bridge == BRIDGE_NONE) continue;
                        if (n->footprintRole == FOOTPRINT_ROLE_WALL) { wallBlocks++; continue; }
                        if (n->river == RIVER_PRESENT && n->bridge == BRIDGE_NONE) { riverBlocks++; continue; }
                        int nIdx = (int)(n - map.tiles);
                        if (!vis[nIdx]) {
                            vis[nIdx] = true;
                            qu[qe++] = n;
                        }
                    }
                }
                printf("       Reachable safe tiles: %d / %d. Blocked by rivers: %d, by walls: %d\n",
                       reachableTiles, map.tileCount, riverBlocks, wallBlocks);
                free(vis);
                free(qu);
                assert(false);
            }
        }
        assert(Map_HasSafePath(&map, sTile, bTile) == true);

        // 8. Gradiente de Dificultad (Fase 7)
        float diffBastion = Map_DifficultyAt(&map, bCoord);
        float diffStart = Map_DifficultyAt(&map, sCoord);
        assert(fabsf(diffBastion - bCfg.difficultyBastion) < 0.01f);
        assert(diffBastion > diffStart);
        assert(fabsf(diffStart - bCfg.difficultyBorder) < 0.05f);

        Map_Free(&map);
    }

    printf("[OK] Las %d semillas pasaron todas las verificaciones.\n", testSeeds);
    printf("     - Promedio de puertas por Bastión: %.2f (rango [1, 3])\n", (float)totalGatesCount / (float)testSeeds);
    printf("     - Tasa de castillo primario huella 3 completa: %.1f%%\n", (float)primaryFootprint3Count * 100.0f / (float)testSeeds);
    printf("     - 100%% de semillas tienen conectividad completamente segura.\n");
    printf("     - 100%% de clusters respetan separación espacial y sectores.\n\n");

    return 0;
}
