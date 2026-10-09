#ifndef GAME_MASTER_H
#define GAME_MASTER_H

#include "models/army.h"
#include "models/map_generator.h"
#include "models/enemy.h"
#include "models/event.h"
#include <stdbool.h>

typedef enum {
    TIME_MORNING = 0,
    TIME_MIDDAY,
    TIME_AFTERNOON,
    TIME_NIGHT
} TimeOfDay;

typedef enum {
    PHASE_PLAYER,
    PHASE_ENEMY,
    PHASE_EVENT
} GamePhase;

typedef struct {
    EventTrigger trigger;
    TimeOfDay timeOfDay;
    int biome;
    float infamy;
    int gold;
    int food;
    int water;
    int armySize;
} EventContext;

typedef struct {
    int currentTurn;
    int currentDay;
    TimeOfDay timeOfDay;
    int biome;
    GamePhase phase;
    float infamy; // 0.0 to 100.0
    
    EventDef events[64];
    int eventCount;
    EventResult results[128];
    int resultCount;
    EventResult* activeResult;
    EventDef* activeEvent;

    int baseActionPoints;
    bool hasRolledThisTurn;
    float eventTimer;

    int maxActionPoints;
    int currentActionPoints;
} GameMaster;

void GameMaster_Init(GameMaster* gm);
void GameMaster_StartTurn(GameMaster* gm, Army* playerArmy);
void GameMaster_EndTurn(GameMaster* gm, Map* map, Army* playerArmy);
bool GameMaster_AttemptMove(GameMaster* gm, Map* map, Army* playerArmy, HexTile* targetTile);
void GameMaster_ProcessEnemyTurn(GameMaster* gm, Map* map);
int GameMaster_GetVisionModifier(const GameMaster* gm);

EventDef* GameMaster_FindTriggeredEvent(GameMaster* gm, const EventContext* context);

EventResult* GameMaster_FindResult(GameMaster* gm, const char* id);

#endif /* GAME_MASTER_H */


void GameMaster_ApplyDiceRoll(GameMaster* gm, int diceTotal);







