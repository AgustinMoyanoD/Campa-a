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

typedef enum {
    POLICY_ALWAYS_CAMP,
    POLICY_NIGHT_MARCH_NO_CARDS,
    POLICY_NIGHT_MARCH_WITH_CARDS
} NightPolicy;

typedef struct {
    float hexesTraveled;
    float foodConsumed;
    float waterConsumed;
    float finalSalud;
    float finalMoral;
    int   casualties;
    int   encounters;
    float netScore;
} SimMetrics;

static SimMetrics RunSingleSimulation(NightPolicy policy, uint64_t seed, int totalDays, const BalanceConfig* cfg) {
    Rng rng;
    Rng_Init(&rng, seed);

    /* Ejercito promedio: 10 sargentos, 40 soldados, 20 iniciados (70 tropas) */
    General gen = General_Create("Comandante Sim", 32, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 0}, gen, 10, 40, 20, 4000.0f, 4000.0f, 200, 0);
    army.moral = 85.0f;
    army.salud = 90.0f;
    army.consecutiveNightMarches = 0;

    int initialTroops = Army_GetTotalSoldiers(&army);
    float initialFood = army.comida_disponible;
    float initialWater = army.agua_disponible;

    float totalHexes = 0.0f;
    int totalEncounters = 0;
    int totalCasualties = 0;

    /* Pesos de biomas por distribución de campaña: 50% Llanura (cost 1), 30% Bosque (cost 2), 20% Colinas/Montaña (cost 2.5) */
    float avgDayHexCost = 0.5f * 1.0f + 0.3f * 2.0f + 0.2f * 2.5f; /* ~1.6 PA/hex */

    for (int day = 0; day < totalDays; day++) {
        /* Mañana, Mediodía, Tarde: Avance normal diurno */
        for (int slot = 0; slot < 3; slot++) {
            Army_ConsumeTurnResources(&army);
            /* Cada franja diurna aporta 5 PA de avance */
            float dayPA = 5.0f;
            totalHexes += (dayPA / avgDayHexCost);
        }

        /* Franja de la TARDE -> Decisión de Campamento o Marcha Nocturna */
        if (policy == POLICY_ALWAYS_CAMP) {
            /* 1. Reset de marchas consecutivas */
            army.consecutiveNightMarches = 0;

            /* 2. Recuperación en campamento */
            bool starving = (army.daysWithoutFood >= 2 || army.daysWithoutWater >= 2);
            if (!starving) {
                army.moral += cfg->campRecoverMoral;
                if (army.moral > 100.0f) army.moral = 100.0f;
                army.salud += cfg->campRecoverSalud;
                if (army.salud > 100.0f) army.salud = 100.0f;
            }

            /* 3. Tirada de emboscada: 2d6 + Eval (2) + Cobertura (~1.0 promedio) */
            int roll = Rng_RollDice(&rng, 2, 6);
            int evalStat = 2;
            int cover = (day % 3 == 0) ? 2 : ((day % 3 == 1) ? 1 : 0); /* Alterna bosque/colina/llanura */
            int totalAmbushRoll = roll + evalStat + cover;

            if (totalAmbushRoll < cfg->campAmbushTarget) {
                totalEncounters++;
                /* Emboscada nocturna en campamento: combate defensivo con daño moderado */
                army.salud -= (float)Rng_Range(&rng, 4, 8);
                army.moral -= (float)Rng_Range(&rng, 5, 9);
                int cas = Rng_Range(&rng, 1, 3);
                if (army.iniciados >= cas) army.iniciados -= cas;
                else if (army.soldados >= cas) army.soldados -= cas;
                totalCasualties += cas;
            }

            /* 4. Cierre del día */
            Army_ConsumeTurnResources(&army);
            DayReport rep;
            Army_EndDay(&army, &rep, &rng);
            totalCasualties += (rep.deadInitiates + rep.deadSoldiers + rep.deadSergeants +
                                rep.desertedInitiates + rep.desertedSoldiers + rep.desertedSergeants);

        } else {
            /* Marcha Nocturna (con o sin cartas) */
            army.consecutiveNightMarches++;

            /* 1. Movimiento nocturno */
            float nightPA = 5.0f;
            float nightCostMult = cfg->nightMoveCostMult; /* 1.5 */
            if (policy == POLICY_NIGHT_MARCH_WITH_CARDS) {
                /* Selene: -1 al costo de movilidad nocturno (reduce mult a ~1.0) */
                nightCostMult = 1.0f;
            }
            float nightHexCost = avgDayHexCost * nightCostMult;
            totalHexes += (nightPA / nightHexCost);

            /* 2. Penalidad por fatiga nocturna */
            float penaltyMult = 1.0f + (army.consecutiveNightMarches - 1) * cfg->nightConsecutiveMult;
            float fatigueMoral = cfg->nightMoralPerTurn * penaltyMult;
            float fatigueSalud = cfg->nightSaludPerTurn * penaltyMult;

            if (policy == POLICY_NIGHT_MARCH_WITH_CARDS) {
                /* Hécate protege a las tropas en la noche: reduce fatiga a la mitad */
                fatigueMoral *= 0.5f;
                fatigueSalud *= 0.5f;
            }

            army.moral += fatigueMoral;
            if (army.moral < 0.0f) army.moral = 0.0f;
            army.salud += fatigueSalud;
            if (army.salud < 0.0f) army.salud = 0.0f;

            /* 3. Encuentros hostiles nocturnos */
            int encChance = cfg->nightEncounterChance; /* 35% */
            if (policy == POLICY_NIGHT_MARCH_WITH_CARDS) {
                /* Artemisa: visión completa y evasión reduce chance a 15% */
                encChance = 15;
            }

            if (Rng_Range(&rng, 1, 100) <= encChance) {
                totalEncounters++;
                /* Hostigamiento nocturno: daño por choque nocturno */
                army.salud -= (float)Rng_Range(&rng, 5, 10);
                army.moral -= (float)Rng_Range(&rng, 6, 12);
                int cas = Rng_Range(&rng, 1, 4);
                if (army.iniciados >= cas) army.iniciados -= cas;
                else if (army.soldados >= cas) army.soldados -= cas;
                totalCasualties += cas;
            }

            /* 4. Cierre del día */
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
    m.foodConsumed = initialFood - army.comida_disponible;
    m.waterConsumed = initialWater - army.agua_disponible;
    m.finalSalud = army.salud;
    m.finalMoral = army.moral;
    m.casualties = totalCasualties;
    m.encounters = totalEncounters;

    /* Funcion de Utilidad Estrategica:
       Score = (Progreso * 2.5) + (Salud * 0.5) + (Moral * 0.5) - (Bajas * 5.0) */
    m.netScore = (m.hexesTraveled * 2.5f) + (m.finalSalud * 0.5f) + (m.finalMoral * 0.5f) - ((float)m.casualties * 5.0f);

    return m;
}

int main(void) {
    printf("========================================================================\n");
    printf("   SIMULACION MONTE CARLO DE POLITICAS NOCTURNAS (SECCION 7.8)         \n");
    printf("========================================================================\n\n");

    const int NUM_RUNS = 1000;
    const int SIM_DAYS = 10;

    BalanceConfig cfg;
    Balance_InitDefaults(&cfg);

    SimMetrics avgCamp = {0};
    SimMetrics avgMarchNoCards = {0};
    SimMetrics avgMarchWithCards = {0};

    for (int i = 0; i < NUM_RUNS; i++) {
        uint64_t seed = 1000003ULL + (uint64_t)i * 7919ULL;

        SimMetrics mCamp = RunSingleSimulation(POLICY_ALWAYS_CAMP, seed, SIM_DAYS, &cfg);
        avgCamp.hexesTraveled += mCamp.hexesTraveled;
        avgCamp.foodConsumed  += mCamp.foodConsumed;
        avgCamp.waterConsumed += mCamp.waterConsumed;
        avgCamp.finalSalud    += mCamp.finalSalud;
        avgCamp.finalMoral    += mCamp.finalMoral;
        avgCamp.casualties    += mCamp.casualties;
        avgCamp.encounters    += mCamp.encounters;
        avgCamp.netScore      += mCamp.netScore;

        SimMetrics mMarchNo = RunSingleSimulation(POLICY_NIGHT_MARCH_NO_CARDS, seed, SIM_DAYS, &cfg);
        avgMarchNoCards.hexesTraveled += mMarchNo.hexesTraveled;
        avgMarchNoCards.foodConsumed  += mMarchNo.foodConsumed;
        avgMarchNoCards.waterConsumed += mMarchNo.waterConsumed;
        avgMarchNoCards.finalSalud    += mMarchNo.finalSalud;
        avgMarchNoCards.finalMoral    += mMarchNo.finalMoral;
        avgMarchNoCards.casualties    += mMarchNo.casualties;
        avgMarchNoCards.encounters    += mMarchNo.encounters;
        avgMarchNoCards.netScore      += mMarchNo.netScore;

        SimMetrics mMarchCards = RunSingleSimulation(POLICY_NIGHT_MARCH_WITH_CARDS, seed, SIM_DAYS, &cfg);
        avgMarchWithCards.hexesTraveled += mMarchCards.hexesTraveled;
        avgMarchWithCards.foodConsumed  += mMarchCards.foodConsumed;
        avgMarchWithCards.waterConsumed += mMarchCards.waterConsumed;
        avgMarchWithCards.finalSalud    += mMarchCards.finalSalud;
        avgMarchWithCards.finalMoral    += mMarchCards.finalMoral;
        avgMarchWithCards.casualties    += mMarchCards.casualties;
        avgMarchWithCards.encounters    += mMarchCards.encounters;
        avgMarchWithCards.netScore      += mMarchCards.netScore;
    }

    avgCamp.hexesTraveled /= NUM_RUNS;
    avgCamp.foodConsumed  /= NUM_RUNS;
    avgCamp.waterConsumed /= NUM_RUNS;
    avgCamp.finalSalud    /= NUM_RUNS;
    avgCamp.finalMoral    /= NUM_RUNS;
    avgCamp.netScore      /= NUM_RUNS;

    avgMarchNoCards.hexesTraveled /= NUM_RUNS;
    avgMarchNoCards.foodConsumed  /= NUM_RUNS;
    avgMarchNoCards.waterConsumed /= NUM_RUNS;
    avgMarchNoCards.finalSalud    /= NUM_RUNS;
    avgMarchNoCards.finalMoral    /= NUM_RUNS;
    avgMarchNoCards.netScore      /= NUM_RUNS;

    avgMarchWithCards.hexesTraveled /= NUM_RUNS;
    avgMarchWithCards.foodConsumed  /= NUM_RUNS;
    avgMarchWithCards.waterConsumed /= NUM_RUNS;
    avgMarchWithCards.finalSalud    /= NUM_RUNS;
    avgMarchWithCards.finalMoral    /= NUM_RUNS;
    avgMarchWithCards.netScore      /= NUM_RUNS;

    printf("Resultados de la simulacion (%d partidas de %d dias cada una):\n\n", NUM_RUNS, SIM_DAYS);

    printf("1. POLITICA: SIEMPRE ACAMPAR (Always Camp)\n");
    printf("   - Progreso medio:         %.1f hexágonos\n", avgCamp.hexesTraveled);
    printf("   - Salud final media:      %.1f / 100\n", avgCamp.finalSalud);
    printf("   - Moral final media:      %.1f / 100\n", avgCamp.finalMoral);
    printf("   - Bajas acumuladas:       %.2f soldados\n", (float)avgCamp.casualties / NUM_RUNS);
    printf("   - Emboscadas sufridas:    %.2f eventos\n", (float)avgCamp.encounters / NUM_RUNS);
    printf("   - Puntuacion Utilidad:    %.1f pts (Base de referencia)\n\n", avgCamp.netScore);

    printf("2. POLITICA: MARCHA NOCTURNA SIN CARTAS (Night March - No Cards)\n");
    printf("   - Progreso medio:         %.1f hexágonos (+%.1f hex extra)\n", avgMarchNoCards.hexesTraveled, avgMarchNoCards.hexesTraveled - avgCamp.hexesTraveled);
    printf("   - Salud final media:      %.1f / 100 (-%.1f salud)\n", avgMarchNoCards.finalSalud, avgCamp.finalSalud - avgMarchNoCards.finalSalud);
    printf("   - Moral final media:      %.1f / 100 (-%.1f moral)\n", avgMarchNoCards.finalMoral, avgCamp.finalMoral - avgMarchNoCards.finalMoral);
    printf("   - Bajas acumuladas:       %.2f soldados (+%.2f bajas)\n", (float)avgMarchNoCards.casualties / NUM_RUNS, (float)(avgMarchNoCards.casualties - avgCamp.casualties) / NUM_RUNS);
    printf("   - Encuentros hostiles:    %.2f eventos\n", (float)avgMarchNoCards.encounters / NUM_RUNS);
    printf("   - Puntuacion Utilidad:    %.1f pts (Delta EV: %+.1f pts)\n\n", avgMarchNoCards.netScore, avgMarchNoCards.netScore - avgCamp.netScore);

    printf("3. POLITICA: MARCHA NOCTURNA CON CARTAS NOCTURNAS (Night March - Artemisa, Selene, Hecate)\n");
    printf("   - Progreso medio:         %.1f hexágonos (+%.1f hex extra)\n", avgMarchWithCards.hexesTraveled, avgMarchWithCards.hexesTraveled - avgCamp.hexesTraveled);
    printf("   - Salud final media:      %.1f / 100 (-%.1f salud)\n", avgMarchWithCards.finalSalud, avgCamp.finalSalud - avgMarchWithCards.finalSalud);
    printf("   - Moral final media:      %.1f / 100 (-%.1f moral)\n", avgMarchWithCards.finalMoral, avgCamp.finalMoral - avgMarchWithCards.finalMoral);
    printf("   - Bajas acumuladas:       %.2f soldados (+%.2f bajas)\n", (float)avgMarchWithCards.casualties / NUM_RUNS, (float)(avgMarchWithCards.casualties - avgCamp.casualties) / NUM_RUNS);
    printf("   - Encuentros hostiles:    %.2f eventos\n", (float)avgMarchWithCards.encounters / NUM_RUNS);
    printf("   - Puntuacion Utilidad:    %.1f pts (Delta EV: %+.1f pts)\n\n", avgMarchWithCards.netScore, avgMarchWithCards.netScore - avgCamp.netScore);

    float deltaNoCards = avgMarchNoCards.netScore - avgCamp.netScore;
    float deltaWithCards = avgMarchWithCards.netScore - avgCamp.netScore;

    printf("------------------------------------------------------------------------\n");
    printf("CONCLUSION DEL ANALISIS DE EQUILIBRIO (SECCION 7.8):\n");
    if (deltaNoCards < 0.0f) {
        printf("[OK] Marchar de noche sin cartas tiene EV levemente negativo (%+.1f pts).\n", deltaNoCards);
        printf("     El desgaste acumulado y las bajas superan la ganancia en distancia.\n");
    } else {
        printf("[!] Marchar de noche sin cartas dio EV positivo (%+.1f pts).\n", deltaNoCards);
    }

    if (deltaWithCards > 0.0f) {
        printf("[OK] Marchar de noche CON cartas divinas nocturnas tiene EV POSITIVO (%+.1f pts).\n", deltaWithCards);
        printf("     Las cartas mitigan la fatiga y el coste, haciendo viable y rentable la marcha.\n");
    } else {
        printf("[!] Marchar de noche con cartas dio EV negativo (%+.1f pts).\n", deltaWithCards);
    }
    printf("------------------------------------------------------------------------\n");

    return 0;
}
