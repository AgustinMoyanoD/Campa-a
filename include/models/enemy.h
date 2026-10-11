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
    TAG_DISORGANIZED = 1 << 4,
    TAG_FORTIFIED = 1 << 5
} EnemyTag;

typedef enum {
    AI_PATROL = 0,
    AI_GUARD,
    AI_HUNTER,
    AI_AMBUSH,
    AI_BEAST
} EnemyBehavior;

typedef enum {
    AIS_IDLE = 0,
    AIS_PATROLLING,
    AIS_ALERT,
    AIS_CHASING,
    AIS_SEARCHING,
    AIS_RETURNING,
    AIS_RESTING
} EnemyAIState;

/* Alias de compatibilidad */
#define AI_IDLE AIS_IDLE
#define AI_CHASE AIS_CHASING

typedef enum {
    INTENT_NONE = 0,
    INTENT_PATROL,
    INTENT_CHASE
} EnemyIntent;

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
    unsigned int knownTags;
    int infoLevel;   // Tags revelados por la evaluacion (bits)
    
    ModifierInstance traits[4];
    int traitCount;
    
    bool isActive;
    
    // IA del escuadrón (Sección 8.2)
    EnemyBehavior behavior;
    EnemyAIState  aiState;
    EnemyIntent   currentIntent;
    
    int senseRadius;
    int leash;                    // Radio de alejamiento máximo para guardia
    HexCoord homeHex;             // Base / spawn de origen
    HexCoord originalSpawn;       // Alias compatible
    HexCoord patrolTarget;        // Objetivo actual
    HexCoord lastKnownPlayerPos;  // Última posición conocida del jugador
    int chaseTurnsLeft;           // Contador antes de rendirse
    int skipNextPhase;            // Salto de fase tras huida
    
    // Rutas para patrulla
    HexCoord route[16];
    int routeLength;
    int routeIndex;
    int patrolWaitTurns;
    
    // Combate
    int initiator; // 0 = jugador, 1 = enemigo
} EnemySquad;

EnemySquad Enemy_Create(const char* name, HexCoord position, int strength, int mobility, int ini, int sol, int sgt);
EnemySquad Enemy_CreateWithBehavior(const char* name, HexCoord position, int strength, int mobility, int ini, int sol, int sgt, EnemyBehavior behavior);
void Enemy_SetPatrolRoute(EnemySquad* enemy, const HexCoord* waypoints, int count);
void Enemy_SetGuardLeash(EnemySquad* enemy, int leash);
void Enemy_AddTag(EnemySquad* enemy, EnemyTag tag);
void Enemy_RevealTags(EnemySquad* enemy, unsigned int tagsToRevealMask);
bool Enemy_HasTag(const EnemySquad* enemy, EnemyTag tag);
bool Enemy_IsTagKnown(const EnemySquad* enemy, EnemyTag tag);
const char* Enemy_GetBehaviorName(EnemyBehavior b);
const char* Enemy_GetAIStateName(EnemyAIState s);

#endif
