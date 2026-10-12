#include "controllers/game_master.h"
#include "models/balance_config.h"
#include <stdio.h>
#include "raylib.h"
#include <string.h>
#include <math.h>

void GameMaster_Init(GameMaster* gm)
 {
    if (!gm) return;
    Rng_Init(&gm->rng, 1337);
    DayClock_Init(&gm->clock, 1, TIME_MORNING);
    gm->currentTurn = 1;
    gm->currentDay = gm->clock.currentDay;
    gm->timeOfDay = gm->clock.timeOfDay;
    gm->phase = PHASE_PLAYER;
    gm->infamy = 0.0f;
    gm->baseActionPoints = 3;
    gm->maxActionPoints = 15;
    gm->currentActionPoints = 0;
    gm->hasRolledThisTurn = false;
    gm->eventTimer = 0.0f;
    gm->hasDayReport = false;
    gm->showCampDecision = false;
    gm->isEncamped = false;
    gm->hasForagedToday = false;
    memset(&gm->lastDayReport, 0, sizeof(DayReport));
    gm->eventCount = Event_LoadAll("assets/events", gm->events, 64);
    gm->resultCount = Result_LoadAll("assets/results", gm->results, 128);
    // Tambien puede haber resultados en la carpeta de eventos
    gm->resultCount += Result_LoadAll("assets/events", &gm->results[gm->resultCount], 128 - gm->resultCount);
    gm->activeEvent = NULL;

    gm->modRegistry.count = Modifier_LoadAll("assets/modifiers", gm->modRegistry.defs, MAX_MODIFIERS);
    TraceLog(LOG_INFO, "[MOD] %d modificadores cargados", gm->modRegistry.count);
    for (int i = 0; i < gm->modRegistry.count; i++) {
        TraceLog(LOG_INFO, "  - %s", gm->modRegistry.defs[i].id);
    }

    gm->inCombat = false;
    gm->pendingCombatEnemy = -1;
    gm->enemyCount = 0;
    gm->lastHunterSpawnDay = 0;
    gm->lastCampPos = (HexCoord){0, 0};
    gm->canInteractStructure = false;
    gm->interactStructureCoord = (HexCoord){0, 0};
    memset(&gm->activeCity, 0, sizeof(CityContext));
    gm->inSiegeCombat = false;
    gm->siegeTargetCoord = (HexCoord){0, 0};
    gm->siegeFactionId = 0;
    GameMaster_InitFactions(gm);
}

void GameMaster_StartTurn(GameMaster* gm, Army* playerArmy) {
    if (!gm || !playerArmy) return;
    gm->phase = PHASE_PLAYER;
    gm->currentActionPoints = 0;
    gm->hasRolledThisTurn = false;
    gm->eventTimer = 0.0f;
}

#include <stdlib.h>
#include "models/map_generator.h"

void GameMaster_ProcessEnemyTurn(GameMaster* gm, Map* map, Army* playerArmy) {
    if (!gm || !map || !playerArmy) return;
    const BalanceConfig* cfg = Balance_GetGlobal();
    
    // 1. Reabastecer enemigos si hay menos de 3 (usando rng con semilla para determinismo)
    if (gm->enemyCount < 3) {
        int r = Rng_Range(&gm->rng, 0, map->tileCount - 1);
        if (map->tiles[r].isRoad || map->tiles[r].structure.type != STRUCT_NONE) {
            EnemySquad e;
            int type = Rng_Range(&gm->rng, 0, (gm->timeOfDay == TIME_NIGHT) ? 4 : 3);
            if (type == 0) {
                e = Enemy_CreateWithBehavior("Milicia Rebelde", map->tiles[r].coord, 20 + (gm->currentDay * 3), 10 + gm->currentDay, 15, 0, 0, AI_PATROL);
                Enemy_AddTag(&e, TAG_DISORGANIZED);
            } else if (type == 1) {
                e = Enemy_CreateWithBehavior("Guardia de Hierro", map->tiles[r].coord, 40 + (gm->currentDay * 6), 5 + gm->currentDay, 5, 20, 5, AI_GUARD);
                Enemy_SetGuardLeash(&e, 4);
                Enemy_AddTag(&e, TAG_HEAVILY_ARMORED);
            } else if (type == 2) {
                e = Enemy_CreateWithBehavior("Incursores a Caballo", map->tiles[r].coord, 30 + (gm->currentDay * 4), 20 + gm->currentDay, 5, 10, 0, AI_AMBUSH);
                Enemy_AddTag(&e, TAG_CAVALRY);
                Enemy_AddTag(&e, TAG_AMBUSH);
            } else if (type == 3) {
                e = Enemy_CreateWithBehavior("Mercenarios Veteranos", map->tiles[r].coord, 50 + (gm->currentDay * 5), 12 + gm->currentDay, 0, 30, 10, AI_PATROL);
            } else {
                e = Enemy_CreateWithBehavior("Manada de Bestias", map->tiles[r].coord, 25 + (gm->currentDay * 3), 14, 20, 0, 0, AI_BEAST);
            }
            e.originalSpawn = e.position;
            e.homeHex = e.position;
            e.patrolTarget = e.position;
            e.patrolWaitTurns = 0;
            gm->enemies[gm->enemyCount++] = e;
        }
    }

    // 2. Procesar cada escuadrón enemigo activo
    for (int i = 0; i < gm->enemyCount; i++) {
        EnemySquad* e = &gm->enemies[i];
        if (!e->isActive) continue;

        // A. Salto de fase tras huida (8.4)
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

        // C. Radio de detección según franja y tamaño del ejército (8.3)
        int baseSense = (gm->timeOfDay == TIME_NIGHT) ? cfg->aiSenseRadiusNight : cfg->aiSenseRadiusDay;
        if (Army_GetTotalSoldiers(playerArmy) > 50) {
            baseSense += cfg->aiSenseRadiusArmySizeBonus;
        }
        int effectiveSense = (e->senseRadius > 0) ? e->senseRadius : baseSense;

        bool detectsPlayer = false;
        if (e->behavior == AI_HUNTER) {
            detectsPlayer = true; // El cazador rastrea al jugador de forma global
        } else if (e->behavior == AI_AMBUSH) {
            detectsPlayer = (distToPlayer <= 2); // Emboscador se activa a <= 2 hexágonos
        } else if (e->behavior == AI_BEAST) {
            if (gm->timeOfDay != TIME_NIGHT) {
                e->aiState = AIS_RESTING; // Bestias descansan de día
                continue;
            }
            detectsPlayer = (distToPlayer <= effectiveSense + 1);
        } else {
            detectsPlayer = (distToPlayer <= effectiveSense);
        }

        // D. Transiciones de la máquina de estados
        if (detectsPlayer) {
            e->lastKnownPlayerPos = playerArmy->position;
            if (e->aiState == AIS_IDLE || e->aiState == AIS_PATROLLING || e->aiState == AIS_RESTING || e->aiState == AIS_RETURNING) {
                e->aiState = AIS_ALERT;
                e->currentIntent = INTENT_CHASE;
                e->chaseTurnsLeft = cfg->aiChaseTurns;
            } else if (e->aiState == AIS_ALERT || e->aiState == AIS_SEARCHING) {
                e->aiState = AIS_CHASING;
                e->currentIntent = INTENT_CHASE;
                e->chaseTurnsLeft = cfg->aiChaseTurns;
            } else if (e->aiState == AIS_CHASING) {
                e->chaseTurnsLeft = cfg->aiChaseTurns; // Refrescar contador al ver al jugador
            }
        } else {
            if (e->aiState == AIS_CHASING) {
                e->chaseTurnsLeft--;
                if (e->chaseTurnsLeft <= 0) {
                    e->aiState = AIS_SEARCHING;
                    e->patrolWaitTurns = 2; // Buscar durante 2 turnos
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
            }
        }

        // Comportamiento Guardia: respetar correa (leash)
        if (e->behavior == AI_GUARD) {
            int leashLimit = (e->leash > 0) ? e->leash : 4;
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
                if (e->behavior == AI_PATROL) {
                    e->aiState = AIS_PATROLLING;
                } else {
                    e->aiState = AIS_IDLE;
                    e->currentIntent = INTENT_NONE;
                }
                shouldMove = false;
            }
        } else if (e->aiState == AIS_PATROLLING) {
            if (e->behavior == AI_PATROL) {
                if (e->routeLength > 0) {
                    targetCoord = e->route[e->routeIndex];
                    if (e->position.q == targetCoord.q && e->position.r == targetCoord.r) {
                        e->routeIndex = (e->routeIndex + 1) % e->routeLength;
                        targetCoord = e->route[e->routeIndex];
                    }
                } else {
                    if (e->patrolWaitTurns > 0) {
                        e->patrolWaitTurns--;
                        shouldMove = false;
                    } else if (e->position.q == e->patrolTarget.q && e->position.r == e->patrolTarget.r) {
                        for (int tries = 0; tries < 10; tries++) {
                            int rTile = Rng_Range(&gm->rng, 0, map->tileCount - 1);
                            if (HexCoord_Distance(map->tiles[rTile].coord, e->originalSpawn) <= 5 &&
                                map->tiles[rTile].biome != BIOME_WATER && map->tiles[rTile].terrain != TERRAIN_WATER) {
                                e->patrolTarget = map->tiles[rTile].coord;
                                break;
                            }
                        }
                        e->patrolWaitTurns = 1;
                        shouldMove = false;
                    } else {
                        targetCoord = e->patrolTarget;
                    }
                }
            } else {
                shouldMove = false;
            }
        } else {
            shouldMove = false;
        }

        // F. Movimiento con presupuesto estricto de PA (mobility) y pathfinding
        if (shouldMove && (targetCoord.q != e->position.q || targetCoord.r != e->position.r)) {
            HexTile* startTile = Map_GetTile(map, e->position);
            HexTile* endTile = Map_GetTile(map, targetCoord);
            if (startTile && endTile) {
                HexTile* path[128];
                int totalCost = 0;
                int pathLen = Map_FindPathEnemy(map, startTile, endTile, &totalCost, path);
                if (pathLen > 0) {
                    int apBudget = e->mobility;
                    for (int step = 0; step < pathLen; step++) {
                        HexTile* nextTile = path[step];
                        // Garantía absoluta de nunca entrar a agua o casillas intransitables
                        if ((nextTile->terrain == TERRAIN_WATER || nextTile->biome == BIOME_WATER) && nextTile->bridge == BRIDGE_NONE) {
                            break;
                        }

                        int stepCost = Biome_GetProperties(nextTile->biome).actionPointCost;
                        if (nextTile->isRoad) stepCost = (stepCost > 1) ? stepCost - 1 : 1;

                        if (apBudget >= stepCost) {
                            e->position = nextTile->coord;
                            apBudget -= stepCost;

                            // Contacto durante la marcha (distancia <= 1 o misma casilla)
                            if (HexCoord_Distance(e->position, playerArmy->position) <= 1) {
                                if (gm->pendingCombatEnemy < 0) {
                                    gm->pendingCombatEnemy = i;
                                    e->initiator = 1;
                                    gm->activeCombat.initiator = 1;
                                    gm->activeCombat.ambushed = (e->behavior == AI_AMBUSH || Enemy_HasTag(e, TAG_AMBUSH));
                                }
                                break; // Detener movimiento al entrar en contacto
                            }
                        } else {
                            break; // Agotado el presupuesto de PA para este turno
                        }
                    }
                }
            }
        }

        // G. Verificación final de contacto post-movimiento
        if (gm->pendingCombatEnemy < 0 && HexCoord_Distance(e->position, playerArmy->position) <= 1) {
            gm->pendingCombatEnemy = i;
            e->initiator = 1;
            gm->activeCombat.initiator = 1;
            gm->activeCombat.ambushed = (e->behavior == AI_AMBUSH || Enemy_HasTag(e, TAG_AMBUSH));
        }
    }
}

void GameMaster_EndTurn(GameMaster* gm, Map* map, Army* playerArmy) {
    if (!gm || !map || !playerArmy) return;

    /* Decisión de la Tarde (7.2): Al terminar la Tarde se muestra la opción de acampar o seguir marchando */
    if (gm->timeOfDay == TIME_AFTERNOON && !gm->showCampDecision) {
        gm->showCampDecision = true;
        return;
    }

    /* 1. Avanzar franja horaria a través de DayClock (I4) */
    DayClock_AdvanceSlot(&gm->clock);
    gm->timeOfDay = gm->clock.timeOfDay;
    gm->currentDay = gm->clock.currentDay;
    gm->currentTurn = gm->clock.totalTurnsElapsed + 1;
    gm->isEncamped = false;

    /* 2. Si terminó el día (4 franjas), ejecutar cierre de día y escalera de supervivencia (I7, E1) */
    if (DayClock_HasDayEnded(&gm->clock)) {
        Army_EndDay(playerArmy, &gm->lastDayReport, &gm->rng);
        gm->hasDayReport = true;
        gm->hasForagedToday = false; /* Reset para el nuevo día */
        gm->infamy += 1.0f; /* Sube la infamia levemente al pasar un día */
        if (gm->infamy > 100.0f) gm->infamy = 100.0f;

        for (int f = 0; f < 3; f++) {
            Faction_TickDay(&gm->factions[f]);
        }

        /* Spawn periódico de Cazador (8.5) */
        const BalanceConfig* cfg = Balance_GetGlobal();
        if (cfg->hunterEnabled && cfg->hunterSpawnEveryDays > 0) {
            if (gm->currentDay >= cfg->hunterSpawnEveryDays && 
               (gm->currentDay - gm->lastHunterSpawnDay) >= cfg->hunterSpawnEveryDays) {
                HexCoord spawnPos = gm->lastCampPos;
                if (spawnPos.q == 0 && spawnPos.r == 0) {
                    for (int k = 0; k < map->tileCount; k++) {
                        if (map->tiles[k].special == TILE_SPECIAL_START) {
                            spawnPos = map->tiles[k].coord;
                            break;
                        }
                    }
                }
                if (gm->enemyCount < 64) {
                    int hunterMob = (int)roundf(Army_CalculateMobility(playerArmy) * cfg->hunterMobilityMult);
                    if (hunterMob < 4) hunterMob = 4;
                    EnemySquad hunter = Enemy_CreateWithBehavior(
                        "Cazador Rastreador",
                        spawnPos,
                        35 + (gm->currentDay * 4),
                        hunterMob,
                        0, 20, 5,
                        AI_HUNTER
                    );
                    hunter.aiState = AIS_CHASING;
                    hunter.currentIntent = INTENT_CHASE;
                    hunter.lastKnownPlayerPos = playerArmy->position;
                    hunter.chaseTurnsLeft = 999;
                    gm->enemies[gm->enemyCount++] = hunter;
                    gm->lastHunterSpawnDay = gm->currentDay;
                }
            }
        }
    }

    if (gm->currentDay > 30) {
        gm->phase = PHASE_VICTORY;
        return;
    }

    // Fase del enemigo
    gm->phase = PHASE_ENEMY;
    GameMaster_ProcessEnemyTurn(gm, map, playerArmy);

    // Volver al jugador
    GameMaster_StartTurn(gm, playerArmy);
}

void GameMaster_ChooseCamp(GameMaster* gm, Map* map, Army* playerArmy) {
    if (!gm || !map || !playerArmy) return;
    const BalanceConfig* cfg = Balance_GetGlobal();
    
    gm->showCampDecision = false;
    playerArmy->consecutiveNightMarches = 0;
    gm->lastCampPos = playerArmy->position;

    /* 1. Recuperar moral y salud si no sufre hambruna/sed severa desde el día 2+ (7.3) */
    bool starvingNoRec = cfg->campStarvingNoRecovery && (playerArmy->daysWithoutFood >= 2 || playerArmy->daysWithoutWater >= 2);
    if (!starvingNoRec) {
        playerArmy->moral += cfg->campRecoverMoral;
        if (playerArmy->moral > 100.0f) playerArmy->moral = 100.0f;
        playerArmy->salud += cfg->campRecoverSalud;
        if (playerArmy->salud > 100.0f) playerArmy->salud = 100.0f;
    }

    /* 2. Tirada de emboscada: 2d6 + Evaluación + cobertura (7.3) */
    HexTile* tile = Map_GetTile(map, playerArmy->position);
    int cover = Balance_GetTerrainCover(cfg, tile ? (int)tile->biome : 0, tile ? (int)tile->structure.type : 0);
    
    int evalStat = (int)Army_CalculateEvaluation(playerArmy);
    ModContext mCtx = { &gm->modRegistry, playerArmy, NULL, NULL, gm->timeOfDay, gm->infamy };
    evalStat = Modifier_Query(HOOK_AMBUSH_ROLL, EFFECT_EVALUATION, EFFECT_SIDE_SELF, &mCtx, evalStat, NULL);

    int diceRoll = Rng_RollDice(&gm->rng, 2, 6);
    int totalRoll = (cover >= 90) ? 99 : (diceRoll + evalStat + cover);

    bool ambushTriggered = (totalRoll < cfg->campAmbushTarget);

    /* 3. Consumo de la noche en campamento */
    Army_ConsumeTurnResources(playerArmy);

    /* 4. Avanzar reloj: la noche transcurre en el campamento hasta la mañana siguiente */
    DayClock_AdvanceSlot(&gm->clock); /* Avanza a NIGHT */
    DayClock_AdvanceSlot(&gm->clock); /* Cierra el día y pasa a MORNING (Dawn) */
    gm->timeOfDay = gm->clock.timeOfDay;
    gm->currentDay = gm->clock.currentDay;
    gm->currentTurn = gm->clock.totalTurnsElapsed + 1;

    /* Al despertar por la mañana, el campamento se levanta: isEncamped = false */
    gm->isEncamped = false;

    /* 5. Ejecutar cierre de día (I7) */
    Army_EndDay(playerArmy, &gm->lastDayReport, &gm->rng);
    gm->hasDayReport = true;
    gm->hasForagedToday = false; /* Reset para el nuevo día */
    gm->infamy += 1.0f;
    if (gm->infamy > 100.0f) gm->infamy = 100.0f;

    for (int f = 0; f < 3; f++) {
        Faction_TickDay(&gm->factions[f]);
    }

    /* Spawn periódico de Cazador tras campamento (8.5) */
    if (cfg->hunterEnabled && cfg->hunterSpawnEveryDays > 0) {
        if (gm->currentDay >= cfg->hunterSpawnEveryDays && 
           (gm->currentDay - gm->lastHunterSpawnDay) >= cfg->hunterSpawnEveryDays) {
            HexCoord spawnPos = gm->lastCampPos;
            if (spawnPos.q == 0 && spawnPos.r == 0) {
                for (int k = 0; k < map->tileCount; k++) {
                    if (map->tiles[k].special == TILE_SPECIAL_START) {
                        spawnPos = map->tiles[k].coord;
                        break;
                    }
                }
            }
            if (gm->enemyCount < 64) {
                int hunterMob = (int)roundf(Army_CalculateMobility(playerArmy) * cfg->hunterMobilityMult);
                if (hunterMob < 4) hunterMob = 4;
                EnemySquad hunter = Enemy_CreateWithBehavior(
                    "Cazador Rastreador",
                    spawnPos,
                    35 + (gm->currentDay * 4),
                    hunterMob,
                    0, 20, 5,
                    AI_HUNTER
                );
                hunter.aiState = AIS_CHASING;
                hunter.currentIntent = INTENT_CHASE;
                hunter.lastKnownPlayerPos = playerArmy->position;
                hunter.chaseTurnsLeft = 999;
                gm->enemies[gm->enemyCount++] = hunter;
                gm->lastHunterSpawnDay = gm->currentDay;
            }
        }
    }

    /* 6. Si hubo emboscada, disparar evento CAMP_AMBUSH */
    if (ambushTriggered) {
        EventContext ctx;
        memset(&ctx, 0, sizeof(EventContext));
        ctx.timeOfDay = TIME_NIGHT;
        ctx.biome = tile ? (int)tile->biome : 0;
        ctx.infamy = gm->infamy;
        ctx.gold = playerArmy->monedas;
        ctx.food = (int)playerArmy->comida_disponible;
        ctx.water = (int)playerArmy->agua_disponible;
        ctx.armySize = Army_GetTotalSoldiers(playerArmy);
        ctx.trigger = TRIGGER_CAMP_AMBUSH;

        EventDef* ambEvt = GameMaster_FindTriggeredEvent(gm, &ctx);
        if (ambEvt) {
            gm->phase = PHASE_EVENT;
            gm->activeEvent = ambEvt;
            gm->eventTimer = 0.0f;
        }
    }

    // Procesar turno de enemigos durante la noche
    GameMaster_ProcessEnemyTurn(gm, map, playerArmy);

    // Reiniciar turno matutino del jugador (Dawn)
    GameMaster_StartTurn(gm, playerArmy);
}

void GameMaster_ChooseNightMarch(GameMaster* gm, Map* map, Army* playerArmy) {
    if (!gm || !map || !playerArmy) return;
    const BalanceConfig* cfg = Balance_GetGlobal();

    gm->showCampDecision = false;
    gm->isEncamped = false;
    playerArmy->consecutiveNightMarches++;

    /* 1. Avanzar a franja NIGHT */
    DayClock_AdvanceSlot(&gm->clock);
    gm->timeOfDay = gm->clock.timeOfDay;
    gm->currentDay = gm->clock.currentDay;
    gm->currentTurn = gm->clock.totalTurnsElapsed + 1;

    /* 2. Penalidades nocturnas por turno (7.4) */
    float penaltyMult = 1.0f + (playerArmy->consecutiveNightMarches - 1) * cfg->nightConsecutiveMult;
    playerArmy->moral += cfg->nightMoralPerTurn * penaltyMult;
    if (playerArmy->moral < 0.0f) playerArmy->moral = 0.0f;
    playerArmy->salud += cfg->nightSaludPerTurn * penaltyMult;
    if (playerArmy->salud < 0.0f) playerArmy->salud = 0.0f;

    /* 3. Tirada de encuentro nocturno (7.4) */
    int encRoll = Rng_Range(&gm->rng, 1, 100);
    ModContext mCtx = { &gm->modRegistry, playerArmy, NULL, NULL, gm->timeOfDay, gm->infamy };
    int encChance = Modifier_Query(HOOK_NIGHT_ENCOUNTER_CHANCE, EFFECT_MORAL, EFFECT_SIDE_SELF, &mCtx, cfg->nightEncounterChance, NULL);

    if (encRoll <= encChance) {
        HexTile* tile = Map_GetTile(map, playerArmy->position);
        EventContext ctx;
        memset(&ctx, 0, sizeof(EventContext));
        ctx.timeOfDay = TIME_NIGHT;
        ctx.biome = tile ? (int)tile->biome : 0;
        ctx.infamy = gm->infamy;
        ctx.gold = playerArmy->monedas;
        ctx.food = (int)playerArmy->comida_disponible;
        ctx.water = (int)playerArmy->agua_disponible;
        ctx.armySize = Army_GetTotalSoldiers(playerArmy);
        ctx.trigger = TRIGGER_NIGHT;

        EventDef* nightEvt = GameMaster_FindTriggeredEvent(gm, &ctx);
        if (nightEvt) {
            gm->phase = PHASE_EVENT;
            gm->activeEvent = nightEvt;
            gm->eventTimer = 0.0f;
        }
    }

    // Fase del enemigo
    gm->phase = PHASE_ENEMY;
    GameMaster_ProcessEnemyTurn(gm, map, playerArmy);

    // Iniciar turno nocturno del jugador
    GameMaster_StartTurn(gm, playerArmy);
}

bool GameMaster_Forage(GameMaster* gm, Army* army, Map* map) {
    if (!gm || !army || !map) return false;
    const BalanceConfig* cfg = Balance_GetGlobal();
    if (!cfg->forageEnabled) return false;
    if (gm->hasForagedToday) return false;
    if (gm->timeOfDay == TIME_NIGHT) return false;

    HexTile* tile = Map_GetTile(map, army->position);
    if (!tile) return false;

    int totalSoldiers = Army_GetTotalSoldiers(army);
    float dailyFoodUsage = Army_CalculateFoodUsage(totalSoldiers, army->ganado);
    float dailyWaterUsage = Army_CalculateWaterUsage(totalSoldiers, army->ganado);

    /* Tirada 2d6 + Evaluación (6.5) */
    int diceRoll = Rng_RollDice(&gm->rng, 2, 6);
    int evalStat = (int)Army_CalculateEvaluation(army);
    int totalRoll = diceRoll + evalStat;

    float mult = 1.0f;
    if (totalRoll < 6) mult = 0.5f;
    else if (totalRoll >= 10) mult = 1.5f;

    int bIdx = (int)tile->biome;
    if (bIdx < 0 || bIdx > 5) bIdx = 0;

    float gainedFood = dailyFoodUsage * cfg->forageFood[bIdx] * mult;
    float gainedWater = dailyWaterUsage * cfg->forageWater[bIdx] * mult;

    army->comida_disponible += gainedFood;
    army->agua_disponible += gainedWater;

    gm->hasForagedToday = true;
    gm->currentActionPoints = 0;

    /* Al forrajear se pasa directamente a la tarde; si ya es de tarde, abre la decisión nocturna obligatoria */
    if (gm->timeOfDay == TIME_MORNING) {
        DayClock_AdvanceSlot(&gm->clock); /* A Midday */
        DayClock_AdvanceSlot(&gm->clock); /* A Afternoon */
        gm->timeOfDay = gm->clock.timeOfDay;
        gm->currentTurn = gm->clock.totalTurnsElapsed + 1;
        Army_ConsumeTurnResources(army);
        Army_ConsumeTurnResources(army);
        gm->phase = PHASE_ENEMY;
        GameMaster_ProcessEnemyTurn(gm, map, army);
        GameMaster_StartTurn(gm, army);
    } else if (gm->timeOfDay == TIME_MIDDAY) {
        DayClock_AdvanceSlot(&gm->clock); /* A Afternoon */
        gm->timeOfDay = gm->clock.timeOfDay;
        gm->currentTurn = gm->clock.totalTurnsElapsed + 1;
        Army_ConsumeTurnResources(army);
        gm->phase = PHASE_ENEMY;
        GameMaster_ProcessEnemyTurn(gm, map, army);
        GameMaster_StartTurn(gm, army);
    } else if (gm->timeOfDay == TIME_AFTERNOON) {
        Army_ConsumeTurnResources(army);
        gm->showCampDecision = true;
    }

    return true;
}

static bool Event_MatchesContext(const EventDef* event, const EventContext* context) {
    if (event->hasTriggerKey && !event->triggers[context->trigger]) return false;
    if (event->hasTimeKey && !event->timesOfDay[context->timeOfDay]) return false;
    if (event->hasBiomeKey && !event->biomes[context->biome]) return false;

    if (context->infamy < event->minInfamy) return false;
    if (event->maxInfamy > 0 && context->infamy > event->maxInfamy) return false;
    if (context->gold < event->minGold) return false;
    if (context->food < event->minFood) return false;
    if (context->water < event->minWater) return false;
    if (context->armySize < event->minArmySize) return false;
    
    return true;
}

EventDef* GameMaster_FindTriggeredEvent(GameMaster* gm, const EventContext* context) {
    EventDef* validEvents[64];
    int validCount = 0;
    
    for (int i = 0; i < gm->eventCount; i++) {
        if (Event_MatchesContext(&gm->events[i], context)) {
            validEvents[validCount++] = &gm->events[i];
        }
    }
    
    if (validCount == 0) return NULL;
    
    EventDef* chosen = validEvents[GetRandomValue(0, validCount - 1)];
    int roll = GetRandomValue(1, 100);
    if (roll <= chosen->probability) {
        return chosen;
    }
    return NULL;
}

bool GameMaster_AttemptMove(GameMaster* gm, Map* map, Army* playerArmy, HexTile* targetTile) {
    if (!gm || !map || !playerArmy || !targetTile) return false;
    if (gm->isEncamped) return false;
    
    // Debe ser adyacente
    if (HexCoord_Distance(playerArmy->position, targetTile->coord) > 1) return false;

    // Coste de PA según el bioma y noche (7.4)
    BiomeProperties props = Biome_GetProperties(targetTile->biome);
    int apCost = props.actionPointCost;
    const BalanceConfig* cfg = Balance_GetGlobal();
    if (gm->timeOfDay == TIME_NIGHT) {
        apCost = (int)ceilf((float)apCost * cfg->nightMoveCostMult);

        ModContext mCtx = { &gm->modRegistry, playerArmy, NULL, NULL, gm->timeOfDay, gm->infamy };
        apCost = Modifier_Query(HOOK_MAP_MOVE_COST, EFFECT_MOVE_COST, EFFECT_SIDE_SELF, &mCtx, apCost, NULL);
        if (apCost < 1) apCost = 1;
    }

    // ZOC (8.3): Salir de un hexágono adyacente a un enemigo activo cuesta +1 PA
    if (cfg->aiZocEnabled) {
        bool inZoc = false;
        for (int i = 0; i < gm->enemyCount; i++) {
            if (gm->enemies[i].isActive && HexCoord_Distance(playerArmy->position, gm->enemies[i].position) == 1) {
                inZoc = true;
                break;
            }
        }
        if (inZoc) {
            apCost += cfg->aiZocExtraCost;
        }
    }

    if (gm->currentActionPoints >= apCost) {    
        gm->currentActionPoints -= apCost;
        playerArmy->previousPosition = playerArmy->position;
        playerArmy->position = targetTile->coord;
        
        // Verificación de contacto con enemigo (el jugador inicia el combate)
        for (int e = 0; e < gm->enemyCount; e++) {
            if (gm->enemies[e].isActive && gm->enemies[e].position.q == playerArmy->position.q && gm->enemies[e].position.r == playerArmy->position.r) {
                gm->pendingCombatEnemy = e;
                gm->enemies[e].initiator = 0;
                gm->activeCombat.initiator = 0;
                gm->activeCombat.ambushed = (gm->enemies[e].behavior == AI_AMBUSH || Enemy_HasTag(&gm->enemies[e], TAG_AMBUSH));
                break;
            }
        }

        // Consumir recursos (simplificado)
        playerArmy->agua_disponible -= 5.0f * props.waterConsumption;
        playerArmy->comida_disponible -= 5.0f * props.foodConsumption;
        if (playerArmy->agua_disponible < 0) playerArmy->agua_disponible = 0;
        if (playerArmy->comida_disponible < 0) playerArmy->comida_disponible = 0;

        // Actualizar interacción no-modal con estructuras urbanas (9.1)
        gm->canInteractStructure = false;
        if (targetTile->structure.type != STRUCT_NONE && targetTile->structure.type != STRUCT_BRIDGE &&
            targetTile->structure.type != STRUCT_ALTAR && targetTile->structure.type != STRUCT_SIGNPOST &&
            targetTile->structure.type != STRUCT_WATCHTOWER) {
            gm->canInteractStructure = true;
            gm->interactStructureCoord = targetTile->coord;
        } else {
            for (int d = 0; d < 6; d++) {
                HexCoord nCoord = HexCoord_GetNeighbor(targetTile->coord, (HexDirection)d);
                HexTile* nTile = Map_GetTile(map, nCoord);
                if (nTile && nTile->structure.type != STRUCT_NONE && nTile->structure.type != STRUCT_BRIDGE &&
                    nTile->structure.type != STRUCT_ALTAR && nTile->structure.type != STRUCT_SIGNPOST &&
                    nTile->structure.type != STRUCT_WATCHTOWER) {
                    gm->canInteractStructure = true;
                    gm->interactStructureCoord = nTile->coord;
                    break;
                }
            }
        }

        // Preparar el contexto del evento
        EventContext ctx;
        ctx.timeOfDay = gm->timeOfDay;
        ctx.biome = targetTile->biome;
        ctx.infamy = gm->infamy;
        ctx.gold = playerArmy->monedas;
        ctx.food = (int)playerArmy->comida_disponible;
        ctx.water = (int)playerArmy->agua_disponible;
        ctx.armySize = Army_GetTotalSoldiers(playerArmy);
        
        ctx.trigger = TRIGGER_RANDOM;
        if (targetTile->isRoad) ctx.trigger = TRIGGER_ROAD;

        if (targetTile->structure.type == STRUCT_ALTAR) {
            gm->phase = PHASE_DRAFT;
            targetTile->structure.type = STRUCT_NONE;
            return true;
        }
        if (gm->timeOfDay == TIME_NIGHT && ctx.trigger == TRIGGER_RANDOM) {
            ctx.trigger = TRIGGER_NIGHT;
        }

        // Si no es una estructura con menú de ciudad, evaluar eventos de viaje
        if (targetTile->structure.type == STRUCT_NONE || targetTile->structure.type == STRUCT_BRIDGE) {
            EventDef* triggeredEvent = GameMaster_FindTriggeredEvent(gm, &ctx);
            if (triggeredEvent) {
                gm->phase = PHASE_EVENT;
                gm->activeEvent = triggeredEvent;
                gm->eventTimer = 0.0f;
            }
        }
        return true;
    }

    return false;
}

void GameMaster_InitFactions(GameMaster* gm) {
    if (!gm) return;
    const BalanceConfig* cfg = Balance_GetGlobal();
    // 3 facciones con temperamentos distintos (9.4)
    Faction_Init(&gm->factions[0], 0, "Gremio de Comerciantes", -1, gm->infamy, cfg);
    Faction_Init(&gm->factions[1], 1, "Legion del Sol", 0, gm->infamy, cfg);
    Faction_Init(&gm->factions[2], 2, "Hermandad del Hierro", 1, gm->infamy, cfg);
}

void GameMaster_OpenCity(GameMaster* gm, Map* map, Army* playerArmy) {
    if (!gm || !map || !playerArmy) return;
    if (!gm->canInteractStructure) return;
    HexTile* tile = Map_GetTile(map, gm->interactStructureCoord);
    if (!tile || tile->structure.type == STRUCT_NONE) return;

    City_InitVisit(&gm->activeCity, tile->coord, tile->structure.type, tile->structure.settlementType, tile->structure.settlementId, gm->currentDay);
    int fId = tile->structure.factionId;
    if (fId >= 0 && fId < 3) {
        City_SetFaction(&gm->activeCity, &gm->factions[fId], tile->structure.isBesieged);
    }
    gm->phase = PHASE_CITY;
}

void GameMaster_StartSiege(GameMaster* gm, Map* map, Army* playerArmy, HexCoord targetCoord) {
    if (!gm || !map || !playerArmy) return;
    HexTile* tile = Map_GetTile(map, targetCoord);
    if (!tile || tile->structure.type == STRUCT_NONE) return;

    const BalanceConfig* cfg = Balance_GetGlobal();
    int fId = tile->structure.factionId;
    if (fId >= 0 && fId < 3) {
        Faction_SetAlert(&gm->factions[fId], 2, cfg ? cfg->siegeAlertDays : 5);
    }

    gm->inSiegeCombat = true;
    gm->siegeTargetCoord = targetCoord;
    gm->siegeFactionId = fId;

    // Crear enemigo de guarnición fortificada (11, 9.5)
    static EnemySquad garrisonEnemy;
    int gHealth = cfg ? cfg->siegeGarrisonHealth : 150;
    garrisonEnemy = Enemy_Create("Guarnicion Fortificada", targetCoord, 50, 10, 15, 20, 5);
    Enemy_AddTag(&garrisonEnemy, TAG_FORTIFIED);
    Enemy_AddTag(&garrisonEnemy, TAG_HEAVILY_ARMORED);

    gm->activeCombat.ambushed = false;
    gm->activeCombat.initiator = 0; // Jugador inicia el asedio
    Combat_Init(&gm->activeCombat, gm, playerArmy, &garrisonEnemy);
    gm->activeCombat.enemyHealth = (float)gHealth;
    gm->activeCombat.enemyInitialHealth = (float)gHealth;
    gm->activeCombat.maxRounds = cfg ? cfg->siegeMaxRounds : 6;

    gm->inCombat = true;
    gm->phase = PHASE_EVENT;
}

void GameMaster_ResolveSiegeVictory(GameMaster* gm, Map* map, Army* playerArmy, HexCoord targetCoord) {
    if (!gm || !map || !playerArmy) return;
    const BalanceConfig* cfg = Balance_GetGlobal();

    HexTile* tile = Map_GetTile(map, targetCoord);
    if (tile) {
        tile->structure.isBesieged = true;
    }

    // Acreditar botín de asedio (multiplicador) (9.5)
    float lootMult = (cfg && cfg->siegeLootMultiplier > 0.0f) ? cfg->siegeLootMultiplier : 3.0f;
    int extraGold = (int)(gm->activeCombat.lootGold * (lootMult - 1.0f));
    int extraFood = (int)(gm->activeCombat.lootFood * (lootMult - 1.0f));
    if (extraGold > 0) playerArmy->monedas += extraGold;
    if (extraFood > 0) playerArmy->comida_disponible += (float)extraFood;

    // Aumentar infamia (9.5)
    int infGain = cfg ? cfg->siegeInfamyGain : 15;
    gm->infamy += (float)infGain;
    if (gm->infamy > 100.0f) gm->infamy = 100.0f;

    // Bajar opinión de la facción y alertar clusters (9.5)
    int fId = gm->siegeFactionId;
    if (fId >= 0 && fId < 3) {
        int opLoss = cfg ? cfg->siegeOpinionLossOnSiege : 40;
        Faction_ModifyOpinion(&gm->factions[fId], -opLoss);
        Faction_SetAlert(&gm->factions[fId], 1, cfg ? cfg->siegeAlertDays : 5);
    }

    gm->inSiegeCombat = false;
}

void GameMaster_CloseCity(GameMaster* gm, Map* map, Army* playerArmy) {
    if (!gm || !map || !playerArmy) return;
    if (gm->phase != PHASE_CITY) return;

    if (gm->activeCity.requestSiege) {
        HexCoord targetCoord = gm->activeCity.tileCoord;
        gm->activeCity.requestSiege = false;
        gm->phase = PHASE_PLAYER;
        GameMaster_StartSiege(gm, map, playerArmy, targetCoord);
        return;
    }

    if (gm->activeCity.daySpent) {
        DayClock_AdvanceSlot(&gm->clock);
        if (DayClock_HasDayEnded(&gm->clock)) {
            Army_EndDay(playerArmy, &gm->lastDayReport, &gm->rng);
            gm->hasDayReport = true;
            gm->hasForagedToday = false;
        }
    }

    gm->phase = PHASE_PLAYER;
}

int GameMaster_GetVisionModifier(const GameMaster* gm, const Army* army) {
    if (!gm) return 0;
    if (gm->timeOfDay == TIME_NIGHT) {
        ModContext mCtx = { (ModifierRegistry*)&gm->modRegistry, (Army*)army, NULL, NULL, gm->timeOfDay, gm->infamy };
        int vis = Modifier_Query(HOOK_MAP_VISION, EFFECT_VISION, EFFECT_SIDE_SELF, &mCtx, -1, NULL);
        return vis;
    }
    return 0;
}




void GameMaster_ApplyDiceRoll(GameMaster* gm, Army* army, int diceTotal) {
    if (!gm) return;
    int baseAP = gm->baseActionPoints + diceTotal;
    ModContext mCtx = { &gm->modRegistry, army, NULL, NULL, gm->timeOfDay, gm->infamy };
    gm->currentActionPoints = Modifier_Query(HOOK_MAP_BASE_PA, EFFECT_PA, EFFECT_SIDE_SELF, &mCtx, baseAP, NULL);
    gm->hasRolledThisTurn = true;
}






EventResult* GameMaster_FindResult(GameMaster* gm, const char* id) {
    for (int i = 0; i < gm->resultCount; i++) {
        if (strcmp(gm->results[i].id, id) == 0) {
            return &gm->results[i];
        }
    }
    return NULL;
}









void GameMaster_SpawnTestEnemies(GameMaster* gm, Map* map, HexCoord playerPos) {
    if (!gm || !map) return;
    gm->enemyCount = 0;
    int maxInitialEnemies = 4;
    int attempts = 0;
    while (gm->enemyCount < maxInitialEnemies && attempts < 100) {
        int r = Rng_Range(&gm->rng, 0, map->tileCount - 1);
        if ((map->tiles[r].isRoad || map->tiles[r].structure.type != STRUCT_NONE) && HexCoord_Distance(map->tiles[r].coord, playerPos) > 3) {
            EnemySquad e;
            int type = Rng_Range(&gm->rng, 0, 3);
            if (type == 0) {
                e = Enemy_CreateWithBehavior("Milicia Rebelde", map->tiles[r].coord, 20 + (gm->currentDay * 3), 10 + gm->currentDay, 15, 0, 0, AI_PATROL);
                Enemy_AddTag(&e, TAG_DISORGANIZED);
            } else if (type == 1) {
                e = Enemy_CreateWithBehavior("Guardia de Hierro", map->tiles[r].coord, 40 + (gm->currentDay * 6), 5 + gm->currentDay, 5, 20, 5, AI_GUARD);
                Enemy_SetGuardLeash(&e, 4);
                Enemy_AddTag(&e, TAG_HEAVILY_ARMORED);
            } else if (type == 2) {
                e = Enemy_CreateWithBehavior("Incursores a Caballo", map->tiles[r].coord, 30 + (gm->currentDay * 4), 20 + gm->currentDay, 5, 10, 0, AI_AMBUSH);
                Enemy_AddTag(&e, TAG_CAVALRY);
                Enemy_AddTag(&e, TAG_AMBUSH);
            } else {
                e = Enemy_CreateWithBehavior("Mercenarios Veteranos", map->tiles[r].coord, 50 + (gm->currentDay * 5), 12 + gm->currentDay, 0, 30, 10, AI_PATROL);
            }
            
            e.originalSpawn = e.position;
            e.homeHex = e.position;
            e.patrolTarget = e.position;
            e.patrolWaitTurns = 0;
            
            gm->enemies[gm->enemyCount++] = e;
        }
        attempts++;
    }
}
