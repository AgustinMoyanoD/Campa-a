#ifndef MAP_GENERATOR_H
#define MAP_GENERATOR_H

#include "hex_tile.h"


typedef struct Map {

    HexTile* tiles;

    int tileCount;

    HexCoord startCoord;

    HexCoord bastionCoord;

} Map;

typedef struct {
    int radius;
    int landProbabilityPercent;
    int exitCount; /* Deprecado: la provincia ahora tiene bastión central */
    int provinceLevel;
} MapConfig;

void MapConfig_InitDefault(MapConfig* cfg, int provinceLevel);
void Map_Init(Map* map, const MapConfig* cfg);
HexCoord Map_GetStartCoord(const Map* map);
HexCoord Map_GetBastionCoord(const Map* map);
HexTile* Map_GetBastionTile(const Map* map);
float Map_DifficultyAt(const Map* map, HexCoord coord);
bool Map_HasSafePath(const Map* map, const HexTile* from, const HexTile* to);


/*
    Genera un mapa hexagonal procedural.

    mapRadius:
        Radio del mapa.

    landProbabilityPercent:
        Probabilidad de que una celda sea tierra.

    endCount:
        Cantidad de salidas de la provincia.

    provinceLevel:
        Nivel de la provincia (1 a 4) que define el pool de biomas y dificultad.
*/

Map Map_GenerateProcedural(
    int mapRadius,
    int landProbabilityPercent,
    int endCount,
    int provinceLevel
);


/*
    Libera la memoria utilizada por el mapa.
*/

void Map_Free(
    Map* map
);

HexTile* Map_GetTile(const Map* map, HexCoord coord);
void Map_UpdateVision(Map* map, HexCoord center, int visionRange);

int Map_FindPath(Map* map, HexTile* start, HexTile* end, int* outTotalCost, HexTile** pathOut);
int Map_FindPathEnemy(Map* map, HexTile* start, HexTile* end, int* outTotalCost, HexTile** pathOut);
int Map_FindPathEx(Map* map, HexTile* start, HexTile* end, bool ignoreFog, int* outTotalCost, HexTile** pathOut);

/* Bloque D: Descubrimiento de asentamientos */
void Map_EnsureSettlementGuarantee(Map* map, HexTile* start, int provinceLevel);
void Map_GenerateSignposts(Map* map);

#endif


