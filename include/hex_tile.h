#ifndef HEX_TILE_H
#define HEX_TILE_H

#include <stdbool.h>

typedef struct {
    int q;
    int r;
} HexCoord;


typedef enum {
    HEX_RIGHT = 0,
    HEX_DOWN_RIGHT,
    HEX_DOWN_LEFT,
    HEX_LEFT,
    HEX_UP_LEFT,
    HEX_UP_RIGHT
} HexDirection;


typedef enum {
    TERRAIN_LAND,
    TERRAIN_WATER
} TerrainType;


typedef enum {
    TILE_SPECIAL_NONE,
    TILE_SPECIAL_START,
    TILE_SPECIAL_END
} TileSpecial;


typedef enum {
    RIVER_NONE,
    RIVER_PRESENT
} RiverType;


typedef enum {
    BRIDGE_NONE,
    BRIDGE_ABANDONED,
    BRIDGE_OCCUPIED
} BridgeType;


typedef struct HexTile {

    HexCoord coord;

    int nivel;

    TerrainType terrain;

    bool isRoad;

    TileSpecial special;

    RiverType river;

    BridgeType bridge;

    struct HexTile* neighbors[6];

} HexTile;


HexCoord HexCoord_Create(
    int q,
    int r
);


HexCoord HexCoord_GetNeighbor(
    HexCoord coord,
    HexDirection direction
);


HexTile HexTile_Create(
    HexCoord coord,
    int nivel,
    TerrainType terrain
);


void HexTile_Connect(
    HexTile* tileA,
    HexDirection direction,
    HexTile* tileB
);

#endif