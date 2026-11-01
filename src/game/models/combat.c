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
    ctx->previousPlan = PLAN_NONE;
    ctx->selectedPlan = PLAN_NONE;
    ctx->selectedDivineCardSlot = -1;
    ctx->activeInvocationDef = NULL;
    ctx->lootMultiplier = 1.0f;
    ctx->lootGold = 0;
    ctx->lootFood = 0;
    ctx->lootWater = 0;
    ctx->lootWine = 0;
    ctx->lootMeat = 0;

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
    if (plan != PLAN_DIVINE_INVOCATION) {
        ctx->selectedDivineCardSlot = -1;
        ctx->activeInvocationDef = NULL;
    }
    ctx->selectedPlan = plan;
    
    if (ctx->log.count < MAX_STEPS) {
        ctx->log.steps[ctx->log.count].type = STEP_PLAN_CHOSEN;
        ctx->log.steps[ctx->log.count].as.plan_chosen.plan = plan;
        ctx->log.count++;
    }
}

void Combat_SelectDivineInvocation(CombatContext* ctx, int slot) {
    if (!ctx || !ctx->playerArmy || !ctx->gm) return;
    ModifierInstance* inst = Commander_GetCardInstance(ctx->playerArmy->commander, slot);
    if (!inst || inst->isExhausted) return;

    const ModifierDef* def = Modifier_GetById(ctx->gm->modRegistry.defs, ctx->gm->modRegistry.count, inst->id);
    if (!def || !def->hasInvocation) return;

    ctx->selectedDivineCardSlot = slot;
    ctx->activeInvocationDef = def;
    ctx->selectedPlan = PLAN_DIVINE_INVOCATION;

    if (ctx->log.count < MAX_STEPS) {
        ctx->log.steps[ctx->log.count].type = STEP_PLAN_CHOSEN;
        ctx->log.steps[ctx->log.count].as.plan_chosen.plan = PLAN_DIVINE_INVOCATION;
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
    else if (ctx->selectedPlan == PLAN_DIVINE_INVOCATION && ctx->activeInvocationDef) {
        playerPlanMods += ctx->activeInvocationDef->invocationRollMod;
        if (ctx->activeInvocationDef->invocationHighVariance) {
            // Alta varianza divina: tirada par +4, impar -3
            if ((ctx->execRoll % 2) == 0) {
                playerPlanMods += 4;
            } else {
                playerPlanMods -= 3;
            }
        }
    }
    
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
    
    // Calculo de fuerza enemiga considerando composiciÃ³n de tropas y salud
    float enemyTroopForce = 0.0f;
    if (ctx->enemy) {
        enemyTroopForce = (ctx->enemy->initiates * 1.0f) + 
                          (ctx->enemy->soldiers * 2.0f) + 
                          (ctx->enemy->sergeants * 5.0f);
    }
    float enemyForce = (enemyTroopForce > 0.0f) ? 
        ((enemyTroopForce + (float)ctx->enemy->baseStrength) * 0.5f) : 
        (ctx->enemy ? (float)ctx->enemy->baseStrength : 20.0f);
    
    float enemySaludFactor = (ctx->enemyHealth > 10.0f) ? (ctx->enemyHealth / 100.0f) : 0.1f;
    enemyForce *= enemySaludFactor;
    if (enemyForce < 1.0f) enemyForce = 1.0f;
    
    float ratio = playerForce / enemyForce;
    ctx->forceAdvantage = (int)roundf(log2f(ratio) * 3.0f);
    if (ctx->forceAdvantage > 8) ctx->forceAdvantage = 8;
    if (ctx->forceAdvantage < -8) ctx->forceAdvantage = -8;
    
    float dmgFactor = 10.0f;
    if (ctx->selectedPlan == PLAN_SKIRMISH) dmgFactor *= 0.7f;
    else if (ctx->selectedPlan == PLAN_PIKES) dmgFactor *= 0.6f;
    else if (ctx->selectedPlan == PLAN_ASSAULT) dmgFactor *= 1.2f;
    else if (ctx->selectedPlan == PLAN_DIVINE_INVOCATION && ctx->activeInvocationDef) {
        dmgFactor *= ctx->activeInvocationDef->invocationClashMult;
    }

    int enemyPlanMods = 0;

    // Contrajuegos de Intencion vs Plan
    if (ctx->enemyIntent == 0) { // Carga (Agresivo)
        if (ctx->selectedPlan == PLAN_PIKES) { 
            playerPlanMods += 3; 
            dmgFactor *= 0.4f; 
        } else if (ctx->selectedPlan == PLAN_SKIRMISH) { 
            enemyPlanMods += 2; 
            dmgFactor *= 1.5f; 
        } else if (ctx->selectedPlan == PLAN_ASSAULT) { 
            dmgFactor *= 1.3f; 
        }
    } else if (ctx->enemyIntent == 1) { // Asedio / Hold (Defensivo)
        if (ctx->selectedPlan == PLAN_FLANK) { 
            playerPlanMods += 3; 
            dmgFactor *= 1.2f; 
        } else if (ctx->selectedPlan == PLAN_SKIRMISH) { 
            playerPlanMods += 2; 
            dmgFactor *= 0.8f; 
        } else if (ctx->selectedPlan == PLAN_ASSAULT) { 
            enemyPlanMods += 3; 
            dmgFactor *= 1.3f; 
        }
    } else if (ctx->enemyIntent == 2) { // Repliegue / Maneuver (Tactico)
        if (ctx->selectedPlan == PLAN_ASSAULT) { 
            playerPlanMods += 3; 
            dmgFactor *= 1.5f; 
        } else if (ctx->selectedPlan == PLAN_PIKES) { 
            enemyPlanMods += 2; 
            dmgFactor *= 0.5f; 
        }
    }

    // Contrajuego de Invocación Divina contra la intención enemiga
    if (ctx->selectedPlan == PLAN_DIVINE_INVOCATION && ctx->activeInvocationDef) {
        if (ctx->activeInvocationDef->invocationCounterIntent >= 0 &&
            ctx->activeInvocationDef->invocationCounterIntent == ctx->enemyIntent) {
            playerPlanMods += 4;
            dmgFactor *= 1.3f;
        }
    }

    // Fatiga por repetir plan consecutivamente
    if (ctx->selectedPlan != PLAN_NONE && ctx->selectedPlan == ctx->previousPlan) {
        playerPlanMods /= 2;
        dmgFactor = 10.0f + (dmgFactor - 10.0f) * 0.5f;
    }
    
    // Criticos y pifias del jugador
    if (ctx->criticalHit) {
        playerPlanMods += 5; // Enormous bonus
        dmgFactor *= 1.5f;   // Extra damage multiplier
    } else if (ctx->criticalFumble) {
        playerPlanMods -= 5;
        dmgFactor *= 0.5f;
    }

    // Tirada real de dados (2d6) para el enemigo
    int enemyD1 = 0, enemyD2 = 0;
    if (ctx->gm && ctx->gm->rng.state != 0) {
        enemyD1 = Rng_Range(&ctx->gm->rng, 1, 6);
        enemyD2 = Rng_Range(&ctx->gm->rng, 1, 6);
    } else {
        enemyD1 = (rand() % 6) + 1;
        enemyD2 = (rand() % 6) + 1;
    }
    int enemyRoll = enemyD1 + enemyD2;
    ctx->enemyRoll = enemyRoll;

    // Bonificaciones tacticas del enemigo segun doctrina, mando y composicion
    if (ctx->enemy) {
        // Los sargentos aportan mando/liderazgo (1 punto por cada 2 sargentos, max +4)
        int enemyLeadership = ctx->enemy->sergeants / 2;
        if (enemyLeadership > 4) enemyLeadership = 4;
        enemyPlanMods += enemyLeadership;

        // Influencia de etiquetas (Tags) del escuadron enemigo
        if (Enemy_HasTag(ctx->enemy, TAG_FORTIFIED)) {
            enemyPlanMods += 2;
        }
        if (Enemy_HasTag(ctx->enemy, TAG_HEAVILY_ARMORED)) {
            enemyPlanMods += 1;
        }
        if (Enemy_HasTag(ctx->enemy, TAG_CAVALRY)) {
            enemyPlanMods += 1;
        }
        if (Enemy_HasTag(ctx->enemy, TAG_AMBUSH) && ctx->ambushed && ctx->initiator == 1) {
            enemyPlanMods += 2; // Emboscada sorpresa
        }
        if (Enemy_HasTag(ctx->enemy, TAG_DISORGANIZED)) {
            enemyPlanMods -= 2;
        }
        if (Enemy_HasTag(ctx->enemy, TAG_EXPOSED_FLANK)) {
            enemyPlanMods -= 2;
        }
    }

    // Critico o pifia del enemigo
    if (enemyD1 == 6 && enemyD2 == 6) {
        enemyPlanMods += 4;
        dmgFactor *= 1.3f;
    } else if (enemyD1 == 1 && enemyD2 == 1) {
        enemyPlanMods -= 4;
        dmgFactor *= 0.7f;
    }

    // Calculo y registro de puntuaciones finales
    int playerScore = ctx->execRoll + ctx->forceAdvantage + playerPlanMods;
    ModContext mCtx = { &ctx->gm->modRegistry, ctx->playerArmy, ctx->enemy, ctx, 0, 0 };
    playerScore = Modifier_Query(HOOK_EXEC_ROLL, EFFECT_EXECUTION, EFFECT_SIDE_SELF, &mCtx, playerScore, &ctx->breakdown);
    
    int enemyScore = enemyRoll + enemyPlanMods;
    enemyScore = Modifier_Query(HOOK_EXEC_ROLL, EFFECT_EXECUTION, EFFECT_SIDE_OPPONENT, &mCtx, enemyScore, NULL);
    
    ctx->playerScore = playerScore;
    ctx->enemyScore = enemyScore;
    
    int margin = enemyScore - playerScore;
    ctx->margin = margin;

    if (ctx->log.count < MAX_STEPS) {
        ctx->log.steps[ctx->log.count].type = STEP_ADVANTAGE;
        ctx->log.steps[ctx->log.count].as.advantage.advantage = ctx->forceAdvantage;
        ctx->log.steps[ctx->log.count].as.advantage.playerForce = (int)playerForce;
        ctx->log.steps[ctx->log.count].as.advantage.enemyForce = (int)enemyForce;
        ctx->log.count++;
    }

    if (ctx->log.count < MAX_STEPS) {
        ctx->log.steps[ctx->log.count].type = STEP_ROLL;
        ctx->log.steps[ctx->log.count].as.roll.d1 = dice1;
        ctx->log.steps[ctx->log.count].as.roll.d2 = dice2;
        ctx->log.steps[ctx->log.count].as.roll.total = playerScore;
        ctx->log.count++;
    }
    
    if (margin > 0) {
        // Jugador recibe dano (pierde el choque por margin)
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
        // Enemigo recibe dano (jugador gana o empata el choque)
        float lostHealth = (float)(-margin) * dmgFactor;
        if (lostHealth == 0.0f) lostHealth = 5.0f * dmgFactor; // Empate rompe con golpe leve
        
        lostHealth = (float)Modifier_Query(HOOK_EXEC_DAMAGE, EFFECT_EXECUTION, EFFECT_SIDE_SELF, &mCtx, (int)lostHealth, NULL);
        if (lostHealth < 1.0f) lostHealth = 1.0f;
        
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
    
    if (ctx->selectedPlan == PLAN_DIVINE_INVOCATION && ctx->selectedDivineCardSlot >= 0 && ctx->playerArmy) {
        Commander_SetCardExhausted(ctx->playerArmy->commander, ctx->selectedDivineCardSlot, true);
    }

    ctx->state = COMBAT_STATE_ROUND_CLOSE;
}

void Combat_ComputeLoot(CombatContext* ctx, Rng* rng) {
    if (!ctx || !ctx->enemy) return;
    const BalanceConfig* cfg = Balance_GetGlobal();
    ctx->lootWine = 0;
    ctx->lootMeat = 0;

    if (ctx->enemy->behavior == AI_BEAST) {
        ctx->lootGold = 0;
        int beastStr = ctx->enemy->baseStrength;
        if (beastStr <= 0) beastStr = 20;
        ctx->lootFood = (int)roundf((float)beastStr * 0.8f);
        if (ctx->lootFood < 15) ctx->lootFood = 15;
        ctx->lootWater = (int)roundf((float)beastStr * 0.4f);
        if (ctx->lootWater < 10) ctx->lootWater = 10;

        /* Bestias grandes (fuerza >= 25): probabilidad de soltar Carne fresca */
        if (beastStr >= 25) {
            float meatChance = (cfg && cfg->lootMeatChanceLarge > 0.0f) ? cfg->lootMeatChanceLarge : 0.45f;
            int roll = rng ? Rng_Range(rng, 1, 100) : 50;
            if (roll <= (int)(meatChance * 100.0f)) {
                ctx->lootMeat = 2 + (beastStr - 20) / 10;
                if (ctx->lootMeat < 2) ctx->lootMeat = 2;
            }
        }
        return;
    }

    int enemyStr = ctx->enemy->baseStrength;
    if (enemyStr <= 0) enemyStr = 25;

    float baseGold = (float)enemyStr * cfg->lootGoldPerStr;
    float baseFood = (float)enemyStr * cfg->lootFoodPerStr;
    float baseWater = (float)enemyStr * (cfg->lootFoodPerStr * 0.8f);

    float variance = (cfg->lootVariancePct > 0.0f) ? cfg->lootVariancePct : 20.0f;
    float factorGold = 1.0f;
    float factorFood = 1.0f;
    float factorWater = 1.0f;

    if (rng) {
        float rollG = (float)Rng_Range(rng, (int)(-variance), (int)variance) / 100.0f;
        float rollF = (float)Rng_Range(rng, (int)(-variance), (int)variance) / 100.0f;
        float rollW = (float)Rng_Range(rng, (int)(-variance), (int)variance) / 100.0f;
        factorGold += rollG;
        factorFood += rollF;
        factorWater += rollW;
    }

    float mult = (ctx->lootMultiplier > 0.0f) ? ctx->lootMultiplier : 1.0f;

    ctx->lootGold = (int)roundf(baseGold * factorGold * mult);
    if (ctx->lootGold < 5) ctx->lootGold = 5;

    ctx->lootFood = (int)roundf(baseFood * factorFood * mult);
    if (ctx->lootFood < 5) ctx->lootFood = 5;

    ctx->lootWater = (int)roundf(baseWater * factorWater * mult);
    if (ctx->lootWater < 5) ctx->lootWater = 5;

    /* Tropas enemigas grandes: alta probabilidad de arrojar Vino y Carne */
    bool isLargeTroop = (enemyStr >= 35) || (ctx->enemy->sergeants >= 3) || (ctx->enemy->tags & TAG_FORTIFIED);

    float wineChance = isLargeTroop ?
        ((cfg && cfg->lootWineChanceLarge > 0.0f) ? cfg->lootWineChanceLarge : 0.45f) : 0.15f;
    float meatChance = isLargeTroop ?
        ((cfg && cfg->lootMeatChanceLarge > 0.0f) ? cfg->lootMeatChanceLarge : 0.45f) : 0.15f;

    if (ctx->enemy->tags & TAG_FORTIFIED) {
        wineChance = 0.80f;
        meatChance = 0.80f;
    }

    int rollWine = rng ? Rng_Range(rng, 1, 100) : 50;
    if (rollWine <= (int)(wineChance * 100.0f)) {
        int baseWine = isLargeTroop ? (2 + (enemyStr - 30) / 8) : 1;
        if (baseWine < 1) baseWine = 1;
        ctx->lootWine = (int)roundf((float)baseWine * mult);
        if (ctx->lootWine < 1) ctx->lootWine = 1;
    }

    int rollMeat = rng ? Rng_Range(rng, 1, 100) : 50;
    if (rollMeat <= (int)(meatChance * 100.0f)) {
        int baseMeat = isLargeTroop ? (2 + (enemyStr - 30) / 8) : 1;
        if (baseMeat < 1) baseMeat = 1;
        ctx->lootMeat = (int)roundf((float)baseMeat * mult);
        if (ctx->lootMeat < 1) ctx->lootMeat = 1;
    }

    /* BotÃ­n garantizado de asedio contra guarniciÃ³n fortificada (C3) */
    if (ctx->enemy && (ctx->enemy->tags & TAG_FORTIFIED)) {
        int targetGold = (cfg->siegeBaseGold > 0) ? cfg->siegeBaseGold : 200;
        if (ctx->lootGold < targetGold) ctx->lootGold = targetGold;

        if (ctx->playerArmy) {
            float dailyUsage = Army_CalculateFoodUsage(Army_GetTotalSoldiers(ctx->playerArmy), ctx->playerArmy->ganado);
            float targetFood = (cfg->siegeSupplyDays > 0.0f ? cfg->siegeSupplyDays : 5.0f) * dailyUsage;
            if (ctx->lootFood < (int)targetFood) ctx->lootFood = (int)targetFood;

            float dailyWater = Army_CalculateWaterUsage(Army_GetTotalSoldiers(ctx->playerArmy), ctx->playerArmy->ganado);
            float targetWater = (cfg->siegeSupplyDays > 0.0f ? cfg->siegeSupplyDays : 5.0f) * dailyWater;
            if (ctx->lootWater < (int)targetWater) ctx->lootWater = (int)targetWater;
        }

        if (ctx->lootWine < 6) ctx->lootWine = 6;
        if (ctx->lootMeat < 8) ctx->lootMeat = 8;
    }
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
        ctx->selectedPlan = PLAN_NONE;
        ctx->activeInvocationDef = NULL;
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
    
    float healthDelta = ctx->playerHealth - ctx->playerInitialHealth;
    Army_ApplyHealthDelta(ctx->playerArmy, healthDelta);
    
    ctx->playerArmy->iniciados -= ctx->deadInitiates;
    ctx->playerArmy->soldados -= ctx->deadSoldiers;
    ctx->playerArmy->sargentos -= ctx->deadSergeants;
    
    if (ctx->playerArmy->iniciados < 0) ctx->playerArmy->iniciados = 0;
    if (ctx->playerArmy->soldados < 0) ctx->playerArmy->soldados = 0;
    if (ctx->playerArmy->sargentos < 0) ctx->playerArmy->sargentos = 0;
    
    if (ctx->endReason == COMBAT_END_DEFEAT) {
        /* Pérdida de moral en todos los niveles al ser derrotados en combate (~20 promedio) */
        ctx->playerArmy->groupState[RG_INICIADOS].morale -= 24.0f;
        ctx->playerArmy->groupState[RG_SOLDADOS].morale  -= 18.0f;
        ctx->playerArmy->groupState[RG_SARGENTOS].morale -= 14.0f;
        for (int g = 0; g < RG_COUNT; g++) {
            if (ctx->playerArmy->groupState[g].morale < 0.0f) {
                ctx->playerArmy->groupState[g].morale = 0.0f;
            }
        }
        
        // Empuje de 2 hexágonos en retirada alejándose del enemigo
        HexCoord enemyPos = ctx->enemy ? ctx->enemy->position : ctx->playerArmy->position;
        HexCoord p1 = ctx->playerArmy->previousPosition;
        int dq = p1.q - enemyPos.q;
        int dr = p1.r - enemyPos.r;
        if (dq != 0 || dr != 0) {
            ctx->playerArmy->position = (HexCoord){ p1.q + dq, p1.r + dr };
        } else {
            ctx->playerArmy->position = p1;
        }

        if (ctx->gm) {
            GameMaster_SetFlag(ctx->gm, "derrota_reciente");
        }
    } else if (ctx->endReason == COMBAT_END_FLEE) {
        ctx->playerArmy->comida_disponible -= ctx->fleeCostFood;
        ctx->playerArmy->groupState[RG_INICIADOS].morale -= (float)ctx->fleeCostMorale * 1.25f;
        ctx->playerArmy->groupState[RG_SOLDADOS].morale  -= (float)ctx->fleeCostMorale * 1.0f;
        ctx->playerArmy->groupState[RG_SARGENTOS].morale -= (float)ctx->fleeCostMorale * 0.75f;
        for (int g = 0; g < RG_COUNT; g++) {
            if (ctx->playerArmy->groupState[g].morale < 0.0f) {
                ctx->playerArmy->groupState[g].morale = 0.0f;
            }
        }
        ctx->playerArmy->position = ctx->playerArmy->previousPosition;
    }
    Army_RecalculateAggregates(ctx->playerArmy);

    // Progresión de recarga de cartas divinas agotadas
    if (ctx->playerArmy && ctx->playerArmy->commander.ptr) {
        for (int c = 0; c < 8; c++) {
            ModifierInstance* inst = Commander_GetCardInstance(ctx->playerArmy->commander, c);
            if (!inst) break;
            if (inst->isExhausted) {
                if (c != ctx->selectedDivineCardSlot) {
                    if (inst->battlesUntilRecharge > 0) inst->battlesUntilRecharge--;
                    if (inst->battlesUntilRecharge <= 0) inst->isExhausted = false;
                }
            }
        }
    }
    
    if (ctx->endReason == COMBAT_END_VICTORY) {
        if (ctx->lootGold == 0 && ctx->lootFood == 0 && ctx->lootWater == 0 && ctx->lootWine == 0 && ctx->lootMeat == 0) {
            Combat_ComputeLoot(ctx, (ctx->gm) ? &ctx->gm->rng : NULL);
        }
        
        // Aplicar bonificaciones de fin de combate (HOOK_COMBAT_END)
        if (ctx->gm && ctx->playerArmy) {
            ModContext mCtx = {
                .reg = &ctx->gm->modRegistry,
                .army = ctx->playerArmy,
                .enemy = ctx->enemy,
                .combat = ctx,
                .timeOfDay = ctx->gm->timeOfDay,
                .infamy = ctx->playerArmy->infamia
            };
            ModAction actions[16];
            int nAct = Modifier_CollectActions(HOOK_COMBAT_END, &mCtx, actions, 16);
            for (int a = 0; a < nAct; a++) {
                if (actions[a].type == EFFECT_GOLD) ctx->lootGold += (int)actions[a].value;
                else if (actions[a].type == EFFECT_FOOD) ctx->lootFood += (int)actions[a].value;
                else if (actions[a].type == EFFECT_WATER) ctx->lootWater += (int)actions[a].value;
                else if (actions[a].type == EFFECT_WINE) ctx->lootWine += (int)actions[a].value;
                else if (actions[a].type == EFFECT_MEAT) ctx->lootMeat += (int)actions[a].value;
                else if (actions[a].type == EFFECT_MORAL) Army_ApplyMoraleDelta(ctx->playerArmy, actions[a].value);
            }
        }
        
        ctx->playerArmy->monedas += ctx->lootGold;
        ctx->playerArmy->comida_disponible += (float)ctx->lootFood;
        ctx->playerArmy->agua_disponible += (float)ctx->lootWater;
        ctx->playerArmy->vino_disponible += (float)ctx->lootWine;
        ctx->playerArmy->carne_disponible += (float)ctx->lootMeat;
        if (ctx->enemy) ctx->enemy->isActive = false;
    } else if (ctx->endReason == COMBAT_END_STALEMATE || ctx->endReason == COMBAT_END_DEFEAT || ctx->endReason == COMBAT_END_FLEE) {
        if (ctx->endReason == COMBAT_END_DEFEAT) {
            // El jugador ya fue empujado 2 hexes en retirada.
            // El enemigo victorioso entra en descanso/saqueo y no persigue de inmediato (rompe la espiral de derrota)
            if (ctx->enemy) {
                ctx->enemy->aiState = AIS_RESTING;
                ctx->enemy->currentIntent = INTENT_NONE;
                ctx->enemy->patrolWaitTurns = 2; // 2 turnos de descanso/saqueo
                ctx->enemy->chaseTurnsLeft = 0;
                ctx->enemy->skipNextPhase = 2;
                ctx->enemy->lastKnownPlayerPos = ctx->playerArmy->position;
            }
        } else {
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
}

CombatPlanInfo Combat_GetPlanInfo(const CombatContext* ctx, CombatPlan plan) {
    CombatPlanInfo info = {0};
    info.plan = plan;
    info.counterIntent = -1;
    info.isFatigued = (ctx && ctx->previousPlan == plan);

    switch (plan) {
        case PLAN_SKIRMISH:
            info.name = "Escaramuza";
            info.requiredRoll = 0;
            info.rollModifier = 0;
            info.clashMultiplier = 0.7f;
            info.description = "Hostigamiento a distancia que mitiga 30% del dano recibido.";
            info.counterIntent = 1;
            info.isUnlocked = ctx ? ctx->canSkirmish : true;
            info.pointsNeeded = 0;
            if (ctx && ctx->intentRevealed) {
                if (ctx->enemyIntent == 1) info.hasAdvantage = true;
                else if (ctx->enemyIntent == 0) info.hasDisadvantage = true;
            }
            break;

        case PLAN_PIKES:
            info.name = "Linea de Picas";
            info.requiredRoll = 8;
            info.rollModifier = -1;
            info.clashMultiplier = 1.0f;
            info.description = "Muro de astas que frena en seco embestidas de caballeria.";
            info.counterIntent = 0;
            info.isUnlocked = ctx ? ctx->canPikes : false;
            info.pointsNeeded = (ctx && ctx->planRoll < 8) ? (8 - ctx->planRoll) : 0;
            if (ctx && ctx->intentRevealed) {
                if (ctx->enemyIntent == 0) info.hasAdvantage = true;
                else if (ctx->enemyIntent == 2) info.hasDisadvantage = true;
            }
            break;

        case PLAN_ASSAULT:
            info.name = "Asalto Frontal";
            info.requiredRoll = 6;
            info.rollModifier = 2;
            info.clashMultiplier = 1.2f;
            info.description = "Embestida cuerpo a cuerpo con bono de tirada y choque incrementado.";
            info.counterIntent = 2;
            info.isUnlocked = ctx ? ctx->canAssault : false;
            info.pointsNeeded = (ctx && ctx->planRoll < 6) ? (6 - ctx->planRoll) : 0;
            if (ctx && ctx->intentRevealed) {
                if (ctx->enemyIntent == 2) info.hasAdvantage = true;
                else if (ctx->enemyIntent == 1) info.hasDisadvantage = true;
            }
            break;

        case PLAN_FLANK:
            info.name = "Flanqueo";
            info.requiredRoll = 9;
            info.rollModifier = 4;
            info.clashMultiplier = 1.3f;
            info.description = "Maniobra envolvente veloz para golpear el punto ciego enemigo.";
            info.counterIntent = 1;
            info.isUnlocked = ctx ? ctx->canFlank : false;
            info.pointsNeeded = (ctx && ctx->planRoll < 9) ? (9 - ctx->planRoll) : 0;
            if (ctx && ctx->intentRevealed) {
                if (ctx->enemyIntent == 1) info.hasAdvantage = true;
                else if (ctx->enemyIntent == 0) info.hasDisadvantage = true;
            }
            break;

        case PLAN_FLEE:
            info.name = "Retirada Tactica";
            info.requiredRoll = 0;
            info.rollModifier = 0;
            info.clashMultiplier = 0.5f;
            info.description = "Desenganche ordenado para preservar la vida del batallon.";
            info.counterIntent = -1;
            info.isUnlocked = true;
            info.pointsNeeded = 0;
            break;

        case PLAN_DIVINE_INVOCATION:
            if (ctx && ctx->activeInvocationDef) {
                info.name = ctx->activeInvocationDef->invocationName;
                info.description = ctx->activeInvocationDef->invocationDesc;
                info.requiredRoll = 0;
                info.rollModifier = ctx->activeInvocationDef->invocationRollMod;
                info.clashMultiplier = ctx->activeInvocationDef->invocationClashMult;
                info.counterIntent = ctx->activeInvocationDef->invocationCounterIntent;
                info.isUnlocked = true;
                info.pointsNeeded = 0;
                if (ctx->intentRevealed && info.counterIntent >= 0) {
                    if (ctx->enemyIntent == info.counterIntent) info.hasAdvantage = true;
                }
            } else {
                info.name = "Invocacion Divina";
                info.description = "Canaliza el poder de un dios patron para desatar un milagro en combate.";
                info.requiredRoll = 0;
                info.rollModifier = 0;
                info.clashMultiplier = 1.0f;
                info.counterIntent = -1;
                info.isUnlocked = true;
                info.pointsNeeded = 0;
            }
            break;

        default:
            info.name = "Desconocido";
            info.description = "";
            info.isUnlocked = false;
            break;
    }

    return info;
}
