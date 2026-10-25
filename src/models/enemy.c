#include "models/enemy.h"
#include <string.h>

EnemySquad Enemy_Create(const char* name, HexCoord position, int strength, int mobility, int ini, int sol, int sgt) {
    return Enemy_CreateWithBehavior(name, position, strength, mobility, ini, sol, sgt, AI_PATROL);
}

EnemySquad Enemy_CreateWithBehavior(const char* name, HexCoord position, int strength, int mobility, int ini, int sol, int sgt, EnemyBehavior behavior) {
    EnemySquad squad;
    memset(&squad, 0, sizeof(EnemySquad));
    squad.name = name;
    squad.position = position;
    squad.homeHex = position;
    squad.originalSpawn = position;
    squad.patrolTarget = position;
    squad.lastKnownPlayerPos = position;

    squad.baseStrength = strength;
    squad.maxStrength = strength;
    squad.mobility = mobility;
    squad.initiates = ini;
    squad.soldiers = sol;
    squad.sergeants = sgt;
    squad.tags = TAG_NONE;
    squad.knownTags = TAG_NONE;
    squad.isActive = true;
    squad.infoLevel = 0;

    squad.behavior = behavior;
    squad.aiState = AIS_IDLE;
    squad.currentIntent = INTENT_NONE;

    squad.senseRadius = 3;
    squad.leash = 4;
    squad.chaseTurnsLeft = 0;
    squad.skipNextPhase = 0;
    squad.initiator = 0;
    squad.factionId = -1;
    squad.alwaysHostile = true;
    squad.lootMultiplier = 1.0f;

    if (behavior == AI_PATROL) {
        squad.aiState = AIS_PATROLLING;
        squad.currentIntent = INTENT_PATROL;
    } else if (behavior == AI_AMBUSH) {
        squad.tags |= TAG_AMBUSH;
        squad.senseRadius = 2;
        squad.aiState = AIS_PATROLLING;
        squad.currentIntent = INTENT_PATROL;
    } else if (behavior == AI_GUARD) {
        squad.leash = 3;
        squad.aiState = AIS_PATROLLING;
        squad.currentIntent = INTENT_PATROL;
    } else if (behavior == AI_HUNTER) {
        squad.senseRadius = 5;
        squad.aiState = AIS_CHASING;
        squad.currentIntent = INTENT_CHASE;
        squad.chaseTurnsLeft = 999;
    } else if (behavior == AI_BEAST) {
        squad.mobility += 2;
        squad.senseRadius = 3;
        squad.aiState = AIS_PATROLLING;
        squad.currentIntent = INTENT_PATROL;
    }

    return squad;
}

void Enemy_SetPatrolRoute(EnemySquad* enemy, const HexCoord* waypoints, int count) {
    if (!enemy || !waypoints) return;
    int limit = count;
    if (limit > 16) limit = 16;
    for (int i = 0; i < limit; i++) {
        enemy->route[i] = waypoints[i];
    }
    enemy->routeLength = limit;
    enemy->routeIndex = 0;
    if (limit > 0) {
        enemy->patrolTarget = enemy->route[0];
    }
}

void Enemy_SetGuardLeash(EnemySquad* enemy, int leash) {
    if (enemy) {
        enemy->leash = leash;
    }
}

void Enemy_AddTag(EnemySquad* enemy, EnemyTag tag) {
    if (enemy) {
        enemy->tags |= tag;
    }
}

void Enemy_RevealTags(EnemySquad* enemy, unsigned int tagsToRevealMask) {
    if (enemy) {
        enemy->knownTags |= (enemy->tags & tagsToRevealMask);
    }
}

bool Enemy_HasTag(const EnemySquad* enemy, EnemyTag tag) {
    if (!enemy) return false;
    return (enemy->tags & tag) != 0;
}

bool Enemy_IsTagKnown(const EnemySquad* enemy, EnemyTag tag) {
    if (!enemy) return false;
    return (enemy->knownTags & tag) != 0;
}

const char* Enemy_GetBehaviorName(EnemyBehavior b) {
    switch (b) {
        case AI_PATROL: return "Patrulla";
        case AI_GUARD:  return "Guardia";
        case AI_HUNTER: return "Cazador";
        case AI_AMBUSH: return "Emboscador";
        case AI_BEAST:  return "Bestia";
        default:        return "Desconocido";
    }
}

const char* Enemy_GetAIStateName(EnemyAIState s) {
    switch (s) {
        case AIS_IDLE:       return "Guardia / Inmovil";
        case AIS_PATROLLING: return "Patrullando";
        case AIS_ALERT:      return "Alerta / Sospecha";
        case AIS_CHASING:    return "En Persecucion";
        case AIS_SEARCHING:  return "Rastreando";
        case AIS_RETURNING:  return "Regresando al puesto";
        case AIS_RESTING:    return "Descansando / Saqueando";
        default:             return "Normal";
    }
}
