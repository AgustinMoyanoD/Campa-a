#ifndef MAP_GENERATOR_H
#define MAP_GENERATOR_H

#include "hex_tile.h"


typedef struct {

    HexTile* tiles;

    int tileCount;

} Map;


/*
    Genera un mapa hexagonal procedural.

    mapRadius:
        Radio del mapa.

    landProbabilityPercent:
        Probabilidad de que una celda sea tierra.
*/

Map Map_GenerateProcedural(
    int mapRadius,
    int landProbabilityPercent,
    int endCount
);


/*
    Libera la memoria utilizada por el mapa.
*/

void Map_Free(
    Map* map
);

#endif