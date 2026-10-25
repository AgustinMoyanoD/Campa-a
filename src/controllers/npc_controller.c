#include "controllers/npc_controller.h"
#include "controllers/game_master.h"
#include "models/balance_config.h"
#include "models/rng.h"
#include "models/combat.h"
#include "presentation/audio_manager.h"
#include "raylib.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

/* ========================================================
   CONSULTAS DE FACCIÃ“N Y RELACIÃ“N
   ======================================================== */

int NPC_GetTileFaction(const Map* map, HexCoord coord) {
    if (!map) return 0;
    HexTile* tile = Map_GetTile(map, coord);
    if (tile && tile->structure.type != STRUCT_NONE && 
        tile->structure.factionId >= 0 && tile->structure.factionId < 3) {
        return tile->structure.factionId;
    }

    int bestFaction = 0;
    int bestDist = 9999;
    for (int i = 0; i < map->tileCount; i++) {
        if (map->tiles[i].structure.type != STRUCT_NONE && 
            map->tiles[i].structure.factionId >= 0 && map->tiles[i].structure.factionId < 3) {
            int d = HexCoord_Distance(coord, map->tiles[i].coord);
            if (d < bestDist) {
                bestDist = d;
                bestFaction = map->tiles[i].structure.factionId;
            }
        }
    }
    return bestFaction;
}

FactionOpinionLevel NPC_GetRelationWithPlayer(const EnemySquad* npc, const struct GameMaster* gm) {
    if (!npc || !gm) return FACTION_HOSTILE;
    if (npc->alwaysHostile || npc->factionId < 0 || npc->factionId >= 3) {
        return FACTION_HOSTILE;
    }
    const BalanceConfig* cfg = Balance_GetGlobal();
    return Faction_GetLevel(gm->factions[npc->factionId].opinion, cfg);
}

bool NPC_IsHostileToPlayer(const EnemySquad* npc, const struct GameMaster* gm) {
    if (!npc || !gm) return true;
    if (npc->alwaysHostile || npc->factionId < 0) {
        return true;
    }
    return (NPC_GetRelationWithPlayer(npc, gm) == FACTION_HOSTILE);
}

bool NPC_IsBandit(const EnemySquad* npc) {
    if (!npc) return false;
    if (npc->factionId == -1 && 
        (strstr(npc->name, "Salteadores") != NULL || strstr(npc->name, "Bandidos") != NULL)) {
        return true;
    }
    return false;
}

/* Selecciona un objetivo de patrulla dinamico segun la doctrina y tipo de escuadron */
static HexCoord NPC_PickPatrolTarget(struct GameMaster* gm, Map* map, const EnemySquad* e) {
    if (!gm || !map || !e) return (HexCoord){0, 0};

    // 1. Guardias: patrullar cerca de su puesto / homeHex dentro de su correa (leash)
    if (e->behavior == AI_GUARD) {
        int leashLimit = (e->leash > 0) ? e->leash : 3;
        for (int tries = 0; tries < 20; tries++) {
            int r = Rng_Range(&gm->rng, 0, map->tileCount - 1);
            HexTile* t = &map->tiles[r];
            if (HexCoord_Distance(t->coord, e->homeHex) <= leashLimit &&
                t->biome != BIOME_WATER && t->terrain != TERRAIN_WATER) {
                return t->coord;
            }
        }
        return e->homeHex;
    }

    // 2. Bandidos / Salteadores: patrullar calzadas o acechar cerca de asentamientos
    if (NPC_IsBandit(e)) {
        for (int tries = 0; tries < 25; tries++) {
            int r = Rng_Range(&gm->rng, 0, map->tileCount - 1);
            HexTile* t = &map->tiles[r];
            int dist = HexCoord_Distance(t->coord, e->position);
            if (dist >= 3 && dist <= 7 && (t->isRoad || t->structure.type != STRUCT_NONE) &&
                t->biome != BIOME_WATER && t->terrain != TERRAIN_WATER) {
                return t->coord;
            }
        }
    }

    // 3. Patrullas de Faccion: priorizar calzadas y estructuras de su propio territorio
    int myFaction = e->factionId;
    for (int tries = 0; tries < 30; tries++) {
        int r = Rng_Range(&gm->rng, 0, map->tileCount - 1);
        HexTile* t = &map->tiles[r];
        int dist = HexCoord_Distance(t->coord, e->position);
        if (dist >= 3 && dist <= 8 && t->biome != BIOME_WATER && t->terrain != TERRAIN_WATER) {
            if (t->isRoad || (myFaction >= 0 && t->structure.factionId == myFaction)) {
                return t->coord;
            }
        }
    }

    // 4. Fallback general: cualquier casilla de tierra transitable a distancia 2 a 6
    for (int tries = 0; tries < 20; tries++) {
        int r = Rng_Range(&gm->rng, 0, map->tileCount - 1);
        HexTile* t = &map->tiles[r];
        int dist = HexCoord_Distance(t->coord, e->position);
        if (dist >= 2 && dist <= 6 && t->biome != BIOME_WATER && t->terrain != TERRAIN_WATER) {
            return t->coord;
        }
    }

    return e->position;
}

/* ========================================================
   FÃBRICA DE ESCUADRONES (Mapa, Eventos, Cazador y Asedios)
   ======================================================== */

EnemySquad NPC_CreateMapSquad(struct GameMaster* gm, Map* map, HexCoord spawnCoord) {
    HexTile* tile = Map_GetTile(map, spawnCoord);
    int factionId = NPC_GetTileFaction(map, spawnCoord);
    EnemySquad e;

    // 1. DecisiÃ³n de Bandidos / Salteadores (Independientes - Prioridad Nocturna)
    bool spawnBandits = false;
    if (gm->timeOfDay == TIME_NIGHT) {
        spawnBandits = (Rng_Range(&gm->rng, 0, 99) < 60);
    } else {
        spawnBandits = (Rng_Range(&gm->rng, 0, 99) < 15);
    }

    if (spawnBandits) {
        const char* name = (gm->timeOfDay == TIME_NIGHT) ? "Salteadores Nocturnos" : "Bandidos de los Caminos";
        int strength = 30 + (gm->currentDay * 4);
        int mobility = 16 + gm->currentDay;
        EnemyBehavior beh = (Rng_Range(&gm->rng, 0, 1) == 0) ? AI_AMBUSH : AI_HUNTER;
        e = Enemy_CreateWithBehavior(name, spawnCoord, strength, mobility, 0, 5, 3, beh);
        e.factionId = -1;
        e.alwaysHostile = true;
        e.lootMultiplier = 1.0f;
        Enemy_AddTag(&e, TAG_AMBUSH);
        if (beh == AI_AMBUSH) {
            Enemy_AddTag(&e, TAG_CAVALRY);
        }
    } else if (factionId == 0) {
        // FacciÃ³n 0: Gremio de Comerciantes
        bool isMarket = (tile && tile->structure.type == STRUCT_TOWN_MARKET);
        bool isMainRoad = (tile && tile->road == ROAD_MAIN);
        int treasureChance = isMarket ? 40 : (isMainRoad ? 25 : 20);

        if (Rng_Range(&gm->rng, 0, 99) < treasureChance) {
            // Caravana de Tesoro / Custodia Especial (20% o mercados/rutas principales)
            int strength = 55 + (gm->currentDay * 5);
            int mobility = 7;
            e = Enemy_CreateWithBehavior("Guardia del Tesoro del Gremio", spawnCoord, strength, mobility, 10, 22, 6, AI_GUARD);
            e.factionId = 0;
            e.alwaysHostile = false;
            e.lootMultiplier = 2.0f;
            Enemy_SetGuardLeash(&e, 3);
            Enemy_AddTag(&e, TAG_HEAVILY_ARMORED);
        } else {
            // Escolta Ligera del Gremio (80%)
            int strength = 20 + (gm->currentDay * 2);
            int mobility = 12;
            e = Enemy_CreateWithBehavior("Escolta Ligera del Gremio", spawnCoord, strength, mobility, 6, 4, 0, AI_PATROL);
            e.factionId = 0;
            e.alwaysHostile = false;
            e.lootMultiplier = 1.0f;
            if (Rng_Range(&gm->rng, 0, 1) == 0) {
                Enemy_AddTag(&e, TAG_EXPOSED_FLANK);
            } else {
                Enemy_AddTag(&e, TAG_DISORGANIZED);
            }
        }
    } else if (factionId == 1) {
        // FacciÃ³n 1: LegiÃ³n del Sol
        int strength = 40 + (gm->currentDay * 4);
        int mobility = 10 + gm->currentDay;
        EnemyBehavior beh = (tile && tile->structure.type != STRUCT_NONE) ? AI_GUARD : AI_PATROL;
        e = Enemy_CreateWithBehavior("Patrulla de la Legion del Sol", spawnCoord, strength, mobility, 10, 18, 4, beh);
        e.factionId = 1;
        e.alwaysHostile = false;
        e.lootMultiplier = 1.0f;
        if (beh == AI_GUARD) {
            Enemy_SetGuardLeash(&e, 4);
        }
        Enemy_AddTag(&e, TAG_HEAVILY_ARMORED);
    } else {
        // FacciÃ³n 2: Hermandad del Hierro
        int strength = 45 + (gm->currentDay * 5);
        int mobility = 12 + gm->currentDay;
        EnemyBehavior beh = (Rng_Range(&gm->rng, 0, 1) == 0) ? AI_HUNTER : AI_PATROL;
        e = Enemy_CreateWithBehavior("Incursores de la Hermandad del Hierro", spawnCoord, strength, mobility, 5, 20, 8, beh);
        e.factionId = 2;
        e.alwaysHostile = false;
        e.lootMultiplier = 1.0f;
        if (Rng_Range(&gm->rng, 0, 1) == 0) {
            Enemy_AddTag(&e, TAG_CAVALRY);
        } else {
            Enemy_AddTag(&e, TAG_HEAVILY_ARMORED);
        }
    }

    e.originalSpawn = e.position;
    e.homeHex = e.position;
    e.patrolTarget = NPC_PickPatrolTarget(gm, map, &e);
    e.patrolWaitTurns = 0;
    return e;
}

EnemySquad NPC_CreateEventSquad(const char* battleEnemyId, const char* resultId, bool* outAmbushed, int* outInitiator) {
    EnemySquad eventEnemy;
    const char* bId = battleEnemyId ? battleEnemyId : "";

    if (strcasecmp(bId, "Bandidos_Nocturnos") == 0) {
        eventEnemy = Enemy_Create("Salteadores Nocturnos", (HexCoord){0,0}, 35, 16, 0, 6, 4);
        eventEnemy.factionId = -1;
        eventEnemy.alwaysHostile = true;
        eventEnemy.lootMultiplier = 1.0f;
        Enemy_AddTag(&eventEnemy, TAG_AMBUSH);
        Enemy_AddTag(&eventEnemy, TAG_CAVALRY);
        if (outAmbushed) *outAmbushed = true;
        if (outInitiator) *outInitiator = 1;
    } else if (strcasecmp(bId, "Guardia_Tesoro_Gremio") == 0) {
        eventEnemy = Enemy_Create("Guardia del Tesoro del Gremio", (HexCoord){0,0}, 65, 8, 10, 25, 8);
        eventEnemy.factionId = 0;
        eventEnemy.alwaysHostile = true;
        eventEnemy.lootMultiplier = 2.0f;
        Enemy_AddTag(&eventEnemy, TAG_HEAVILY_ARMORED);
        Enemy_AddTag(&eventEnemy, TAG_FORTIFIED);
        if (outAmbushed) *outAmbushed = false;
        if (outInitiator) *outInitiator = 0;
    } else {
        eventEnemy = Enemy_Create(bId, (HexCoord){0,0}, 35, 12, 10, 5, 2);
        eventEnemy.factionId = -1;
        eventEnemy.alwaysHostile = true;
        eventEnemy.lootMultiplier = 1.0f;
        Enemy_AddTag(&eventEnemy, TAG_CAVALRY);
        if (outAmbushed) *outAmbushed = (resultId && strcmp(resultId, "res_camp_luchar") == 0);
        if (outInitiator) *outInitiator = 0;
    }

    return eventEnemy;
}

EnemySquad NPC_CreateHunterSquad(struct GameMaster* gm, const Army* playerArmy, HexCoord spawnCoord) {
    const BalanceConfig* cfg = Balance_GetGlobal();
    int hunterMob = (int)roundf(Army_CalculateMobility(playerArmy) * cfg->hunterMobilityMult);
    if (hunterMob < 4) hunterMob = 4;

    EnemySquad hunter = Enemy_CreateWithBehavior(
        "Cazador Rastreador",
        spawnCoord,
        35 + (gm->currentDay * 4),
        hunterMob,
        0, 20, 5,
        AI_HUNTER
    );
    hunter.factionId = -1;
    hunter.alwaysHostile = true;
    hunter.lootMultiplier = 1.0f;
    hunter.aiState = AIS_CHASING;
    hunter.currentIntent = INTENT_CHASE;
    hunter.lastKnownPlayerPos = playerArmy->position;
    hunter.chaseTurnsLeft = 999;
    return hunter;
}

EnemySquad NPC_CreateGarrisonSquad(HexCoord targetCoord, int factionId) {
    const BalanceConfig* cfg = Balance_GetGlobal();
    int gHealth = cfg ? cfg->siegeGarrisonHealth : 150;
    EnemySquad garrison = Enemy_Create("Guarnicion Fortificada", targetCoord, 50, 10, 15, 20, 5);
    Enemy_AddTag(&garrison, TAG_FORTIFIED);
    Enemy_AddTag(&garrison, TAG_HEAVILY_ARMORED);
    garrison.factionId = factionId;
    garrison.alwaysHostile = true;
    garrison.lootMultiplier = (cfg && cfg->siegeLootMultiplier > 0.0f) ? cfg->siegeLootMultiplier : 3.0f;
    return garrison;
}

/* ========================================================
   GESTIÃ“N EN EL MAPA, IA Y COMBATE
   ======================================================== */

void NPC_SpawnInitialSquads(struct GameMaster* gm, Map* map, HexCoord playerPos) {
    if (!gm || !map) return;
    gm->enemyCount = 0;
    int maxInitialEnemies = 4;
    int attempts = 0;
    while (gm->enemyCount < maxInitialEnemies && attempts < 100) {
        int r = Rng_Range(&gm->rng, 0, map->tileCount - 1);
        if ((map->tiles[r].isRoad || map->tiles[r].structure.type != STRUCT_NONE) && 
            HexCoord_Distance(map->tiles[r].coord, playerPos) > 3) {
            EnemySquad e = NPC_CreateMapSquad(gm, map, map->tiles[r].coord);
            gm->enemies[gm->enemyCount++] = e;
        }
        attempts++;
    }
}

void NPC_ProcessTurn(struct GameMaster* gm, Map* map, Army* playerArmy) {
    if (!gm || !map || !playerArmy) return;
    const BalanceConfig* cfg = Balance_GetGlobal();

    // 1. Reabastecer escuadrones si hay menos de 3
    if (gm->enemyCount < 3) {
        int r = Rng_Range(&gm->rng, 0, map->tileCount - 1);
        if (map->tiles[r].isRoad || map->tiles[r].structure.type != STRUCT_NONE) {
            EnemySquad e = NPC_CreateMapSquad(gm, map, map->tiles[r].coord);
            gm->enemies[gm->enemyCount++] = e;
        }
    }

    // 2. Procesar cada escuadrÃ³n activo
    for (int i = 0; i < gm->enemyCount; i++) {
        EnemySquad* e = &gm->enemies[i];
        if (!e->isActive) continue;

        // A. Salto de fase tras huida
        if (e->skipNextPhase > 0) {
            e->skipNextPhase--;
            continue;
        }

        HexTile* myTile = Map_GetTile(map, e->position);
        if (!myTile) continue;

        int distToPlayer = HexCoord_Distance(e->position, playerArmy->position);

        // B. Culling de rendimiento: solo simular dentro de ActivityRadius (salvo Cazador)
        if (e->behavior != AI_HUNTER && distToPlayer > cfg->aiActivityRadius) {
            continue;
        }

        bool isBandit = NPC_IsBandit(e);
        bool isHostile = NPC_IsHostileToPlayer(e, gm);

        // C. Radio de detecciÃ³n segÃºn franja y tamaÃ±o del ejÃ©rcito
        int baseSense = (gm->timeOfDay == TIME_NIGHT) ? cfg->aiSenseRadiusNight : cfg->aiSenseRadiusDay;
        if (Army_GetTotalSoldiers(playerArmy) > 50) {
            baseSense += cfg->aiSenseRadiusArmySizeBonus;
        }

        int effectiveSense = baseSense;
        // Regla nocturna especial de Bandidos: no sufren reducciÃ³n nocturna, aumentan en +2
        if (isBandit && gm->timeOfDay == TIME_NIGHT) {
            effectiveSense = cfg->aiSenseRadiusDay + 2;
            if (Army_GetTotalSoldiers(playerArmy) > 50) {
                effectiveSense += cfg->aiSenseRadiusArmySizeBonus;
            }
        }

        HexTile* playerTile = Map_GetTile(map, playerArmy->position);
        int playerCover = Balance_GetTerrainCover(cfg, playerTile ? (int)playerTile->biome : 0,
                                                 playerTile ? (int)playerTile->structure.type : 0);
        bool playerInForest = playerTile && (playerTile->biome == BIOME_FOREST || playerTile->biome == BIOME_JUNGLE);

        bool detectsPlayer = false;
        if (gm->spentNightAtInn) {
            // El jugador está durmiendo protegido en la posada del asentamiento: inmune a detección
            detectsPlayer = false;
        } else if (!isHostile) {
            // NPCs pacíficos o aliados no persiguen al jugador
            detectsPlayer = false;
        } else if (e->behavior == AI_HUNTER) {
            int hunterSense = (e->senseRadius > 0) ? e->senseRadius : 5;
            if (playerInForest) {
                if (gm->isEncamped) {
                    // Acampado en bosque: follaje denso y campamento camuflado.
                    // Los cazadores solo lo detectan si están pegados (distancia <= 1, o 2 si ejército > 50)
                    int campSight = (Army_GetTotalSoldiers(playerArmy) > 50) ? 2 : 1;
                    detectsPlayer = (distToPlayer <= campSight);
                } else {
                    // En marcha por bosque: la espesura reduce el radio de rastreo a 2
                    detectsPlayer = (distToPlayer <= 2);
                }
            } else {
                // En terreno abierto o caminos: el cazador rastrea a su alcance completo
                detectsPlayer = (distToPlayer <= hunterSense);
            }
        } else if (e->behavior == AI_AMBUSH) {
            int ambSense = (isBandit && gm->timeOfDay == TIME_NIGHT) ? effectiveSense : 2;
            if (playerInForest && gm->isEncamped) ambSense = 1;
            detectsPlayer = (distToPlayer <= ambSense);
        } else if (e->behavior == AI_BEAST) {
            if (gm->timeOfDay != TIME_NIGHT) {
                e->aiState = AIS_RESTING;
                continue;
            }
            int bSense = effectiveSense + 1;
            if (playerInForest && gm->isEncamped) bSense = 1;
            detectsPlayer = (distToPlayer <= bSense);
        } else {
            int normSense = effectiveSense;
            if (playerInForest) {
                normSense -= (gm->isEncamped ? (playerCover + 1) : playerCover);
                if (normSense < 1) normSense = 1;
            }
            detectsPlayer = (distToPlayer <= normSense);
        }

        // D. Transiciones de la mÃ¡quina de estados
        if (detectsPlayer) {
            e->lastKnownPlayerPos = playerArmy->position;
            if (isBandit && gm->timeOfDay == TIME_NIGHT) {
                e->aiState = AIS_CHASING;
                e->currentIntent = INTENT_CHASE;
                e->chaseTurnsLeft = cfg->aiChaseTurns;
            } else if (e->aiState == AIS_IDLE || e->aiState == AIS_PATROLLING || 
                       e->aiState == AIS_RESTING || e->aiState == AIS_RETURNING) {
                e->aiState = AIS_ALERT;
                e->currentIntent = INTENT_CHASE;
                e->chaseTurnsLeft = cfg->aiChaseTurns;
            } else if (e->aiState == AIS_ALERT || e->aiState == AIS_SEARCHING) {
                e->aiState = AIS_CHASING;
                e->currentIntent = INTENT_CHASE;
                e->chaseTurnsLeft = cfg->aiChaseTurns;
            } else if (e->aiState == AIS_CHASING) {
                e->chaseTurnsLeft = cfg->aiChaseTurns;
            }
        } else {
            if (e->aiState == AIS_CHASING) {
                e->chaseTurnsLeft--;
                if (e->chaseTurnsLeft <= 0) {
                    e->aiState = AIS_SEARCHING;
                    e->patrolWaitTurns = 1;
                }
            } else if (e->aiState == AIS_ALERT) {
                e->aiState = AIS_SEARCHING;
                e->patrolWaitTurns = 1;
            } else if (e->aiState == AIS_SEARCHING) {
                if (e->patrolWaitTurns > 0) {
                    e->patrolWaitTurns--;
                } else {
                    e->aiState = AIS_RETURNING;
                    e->currentIntent = INTENT_PATROL;
                }
            } else if (e->aiState == AIS_IDLE) {
                e->aiState = AIS_PATROLLING;
                e->currentIntent = INTENT_PATROL;
            }
        }

        // Respetar correa de guardia (leash)
        if (e->behavior == AI_GUARD) {
            int leashLimit = (e->leash > 0) ? e->leash : 3;
            int distHome = HexCoord_Distance(e->position, e->homeHex);
            if (distHome > leashLimit) {
                e->aiState = AIS_RETURNING;
                e->currentIntent = INTENT_PATROL;
            }
        }

        // E. Determinar casilla objetivo
        HexCoord targetCoord = e->position;
        bool shouldMove = true;

        if (e->aiState == AIS_CHASING || e->aiState == AIS_ALERT) {
            targetCoord = detectsPlayer ? playerArmy->position : e->lastKnownPlayerPos;
        } else if (e->aiState == AIS_SEARCHING) {
            targetCoord = e->lastKnownPlayerPos;
        } else if (e->aiState == AIS_RETURNING) {
            targetCoord = e->homeHex;
            if (e->position.q == e->homeHex.q && e->position.r == e->homeHex.r) {
                e->aiState = AIS_PATROLLING;
                e->currentIntent = INTENT_PATROL;
                e->patrolTarget = NPC_PickPatrolTarget(gm, map, e);
                targetCoord = e->patrolTarget;
            }
        } else if (e->aiState == AIS_PATROLLING || e->aiState == AIS_IDLE) {
            e->aiState = AIS_PATROLLING;
            e->currentIntent = INTENT_PATROL;

            if (e->behavior == AI_HUNTER) {
                if (detectsPlayer) {
                    targetCoord = playerArmy->position;
                } else {
                    targetCoord = e->lastKnownPlayerPos;
                    if (e->position.q == e->lastKnownPlayerPos.q && e->position.r == e->lastKnownPlayerPos.r) {
                        e->aiState = AIS_SEARCHING;
                        e->patrolWaitTurns = 2;
                        e->patrolTarget = NPC_PickPatrolTarget(gm, map, e);
                        targetCoord = e->patrolTarget;
                    }
                }
            } else if (e->behavior == AI_BEAST) {
                if (gm->timeOfDay != TIME_NIGHT) {
                    shouldMove = false;
                } else {
                    if (HexCoord_Distance(e->position, e->patrolTarget) == 0) {
                        e->patrolTarget = NPC_PickPatrolTarget(gm, map, e);
                    }
                    targetCoord = e->patrolTarget;
                }
            } else if (e->routeLength > 0) {
                targetCoord = e->route[e->routeIndex];
                if (e->position.q == targetCoord.q && e->position.r == targetCoord.r) {
                    e->routeIndex = (e->routeIndex + 1) % e->routeLength;
                    targetCoord = e->route[e->routeIndex];
                }
            } else {
                if (HexCoord_Distance(e->position, e->patrolTarget) == 0) {
                    e->patrolTarget = NPC_PickPatrolTarget(gm, map, e);
                }
                targetCoord = e->patrolTarget;
            }
        } else {
            shouldMove = false;
        }

        // F. Movimiento con presupuesto de PA y pathfinding
        if (shouldMove && (targetCoord.q != e->position.q || targetCoord.r != e->position.r)) {
            HexTile* startTile = Map_GetTile(map, e->position);
            HexTile* endTile = Map_GetTile(map, targetCoord);
            if (startTile && endTile) {
                HexTile* path[128];
                int totalCost = 0;
                int pathLen = Map_FindPathEnemy(map, startTile, endTile, &totalCost, path);
                int apBudget = e->mobility;
                if (apBudget <= 0) apBudget = 10;

                if (pathLen > 0) {
                    for (int step = 0; step < pathLen; step++) {
                        HexTile* nextTile = path[step];
                        if (!nextTile) break;
                        if ((nextTile->terrain == TERRAIN_WATER || nextTile->biome == BIOME_WATER) && 
                            nextTile->bridge == BRIDGE_NONE) {
                            break;
                        }

                        int stepCost = Biome_GetProperties(nextTile->biome).actionPointCost;
                        if (nextTile->isRoad) stepCost = (stepCost > 1) ? stepCost - 1 : 1;
                        if (stepCost < 1) stepCost = 1;

                        // Si la tropa no puede atacar al jugador (o si duerme en posada), no ocupar su casilla
                        if ((!isHostile || gm->spentNightAtInn) && nextTile->coord.q == playerArmy->position.q && nextTile->coord.r == playerArmy->position.r) {
                            break;
                        }

                        // Evitar apilarse sobre otro escuadron NPC activo
                        bool tileOccupied = false;
                        for (int o = 0; o < gm->enemyCount; o++) {
                            if (o == i) continue;
                            if (gm->enemies[o].isActive && 
                                gm->enemies[o].position.q == nextTile->coord.q && 
                                gm->enemies[o].position.r == nextTile->coord.r) {
                                tileOccupied = true;
                                break;
                            }
                        }
                        if (tileOccupied) break;

                        if (apBudget >= stepCost) {
                            e->position = nextTile->coord;
                            apBudget -= stepCost;

                            // Contacto durante la marcha (si duerme en la posada, está a salvo de ataques)
                            if (!gm->spentNightAtInn && HexCoord_Distance(e->position, playerArmy->position) <= 1) {
                                if (isHostile) {
                                    if (gm->pendingCombatEnemy < 0) {
                                        gm->pendingCombatEnemy = i;
                                        e->initiator = 1;
                                        gm->activeCombat.initiator = 1;
                                        gm->activeCombat.ambushed = (e->behavior == AI_AMBUSH || Enemy_HasTag(e, TAG_AMBUSH));
                                    }
                                    break;
                                }
                            }
                        } else {
                            break;
                        }
                    }
                } else {
                    // Fallback directo: elegir el vecino transitable que mas aproxime a targetCoord
                    HexCoord bestNeighbor = e->position;
                    int bestDist = HexCoord_Distance(e->position, targetCoord);
                    for (int dir = 0; dir < 6; dir++) {
                        HexTile* nb = startTile->neighbors[dir];
                        if (!nb) continue;
                        if ((nb->terrain == TERRAIN_WATER || nb->biome == BIOME_WATER) && nb->bridge == BRIDGE_NONE) continue;
                        if ((!isHostile || gm->spentNightAtInn) && nb->coord.q == playerArmy->position.q && nb->coord.r == playerArmy->position.r) continue;

                        bool occ = false;
                        for (int o = 0; o < gm->enemyCount; o++) {
                            if (o == i) continue;
                            if (gm->enemies[o].isActive && gm->enemies[o].position.q == nb->coord.q && gm->enemies[o].position.r == nb->coord.r) {
                                occ = true; break;
                            }
                        }
                        if (occ) continue;

                        int d = HexCoord_Distance(nb->coord, targetCoord);
                        if (d < bestDist) {
                            bestDist = d;
                            bestNeighbor = nb->coord;
                        }
                    }
                    if (bestNeighbor.q != e->position.q || bestNeighbor.r != e->position.r) {
                        e->position = bestNeighbor;
                        if (!gm->spentNightAtInn && HexCoord_Distance(e->position, playerArmy->position) <= 1 && isHostile) {
                            if (gm->pendingCombatEnemy < 0) {
                                gm->pendingCombatEnemy = i;
                                e->initiator = 1;
                                gm->activeCombat.initiator = 1;
                                gm->activeCombat.ambushed = (e->behavior == AI_AMBUSH || Enemy_HasTag(e, TAG_AMBUSH));
                            }
                        }
                    }
                }
            }
        }

        // G. Verificación final de contacto post-movimiento
        if (!gm->spentNightAtInn && isHostile && gm->pendingCombatEnemy < 0 && HexCoord_Distance(e->position, playerArmy->position) <= 1) {
            gm->pendingCombatEnemy = i;
            e->initiator = 1;
            gm->activeCombat.initiator = 1;
            gm->activeCombat.ambushed = (e->behavior == AI_AMBUSH || Enemy_HasTag(e, TAG_AMBUSH));
        }
    }
}

bool NPC_IsPlayerInZOC(const struct GameMaster* gm, HexCoord playerPos) {
    if (!gm) return false;
    const BalanceConfig* cfg = Balance_GetGlobal();
    if (!cfg || !cfg->aiZocEnabled) return false;

    for (int i = 0; i < gm->enemyCount; i++) {
        if (gm->enemies[i].isActive && HexCoord_Distance(playerPos, gm->enemies[i].position) == 1) {
            if (NPC_IsHostileToPlayer(&gm->enemies[i], gm)) {
                return true;
            }
        }
    }
    return false;
}

int NPC_FindHostileSquadAt(const struct GameMaster* gm, HexCoord pos) {
    if (!gm) return -1;
    for (int i = 0; i < gm->enemyCount; i++) {
        if (gm->enemies[i].isActive && 
            gm->enemies[i].position.q == pos.q && 
            gm->enemies[i].position.r == pos.r) {
            if (NPC_IsHostileToPlayer(&gm->enemies[i], gm)) {
                return i;
            }
        }
    }
    return -1;
}

int NPC_FindAttackableSquadNear(const struct GameMaster* gm, HexCoord playerPos) {
    if (!gm) return -1;
    for (int i = 0; i < gm->enemyCount; i++) {
        if (gm->enemies[i].isActive && 
            HexCoord_Distance(playerPos, gm->enemies[i].position) <= 1) {
            return i;
        }
    }
    return -1;
}

void NPC_InitiateCombat(struct GameMaster* gm, Army* playerArmy, EnemySquad* npc, int initiator, bool ambushed) {
    if (!gm || !playerArmy || !npc) return;

    gm->inCombat = true;
    gm->phase = PHASE_EVENT;
    npc->initiator = initiator;
    gm->activeCombat.initiator = initiator;
    gm->activeCombat.ambushed = ambushed;

    Combat_Init(&gm->activeCombat, gm, playerArmy, npc);

    if (npc->lootMultiplier > 0.0f) {
        gm->activeCombat.lootMultiplier = npc->lootMultiplier;
    }
}

void NPC_PlayerVoluntaryAttack(struct GameMaster* gm, Army* playerArmy, int npcIndex) {
    if (!gm || !playerArmy) return;
    if (npcIndex < 0 || npcIndex >= gm->enemyCount) return;
    EnemySquad* npc = &gm->enemies[npcIndex];
    if (!npc->isActive) return;

    // Si atacamos voluntariamente a un escuadrÃ³n que no era hostil, penalizar opiniÃ³n e infamia
    if (!NPC_IsHostileToPlayer(npc, gm) && npc->factionId >= 0 && npc->factionId < 3) {
        Faction_ModifyOpinion(&gm->factions[npc->factionId], -25);
        gm->infamy += 10.0f;
        if (gm->infamy > 100.0f) gm->infamy = 100.0f;
        TraceLog(LOG_WARNING, "[NPC] Ataque voluntario a faccion %s: Opinion -25, Infamia +10", 
                 gm->factions[npc->factionId].name);
    }

    Audio_PlaySfxByName("enemy_alert");
    bool ambushed = (npc->behavior == AI_AMBUSH || Enemy_HasTag(npc, TAG_AMBUSH));
    NPC_InitiateCombat(gm, playerArmy, npc, 0, ambushed);
}
