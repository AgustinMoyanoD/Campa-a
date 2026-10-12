#ifndef BALANCE_CONFIG_H
#define BALANCE_CONFIG_H

#include <stdbool.h>

/* ============================================================
   CONFIGURACIÓN DE BALANCE CARGADA DE ARCHIVO (I2, I3)
   Cero números mágicos en el modelo.
   ============================================================ */

typedef struct {
    float moral;
    float salud;
    float desertPct;
    float deathPct;
} StarvationRow;

typedef struct {
    /* Hambre y Sed (6.2) */
    StarvationRow starvationFood[3];  /* Indice 0: Dia 1, 1: Dia 2, 2: Dia 3+ */
    StarvationRow starvationWater[3]; /* Indice 0: Dia 1, 1: Dia 2, 2: Dia 3+ */
    float rankMultInitiates;
    float rankMultSoldiers;
    float rankMultSergeants;
    float lowSupplyDays;
    float supplyThresholdOK;
    float supplyThresholdLow;
    int   forecastRadius;

    /* Forrajeo (6.5) - Biomas: 0:Plains, 1:Forest, 2:Desert, 3:Jungle, 4:Mountain, 5:Water */
    bool forageEnabled;
    float forageFood[6];
    float forageWater[6];

    /* Botín de Combate (6.6) */
    float lootGoldPerStr;
    float lootFoodPerStr;
    float lootVariancePct;

    /* Campamento (7.3) */
    float campRecoverMoral;
    float campRecoverSalud;
    int   campAmbushTarget;
    bool  campStarvingNoRecovery;
    int   campCoverForest;
    int   campCoverHills;
    int   campCoverPlains;
    int   campCoverMountain;
    int   campCoverDesert;
    int   campCoverRuins;

    /* Marcha Nocturna (7.4) */
    float nightMoveCostMult;
    float nightMoralPerTurn;
    float nightSaludPerTurn;
    float nightConsecutiveMult;
    int   nightEncounterChance;

    /* Combate */
    int maxCombatRounds;
    float defeatMinHealth;
    float recountDeadPctVictory;
    float recountDeadPctStalemate;
    float recountDeadPctDefeat;
    float recountDeadPctFled;
    float fleeFoodLossPct;

    /* IA de Enemigos (8.3) */
    int   aiActivityRadius;
    int   aiSenseRadiusDay;
    int   aiSenseRadiusNight;
    int   aiSenseRadiusArmySizeBonus;
    int   aiChaseTurns;
    bool  aiZocEnabled;
    int   aiZocExtraCost;
    int   aiFleeHeadStartPhases;

    /* Perseguidor / Cazador (8.5) */
    bool  hunterEnabled;
    int   hunterSpawnEveryDays;
    float hunterMobilityMult;

    /* Mercado (9.2) */
    float marketSellRatio;
    int   marketPriceFood;
    int   marketPriceWater;
    int   marketPriceCattle;
    int   marketPriceInitiate;
    int   marketStockViviendaFood;
    int   marketStockViviendaWater;
    int   marketStockUrbanoFood;
    int   marketStockUrbanoWater;
    int   marketStockUrbanoCattle;
    int   marketStockUrbanoInitiates;

    /* Armería (9.3) */
    int   armoryPromoteInitiateCost;
    int   armoryPromoteInitiateMaxPerVisit;
    int   armoryPromoteSoldierCost;
    int   armoryPromoteSoldierMaxPerVisit;
    int   armorySergeantPerSoldiers;
    bool  armoryTrainingTakesDay;

    /* Servicios: Hospital, Templo, Posada (9.3) */
    int   servicesHealGoldPerSalud;
    int   servicesTempleCurseRemoveCost;
    int   servicesInnRestCost;
    float servicesInnRecoverMoral;

    /* Facciones (9.4) */
    int   factionsBaselineOpinion;
    float factionsInfamyFactorK;
    int   factionsThresholdHostile;
    int   factionsThresholdSuspicious;
    int   factionsThresholdNeutral;
    int   factionsThresholdFriendly;
    int   factionsThresholdAllied;
    float factionsPriceMultSuspicious;
    float factionsPriceMultFriendly;
    float factionsPriceMultAllied;

    /* Asedio (9.5) */
    int   siegeMaxRounds;
    int   siegeGarrisonHealth;
    int   siegeOpinionLossOnSiege;
    int   siegeInfamyGain;
    int   siegeAlertDays;
    float siegeLootMultiplier;
} BalanceConfig;

void Balance_InitDefaults(BalanceConfig* cfg);
bool Balance_LoadFromFile(BalanceConfig* cfg, const char* filepath);
const BalanceConfig* Balance_GetGlobal(void);
int Balance_GetTerrainCover(const BalanceConfig* cfg, int biome, int structType);

#endif /* BALANCE_CONFIG_H */
