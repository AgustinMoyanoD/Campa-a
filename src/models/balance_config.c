#include "models/balance_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static BalanceConfig g_balanceConfig;
static bool g_balanceLoaded = false;

static void TrimWhitespace(char* str) {
    if (!str) return;
    int len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) str[--len] = 0;
    char* start = str;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != str) memmove(str, start, strlen(start) + 1);
}

void Balance_InitDefaults(BalanceConfig* cfg) {
    if (!cfg) return;
    memset(cfg, 0, sizeof(BalanceConfig));

    /* Starvation Food */
    cfg->starvationFood[0] = (StarvationRow){ .moral = 0.0f,  .salud = 0.0f,   .desertPct = 0.0f, .deathPct = 0.0f };
    cfg->starvationFood[1] = (StarvationRow){ .moral = -6.0f, .salud = 0.0f,   .desertPct = 0.0f, .deathPct = 0.0f };
    cfg->starvationFood[2] = (StarvationRow){ .moral = -8.0f, .salud = -10.0f, .desertPct = 3.0f, .deathPct = 2.0f };

    /* Starvation Water */
    cfg->starvationWater[0] = (StarvationRow){ .moral = 0.0f,   .salud = 0.0f,   .desertPct = 0.0f, .deathPct = 0.0f };
    cfg->starvationWater[1] = (StarvationRow){ .moral = -8.0f,  .salud = -5.0f,  .desertPct = 0.0f, .deathPct = 0.0f };
    cfg->starvationWater[2] = (StarvationRow){ .moral = -10.0f, .salud = -18.0f, .desertPct = 4.0f, .deathPct = 3.0f };

    /* Rank Multipliers */
    cfg->rankMultInitiates = 1.0f;
    cfg->rankMultSoldiers  = 0.5f;
    cfg->rankMultSergeants = 0.25f;

    cfg->lowSupplyDays = 2.0f;

    /* Forage */
    cfg->forageEnabled = true;
    cfg->forageFood[0]  = 0.5f;  cfg->forageWater[0] = 0.25f; /* Plains */
    cfg->forageFood[1]  = 0.75f; cfg->forageWater[1] = 0.25f; /* Forest */
    cfg->forageFood[2]  = 0.1f;  cfg->forageWater[2] = 0.1f;  /* Desert */
    cfg->forageFood[3]  = 0.8f;  cfg->forageWater[3] = 0.4f;  /* Jungle */
    cfg->forageFood[4]  = 0.25f; cfg->forageWater[4] = 0.5f;  /* Mountain */
    cfg->forageFood[5]  = 0.25f; cfg->forageWater[5] = 1.0f;  /* Water */

    /* Loot */
    cfg->lootGoldPerStr   = 1.0f;
    cfg->lootFoodPerStr   = 0.5f;
    cfg->lootVariancePct  = 20.0f;

    /* Camp */
    cfg->campRecoverMoral       = 6.0f;
    cfg->campRecoverSalud       = 5.0f;
    cfg->campAmbushTarget       = 7;
    cfg->campStarvingNoRecovery = true;
    cfg->campCoverForest        = 2;
    cfg->campCoverHills         = 1;
    cfg->campCoverPlains        = 0;
    cfg->campCoverMountain      = 1;
    cfg->campCoverDesert        = -1;
    cfg->campCoverRuins         = 1;

    /* Night March */
    cfg->nightMoveCostMult      = 1.5f;
    cfg->nightMoralPerTurn      = -2.0f;
    cfg->nightSaludPerTurn      = -2.0f;
    cfg->nightConsecutiveMult   = 0.5f;
    cfg->nightEncounterChance   = 35;

    /* Combat defaults */
    cfg->maxCombatRounds         = 4;
    cfg->defeatMinHealth         = 10.0f;
    cfg->recountDeadPctVictory   = 25.0f;
    cfg->recountDeadPctStalemate = 35.0f;
    cfg->recountDeadPctDefeat    = 60.0f;
    cfg->recountDeadPctFled      = 70.0f;
    cfg->fleeFoodLossPct         = 15.0f;

    /* Enemy AI defaults */
    cfg->aiActivityRadius           = 25;
    cfg->aiSenseRadiusDay           = 3;
    cfg->aiSenseRadiusNight         = 2;
    cfg->aiSenseRadiusArmySizeBonus = 1;
    cfg->aiChaseTurns               = 6;
    cfg->aiZocEnabled               = true;
    cfg->aiZocExtraCost             = 1;
    cfg->aiFleeHeadStartPhases      = 1;

    /* Hunter defaults */
    cfg->hunterEnabled              = true;
    cfg->hunterSpawnEveryDays       = 4;
    cfg->hunterMobilityMult         = 0.9f;

    /* Market defaults (9.2) */
    cfg->marketSellRatio            = 0.4f;
    cfg->marketPriceFood            = 2;
    cfg->marketPriceWater           = 3;
    cfg->marketPriceCattle          = 25;
    cfg->marketPriceInitiate        = 5;
    cfg->marketStockViviendaFood    = 20;
    cfg->marketStockViviendaWater   = 20;
    cfg->marketStockUrbanoFood      = 100;
    cfg->marketStockUrbanoWater     = 100;
    cfg->marketStockUrbanoCattle    = 6;
    cfg->marketStockUrbanoInitiates = 20;

    /* Armory defaults (9.3) */
    cfg->armoryPromoteInitiateCost        = 15;
    cfg->armoryPromoteInitiateMaxPerVisit = 10;
    cfg->armoryPromoteSoldierCost         = 60;
    cfg->armoryPromoteSoldierMaxPerVisit  = 2;
    cfg->armorySergeantPerSoldiers        = 15;
    cfg->armoryTrainingTakesDay           = true;

    /* Services defaults (9.3) */
    cfg->servicesHealGoldPerSalud         = 2;
    cfg->servicesTempleCurseRemoveCost    = 40;
    cfg->servicesInnRestCost              = 10;
    cfg->servicesInnRecoverMoral          = 15.0f;

    /* Factions defaults (9.4) */
    cfg->factionsBaselineOpinion          = 0;
    cfg->factionsInfamyFactorK            = 0.5f;
    cfg->factionsThresholdHostile         = -50;
    cfg->factionsThresholdSuspicious      = -20;
    cfg->factionsThresholdNeutral         = 20;
    cfg->factionsThresholdFriendly        = 50;
    cfg->factionsThresholdAllied          = 80;
    cfg->factionsPriceMultSuspicious      = 1.5f;
    cfg->factionsPriceMultFriendly        = 0.85f;
    cfg->factionsPriceMultAllied          = 0.70f;

    /* Siege defaults (9.5) */
    cfg->siegeMaxRounds                   = 6;
    cfg->siegeGarrisonHealth              = 150;
    cfg->siegeOpinionLossOnSiege          = 40;
    cfg->siegeInfamyGain                  = 15;
    cfg->siegeAlertDays                   = 5;
    cfg->siegeLootMultiplier              = 3.0f;
}

static void ParseStarvationLine(StarvationRow* row, const char* val) {
    char buf[128];
    strncpy(buf, val, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;

    char* token = strtok(buf, " \t");
    while (token) {
        char* colon = strchr(token, ':');
        if (colon) {
            *colon = 0;
            char* k = token;
            float v = (float)atof(colon + 1);
            if (strcmp(k, "moral") == 0) row->moral = v;
            else if (strcmp(k, "salud") == 0) row->salud = v;
            else if (strcmp(k, "desert") == 0) row->desertPct = v;
            else if (strcmp(k, "death") == 0) row->deathPct = v;
        }
        token = strtok(NULL, " \t");
    }
}

static void ParseForageLine(float* outFood, float* outWater, const char* val) {
    char buf[128];
    strncpy(buf, val, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;

    char* token = strtok(buf, " \t");
    while (token) {
        char* colon = strchr(token, ':');
        if (colon) {
            *colon = 0;
            char* k = token;
            float v = (float)atof(colon + 1);
            if (strcmp(k, "food") == 0) *outFood = v;
            else if (strcmp(k, "water") == 0) *outWater = v;
        }
        token = strtok(NULL, " \t");
    }
}

bool Balance_LoadFromFile(BalanceConfig* cfg, const char* filepath) {
    if (!cfg) return false;
    Balance_InitDefaults(cfg);

    FILE* f = fopen(filepath, "r");
    if (!f) return false;

    char line[256];
    char section[64] = {0};

    while (fgets(line, sizeof(line), f)) {
        TrimWhitespace(line);
        if (line[0] == '#' || line[0] == ';' || line[0] == 0) continue;

        if (line[0] == '[' && line[strlen(line) - 1] == ']') {
            strncpy(section, line + 1, strlen(line) - 2);
            section[strlen(line) - 2] = 0;
            continue;
        }

        char* sep = strchr(line, '=');
        if (!sep) continue;
        *sep = 0;
        char* key = line;
        char* val = sep + 1;
        TrimWhitespace(key);
        TrimWhitespace(val);

        if (strcmp(section, "STARVATION_FOOD") == 0) {
            if (strcmp(key, "Day1") == 0) ParseStarvationLine(&cfg->starvationFood[0], val);
            else if (strcmp(key, "Day2") == 0) ParseStarvationLine(&cfg->starvationFood[1], val);
            else if (strcmp(key, "Day3") == 0) ParseStarvationLine(&cfg->starvationFood[2], val);
        }
        else if (strcmp(section, "STARVATION_WATER") == 0) {
            if (strcmp(key, "Day1") == 0) ParseStarvationLine(&cfg->starvationWater[0], val);
            else if (strcmp(key, "Day2") == 0) ParseStarvationLine(&cfg->starvationWater[1], val);
            else if (strcmp(key, "Day3") == 0) ParseStarvationLine(&cfg->starvationWater[2], val);
        }
        else if (strcmp(section, "STARVATION_RANK_MULT") == 0) {
            if (strcmp(key, "Iniciados") == 0) cfg->rankMultInitiates = (float)atof(val);
            else if (strcmp(key, "Soldados") == 0) cfg->rankMultSoldiers = (float)atof(val);
            else if (strcmp(key, "Sargentos") == 0) cfg->rankMultSergeants = (float)atof(val);
        }
        else if (strcmp(section, "SUPPLY_WARNING") == 0) {
            if (strcmp(key, "LowSupplyDays") == 0) cfg->lowSupplyDays = (float)atof(val);
        }
        else if (strcmp(section, "FORAGE") == 0) {
            if (strcmp(key, "Enabled") == 0) cfg->forageEnabled = (strcmp(val, "true") == 0 || strcmp(val, "1") == 0);
            else if (strcmp(key, "Plains") == 0) ParseForageLine(&cfg->forageFood[0], &cfg->forageWater[0], val);
            else if (strcmp(key, "Forest") == 0) ParseForageLine(&cfg->forageFood[1], &cfg->forageWater[1], val);
            else if (strcmp(key, "Desert") == 0) ParseForageLine(&cfg->forageFood[2], &cfg->forageWater[2], val);
            else if (strcmp(key, "Jungle") == 0) ParseForageLine(&cfg->forageFood[3], &cfg->forageWater[3], val);
            else if (strcmp(key, "Mountain") == 0) ParseForageLine(&cfg->forageFood[4], &cfg->forageWater[4], val);
            else if (strcmp(key, "Water") == 0) ParseForageLine(&cfg->forageFood[5], &cfg->forageWater[5], val);
        }
        else if (strcmp(section, "LOOT") == 0) {
            if (strcmp(key, "GoldPerStr") == 0) cfg->lootGoldPerStr = (float)atof(val);
            else if (strcmp(key, "FoodPerStr") == 0) cfg->lootFoodPerStr = (float)atof(val);
            else if (strcmp(key, "VariancePct") == 0) cfg->lootVariancePct = (float)atof(val);
        }
        else if (strcmp(section, "COMBAT_END") == 0) {
            if (strcmp(key, "MaxRounds") == 0) cfg->maxCombatRounds = atoi(val);
            else if (strcmp(key, "DefeatMinHealth") == 0) cfg->defeatMinHealth = (float)atof(val);
        }
        else if (strcmp(section, "RECOUNT_VICTORY") == 0) {
            if (strcmp(key, "DeadPct") == 0) cfg->recountDeadPctVictory = (float)atof(val);
        }
        else if (strcmp(section, "RECOUNT_STALEMATE") == 0) {
            if (strcmp(key, "DeadPct") == 0) cfg->recountDeadPctStalemate = (float)atof(val);
        }
        else if (strcmp(section, "RECOUNT_DEFEAT") == 0) {
            if (strcmp(key, "DeadPct") == 0) cfg->recountDeadPctDefeat = (float)atof(val);
        }
        else if (strcmp(section, "RECOUNT_FLED") == 0) {
            if (strcmp(key, "DeadPct") == 0) cfg->recountDeadPctFled = (float)atof(val);
        }
        else if (strcmp(section, "CAMP") == 0) {
            if (strcmp(key, "Recover_Moral") == 0) cfg->campRecoverMoral = (float)atof(val);
            else if (strcmp(key, "Recover_Salud") == 0) cfg->campRecoverSalud = (float)atof(val);
            else if (strcmp(key, "AmbushTarget") == 0) cfg->campAmbushTarget = atoi(val);
            else if (strcmp(key, "StarvingNoRecovery") == 0) cfg->campStarvingNoRecovery = (strcmp(val, "true") == 0 || strcmp(val, "1") == 0);
            else if (strcmp(key, "Cover_Forest") == 0) cfg->campCoverForest = atoi(val);
            else if (strcmp(key, "Cover_Hills") == 0) cfg->campCoverHills = atoi(val);
            else if (strcmp(key, "Cover_Plains") == 0) cfg->campCoverPlains = atoi(val);
            else if (strcmp(key, "Cover_Mountain") == 0) cfg->campCoverMountain = atoi(val);
            else if (strcmp(key, "Cover_Desert") == 0) cfg->campCoverDesert = atoi(val);
            else if (strcmp(key, "Cover_Ruins") == 0) cfg->campCoverRuins = atoi(val);
        }
        else if (strcmp(section, "NIGHT_MARCH") == 0) {
            if (strcmp(key, "MoveCostMult") == 0) cfg->nightMoveCostMult = (float)atof(val);
            else if (strcmp(key, "Moral_PerTurn") == 0) cfg->nightMoralPerTurn = (float)atof(val);
            else if (strcmp(key, "Salud_PerTurn") == 0) cfg->nightSaludPerTurn = (float)atof(val);
            else if (strcmp(key, "ConsecutiveMult") == 0) cfg->nightConsecutiveMult = (float)atof(val);
            else if (strcmp(key, "EncounterChance") == 0) cfg->nightEncounterChance = atoi(val);
        }
        else if (strcmp(section, "FLEE") == 0) {
            if (strcmp(key, "FoodLossPct") == 0) cfg->fleeFoodLossPct = (float)atof(val);
        }
        else if (strcmp(section, "ENEMY_AI") == 0) {
            if (strcmp(key, "ActivityRadius") == 0) cfg->aiActivityRadius = atoi(val);
            else if (strcmp(key, "SenseRadius_Day") == 0) cfg->aiSenseRadiusDay = atoi(val);
            else if (strcmp(key, "SenseRadius_Night") == 0) cfg->aiSenseRadiusNight = atoi(val);
            else if (strcmp(key, "SenseRadius_ArmySizeBonus") == 0) cfg->aiSenseRadiusArmySizeBonus = atoi(val);
            else if (strcmp(key, "ChaseTurns") == 0) cfg->aiChaseTurns = atoi(val);
            else if (strcmp(key, "ZOC_Enabled") == 0) cfg->aiZocEnabled = (strcmp(val, "true") == 0 || strcmp(val, "1") == 0);
            else if (strcmp(key, "ZOC_ExtraCost") == 0) cfg->aiZocExtraCost = atoi(val);
            else if (strcmp(key, "FleeHeadStartPhases") == 0) cfg->aiFleeHeadStartPhases = atoi(val);
        }
        else if (strcmp(section, "HUNTER") == 0) {
            if (strcmp(key, "Enabled") == 0) cfg->hunterEnabled = (strcmp(val, "true") == 0 || strcmp(val, "1") == 0);
            else if (strcmp(key, "SpawnEveryDays") == 0) cfg->hunterSpawnEveryDays = atoi(val);
            else if (strcmp(key, "MobilityMult") == 0) cfg->hunterMobilityMult = (float)atof(val);
        }
        else if (strcmp(section, "MARKET") == 0) {
            if (strcmp(key, "SellRatio") == 0) cfg->marketSellRatio = (float)atof(val);
            else if (strcmp(key, "PriceFood") == 0) cfg->marketPriceFood = atoi(val);
            else if (strcmp(key, "PriceWater") == 0) cfg->marketPriceWater = atoi(val);
            else if (strcmp(key, "PriceCattle") == 0) cfg->marketPriceCattle = atoi(val);
            else if (strcmp(key, "PriceInitiate") == 0) cfg->marketPriceInitiate = atoi(val);
            else if (strcmp(key, "StockViviendaFood") == 0) cfg->marketStockViviendaFood = atoi(val);
            else if (strcmp(key, "StockViviendaWater") == 0) cfg->marketStockViviendaWater = atoi(val);
            else if (strcmp(key, "StockUrbanoFood") == 0) cfg->marketStockUrbanoFood = atoi(val);
            else if (strcmp(key, "StockUrbanoWater") == 0) cfg->marketStockUrbanoWater = atoi(val);
            else if (strcmp(key, "StockUrbanoCattle") == 0) cfg->marketStockUrbanoCattle = atoi(val);
            else if (strcmp(key, "StockUrbanoInitiates") == 0) cfg->marketStockUrbanoInitiates = atoi(val);
        }
        else if (strcmp(section, "ARMORY") == 0) {
            if (strcmp(key, "PromoteInitiateCost") == 0) cfg->armoryPromoteInitiateCost = atoi(val);
            else if (strcmp(key, "PromoteInitiateMaxPerVisit") == 0) cfg->armoryPromoteInitiateMaxPerVisit = atoi(val);
            else if (strcmp(key, "PromoteSoldierCost") == 0) cfg->armoryPromoteSoldierCost = atoi(val);
            else if (strcmp(key, "PromoteSoldierMaxPerVisit") == 0) cfg->armoryPromoteSoldierMaxPerVisit = atoi(val);
            else if (strcmp(key, "SergeantPerSoldiers") == 0) cfg->armorySergeantPerSoldiers = atoi(val);
            else if (strcmp(key, "TrainingTakesDay") == 0) cfg->armoryTrainingTakesDay = (strcmp(val, "true") == 0 || strcmp(val, "1") == 0);
        }
        else if (strcmp(section, "SERVICES") == 0) {
            if (strcmp(key, "HealGoldPerSalud") == 0) cfg->servicesHealGoldPerSalud = atoi(val);
            else if (strcmp(key, "TempleCurseRemoveCost") == 0) cfg->servicesTempleCurseRemoveCost = atoi(val);
            else if (strcmp(key, "InnRestCost") == 0) cfg->servicesInnRestCost = atoi(val);
            else if (strcmp(key, "InnRecoverMoral") == 0) cfg->servicesInnRecoverMoral = (float)atof(val);
        }
        else if (strcmp(section, "FACTIONS") == 0) {
            if (strcmp(key, "BaselineOpinion") == 0) cfg->factionsBaselineOpinion = atoi(val);
            else if (strcmp(key, "InfamyFactorK") == 0) cfg->factionsInfamyFactorK = (float)atof(val);
            else if (strcmp(key, "ThresholdHostile") == 0) cfg->factionsThresholdHostile = atoi(val);
            else if (strcmp(key, "ThresholdSuspicious") == 0) cfg->factionsThresholdSuspicious = atoi(val);
            else if (strcmp(key, "ThresholdNeutral") == 0) cfg->factionsThresholdNeutral = atoi(val);
            else if (strcmp(key, "ThresholdFriendly") == 0) cfg->factionsThresholdFriendly = atoi(val);
            else if (strcmp(key, "ThresholdAllied") == 0) cfg->factionsThresholdAllied = atoi(val);
            else if (strcmp(key, "PriceMultSuspicious") == 0) cfg->factionsPriceMultSuspicious = (float)atof(val);
            else if (strcmp(key, "PriceMultFriendly") == 0) cfg->factionsPriceMultFriendly = (float)atof(val);
            else if (strcmp(key, "PriceMultAllied") == 0) cfg->factionsPriceMultAllied = (float)atof(val);
        }
        else if (strcmp(section, "SIEGE") == 0) {
            if (strcmp(key, "MaxRounds") == 0) cfg->siegeMaxRounds = atoi(val);
            else if (strcmp(key, "GarrisonHealth") == 0) cfg->siegeGarrisonHealth = atoi(val);
            else if (strcmp(key, "OpinionLossOnSiege") == 0) cfg->siegeOpinionLossOnSiege = atoi(val);
            else if (strcmp(key, "InfamyGain") == 0) cfg->siegeInfamyGain = atoi(val);
            else if (strcmp(key, "AlertDays") == 0) cfg->siegeAlertDays = atoi(val);
            else if (strcmp(key, "LootMultiplier") == 0) cfg->siegeLootMultiplier = (float)atof(val);
        }
    }

    fclose(f);
    return true;
}

const BalanceConfig* Balance_GetGlobal(void) {
    if (!g_balanceLoaded) {
        if (!Balance_LoadFromFile(&g_balanceConfig, "assets/balance.cfg")) {
            Balance_InitDefaults(&g_balanceConfig);
        }
        g_balanceLoaded = true;
    }
    return &g_balanceConfig;
}

int Balance_GetTerrainCover(const BalanceConfig* cfg, int biome, int structType) {
    if (!cfg) return 0;
    /* Ciudades y centros urbanos aliados (99 = cobertura total / sin emboscadas) */
    if (structType == 4 || structType == 5 || structType == 6 || structType == 7) {
        return 99;
    }
    /* Ruinas, altares, torres */
    if (structType == 1 || structType == 2 || structType == 3) {
        return cfg->campCoverRuins;
    }
    /* Biomas */
    switch (biome) {
        case 1: /* BIOME_FOREST */
        case 3: /* BIOME_JUNGLE */
            return cfg->campCoverForest;
        case 4: /* BIOME_MOUNTAIN */
            return cfg->campCoverMountain;
        case 2: /* BIOME_DESERT */
            return cfg->campCoverDesert;
        case 0: /* BIOME_PLAINS */
        default:
            return cfg->campCoverPlains;
    }
}
