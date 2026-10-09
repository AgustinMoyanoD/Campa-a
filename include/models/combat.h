#ifndef COMBAT_H
#define COMBAT_H

#include "models/army.h"
#include "models/enemy.h"
#include "models/modifier.h"

// Fases de la maquina de estados de combate
typedef enum {
    COMBAT_STATE_INIT = 0,
    COMBAT_STATE_EVALUATION,
    COMBAT_STATE_PLANNING,
    COMBAT_STATE_SELECT_PLAN,
    COMBAT_STATE_EXECUTION,
    COMBAT_STATE_RESOLUTION,
    COMBAT_STATE_FINISHED
} CombatState;

typedef enum {
    COMBAT_END_NONE = 0,
    COMBAT_END_VICTORY_BREAK,
    COMBAT_END_PLAYER_ROUT,
    COMBAT_END_STALEMATE,
    COMBAT_END_FLEE,
    COMBAT_END_ANNIHILATION
} CombatEndReason;

typedef enum {
    PLAN_ASSAULT = 0,
    PLAN_SKIRMISH,
    PLAN_FLEE,
    PLAN_PIKES,
    PLAN_FLANK,
    PLAN_NONE
} CombatPlan;

// Forward decl
struct GameMaster;

typedef struct CombatContext {
    CombatState state;
    CombatEndReason endReason;
    int round;
    int playerInitialWeight;
    float playerInitialMorale;
    struct GameMaster* gm;
    Army* playerArmy;
    EnemySquad* enemy;
    
    // Resultados de los dados (2d6)
    int evalRoll;
    int evalMargin;
    int planRoll;
    int execRoll;
    int forceAdvantage;
    bool canAssault;
    bool canSkirmish;
    bool canFlank;
    bool canPikes;
    int playerScore;
    int enemyScore;
    int margin;
    ModBreakdown breakdown;

    // Decision del jugador
    CombatPlan selectedPlan;
    CombatPlan enemyPlan;
    
    // Costo de huir (calculado en fase de evaluacion/planificacion)
    int fleeCostCattle;
    int fleeCostFood;
    int fleeCostMorale;
    
    // Bajas calculadas en ejecucion
    int totalCasualtyWeight;
    int casualtiesInitiates;
    int casualtiesSoldiers;
    int casualtiesSergeants;
    
    // Recompensas
    int lootGold;
    int lootFood;
    
} CombatContext;

void Combat_Init(CombatContext* ctx, struct GameMaster* gm, Army* army, EnemySquad* enemy);
void Combat_RollEvaluation(CombatContext* ctx, int dice1, int dice2);
void Combat_RollPlanning(CombatContext* ctx, int dice1, int dice2);
void Combat_SelectPlan(CombatContext* ctx, CombatPlan plan);
void Combat_RollExecution(CombatContext* ctx, int dice1, int dice2);
void Combat_ApplyResolution(CombatContext* ctx);
void Combat_DistributeCasualties(CombatContext* ctx);

#endif


