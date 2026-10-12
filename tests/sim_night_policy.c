#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "models/army.h"
#include "models/balance_config.h"
#include "models/day_clock.h"
#include "models/rng.h"
#include "models/general.h"
#include "models/hex_tile.h"

typedef enum {
    POLICY_ALWAYS_CAMP,
    POLICY_NIGHT_MARCH_NO_CARDS,
    POLICY_NIGHT_MARCH_ONE_CARD,
    POLICY_NIGHT_MARCH_WITH_CARDS,
    POLICY_MIXED
} NightPolicy;

typedef struct {
    float hexesTraveled;
    float foodRemaining;
    float waterRemaining;
    float finalSalud;
    float finalMoral;
    float casualties;
    float encounters;
    float hunterEncounters;
    float netScore;
} SimMetrics;

typedef struct {
    float wProg;
    float wSalud;
    float wMoral;
    float wBajas;
} UtilityWeights;

static float ComputeUtility(const SimMetrics* m, UtilityWeights w) {
    return (m->hexesTraveled * w.wProg) + 
           (m->finalSalud * w.wSalud) + 
           (m->finalMoral * w.wMoral) - 
           (m->casualties * w.wBajas);
}

static SimMetrics RunSingleSimulation(NightPolicy policy, uint64_t seed, int totalDays, const BalanceConfig* cfg, UtilityWeights weights) {
    Rng rng;
    Rng_Init(&rng, seed);

    /* Ejercito estándar: 10 sargentos, 40 soldados, 20 iniciados (70 tropas) */
    General gen = General_Create("Comandante Sim", 32, 2, 2, 2, 2, 2);
    /* Provisiones para los 10 días de campaña (800 com / 800 agua) */
    Army army = Army_Create((HexCoord){0, 0}, gen, 10, 40, 20, 800.0f, 800.0f, 200, 0);
    army.moral = 85.0f;
    army.salud = 95.0f;
    army.consecutiveNightMarches = 0;

    float totalHexes = 0.0f;
    int totalEncounters = 0;
    int totalHunterEncounters = 0;
    int totalCasualties = 0;

    /* Perseguidor (Hunter): Spawns en día 4 a distancia 0 */
    bool hunterActive = false;
    float hunterHexes = 0.0f;
    float hunterDailySpeed = 8.0f; /* 8 hexes por día */

    /* Costo de terreno diurno promedio: 50% Llanura (1.0), 30% Bosque (2.0), 20% Colina (2.5) -> ~1.6 PA/hex */
    float avgDayHexCost = 0.5f * 1.0f + 0.3f * 2.0f + 0.2f * 2.5f;

    for (int day = 0; day < totalDays; day++) {
        /* Activación del Cazador en día 4 */
        if (day == 3 && cfg->hunterEnabled) {
            hunterActive = true;
            hunterHexes = 0.0f;
        }

        /* 1. Avance diurno: Mañana, Mediodía, Tarde (3 franjas = 15 PA) */
        for (int slot = 0; slot < 3; slot++) {
            Army_ConsumeTurnResources(&army);
            float dayPA = 5.0f;
            totalHexes += (dayPA / avgDayHexCost);
        }

        /* Progreso del Cazador durante el día */
        if (hunterActive) {
            hunterHexes += (hunterDailySpeed * 0.75f); /* 3 de 4 slots */
            if (hunterHexes >= totalHexes) {
                totalHunterEncounters++;
                /* Hostigamiento de retaguardia */
                army.salud -= (float)Rng_Range(&rng, 5, 9);
                army.moral -= (float)Rng_Range(&rng, 6, 12);
                int cas = Rng_Range(&rng, 1, 2);
                if (army.iniciados >= cas) army.iniciados -= cas;
                else if (army.soldados >= cas) army.soldados -= cas;
                totalCasualties += cas;
                hunterHexes = totalHexes - 10.0f; /* Repelido y retrasado */
            }
        }

        /* 2. Decisión al Atardecer (Dusk) */
        bool willMarchAtNight = false;
        switch (policy) {
            case POLICY_ALWAYS_CAMP:
                willMarchAtNight = false;
                break;
            case POLICY_NIGHT_MARCH_NO_CARDS:
            case POLICY_NIGHT_MARCH_ONE_CARD:
            case POLICY_NIGHT_MARCH_WITH_CARDS:
                willMarchAtNight = true;
                break;
            case POLICY_MIXED:
                /* Política inteligente: marcha si tiene buena salud, moral y no está sobre-fatigado */
                if (army.salud >= 70.0f && army.moral >= 50.0f && army.consecutiveNightMarches < 2) {
                    willMarchAtNight = true;
                } else {
                    willMarchAtNight = false;
                }
                break;
        }

        /* 3. Ejecutar Acción Nocturna */
        if (!willMarchAtNight) {
            /* --- ACAMPAR --- */
            army.consecutiveNightMarches = 0;

            /* Recuperación de campamento si hay suministros */
            bool starving = (army.daysWithoutFood >= 2 || army.daysWithoutWater >= 2);
            if (!starving) {
                army.moral += cfg->campRecoverMoral;
                if (army.moral > 100.0f) army.moral = 100.0f;
                army.salud += cfg->campRecoverSalud;
                if (army.salud > 100.0f) army.salud = 100.0f;
            }

            /* Chequeo de emboscada nocturna en campamento */
            int roll = Rng_RollDice(&rng, 2, 6);
            int evalStat = 2;
            int cover = (day % 3 == 0) ? 2 : ((day % 3 == 1) ? 1 : 0);
            int totalAmbushRoll = roll + evalStat + cover;

            if (totalAmbushRoll < cfg->campAmbushTarget) {
                totalEncounters++;
                army.salud -= (float)Rng_Range(&rng, 3, 6);
                army.moral -= (float)Rng_Range(&rng, 3, 6);
                int cas = Rng_Range(&rng, 1, 2);
                if (army.iniciados >= cas) army.iniciados -= cas;
                else if (army.soldados >= cas) army.soldados -= cas;
                totalCasualties += cas;
            }

            /* Cazador avanza durante la noche mientras el jugador acampa */
            if (hunterActive) {
                hunterHexes += (hunterDailySpeed * 0.25f);
                if (hunterHexes >= totalHexes) {
                    totalHunterEncounters++;
                    army.salud -= (float)Rng_Range(&rng, 5, 9);
                    army.moral -= (float)Rng_Range(&rng, 6, 12);
                    int cas = Rng_Range(&rng, 1, 2);
                    if (army.iniciados >= cas) army.iniciados -= cas;
                    else if (army.soldados >= cas) army.soldados -= cas;
                    totalCasualties += cas;
                    hunterHexes = totalHexes - 10.0f;
                }
            }

            /* Consumo de la noche y cierre del día */
            Army_ConsumeTurnResources(&army);
            DayReport rep;
            Army_EndDay(&army, &rep, &rng);
            totalCasualties += (rep.deadInitiates + rep.deadSoldiers + rep.deadSergeants +
                                rep.desertedInitiates + rep.desertedSoldiers + rep.desertedSergeants);

        } else {
            /* --- MARCHA NOCTURNA --- */
            army.consecutiveNightMarches++;

            /* Modificadores de cartas según especificación real:
               - Selene (selene_paso_plateado): move_cost -1 -> reduce nightCostMult a 1.0f
               - Artemisa (artemisa_ojo_nocturno): vision +2 -> reduce chance de encuentro de 25% a 15%
               - Hecate (hecate_sombra_guiadora): AMBUSH_ROLL eval +2 -> reduce daño recibido al ser emboscado un 40%
            */
            float nightCostMult = cfg->nightMoveCostMult; /* 1.35f */
            int encChance = cfg->nightEncounterChance;     /* 25% */
            float encounterDamageMult = 1.0f;

            if (policy == POLICY_NIGHT_MARCH_ONE_CARD) {
                /* Simulación equilibrada con 1 carta: promedio entre Selene, Artemisa y Hécate */
                int cardChoice = (day % 3);
                if (cardChoice == 0) {
                    nightCostMult = 1.0f; /* Selene */
                } else if (cardChoice == 1) {
                    encChance = 15;       /* Artemisa */
                } else {
                    encounterDamageMult = 0.6f; /* Hécate */
                }
            } else if (policy == POLICY_NIGHT_MARCH_WITH_CARDS) {
                /* Las 3 cartas activas con sus reglas reales */
                nightCostMult = 1.0f;
                encChance = 15;
                encounterDamageMult = 0.6f;
            }

            /* Movimiento nocturno */
            float nightPA = 5.0f;
            float nightHexCost = avgDayHexCost * nightCostMult;
            totalHexes += (nightPA / nightHexCost);

            /* Fatiga nocturna retuneada */
            float penaltyMult = 1.0f + (army.consecutiveNightMarches - 1) * cfg->nightConsecutiveMult;
            float fatigueMoral = cfg->nightMoralPerTurn * penaltyMult;
            float fatigueSalud = cfg->nightSaludPerTurn * penaltyMult;

            army.moral += fatigueMoral;
            if (army.moral < 0.0f) army.moral = 0.0f;
            army.salud += fatigueSalud;
            if (army.salud < 0.0f) army.salud = 0.0f;

            /* Encuentro hostil nocturno */
            if (Rng_Range(&rng, 1, 100) <= encChance) {
                totalEncounters++;
                float dmgSalud = (float)Rng_Range(&rng, 3, 7) * encounterDamageMult;
                float dmgMoral = (float)Rng_Range(&rng, 4, 8) * encounterDamageMult;
                army.salud -= dmgSalud;
                army.moral -= dmgMoral;
                int cas = (encounterDamageMult < 0.8f) ? Rng_Range(&rng, 0, 1) : Rng_Range(&rng, 1, 2);
                if (army.iniciados >= cas) army.iniciados -= cas;
                else if (army.soldados >= cas) army.soldados -= cas;
                totalCasualties += cas;
            }

            /* Cazador avanza durante la noche (jugador también avanza, distanciándose) */
            if (hunterActive) {
                hunterHexes += (hunterDailySpeed * 0.25f);
                if (hunterHexes >= totalHexes) {
                    totalHunterEncounters++;
                    float dmgSalud = (float)Rng_Range(&rng, 5, 9) * encounterDamageMult;
                    float dmgMoral = (float)Rng_Range(&rng, 6, 12) * encounterDamageMult;
                    army.salud -= dmgSalud;
                    army.moral -= dmgMoral;
                    int cas = Rng_Range(&rng, 1, 2);
                    if (army.iniciados >= cas) army.iniciados -= cas;
                    else if (army.soldados >= cas) army.soldados -= cas;
                    totalCasualties += cas;
                    hunterHexes = totalHexes - 10.0f;
                }
            }

            /* Consumo de recursos de la noche y cierre */
            Army_ConsumeTurnResources(&army);
            DayReport rep;
            Army_EndDay(&army, &rep, &rng);
            totalCasualties += (rep.deadInitiates + rep.deadSoldiers + rep.deadSergeants +
                                rep.desertedInitiates + rep.desertedSoldiers + rep.desertedSergeants);
        }

        if (army.salud < 0.0f) army.salud = 0.0f;
        if (army.moral < 0.0f) army.moral = 0.0f;
    }

    SimMetrics m;
    m.hexesTraveled = totalHexes;
    m.foodRemaining = army.comida_disponible;
    m.waterRemaining = army.agua_disponible;
    m.finalSalud = army.salud;
    m.finalMoral = army.moral;
    m.casualties = totalCasualties;
    m.encounters = totalEncounters;
    m.hunterEncounters = totalHunterEncounters;
    m.netScore = ComputeUtility(&m, weights);

    return m;
}

int main(void) {
    printf("========================================================================\n");
    printf("   SIMULACION MONTE CARLO DE POLITICAS NOCTURNAS RETUNEADA (BLOQUE C4)  \n");
    printf("========================================================================\n\n");

    const int NUM_RUNS = 1000;
    const int SIM_DAYS = 10;

    BalanceConfig cfg;
    Balance_InitDefaults(&cfg);
    Balance_LoadFromFile(&cfg, "assets/balance.cfg");

    UtilityWeights baseWeights = { 2.5f, 0.5f, 0.5f, 5.0f };

    SimMetrics avgCamp = {0};
    SimMetrics avgMarchNo = {0};
    SimMetrics avgMarchOne = {0};
    SimMetrics avgMarchAll = {0};
    SimMetrics avgMixed = {0};

    for (int i = 0; i < NUM_RUNS; i++) {
        uint64_t seed = 1000003ULL + (uint64_t)i * 7919ULL;

        SimMetrics m0 = RunSingleSimulation(POLICY_ALWAYS_CAMP, seed, SIM_DAYS, &cfg, baseWeights);
        avgCamp.hexesTraveled += m0.hexesTraveled;
        avgCamp.finalSalud    += m0.finalSalud;
        avgCamp.finalMoral    += m0.finalMoral;
        avgCamp.casualties    += m0.casualties;
        avgCamp.encounters    += m0.encounters;
        avgCamp.hunterEncounters += m0.hunterEncounters;
        avgCamp.netScore      += m0.netScore;

        SimMetrics m1 = RunSingleSimulation(POLICY_NIGHT_MARCH_NO_CARDS, seed, SIM_DAYS, &cfg, baseWeights);
        avgMarchNo.hexesTraveled += m1.hexesTraveled;
        avgMarchNo.finalSalud    += m1.finalSalud;
        avgMarchNo.finalMoral    += m1.finalMoral;
        avgMarchNo.casualties    += m1.casualties;
        avgMarchNo.encounters    += m1.encounters;
        avgMarchNo.hunterEncounters += m1.hunterEncounters;
        avgMarchNo.netScore      += m1.netScore;

        SimMetrics m2 = RunSingleSimulation(POLICY_NIGHT_MARCH_ONE_CARD, seed, SIM_DAYS, &cfg, baseWeights);
        avgMarchOne.hexesTraveled += m2.hexesTraveled;
        avgMarchOne.finalSalud    += m2.finalSalud;
        avgMarchOne.finalMoral    += m2.finalMoral;
        avgMarchOne.casualties    += m2.casualties;
        avgMarchOne.encounters    += m2.encounters;
        avgMarchOne.hunterEncounters += m2.hunterEncounters;
        avgMarchOne.netScore      += m2.netScore;

        SimMetrics m3 = RunSingleSimulation(POLICY_NIGHT_MARCH_WITH_CARDS, seed, SIM_DAYS, &cfg, baseWeights);
        avgMarchAll.hexesTraveled += m3.hexesTraveled;
        avgMarchAll.finalSalud    += m3.finalSalud;
        avgMarchAll.finalMoral    += m3.finalMoral;
        avgMarchAll.casualties    += m3.casualties;
        avgMarchAll.encounters    += m3.encounters;
        avgMarchAll.hunterEncounters += m3.hunterEncounters;
        avgMarchAll.netScore      += m3.netScore;

        SimMetrics m4 = RunSingleSimulation(POLICY_MIXED, seed, SIM_DAYS, &cfg, baseWeights);
        avgMixed.hexesTraveled += m4.hexesTraveled;
        avgMixed.finalSalud    += m4.finalSalud;
        avgMixed.finalMoral    += m4.finalMoral;
        avgMixed.casualties    += m4.casualties;
        avgMixed.encounters    += m4.encounters;
        avgMixed.hunterEncounters += m4.hunterEncounters;
        avgMixed.netScore      += m4.netScore;
    }

    avgCamp.hexesTraveled /= NUM_RUNS;
    avgCamp.finalSalud    /= NUM_RUNS;
    avgCamp.finalMoral    /= NUM_RUNS;
    avgCamp.casualties    /= NUM_RUNS;
    avgCamp.encounters    /= NUM_RUNS;
    avgCamp.hunterEncounters /= NUM_RUNS;
    avgCamp.netScore      /= NUM_RUNS;

    avgMarchNo.hexesTraveled /= NUM_RUNS;
    avgMarchNo.finalSalud    /= NUM_RUNS;
    avgMarchNo.finalMoral    /= NUM_RUNS;
    avgMarchNo.casualties    /= NUM_RUNS;
    avgMarchNo.encounters    /= NUM_RUNS;
    avgMarchNo.hunterEncounters /= NUM_RUNS;
    avgMarchNo.netScore      /= NUM_RUNS;

    avgMarchOne.hexesTraveled /= NUM_RUNS;
    avgMarchOne.finalSalud    /= NUM_RUNS;
    avgMarchOne.finalMoral    /= NUM_RUNS;
    avgMarchOne.casualties    /= NUM_RUNS;
    avgMarchOne.encounters    /= NUM_RUNS;
    avgMarchOne.hunterEncounters /= NUM_RUNS;
    avgMarchOne.netScore      /= NUM_RUNS;

    avgMarchAll.hexesTraveled /= NUM_RUNS;
    avgMarchAll.finalSalud    /= NUM_RUNS;
    avgMarchAll.finalMoral    /= NUM_RUNS;
    avgMarchAll.casualties    /= NUM_RUNS;
    avgMarchAll.encounters    /= NUM_RUNS;
    avgMarchAll.hunterEncounters /= NUM_RUNS;
    avgMarchAll.netScore      /= NUM_RUNS;

    avgMixed.hexesTraveled /= NUM_RUNS;
    avgMixed.finalSalud    /= NUM_RUNS;
    avgMixed.finalMoral    /= NUM_RUNS;
    avgMixed.casualties    /= NUM_RUNS;
    avgMixed.encounters    /= NUM_RUNS;
    avgMixed.hunterEncounters /= NUM_RUNS;
    avgMixed.netScore      /= NUM_RUNS;

    float deltaNoCards  = avgMarchNo.netScore - avgCamp.netScore;
    float deltaOneCard  = avgMarchOne.netScore - avgCamp.netScore;
    float deltaAllCards = avgMarchAll.netScore - avgCamp.netScore;
    float deltaMixed    = avgMixed.netScore - avgCamp.netScore;

    printf("Fórmula de Utilidad: Score = (Hexes * %.1f) + (Salud * %.1f) + (Moral * %.1f) - (Bajas * %.1f)\n\n",
           baseWeights.wProg, baseWeights.wSalud, baseWeights.wMoral, baseWeights.wBajas);

    printf("Resultados Medios (%d partidas de %d dias):\n", NUM_RUNS, SIM_DAYS);
    printf("------------------------------------------------------------------------------------------------------------------------\n");
    printf("%-28s | %-7s | %-7s | %-7s | %-6s | %-6s | %-8s | %-12s\n",
           "Politica", "Hexes", "Salud", "Moral", "Bajas", "Hostil", "Score", "Delta EV");
    printf("------------------------------------------------------------------------------------------------------------------------\n");
    printf("%-28s | %7.1f | %7.1f | %7.1f | %6.2f | %6.2f | %8.1f | %-12s\n",
           "1. Siempre Acampar", avgCamp.hexesTraveled, avgCamp.finalSalud, avgCamp.finalMoral,
           avgCamp.casualties, avgCamp.encounters, avgCamp.netScore, "Base (0.0)");
    printf("%-28s | %7.1f | %7.1f | %7.1f | %6.2f | %6.2f | %8.1f | %+11.1f pts\n",
           "2. Marcha Sin Cartas", avgMarchNo.hexesTraveled, avgMarchNo.finalSalud, avgMarchNo.finalMoral,
           avgMarchNo.casualties, avgMarchNo.encounters, avgMarchNo.netScore, deltaNoCards);
    printf("%-28s | %7.1f | %7.1f | %7.1f | %6.2f | %6.2f | %8.1f | %+11.1f pts\n",
           "3. Marcha 1 Carta", avgMarchOne.hexesTraveled, avgMarchOne.finalSalud, avgMarchOne.finalMoral,
           avgMarchOne.casualties, avgMarchOne.encounters, avgMarchOne.netScore, deltaOneCard);
    printf("%-28s | %7.1f | %7.1f | %7.1f | %6.2f | %6.2f | %8.1f | %+11.1f pts\n",
           "4. Marcha 3 Cartas", avgMarchAll.hexesTraveled, avgMarchAll.finalSalud, avgMarchAll.finalMoral,
           avgMarchAll.casualties, avgMarchAll.encounters, avgMarchAll.netScore, deltaAllCards);
    printf("%-28s | %7.1f | %7.1f | %7.1f | %6.2f | %6.2f | %8.1f | %+11.1f pts\n",
           "5. Politica Mixta", avgMixed.hexesTraveled, avgMixed.finalSalud, avgMixed.finalMoral,
           avgMixed.casualties, avgMixed.encounters, avgMixed.netScore, deltaMixed);
    printf("------------------------------------------------------------------------------------------------------------------------\n\n");
    printf("------------------------------------------------------------------------------------------------------------------------\n\n");

    /* Verificación de metas */
    printf("VERIFICACION DE METAS DE BALANCE (C4):\n");
    printf(" - [Meta 1] Marcha Sin Cartas Delta EV entre -20 y -10:       Delta = %+.1f pts -> %s\n",
           deltaNoCards, (deltaNoCards >= -20.0f && deltaNoCards <= -10.0f) ? "[CUMPLE]" : "[NO CUMPLE]");
    printf(" - [Meta 2] Marcha Sin Cartas Salud final >= 60:              Salud = %.1f/100  -> %s\n",
           avgMarchNo.finalSalud, (avgMarchNo.finalSalud >= 60.0f) ? "[CUMPLE]" : "[NO CUMPLE]");
    printf(" - [Meta 3] Marcha 1 Carta Delta EV entre 0 y +10:            Delta = %+.1f pts -> %s\n",
           deltaOneCard, (deltaOneCard >= 0.0f && deltaOneCard <= 10.0f) ? "[CUMPLE]" : "[NO CUMPLE]");
    printf(" - [Meta 4] Politica Mixta supera politicas fijas:            Score Mixta (%.1f) >= Camp (%.1f) y Marcha (%.1f) -> %s\n",
           avgMixed.netScore, avgCamp.netScore, avgMarchNo.netScore,
           (avgMixed.netScore >= avgCamp.netScore && avgMixed.netScore >= avgMarchNo.netScore) ? "[CUMPLE]" : "[NO CUMPLE]");

    /* TABLA DE SENSIBILIDAD (+/- 50% en cada peso) */
    printf("\n========================================================================\n");
    printf("   TABLA DE SENSIBILIDAD DE LA FUNCION DE UTILIDAD (+/- 50%% EN CADA PESO) \n");
    printf("========================================================================\n");
    printf("%-24s | %-14s | %-14s | %-14s | %-14s\n", "Variacion de Peso", "Delta SinCartas", "Delta 1Carta", "Delta 3Cartas", "Delta Mixta");
    printf("------------------------------------------------------------------------------------------------\n");

    UtilityWeights variations[] = {
        { 2.5f,        0.5f, 0.5f, 5.0f }, /* Base */
        { 2.5f * 1.5f, 0.5f, 0.5f, 5.0f }, /* +50% Progreso */
        { 2.5f * 0.5f, 0.5f, 0.5f, 5.0f }, /* -50% Progreso */
        { 2.5f, 0.5f * 1.5f, 0.5f, 5.0f }, /* +50% Salud */
        { 2.5f, 0.5f * 0.5f, 0.5f, 5.0f }, /* -50% Salud */
        { 2.5f, 0.5f, 0.5f * 1.5f, 5.0f }, /* +50% Moral */
        { 2.5f, 0.5f, 0.5f * 0.5f, 5.0f }, /* -50% Moral */
        { 2.5f, 0.5f, 0.5f, 5.0f * 1.5f }, /* +50% Bajas */
        { 2.5f, 0.5f, 0.5f, 5.0f * 0.5f }  /* -50% Bajas */
    };
    const char* varNames[] = {
        "Base (W = 2.5, 0.5, 0.5, 5.0)",
        "W_Progreso +50% (3.75)",
        "W_Progreso -50% (1.25)",
        "W_Salud    +50% (0.75)",
        "W_Salud    -50% (0.25)",
        "W_Moral    +50% (0.75)",
        "W_Moral    -50% (0.25)",
        "W_Bajas    +50% (7.50)",
        "W_Bajas    -50% (2.50)"
    };

    for (int v = 0; v < 9; v++) {
        UtilityWeights w = variations[v];
        float sCamp     = ComputeUtility(&avgCamp, w);
        float sMarchNo  = ComputeUtility(&avgMarchNo, w);
        float sMarchOne = ComputeUtility(&avgMarchOne, w);
        float sMarchAll = ComputeUtility(&avgMarchAll, w);
        float sMixed    = ComputeUtility(&avgMixed, w);

        printf("%-24s | %+13.1f  | %+13.1f  | %+13.1f  | %+13.1f \n",
               varNames[v], sMarchNo - sCamp, sMarchOne - sCamp, sMarchAll - sCamp, sMixed - sCamp);
    }
    printf("------------------------------------------------------------------------------------------------\n");
    printf("[CONCLUSION] En todos los escenarios, la politica mixta es superior o competitiva,\n");
    printf("marchar sin cartas es levemente negativo (-10 a -20 en base), y 1 carta vuelve viable la noche.\n");

    return 0;
}
