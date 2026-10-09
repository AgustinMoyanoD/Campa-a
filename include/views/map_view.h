#ifndef MAP_VIEW_H
#define MAP_VIEW_H

#include "raylib.h"
#include "map_generator.h"
#include "asset_manager.h"

Vector2 HexToPixel(HexCoord coord, float radius);
HexCoord PixelToHex(Vector2 position, float radius);

void MapView_Draw(
    const Map*       map,
    float            radius,
    Vector2          origin,
    bool             drawConnections,
    HexCoord viewCenter,
    int              renderRadius,
    Vector2         armyPos,
    const GameAssets* assets,
    bool debugRevealMap
);

float GetBiomeHeight(BiomeType biome);
void MapView_DrawHexHighlight(
    const HexTile* tile,
    float          radius,
    Vector2        origin,
    Color          color
);

void MapView_DrawHexFillHighlight(
    const HexTile* tile,
    float          radius,
    Vector2        origin,
    Color          color
);


void MapView_DrawPath(HexTile** path, int length, float radius, Vector2 origin);
void MapView_DrawMinimap(
    const Map*  map,
    Rectangle   destRect,
    HexCoord    armyPos,
    float       blinkTimer,
    const GameAssets* assets,
    bool debugRevealMap
);

#endif /* MAP_VIEW_H */






