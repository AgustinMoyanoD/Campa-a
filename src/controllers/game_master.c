#include "controllers/game_master.h"
#include "controllers/npc_controller.h"
#include "models/balance_config.h"
#include <stdio.h>
#include <stdlib.h>
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
    gm->campDecisionTimer = 0.0f;
    gm->isEncamped = false;
    gm->spentNightAtInn = false;
    gm->hasForagedToday = false;
    memset(&gm->lastDayReport, 0, sizeof(DayReport));
    gm->eventCount = Event_LoadAll("assets/events", gm->events, 64);
    gm->resultCount = Result_LoadAll("assets/results", gm->results, 256);
    // Tambien puede haber resultados en la carpeta de eventos
    gm->resultCount += Result_LoadAll("assets/events", &gm->results[gm->resultCount], 256 - gm->resultCount);
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
    gm->pendingCityAfterEvent = false;
    gm->pendingCityCoord = (HexCoord){0, 0};
    gm->pendingCityBlocked = false;
    gm->currentEventFactionId = -1;
    gm->flagCount = 0;
    memset(gm->flags, 0, sizeof(gm->flags));
    gm->inSiegeCombat = false;
    gm->siegeTargetCoord = (HexCoord){0, 0};
    gm->siegeFactionId = 0;
    GameMaster_InitFactions(gm);
}

void GameMaster_StartCampaign(GameMaster* gm, Map* map, Army* playerArmy, const ArmySetupConfig* setup, int provinceLevel) {
    if (!gm || !map || !playerArmy) return;

    MapConfig mapCfg;
    MapConfig_InitDefault(&mapCfg, provinceLevel);
    Map_Init(map, &mapCfg);

    HexCoord startCoord = Map_GetStartCoord(map);
    Army_Init(playerArmy, setup, startCoord);

    GameMaster_Init(gm);
    GameMaster_SpawnTestEnemies(gm, map, playerArmy->position);
    GameMaster_StartTurn(gm, playerArmy);

    int visionRange = Army_GetVision(playerArmy) + GameMaster_GetVisionModifier(gm, playerArmy);
    Map_UpdateVision(map, playerArmy->position, visionRange);
}

void GameMaster_TransitionProvince(GameMaster* gm, Map* map, Army* playerArmy, int provinceLevel) {
    if (!gm || !map || !playerArmy) return;

    MapConfig mapCfg;
    MapConfig_InitDefault(&mapCfg, provinceLevel);
    Map_Init(map, &mapCfg);

    playerArmy->position = Map_GetStartCoord(map);
    playerArmy->previousPosition = playerArmy->position;
    playerArmy->isMoving = false;
    playerArmy->pathLength = 0;

    int visionRange = Army_GetVision(playerArmy) + GameMaster_GetVisionModifier(gm, playerArmy);
    Map_UpdateVision(map, playerArmy->position, visionRange);

    /* Evaluación de retorno del general depuesto (E1) */
    if (playerArmy->hasDeposedGeneral) {
        const BalanceConfig* bcfg = Balance_GetGlobal();
        float returnMoral = (bcfg && bcfg->returnMoral > 0.0f) ? bcfg->returnMoral : 60.0f;
        if (playerArmy->moral >= returnMoral) {
            EventDef* retEvt = Event_FindById(gm->events, gm->eventCount, "ev_general_regresa");
            if (retEvt) {
                gm->phase = PHASE_EVENT;
                gm->activeEvent = retEvt;
                gm->eventTimer = 0.0f;
            }
        }
    }
}

void GameMaster_StartTurn(GameMaster* gm, Army* playerArmy) {
    if (!gm || !playerArmy) return;
    gm->phase = PHASE_PLAYER;
    gm->currentActionPoints = 0;
    gm->hasRolledThisTurn = false;
    gm->eventTimer = 0.0f;
    gm->isEncamped = false;
    gm->spentNightAtInn = false;
}

void GameMaster_ProcessEnemyTurn(GameMaster* gm, Map* map, Army* playerArmy) {
    NPC_ProcessTurn(gm, map, playerArmy);
}

void GameMaster_EndTurn(GameMaster* gm, Map* map, Army* playerArmy) {
    if (!gm || !map || !playerArmy) return;

    /* Si la decisión de campamento está abierta, no avanzar el turno hasta que el jugador elija */
    if (gm->showCampDecision) return;

    /* Decisión de la Tarde (7.2): Al terminar la Tarde se muestra la opción de acampar o seguir marchando */
    if (gm->timeOfDay == TIME_AFTERNOON) {
        gm->showCampDecision = true;
        gm->campDecisionTimer = 0.0f;
        return;
    }

    /* 1. Avanzar franja horaria a travÃƒÂ©s de DayClock (I4) */
    DayClock_AdvanceSlot(&gm->clock);
    gm->timeOfDay = gm->clock.timeOfDay;
    gm->currentDay = gm->clock.currentDay;
    gm->currentTurn = gm->clock.totalTurnsElapsed + 1;
    gm->isEncamped = false;

    /* 2. Si terminÃ³ el dÃ­a (4 franjas), abrir reparto de raciones y cierre de dÃ­a */
    if (DayClock_HasDayEnded(&gm->clock)) {
        gm->rationPending = true;
        RationNeeds needs = Ration_GetNeeds(playerArmy, map, playerArmy->position);
        gm->pendingRationPlan = Ration_Auto(playerArmy, &needs);
        gm->hasForagedToday = false; /* Reset para el nuevo dÃ­a */
        gm->infamy += 1.0f; /* Sube la infamia levemente al pasar un dÃ­a */
        if (gm->infamy > 100.0f) gm->infamy = 100.0f;

        for (int f = 0; f < 3; f++) {
            Faction_TickDay(&gm->factions[f]);
        }

        /* Oficial Explorador (E3): Al comenzar el dÃƒÂ­a revela asentamientos en un radio de 8 hexÃƒÂ¡gonos */
        if (Army_GetLieutenantByRole(playerArmy, ROLE_EXPLORADOR)) {
            for (int ti = 0; ti < map->tileCount; ti++) {
                HexTile* t = &map->tiles[ti];
                if (HexCoord_Distance(playerArmy->position, t->coord) <= 8) {
                    if (t->structure.type == STRUCT_TOWN_CASTLE || t->structure.type == STRUCT_TOWN_MARKET ||
                        t->structure.type == STRUCT_TOWN_ARMORY || t->structure.type == STRUCT_TOWN_HOUSES) {
                        t->visited = true; /* Revelado en el mapa */
                    }
                }
            }
        }

        /* Spawn periÃƒÂ³dico de Cazador (8.5) */
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
                    gm->enemies[gm->enemyCount++] = NPC_CreateHunterSquad(gm, playerArmy, spawnPos);
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
    gm->campDecisionTimer = 0.0f;
    gm->isEncamped = true;
    gm->spentNightAtInn = false;
    playerArmy->consecutiveNightMarches = 0;
    gm->lastCampPos = playerArmy->position;

    /* 1. Recuperar moral y salud si no sufre hambruna/sed severa desde el día 2+ (7.3) */
    HexTile* tile = Map_GetTile(map, playerArmy->position);
    bool inForest = tile && (tile->biome == BIOME_FOREST || tile->biome == BIOME_JUNGLE);
    bool starvingNoRec = cfg->campStarvingNoRecovery && (playerArmy->daysWithoutFood >= 2 || playerArmy->daysWithoutWater >= 2);
    if (!starvingNoRec) {
        float recMoral = cfg->campRecoverMoral + (inForest ? 3.0f : 0.0f);
        float recSalud = cfg->campRecoverSalud + (inForest ? 3.0f : 0.0f);
        Army_ApplyMoraleDelta(playerArmy, recMoral);
        Army_ApplyHealthDelta(playerArmy, recSalud);
    }

    /* Reagrupamiento de rezagados tras derrota reciente (rompe la espiral de derrota) */
    if (GameMaster_HasFlag(gm, "derrota_reciente")) {
        GameMaster_ClearFlag(gm, "derrota_reciente");
        GameMaster_SetFlag(gm, "reagrupamiento_reciente");
        playerArmy->iniciados += 3;
        playerArmy->soldados += 2;
        if (playerArmy->salud < 35.0f) {
            playerArmy->salud = 35.0f;
        }
        Army_ApplyMoraleDelta(playerArmy, 10.0f);
        Army_RecalculateAggregates(playerArmy);
    } else {
        GameMaster_ClearFlag(gm, "reagrupamiento_reciente");
    }

    /* El descanso de campamento ayuda a recargar cartas divinas agotadas */
    if (playerArmy->commander.ptr) {
        for (int c = 0; c < 8; c++) {
            ModifierInstance* inst = Commander_GetCardInstance(playerArmy->commander, c);
            if (!inst) break;
            if (inst->isExhausted) {
                if (inst->battlesUntilRecharge > 0) inst->battlesUntilRecharge--;
                if (inst->battlesUntilRecharge <= 0) inst->isExhausted = false;
            }
        }
    }

    /* 2. Tirada de emboscada: 2d6 + Evaluación + cobertura (7.3) */
    int cover = Balance_GetTerrainCover(cfg, tile ? (int)tile->biome : 0, tile ? (int)tile->structure.type : 0);
    
    int evalStat = (int)Army_CalculateEvaluation(playerArmy);
    ModContext mCtx = { &gm->modRegistry, playerArmy, NULL, NULL, gm->timeOfDay, gm->infamy };
    evalStat = Modifier_Query(HOOK_AMBUSH_ROLL, EFFECT_EVALUATION, EFFECT_SIDE_SELF, &mCtx, evalStat, NULL);

    int diceRoll = Rng_RollDice(&gm->rng, 2, 6);
    int totalRoll = (cover >= 90) ? 99 : (diceRoll + evalStat + cover);

    bool ambushTriggered = (totalRoll < cfg->campAmbushTarget);

    /* 4. Avanzar reloj: la noche transcurre en el campamento hasta la mañana siguiente */
    DayClock_AdvanceSlot(&gm->clock); /* Avanza a NIGHT */
    DayClock_AdvanceSlot(&gm->clock); /* Cierra el día y pasa a MORNING (Dawn) */
    gm->timeOfDay = gm->clock.timeOfDay;
    gm->currentDay = gm->clock.currentDay;
    gm->currentTurn = gm->clock.totalTurnsElapsed + 1;

    /* 5. Abrir reparto de raciones y cierre de día */
    gm->rationPending = true;
    RationNeeds needs = Ration_GetNeeds(playerArmy, map, playerArmy->position);
    gm->pendingRationPlan = Ration_Auto(playerArmy, &needs);
    gm->hasForagedToday = false; /* Reset para el nuevo día */
    gm->infamy += 1.0f;
    if (gm->infamy > 100.0f) gm->infamy = 100.0f;

    for (int f = 0; f < 3; f++) {
        Faction_TickDay(&gm->factions[f]);
    }

    /* Oficial Explorador (E3): Al comenzar el día revela asentamientos en un radio de 8 hexágonos */
    if (Army_GetLieutenantByRole(playerArmy, ROLE_EXPLORADOR)) {
        for (int ti = 0; ti < map->tileCount; ti++) {
            HexTile* t = &map->tiles[ti];
            if (HexCoord_Distance(playerArmy->position, t->coord) <= 8) {
                if (t->structure.type == STRUCT_TOWN_CASTLE || t->structure.type == STRUCT_TOWN_MARKET ||
                    t->structure.type == STRUCT_TOWN_ARMORY || t->structure.type == STRUCT_TOWN_HOUSES) {
                    t->visited = true; /* Revelado en el mapa */
                }
            }
        }
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
                gm->enemies[gm->enemyCount++] = NPC_CreateHunterSquad(gm, playerArmy, spawnPos);
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

    // Procesar turno de enemigos durante la noche (con isEncamped = true)
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
    Army_ApplyMoraleDelta(playerArmy, cfg->nightMoralPerTurn * penaltyMult);
    Army_ApplyHealthDelta(playerArmy, cfg->nightSaludPerTurn * penaltyMult);

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
        ctx.cattle = playerArmy->ganado;
        ctx.daysWithoutFood = playerArmy->daysWithoutFood;
        ctx.daysWithoutWater = playerArmy->daysWithoutWater;
        ctx.trigger = TRIGGER_NIGHT;
        ctx.factionId = tile ? tile->structure.factionId : -1;

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

void GameMaster_ChooseInn(GameMaster* gm, Map* map, Army* playerArmy) {
    if (!gm || !playerArmy) return;
    const BalanceConfig* cfg = Balance_GetGlobal();

    gm->showCampDecision = false;
    gm->campDecisionTimer = 0.0f;
    gm->isEncamped = false;
    gm->spentNightAtInn = true;
    playerArmy->consecutiveNightMarches = 0;
    gm->lastCampPos = playerArmy->position;

    /* 1. Coste del hospedaje en la posada */
    int innCost = (cfg && cfg->servicesInnRestCost > 0) ? cfg->servicesInnRestCost : 10;
    HexTile* tile = map ? Map_GetTile(map, playerArmy->position) : NULL;
    int fId = tile ? tile->structure.factionId : -1;
    if (fId >= 0 && fId < 3 && cfg && gm->factions[fId].opinion >= cfg->factionsThresholdAllied) {
        innCost = 0; /* Aliados se hospedan y comen gratis */
    }
    if (playerArmy->monedas >= innCost) {
        playerArmy->monedas -= innCost;
    } else {
        playerArmy->monedas = 0;
    }

    /* 2. Recuperación de moral y salud por comodidades de la posada */
    float recMoral = (cfg && cfg->servicesInnRecoverMoral > 0.0f) ? cfg->servicesInnRecoverMoral : 15.0f;
    Army_ApplyMoraleDelta(playerArmy, recMoral);
    Army_ApplyHealthDelta(playerArmy, 10.0f);

    /* La posada alimenta automáticamente al ejército (todos reciben ración Completa, no se consumen provisiones de la mochila) */
    for (int g = 0; g < RG_COUNT; g++) {
        playerArmy->groupState[g].daysUnfed = 0;
        playerArmy->groupState[g].health = fminf(100.0f, playerArmy->groupState[g].health + 10.0f);
        playerArmy->groupState[g].morale = fminf(100.0f, playerArmy->groupState[g].morale + 15.0f);
    }
    playerArmy->groupState[RG_SARGENTOS].satisfaction += 3.0f;
    if (playerArmy->groupState[RG_SARGENTOS].satisfaction > playerArmy->groupState[RG_SARGENTOS].satisfactionMax) {
        playerArmy->groupState[RG_SARGENTOS].satisfaction = playerArmy->groupState[RG_SARGENTOS].satisfactionMax;
    }
    playerArmy->daysWithoutFood = 0;
    playerArmy->daysWithoutWater = 0;
    playerArmy->dayRationCommitted = true;

    /* No se abre la pantalla de reparto diario */
    gm->rationPending = false;
    gm->hasForagedToday = false;

    /* 3. Avanzar reloj hasta la mañana siguiente */
    while (true) {
        DayClock_AdvanceSlot(&gm->clock);
        if (gm->clock.timeOfDay == TIME_MORNING) {
            break;
        }
    }
    gm->timeOfDay = gm->clock.timeOfDay;
    gm->currentDay = gm->clock.currentDay;
    gm->currentTurn = gm->clock.totalTurnsElapsed + 1;

    gm->infamy += 1.0f;
    if (gm->infamy > 100.0f) gm->infamy = 100.0f;

    for (int f = 0; f < 3; f++) {
        Faction_TickDay(&gm->factions[f]);
    }

    /* Oficial Explorador (E3) */
    if (map && Army_GetLieutenantByRole(playerArmy, ROLE_EXPLORADOR)) {
        for (int ti = 0; ti < map->tileCount; ti++) {
            HexTile* t = &map->tiles[ti];
            if (HexCoord_Distance(playerArmy->position, t->coord) <= 8) {
                if (t->structure.type == STRUCT_TOWN_CASTLE || t->structure.type == STRUCT_TOWN_MARKET ||
                    t->structure.type == STRUCT_TOWN_ARMORY || t->structure.type == STRUCT_TOWN_HOUSES) {
                    t->visited = true;
                }
            }
        }
    }

    /* Spawn periódico de Cazador tras nuevo día */
    if (map && cfg && cfg->hunterEnabled && cfg->hunterSpawnEveryDays > 0) {
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
                gm->enemies[gm->enemyCount++] = NPC_CreateHunterSquad(gm, playerArmy, spawnPos);
                gm->lastHunterSpawnDay = gm->currentDay;
            }
        }
    }

    /* 4. Procesar turno de enemigos durante la noche con protección total (spentNightAtInn = true) */
    if (map) {
        GameMaster_ProcessEnemyTurn(gm, map, playerArmy);
    }

    /* 5. Reiniciar turno matutino del jugador (Dawn) */
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

    /* Tirada 2d6 + EvaluaciÃƒÂ³n (6.5) */
    int diceRoll = Rng_RollDice(&gm->rng, 2, 6);
    int evalStat = (int)Army_CalculateEvaluation(army);
    int totalRoll = diceRoll + evalStat;

    float mult = 1.0f;
    if (totalRoll < 6) mult = 0.5f;
    else if (totalRoll >= 10) mult = 1.5f;

    /* Oficial Intendente (E3): +25% al rendimiento del forrajeo */
    if (Army_GetLieutenantByRole(army, ROLE_INTENDENTE)) {
        mult *= 1.25f;
    }

    int bIdx = (int)tile->biome;
    if (bIdx < 0 || bIdx > 5) bIdx = 0;

    float gainedFood = dailyFoodUsage * cfg->forageFood[bIdx] * mult;
    float gainedWater = dailyWaterUsage * cfg->forageWater[bIdx] * mult;

    army->comida_disponible += gainedFood;
    army->agua_disponible += gainedWater;

    gm->hasForagedToday = true;
    gm->currentActionPoints = 0;

    /* Al forrajear se pasa directamente a la tarde; si ya es de tarde, abre la decisiÃ³n nocturna obligatoria */
    if (gm->timeOfDay == TIME_MORNING) {
        DayClock_AdvanceSlot(&gm->clock); /* A Midday */
        DayClock_AdvanceSlot(&gm->clock); /* A Afternoon */
        gm->timeOfDay = gm->clock.timeOfDay;
        gm->currentTurn = gm->clock.totalTurnsElapsed + 1;
        gm->phase = PHASE_ENEMY;
        GameMaster_ProcessEnemyTurn(gm, map, army);
        GameMaster_StartTurn(gm, army);
    } else if (gm->timeOfDay == TIME_MIDDAY) {
        DayClock_AdvanceSlot(&gm->clock); /* A Afternoon */
        gm->timeOfDay = gm->clock.timeOfDay;
        gm->currentTurn = gm->clock.totalTurnsElapsed + 1;
        gm->phase = PHASE_ENEMY;
        GameMaster_ProcessEnemyTurn(gm, map, army);
        GameMaster_StartTurn(gm, army);
    } else if (gm->timeOfDay == TIME_AFTERNOON) {
        gm->showCampDecision = true;
    }

    return true;
}

static bool Event_MatchesContext(const GameMaster* gm, const EventDef* event, const EventContext* context) {
    if (event->hasTriggerKey && !event->triggers[context->trigger]) return false;
    if (event->hasTimeKey && !event->timesOfDay[context->timeOfDay]) return false;
    if (event->hasBiomeKey && !event->biomes[context->biome]) return false;

    if (context->infamy < event->minInfamy) return false;
    if (event->maxInfamy > 0 && context->infamy > event->maxInfamy) return false;
    if (context->gold < event->minGold) return false;
    if (context->food < event->minFood) return false;
    if (context->water < event->minWater) return false;
    if (context->armySize < event->minArmySize) return false;
    if (context->cattle < event->minCattle) return false;

    // Filtros de Inanicion y Sed (Prioridad 4)
    if (event->daysWithoutFoodMin > 0 && context->daysWithoutFood < event->daysWithoutFoodMin) return false;
    if (event->daysWithoutWaterMin > 0 && context->daysWithoutWater < event->daysWithoutWaterMin) return false;

    // Filtro de Banderas Narrativas en Cabecera (Prioridad 3)
    if (event->requiredFlag[0] != '\0') {
        if (strncmp(event->requiredFlag, "!flag:", 6) == 0) {
            if (GameMaster_HasFlag(gm, event->requiredFlag + 6)) return false;
        } else if (strncmp(event->requiredFlag, "flag:", 5) == 0) {
            if (!GameMaster_HasFlag(gm, event->requiredFlag + 5)) return false;
        } else if (event->requiredFlag[0] == '!') {
            if (GameMaster_HasFlag(gm, event->requiredFlag + 1)) return false;
        } else {
            if (!GameMaster_HasFlag(gm, event->requiredFlag)) return false;
        }
    }

    return true;
}

EventDef* GameMaster_FindTriggeredEvent(GameMaster* gm, const EventContext* context) {
    EventDef* validEvents[64];
    int validCount = 0;
    
    for (int i = 0; i < gm->eventCount; i++) {
        if (Event_MatchesContext(gm, &gm->events[i], context)) {
            validEvents[validCount++] = &gm->events[i];
        }
    }
    
    if (validCount == 0) return NULL;
    
    EventDef* chosen = validEvents[GetRandomValue(0, validCount - 1)];
    int roll = GetRandomValue(1, 100);
    if (roll <= chosen->probability) {
        gm->currentEventFactionId = context->factionId;
        return chosen;
    }
    return NULL;
}

bool GameMaster_AttemptMove(GameMaster* gm, Map* map, Army* playerArmy, HexTile* targetTile) {
    if (!gm || !map || !playerArmy || !targetTile) return false;
    if (gm->isEncamped) return false;
    
    // Debe ser adyacente
    if (HexCoord_Distance(playerArmy->position, targetTile->coord) > 1) return false;

    // Coste de PA segÃƒÂºn el bioma y noche (7.4)
    BiomeProperties props = Biome_GetProperties(targetTile->biome);
    int apCost = props.actionPointCost;
    const BalanceConfig* cfg = Balance_GetGlobal();
    if (gm->timeOfDay == TIME_NIGHT) {
        apCost = (int)ceilf((float)apCost * cfg->nightMoveCostMult);

        ModContext mCtx = { &gm->modRegistry, playerArmy, NULL, NULL, gm->timeOfDay, gm->infamy };
        apCost = Modifier_Query(HOOK_MAP_MOVE_COST, EFFECT_MOVE_COST, EFFECT_SIDE_SELF, &mCtx, apCost, NULL);
        if (apCost < 1) apCost = 1;
    }

    // ZOC (8.3): Salir de un hexÃƒÂ¡gono adyacente a un enemigo activo cuesta +1 PA
    if (cfg->aiZocEnabled) {
        if (NPC_IsPlayerInZOC(gm, playerArmy->position)) {
            apCost += cfg->aiZocExtraCost;
        }
    }

    if (gm->currentActionPoints >= apCost) {    
        gm->currentActionPoints -= apCost;
        playerArmy->previousPosition = playerArmy->position;
        playerArmy->position = targetTile->coord;
        
        // VerificaciÃ³n de contacto con enemigo hostil (paso pacÃ­fico si no es hostil)
        int hostileIdx = NPC_FindHostileSquadAt(gm, playerArmy->position);
        if (hostileIdx >= 0) {
            gm->pendingCombatEnemy = hostileIdx;
            gm->enemies[hostileIdx].initiator = 0;
            gm->activeCombat.initiator = 0;
            gm->activeCombat.ambushed = (gm->enemies[hostileIdx].behavior == AI_AMBUSH || Enemy_HasTag(&gm->enemies[hostileIdx], TAG_AMBUSH));
        }

        // Actualizar interacciÃ³n no-modal con estructuras urbanas (9.1)
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
        memset(&ctx, 0, sizeof(EventContext));
        ctx.timeOfDay = gm->timeOfDay;
        ctx.biome = targetTile->biome;
        ctx.infamy = gm->infamy;
        ctx.gold = playerArmy->monedas;
        ctx.food = (int)playerArmy->comida_disponible;
        ctx.water = (int)playerArmy->agua_disponible;
        ctx.armySize = Army_GetTotalSoldiers(playerArmy);
        ctx.cattle = playerArmy->ganado;
        ctx.daysWithoutFood = playerArmy->daysWithoutFood;
        ctx.daysWithoutWater = playerArmy->daysWithoutWater;
        ctx.factionId = targetTile->structure.factionId;
        
        ctx.trigger = TRIGGER_RANDOM;
        if (targetTile->isRoad) ctx.trigger = TRIGGER_ROAD;

        if (targetTile->structure.type == STRUCT_ALTAR) {
            gm->phase = PHASE_DRAFT;
            targetTile->structure.type = STRUCT_NONE;
            // Un altar restaura todas las cartas divinas agotadas
            if (playerArmy && playerArmy->commander.ptr) {
                for (int c = 0; c < 8; c++) {
                    Commander_SetCardExhausted(playerArmy->commander, c, false);
                }
            }
            return true;
        }
        if (gm->timeOfDay == TIME_NIGHT && ctx.trigger == TRIGGER_RANDOM) {
            ctx.trigger = TRIGGER_NIGHT;
        }

        // Si no es una estructura con menÃƒÂº de ciudad, evaluar eventos de viaje
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

    // Crear enemigo de guarniciÃ³n fortificada (11, 9.5)
    static EnemySquad garrisonEnemy;
    int gHealth = cfg ? cfg->siegeGarrisonHealth : 150;
    garrisonEnemy = NPC_CreateGarrisonSquad(targetCoord, fId);

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

    // Acreditar botÃ­n de asedio (multiplicador) (9.5)
    float lootMult = (cfg && cfg->siegeLootMultiplier > 0.0f) ? cfg->siegeLootMultiplier : 3.0f;
    int extraGold = (int)(gm->activeCombat.lootGold * (lootMult - 1.0f));
    int extraFood = (int)(gm->activeCombat.lootFood * (lootMult - 1.0f));
    int extraWine = (int)(gm->activeCombat.lootWine * (lootMult - 1.0f));
    int extraMeat = (int)(gm->activeCombat.lootMeat * (lootMult - 1.0f));
    if (extraGold > 0) playerArmy->monedas += extraGold;
    if (extraFood > 0) playerArmy->comida_disponible += (float)extraFood;
    if (extraWine > 0) playerArmy->vino_disponible += (float)extraWine;
    if (extraMeat > 0) playerArmy->carne_disponible += (float)extraMeat;

    // Aumentar infamia (9.5)
    int infGain = cfg ? cfg->siegeInfamyGain : 15;
    gm->infamy += (float)infGain;
    if (gm->infamy > 100.0f) gm->infamy = 100.0f;

    // Bajar opiniÃƒÂ³n de la facciÃƒÂ³n y alertar clusters (9.5)
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
        gm->activeCity.daySpent = false;
        GameMaster_ChooseInn(gm, map, playerArmy);
        return;
    }

    gm->phase = PHASE_PLAYER;
}

bool GameMaster_InteractStructure(GameMaster* gm, Map* map, Army* playerArmy) {
    if (!gm || !map || !playerArmy) return false;
    if (!gm->canInteractStructure) return false;
    HexTile* tile = Map_GetTile(map, gm->interactStructureCoord);
    if (!tile || tile->structure.type == STRUCT_NONE) return false;

    // Mapear estructura al trigger correspondiente
    EventTrigger trig = TRIGGER_CITY;
    switch (tile->structure.type) {
        case STRUCT_TOWN_MARKET: trig = TRIGGER_MARKET; break;
        case STRUCT_TOWN_ARMORY: trig = TRIGGER_ARMORY; break;
        case STRUCT_TOWN_CASTLE: trig = TRIGGER_CASTLE; break;
        case STRUCT_TOWN_HOUSES: trig = TRIGGER_HOUSE; break;
        default: trig = TRIGGER_CITY; break;
    }

    EventContext ctx;
    memset(&ctx, 0, sizeof(EventContext));
    ctx.trigger = trig;
    ctx.timeOfDay = gm->timeOfDay;
    ctx.biome = tile->biome;
    ctx.infamy = gm->infamy;
    ctx.gold = playerArmy->monedas;
    ctx.food = (int)playerArmy->comida_disponible;
    ctx.water = (int)playerArmy->agua_disponible;
    ctx.armySize = Army_GetTotalSoldiers(playerArmy);
    ctx.cattle = playerArmy->ganado;
    ctx.daysWithoutFood = playerArmy->daysWithoutFood;
    ctx.daysWithoutWater = playerArmy->daysWithoutWater;
    ctx.factionId = tile->structure.factionId;

    EventDef* triggeredEvent = GameMaster_FindTriggeredEvent(gm, &ctx);
    if (!triggeredEvent && trig != TRIGGER_CITY) {
        // Fallback a TRIGGER_CITY si no hubo evento especÃ­fico
        ctx.trigger = TRIGGER_CITY;
        triggeredEvent = GameMaster_FindTriggeredEvent(gm, &ctx);
    }

    if (triggeredEvent) {
        gm->pendingCityAfterEvent = true;
        gm->pendingCityCoord = tile->coord;
        gm->pendingCityBlocked = false;
        gm->currentEventFactionId = tile->structure.factionId;
        gm->phase = PHASE_EVENT;
        gm->activeEvent = triggeredEvent;
        gm->eventTimer = 0.0f;
        return true;
    }

    // Sin evento previo: abrir menÃº de ciudad directamente
    GameMaster_OpenCity(gm, map, playerArmy);
    return false;
}

bool GameMaster_HasFlag(const GameMaster* gm, const char* flag) {
    if (!gm || !flag || flag[0] == '\0') return false;
    for (int i = 0; i < gm->flagCount; i++) {
        if (strcmp(gm->flags[i], flag) == 0) return true;
    }
    return false;
}

void GameMaster_SetFlag(GameMaster* gm, const char* flag) {
    if (!gm || !flag || flag[0] == '\0') return;
    if (GameMaster_HasFlag(gm, flag)) return;
    if (gm->flagCount < MAX_FLAGS) {
        strncpy(gm->flags[gm->flagCount], flag, MAX_FLAG_LEN - 1);
        gm->flags[gm->flagCount][MAX_FLAG_LEN - 1] = '\0';
        TraceLog(LOG_INFO, "[FLAG] Bandera agregada: '%s' (Total: %d)", gm->flags[gm->flagCount], gm->flagCount + 1);
        gm->flagCount++;
    } else {
        TraceLog(LOG_WARNING, "[FLAG] Limite de banderas alcanzado (%d)", MAX_FLAGS);
    }
}

void GameMaster_ClearFlag(GameMaster* gm, const char* flag) {
    if (!gm || !flag || flag[0] == '\0') return;
    for (int i = 0; i < gm->flagCount; i++) {
        if (strcmp(gm->flags[i], flag) == 0) {
            TraceLog(LOG_INFO, "[FLAG] Bandera removida: '%s'", flag);
            for (int j = i; j < gm->flagCount - 1; j++) {
                strncpy(gm->flags[j], gm->flags[j + 1], MAX_FLAG_LEN);
            }
            gm->flagCount--;
            return;
        }
    }
}

void GameMaster_ApplyFactionOpinion(GameMaster* gm, const char* targetStr, int delta) {
    if (!gm || !targetStr || targetStr[0] == '\0') return;
    int fId = -1;
    if (strcasecmp(targetStr, "current") == 0) {
        if (gm->currentEventFactionId >= 0 && gm->currentEventFactionId < 3) {
            fId = gm->currentEventFactionId;
        } else if (gm->activeCity.settlementId > 0 && gm->activeCity.factionId >= 0) {
            fId = gm->activeCity.factionId;
        }
    } else if (strcasecmp(targetStr, "gremio") == 0) {
        fId = 0;
    } else if (strcasecmp(targetStr, "sol") == 0) {
        fId = 1;
    } else if (strcasecmp(targetStr, "hierro") == 0) {
        fId = 2;
    } else if (targetStr[0] >= '0' && targetStr[0] <= '9') {
        fId = atoi(targetStr);
    } else {
        for (int i = 0; i < 3; i++) {
            if (strstr(gm->factions[i].name, targetStr)) {
                fId = i;
                break;
            }
        }
    }

    if (fId >= 0 && fId < 3) {
        Faction_ModifyOpinion(&gm->factions[fId], delta);
        TraceLog(LOG_INFO, "[FACTION] Opinion de '%s' modificada %+d -> Actual: %d", gm->factions[fId].name, delta, gm->factions[fId].opinion);
    } else {
        TraceLog(LOG_WARNING, "[FACTION] Objetivo de faccion desconocido '%s'", targetStr);
    }
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
    NPC_SpawnInitialSquads(gm, map, playerPos);
}

bool GameMaster_RationPending(const GameMaster* gm) {
    return gm ? gm->rationPending : false;
}

bool GameMaster_SubmitRation(GameMaster* gm, Army* playerArmy, const RationPlan* plan) {
    if (!gm || !playerArmy || !plan) return false;
    char err[64];
    if (!Ration_Validate(playerArmy, plan, err, sizeof(err))) return false;

    Ration_Commit(playerArmy, plan, &gm->lastDayReport, &gm->rng);
    Army_EndDay(playerArmy, &gm->lastDayReport, &gm->rng);
    gm->hasDayReport = true;
    gm->rationPending = false;

    /* Chequear triggers narrativos de raciones */
    EventContext ctx;
    memset(&ctx, 0, sizeof(EventContext));
    ctx.timeOfDay = gm->timeOfDay;
    ctx.infamy = gm->infamy;
    ctx.gold = playerArmy->monedas;
    ctx.food = (int)playerArmy->comida_disponible;
    ctx.water = (int)playerArmy->agua_disponible;
    ctx.armySize = Army_GetTotalSoldiers(playerArmy);
    ctx.cattle = playerArmy->ganado;
    ctx.daysWithoutFood = playerArmy->daysWithoutFood;
    ctx.daysWithoutWater = playerArmy->daysWithoutWater;

    /* TRIGGER_SARGENTOS_UNSATISFIED */
    if (playerArmy->groupState[RG_SARGENTOS].satisfaction <= 0.001f) {
        ctx.trigger = TRIGGER_SARGENTOS_UNSATISFIED;
        EventDef* evt = GameMaster_FindTriggeredEvent(gm, &ctx);
        if (evt) {
            gm->phase = PHASE_EVENT;
            gm->activeEvent = evt;
            gm->eventTimer = 0.0f;
            return true;
        }
    }

    /* TRIGGER_GROUP_STARVING */
    if (playerArmy->groupState[RG_INICIADOS].daysUnfed > 0 ||
        playerArmy->groupState[RG_SOLDADOS].daysUnfed > 0 ||
        playerArmy->groupState[RG_SARGENTOS].daysUnfed > 0) {
        ctx.trigger = TRIGGER_GROUP_STARVING;
        EventDef* evt = GameMaster_FindTriggeredEvent(gm, &ctx);
        if (evt) {
            gm->phase = PHASE_EVENT;
            gm->activeEvent = evt;
            gm->eventTimer = 0.0f;
            return true;
        }
    }

    return true;
}

int GameMaster_SlaughterCattle(GameMaster* gm, Army* playerArmy, int heads) {
    if (!gm || !playerArmy) return 0;
    return Ration_SlaughterCattle(playerArmy, heads);
}