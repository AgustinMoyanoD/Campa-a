#include "engine/core/pathfinding.h"
#include <stdlib.h>
#include <string.h>

#define DEFAULT_MAX_NODES 2048

typedef struct PathNode {
    HexCoord coord;
    float g;
    float f;
    int parentIndex;
    bool inOpen;
    bool inClosed;
} PathNode;

static int FindOrAddNode(PathNode* pool, int* poolSize, int maxNodes, HexCoord coord) {
    for (int i = 0; i < *poolSize; i++) {
        if (HexCoord_Equal(pool[i].coord, coord)) {
            return i;
        }
    }
    if (*poolSize >= maxNodes) return -1;
    int idx = (*poolSize)++;
    pool[idx].coord = coord;
    pool[idx].g = 1e9f;
    pool[idx].f = 1e9f;
    pool[idx].parentIndex = -1;
    pool[idx].inOpen = false;
    pool[idx].inClosed = false;
    return idx;
}

int Pathfind_AStar(const PathfindParams* params,
                   HexCoord* outPath,
                   int maxPathLength,
                   float* outTotalCost)
{
    if (outTotalCost) *outTotalCost = 0.0f;
    if (!params || !outPath || maxPathLength <= 0) return 0;
    if (HexCoord_Equal(params->start, params->goal)) return 0;

    int maxNodes = params->maxSearchNodes > 0 ? params->maxSearchNodes : DEFAULT_MAX_NODES;
    PathNode* pool = (PathNode*)calloc(maxNodes, sizeof(PathNode));
    if (!pool) return 0;

    int poolSize = 0;
    int startIdx = FindOrAddNode(pool, &poolSize, maxNodes, params->start);
    if (startIdx < 0) {
        free(pool);
        return 0;
    }

    pool[startIdx].g = 0.0f;
    float startH = params->heuristic ? params->heuristic(params->start, params->goal, params->userData)
                                     : (float)HexCoord_Distance(params->start, params->goal);
    pool[startIdx].f = startH;
    pool[startIdx].inOpen = true;

    int goalIdx = -1;

    while (true) {
        int currentIdx = -1;
        float lowestF = 1e9f;

        for (int i = 0; i < poolSize; i++) {
            if (pool[i].inOpen && pool[i].f < lowestF) {
                lowestF = pool[i].f;
                currentIdx = i;
            }
        }

        if (currentIdx == -1) break; // Sin ruta posible

        if (HexCoord_Equal(pool[currentIdx].coord, params->goal)) {
            goalIdx = currentIdx;
            break;
        }

        pool[currentIdx].inOpen = false;
        pool[currentIdx].inClosed = true;

        HexCoord currCoord = pool[currentIdx].coord;
        HexCoord neighbors[6];
        HexCoord_GetNeighbors(currCoord, neighbors);

        for (int dir = 0; dir < 6; dir++) {
            HexCoord nbCoord = neighbors[dir];

            // Si es la meta o es transitable
            bool isGoal = HexCoord_Equal(nbCoord, params->goal);
            if (!isGoal && params->isPassable && !params->isPassable(nbCoord, params->userData)) {
                continue;
            }

            int nbIdx = FindOrAddNode(pool, &poolSize, maxNodes, nbCoord);
            if (nbIdx < 0) continue;
            if (pool[nbIdx].inClosed) continue;

            float stepCost = params->getCost ? params->getCost(currCoord, nbCoord, params->userData) : 1.0f;
            float tentativeG = pool[currentIdx].g + stepCost;

            if (!pool[nbIdx].inOpen || tentativeG < pool[nbIdx].g) {
                pool[nbIdx].parentIndex = currentIdx;
                pool[nbIdx].g = tentativeG;
                float h = params->heuristic ? params->heuristic(nbCoord, params->goal, params->userData)
                                            : (float)HexCoord_Distance(nbCoord, params->goal);
                pool[nbIdx].f = tentativeG + h;
                pool[nbIdx].inOpen = true;
            }
        }
    }

    int length = 0;
    if (goalIdx >= 0) {
        // Calcular longitud contando hacia atrás
        int temp = goalIdx;
        while (pool[temp].parentIndex != -1) {
            length++;
            temp = pool[temp].parentIndex;
        }

        if (length > maxPathLength) {
            length = 0; // Excede buffer
        } else {
            if (outTotalCost) *outTotalCost = pool[goalIdx].g;
            int writePos = length - 1;
            int curr = goalIdx;
            while (pool[curr].parentIndex != -1 && writePos >= 0) {
                outPath[writePos--] = pool[curr].coord;
                curr = pool[curr].parentIndex;
            }
        }
    }

    free(pool);
    return length;
}
