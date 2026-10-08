#include "models/enemy.h"

EnemySquad Enemy_Create(int id, EnemyType type, HexCoord position, int strength) {
    EnemySquad squad;
    squad.id = id;
    squad.type = type;
    squad.position = position;
    squad.strength = strength;
    squad.isActive = true;
    return squad;
}
