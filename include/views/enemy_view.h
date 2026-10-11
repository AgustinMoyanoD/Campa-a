#ifndef ENEMY_VIEW_H
#define ENEMY_VIEW_H

#include "models/enemy.h"
#include "models/hex_tile.h"
#include <raylib.h>
#include "controllers/asset_manager.h"

// Dibuja un escuadron enemigo en el mapa con indicador de estado y rango de amenaza
void EnemyView_DrawOnMap(const EnemySquad* enemy, Vector2 visualPos, Vector2 origin, const GameAssets* assets, bool isHovered, float hexRadius);

#endif
