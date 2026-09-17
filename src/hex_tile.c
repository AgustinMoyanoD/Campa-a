#include "hex_tile.h"

#include <stddef.h>


/*
    Direcciones de un hexágono con coordenadas axiales.

    RIGHT  = { 1,  0}
    DOWN_RIGHT = { 0,  1}
    DOWN_LEFT  = {-1,  1}
    LEFT       = {-1,  0}
    UP_LEFT    = { 0, -1}
    UP_RIGHT   = { 1, -1}
*/

static const HexCoord HEX_DIRECTIONS[6] = {
    { 1,  0},
    { 0,  1},
    {-1,  1},
    {-1,  0},
    { 0, -1},
    { 1, -1}
};


HexCoord HexCoord_Create(
    int q,
    int r
)
{
    HexCoord coord;

    coord.q = q;
    coord.r = r;

    return coord;
}


HexCoord HexCoord_GetNeighbor(
    HexCoord coord,
    HexDirection direction
)
{
    coord.q += HEX_DIRECTIONS[direction].q;
    coord.r += HEX_DIRECTIONS[direction].r;

    return coord;
}


HexTile HexTile_Create(
    HexCoord coord,
    int nivel,
    TerrainType terrain
)
{
    HexTile tile;

    tile.coord = coord;

    tile.nivel = nivel;

    tile.terrain = terrain;

    tile.isRoad = false;

    tile.special = TILE_SPECIAL_NONE;

    tile.river = RIVER_NONE;

    tile.bridge = BRIDGE_NONE;


    for (int i = 0; i < 6; i++)
    {
        tile.neighbors[i] = NULL;
    }

    return tile;
}


void HexTile_Connect(
    HexTile* tileA,
    HexDirection direction,
    HexTile* tileB
)
{
    if (tileA == NULL || tileB == NULL)
        return;


    tileA->neighbors[direction] = tileB;


    /*
        Las direcciones opuestas están
        separadas por 3 posiciones.
    */

    HexDirection opposite =
        (direction + 3) % 6;


    tileB->neighbors[opposite] = tileA;
}