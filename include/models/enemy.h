#ifndef ENEMY_H
#define ENEMY_H

#include "models/hex_tile.h"
#include <stdbool.h>

typedef enum {
    ENEMY_BANDITS,
    ENEMY_REBELS,
    ENEMY_MONSTER
} EnemyType;

typedef struct {
    int id;
    EnemyType type;
    HexCoord position;
    int strength;
    bool isActive;
} EnemySquad;

EnemySquad Enemy_Create(int id, EnemyType type, HexCoord position, int strength);

#endif /* ENEMY_H */
