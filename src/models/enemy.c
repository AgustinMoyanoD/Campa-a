#include "models/enemy.h"

EnemySquad Enemy_Create(const char* name, HexCoord position, int strength, int mobility, int ini, int sol, int sgt) {
    EnemySquad squad;
    squad.name = name;
    squad.position = position;
    squad.baseStrength = strength;
    squad.mobility = mobility;
    squad.initiates = ini;
    squad.soldiers = sol;
    squad.sergeants = sgt;
    squad.tags = TAG_NONE;
    squad.knownTags = TAG_NONE;
    squad.isActive = true;
    return squad;
}

void Enemy_AddTag(EnemySquad* enemy, EnemyTag tag) {
    if (enemy) {
        enemy->tags |= tag;
    }
}

void Enemy_RevealTags(EnemySquad* enemy, unsigned int tagsToRevealMask) {
    if (enemy) {
        // Revela solo los tags que el enemigo realmente tiene y que estan en la mascara
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

