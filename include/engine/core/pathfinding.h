#ifndef ENGINE_PATHFINDING_H
#define ENGINE_PATHFINDING_H

#include "engine/core/hex_grid.h"
#include <stdbool.h>

/* ============================================================
   ENGINE PATHFINDING: ALGORITMO A* GENÉRICO EN CUADRÍCULA HEX
   Independiente de reglas de juego, niebla, carreteras o unidades.
   ============================================================ */

/* Callback para verificar si una coordenada es transitable */
typedef bool  (*PathPassableFn)(HexCoord coord, void* userData);

/* Callback para calcular el coste de movimiento entre casillas adyacentes */
typedef float (*PathCostFn)(HexCoord from, HexCoord to, void* userData);

/* Callback heurístico opcional (si es NULL se usa HexCoord_Distance) */
typedef float (*PathHeuristicFn)(HexCoord a, HexCoord b, void* userData);

/* Estructura de parámetros para la búsqueda A* */
typedef struct {
    HexCoord          start;
    HexCoord          goal;
    PathPassableFn    isPassable;
    PathCostFn        getCost;
    PathHeuristicFn   heuristic;
    void*             userData;
    int               maxSearchNodes; /* Límite de nodos a explorar (ej: 2048) */
} PathfindParams;

/* Ejecuta A* y llena outPath con las coordenadas en orden (desde el 1er paso hasta goal).
   Retorna la longitud del camino (0 si no hay camino o start == goal). */
int Pathfind_AStar(const PathfindParams* params,
                   HexCoord* outPath,
                   int maxPathLength,
                   float* outTotalCost);

#endif /* ENGINE_PATHFINDING_H */
