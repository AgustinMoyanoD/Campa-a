#ifndef GAME_MASTER_H
#define GAME_MASTER_H

#include "models/army.h"
#include "models/map_generator.h"
#include "models/enemy.h"
#include "models/event.h"
#include "models/modifier.h"
#include "models/combat.h"
#include "models/day_clock.h"
#include "models/city.h"
#include "models/ration.h"
#include <stdbool.h>

typedef enum {
    PHASE_MENU,
    PHASE_SETUP,
    PHASE_PLAYER,
    PHASE_ENEMY,
    PHASE_EVENT,
    PHASE_DRAFT,
    PHASE_GAME_OVER,
    PHASE_VICTORY,
    PHASE_CITY
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
    int cattle;
    int daysWithoutFood;
    int daysWithoutWater;
    int factionId;
} EventContext;

#define MAX_FLAGS 64
#define MAX_FLAG_LEN 32

typedef struct GameMaster {
    int currentTurn;
    int currentDay;
    TimeOfDay timeOfDay;
    int biome;
    GamePhase phase;
    float infamy; // 0.0 to 100.0
    
    EventDef events[64];
    int eventCount;
    EventResult results[256];
    int resultCount;
    EventResult* activeResult;
    EventDef* activeEvent;
    
    ModifierRegistry modRegistry;

    EnemySquad enemies[64];
    int enemyCount;

    bool inCombat;
    int pendingCombatEnemy;
    struct CombatContext activeCombat;

    int baseActionPoints;
    bool hasRolledThisTurn;
    float eventTimer;

    int maxActionPoints;
    int currentActionPoints;

    /* Reloj, Supervivencia y RNG (I4, I5, I7) */
    DayClock clock;
    Rng rng;
    DayReport lastDayReport;
    bool hasDayReport;

    /* Campamento y Noche (Fase 2) */
    bool showCampDecision;
    float campDecisionTimer;
    bool isEncamped;
    bool spentNightAtInn;
    bool hasForagedToday;
    bool rationPending;
    RationPlan pendingRationPlan;

    /* Cazador y PosiciÃ³n de Retaguardia (Fase 3) */
    int lastHunterSpawnDay;
    HexCoord lastCampPos;

    /* Ciudades y Estructuras (Fase 4) */
    bool canInteractStructure;
    HexCoord interactStructureCoord;
    CityContext activeCity;

    /* Transicion e interaccion de eventos en estructuras ([E]) */
    bool pendingCityAfterEvent;
    HexCoord pendingCityCoord;
    bool pendingCityBlocked;
    int currentEventFactionId;

    /* Banderas Narrativas Persistentes (Flags / Memoria) */
    char flags[MAX_FLAGS][MAX_FLAG_LEN];
    int flagCount;

    /* Facciones y Asedio (Fase 5) */
    Faction factions[3];
    bool inSiegeCombat;
    HexCoord siegeTargetCoord;
    int siegeFactionId;
} GameMaster;

void GameMaster_Init(GameMaster* gm);
void GameMaster_StartCampaign(GameMaster* gm, Map* map, Army* playerArmy, const ArmySetupConfig* setup, int provinceLevel);
void GameMaster_TransitionProvince(GameMaster* gm, Map* map, Army* playerArmy, int provinceLevel);
void GameMaster_InitFactions(GameMaster* gm);
void GameMaster_SpawnTestEnemies(GameMaster* gm, Map* map, HexCoord playerPos);
void GameMaster_StartTurn(GameMaster* gm, Army* playerArmy);
void GameMaster_EndTurn(GameMaster* gm, Map* map, Army* playerArmy);
bool GameMaster_AttemptMove(GameMaster* gm, Map* map, Army* playerArmy, HexTile* targetTile);
void GameMaster_ProcessEnemyTurn(GameMaster* gm, Map* map, Army* playerArmy);
int GameMaster_GetVisionModifier(const GameMaster* gm, const Army* army);
bool GameMaster_Forage(GameMaster* gm, Army* army, Map* map);
void GameMaster_ChooseCamp(GameMaster* gm, Map* map, Army* playerArmy);
void GameMaster_ChooseNightMarch(GameMaster* gm, Map* map, Army* playerArmy);
void GameMaster_ChooseInn(GameMaster* gm, Map* map, Army* playerArmy);
void GameMaster_OpenCity(GameMaster* gm, Map* map, Army* playerArmy);
void GameMaster_CloseCity(GameMaster* gm, Map* map, Army* playerArmy);
bool GameMaster_InteractStructure(GameMaster* gm, Map* map, Army* playerArmy);
void GameMaster_StartSiege(GameMaster* gm, Map* map, Army* playerArmy, HexCoord targetCoord);
void GameMaster_ResolveSiegeVictory(GameMaster* gm, Map* map, Army* playerArmy, HexCoord targetCoord);

/* Reparto de Raciones (Paso 4) */
bool GameMaster_RationPending(const GameMaster* gm);
bool GameMaster_SubmitRation(GameMaster* gm, Army* playerArmy, const RationPlan* plan);
int  GameMaster_SlaughterCattle(GameMaster* gm, Army* playerArmy, int heads);

/* Banderas Narrativas */
bool GameMaster_HasFlag(const GameMaster* gm, const char* flag);
void GameMaster_SetFlag(GameMaster* gm, const char* flag);
void GameMaster_ClearFlag(GameMaster* gm, const char* flag);

/* Opinion de Facciones */
void GameMaster_ApplyFactionOpinion(GameMaster* gm, const char* targetStr, int delta);

EventDef* GameMaster_FindTriggeredEvent(GameMaster* gm, const EventContext* context);

EventResult* GameMaster_FindResult(GameMaster* gm, const char* id);

#endif /* GAME_MASTER_H */


void GameMaster_ApplyDiceRoll(GameMaster* gm, Army* army, int diceTotal);














