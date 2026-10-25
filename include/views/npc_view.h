#ifndef NPC_VIEW_H
#define NPC_VIEW_H

#include "models/enemy.h"
#include "models/hex_tile.h"
#include "models/city.h"
#include <raylib.h>
#include "controllers/asset_manager.h"

struct GameMaster;

/* =========================================================================
   NPC_VIEW: Renderizado reactivo de personajes del mapa (Facciones y Diplomacia)
   ========================================================================= */

/* Dibuja un escuadrón NPC en el mapa con anillos diplomáticos, rango de visión/amenaza,
   heráldica de facción, insignias de IA y tooltip contextual enriquecido */
void NPCView_DrawOnMap(const NPCSquad* npc, const struct GameMaster* gm, 
                       Vector2 visualPos, Vector2 origin, 
                       const GameAssets* assets, bool isHovered, float hexRadius);

/* Consultas auxiliares de color y nombres de facción para la UI */
Color NPCView_GetRelationColor(FactionOpinionLevel relation);
const char* NPCView_GetFactionName(int factionId, const struct GameMaster* gm);

#endif /* NPC_VIEW_H */
