#include "models/ration.h"
#include "models/army.h"
#include "models/balance_config.h"
#include "controllers/game_master.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ============================================================
   SISTEMA DE RACIONES Y REPARTO DIARIO (models/ration.c)
   ============================================================ */

RationNeeds Ration_GetNeeds(const Army* army, const Map* map, HexCoord pos) {
    RationNeeds needs;
    memset(&needs, 0, sizeof(RationNeeds));
    if (!army) return needs;

    const BalanceConfig* cfg = Balance_GetGlobal();
    float humanDrinkNeed = (cfg && cfg->humanDrinkNeed > 0.0f) ? cfg->humanDrinkNeed : 1.0f;
    float humanFoodNeed  = (cfg && cfg->humanFoodNeed > 0.0f)  ? cfg->humanFoodNeed  : 1.0f;
    float cattleDrinkNeed = (cfg && cfg->cattleDrinkNeed > 0.0f) ? cfg->cattleDrinkNeed : 1.5f;
    float cattleFoodNeed  = (cfg && cfg->cattleFoodNeed > 0.0f)  ? cfg->cattleFoodNeed  : 0.2f;

    /* Humanos: Iniciados, Soldados, Sargentos */
    needs.drink[RG_INICIADOS] = (float)army->iniciados * humanDrinkNeed;
    needs.food[RG_INICIADOS]  = (float)army->iniciados * humanFoodNeed;

    needs.drink[RG_SOLDADOS]  = (float)army->soldados * humanDrinkNeed;
    needs.food[RG_SOLDADOS]   = (float)army->soldados * humanFoodNeed;

    needs.drink[RG_SARGENTOS] = (float)army->sargentos * humanDrinkNeed;
    needs.food[RG_SARGENTOS]  = (float)army->sargentos * humanFoodNeed;

    /* Ganado: solo requiere Agua para subsistir. Pasta naturalmente del terreno (0 comida requerida). */
    needs.drink[RG_GANADO] = (float)army->ganado * cattleDrinkNeed;
    needs.food[RG_GANADO]  = 0.0f;

    return needs;
}

float Ration_GetStock(const Army* army, RationResource r) {
    if (!army) return 0.0f;
    switch (r) {
        case RS_AGUA:   return army->agua_disponible;
        case RS_VINO:   return army->vino_disponible;
        case RS_COMIDA: return army->comida_disponible;
        case RS_CARNE:  return army->carne_disponible;
        default:        return 0.0f;
    }
}

GroupState Ration_GetGroupState(const Army* army, RationGroup g) {
    GroupState state;
    memset(&state, 0, sizeof(GroupState));
    if (!army || g < 0 || g >= RG_COUNT) return state;
    return army->groupState[g];
}

bool Ration_Validate(const Army* army, const RationPlan* plan, char* err, int cap) {
    if (!army || !plan) {
        if (err && cap > 0) snprintf(err, cap, "Puntero nulo");
        return false;
    }

    /* Asignación negativa */
    for (int g = 0; g < RG_COUNT; g++) {
        for (int r = 0; r < RS_COUNT; r++) {
            if (plan->amount[g][r] < -0.001f) {
                if (err && cap > 0) snprintf(err, cap, "Asignacion negativa invalida");
                return false;
            }
        }
    }

    /* Ganado solo recibe Agua y Comida (nunca Vino ni Carne) */
    if (plan->amount[RG_GANADO][RS_VINO] > 0.001f ||
        plan->amount[RG_GANADO][RS_CARNE] > 0.001f) {
        if (err && cap > 0) snprintf(err, cap, "El ganado no puede consumir vino ni carne");
        return false;
    }

    /* No exceder existencias en inventario */
    float sum[RS_COUNT] = {0};
    for (int g = 0; g < RG_COUNT; g++) {
        for (int r = 0; r < RS_COUNT; r++) {
            sum[r] += plan->amount[g][r];
        }
    }

    for (int r = 0; r < RS_COUNT; r++) {
        float stock = Ration_GetStock(army, (RationResource)r);
        if (sum[r] > stock + 0.001f) {
            if (err && cap > 0) snprintf(err, cap, "Asignacion excede el stock disponible");
            return false;
        }
    }

    return true;
}

RationPlan Ration_Auto(const Army* army, const RationNeeds* needs) {
    RationPlan plan;
    memset(&plan, 0, sizeof(RationPlan));
    if (!army) return plan;

    RationNeeds localNeeds;
    if (!needs) {
        localNeeds = Ration_GetNeeds(army, NULL, army->position);
        needs = &localNeeds;
    }

    float stockAgua   = Ration_GetStock(army, RS_AGUA);
    float stockVino   = Ration_GetStock(army, RS_VINO);
    float stockComida = Ration_GetStock(army, RS_COMIDA);
    float stockCarne  = Ration_GetStock(army, RS_CARNE);

    /* 1. Ganado solo consume Agua (pasta naturalmente sin requerir comida) */
    float cattleAgua = needs->drink[RG_GANADO];
    if (cattleAgua > stockAgua) cattleAgua = stockAgua;
    plan.amount[RG_GANADO][RS_AGUA] = cattleAgua;
    stockAgua -= cattleAgua;
    plan.amount[RG_GANADO][RS_COMIDA] = 0.0f;

    /* 2. Sargentos: asignar lujos primero para sostener o recuperar satisfacción */
    float srgDrinkNeed = needs->drink[RG_SARGENTOS];
    float srgFoodNeed  = needs->food[RG_SARGENTOS];

    float srgVino = (stockVino >= srgDrinkNeed) ? srgDrinkNeed : stockVino;
    plan.amount[RG_SARGENTOS][RS_VINO] = srgVino;
    stockVino -= srgVino;
    srgDrinkNeed -= srgVino;

    float srgCarne = (stockCarne >= srgFoodNeed) ? srgFoodNeed : stockCarne;
    plan.amount[RG_SARGENTOS][RS_CARNE] = srgCarne;
    stockCarne -= srgCarne;
    srgFoodNeed -= srgCarne;

    /* 3. Recursos restantes para humanos */
    float remDrinkNeed[3] = { needs->drink[RG_INICIADOS], needs->drink[RG_SOLDADOS], srgDrinkNeed };
    float remFoodNeed[3]  = { needs->food[RG_INICIADOS],  needs->food[RG_SOLDADOS],  srgFoodNeed };
    RationGroup humanGroups[3] = { RG_INICIADOS, RG_SOLDADOS, RG_SARGENTOS };

    float totRemDrink = remDrinkNeed[0] + remDrinkNeed[1] + remDrinkNeed[2];
    float totRemFood  = remFoodNeed[0]  + remFoodNeed[1]  + remFoodNeed[2];

    float availDrink = stockAgua + stockVino;
    float availFood  = stockComida + stockCarne;

    float drinkRatio = (totRemDrink > 0.0f) ? (availDrink / totRemDrink) : 1.0f;
    if (drinkRatio > 1.0f) drinkRatio = 1.0f;

    float foodRatio  = (totRemFood > 0.0f) ? (availFood / totRemFood) : 1.0f;
    if (foodRatio > 1.0f) foodRatio = 1.0f;

    /* Distribuir bebida a humanos */
    for (int i = 0; i < 3; i++) {
        RationGroup g = humanGroups[i];
        float target = remDrinkNeed[i] * drinkRatio;
        if (target <= 0.0f) continue;

        float fromAgua = (stockAgua >= target) ? target : stockAgua;
        plan.amount[g][RS_AGUA] += fromAgua;
        stockAgua -= fromAgua;
        target -= fromAgua;

        if (target > 0.0f && stockVino > 0.0f) {
            float fromVino = (stockVino >= target) ? target : stockVino;
            plan.amount[g][RS_VINO] += fromVino;
            stockVino -= fromVino;
        }
    }

    /* Distribuir comida a humanos */
    for (int i = 0; i < 3; i++) {
        RationGroup g = humanGroups[i];
        float target = remFoodNeed[i] * foodRatio;
        if (target <= 0.0f) continue;

        float fromComida = (stockComida >= target) ? target : stockComida;
        plan.amount[g][RS_COMIDA] += fromComida;
        stockComida -= fromComida;
        target -= fromComida;

        if (target > 0.0f && stockCarne > 0.0f) {
            float fromCarne = (stockCarne >= target) ? target : stockCarne;
            plan.amount[g][RS_CARNE] += fromCarne;
            stockCarne -= fromCarne;
        }
    }

    return plan;
}

RationPlan Ration_Repeat(const Army* army) {
    if (!army || !army->hasLastRationPlan) {
        RationNeeds needs = Ration_GetNeeds(army, NULL, army ? army->position : (HexCoord){0,0});
        return Ration_Auto(army, &needs);
    }

    RationPlan plan = army->lastRationPlan;
    RationNeeds needs = Ration_GetNeeds(army, NULL, army->position);

    /* Clamping de plan anterior contra existencias actuales */
    float sum[RS_COUNT] = {0};
    for (int g = 0; g < RG_COUNT; g++) {
        for (int r = 0; r < RS_COUNT; r++) {
            sum[r] += plan.amount[g][r];
        }
    }

    for (int r = 0; r < RS_COUNT; r++) {
        float stock = Ration_GetStock(army, (RationResource)r);
        if (sum[r] > stock && sum[r] > 0.001f) {
            float factor = stock / sum[r];
            for (int g = 0; g < RG_COUNT; g++) {
                plan.amount[g][r] *= factor;
            }
        }
    }

    /* Clamping para no sobre-asignar si las necesidades disminuyeron (bajas) */
    for (int g = 0; g < RG_COUNT; g++) {
        float totDrink = plan.amount[g][RS_AGUA] + plan.amount[g][RS_VINO];
        if (totDrink > needs.drink[g] && totDrink > 0.001f) {
            float f = needs.drink[g] / totDrink;
            plan.amount[g][RS_AGUA] *= f;
            plan.amount[g][RS_VINO] *= f;
        }
        float totFood = plan.amount[g][RS_COMIDA] + plan.amount[g][RS_CARNE];
        if (totFood > needs.food[g] && totFood > 0.001f) {
            float f = needs.food[g] / totFood;
            plan.amount[g][RS_COMIDA] *= f;
            plan.amount[g][RS_CARNE] *= f;
        }
    }

    return plan;
}

static void CalculateGroupDeltas(const Army* army, RationGroup g, const RationPlan* plan, const RationNeeds* needs,
                                 RationLevel* outLevel, float* outMoralDelta, float* outSaludDelta,
                                 float* outSatDelta, int* outDaysUnfed, bool* outRisksDesertion) {
    const BalanceConfig* cfg = Balance_GetGlobal();
    float totalDrink = plan->amount[g][RS_AGUA] + plan->amount[g][RS_VINO];
    float totalFood  = plan->amount[g][RS_COMIDA] + plan->amount[g][RS_CARNE];

    float needD = needs->drink[g];
    float needF = needs->food[g];

    float ratioD = (needD > 0.001f) ? (totalDrink / needD) : 1.0f;
    float ratioF = (needF > 0.001f) ? (totalFood / needF) : 1.0f;

    float minRatio = (ratioD < ratioF) ? ratioD : ratioF;

    float moralDelta = 0.0f;
    float saludDelta = 0.0f;
    float satDelta = 0.0f;
    int daysUnfed = army ? army->groupState[g].daysUnfed : 0;
    bool risksDesertion = false;

    RationLevel level;
    if (minRatio >= 0.999f) {
        level = RATION_FULL;
        if (daysUnfed > 0) daysUnfed--;
    } else if (minRatio >= 0.5f) {
        level = RATION_REDUCED;
        if (daysUnfed > 0) daysUnfed--;
        moralDelta -= (cfg && cfg->reducedMoraleLossPerDay > 0.0f) ? cfg->reducedMoraleLossPerDay : 1.0f;
    } else {
        level = RATION_STARVING;
        daysUnfed++;
    }

    if (g == RG_GANADO) {
        if (level == RATION_STARVING) {
            saludDelta -= 20.0f;
            risksDesertion = true;
        } else {
            saludDelta += 10.0f;
        }
        moralDelta = 0.0f;
        satDelta = 0.0f;
    } else {
        /* Humanos: escalera por grupo con multiplicadores y desplazamiento */
        if (daysUnfed > 0) {
            int displacement = (g == RG_INICIADOS) ? -1 : (g == RG_SOLDADOS) ? 0 : 1;
            int effectiveDay = daysUnfed - displacement;
            if (effectiveDay >= 1) {
                int rowIdx = effectiveDay - 1;
                if (rowIdx > 2) rowIdx = 2;
                float rankMult = (g == RG_INICIADOS) ? 1.0f : (g == RG_SOLDADOS) ? 0.75f : 0.5f;

                if (ratioF < 0.5f && cfg) {
                    const StarvationRow* row = &cfg->starvationFood[rowIdx];
                    float mLoss = (row->moral < 0.0f) ? row->moral : -row->moral;
                    float sLoss = (row->salud < 0.0f) ? row->salud : -row->salud;
                    moralDelta += mLoss * rankMult;
                    saludDelta += sLoss * rankMult;
                    if (row->desertPct > 0.0f || row->deathPct > 0.0f) risksDesertion = true;
                }
                if (ratioD < 0.5f && cfg) {
                    const StarvationRow* row = &cfg->starvationWater[rowIdx];
                    float mLoss = (row->moral < 0.0f) ? row->moral : -row->moral;
                    float sLoss = (row->salud < 0.0f) ? row->salud : -row->salud;
                    moralDelta += mLoss * rankMult;
                    saludDelta += sLoss * rankMult;
                    if (row->desertPct > 0.0f || row->deathPct > 0.0f) risksDesertion = true;
                }
            }
        }

        /* Bonificación de lujos: si lujo >= 25% de la necesidad */
        float totalLuxury = plan->amount[g][RS_VINO] + plan->amount[g][RS_CARNE];
        float totalNeed = needD + needF;
        float luxThreshold = (cfg && cfg->luxuryThresholdPct > 0.0f) ? cfg->luxuryThresholdPct : 0.25f;
        if (totalNeed > 0.001f && totalLuxury >= (totalNeed * luxThreshold)) {
            moralDelta += (cfg && cfg->luxuryMoraleBonus > 0.0f) ? cfg->luxuryMoraleBonus : 2.0f;
        }

        /* Satisfacción de Sargentos */
        if (g == RG_SARGENTOS) {
            if (totalLuxury >= (totalNeed * luxThreshold) || totalLuxury > 0.001f) {
                satDelta = (cfg && cfg->sargentosLuxurySatisfactionBonus > 0.0f) ? cfg->sargentosLuxurySatisfactionBonus : 2.0f;
            } else {
                satDelta = (cfg && cfg->sargentosDailySatisfactionDecay > 0.0f) ? -cfg->sargentosDailySatisfactionDecay : -1.0f;
            }
            float curSat = army ? army->groupState[g].satisfaction : 10.0f;
            if (curSat + satDelta <= 0.001f) {
                moralDelta -= 3.0f; /* Quejas de veteranos en satisfacción cero */
            }
        }
    }

    /* Clamping a los límites reales [0, 100] de moral y salud */
    if (army) {
        float curM = army->groupState[g].morale;
        float curH = army->groupState[g].health;

        float finalM = curM + moralDelta;
        if (finalM > 100.0f) finalM = 100.0f;
        if (finalM < 0.0f) finalM = 0.0f;
        moralDelta = finalM - curM;

        float finalH = curH + saludDelta;
        if (finalH > 100.0f) finalH = 100.0f;
        if (finalH < 0.0f) finalH = 0.0f;
        saludDelta = finalH - curH;
    }

    if (outLevel) *outLevel = level;
    if (outMoralDelta) *outMoralDelta = moralDelta;
    if (outSaludDelta) *outSaludDelta = saludDelta;
    if (outSatDelta) *outSatDelta = satDelta;
    if (outDaysUnfed) *outDaysUnfed = daysUnfed;
    if (outRisksDesertion) *outRisksDesertion = risksDesertion;
}

RationPreview Ration_Preview(const Army* army, const RationPlan* plan) {
    RationPreview prev;
    memset(&prev, 0, sizeof(RationPreview));
    if (!army || !plan) return prev;

    RationNeeds needs = Ration_GetNeeds(army, NULL, army->position);

    /* Calcular existencias resultantes */
    for (int r = 0; r < RS_COUNT; r++) {
        float sum = 0.0f;
        for (int g = 0; g < RG_COUNT; g++) sum += plan->amount[g][r];
        float stock = Ration_GetStock(army, (RationResource)r);
        prev.stockAfter[r] = stock - sum;
        if (prev.stockAfter[r] < 0.0f) prev.stockAfter[r] = 0.0f;

        /* Días de reserva restantes para este recurso */
        float dailyBurn = 0.0f;
        for (int g = 0; g < RG_COUNT; g++) {
            if (r == RS_AGUA || r == RS_VINO) dailyBurn += needs.drink[g];
            else dailyBurn += needs.food[g];
        }
        prev.daysLeft[r] = (dailyBurn > 0.001f) ? (prev.stockAfter[r] / dailyBurn) : 999.0f;
    }

    /* Calcular estado y proyecciones por grupo */
    for (int g = 0; g < RG_COUNT; g++) {
        CalculateGroupDeltas(army, (RationGroup)g, plan, &needs,
                             &prev.group[g].level,
                             &prev.group[g].moraleDelta,
                             &prev.group[g].healthDelta,
                             &prev.group[g].satisfactionDelta,
                             &prev.group[g].daysUnfed,
                             &prev.group[g].risksDesertion);
    }

    return prev;
}

bool Ration_NeedsDecision(const Army* army, const Map* map, HexCoord pos) {
    if (!army) return false;
    RationNeeds needs = Ration_GetNeeds(army, map, pos);

    float totDrink = 0.0f, totFood = 0.0f;
    for (int g = 0; g < RG_COUNT; g++) {
        totDrink += needs.drink[g];
        totFood  += needs.food[g];
    }

    float stockDrink = Ration_GetStock(army, RS_AGUA) + Ration_GetStock(army, RS_VINO);
    float stockFood  = Ration_GetStock(army, RS_COMIDA) + Ration_GetStock(army, RS_CARNE);

    /* Chequeo general de existencias */
    if (stockDrink < (totDrink - 0.001f) || stockFood < (totFood - 0.001f)) {
        return true;
    }

    /* Chequeo específico de Ganado (no consume lujos) */
    if (needs.drink[RG_GANADO] > 0.0f && army->agua_disponible < (needs.drink[RG_GANADO] - 0.001f)) {
        return true;
    }
    if (needs.food[RG_GANADO] > 0.0f && army->comida_disponible < (needs.food[RG_GANADO] - 0.001f)) {
        return true;
    }

    return false;
}

bool Ration_Commit(Army* army, const RationPlan* plan, DayReport* report, Rng* rng) {
    if (!army || !plan) return false;
    if (!Ration_Validate(army, plan, NULL, 0)) return false;

    DayReport localReport;
    if (!report) report = &localReport;

    const BalanceConfig* cfg = Balance_GetGlobal();
    RationNeeds needs = Ration_GetNeeds(army, NULL, army->position);

    /* 1. Descontar recursos de los inventarios */
    float sum[RS_COUNT] = {0};
    for (int g = 0; g < RG_COUNT; g++) {
        for (int r = 0; r < RS_COUNT; r++) {
            sum[r] += plan->amount[g][r];
        }
    }

    army->agua_disponible   -= sum[RS_AGUA];   if (army->agua_disponible < 0.0f)   army->agua_disponible = 0.0f;
    army->vino_disponible   -= sum[RS_VINO];   if (army->vino_disponible < 0.0f)   army->vino_disponible = 0.0f;
    army->comida_disponible -= sum[RS_COMIDA]; if (army->comida_disponible < 0.0f) army->comida_disponible = 0.0f;
    army->carne_disponible  -= sum[RS_CARNE];  if (army->carne_disponible < 0.0f)  army->carne_disponible = 0.0f;

    /* 2. Evaluar y actualizar cada grupo */
    for (int g = 0; g < RG_COUNT; g++) {
        RationLevel level;
        float moraleDelta, saludDelta, satDelta;
        int daysUnfed;
        bool risksDesertion;

        CalculateGroupDeltas(army, (RationGroup)g, plan, &needs,
                             &level, &moraleDelta, &saludDelta, &satDelta,
                             &daysUnfed, &risksDesertion);

        army->groupState[g].daysUnfed = daysUnfed;

        if (g == RG_GANADO) {
            army->groupState[g].health += saludDelta;
            if (army->groupState[g].health > 100.0f) army->groupState[g].health = 100.0f;
            if (army->groupState[g].health <= 0.0f) {
                army->groupState[g].health = 0.0f;
                army->ganado = 0; /* Ganado muerto: carne perdida */
            }
        } else {
            army->groupState[g].morale += moraleDelta;
            if (army->groupState[g].morale < 0.0f) army->groupState[g].morale = 0.0f;
            if (army->groupState[g].morale > 100.0f) army->groupState[g].morale = 100.0f;

            army->groupState[g].health += saludDelta;
            if (army->groupState[g].health < 0.0f) army->groupState[g].health = 0.0f;
            if (army->groupState[g].health > 100.0f) army->groupState[g].health = 100.0f;

            if (g == RG_SARGENTOS) {
                army->groupState[g].satisfaction += satDelta;
                if (army->groupState[g].satisfaction < 0.0f) army->groupState[g].satisfaction = 0.0f;
                if (army->groupState[g].satisfaction > army->groupState[g].satisfactionMax) {
                    army->groupState[g].satisfaction = army->groupState[g].satisfactionMax;
                }
            }

            /* Bajas y deserciones por hambre/sed */
            if (daysUnfed > 0 && cfg) {
                int displacement = (g == RG_INICIADOS) ? -1 : (g == RG_SOLDADOS) ? 0 : 1;
                int effectiveDay = daysUnfed - displacement;
                if (effectiveDay >= 1) {
                    int rowIdx = effectiveDay - 1;
                    if (rowIdx > 2) rowIdx = 2;
                    float rankMult = (g == RG_INICIADOS) ? 1.0f : (g == RG_SOLDADOS) ? 0.75f : 0.5f;

                    float totDesert = 0.0f, totDeath = 0.0f;
                    float ratioF = (needs.food[g] > 0.001f) ? ((plan->amount[g][RS_COMIDA] + plan->amount[g][RS_CARNE]) / needs.food[g]) : 1.0f;
                    float ratioD = (needs.drink[g] > 0.001f) ? ((plan->amount[g][RS_AGUA] + plan->amount[g][RS_VINO]) / needs.drink[g]) : 1.0f;

                    if (ratioF < 0.5f) {
                        totDesert += cfg->starvationFood[rowIdx].desertPct;
                        totDeath  += cfg->starvationFood[rowIdx].deathPct;
                    }
                    if (ratioD < 0.5f) {
                        totDesert += cfg->starvationWater[rowIdx].desertPct;
                        totDeath  += cfg->starvationWater[rowIdx].deathPct;
                    }

                    int deserted = 0, dead = 0;
                    if (g == RG_INICIADOS) {
                        Army_CalculateRankCasualties(&army->iniciados, totDesert, totDeath, rankMult, &deserted, &dead, rng);
                        report->desertedInitiates += deserted;
                        report->deadInitiates += dead;
                    } else if (g == RG_SOLDADOS) {
                        Army_CalculateRankCasualties(&army->soldados, totDesert, totDeath, rankMult, &deserted, &dead, rng);
                        report->desertedSoldiers += deserted;
                        report->deadSoldiers += dead;
                    } else if (g == RG_SARGENTOS) {
                        Army_CalculateRankCasualties(&army->sargentos, totDesert, totDeath, rankMult, &deserted, &dead, rng);
                        report->desertedSergeants += deserted;
                        report->deadSergeants += dead;
                    }
                }
            }
        }
    }

    /* 3. Recalcular agregados derivados y registrar plan para repetición */
    Army_RecalculateAggregates(army);
    army->lastRationPlan = *plan;
    army->hasLastRationPlan = true;
    army->dayRationCommitted = true;

    /* 4. Completar reporte del día */
    report->foodConsumed = sum[RS_COMIDA] + sum[RS_CARNE];
    report->waterConsumed = sum[RS_AGUA] + sum[RS_VINO];
    report->foodShortage = (army->comida_disponible <= 0.001f && army->carne_disponible <= 0.001f);
    report->waterShortage = (army->agua_disponible <= 0.001f && army->vino_disponible <= 0.001f);
    report->daysWithoutFood = army->daysWithoutFood;
    report->daysWithoutWater = army->daysWithoutWater;

    return true;
}

int Ration_SlaughterCattle(Army* army, int heads) {
    if (!army || heads <= 0 || army->ganado <= 0) return 0;
    if (heads > army->ganado) heads = army->ganado;

    army->ganado -= heads;

    const BalanceConfig* cfg = Balance_GetGlobal();
    int yieldPerHead = (cfg && cfg->cattleMeatYield > 0) ? cfg->cattleMeatYield : 6;
    int meatGained = heads * yieldPerHead;

    army->carne_disponible += (float)meatGained;
    return meatGained;
}
