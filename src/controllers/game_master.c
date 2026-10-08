#include "controllers/game_master.h"
#include <stdio.h>
#include "raylib.h"
#include <string.h>

void GameMaster_Init(GameMaster* gm)
 {
    if (!gm) return;
    gm->currentTurn = 1;
    gm->currentDay = 1;
    gm->timeOfDay = TIME_MORNING;
    gm->phase = PHASE_PLAYER;
    gm->infamy = 0.0f;
    gm->baseActionPoints = 3;
    gm->maxActionPoints = 15;
    gm->currentActionPoints = 0;
    gm->hasRolledThisTurn = false;
    gm->eventTimer = 0.0f;
    gm->eventCount = Event_LoadAll("assets/events", gm->events, 64);
    gm->resultCount = Result_LoadAll("assets/results", gm->results, 128);
    // Tambien puede haber resultados en la carpeta de eventos
    gm->resultCount += Result_LoadAll("assets/events", &gm->results[gm->resultCount], 128 - gm->resultCount);
    gm->activeEvent = NULL;
}

void GameMaster_StartTurn(GameMaster* gm, Army* playerArmy) {
    if (!gm || !playerArmy) return;
    gm->phase = PHASE_PLAYER;
    gm->currentActionPoints = 0;
    gm->hasRolledThisTurn = false;
    gm->eventTimer = 0.0f;
}

void GameMaster_ProcessEnemyTurn(GameMaster* gm, Map* map) {
    if (!gm || !map) return;
    // Lógica futura: mover enemigos, spanwear, etc.
    // Por ahora solo pasa la fase.
}

void GameMaster_EndTurn(GameMaster* gm, Map* map, Army* playerArmy) {
    if (!gm || !map || !playerArmy) return;

    // Incrementar tiempo
    gm->timeOfDay++;
    if (gm->timeOfDay > TIME_NIGHT) {
        gm->timeOfDay = TIME_MORNING;
        gm->currentDay++;
        gm->infamy += 1.0f; // Sube la infamia levemente al pasar un día
        if (gm->infamy > 100.0f) gm->infamy = 100.0f;
    }
    gm->currentTurn++;

    // Fase del enemigo
    gm->phase = PHASE_ENEMY;
    GameMaster_ProcessEnemyTurn(gm, map);

    // Volver al jugador
    GameMaster_StartTurn(gm, playerArmy);
}

static bool Event_MatchesContext(const EventDef* event, const EventContext* context) {
    if (event->trigger != context->trigger && event->trigger != TRIGGER_RANDOM) return false;
    
    if (event->timeOfDay != -1 && event->timeOfDay != context->timeOfDay) return false;
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
    
    // Debe ser adyacente
    if (HexCoord_Distance(playerArmy->position, targetTile->coord) > 1) return false;

    // Coste de PA según el bioma
    BiomeProperties props = Biome_GetProperties(targetTile->biome);
    int apCost = props.actionPointCost;

    if (gm->currentActionPoints >= apCost) {
        gm->currentActionPoints -= apCost;
        playerArmy->position = targetTile->coord;
        
        // Consumir recursos (simplificado)
        playerArmy->agua_disponible -= 5.0f * props.waterConsumption;
        playerArmy->comida_disponible -= 5.0f * props.foodConsumption;
        if (playerArmy->agua_disponible < 0) playerArmy->agua_disponible = 0;
        if (playerArmy->comida_disponible < 0) playerArmy->comida_disponible = 0;

                        // Preparar el contexto del evento
        EventContext ctx;
        ctx.timeOfDay = gm->timeOfDay;
        ctx.infamy = gm->infamy;
        ctx.gold = playerArmy->monedas;
        ctx.food = (int)playerArmy->comida_disponible;
        ctx.water = (int)playerArmy->agua_disponible;
        ctx.armySize = Army_GetTotalSoldiers(playerArmy);
        
        ctx.trigger = TRIGGER_RANDOM;
        if (targetTile->isRoad) ctx.trigger = TRIGGER_ROAD;
        if (targetTile->structure.type != STRUCT_NONE) {
            switch(targetTile->structure.type) {
                case STRUCT_TOWN_MARKET: ctx.trigger = TRIGGER_MARKET; break;
                case STRUCT_TOWN_HOUSES: ctx.trigger = TRIGGER_HOUSE; break;
                case STRUCT_TOWN_CASTLE: ctx.trigger = TRIGGER_CASTLE; break;
                case STRUCT_TOWN_ARMORY: ctx.trigger = TRIGGER_ARMORY; break;
                default: ctx.trigger = TRIGGER_CITY; break;
            }
        }
        if (gm->timeOfDay == TIME_NIGHT && ctx.trigger == TRIGGER_RANDOM) {
            ctx.trigger = TRIGGER_NIGHT;
        }

        EventDef* triggeredEvent = GameMaster_FindTriggeredEvent(gm, &ctx);
        if (triggeredEvent) {
            gm->phase = PHASE_EVENT;
            gm->activeEvent = triggeredEvent;
            gm->eventTimer = 0.0f;
        }
        return true;
    }

    return false;
}

int GameMaster_GetVisionModifier(const GameMaster* gm) {
    if (!gm) return 0;
    // De noche, la visión baja en 1
    if (gm->timeOfDay == TIME_NIGHT) return -1;
    return 0;
}




void GameMaster_ApplyDiceRoll(GameMaster* gm, int diceTotal) {
    if (!gm) return;
    gm->currentActionPoints = gm->baseActionPoints + diceTotal;
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




