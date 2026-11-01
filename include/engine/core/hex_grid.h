#ifndef ENGINE_HEX_GRID_H
#define ENGINE_HEX_GRID_H

#include <stdbool.h>
#include "raylib.h"

/* ============================================================
   ENGINE HEX GRID: GEOMETRÍA AXIAL PURA
   Independiente de cualquier regla de juego, biomas o estructuras.
   ============================================================ */

/* Coordenadas axiales hexagonales (q, r) */
typedef struct {
    int q;
    int r;
} HexCoord;

/* Las 6 direcciones ortogonales en cuadrícula axial */
typedef enum {
    HEX_RIGHT = 0,
    HEX_DOWN_RIGHT,
    HEX_DOWN_LEFT,
    HEX_LEFT,
    HEX_UP_LEFT,
    HEX_UP_RIGHT
} HexDirection;

/* Constructores y operadores básicos */
HexCoord HexCoord_Create(int q, int r);
bool     HexCoord_Equal(HexCoord a, HexCoord b);
HexCoord HexCoord_Add(HexCoord a, HexCoord b);
HexCoord HexCoord_Sub(HexCoord a, HexCoord b);

/* Navegación y Vecinos */
HexCoord HexCoord_GetNeighbor(HexCoord coord, HexDirection direction);
HexCoord HexCoord_GetDirectionVector(HexDirection direction);
void     HexCoord_GetNeighbors(HexCoord coord, HexCoord outNeighbors[6]);

/* Distancias y Rangos */
int  HexCoord_Distance(HexCoord a, HexCoord b);
int  HexCoord_Length(HexCoord coord);
int  HexCoord_GetRing(HexCoord center, int radius, HexCoord* outCoords, int maxCoords);
int  HexCoord_GetSpiral(HexCoord center, int maxRadius, HexCoord* outCoords, int maxCoords);

/* Proyección y Renderizado Geométrico */
Vector2 HexToPixel(HexCoord coord, float radius);
HexCoord PixelToHex(Vector2 position, float radius);
void Hex_GetVertices(Vector2 centerPixel, float radius, Vector2 outVertices[6]);
void Hex_DrawOutline(Vector2 centerPixel, float radius, float lineThick, Color color);
void Hex_DrawFilled(Vector2 centerPixel, float radius, Color fillColor);

#endif /* ENGINE_HEX_GRID_H */
