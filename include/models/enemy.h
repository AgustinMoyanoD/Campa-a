#ifndef ENEMY_H
#define ENEMY_H

#include <stdbool.h>
#include "models/hex_tile.h"
#include "models/effect.h"

// Modifiers stub para Enemy
typedef struct {
    int id; // Placeholder para la estructura de Modifier completa
} Modifier;

typedef enum {
    TAG_NONE = 0,
    TAG_CAVALRY = 1 << 0,
    TAG_AMBUSH = 1 << 1,
    TAG_EXPOSED_FLANK = 1 << 2,
    TAG_HEAVILY_ARMORED = 1 << 3,
    TAG_DISORGANIZED = 1 << 4
} EnemyTag;

typedef struct {
    const char* name;
    HexCoord position;
    int baseStrength;
    int maxStrength;
    int mobility;
    
    // Composicion de tropas
    int initiates;
    int soldiers;
    int sergeants;

    unsigned int tags;        // Tags inherentes al escuadron (bits)
    unsigned int knownTags;   // Tags revelados por la evaluacion (bits)
    
    ModifierInstance traits[4];
    int traitCount;
    
    bool isActive;
} EnemySquad;

EnemySquad Enemy_Create(const char* name, HexCoord position, int strength, int mobility, int ini, int sol, int sgt);
void Enemy_AddTag(EnemySquad* enemy, EnemyTag tag);
void Enemy_RevealTags(EnemySquad* enemy, unsigned int tagsToRevealMask);
bool Enemy_HasTag(const EnemySquad* enemy, EnemyTag tag);
bool Enemy_IsTagKnown(const EnemySquad* enemy, EnemyTag tag);

#endif


