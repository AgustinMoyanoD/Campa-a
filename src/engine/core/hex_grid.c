#include "engine/core/hex_grid.h"
#include <stdlib.h>
#include <math.h>

#define ENGINE_ISO_SCALE_Y 0.5f

static const HexCoord HEX_DIRECTIONS[6] = {
    {  1,  0 }, // HEX_RIGHT
    {  0,  1 }, // HEX_DOWN_RIGHT
    { -1,  1 }, // HEX_DOWN_LEFT
    { -1,  0 }, // HEX_LEFT
    {  0, -1 }, // HEX_UP_LEFT
    {  1, -1 }  // HEX_UP_RIGHT
};

HexCoord HexCoord_Create(int q, int r) {
    HexCoord coord;
    coord.q = q;
    coord.r = r;
    return coord;
}

bool HexCoord_Equal(HexCoord a, HexCoord b) {
    return a.q == b.q && a.r == b.r;
}

HexCoord HexCoord_Add(HexCoord a, HexCoord b) {
    return (HexCoord){ a.q + b.q, a.r + b.r };
}

HexCoord HexCoord_Sub(HexCoord a, HexCoord b) {
    return (HexCoord){ a.q - b.q, a.r - b.r };
}

HexCoord HexCoord_GetNeighbor(HexCoord coord, HexDirection direction) {
    coord.q += HEX_DIRECTIONS[direction].q;
    coord.r += HEX_DIRECTIONS[direction].r;
    return coord;
}

HexCoord HexCoord_GetDirectionVector(HexDirection direction) {
    return HEX_DIRECTIONS[direction];
}

void HexCoord_GetNeighbors(HexCoord coord, HexCoord outNeighbors[6]) {
    for (int i = 0; i < 6; i++) {
        outNeighbors[i] = HexCoord_GetNeighbor(coord, (HexDirection)i);
    }
}

int HexCoord_Distance(HexCoord a, HexCoord b) {
    int dq = a.q - b.q;
    int dr = a.r - b.r;
    return (abs(dq) + abs(dq + dr) + abs(dr)) / 2;
}

int HexCoord_Length(HexCoord coord) {
    return (abs(coord.q) + abs(coord.q + coord.r) + abs(coord.r)) / 2;
}

int HexCoord_GetRing(HexCoord center, int radius, HexCoord* outCoords, int maxCoords) {
    if (radius <= 0) {
        if (maxCoords > 0 && outCoords) {
            outCoords[0] = center;
            return 1;
        }
        return 0;
    }
    int count = 0;
    // Empezar en una esquina
    HexCoord current = center;
    for (int i = 0; i < radius; i++) {
        current = HexCoord_GetNeighbor(current, HEX_LEFT);
    }
    for (int dir = 0; dir < 6; dir++) {
        HexDirection stepDir = (HexDirection)((dir + 2) % 6);
        for (int step = 0; step < radius; step++) {
            if (count < maxCoords && outCoords) {
                outCoords[count++] = current;
            }
            current = HexCoord_GetNeighbor(current, stepDir);
        }
    }
    return count;
}

int HexCoord_GetSpiral(HexCoord center, int maxRadius, HexCoord* outCoords, int maxCoords) {
    int total = 0;
    if (total < maxCoords && outCoords) {
        outCoords[total++] = center;
    }
    for (int r = 1; r <= maxRadius; r++) {
        int added = HexCoord_GetRing(center, r, outCoords ? &outCoords[total] : NULL, maxCoords - total);
        total += added;
        if (total >= maxCoords) break;
    }
    return total;
}

Vector2 HexToPixel(HexCoord coord, float radius) {
    Vector2 position;
    position.x = radius * sqrtf(3.0f) * ((float)coord.q + (float)coord.r / 2.0f);
    position.y = (radius * 1.5f * (float)coord.r) * ENGINE_ISO_SCALE_Y;
    return position;
}

HexCoord PixelToHex(Vector2 position, float radius) {
    float unscaledY = position.y / ENGINE_ISO_SCALE_Y;
    float q = (sqrtf(3.0f) / 3.0f * position.x - 1.0f / 3.0f * unscaledY) / radius;
    float r = (2.0f / 3.0f * unscaledY) / radius;

    float x = q, z = r, y = -x - z;
    int rx = (int)roundf(x);
    int ry = (int)roundf(y);
    int rz = (int)roundf(z);

    float dx = fabsf((float)rx - x);
    float dy = fabsf((float)ry - y);
    float dz = fabsf((float)rz - z);

    if (dx > dy && dx > dz) {
        rx = -ry - rz;
    } else if (dy > dz) {
        ry = -rx - rz;
    } else {
        rz = -rx - ry;
    }

    return (HexCoord){ rx, rz };
}

void Hex_GetVertices(Vector2 centerPixel, float radius, Vector2 outVertices[6]) {
    for (int i = 0; i < 6; i++) {
        float angleDeg = 60.0f * (float)i - 30.0f;
        float angleRad = angleDeg * (PI / 180.0f);
        outVertices[i] = (Vector2){
            centerPixel.x + radius * cosf(angleRad),
            centerPixel.y + radius * sinf(angleRad) * ENGINE_ISO_SCALE_Y
        };
    }
}

void Hex_DrawOutline(Vector2 centerPixel, float radius, float lineThick, Color color) {
    Vector2 v[6];
    Hex_GetVertices(centerPixel, radius, v);
    for (int i = 0; i < 6; i++) {
        Vector2 start = v[i];
        Vector2 end = v[(i + 1) % 6];
        DrawLineEx(start, end, lineThick, color);
    }
}

void Hex_DrawFilled(Vector2 centerPixel, float radius, Color fillColor) {
    Vector2 v[6];
    Hex_GetVertices(centerPixel, radius, v);
    for (int i = 0; i < 6; i++) {
        DrawTriangle(centerPixel, v[(i + 1) % 6], v[i], fillColor);
    }
}
