#include "models/combat.h"
#include <math.h>
#include "models/modifier.h"
#include <stddef.h>
#include "controllers/game_master.h"

void Combat_Init(CombatContext* ctx, struct GameMaster* gm, Army* army, EnemySquad* enemy) {
    if (!ctx || !army || !enemy) return;
    ctx->state = COMBAT_STATE_INIT;
    ctx->gm = gm;
    ctx->playerArmy = army;
    ctx->enemy = enemy;
    
    ctx->evalRoll = 0;
    ctx->planRoll = 0;
    ctx->execRoll = 0;
    ctx->selectedPlan = PLAN_NONE;
    
    ctx->fleeCostCattle = 0;
    ctx->fleeCostFood = 0;
    ctx->fleeCostMorale = 0;
    
    ctx->totalCasualtyWeight = 0;
    ctx->casualtiesInitiates = 0;
    ctx->casualtiesSoldiers = 0;
    ctx->casualtiesSergeants = 0;
    
    ctx->lootGold = 0;
    ctx->lootFood = 0;
    
    ModContext mCtx = { &ctx->gm->modRegistry, army, enemy, ctx, 0, 0 };
    ModAction actions[8];
    Modifier_CollectActions(HOOK_COMBAT_START, &mCtx, actions, 8);
    ctx->state = COMBAT_STATE_EVALUATION;
    ctx->round = 1;
    ctx->endReason = COMBAT_END_NONE;
    ctx->playerInitialWeight = Army_GetTotalWeight(army);
    ctx->playerInitialMorale = army->moral;
}

void Combat_RollEvaluation(CombatContext* ctx, int dice1, int dice2) {
    if (ctx->state != COMBAT_STATE_EVALUATION) return;
    
    ctx->evalRoll = dice1 + dice2;
    ModContext mCtx = { &ctx->gm->modRegistry, ctx->playerArmy, ctx->enemy, ctx, 0, 0 };
    ctx->evalRoll = Modifier_Query(HOOK_EVAL_ROLL, EFFECT_EVALUATION, EFFECT_SIDE_SELF, &mCtx, ctx->evalRoll, NULL);
    
    // Evaluar y revelar
    if (ctx->evalRoll >= 6 && ctx->evalRoll <= 8) {
        Enemy_RevealTags(ctx->enemy, 1); // Mock: reveal 1
    } else if (ctx->evalRoll >= 9) {
        Enemy_RevealTags(ctx->enemy, 0xFFFFFFFF); // Reveal all
    }
    // ctx->evalMargin is no longer strictly used for tags, we use evalRoll directly.
    ctx->evalMargin = ctx->evalRoll; 
    
    ModAction actions[8];
    Modifier_CollectActions(HOOK_EVAL_REVEAL, &mCtx, actions, 8);
    
    ctx->state = COMBAT_STATE_PLANNING;
}

void Combat_RollPlanning(CombatContext* ctx, int dice1, int dice2) {
    if (ctx->state != COMBAT_STATE_PLANNING) return;
    
    ctx->planRoll = dice1 + dice2;
    ModContext mCtx = { &ctx->gm->modRegistry, ctx->playerArmy, ctx->enemy, ctx, 0, 0 };
    ctx->planRoll = Modifier_Query(HOOK_PLAN_ROLL, EFFECT_PLANNING, EFFECT_SIDE_SELF, &mCtx, ctx->planRoll, NULL);
    
    // Unlock plans
    ctx->canSkirmish = true;
    ctx->canAssault = (ctx->planRoll >= 6);
    ctx->canPikes = (ctx->planRoll >= 8);
    ctx->canFlank = (ctx->planRoll >= 9 && Enemy_IsTagKnown(ctx->enemy, TAG_EXPOSED_FLANK)); // Mock
    
    ModAction actions[8];
    Modifier_CollectActions(HOOK_PLAN_OPTIONS, &mCtx, actions, 8);
    
    ctx->state = COMBAT_STATE_SELECT_PLAN;
}

void Combat_SelectPlan(CombatContext* ctx, CombatPlan plan) {
    if (ctx->state != COMBAT_STATE_SELECT_PLAN) return;
    
    ctx->selectedPlan = plan;
    
    if (plan == PLAN_FLEE) {
        ctx->state = COMBAT_STATE_RESOLUTION;
    } else {
        ctx->state = COMBAT_STATE_EXECUTION;
    }
}

void Combat_RollExecution(CombatContext* ctx, int dice1, int dice2) {
    if (ctx->state != COMBAT_STATE_EXECUTION) return;
    
    ctx->execRoll = dice1 + dice2;
    
    int playerPlanMods = 0;
    if (ctx->selectedPlan == PLAN_ASSAULT) playerPlanMods = 2;
    else if (ctx->selectedPlan == PLAN_PIKES) playerPlanMods = -1;
    else if (ctx->selectedPlan == PLAN_FLANK) playerPlanMods = 4;
    
    ArmyStats stats = Army_GetStats(ctx->playerArmy, 2.0f);
    float playerForce = stats.ejecucion;
    if (playerForce < 1.0f) playerForce = 1.0f;
    float enemyForce = (float)ctx->enemy->baseStrength;
    if (enemyForce < 1.0f) enemyForce = 1.0f;
    
    float ratio = playerForce / enemyForce;
    ctx->forceAdvantage = (int)roundf(log2f(ratio) * 3.0f);
    if (ctx->forceAdvantage > 8) ctx->forceAdvantage = 8;
    if (ctx->forceAdvantage < -8) ctx->forceAdvantage = -8;
    
    ctx->playerScore = ctx->execRoll + ctx->forceAdvantage + playerPlanMods;
    
    ModContext mCtx = { &ctx->gm->modRegistry, ctx->playerArmy, ctx->enemy, ctx, 0, 0 };
    ctx->playerScore = Modifier_Query(HOOK_EXEC_ROLL, EFFECT_EXECUTION, EFFECT_SIDE_SELF, &mCtx, ctx->playerScore, &ctx->breakdown);
    
    int enemyRoll = 7;
    int enemyPlanMods = 0;
    if (ctx->enemyPlan == PLAN_ASSAULT) enemyPlanMods = 2;
    
    ctx->enemyScore = enemyRoll + enemyPlanMods;
    ctx->enemyScore = Modifier_Query(HOOK_EXEC_ROLL, EFFECT_EXECUTION, EFFECT_SIDE_OPPONENT, &mCtx, ctx->enemyScore, NULL);
    
    ctx->margin = ctx->enemyScore - ctx->playerScore;
    
    int armyWeight = Army_GetTotalWeight(ctx->playerArmy);
    
    if (ctx->margin > 0) {
        float casualtyPct = ctx->margin * 0.05f;
        if (casualtyPct > 1.0f) casualtyPct = 1.0f;
        ctx->totalCasualtyWeight = (int)(armyWeight * casualtyPct);
    } else {
        ctx->totalCasualtyWeight = 0;
        float dmg = (float)(ctx->margin * 5);
        if (ctx->selectedPlan == PLAN_SKIRMISH) dmg *= 0.7f;
        ctx->enemy->baseStrength += (int)dmg;
        if (ctx->enemy->baseStrength < 0) ctx->enemy->baseStrength = 0;
    }
    
    if (ctx->selectedPlan == PLAN_SKIRMISH) ctx->totalCasualtyWeight = (int)(ctx->totalCasualtyWeight * 0.7f);
    else if (ctx->selectedPlan == PLAN_PIKES) ctx->totalCasualtyWeight = (int)(ctx->totalCasualtyWeight * 0.6f);
    else if (ctx->selectedPlan == PLAN_ASSAULT) ctx->totalCasualtyWeight = (int)(ctx->totalCasualtyWeight * 1.2f);
    
    ctx->totalCasualtyWeight = Modifier_Query(HOOK_EXEC_DAMAGE, EFFECT_CASUALTIES, EFFECT_SIDE_SELF, &mCtx, ctx->totalCasualtyWeight, NULL);
    
    Combat_DistributeCasualties(ctx);
    
    // Moral drop
    float moralDrop = 0.0f;
    if (armyWeight > 0) moralDrop = ((float)ctx->totalCasualtyWeight / armyWeight) * 100.0f;
    ctx->playerArmy->moral -= moralDrop;
    if (ctx->playerArmy->moral < 0.0f) ctx->playerArmy->moral = 0.0f;
    
    ctx->state = COMBAT_STATE_RESOLUTION;
}

void Combat_DistributeCasualties(CombatContext* ctx) {
    ctx->casualtiesInitiates = 0;
    ctx->casualtiesSoldiers = 0;
    ctx->casualtiesSergeants = 0;
    
    int remainingWeight = ctx->totalCasualtyWeight;
    
    while (remainingWeight > 0) {
        if (ctx->playerArmy->iniciados - ctx->casualtiesInitiates > 0) {
            ctx->casualtiesInitiates++;
            remainingWeight -= 1;
        } else if (ctx->playerArmy->soldados - ctx->casualtiesSoldiers > 0) {
            ctx->casualtiesSoldiers++;
            remainingWeight -= 2;
        } else if (ctx->playerArmy->sargentos - ctx->casualtiesSergeants > 0) {
            ctx->casualtiesSergeants++;
            remainingWeight -= 5;
        } else {
            break;
        }
    }
    
    ModContext mCtx = { &ctx->gm->modRegistry, ctx->playerArmy, ctx->enemy, ctx, 0, 0 };
    ModAction actions[8];
    Modifier_CollectActions(HOOK_CASUALTIES, &mCtx, actions, 8);
}

void Combat_ApplyResolution(CombatContext* ctx) {
    if (ctx->state != COMBAT_STATE_RESOLUTION) return;
    
    if (ctx->selectedPlan == PLAN_FLEE) {
        ctx->playerArmy->position = ctx->playerArmy->previousPosition;
        ctx->playerArmy->comida_disponible -= ctx->fleeCostFood;
        ctx->playerArmy->ganado -= ctx->fleeCostCattle;
        if (ctx->playerArmy->comida_disponible < 0) ctx->playerArmy->comida_disponible = 0;
        if (ctx->playerArmy->ganado < 0) ctx->playerArmy->ganado = 0;
        ctx->endReason = COMBAT_END_FLEE;
        ctx->state = COMBAT_STATE_FINISHED;
    } else {
        ctx->playerArmy->iniciados -= ctx->casualtiesInitiates;
        ctx->playerArmy->soldados -= ctx->casualtiesSoldiers;
        ctx->playerArmy->sargentos -= ctx->casualtiesSergeants;
        
        if (ctx->playerArmy->iniciados < 0) ctx->playerArmy->iniciados = 0;
        if (ctx->playerArmy->soldados < 0) ctx->playerArmy->soldados = 0;
        if (ctx->playerArmy->sargentos < 0) ctx->playerArmy->sargentos = 0;
        
        // Check end conditions
        int currentWeight = Army_GetTotalWeight(ctx->playerArmy);
        float enemyBreakPct = 0.40f;
        if (Enemy_HasTag(ctx->enemy, TAG_DISORGANIZED)) enemyBreakPct = 0.60f;
        
        if (currentWeight <= 0) {
            ctx->endReason = COMBAT_END_ANNIHILATION;
        } else if (ctx->enemy->baseStrength <= ctx->enemy->maxStrength * enemyBreakPct) {
            ctx->endReason = COMBAT_END_VICTORY_BREAK;
        } else if (currentWeight < ctx->playerInitialWeight * 0.35f || ctx->playerArmy->moral <= 0) {
            ctx->endReason = COMBAT_END_PLAYER_ROUT;
        } else if (ctx->round >= 4) {
            ctx->endReason = COMBAT_END_STALEMATE;
        }
        
        if (ctx->endReason != COMBAT_END_NONE) {
            if (ctx->endReason == COMBAT_END_VICTORY_BREAK) {
                ctx->playerArmy->monedas += ctx->lootGold;
                ctx->playerArmy->comida_disponible += ctx->lootFood;
                ctx->enemy->isActive = false;
            } else if (ctx->endReason == COMBAT_END_PLAYER_ROUT || ctx->endReason == COMBAT_END_STALEMATE) {
                ctx->playerArmy->position = ctx->playerArmy->previousPosition;
            }
            ctx->state = COMBAT_STATE_FINISHED;
        } else {
            ctx->round++;
            ctx->state = COMBAT_STATE_SELECT_PLAN;
        }
    }
    
    ModContext mCtx = { &ctx->gm->modRegistry, ctx->playerArmy, ctx->enemy, ctx, 0, 0 };
    ModAction actions[8];
    Modifier_CollectActions(HOOK_COMBAT_END, &mCtx, actions, 8);
}



