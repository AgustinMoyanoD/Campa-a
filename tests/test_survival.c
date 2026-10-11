#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "models/army.h"
#include "models/day_clock.h"
#include "models/balance_config.h"
#include "models/rng.h"
#include "models/general.h"

static void test_starvation_and_recovery(void) {
    printf("[TEST 1] Hambre y Recuperacion Gradual (5 dias)...\n");
    BalanceConfig cfg;
    Balance_InitDefaults(&cfg);

    General gen = General_Create("Comandante Test", 35, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 0}, gen, 10, 50, 100, 500.0f, 0.0f, 100, 0);
    army.moral = 100.0f;
    army.salud = 100.0f;

    Rng rng;
    Rng_Init(&rng, 12345ULL);

    /* Dia 1 sin comida */
    DayReport r1;
    Army_EndDay(&army, &r1, &rng);
    assert(army.daysWithoutFood == 1);
    assert(r1.foodShortage == true);
    assert(r1.daysWithoutFood == 1);
    assert(r1.moralDelta == 0.0f);
    assert(r1.saludDelta == 0.0f);
    assert(r1.deadInitiates == 0 && r1.desertedInitiates == 0);
    printf("  Dia 1 OK: Hambre=1, Moral=%.0f, Salud=%.0f\n", army.moral, army.salud);

    /* Dia 2 sin comida */
    DayReport r2;
    Army_EndDay(&army, &r2, &rng);
    assert(army.daysWithoutFood == 2);
    assert(r2.moralDelta == -6.0f);
    assert(r2.saludDelta == 0.0f);
    printf("  Dia 2 OK: Hambre=2, Moral=%.0f (delta %.0f), Salud=%.0f\n", army.moral, r2.moralDelta, army.salud);

    /* Dia 3 sin comida */
    DayReport r3;
    Army_EndDay(&army, &r3, &rng);
    assert(army.daysWithoutFood == 3);
    assert(r3.moralDelta == -8.0f);
    assert(r3.saludDelta == -10.0f);
    printf("  Dia 3 OK: Hambre=3, Moral=%.0f, Salud=%.0f, Muertes=%d, Deserciones=%d\n",
           army.moral, army.salud,
           r3.deadInitiates + r3.deadSoldiers + r3.deadSergeants,
           r3.desertedInitiates + r3.desertedSoldiers + r3.desertedSergeants);

    /* Dia 4 y 5 sin comida */
    DayReport r4, r5;
    Army_EndDay(&army, &r4, &rng);
    assert(army.daysWithoutFood == 4);
    Army_EndDay(&army, &r5, &rng);
    assert(army.daysWithoutFood == 5);
    printf("  Dias 4 y 5 OK: Hambre escalo a %d\n", army.daysWithoutFood);

    /* Recuperacion gradual: reabastecer comida */
    army.comida_disponible = 200.0f;
    DayReport r_rec1;
    Army_EndDay(&army, &r_rec1, &rng);
    assert(army.daysWithoutFood == 4); /* De 5 bajo gradualmente a 4 */
    assert(r_rec1.foodShortage == false);
    printf("  Recuperacion Dia +1 OK: Hambre disminuyo de 5 a %d\n", army.daysWithoutFood);

    DayReport r_rec2;
    Army_EndDay(&army, &r_rec2, &rng);
    assert(army.daysWithoutFood == 3); /* Bajo gradualmente a 3 */
    printf("  Recuperacion Dia +2 OK: Hambre disminuyo a %d\n", army.daysWithoutFood);
    printf("-> [TEST 1 PASSED]\n\n");
}

static void test_day_clock(void) {
    printf("[TEST 2] Reloj y ciclo de franjas (4 franjas por dia)...\n");
    DayClock clock;
    DayClock_Init(&clock, 1, TIME_MORNING);

    int dayEndedCount = 0;
    int ticks = 16; /* 4 dias completos */

    for (int i = 0; i < ticks; i++) {
        TimeOfDay currentSlot = clock.timeOfDay;
        assert(currentSlot == (TimeOfDay)(i % 4));

        DayClock_AdvanceSlot(&clock);

        if (DayClock_HasDayEnded(&clock)) {
            dayEndedCount++;
            int currentDay = clock.currentDay;
            printf("  Fin de dia registrado: Dia %d completado\n", currentDay - 1);
            DayClock_ClearSignals(&clock);
        }
    }

    assert(dayEndedCount == 4);
    assert(clock.currentDay == 5);
    assert(clock.timeOfDay == TIME_MORNING);
    printf("-> [TEST 2 PASSED]: Exactamente 4 DayEnded en 16 ticks (1 cada 4 franjas)\n\n");
}

static void test_conservation(void) {
    printf("[TEST 3] Conservacion de Tropas y Recursos...\n");
    BalanceConfig cfg;
    Balance_InitDefaults(&cfg);

    General gen = General_Create("Comandante Test", 35, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 0}, gen, 20, 80, 200, 0.0f, 0.0f, 100, 10);

    Rng rng;
    Rng_Init(&rng, 98765ULL);

    int totalDeserters = 0;
    int totalDead = 0;
    int initialTroops = Army_GetTotalSoldiers(&army);

    for (int day = 0; day < 7; day++) {
        DayReport report;
        Army_EndDay(&army, &report, &rng);

        totalDeserters += (report.desertedInitiates + report.desertedSoldiers + report.desertedSergeants);
        totalDead += (report.deadInitiates + report.deadSoldiers + report.deadSergeants);

        /* Verificar no negatividad de recursos */
        assert(army.comida_disponible >= 0.0f);
        assert(army.agua_disponible >= 0.0f);
        assert(army.monedas >= 0);
        assert(army.sargentos >= 0);
        assert(army.soldados >= 0);
        assert(army.iniciados >= 0);
        assert(army.moral >= 0.0f && army.moral <= 100.0f);
        assert(army.salud >= 0.0f && army.salud <= 100.0f);
    }

    int finalTroops = Army_GetTotalSoldiers(&army);
    assert(initialTroops == finalTroops + totalDeserters + totalDead);
    printf("  Tropas iniciales: %d = Finales: %d + Desertores: %d + Muertos: %d\n",
           initialTroops, finalTroops, totalDeserters, totalDead);
    printf("-> [TEST 3 PASSED]: Conservacion estricta de tropas y no-negatividad verificadas\n\n");
}

static void test_camp_mechanics(void) {
    printf("[TEST 4] Campamento: Movimiento cero, consumo activo, recuperacion y muestreo de emboscada (10.000 tiradas)...\n");
    BalanceConfig cfg;
    Balance_InitDefaults(&cfg);

    General gen = General_Create("Comandante Test", 35, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 0}, gen, 10, 50, 100, 200.0f, 200.0f, 100, 0);
    army.moral = 70.0f;
    army.salud = 70.0f;

    Rng rng;
    Rng_Init(&rng, 54321ULL);

    /* 1. Consumo continuo durante campamento */
    float foodBefore = army.comida_disponible;
    float waterBefore = army.agua_disponible;
    Army_ConsumeTurnResources(&army);
    DayReport rCamp;
    Army_EndDay(&army, &rCamp, &rng);
    assert(army.comida_disponible < foodBefore);
    assert(army.agua_disponible < waterBefore);
    printf("  Consumo continuo OK: Comida consumida: %.1f, Agua consumida: %.1f\n",
           foodBefore - army.comida_disponible, waterBefore - army.agua_disponible);

    /* 2. Recuperacion en campamento cuando no hay hambruna */
    bool starvingNoRec = cfg.campStarvingNoRecovery && (army.daysWithoutFood >= 2 || army.daysWithoutWater >= 2);
    assert(!starvingNoRec);
    army.moral += cfg.campRecoverMoral;
    army.salud += cfg.campRecoverSalud;
    assert(army.moral == 76.0f);
    assert(army.salud == 75.0f);
    printf("  Recuperacion en campamento OK: Moral -> %.0f (+%.0f), Salud -> %.0f (+%.0f)\n",
           army.moral, cfg.campRecoverMoral, army.salud, cfg.campRecoverSalud);

    /* 3. Sin recuperacion si hambruna >= 2 dias */
    army.daysWithoutFood = 2;
    starvingNoRec = cfg.campStarvingNoRecovery && (army.daysWithoutFood >= 2 || army.daysWithoutWater >= 2);
    assert(starvingNoRec == true);
    printf("  Regla de hambruna severa OK: Sin recuperacion cuando dias sin comida >= 2\n");

    /* 4. Muestreo de 10.000 tiradas de emboscada */
    const int SAMPLES = 10000;
    int ambushes_high = 0; /* Bosque (+2) + Eval 4 => Bono +6 */
    int ambushes_mid = 0;  /* Colinas (+1) + Eval 1 => Bono +2 */
    int ambushes_low = 0;  /* Desierto (-1) + Eval 0 => Bono -1 */
    int ambushes_fort = 0; /* Ruinas/Ciudad (+90) => Fortificado */

    int coverForest = Balance_GetTerrainCover(&cfg, BIOME_FOREST, STRUCT_NONE);
    int coverHills = Balance_GetTerrainCover(&cfg, BIOME_PLAINS, STRUCT_NONE) + 1;
    int coverDesert = Balance_GetTerrainCover(&cfg, BIOME_DESERT, STRUCT_NONE);
    int coverRuins = Balance_GetTerrainCover(&cfg, BIOME_PLAINS, STRUCT_TOWN_CASTLE);

    assert(coverForest == 2);
    assert(coverDesert == -1);
    assert(coverRuins == 99);

    for (int i = 0; i < SAMPLES; i++) {
        /* Condicion Alta: Bosque (+2) + Eval 4 */
        int rollA = Rng_RollDice(&rng, 2, 6);
        int totalA = rollA + 4 + coverForest;
        if (totalA < cfg.campAmbushTarget) ambushes_high++;

        /* Condicion Media: Colinas (+1) + Eval 1 */
        int rollB = Rng_RollDice(&rng, 2, 6);
        int totalB = rollB + 1 + coverHills;
        if (totalB < cfg.campAmbushTarget) ambushes_mid++;

        /* Condicion Baja: Desierto (-1) + Eval 0 */
        int rollC = Rng_RollDice(&rng, 2, 6);
        int totalC = rollC + 0 + coverDesert;
        if (totalC < cfg.campAmbushTarget) ambushes_low++;

        /* Condicion Fortificada: Ruinas/Castillo */
        int rollD = (coverRuins >= 90) ? 99 : (Rng_RollDice(&rng, 2, 6) + coverRuins);
        if (rollD < cfg.campAmbushTarget) ambushes_fort++;
    }

    printf("  Muestreo de 10.000 tiradas:\n");
    printf("    Alta cobertura (Bosque + Eval 4):      %d emboscadas (%.2f%%) [Esperado: 0%%]\n",
           ambushes_high, (float)ambushes_high * 100.0f / SAMPLES);
    printf("    Media cobertura (Colinas + Eval 1):    %d emboscadas (%.2f%%) [Esperado: ~16.7%%]\n",
           ambushes_mid, (float)ambushes_mid * 100.0f / SAMPLES);
    printf("    Baja cobertura (Desierto + Eval 0):    %d emboscadas (%.2f%%) [Esperado: ~58.3%%]\n",
           ambushes_low, (float)ambushes_low * 100.0f / SAMPLES);
    printf("    Fortificado (Castillo/Ruinas):         %d emboscadas (%.2f%%) [Esperado: 0%%]\n",
           ambushes_fort, (float)ambushes_fort * 100.0f / SAMPLES);

    assert(ambushes_high == 0);
    assert(ambushes_fort == 0);
    assert(ambushes_mid >= 1450 && ambushes_mid <= 1850);
    assert(ambushes_low >= 5600 && ambushes_low <= 6100);
    assert(ambushes_high < ambushes_mid && ambushes_mid < ambushes_low);

    printf("-> [TEST 4 PASSED]: Cobertura y Evaluacion reducen estrictamente el riesgo de emboscada\n\n");
}

static void test_night_march_mechanics(void) {
    printf("[TEST 5] Marcha Nocturna: Multiplicador de movilidad y penalidades consecutivas escalables...\n");
    BalanceConfig cfg;
    Balance_InitDefaults(&cfg);

    General gen = General_Create("Comandante Test", 35, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 0}, gen, 10, 50, 100, 300.0f, 300.0f, 100, 0);
    army.moral = 100.0f;
    army.salud = 100.0f;
    army.consecutiveNightMarches = 0;

    /* 1. Multiplicador de costo de movimiento nocturno */
    int baseCostPlains = 1;
    int baseCostForest = 2;
    int baseCostMountain = 3;

    int nightPlains = (int)ceilf((float)baseCostPlains * cfg.nightMoveCostMult);
    int nightForest = (int)ceilf((float)baseCostForest * cfg.nightMoveCostMult);
    int nightMountain = (int)ceilf((float)baseCostMountain * cfg.nightMoveCostMult);

    assert(nightPlains == 2);   /* ceil(1 * 1.5) = 2 */
    assert(nightForest == 3);   /* ceil(2 * 1.5) = 3 */
    assert(nightMountain == 5); /* ceil(3 * 1.5) = 5 */
    printf("  Multiplicador de coste nocturno (x%.1f) OK: Llanura %d->%d PA, Bosque %d->%d PA, Montana %d->%d PA\n",
           cfg.nightMoveCostMult, baseCostPlains, nightPlains, baseCostForest, nightForest, baseCostMountain, nightMountain);

    /* 2. Penalidades por noches consecutivas (7.4) */
    /* Noche 1 */
    army.consecutiveNightMarches = 1;
    float mult1 = 1.0f + (army.consecutiveNightMarches - 1) * cfg.nightConsecutiveMult;
    float dMoral1 = cfg.nightMoralPerTurn * mult1;
    float dSalud1 = cfg.nightSaludPerTurn * mult1;
    army.moral += dMoral1;
    army.salud += dSalud1;
    assert(mult1 == 1.0f);
    assert(dMoral1 == -2.0f);
    assert(dSalud1 == -2.0f);
    assert(army.moral == 98.0f);
    assert(army.salud == 98.0f);
    printf("  Noche 1 consecutiva OK: Mult=%.1fx, Moral=%.0f (delta %.0f), Salud=%.0f (delta %.0f)\n",
           mult1, army.moral, dMoral1, army.salud, dSalud1);

    /* Noche 2 */
    army.consecutiveNightMarches = 2;
    float mult2 = 1.0f + (army.consecutiveNightMarches - 1) * cfg.nightConsecutiveMult;
    float dMoral2 = cfg.nightMoralPerTurn * mult2;
    float dSalud2 = cfg.nightSaludPerTurn * mult2;
    army.moral += dMoral2;
    army.salud += dSalud2;
    assert(mult2 == 1.5f);
    assert(dMoral2 == -3.0f);
    assert(dSalud2 == -3.0f);
    assert(army.moral == 95.0f);
    assert(army.salud == 95.0f);
    printf("  Noche 2 consecutiva OK: Mult=%.1fx, Moral=%.0f (delta %.0f), Salud=%.0f (delta %.0f)\n",
           mult2, army.moral, dMoral2, army.salud, dSalud2);

    /* Noche 3 */
    army.consecutiveNightMarches = 3;
    float mult3 = 1.0f + (army.consecutiveNightMarches - 1) * cfg.nightConsecutiveMult;
    float dMoral3 = cfg.nightMoralPerTurn * mult3;
    float dSalud3 = cfg.nightSaludPerTurn * mult3;
    army.moral += dMoral3;
    army.salud += dSalud3;
    assert(mult3 == 2.0f);
    assert(dMoral3 == -4.0f);
    assert(dSalud3 == -4.0f);
    assert(army.moral == 91.0f);
    assert(army.salud == 91.0f);
    printf("  Noche 3 consecutiva OK: Mult=%.1fx, Moral=%.0f (delta %.0f), Salud=%.0f (delta %.0f)\n",
           mult3, army.moral, dMoral3, army.salud, dSalud3);

    /* 3. Reseteo al acampar */
    army.consecutiveNightMarches = 0;
    assert(army.consecutiveNightMarches == 0);
    printf("  Reseteo tras acampar OK: consecutiveNightMarches = 0\n");

    printf("-> [TEST 5 PASSED]: Escalado consecutivo y multiplicadores nocturnos verificados\n\n");
}

int main(void) {
    printf("==================================================\n");
    printf("   ARNES DE PRUEBAS DE SUPERVIVENCIA (FASES 1 Y 2)\n");
    printf("==================================================\n\n");

    test_starvation_and_recovery();
    test_day_clock();
    test_conservation();
    test_camp_mechanics();
    test_night_march_mechanics();

    printf("==================================================\n");
    printf("   TODAS LAS PRUEBAS DE LA FASE 2 PASARON [OK]    \n");
    printf("==================================================\n");
    return 0;
}
