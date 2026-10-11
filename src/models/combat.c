#include "models/combat.h"
#include "controllers/game_master.h"
#include "models/balance_config.h"
#include "models/rng.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

void Combat_Init(CombatContext* ctx, struct GameMaster* gm, Army* army, EnemySquad* enemy) {
    bool wasAmbushed = ctx->ambushed;
    int wasInitiator = ctx->initiator;
    memset(ctx, 0, sizeof(CombatContext));
    ctx->ambushed = wasAmbushed;
    ctx->initiator = wasInitiator;
    ctx->gm = gm;
    ctx->playerArmy = army;
    ctx->enemy = enemy;
    ctx->round = 1;

    const BalanceConfig* cfg = Balance_GetGlobal();
    ctx->maxRounds = (cfg->maxCombatRounds > 0) ? cfg->maxCombatRounds : 4;
    
    ctx->playerInitialHealth = army->salud;
    ctx->playerHealth = army->salud;
    ctx->playerInitialWeight = Army_GetTotalWeight(army);
    
    // El enemigo siempre arranca en 100 de salud por defecto (D3)
    ctx->enemyInitialHealth = 100.0f;
    ctx->enemyHealth = 100.0f;
    
    if (ctx->log.count < MAX_STEPS) {
        ctx->log.steps[ctx->log.count].type = STEP_DEPLOY;
        ctx->log.steps[ctx->log.count].as.deploy.totalTroops = army->iniciados + army->soldados + army->sargentos;
        ctx->log.steps[ctx->log.count].as.deploy.initialHealth = ctx->playerInitialHealth;
        ctx->log.count++;
    }
    
    if (ctx->ambushed) {
        ctx->intentRevealed = false;
        if (ctx->enemy) ctx->enemy->infoLevel = 0;
        ctx->state = COMBAT_STATE_PLANNING;
    } else {
        ctx->state = COMBAT_STATE_EVALUATION;
    }
}

void Combat_RollEvaluation(CombatContext* ctx, int dice1, int dice2) {
    if (ctx->state != COMBAT_STATE_EVALUATION) return;
    if (ctx->ambushed) {
        ctx->state = COMBAT_STATE_PLANNING;
        return;
    }
    
    ctx->evalRoll = dice1 + dice2;
    int rollScore = ctx->evalRoll;
    
    // Hook de modifiers opcional (no definido a fondo en el GDD pero valido)
    ModContext mCtx = { &ctx->gm->modRegistry, ctx->playerArmy, ctx->enemy, ctx, 0, 0 };
    rollScore = Modifier_Query(HOOK_EVAL_ROLL, EFFECT_EVALUATION, EFFECT_SIDE_SELF, &mCtx, rollScore, NULL);
    
    if (ctx->log.count < MAX_STEPS) {
        ctx->log.steps[ctx->log.count].type = STEP_ROLL;
        ctx->log.steps[ctx->log.count].as.roll.d1 = dice1;
        ctx->log.steps[ctx->log.count].as.roll.d2 = dice2;
        ctx->log.steps[ctx->log.count].as.roll.total = rollScore;
        ctx->log.count++;
    }
    
    // Calculo de nivel de informacion
    int info = 0;
    if (rollScore >= 8) info = 1;
    if (rollScore >= 10) info = 2;
    
    if (info > 0) {
        ctx->enemy->infoLevel = info;
    }
    ctx->intentRevealed = (ctx->enemy->infoLevel >= 1);
    
    if (ctx->log.count < MAX_STEPS) {
        ctx->log.steps[ctx->log.count].type = STEP_REVEAL;
        ctx->log.steps[ctx->log.count].as.reveal.infoLevel = 0;
        ctx->log.count++;
    }
    
    ctx->state = COMBAT_STATE_PLANNING;
}

void Combat_RollPlanning(CombatContext* ctx, int dice1, int dice2) {
    if (ctx->state != COMBAT_STATE_PLANNING) return;
    
    ctx->planRoll = dice1 + dice2;
    int rollScore = ctx->planRoll;
    
    if (ctx->ambushed && ctx->round == 1) {
        rollScore -= 2;
        if (rollScore < 2) rollScore = 2;
    }

    ModContext mCtx = { &ctx->gm->modRegistry, ctx->playerArmy, ctx->enemy, ctx, 0, 0 };
    rollScore = Modifier_Query(HOOK_PLAN_ROLL, EFFECT_PLANNING, EFFECT_SIDE_SELF, &mCtx, rollScore, NULL);
    
    if (ctx->log.count < MAX_STEPS) {
        ctx->log.steps[ctx->log.count].type = STEP_ROLL;
        ctx->log.steps[ctx->log.count].as.roll.d1 = dice1;
        ctx->log.steps[ctx->log.count].as.roll.d2 = dice2;
        ctx->log.steps[ctx->log.count].as.roll.total = rollScore;
        ctx->log.count++;
    }
    
    // Calcular costo de huida (Leido de config provisorio: 15% de comida, 1 moral)
    // Para simplificar, hardcoded values from balance.cfg spec
    int foodLossPct = 15;
    ctx->fleeCostFood = (ctx->playerArmy->comida_disponible * foodLossPct) / 100;
    ctx->fleeCostMorale = 1;
    ctx->fleeCostCattle = 0;
    
    // Unlock plans
    ctx->canSkirmish = true;
    ctx->canAssault = (rollScore >= 6);
    ctx->canPikes = (rollScore >= 8);
    ctx->canFlank = (rollScore >= 9 && Enemy_IsTagKnown(ctx->enemy, TAG_EXPOSED_FLANK));
    
    unsigned int mask = 0;
    mask |= (1 << PLAN_FLEE);
    if (ctx->canSkirmish) mask |= (1 << PLAN_SKIRMISH);
    if (ctx->canAssault) mask |= (1 << PLAN_ASSAULT);
    if (ctx->canPikes) mask |= (1 << PLAN_PIKES);
    if (ctx->canFlank) mask |= (1 << PLAN_FLANK);
    
    if (ctx->log.count < MAX_STEPS) {
        ctx->log.steps[ctx->log.count].type = STEP_PLANS_UNLOCKED;
        ctx->log.steps[ctx->log.count].as.plans.unlockedMask = mask;
        ctx->log.count++;
    }
    
    ctx->state = COMBAT_STATE_EXECUTION; // Espera, la vista debe setear selectedPlan
}

void Combat_UseCommandToken_Add(CombatContext* ctx) {
    if (ctx->state != COMBAT_STATE_EXECUTION) return;
    if (ctx->playerArmy->commandTokens <= 0) return;
    ctx->playerArmy->commandTokens--;
    
    ctx->planRoll += 2;
    
    int rollScore = ctx->planRoll;
    ModContext mCtx = { &ctx->gm->modRegistry, ctx->playerArmy, ctx->enemy, ctx, 0, 0 };
    rollScore = Modifier_Query(HOOK_PLAN_ROLL, EFFECT_PLANNING, EFFECT_SIDE_SELF, &mCtx, rollScore, NULL);
    
    ctx->canAssault = (rollScore >= 6);
    ctx->canPikes = (rollScore >= 8);
    ctx->canFlank = (rollScore >= 9 && Enemy_IsTagKnown(ctx->enemy, TAG_EXPOSED_FLANK));
}

void Combat_UseCommandToken_Reroll(CombatContext* ctx, int dice1, int dice2) {
    if (ctx->state != COMBAT_STATE_EXECUTION) return;
    if (ctx->playerArmy->commandTokens <= 0) return;
    ctx->playerArmy->commandTokens--;
    
    ctx->planRoll = dice1 + dice2;
    
    int rollScore = ctx->planRoll;
    ModContext mCtx = { &ctx->gm->modRegistry, ctx->playerArmy, ctx->enemy, ctx, 0, 0 };
    rollScore = Modifier_Query(HOOK_PLAN_ROLL, EFFECT_PLANNING, EFFECT_SIDE_SELF, &mCtx, rollScore, NULL);
    
    ctx->canAssault = (rollScore >= 6);
    ctx->canPikes = (rollScore >= 8);
    ctx->canFlank = (rollScore >= 9 && Enemy_IsTagKnown(ctx->enemy, TAG_EXPOSED_FLANK));
}

void Combat_SelectPlan(CombatContext* ctx, CombatPlan plan) {
    // Si bien el GDD marca EXECUTION despues, el plan lo setea el input.
    ctx->selectedPlan = plan;
    
    if (ctx->log.count < MAX_STEPS) {
        ctx->log.steps[ctx->log.count].type = STEP_PLAN_CHOSEN;
        ctx->log.steps[ctx->log.count].as.plan_chosen.plan = plan;
        ctx->log.count++;
    }
}

void Combat_RollExecution(CombatContext* ctx, int dice1, int dice2) {
    if (ctx->state != COMBAT_STATE_EXECUTION) return;
    
    ctx->execRoll = dice1 + dice2;
    ctx->criticalHit = (dice1 == 6 && dice2 == 6);
    ctx->criticalFumble = (dice1 == 1 && dice2 == 1);
    
    int playerPlanMods = 0;
    if (ctx->selectedPlan == PLAN_ASSAULT) playerPlanMods = 2;
    else if (ctx->selectedPlan == PLAN_PIKES) playerPlanMods = -1;
    else if (ctx->selectedPlan == PLAN_FLANK) playerPlanMods = 4;
    
    // I2: Active troops only?
    // Calculo de fuerza con tropas activas (D4 / Sec 4)
    int actInit = ctx->playerArmy->iniciados - ctx->outOfActionInitiates;
    int actSold = ctx->playerArmy->soldados - ctx->outOfActionSoldiers;
    int actSerg = ctx->playerArmy->sargentos - ctx->outOfActionSergeants;
    
    float playerForce = (actInit * 1.0f) + (actSold * 2.0f) + (actSerg * 5.0f);
    
    // Penalizaciones por hambre, sed (salud y moral bajas)
    float moralFactor = (ctx->playerArmy->moral > 10.0f) ? (ctx->playerArmy->moral / 100.0f) : 0.1f;
    float saludFactor = (ctx->playerArmy->salud > 10.0f) ? (ctx->playerArmy->salud / 100.0f) : 0.1f;
    playerForce *= moralFactor;
    playerForce *= saludFactor;

    if (playerForce < 1.0f) playerForce = 1.0f;
    
    float enemyForce = (float)ctx->enemy->baseStrength;
    if (enemyForce < 1.0f) enemyForce = 1.0f;
    
    float ratio = playerForce / enemyForce;
    ctx->forceAdvantage = (int)roundf(log2f(ratio) * 3.0f);
    if (ctx->forceAdvantage > 8) ctx->forceAdvantage = 8;
    if (ctx->forceAdvantage < -8) ctx->forceAdvantage = -8;
    
    if (ctx->log.count < MAX_STEPS) {
        ctx->log.steps[ctx->log.count].type = STEP_ADVANTAGE;
        ctx->log.steps[ctx->log.count].as.advantage.advantage = ctx->forceAdvantage;
        ctx->log.steps[ctx->log.count].as.advantage.playerForce = (int)playerForce;
        ctx->log.steps[ctx->log.count].as.advantage.enemyForce = (int)enemyForce;
        ctx->log.count++;
    }
    
    int playerScore = ctx->execRoll + ctx->forceAdvantage + playerPlanMods;
    
    if (ctx->log.count < MAX_STEPS) {
        ctx->log.steps[ctx->log.count].type = STEP_ROLL;
        ctx->log.steps[ctx->log.count].as.roll.d1 = dice1;
        ctx->log.steps[ctx->log.count].as.roll.d2 = dice2;
        ctx->log.steps[ctx->log.count].as.roll.total = playerScore;
        ctx->log.count++;
    }
    
    ModContext mCtx = { &ctx->gm->modRegistry, ctx->playerArmy, ctx->enemy, ctx, 0, 0 };
    playerScore = Modifier_Query(HOOK_EXEC_ROLL, EFFECT_EXECUTION, EFFECT_SIDE_SELF, &mCtx, playerScore, &ctx->breakdown);
    
    int enemyRoll = 7;
    int enemyPlanMods = 0;
    int enemyScore = enemyRoll + enemyPlanMods;
    enemyScore = Modifier_Query(HOOK_EXEC_ROLL, EFFECT_EXECUTION, EFFECT_SIDE_OPPONENT, &mCtx, enemyScore, NULL);
    
    ctx->playerScore = playerScore;
    ctx->enemyScore = enemyScore;
    
    int margin = enemyScore - playerScore;
    
    float dmgFactor = 10.0f;
    if (ctx->selectedPlan == PLAN_SKIRMISH) dmgFactor *= 0.7f;
    else if (ctx->selectedPlan == PLAN_PIKES) dmgFactor *= 0.6f;
    else if (ctx->selectedPlan == PLAN_ASSAULT) dmgFactor *= 1.2f;

    // Contrajuegos de Intencion vs Plan
    if (ctx->enemyIntent == 0) { // Carga
        if (ctx->selectedPlan == PLAN_PIKES) { playerPlanMods += 3; dmgFactor *= 0.4f; }
        if (ctx->selectedPlan == PLAN_SKIRMISH) { playerPlanMods -= 2; dmgFactor *= 1.5f; }
    } else if (ctx->enemyIntent == 1) { // Asedio
        if (ctx->selectedPlan == PLAN_FLANK) { playerPlanMods += 3; dmgFactor *= 1.2f; }
        if (ctx->selectedPlan == PLAN_SKIRMISH) { playerPlanMods += 2; dmgFactor *= 0.8f; }
        if (ctx->selectedPlan == PLAN_ASSAULT) { playerPlanMods -= 3; dmgFactor *= 1.3f; }
    } else if (ctx->enemyIntent == 2) { // Repliegue
        if (ctx->selectedPlan == PLAN_ASSAULT) { playerPlanMods += 3; dmgFactor *= 1.5f; }
        if (ctx->selectedPlan == PLAN_PIKES) { playerPlanMods -= 2; dmgFactor *= 0.5f; }
    }

    if (ctx->selectedPlan != PLAN_NONE && ctx->selectedPlan == ctx->previousPlan) {
        playerPlanMods /= 2;
        // Move dmgFactor halfway towards 10.0f
        dmgFactor = 10.0f + (dmgFactor - 10.0f) * 0.5f;
    }
    
    if (ctx->criticalHit) {
        playerPlanMods += 5; // Enormous bonus
        dmgFactor *= 1.5f; // Extra damage multiplier
    } else if (ctx->criticalFumble) {
        playerPlanMods -= 5;
        dmgFactor *= 0.5f;
    }
    
    if (margin > 0) {
        // Player takes damage
        float lostHealth = (float)margin * dmgFactor;
        
        // Hook Bajas transformado en Hook Salud (Sec 11)
        lostHealth = (float)Modifier_Query(HOOK_HEALTH_DAMAGE_RECEIVED, EFFECT_CASUALTIES, EFFECT_SIDE_SELF, &mCtx, (int)lostHealth, NULL);
        
        ctx->playerHealth -= lostHealth;
        if (ctx->playerHealth < 0) ctx->playerHealth = 0;
        
        if (ctx->log.count < MAX_STEPS) {
            ctx->log.steps[ctx->log.count].type = STEP_DAMAGE;
            ctx->log.steps[ctx->log.count].as.damage.side = 0;
            ctx->log.steps[ctx->log.count].as.damage.lostHealth = lostHealth;
            ctx->log.steps[ctx->log.count].as.damage.newHealth = ctx->playerHealth;
            ctx->log.count++;
        }
    } else {
        // Enemy takes damage
        float lostHealth = (float)(-margin) * dmgFactor;
        if (lostHealth == 0.0f) lostHealth = 5.0f * dmgFactor; // Empate rompe a favor de algo de dao?
        
        ctx->enemyHealth -= lostHealth;
        if (ctx->enemyHealth < 0) ctx->enemyHealth = 0;
        
        if (ctx->log.count < MAX_STEPS) {
            ctx->log.steps[ctx->log.count].type = STEP_DAMAGE;
            ctx->log.steps[ctx->log.count].as.damage.side = 1;
            ctx->log.steps[ctx->log.count].as.damage.lostHealth = lostHealth;
            ctx->log.steps[ctx->log.count].as.damage.newHealth = ctx->enemyHealth;
            ctx->log.count++;
        }
    }
    
    ctx->state = COMBAT_STATE_ROUND_CLOSE;
}

void Combat_ComputeLoot(CombatContext* ctx, Rng* rng) {
    if (!ctx || !ctx->enemy) return;
    if (ctx->enemy->behavior == AI_BEAST) {
        ctx->lootGold = 0;
        ctx->lootFood = 0;
        return;
    }
    const BalanceConfig* cfg = Balance_GetGlobal();

    int enemyStr = ctx->enemy->baseStrength;
    if (enemyStr <= 0) enemyStr = 25;

    float baseGold = (float)enemyStr * cfg->lootGoldPerStr;
    float baseFood = (float)enemyStr * cfg->lootFoodPerStr;

    float variance = (cfg->lootVariancePct > 0.0f) ? cfg->lootVariancePct : 20.0f;
    float factorGold = 1.0f;
    float factorFood = 1.0f;

    if (rng) {
        float rollG = (float)Rng_Range(rng, (int)(-variance), (int)variance) / 100.0f;
        float rollF = (float)Rng_Range(rng, (int)(-variance), (int)variance) / 100.0f;
        factorGold += rollG;
        factorFood += rollF;
    }

    ctx->lootGold = (int)roundf(baseGold * factorGold);
    if (ctx->lootGold < 5) ctx->lootGold = 5;

    ctx->lootFood = roundf(baseFood * factorFood);
    if (ctx->lootFood < 2.0f) ctx->lootFood = 2.0f;
}

void Combat_RoundClose(CombatContext* ctx) {
    if (ctx->state != COMBAT_STATE_ROUND_CLOSE) return;
    
    // Fraccion de salud perdida -> Bajas (Sec 4)
    float playerLostHealth = ctx->playerInitialHealth - ctx->playerHealth;
    float frac = 0.0f;
    if (ctx->playerInitialHealth > 1.0f) frac = playerLostHealth / ctx->playerInitialHealth;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    
    int targetOutOfActionWeight = (int)(ctx->playerInitialWeight * frac);
    
    // Distribuir peso a OutOfAction (siempre restando lo que ya estaba fuera)
    int currentOOAWeight = (ctx->outOfActionInitiates * 1) + (ctx->outOfActionSoldiers * 2) + (ctx->outOfActionSergeants * 5);
    int weightToAdd = targetOutOfActionWeight - currentOOAWeight;
    
    int newInit = 0, newSold = 0, newSerg = 0;
    while (weightToAdd > 0) {
        if (ctx->playerArmy->iniciados - ctx->outOfActionInitiates - newInit > 0) {
            newInit++; weightToAdd -= 1;
        } else if (ctx->playerArmy->soldados - ctx->outOfActionSoldiers - newSold > 0) {
            newSold++; weightToAdd -= 2;
        } else if (ctx->playerArmy->sargentos - ctx->outOfActionSergeants - newSerg > 0) {
            newSerg++; weightToAdd -= 5;
        } else {
            break;
        }
    }
    
    ctx->outOfActionInitiates += newInit;
    ctx->outOfActionSoldiers += newSold;
    ctx->outOfActionSergeants += newSerg;
    
    if (newInit > 0 || newSold > 0 || newSerg > 0) {
        if (ctx->log.count < MAX_STEPS) {
            ctx->log.steps[ctx->log.count].type = STEP_OUT_OF_ACTION;
            ctx->log.steps[ctx->log.count].as.out_of_action.side = 0;
            ctx->log.steps[ctx->log.count].as.out_of_action.countInitiates = ctx->outOfActionInitiates;
            ctx->log.steps[ctx->log.count].as.out_of_action.countSoldiers = ctx->outOfActionSoldiers;
            ctx->log.steps[ctx->log.count].as.out_of_action.countSergeants = ctx->outOfActionSergeants;
            ctx->log.count++;
        }
    }
    
    // Fin de ronda
    if (ctx->log.count < MAX_STEPS) {
        ctx->log.steps[ctx->log.count].type = STEP_ROUND_END;
        ctx->log.steps[ctx->log.count].as.round_end.round = ctx->round;
        ctx->log.count++;
    }
    
    // Evaluar Condiciones de Corte (Sec 7.5)
    if (ctx->enemyHealth <= 0.0f) {
        ctx->endReason = COMBAT_END_VICTORY;
        Combat_ComputeLoot(ctx, (ctx->gm) ? &ctx->gm->rng : NULL);
        ctx->state = COMBAT_STATE_END;
    } else if (ctx->playerHealth <= 0.0f) {
        ctx->endReason = COMBAT_END_DEFEAT;
        ctx->state = COMBAT_STATE_END;
    } else if (ctx->selectedPlan == PLAN_FLEE) {
        ctx->endReason = COMBAT_END_FLEE;
        ctx->state = COMBAT_STATE_END;
    } else if (ctx->round >= ctx->maxRounds) {
        float playerPct = ctx->playerHealth / ctx->playerInitialHealth;
        float enemyPct = ctx->enemyHealth / ctx->enemyInitialHealth;
        if (playerPct > enemyPct + 0.05f) {
            ctx->endReason = COMBAT_END_VICTORY;
            Combat_ComputeLoot(ctx, (ctx->gm) ? &ctx->gm->rng : NULL);
        } else if (enemyPct > playerPct + 0.05f) {
            ctx->endReason = COMBAT_END_DEFEAT;
        } else {
            ctx->endReason = COMBAT_END_STALEMATE;
        }
        ctx->state = COMBAT_STATE_END;
    } else {
        ctx->previousPlan = ctx->selectedPlan;
        ctx->round++;
        ctx->state = COMBAT_STATE_PLANNING;
        ctx->enemyIntent = rand() % 3;
        ctx->intentRevealed = (ctx->enemy->infoLevel >= 1); // Vuelve a planificar (D6)
    }
}

void Combat_Recount(CombatContext* ctx) {
    if (ctx->state != COMBAT_STATE_END) return;
    
    int deadPct = 0;
    if (ctx->endReason == COMBAT_END_VICTORY) deadPct = 25;
    else if (ctx->endReason == COMBAT_END_STALEMATE) deadPct = 35;
    else if (ctx->endReason == COMBAT_END_DEFEAT) deadPct = 60;
    else if (ctx->endReason == COMBAT_END_FLEE) deadPct = 70;
    
    ModContext mCtx = { &ctx->gm->modRegistry, ctx->playerArmy, ctx->enemy, ctx, 0, 0 };
    deadPct = Modifier_Query(HOOK_RECOUNT_DEATH, EFFECT_CASUALTIES, EFFECT_SIDE_SELF, &mCtx, deadPct, NULL);
    
    // Recount Initiates
    ctx->deadInitiates = 0;
    for(int i=0; i<ctx->outOfActionInitiates; i++) {
        if ((rand() % 100) < deadPct) ctx->deadInitiates++;
    }
    if (ctx->log.count < MAX_STEPS) {
        ctx->log.steps[ctx->log.count].type = STEP_RECOUNT;
        ctx->log.steps[ctx->log.count].as.recount.rank = 0;
        ctx->log.steps[ctx->log.count].as.recount.outOfAction = ctx->outOfActionInitiates;
        ctx->log.steps[ctx->log.count].as.recount.died = ctx->deadInitiates;
        ctx->log.steps[ctx->log.count].as.recount.survived = ctx->outOfActionInitiates - ctx->deadInitiates;
        ctx->log.count++;
    }
    
    // Recount Soldiers
    ctx->deadSoldiers = 0;
    for(int i=0; i<ctx->outOfActionSoldiers; i++) {
        if ((rand() % 100) < deadPct) ctx->deadSoldiers++;
    }
    if (ctx->log.count < MAX_STEPS) {
        ctx->log.steps[ctx->log.count].type = STEP_RECOUNT;
        ctx->log.steps[ctx->log.count].as.recount.rank = 1;
        ctx->log.steps[ctx->log.count].as.recount.outOfAction = ctx->outOfActionSoldiers;
        ctx->log.steps[ctx->log.count].as.recount.died = ctx->deadSoldiers;
        ctx->log.steps[ctx->log.count].as.recount.survived = ctx->outOfActionSoldiers - ctx->deadSoldiers;
        ctx->log.count++;
    }
    
    // Recount Sergeants
    ctx->deadSergeants = 0;
    for(int i=0; i<ctx->outOfActionSergeants; i++) {
        if ((rand() % 100) < deadPct) ctx->deadSergeants++;
    }
    if (ctx->log.count < MAX_STEPS) {
        ctx->log.steps[ctx->log.count].type = STEP_RECOUNT;
        ctx->log.steps[ctx->log.count].as.recount.rank = 2;
        ctx->log.steps[ctx->log.count].as.recount.outOfAction = ctx->outOfActionSergeants;
        ctx->log.steps[ctx->log.count].as.recount.died = ctx->deadSergeants;
        ctx->log.steps[ctx->log.count].as.recount.survived = ctx->outOfActionSergeants - ctx->deadSergeants;
        ctx->log.count++;
    }
    
    if (ctx->endReason == COMBAT_END_DEFEAT) {
        if (ctx->playerHealth < 10.0f) ctx->playerHealth = 10.0f; // DefeatMinHealth
    }
    
    ctx->state = COMBAT_STATE_DONE;
}

void Combat_Commit(CombatContext* ctx) {
    if (ctx->state != COMBAT_STATE_DONE) return;
    
    ctx->playerArmy->salud = ctx->playerHealth;
    
    ctx->playerArmy->iniciados -= ctx->deadInitiates;
    ctx->playerArmy->soldados -= ctx->deadSoldiers;
    ctx->playerArmy->sargentos -= ctx->deadSergeants;
    
    if (ctx->playerArmy->iniciados < 0) ctx->playerArmy->iniciados = 0;
    if (ctx->playerArmy->soldados < 0) ctx->playerArmy->soldados = 0;
    if (ctx->playerArmy->sargentos < 0) ctx->playerArmy->sargentos = 0;
    
    if (ctx->endReason == COMBAT_END_FLEE) {
        ctx->playerArmy->comida_disponible -= ctx->fleeCostFood;
        ctx->playerArmy->moral -= ctx->fleeCostMorale;
        ctx->playerArmy->position = ctx->playerArmy->previousPosition;
    }
    
    if (ctx->endReason == COMBAT_END_VICTORY) {
        if (ctx->lootGold == 0 && ctx->lootFood == 0.0f) {
            Combat_ComputeLoot(ctx, (ctx->gm) ? &ctx->gm->rng : NULL);
        }
        ctx->playerArmy->monedas += ctx->lootGold;
        ctx->playerArmy->comida_disponible += ctx->lootFood;
        if (ctx->enemy) ctx->enemy->isActive = false;
    } else if (ctx->endReason == COMBAT_END_STALEMATE || ctx->endReason == COMBAT_END_DEFEAT || ctx->endReason == COMBAT_END_FLEE) {
        if (ctx->endReason != COMBAT_END_FLEE) {
            ctx->playerArmy->position = ctx->playerArmy->previousPosition;
        }
        if (ctx->enemy) {
            const BalanceConfig* cfg = Balance_GetGlobal();
            int fleeHeadStart = (cfg && cfg->aiFleeHeadStartPhases > 0) ? cfg->aiFleeHeadStartPhases : 1;
            int chaseTurns = (cfg && cfg->aiChaseTurns > 0) ? cfg->aiChaseTurns : 6;
            ctx->enemy->aiState = AIS_CHASING;
            ctx->enemy->currentIntent = INTENT_CHASE;
            ctx->enemy->chaseTurnsLeft = chaseTurns;
            ctx->enemy->skipNextPhase = fleeHeadStart;
            ctx->enemy->lastKnownPlayerPos = ctx->playerArmy->position;
        }
    }
}
