#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "models/map_generator.h"
#include "models/balance_config.h"

static bool IsSettlement(StructureType type) {
    return (type == STRUCT_TOWN_CASTLE ||
            type == STRUCT_TOWN_MARKET ||
            type == STRUCT_TOWN_ARMORY ||
            type == STRUCT_TOWN_HOUSES);
}

int main(void) {
    printf("====================================================\n");
    printf("PRUEBA D1 & D2: GARANTIA DE ASENTAMIENTOS Y POSTES\n");
    printf("====================================================\n");

    /* 1. Inicializar balance */
    const BalanceConfig* cfg = Balance_GetGlobal();
    assert(cfg != NULL);
    printf("[1] Configuracion cargada: MaxDaysToSettlement=%.2f, HexPerDay=%d\n",
           cfg->maxDaysToSettlement, cfg->hexPerDay);
    
    int maxAllowedDist = (int)(cfg->maxDaysToSettlement * (float)cfg->hexPerDay + 0.5f);
    assert(maxAllowedDist == 18);
    printf("-> Distancia maxima por camino permitida a un asentamiento: %d hexes\n\n", maxAllowedDist);

    /* 2. Probar 1.000 mapas procedimentales */
    const int NUM_MAPS = 1000;
    int totalExitsTested = 0;
    int totalTilesOnRoutes = 0;
    int totalSignposts = 0;
    int maxObservedDistance = 0;
    int violations = 0;

    printf("[2] Generando y validando %d niveles procedimentales...\n", NUM_MAPS);

    for (int mapIdx = 0; mapIdx < NUM_MAPS; mapIdx++) {
        srand(1000 + mapIdx);
        Map map = Map_GenerateProcedural(15, 75, 3, 1000 + mapIdx);
        assert(map.tileCount > 0);

        /* Encontrar start */
        HexTile* start = NULL;
        for (int i = 0; i < map.tileCount; i++) {
            if (map.tiles[i].special == TILE_SPECIAL_START) {
                start = &map.tiles[i];
                break;
            }
        }
        assert(start != NULL && "El mapa debe tener casilla de inicio");

        /* Contar postes indicadores */
        int signpostsInMap = 0;
        for (int i = 0; i < map.tileCount; i++) {
            if (map.tiles[i].structure.type == STRUCT_SIGNPOST) {
                signpostsInMap++;
            }
        }
        totalSignposts += signpostsInMap;
        assert(signpostsInMap > 0 && "El mapa debe generar al menos un poste indicador (D2)");

        /* Multi-source BFS para calcular distancias por red vial a asentamientos */
        int* roadDist = (int*)malloc(map.tileCount * sizeof(int));
        HexTile** queue = (HexTile**)malloc(map.tileCount * sizeof(HexTile*));
        assert(roadDist && queue);

        for (int i = 0; i < map.tileCount; i++) roadDist[i] = -1;
        int qStart = 0, qEnd = 0;

        for (int i = 0; i < map.tileCount; i++) {
            HexTile* t = &map.tiles[i];
            if (IsSettlement(t->structure.type) || t->structure.settlementId > 0) {
                roadDist[i] = 0;
                queue[qEnd++] = t;
            }
        }

        while (qStart < qEnd) {
            HexTile* curr = queue[qStart++];
            int cDist = roadDist[(int)(curr - map.tiles)];
            for (int d = 0; d < 6; d++) {
                HexTile* nb = curr->neighbors[d];
                if (!nb) continue;
                if (!nb->isRoad && !IsSettlement(nb->structure.type)) continue;
                int nIdx = (int)(nb - map.tiles);
                if (roadDist[nIdx] == -1 || roadDist[nIdx] > cDist + 1) {
                    roadDist[nIdx] = cDist + 1;
                    queue[qEnd++] = nb;
                }
            }
        }

        /* Validar cada ruta desde Start a cada Exit */
        for (int i = 0; i < map.tileCount; i++) {
            HexTile* end = &map.tiles[i];
            if (end->special != TILE_SPECIAL_END) continue;
            totalExitsTested++;

            /* Encontrar ruta por caminos de start a end */
            HexTile** rQueue = (HexTile**)malloc(map.tileCount * sizeof(HexTile*));
            HexTile** prev = (HexTile**)calloc(map.tileCount, sizeof(HexTile*));
            bool* visited = (bool*)calloc(map.tileCount, sizeof(bool));
            assert(rQueue && prev && visited);

            int rqStart = 0, rqEnd = 0;
            visited[(int)(start - map.tiles)] = true;
            rQueue[rqEnd++] = start;
            bool reached = false;

            while (rqStart < rqEnd) {
                HexTile* curr = rQueue[rqStart++];
                if (curr == end) {
                    reached = true;
                    break;
                }
                for (int d = 0; d < 6; d++) {
                    HexTile* nb = curr->neighbors[d];
                    if (!nb || !nb->isRoad) continue;
                    int nIdx = (int)(nb - map.tiles);
                    if (!visited[nIdx]) {
                        visited[nIdx] = true;
                        prev[nIdx] = curr;
                        rQueue[rqEnd++] = nb;
                    }
                }
            }
            assert(reached && "Debe existir camino vial continuo entre Start y Exit");

            /* Verificar distancia a asentamiento de cada casilla en la ruta */
            HexTile* curr = end;
            while (curr != NULL) {
                totalTilesOnRoutes++;
                int cIdx = (int)(curr - map.tiles);
                int d = roadDist[cIdx];
                if (d < 0 || d > maxAllowedDist) {
                    violations++;
                    printf("  [FALLO] Mapa %d, Casilla (%d,%d), roadDist = %d > %d\n",
                           mapIdx, curr->coord.q, curr->coord.r, d, maxAllowedDist);
                }
                if (d > maxObservedDistance) {
                    maxObservedDistance = d;
                }
                if (curr == start) break;
                curr = prev[cIdx];
            }

            free(rQueue);
            free(prev);
            free(visited);
        }

        free(roadDist);
        free(queue);

        /* Validar mecanica de rumores de postes indicadores en el mapa 0 */
        if (mapIdx == 0) {
            /* Resetear visibilidad */
            for (int i = 0; i < map.tileCount; i++) {
                map.tiles[i].visible = false;
                map.tiles[i].visited = false;
                map.tiles[i].isRumor = false;
            }

            /* Encontrar un poste indicador */
            HexTile* signpost = NULL;
            for (int i = 0; i < map.tileCount; i++) {
                if (map.tiles[i].structure.type == STRUCT_SIGNPOST) {
                    signpost = &map.tiles[i];
                    break;
                }
            }
            assert(signpost != NULL);

            /* Actualizar vision sobre el poste */
            Map_UpdateVision(&map, signpost->coord, 2);
            assert(signpost->visible == true);

            /* Debe haberse generado al menos un rumor de asentamiento */
            int rumorCount = 0;
            HexTile* rumorTile = NULL;
            for (int i = 0; i < map.tileCount; i++) {
                if (map.tiles[i].isRumor) {
                    rumorCount++;
                    rumorTile = &map.tiles[i];
                }
            }
            assert(rumorCount > 0 && "Ver un poste indicador debe revelar un rumor (D2)");
            assert(rumorTile != NULL);
            assert(!rumorTile->visited && "El rumor no debe estar visitado todavia");
            printf("  [Mecanica D2 OK] Poste en (%d,%d) revelo rumor en (%d,%d)\n",
                   signpost->coord.q, signpost->coord.r, rumorTile->coord.q, rumorTile->coord.r);

            /* Ahora mover vision al asentamiento con rumor */
            Map_UpdateVision(&map, rumorTile->coord, 2);
            assert(rumorTile->visited == true);
            assert(rumorTile->isRumor == false && "Al descubrir el asentamiento, el rumor debe promoverse/limpiarse");
            printf("  [Mecanica D2 OK] Asentamiento visitado -> rumor promovido a descubierto exitosamente\n");
        }

        Map_Free(&map);

        if ((mapIdx + 1) % 200 == 0) {
            printf("  ... %d mapas evaluados (0 fallos hasta ahora)\n", mapIdx + 1);
        }
    }

    printf("\n====================================================\n");
    printf("RESULTADOS DE LA PRUEBA D1 & D2\n");
    printf("====================================================\n");
    printf("Niveles generados y evaluados: %d\n", NUM_MAPS);
    printf("Rutas A -> B probadas:         %d\n", totalExitsTested);
    printf("Casillas de ruta evaluadas:    %d\n", totalTilesOnRoutes);
    printf("Total postes generados:        %d (promedio %.1f/mapa)\n", totalSignposts, (float)totalSignposts / NUM_MAPS);
    printf("Distancia maxima observada:    %d hexes (limite <= %d)\n", maxObservedDistance, maxAllowedDist);
    printf("Violaciones de la garantia:    %d\n", violations);

    assert(violations == 0 && "La garantia de asentamientos (D1) debe cumplirse al 100%");
    printf(">>> PRUEBA D1 Y D2 PASO AL 100%% SIN FALLOS [OK] <<<\n");

    return 0;
}
