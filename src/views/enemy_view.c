#include "views/enemy_view.h"
#include "views/npc_view.h"
#include <stddef.h>

/* Wrapper de compatibilidad hacia el nuevo NPC_view */
void EnemyView_DrawOnMap(const EnemySquad* enemy, Vector2 visualPos, Vector2 origin, const GameAssets* assets, bool isHovered, float hexRadius) {
    NPCView_DrawOnMap(enemy, NULL, visualPos, origin, assets, isHovered, hexRadius);
}
