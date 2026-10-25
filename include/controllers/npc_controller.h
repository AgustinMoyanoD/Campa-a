#ifndef NPC_CONTROLLER_H
#define NPC_CONTROLLER_H

#include "models/enemy.h"
#include "models/army.h"
#include "models/map_generator.h"
#include "models/city.h"
#include <stdbool.h>

struct GameMaster;

/* ========================================================
   CONSULTAS DE FACCIÓN Y RELACIÓN (Lógica y futuro NPC_view)
   ======================================================== */

/* Obtiene la facción dominante de una casilla (por estructura o cercanía Voronoi) */
int NPC_GetTileFaction(const Map* map, HexCoord coord);

/* Obtiene el nivel de opinión de la facción del NPC con respecto al jugador */
FactionOpinionLevel NPC_GetRelationWithPlayer(const EnemySquad* npc, const struct GameMaster* gm);

/* Determina si el NPC es hostil por defecto contra el jugador (ataca por iniciativa propia) */
bool NPC_IsHostileToPlayer(const EnemySquad* npc, const struct GameMaster* gm);

/* Comprueba si el escuadrón es una banda de bandidos/salteadores */
bool NPC_IsBandit(const EnemySquad* npc);

/* ========================================================
   FÁBRICA DE ESCUADRONES (Mapa, Eventos, Cazador y Asedios)
   ======================================================== */

/* Crea un escuadrón para el mapa según facción del territorio y franja horaria */
EnemySquad NPC_CreateMapSquad(struct GameMaster* gm, Map* map, HexCoord spawnCoord);

/* Crea un escuadrón a partir del identificador de combate de un evento (.res) */
EnemySquad NPC_CreateEventSquad(const char* battleEnemyId, const char* resultId, bool* outAmbushed, int* outInitiator);

/* Crea un Cazador Rastreador para castigo de marcha nocturna */
EnemySquad NPC_CreateHunterSquad(struct GameMaster* gm, const Army* playerArmy, HexCoord spawnCoord);

/* Crea una Guarnición Fortificada para asedio */
EnemySquad NPC_CreateGarrisonSquad(HexCoord targetCoord, int factionId);

/* ========================================================
   GESTIÓN EN EL MAPA, IA Y COMBATE
   ======================================================== */

/* Spawnea los escuadrones iniciales en el mapa al comenzar la partida */
void NPC_SpawnInitialSquads(struct GameMaster* gm, Map* map, HexCoord playerPos);

/* Ejecuta la fase de turno de los NPCs (reabastecimiento, visión, IA y movimiento) */
void NPC_ProcessTurn(struct GameMaster* gm, Map* map, Army* playerArmy);

/* Comprueba si el jugador está en Zona de Control (adyacente a un NPC hostil activo) */
bool NPC_IsPlayerInZOC(const struct GameMaster* gm, HexCoord playerPos);

/* Busca un escuadrón HOSTIL en la coordenada dada (-1 si no hay ninguno) */
int NPC_FindHostileSquadAt(const struct GameMaster* gm, HexCoord pos);

/* Busca cualquier escuadrón activo en la misma casilla o adyacente para ataque voluntario */
int NPC_FindAttackableSquadNear(const struct GameMaster* gm, HexCoord playerPos);

/* Inicia combate contra un NPC centralizando Combat_Init, initiator, ambushed y lootMultiplier */
void NPC_InitiateCombat(struct GameMaster* gm, Army* playerArmy, EnemySquad* npc, int initiator, bool ambushed);

/* Ejecuta un ataque voluntario iniciado por el jugador (acción con tecla [X]) */
void NPC_PlayerVoluntaryAttack(struct GameMaster* gm, Army* playerArmy, int npcIndex);

#endif /* NPC_CONTROLLER_H */
