#ifndef ENEMY_CONTROLLER_H
#define ENEMY_CONTROLLER_H

#include "controllers/npc_controller.h"

/* Alias y macros para compatibilidad entre enemy_controller y npc_controller */
#define Enemy_ProcessTurn        NPC_ProcessTurn
#define Enemy_SpawnInitialSquads NPC_SpawnInitialSquads
#define Enemy_IsPlayerInZOC      NPC_IsPlayerInZOC
#define Enemy_CreateMapSquad     NPC_CreateMapSquad
#define Enemy_CreateEventSquad   NPC_CreateEventSquad
#define Enemy_CreateHunterSquad  NPC_CreateHunterSquad
#define Enemy_CreateGarrisonSquad NPC_CreateGarrisonSquad

#endif /* ENEMY_CONTROLLER_H */
