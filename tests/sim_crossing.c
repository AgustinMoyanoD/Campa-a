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
#include "models/map_generator.h"
#include "models/combat.h"
#include "models/enemy.h"
#include "models/city.h"
#include "controllers/game_master.h"

typedef enum {
    RESULT_VICTORY = 0,
    RESULT_DEATH_HUNGER,
    RESULT_DEATH_COMBAT,
    RESULT_DEATH_MUTINY,
    RESULT_MAP_DEFECT
} RunResult;

typedef struct {
    RunResult result;
    int   daysUsed;
    float finalSalud;
    float finalMoral;
    float finalSupplyDays;
    int   finalTroops;
    int   settlementsVisited;
    int   goldSpent;
    int   combatsFought;
} CrossingRunMetrics;

typedef struct {
    int totalRuns;
    int victories;
    int deathsHunger;
    int deathsCombat;
    int deathsMutiny;
    int mapDefects;

    float avgDaysUsed;
    float avgFinalSalud;
    float avgFinalMoral;
    float avgFinalSupplyDays;
    float avgFinalTroops;
    float avgSettlementsVisited;
    float avgGoldSpent;
} BatchSummary;

static CombatPlan PickBestCombatPlan(const CombatContext* ctx) {
    CombatPlan candidatePlans[] = { PLAN_FLANK, PLAN_PIKES, PLAN_ASSAULT, PLAN_SKIRMISH };
    CombatPlan bestPlan = PLAN_SKIRMISH;
    int bestScore = -999;

    for (int i = 0; i < 4; i++) {
        CombatPlan p = candidatePlans[i];
        CombatPlanInfo info = Combat_GetPlanInfo(ctx, p);
        if (!info.isUnlocked) continue;

        int score = 10;
        if (info.isFatigued) score -= 15;
        if (info.hasAdvantage) score += 10;
        if (info.hasDisadvantage) score -= 10;

        if (p == PLAN_FLANK) score += 5;
        else if (p == PLAN_PIKES) score += 4;
        else if (p == PLAN_ASSAULT) score += 2;

        if (score > bestScore) {
            bestScore = score;
            bestPlan = p;
        }
    }
    return bestPlan;
}

static CrossingRunMetrics SimulateSingleCrossing(uint64_t seed, float initialSupplyDays, const BalanceConfig* cfg) {
    CrossingRunMetrics metrics = {0};
    Rng rng;
    Rng_Init(&rng, seed);

    /* 1. Generar nivel con el generador procedural real */
    int mapRadius = 15;
    int landProb = 75;
    Map map = Map_GenerateProcedural(mapRadius, landProb, 3, 1);

    HexTile* startTile = NULL;
    for (int i = 0; i < map.tileCount; i++) {
        if (map.tiles[i].special == TILE_SPECIAL_START) {
            startTile = &map.tiles[i];
            break;
        }
    }

    /* B es la salida del cruce provincial (la salida principal opuesta a A) */
    HexTile* endTile = NULL;
    int maxDist = -1;
    if (startTile) {
        for (int i = 0; i < map.tileCount; i++) {
            if (map.tiles[i].special == TILE_SPECIAL_END) {
                int d = HexCoord_Distance(startTile->coord, map.tiles[i].coord);
                if (d > maxDist) {
                    maxDist = d;
                    endTile = &map.tiles[i];
                }
            }
        }
    }

    if (!startTile || !endTile) {
        Map_Free(&map);
        metrics.result = RESULT_MAP_DEFECT;
        return metrics;
    }

    /* Calcular ruta global de A a B */
    int totalPathCost = 0;
    HexTile* globalPath[256];
    int pathLen = Map_FindPathEx(&map, startTile, endTile, true, &totalPathCost, globalPath);
    if (pathLen <= 1) {
        Map_Free(&map);
        metrics.result = RESULT_MAP_DEFECT;
        return metrics;
    }

    /* 2. Inicializar ejército */
    General gen = General_Create("Piloto Auto", 35, 2, 2, 2, 2, 2);
    int iniSoldiers = 63;
    int iniCattle = 8;
    float dailyFood = Army_CalculateFoodUsage(iniSoldiers, iniCattle);
    float dailyWater = Army_CalculateWaterUsage(iniSoldiers, iniCattle);

    float startFood = initialSupplyDays * dailyFood;
    float startWater = initialSupplyDays * dailyWater;
    Army army = Army_Create(startTile->coord, gen, 8, 40, 15, startFood, startWater, 250, iniCattle);
    army.moral = 90.0f;
    army.salud = 95.0f;

    /* Inicializar GameMaster y enemigos */
    GameMaster gm;
    GameMaster_Init(&gm);
    GameMaster_SpawnTestEnemies(&gm, &map, army.position);

    /* Perseguidor (Hunter): activo desde el día 3 */
    bool hunterActive = false;
    HexCoord hunterPos = startTile->coord;

    int currentPathIdx = 0;
    const int MAX_DAYS = 30;
    int day = 0;

    while (day < MAX_DAYS) {
        day++;
        gm.currentDay = day;

        /* Activación del Cazador en día 3 */
        if (day == 3 && cfg->hunterEnabled) {
            hunterActive = true;
            hunterPos = startTile->coord;
        }

        /* 3 Franjas diurnas: Mañana, Mediodía, Tarde */
        for (int slot = 0; slot < 3; slot++) {
            gm.timeOfDay = (TimeOfDay)slot;
            Army_ConsumeTurnResources(&army);

            /* A. Chequeo de supervivencia */
            if (Army_GetTotalSoldiers(&army) <= 0 || army.salud <= 0.0f) {
                metrics.result = (army.daysWithoutFood >= 2 || army.daysWithoutWater >= 2) ? RESULT_DEATH_HUNGER : RESULT_DEATH_COMBAT;
                metrics.daysUsed = day;
                Map_Free(&map);
                return metrics;
            }

            /* B. Pronóstico de suministros */
            SupplyForecast forecast = Army_ForecastSupply(&army, &map, army.position);

            /* C. Regla del piloto: Desviarse al asentamiento si campDays < 2.0 y tiene oro */
            if (forecast.campDays < 2.0f && army.monedas >= 20) {
                HexTile* nearestSettlement = NULL;
                int minDist = 999;
                for (int t = 0; t < map.tileCount; t++) {
                    if (map.tiles[t].structure.settlementId > 0 ||
                        map.tiles[t].structure.type == STRUCT_TOWN_CASTLE ||
                        map.tiles[t].structure.type == STRUCT_TOWN_MARKET ||
                        map.tiles[t].structure.type == STRUCT_TOWN_ARMORY ||
                        map.tiles[t].structure.type == STRUCT_TOWN_HOUSES) {
                        int dist = HexCoord_Distance(army.position, map.tiles[t].coord);
                        if (dist < minDist && dist <= 5) {
                            minDist = dist;
                            nearestSettlement = &map.tiles[t];
                        }
                    }
                }

                if (nearestSettlement && minDist <= 5) {
                    metrics.settlementsVisited++;
                    int curSoldiers = Army_GetTotalSoldiers(&army);
                    float curDailyFood = Army_CalculateFoodUsage(curSoldiers, army.ganado);
                    float curDailyWater = Army_CalculateWaterUsage(curSoldiers, army.ganado);
                    float buyTargetDays = 2.5f;
                    float toBuyDays = buyTargetDays - forecast.campDays;
                    if (toBuyDays < 0.75f) toBuyDays = 0.75f;
                    if (toBuyDays > 2.0f) toBuyDays = 2.0f;
                    int neededFood = (int)(toBuyDays * curDailyFood);
                    int neededWater = (int)(toBuyDays * curDailyWater);
                    int foodCost = neededFood * cfg->marketPriceFood;
                    int waterCost = neededWater * cfg->marketPriceWater;
                    int totalCost = foodCost + waterCost;

                    if (army.monedas >= totalCost) {
                        army.monedas -= totalCost;
                        army.comida_disponible += (float)neededFood;
                        army.agua_disponible += (float)neededWater;
                        metrics.goldSpent += totalCost;
                    } else if (army.monedas >= 20) {
                        int portion = army.monedas / 2;
                        army.comida_disponible += (float)(portion / cfg->marketPriceFood);
                        army.agua_disponible += (float)(portion / cfg->marketPriceWater);
                        metrics.goldSpent += army.monedas;
                        army.monedas = 0;
                    }
                }
            }

            /* D. Avance a lo largo de la ruta hacia B (ritmo calibrado: ~3 hexes por franja) */
            int paDice = Rng_RollDice(&rng, 1, 6);
            int availablePA = 3 + paDice; /* 4..9 PA por franja */
            int stepsInSlot = 0;

            while (availablePA > 0 && currentPathIdx < pathLen && stepsInSlot < 3) {
                HexTile* nextTile = globalPath[currentPathIdx];
                BiomeProperties props = Biome_GetProperties(nextTile->biome);
                int stepCost = props.actionPointCost;
                if (nextTile->isRoad && stepCost > 1) stepCost -= 1;

                if (availablePA < stepCost) break;

                availablePA -= stepCost;
                stepsInSlot++;
                currentPathIdx++;
                army.position = nextTile->coord;

                /* Consumo logístico de marcha activa (GameMaster_AttemptMove) */
                army.comida_disponible -= 3.0f * props.foodConsumption;
                army.agua_disponible -= 3.5f * props.waterConsumption;
                if (army.comida_disponible < 0.0f) army.comida_disponible = 0.0f;
                if (army.agua_disponible < 0.0f) army.agua_disponible = 0.0f;

                /* Chequeo de llegada a B */
                if (army.position.q == endTile->coord.q && army.position.r == endTile->coord.r) {
                    SupplyForecast finalFc = Army_ForecastSupply(&army, &map, army.position);
                    metrics.result = RESULT_VICTORY;
                    metrics.daysUsed = (float)(day - 1) + ((float)slot + 1.0f) / 4.0f;
                    metrics.finalSalud = army.salud;
                    metrics.finalMoral = army.moral;
                    metrics.finalSupplyDays = finalFc.campDays;
                    metrics.finalTroops = Army_GetTotalSoldiers(&army);
                    Map_Free(&map);
                    return metrics;
                }

                /* Chequeo de combate con escuadrones enemigos */
                for (int e = 0; e < gm.enemyCount; e++) {
                    if (gm.enemies[e].isActive &&
                        gm.enemies[e].position.q == army.position.q &&
                        gm.enemies[e].position.r == army.position.r) {
                        
                        metrics.combatsFought++;
                        CombatContext cCtx;
                        Combat_Init(&cCtx, &gm, &army, &gm.enemies[e]);
                        Combat_RollEvaluation(&cCtx, Rng_RollDice(&rng, 1, 6), Rng_RollDice(&rng, 1, 6));

                        while (cCtx.state == COMBAT_STATE_PLANNING && cCtx.round <= cfg->maxCombatRounds) {
                            Combat_RollPlanning(&cCtx, Rng_RollDice(&rng, 1, 6), Rng_RollDice(&rng, 1, 6));
                            CombatPlan bestPlan = PickBestCombatPlan(&cCtx);
                            Combat_SelectPlan(&cCtx, bestPlan);
                            Combat_RollExecution(&cCtx, Rng_RollDice(&rng, 1, 6), Rng_RollDice(&rng, 1, 6));
                            Combat_RoundClose(&cCtx);
                        }

                        if (cCtx.state == COMBAT_STATE_END) {
                            Combat_Recount(&cCtx);
                            Combat_Commit(&cCtx);
                        }

                        if (Army_GetTotalSoldiers(&army) <= 0 || army.salud <= 0.0f) {
                            metrics.result = RESULT_DEATH_COMBAT;
                            metrics.daysUsed = day;
                            Map_Free(&map);
                            return metrics;
                        }
                        break;
                    }
                }
            }

            /* E. Turno de la IA enemiga */
            GameMaster_ProcessEnemyTurn(&gm, &map, &army);

            /* F. Avance del Cazador */
            if (hunterActive) {
                int distH = HexCoord_Distance(hunterPos, army.position);
                if (distH > 1) {
                    /* Avanza 1-2 hexágonos hacia el ejército */
                    for (int d = 0; d < 6; d++) {
                        HexCoord nCoord = HexCoord_GetNeighbor(hunterPos, (HexDirection)d);
                        if (HexCoord_Distance(nCoord, army.position) < distH) {
                            hunterPos = nCoord;
                            break;
                        }
                    }
                } else if (distH <= 1) {
                    /* El cazador alcanza al ejército: hostigamiento táctico */
                    metrics.combatsFought++;
                    army.salud -= (float)Rng_Range(&rng, 5, 9);
                    army.moral -= (float)Rng_Range(&rng, 6, 10);
                    int cas = Rng_Range(&rng, 1, 2);
                    if (army.iniciados >= cas) army.iniciados -= cas;
                    else if (army.soldados >= cas) army.soldados -= cas;
                    hunterPos = startTile->coord; /* Repelido hacia retaguardia */
                }
            }
        }

        /* G. Atardecer: Decisión de Campamento o Marcha Nocturna */
        gm.timeOfDay = TIME_NIGHT;
        SupplyForecast duskForecast = Army_ForecastSupply(&army, &map, army.position);
        bool marchAtNight = false;
        if (army.salud >= 75.0f && duskForecast.campDays >= 2.5f && army.consecutiveNightMarches < 2) {
            marchAtNight = true;
        }

        if (marchAtNight) {
            army.consecutiveNightMarches++;
            float penMult = 1.0f + (army.consecutiveNightMarches - 1) * cfg->nightConsecutiveMult;
            army.moral += cfg->nightMoralPerTurn * penMult;
            army.salud += cfg->nightSaludPerTurn * penMult;

            int paNight = 2 + Rng_RollDice(&rng, 1, 4);
            int stepsNight = 0;
            while (paNight > 0 && currentPathIdx < pathLen && stepsNight < 2) {
                HexTile* nextTile = globalPath[currentPathIdx];
                BiomeProperties props = Biome_GetProperties(nextTile->biome);
                int stepCost = (int)ceilf((float)props.actionPointCost * cfg->nightMoveCostMult);
                if (nextTile->isRoad && stepCost > 1) stepCost -= 1;

                if (paNight < stepCost) break;
                paNight -= stepCost;
                stepsNight++;
                currentPathIdx++;
                army.position = nextTile->coord;

                army.comida_disponible -= 3.0f * props.foodConsumption;
                army.agua_disponible -= 3.5f * props.waterConsumption;
                if (army.comida_disponible < 0.0f) army.comida_disponible = 0.0f;
                if (army.agua_disponible < 0.0f) army.agua_disponible = 0.0f;

                if (army.position.q == endTile->coord.q && army.position.r == endTile->coord.r) {
                    SupplyForecast finalFc = Army_ForecastSupply(&army, &map, army.position);
                    metrics.result = RESULT_VICTORY;
                    metrics.daysUsed = (float)day;
                    metrics.finalSalud = army.salud;
                    metrics.finalMoral = army.moral;
                    metrics.finalSupplyDays = finalFc.campDays;
                    metrics.finalTroops = Army_GetTotalSoldiers(&army);
                    Map_Free(&map);
                    return metrics;
                }
            }

            if (Rng_Range(&rng, 1, 100) <= cfg->nightEncounterChance) {
                army.salud -= (float)Rng_Range(&rng, 3, 6);
                army.moral -= (float)Rng_Range(&rng, 4, 7);
                int cas = Rng_Range(&rng, 1, 2);
                if (army.iniciados >= cas) army.iniciados -= cas;
                else if (army.soldados >= cas) army.soldados -= cas;
            }

            Army_ConsumeTurnResources(&army);
            DayReport rep;
            Army_EndDay(&army, &rep, &rng);
        } else {
            /* Acampar */
            army.consecutiveNightMarches = 0;
            bool starving = (army.daysWithoutFood >= 2 || army.daysWithoutWater >= 2);
            if (!starving) {
                army.moral += cfg->campRecoverMoral;
                if (army.moral > 100.0f) army.moral = 100.0f;
                army.salud += cfg->campRecoverSalud;
                if (army.salud > 100.0f) army.salud = 100.0f;
            }

            int ambushRoll = Rng_RollDice(&rng, 2, 6) + 2;
            if (ambushRoll < cfg->campAmbushTarget) {
                army.salud -= (float)Rng_Range(&rng, 3, 6);
                army.moral -= (float)Rng_Range(&rng, 3, 6);
                int cas = Rng_Range(&rng, 1, 2);
                if (army.iniciados >= cas) army.iniciados -= cas;
                else if (army.soldados >= cas) army.soldados -= cas;
            }

            Army_ConsumeTurnResources(&army);
            DayReport rep;
            Army_EndDay(&army, &rep, &rng);
        }

        /* H. Chequeos de fin de día */
        if (Army_GetTotalSoldiers(&army) <= 0 || army.salud <= 0.0f) {
            metrics.result = (army.daysWithoutFood >= 2 || army.daysWithoutWater >= 2) ? RESULT_DEATH_HUNGER : RESULT_DEATH_COMBAT;
            metrics.daysUsed = day;
            Map_Free(&map);
            return metrics;
        }

        if (army.moral <= 0.0f) {
            metrics.result = RESULT_DEATH_MUTINY;
            metrics.daysUsed = day;
            Map_Free(&map);
            return metrics;
        }
    }

    metrics.result = RESULT_DEATH_HUNGER;
    metrics.daysUsed = MAX_DAYS;
    Map_Free(&map);
    return metrics;
}

static BatchSummary RunBatchSimulation(int numRuns, float initialSupplyDays, const BalanceConfig* cfg) {
    BatchSummary summary = {0};
    summary.totalRuns = numRuns;

    for (int i = 0; i < numRuns; i++) {
        uint64_t seed = 5000001ULL + (uint64_t)i * 104729ULL;
        CrossingRunMetrics m = SimulateSingleCrossing(seed, initialSupplyDays, cfg);

        if (m.result == RESULT_VICTORY) {
            summary.victories++;
            summary.avgDaysUsed          += (float)m.daysUsed;
            summary.avgFinalSalud        += m.finalSalud;
            summary.avgFinalMoral        += m.finalMoral;
            summary.avgFinalSupplyDays   += m.finalSupplyDays;
            summary.avgFinalTroops       += (float)m.finalTroops;
            summary.avgSettlementsVisited+= (float)m.settlementsVisited;
            summary.avgGoldSpent         += (float)m.goldSpent;
        } else if (m.result == RESULT_DEATH_HUNGER) {
            summary.deathsHunger++;
        } else if (m.result == RESULT_DEATH_COMBAT) {
            summary.deathsCombat++;
        } else if (m.result == RESULT_DEATH_MUTINY) {
            summary.deathsMutiny++;
        } else {
            summary.mapDefects++;
        }
    }

    if (summary.victories > 0) {
        summary.avgDaysUsed           /= (float)summary.victories;
        summary.avgFinalSalud         /= (float)summary.victories;
        summary.avgFinalMoral         /= (float)summary.victories;
        summary.avgFinalSupplyDays    /= (float)summary.victories;
        summary.avgFinalTroops        /= (float)summary.victories;
        summary.avgSettlementsVisited /= (float)summary.victories;
        summary.avgGoldSpent          /= (float)summary.victories;
    }

    return summary;
}

int main(void) {
    printf("========================================================================\n");
    printf("   SIMULADOR DE CRUCE DE NIVEL (BLOQUE C2 & C3) - N=1.000 NIVELES       \n");
    printf("========================================================================\n\n");

    const int NUM_RUNS = 1000;
    BalanceConfig cfg;
    Balance_InitDefaults(&cfg);
    Balance_LoadFromFile(&cfg, "assets/balance.cfg");

    float supplyOptions[] = { 3.0f, 4.0f, 5.0f, 6.0f };
    BatchSummary summaries[4];

    printf("Ejecutando simulaciones comparativas de raciones iniciales (3, 4, 5, 6 dias)...\n");
    for (int s = 0; s < 4; s++) {
        printf(" -> Simulando con %.1f dias de suministro inicial (N=%d)...", supplyOptions[s], NUM_RUNS);
        fflush(stdout);
        summaries[s] = RunBatchSimulation(NUM_RUNS, supplyOptions[s], &cfg);
        printf(" [LISTO] Victoria: %.1f%%\n", (float)summaries[s].victories / (float)NUM_RUNS * 100.0f);
    }

    printf("\n========================================================================================================================\n");
    printf("   TABLA COMPARATIVA DE RACIONES INICIALES Y SUPERVIVENCIA (BLOQUE C3)                                                  \n");
    printf("========================================================================================================================\n");
    printf("%-10s | %-10s | %-12s | %-10s | %-10s | %-15s | %-10s | %-12s | %-10s\n",
           "Raciones", "% Victoria", "Dias Llegada", "Salud Fin", "Moral Fin", "Reserva Sumin", "Tropas Fin", "Asentam. Vis", "Oro Gast.");
    printf("------------------------------------------------------------------------------------------------------------------------\n");

    for (int s = 0; s < 4; s++) {
        float winRate = (float)summaries[s].victories / (float)NUM_RUNS * 100.0f;
        printf("%-9.1fd | %9.1f%% | %12.1f | %10.1f | %10.1f | %6.2fd (%3.0f%%)    | %10.1f | %12.1f | %10.1f\n",
               supplyOptions[s], winRate,
               summaries[s].avgDaysUsed,
               summaries[s].avgFinalSalud,
               summaries[s].avgFinalMoral,
               summaries[s].avgFinalSupplyDays,
               (summaries[s].avgFinalSupplyDays / supplyOptions[s]) * 100.0f,
               summaries[s].avgFinalTroops,
               summaries[s].avgSettlementsVisited,
               summaries[s].avgGoldSpent);
    }
    printf("------------------------------------------------------------------------------------------------------------------------\n\n");

    printf("DESGLOSE DE CAUSAS DE MUERTE (N=%d por fila):\n", NUM_RUNS);
    printf("------------------------------------------------------------------------------------------------------------------------\n");
    printf("%-10s | %-12s | %-14s | %-14s | %-14s | %-12s\n",
           "Raciones", "Victorias", "Muerte Hambre", "Muerte Combate", "Muerte Motin", "Defecto Mapa");
    printf("------------------------------------------------------------------------------------------------------------------------\n");
    for (int s = 0; s < 4; s++) {
        printf("%-9.1fd | %12d | %14d | %14d | %14d | %12d\n",
               supplyOptions[s],
               summaries[s].victories,
               summaries[s].deathsHunger,
               summaries[s].deathsCombat,
               summaries[s].deathsMutiny,
               summaries[s].mapDefects);
    }
    printf("------------------------------------------------------------------------------------------------------------------------\n\n");

    /* Verificación de metas para la opción recomendada (5.0 días) */
    BatchSummary b5 = summaries[2]; /* 5.0 días */
    float winRate5 = (float)b5.victories / (float)NUM_RUNS * 100.0f;
    float reservePct5 = (b5.avgFinalSupplyDays / 5.0f) * 100.0f;

    printf("EVALUACION DE METAS DE BALANCE (C2) CON RACION RECOMENDADA (5.0 DIAS):\n");
    printf(" - [Meta 1] Tasa de llegada a B >= 60%%:           Victoria = %.1f%%  -> %s\n",
           winRate5, (winRate5 >= 60.0f) ? "[CUMPLE]" : "[NO CUMPLE]");
    printf(" - [Meta 2] Salud media al llegar >= 50:           Salud = %.1f/100   -> %s\n",
           b5.avgFinalSalud, (b5.avgFinalSalud >= 50.0f) ? "[CUMPLE]" : "[NO CUMPLE]");
    printf(" - [Meta 3] Reservas finales entre 15%% y 35%%:       Reserva = %.1f%%   -> %s\n",
           reservePct5, (reservePct5 >= 15.0f && reservePct5 <= 35.0f) ? "[CUMPLE]" : "[NO CUMPLE]");

    return 0;
}
