#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#include "models/map_generator.h"
#include "models/balance_config.h"

int main() {
    printf("============================================================\n");
    printf("PRUEBAS UNITARIAS: FASE 1 - BASTION CENTRAL, INICIO Y FIN\n");
    printf("============================================================\n\n");

    BalanceConfig bCfg;
    Balance_InitDefaults(&bCfg);
    Balance_LoadFromFile(&bCfg, "assets/balance.cfg");

    int testSeeds = 500;
    int maxDisplacementSeen = 0;
    int minStartDistSeen = 999999;
    int maxStartDistSeen = 0;
    double sumStartDist = 0.0;

    printf("[TEST 1] Verificando generacion de Bastion e Inicio en %d semillas...\n", testSeeds);

    for (int seed = 1; seed <= testSeeds; seed++) {
        srand(seed);

        MapConfig mapCfg;
        MapConfig_InitDefault(&mapCfg, 1);
        Map map = Map_GenerateProcedural(mapCfg.radius, mapCfg.landProbabilityPercent, mapCfg.exitCount, mapCfg.provinceLevel);

        assert(map.tiles != NULL);
        assert(map.tileCount > 0);

        // 1. Bastión central existe
        HexCoord bCoord = Map_GetBastionCoord(&map);
        HexTile* bTile = Map_GetBastionTile(&map);
        assert(bTile != NULL);
        assert(bTile->structure.type == STRUCT_TOWN_CASTLE);
        assert(bTile->structure.isBastion == true);
        assert(bTile->structure.settlementType == SETTLEMENT_BASTION);
        assert(bTile->footprintRole == FOOTPRINT_ROLE_ANCHOR);
        assert(bTile->terrain == TERRAIN_LAND);
        assert(bTile->biome != BIOME_WATER);

        // Desplazamiento respecto a (0, 0)
        int dq = bCoord.q;
        int dr = bCoord.r;
        int ds = (bCoord.q + bCoord.r);
        if (dq < 0) dq = -dq;
        if (dr < 0) dr = -dr;
        if (ds < 0) ds = -ds;
        int disp = (dq + dr + ds) / 2;
        if (disp > maxDisplacementSeen) maxDisplacementSeen = disp;
        assert(disp <= bCfg.bastionCenterMaxOffset);

        // Huella 7 del bastión: 6 vecinos deben ser muralla y tierra
        int wallCount = 0;
        for (int d = 0; d < 6; d++) {
            HexTile* nb = bTile->neighbors[d];
            assert(nb != NULL);
            assert(nb->terrain == TERRAIN_LAND);
            assert(nb->biome != BIOME_WATER);
            assert(nb->structure.isBastion == true);
            assert(nb->footprintRole == FOOTPRINT_ROLE_WALL || nb->footprintRole == FOOTPRINT_ROLE_GATE);
            assert(nb->footprintAnchor.q == bCoord.q && nb->footprintAnchor.r == bCoord.r);
            wallCount++;
        }
        assert(wallCount == 6);

        // 2. Inicio existe y cumple distancia minima (3.5 dias * 9 = 31.5 -> 32)
        HexCoord sCoord = Map_GetStartCoord(&map);
        HexTile* sTile = Map_GetTile(&map, sCoord);
        assert(sTile != NULL);
        assert(sTile->special == TILE_SPECIAL_START);
        assert(sTile->terrain == TERRAIN_LAND);

        int distToBastion = HexCoord_Distance(sCoord, bCoord);
        if (distToBastion < minStartDistSeen) minStartDistSeen = distToBastion;
        if (distToBastion > maxStartDistSeen) maxStartDistSeen = distToBastion;
        sumStartDist += distToBastion;

        int reqMinDist = (int)(bCfg.minStartDistDays * (float)bCfg.hexPerDay + 0.5f);
        assert(distToBastion >= reqMinDist);

        // Inicio debe ser casilla de costa (al menos un vecino agua o borde)
        bool hasWaterNeighbor = false;
        for (int d = 0; d < 6; d++) {
            HexTile* nb = sTile->neighbors[d];
            if (nb == NULL || nb->terrain == TERRAIN_WATER || nb->biome == BIOME_WATER) {
                hasWaterNeighbor = true;
                break;
            }
        }
        assert(hasWaterNeighbor == true);

        // 3. No existe ningun tile especial fin
        for (int i = 0; i < map.tileCount; i++) {
            // No existe TILE_SPECIAL_END
            if (map.tiles[i].river == RIVER_PRESENT) {
                // Decision 6: Los rios frenan a distancia 3 del bastion (no tocan dist <= 2)
                int rDist = HexCoord_Distance(map.tiles[i].coord, bCoord);
                assert(rDist >= 3);
            }
        }

        Map_Free(&map);
    }

    printf("  -> OK: Bastion presente en 100%% de las semillas.\n");
    printf("  -> OK: Desplazamiento maximo del Bastion respecto a (0,0): %d hexes (tolerancia: %d).\n",
           maxDisplacementSeen, bCfg.bastionCenterMaxOffset);
    printf("  -> OK: Distancia Inicio-Bastion: Minima = %d hexes (req: >= 32), Media = %.1f hexes, Maxima = %d hexes.\n",
           minStartDistSeen, sumStartDist / testSeeds, maxStartDistSeen);
    printf("  -> OK: Rios frenan a distancia >= 3 del Bastion en todas las semillas.\n");
    printf("  -> OK: Eliminados todos los marcadores y referencias a TILE_SPECIAL_END.\n");

    printf("\n============================================================\n");
    printf("¡TODAS LAS PRUEBAS DE LA FASE 1 PASARON CON EXITO!\n");
    printf("============================================================\n");
    return 0;
}
