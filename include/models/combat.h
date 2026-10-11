#ifndef COMBAT_H
#define COMBAT_H

#include "models/army.h"
#include "models/enemy.h"
#include "models/modifier.h"
#include "models/combat_log.h"
#include "models/rng.h"

typedef enum {
    COMBAT_STATE_DEPLOY = 0,
    COMBAT_STATE_EVALUATION,
    COMBAT_STATE_PLANNING,
    COMBAT_STATE_EXECUTION,
    COMBAT_STATE_ROUND_CLOSE,
    COMBAT_STATE_END,
    COMBAT_STATE_RECOUNT,
    COMBAT_STATE_DONE
} CombatState;

typedef enum {
    COMBAT_END_NONE = 0,
    COMBAT_END_VICTORY,
    COMBAT_END_DEFEAT,
    COMBAT_END_STALEMATE,
    COMBAT_END_FLEE
} CombatEndReason;

typedef enum {
    PLAN_ASSAULT = 0,
    PLAN_SKIRMISH,
    PLAN_FLEE,
    PLAN_PIKES,
    PLAN_FLANK,
    PLAN_NONE
} CombatPlan;

struct GameMaster;

typedef struct CombatContext {
    CombatState state;
    CombatEndReason endReason;
    int round;
    
    // Variables temporales (D1, D5)
    float playerHealth;
    float playerInitialHealth;
    int playerInitialWeight;
    
    float enemyHealth;
    float enemyInitialHealth;
    
    // Bajas acumuladas durante el combate (fuera de accion)
    int outOfActionInitiates;
    int outOfActionSoldiers;
    int outOfActionSergeants;
    
    int enemyOutOfAction;
    
    // Muertos calculados en el Recuento
    int deadInitiates;
    int deadSoldiers;
    int deadSergeants;
    
    // Costos de huida
    int fleeCostCattle;
    int fleeCostFood;
    int fleeCostMorale;
    
    // Botin (solo si victoria)
    int lootGold;
    int lootFood;
    
    CombatLog log;
    struct GameMaster* gm;
    Army* playerArmy; // Referencia al real (NO TOCAR HASTA COMMIT)
    EnemySquad* enemy;
    
    // Dados y planes
    int evalRoll;
    int evalMargin;
    int planRoll;
    int execRoll;
    int forceAdvantage;
    bool canAssault;
    bool canSkirmish;
    bool canFlank;
    bool canPikes;
    
    CombatPlan selectedPlan;
    CombatPlan previousPlan;
    int enemyIntent; /* 0=CHARGE, 1=HOLD, 2=MANEUVER */
    bool intentRevealed;
    
    int playerScore;
    int enemyScore;
    int margin;
    bool criticalHit;
    bool criticalFumble;
    ModBreakdown breakdown;

    /* Puntos de contacto (Sección 11) */
    bool ambushed;
    int initiator;  /* 0 = PLAYER, 1 = ENEMY */
    int maxRounds;
} CombatContext;

void Combat_Init(CombatContext* ctx, struct GameMaster* gm, Army* army, EnemySquad* enemy);
void Combat_ComputeLoot(CombatContext* ctx, Rng* rng);
void Combat_RollEvaluation(CombatContext* ctx, int dice1, int dice2);
void Combat_RollPlanning(CombatContext* ctx, int dice1, int dice2);
void Combat_UseCommandToken_Add(CombatContext* ctx);
void Combat_UseCommandToken_Reroll(CombatContext* ctx, int dice1, int dice2);
void Combat_SelectPlan(CombatContext* ctx, CombatPlan plan);
void Combat_RollExecution(CombatContext* ctx, int dice1, int dice2);
void Combat_RoundClose(CombatContext* ctx);
void Combat_Recount(CombatContext* ctx);
void Combat_Commit(CombatContext* ctx);

#endif
