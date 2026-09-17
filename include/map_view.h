#ifndef MAP_VIEW_H
#define MAP_VIEW_H

#include "raylib.h"
#include "map_generator.h"

Vector2 HexToPixel(
    HexCoord coord,
    float radius
);

HexCoord PixelToHex(
    Vector2 position,
    float radius
);

void MapView_DrawHex(
    const HexTile* tile,
    float radius,
    Vector2 origin
);

void MapView_DrawConnections(
    const Map* map,
    float radius,
    Vector2 origin,
    Color color,
    float thickness
);

void MapView_Draw(
    const Map* map,
    float radius,
    Vector2 origin,
    bool drawConnections,
    const HexTile* centerTile,
    int renderRadius
);

void MapView_DrawHexHighlight(
    const HexTile* tile,
    float radius,
    Vector2 origin,
    Color color
);

#endif