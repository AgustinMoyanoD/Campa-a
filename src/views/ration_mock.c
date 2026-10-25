#include "views/ration_mock.h"
#include <string.h>
#include <stdio.h>

static RationMockScenario s_scenario = MOCK_SCENARIO_SHORTAGE;

void RationMock_SetScenario(RationMockScenario sc) {
    s_scenario = sc;
}

RationMockScenario RationMock_GetScenario(void) {
    return s_scenario;
}

RationNeeds RationMock_GetNeeds(const Army* army, const Map* map, HexCoord pos) {
    (void)army; (void)map; (void)pos;
    RationNeeds needs;
    memset(&needs, 0, sizeof(RationNeeds));
    switch (s_scenario) {
        case MOCK_SCENARIO_ABUNDANCE:
            needs.drink[RG_INICIADOS] = 10.0f; needs.food[RG_INICIADOS] = 10.0f;
            needs.drink[RG_SOLDADOS]  = 20.0f; needs.food[RG_SOLDADOS]  = 20.0f;
            needs.drink[RG_SARGENTOS] = 5.0f;  needs.food[RG_SARGENTOS] = 5.0f;
            needs.drink[RG_GANADO]    = 15.0f; needs.food[RG_GANADO]    = 0.0f; /* pastoreo */
            break;
        case MOCK_SCENARIO_STARVING_GROUP:
            needs.drink[RG_INICIADOS] = 20.0f; needs.food[RG_INICIADOS] = 20.0f;
            needs.drink[RG_SOLDADOS]  = 30.0f; needs.food[RG_SOLDADOS]  = 30.0f;
            needs.drink[RG_SARGENTOS] = 10.0f; needs.food[RG_SARGENTOS] = 10.0f;
            needs.drink[RG_GANADO]    = 10.0f; needs.food[RG_GANADO]    = 0.0f;
            break;
        case MOCK_SCENARIO_ZERO_SATISFACTION:
            needs.drink[RG_INICIADOS] = 12.0f; needs.food[RG_INICIADOS] = 12.0f;
            needs.drink[RG_SOLDADOS]  = 22.0f; needs.food[RG_SOLDADOS]  = 22.0f;
            needs.drink[RG_SARGENTOS] = 8.0f;  needs.food[RG_SARGENTOS] = 8.0f;
            needs.drink[RG_GANADO]    = 12.0f; needs.food[RG_GANADO]    = 0.0f;
            break;
        case MOCK_SCENARIO_SHORTAGE:
        default:
            needs.drink[RG_INICIADOS] = 15.0f; needs.food[RG_INICIADOS] = 15.0f;
            needs.drink[RG_SOLDADOS]  = 25.0f; needs.food[RG_SOLDADOS]  = 25.0f;
            needs.drink[RG_SARGENTOS] = 8.0f;  needs.food[RG_SARGENTOS] = 8.0f;
            needs.drink[RG_GANADO]    = 10.0f; needs.food[RG_GANADO]    = 0.0f;
            break;
    }
    return needs;
}

float RationMock_GetStock(const Army* army, RationResource r) {
    (void)army;
    switch (s_scenario) {
        case MOCK_SCENARIO_ABUNDANCE:
            switch (r) {
                case RS_AGUA:   return 120.0f;
                case RS_VINO:   return 35.0f;
                case RS_COMIDA: return 140.0f;
                case RS_CARNE:  return 30.0f;
                default:        return 0.0f;
            }
        case MOCK_SCENARIO_STARVING_GROUP:
            switch (r) {
                case RS_AGUA:   return 15.0f;
                case RS_VINO:   return 0.0f;
                case RS_COMIDA: return 10.0f;
                case RS_CARNE:  return 0.0f;
                default:        return 0.0f;
            }
        case MOCK_SCENARIO_ZERO_SATISFACTION:
            switch (r) {
                case RS_AGUA:   return 60.0f;
                case RS_VINO:   return 0.0f;
                case RS_COMIDA: return 50.0f;
                case RS_CARNE:  return 0.0f;
                default:        return 0.0f;
            }
        case MOCK_SCENARIO_SHORTAGE:
        default:
            switch (r) {
                case RS_AGUA:   return 32.0f;
                case RS_VINO:   return 6.0f;
                case RS_COMIDA: return 28.0f;
                case RS_CARNE:  return 4.0f;
                default:        return 0.0f;
            }
    }
}

GroupState RationMock_GetGroupState(const Army* army, RationGroup g) {
    (void)army;
    GroupState st;
    memset(&st, 0, sizeof(GroupState));
    st.satisfactionMax = 100.0f;

    switch (s_scenario) {
        case MOCK_SCENARIO_ABUNDANCE:
            st.morale = 85.0f;
            st.health = 92.0f;
            st.daysUnfed = 0;
            st.satisfaction = (g == RG_SARGENTOS) ? 90.0f : 0.0f;
            break;
        case MOCK_SCENARIO_STARVING_GROUP:
            if (g == RG_INICIADOS) {
                st.morale = 20.0f;
                st.health = 45.0f;
                st.daysUnfed = 2;
                st.satisfaction = 0.0f;
            } else {
                st.morale = 45.0f;
                st.health = 60.0f;
                st.daysUnfed = 0;
                st.satisfaction = (g == RG_SARGENTOS) ? 25.0f : 0.0f;
            }
            break;
        case MOCK_SCENARIO_ZERO_SATISFACTION:
            st.morale = 50.0f;
            st.health = 75.0f;
            st.daysUnfed = 0;
            st.satisfaction = 0.0f;
            break;
        case MOCK_SCENARIO_SHORTAGE:
        default:
            st.morale = 60.0f;
            st.health = 70.0f;
            st.daysUnfed = 0;
            st.satisfaction = (g == RG_SARGENTOS) ? 35.0f : 0.0f;
            break;
    }
    return st;
}

RationPlan RationMock_Auto(const Army* army, const RationNeeds* needs) {
    RationPlan plan;
    memset(&plan, 0, sizeof(RationPlan));
    RationNeeds nd = needs ? *needs : RationMock_GetNeeds(army, NULL, (HexCoord){0,0});

    float stockAgua   = RationMock_GetStock(army, RS_AGUA);
    float stockVino   = RationMock_GetStock(army, RS_VINO);
    float stockComida = RationMock_GetStock(army, RS_COMIDA);
    float stockCarne  = RationMock_GetStock(army, RS_CARNE);

    /* Prioridad de reparto: Sargentos, Soldados, Iniciados, Ganado */
    int order[RG_COUNT] = { RG_SARGENTOS, RG_SOLDADOS, RG_INICIADOS, RG_GANADO };

    for (int i = 0; i < RG_COUNT; i++) {
        int g = order[i];
        float dNeed = nd.drink[g];
        float fNeed = nd.food[g];

        /* Sargentos reciben lujos primero si hay disponibles */
        if (g == RG_SARGENTOS) {
            float vTake = (stockVino >= dNeed) ? dNeed : stockVino;
            plan.amount[g][RS_VINO] = vTake;
            stockVino -= vTake;
            dNeed -= vTake;

            float cTake = (stockCarne >= fNeed) ? fNeed : stockCarne;
            plan.amount[g][RS_CARNE] = cTake;
            stockCarne -= cTake;
            fNeed -= cTake;
        }

        /* Bebida básica (Agua) */
        float wTake = (stockAgua >= dNeed) ? dNeed : stockAgua;
        plan.amount[g][RS_AGUA] = wTake;
        stockAgua -= wTake;

        /* Comida básica */
        float fdTake = (stockComida >= fNeed) ? fNeed : stockComida;
        plan.amount[g][RS_COMIDA] = fdTake;
        stockComida -= fdTake;
    }

    return plan;
}

RationPlan RationMock_Repeat(const Army* army) {
    return RationMock_Auto(army, NULL);
}

bool RationMock_Validate(const Army* army, const RationPlan* plan, char* err, int cap) {
    if (!plan) {
        if (err && cap > 0) snprintf(err, cap, "Plan no puede ser nulo");
        return false;
    }
    for (int g = 0; g < RG_COUNT; g++) {
        for (int r = 0; r < RS_COUNT; r++) {
            if (plan->amount[g][r] < -0.001f) {
                if (err && cap > 0) snprintf(err, cap, "Asignacion negativa invalida");
                return false;
            }
        }
    }
    if (plan->amount[RG_GANADO][RS_VINO] > 0.001f ||
        plan->amount[RG_GANADO][RS_COMIDA] > 0.001f ||
        plan->amount[RG_GANADO][RS_CARNE] > 0.001f) {
        if (err && cap > 0) snprintf(err, cap, "El ganado solo consume agua");
        return false;
    }
    float sum[RS_COUNT] = {0};
    for (int g = 0; g < RG_COUNT; g++) {
        for (int r = 0; r < RS_COUNT; r++) {
            sum[r] += plan->amount[g][r];
        }
    }
    for (int r = 0; r < RS_COUNT; r++) {
        float st = RationMock_GetStock(army, (RationResource)r);
        if (sum[r] > st + 0.001f) {
            if (err && cap > 0) snprintf(err, cap, "Asignacion excede el stock disponible");
            return false;
        }
    }
    return true;
}

RationPreview RationMock_Preview(const Army* army, const RationPlan* plan) {
    RationPreview prev;
    memset(&prev, 0, sizeof(RationPreview));

    RationNeeds needs = RationMock_GetNeeds(army, NULL, (HexCoord){0,0});

    float sumAssigned[RS_COUNT] = {0};
    for (int g = 0; g < RG_COUNT; g++) {
        for (int r = 0; r < RS_COUNT; r++) {
            sumAssigned[r] += plan->amount[g][r];
        }
    }
    for (int r = 0; r < RS_COUNT; r++) {
        float cur = RationMock_GetStock(army, (RationResource)r);
        prev.stockAfter[r] = (cur >= sumAssigned[r]) ? (cur - sumAssigned[r]) : 0.0f;
    }

    float totalDailyDrink = 0.0f;
    float totalDailyFood  = 0.0f;
    for (int g = 0; g < RG_COUNT; g++) {
        totalDailyDrink += needs.drink[g];
        totalDailyFood  += needs.food[g];
    }
    if (totalDailyDrink < 0.1f) totalDailyDrink = 1.0f;
    if (totalDailyFood < 0.1f)  totalDailyFood = 1.0f;

    prev.daysLeft[RS_AGUA]   = prev.stockAfter[RS_AGUA] / totalDailyDrink;
    prev.daysLeft[RS_VINO]   = prev.stockAfter[RS_VINO] / totalDailyDrink;
    prev.daysLeft[RS_COMIDA] = prev.stockAfter[RS_COMIDA] / totalDailyFood;
    prev.daysLeft[RS_CARNE]  = prev.stockAfter[RS_CARNE] / totalDailyFood;

    for (int g = 0; g < RG_COUNT; g++) {
        float drinkGot = plan->amount[g][RS_AGUA] + plan->amount[g][RS_VINO];
        float foodGot  = plan->amount[g][RS_COMIDA] + plan->amount[g][RS_CARNE];
        float dNeed = needs.drink[g];
        float fNeed = needs.food[g];

        float drinkRatio = (dNeed > 0.001f) ? (drinkGot / dNeed) : 1.0f;
        float foodRatio  = (fNeed > 0.001f) ? (foodGot / fNeed)  : 1.0f;
        float minRatio   = (drinkRatio < foodRatio) ? drinkRatio : foodRatio;

        GroupState curSt = RationMock_GetGroupState(army, (RationGroup)g);

        if (minRatio >= 0.99f) {
            prev.group[g].level = RATION_FULL;
            prev.group[g].moraleDelta = (g == RG_GANADO) ? 0.0f : +4.0f;
            prev.group[g].healthDelta = +5.0f;
            prev.group[g].daysUnfed   = 0;
            prev.group[g].risksDesertion = false;
        } else if (minRatio >= 0.50f) {
            prev.group[g].level = RATION_REDUCED;
            prev.group[g].moraleDelta = (g == RG_GANADO) ? 0.0f : -2.0f;
            prev.group[g].healthDelta = 0.0f;
            prev.group[g].daysUnfed   = curSt.daysUnfed;
            prev.group[g].risksDesertion = (curSt.daysUnfed > 1);
        } else {
            prev.group[g].level = RATION_STARVING;
            prev.group[g].moraleDelta = (g == RG_GANADO) ? 0.0f : -8.0f;
            prev.group[g].healthDelta = -6.0f;
            prev.group[g].daysUnfed   = curSt.daysUnfed + 1;
            prev.group[g].risksDesertion = true;
        }

        /* Lujos suben satisfacción a Sargentos y aportan bono de moral */
        if (g == RG_SARGENTOS) {
            float luxuryGot = plan->amount[g][RS_VINO] + plan->amount[g][RS_CARNE];
            if (luxuryGot >= 4.0f) {
                prev.group[g].satisfactionDelta = +15.0f;
                prev.group[g].moraleDelta += 3.0f;
            } else if (luxuryGot > 0.0f) {
                prev.group[g].satisfactionDelta = +8.0f;
            } else {
                prev.group[g].satisfactionDelta = -5.0f;
            }
        }
    }

    return prev;
}

bool RationMock_NeedsDecision(const Army* army, const Map* map, HexCoord pos) {
    (void)army; (void)map; (void)pos;
    return (s_scenario != MOCK_SCENARIO_ABUNDANCE);
}
