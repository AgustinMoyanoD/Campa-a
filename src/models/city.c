#include "models/city.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

void Faction_Init(Faction* f, int id, const char* name, int temperament, float currentInfamy, const BalanceConfig* cfg) {
    if (!f) return;
    f->id = id;
    strncpy(f->name, name ? name : "Faccion Desconocida", sizeof(f->name) - 1);
    f->name[sizeof(f->name) - 1] = 0;
    f->temperament = temperament;
    f->alertLevel = 0;
    f->alertDays = 0;

    if (!cfg) cfg = Balance_GetGlobal();
    int baseline = cfg ? cfg->factionsBaselineOpinion : 0;
    float k = cfg ? cfg->factionsInfamyFactorK : 0.5f;

    // Formula (9.4): baseline + temperament * (infamia - 50) * k
    float initOp = (float)baseline + (float)temperament * (currentInfamy - 50.0f) * k;
    int op = (int)roundf(initOp);
    if (op < -100) op = -100;
    if (op > 100) op = 100;
    f->opinion = op;
}

FactionOpinionLevel Faction_GetLevel(int opinion, const BalanceConfig* cfg) {
    if (!cfg) cfg = Balance_GetGlobal();
    int host = cfg ? cfg->factionsThresholdHostile : -50;
    int susp = cfg ? cfg->factionsThresholdSuspicious : -20;
    int neut = cfg ? cfg->factionsThresholdNeutral : 20;
    int frie = cfg ? cfg->factionsThresholdFriendly : 50;

    if (opinion <= host) return FACTION_HOSTILE;
    if (opinion <= susp) return FACTION_SUSPICIOUS;
    if (opinion <= neut) return FACTION_NEUTRAL;
    if (opinion <= frie) return FACTION_FRIENDLY;
    return FACTION_ALLIED;
}

const char* Faction_GetLevelName(FactionOpinionLevel level) {
    switch (level) {
        case FACTION_HOSTILE: return "Hostil";
        case FACTION_SUSPICIOUS: return "Desconfiado";
        case FACTION_NEUTRAL: return "Neutral";
        case FACTION_FRIENDLY: return "Amistoso";
        case FACTION_ALLIED: return "Aliado";
        default: return "Neutral";
    }
}

float Faction_GetPriceMultiplier(int opinion, const BalanceConfig* cfg) {
    if (!cfg) cfg = Balance_GetGlobal();
    FactionOpinionLevel lvl = Faction_GetLevel(opinion, cfg);
    switch (lvl) {
        case FACTION_HOSTILE:
        case FACTION_SUSPICIOUS:
            return cfg ? cfg->factionsPriceMultSuspicious : 1.5f;
        case FACTION_NEUTRAL:
            return 1.0f;
        case FACTION_FRIENDLY:
            return cfg ? cfg->factionsPriceMultFriendly : 0.85f;
        case FACTION_ALLIED:
            return cfg ? cfg->factionsPriceMultAllied : 0.70f;
        default:
            return 1.0f;
    }
}

void Faction_ModifyOpinion(Faction* f, int delta) {
    if (!f) return;
    f->opinion += delta;
    if (f->opinion < -100) f->opinion = -100;
    if (f->opinion > 100) f->opinion = 100;
}

void Faction_SetAlert(Faction* f, int alertLevel, int days) {
    if (!f) return;
    f->alertLevel = alertLevel;
    f->alertDays = days;
}

void Faction_TickDay(Faction* f) {
    if (!f) return;
    if (f->alertDays > 0) {
        f->alertDays--;
        if (f->alertDays == 0 && f->alertLevel > 0) {
            f->alertLevel = 0; // Vuelve a la calma
        }
    }
}

void City_InitVisit(CityContext* ctx, HexCoord coord, StructureType structType, SettlementType settlementType, int settlementId, int currentDay) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(CityContext));
    ctx->tileCoord = coord;
    ctx->structType = structType;
    ctx->settlementType = settlementType;
    ctx->settlementId = settlementId;
    ctx->currentTab = CITY_TAB_MAIN;
    ctx->daySpent = false;
    ctx->factionPriceMult = 1.0f;

    // Asignar nombre según la estructura y tipo de asentamiento
    const char* sTypeStr = (settlementType == SETTLEMENT_PRIMARY) ? "Ciudad" : "Poblado";
    switch (structType) {
        case STRUCT_TOWN_HOUSES:
            snprintf(ctx->name, sizeof(ctx->name), "Viviendas de %s #%d", sTypeStr, settlementId);
            break;
        case STRUCT_TOWN_CASTLE:
            snprintf(ctx->name, sizeof(ctx->name), "Castillo y Cuartel de %s #%d", sTypeStr, settlementId);
            break;
        case STRUCT_TOWN_MARKET:
            snprintf(ctx->name, sizeof(ctx->name), "Mercado Central de %s #%d", sTypeStr, settlementId);
            break;
        case STRUCT_TOWN_ARMORY:
            snprintf(ctx->name, sizeof(ctx->name), "Armeria Municipal de %s #%d", sTypeStr, settlementId);
            break;
        default:
            snprintf(ctx->name, sizeof(ctx->name), "Asentamiento #%d", settlementId);
            break;
    }

    const BalanceConfig* cfg = Balance_GetGlobal();
    if (!cfg) return;

    // Generar stock con semilla consistente por estructura y día (9.2)
    uint64_t seed = ((uint64_t)(coord.q + 500) * 73856093ULL) ^
                    ((uint64_t)(coord.r + 500) * 19349663ULL) ^
                    ((uint64_t)settlementId * 83492791ULL) ^
                    ((uint64_t)currentDay * 2654435761ULL);
    Rng visitRng;
    Rng_Init(&visitRng, seed);

    if (structType == STRUCT_TOWN_HOUSES) {
        ctx->stockFood = cfg->marketStockViviendaFood;
        ctx->stockWater = cfg->marketStockViviendaWater;
        ctx->stockCattle = 0;
        ctx->stockInitiates = 0;
    } else {
        ctx->stockFood = cfg->marketStockUrbanoFood;
        ctx->stockWater = cfg->marketStockUrbanoWater;
        ctx->stockCattle = cfg->marketStockUrbanoCattle;
        ctx->stockInitiates = cfg->marketStockUrbanoInitiates;
    }
}

void City_SetFaction(CityContext* ctx, const Faction* faction, bool isBesieged) {
    if (!ctx) return;
    ctx->isBesieged = isBesieged;
    if (faction) {
        ctx->factionId = faction->id;
        strncpy(ctx->factionName, faction->name, sizeof(ctx->factionName) - 1);
        ctx->factionName[sizeof(ctx->factionName) - 1] = 0;
        ctx->factionOpinion = faction->opinion;
        ctx->factionPriceMult = Faction_GetPriceMultiplier(faction->opinion, Balance_GetGlobal());
        ctx->factionAlertLevel = faction->alertLevel;
    } else {
        ctx->factionId = 0;
        strncpy(ctx->factionName, "Neutral", sizeof(ctx->factionName) - 1);
        ctx->factionOpinion = 0;
        ctx->factionPriceMult = 1.0f;
        ctx->factionAlertLevel = 0;
    }
}

int City_GetBuyPriceFood(const CityContext* ctx) {
    const BalanceConfig* cfg = Balance_GetGlobal();
    int base = (cfg && cfg->marketPriceFood > 0) ? cfg->marketPriceFood : 2;
    float mult = (ctx && ctx->factionPriceMult > 0.0f) ? ctx->factionPriceMult : 1.0f;
    int p = (int)roundf((float)base * mult);
    return (p > 0) ? p : 1;
}

int City_GetSellPriceFood(const CityContext* ctx) {
    int buy = City_GetBuyPriceFood(ctx);
    const BalanceConfig* cfg = Balance_GetGlobal();
    float ratio = (cfg && cfg->marketSellRatio > 0.0f) ? cfg->marketSellRatio : 0.4f;
    int sell = (int)floorf((float)buy * ratio);
    if (sell >= buy) sell = buy - 1;
    if (sell < 1) sell = 1;
    return sell;
}

int City_GetBuyPriceWater(const CityContext* ctx) {
    const BalanceConfig* cfg = Balance_GetGlobal();
    int base = (cfg && cfg->marketPriceWater > 0) ? cfg->marketPriceWater : 3;
    float mult = (ctx && ctx->factionPriceMult > 0.0f) ? ctx->factionPriceMult : 1.0f;
    int p = (int)roundf((float)base * mult);
    return (p > 0) ? p : 1;
}

int City_GetSellPriceWater(const CityContext* ctx) {
    int buy = City_GetBuyPriceWater(ctx);
    const BalanceConfig* cfg = Balance_GetGlobal();
    float ratio = (cfg && cfg->marketSellRatio > 0.0f) ? cfg->marketSellRatio : 0.4f;
    int sell = (int)floorf((float)buy * ratio);
    if (sell >= buy) sell = buy - 1;
    if (sell < 1) sell = 1;
    return sell;
}

int City_GetBuyPriceCattle(const CityContext* ctx) {
    const BalanceConfig* cfg = Balance_GetGlobal();
    int base = (cfg && cfg->marketPriceCattle > 0) ? cfg->marketPriceCattle : 25;
    float mult = (ctx && ctx->factionPriceMult > 0.0f) ? ctx->factionPriceMult : 1.0f;
    int p = (int)roundf((float)base * mult);
    return (p > 0) ? p : 1;
}

int City_GetSellPriceCattle(const CityContext* ctx) {
    int buy = City_GetBuyPriceCattle(ctx);
    const BalanceConfig* cfg = Balance_GetGlobal();
    float ratio = (cfg && cfg->marketSellRatio > 0.0f) ? cfg->marketSellRatio : 0.4f;
    int sell = (int)floorf((float)buy * ratio);
    if (sell >= buy) sell = buy - 1;
    if (sell < 1) sell = 1;
    return sell;
}

int City_GetBuyPriceInitiate(const CityContext* ctx) {
    const BalanceConfig* cfg = Balance_GetGlobal();
    int base = (cfg && cfg->marketPriceInitiate > 0) ? cfg->marketPriceInitiate : 5;
    float mult = (ctx && ctx->factionPriceMult > 0.0f) ? ctx->factionPriceMult : 1.0f;
    int p = (int)roundf((float)base * mult);
    return (p > 0) ? p : 1;
}

bool City_BuyFood(CityContext* ctx, Army* army, int amount) {
    if (!ctx || !army || amount <= 0) return false;
    if (ctx->stockFood < amount) return false;
    int cost = amount * City_GetBuyPriceFood(ctx);
    if (army->monedas < cost) return false;

    army->monedas -= cost;
    ctx->stockFood -= amount;
    army->comida_disponible += (float)amount;
    return true;
}

bool City_SellFood(CityContext* ctx, Army* army, int amount) {
    if (!ctx || !army || amount <= 0) return false;
    if (army->comida_disponible < (float)amount) return false;
    int revenue = amount * City_GetSellPriceFood(ctx);

    army->comida_disponible -= (float)amount;
    army->monedas += revenue;
    return true;
}

bool City_BuyWater(CityContext* ctx, Army* army, int amount) {
    if (!ctx || !army || amount <= 0) return false;
    if (ctx->stockWater < amount) return false;
    int cost = amount * City_GetBuyPriceWater(ctx);
    if (army->monedas < cost) return false;

    army->monedas -= cost;
    ctx->stockWater -= amount;
    army->agua_disponible += (float)amount;
    return true;
}

bool City_SellWater(CityContext* ctx, Army* army, int amount) {
    if (!ctx || !army || amount <= 0) return false;
    if (army->agua_disponible < (float)amount) return false;
    int revenue = amount * City_GetSellPriceWater(ctx);

    army->agua_disponible -= (float)amount;
    army->monedas += revenue;
    return true;
}

bool City_BuyCattle(CityContext* ctx, Army* army, int amount) {
    if (!ctx || !army || amount <= 0) return false;
    if (ctx->stockCattle < amount) return false;
    int cost = amount * City_GetBuyPriceCattle(ctx);
    if (army->monedas < cost) return false;

    army->monedas -= cost;
    ctx->stockCattle -= amount;
    army->ganado += amount;
    return true;
}

bool City_SellCattle(CityContext* ctx, Army* army, int amount) {
    if (!ctx || !army || amount <= 0) return false;
    if (army->ganado < amount) return false;
    int revenue = amount * City_GetSellPriceCattle(ctx);

    army->ganado -= amount;
    army->monedas += revenue;
    return true;
}

bool City_RecruitInitiates(CityContext* ctx, Army* army, int amount) {
    if (!ctx || !army || amount <= 0) return false;
    if (ctx->stockInitiates < amount) return false;
    int cost = amount * City_GetBuyPriceInitiate(ctx);
    if (army->monedas < cost) return false;

    army->monedas -= cost;
    ctx->stockInitiates -= amount;
    army->iniciados += amount;
    return true;
}

int City_GetMaxPromoteInitiates(const CityContext* ctx, const Army* army) {
    if (!ctx || !army) return 0;
    const BalanceConfig* cfg = Balance_GetGlobal();
    int maxPerVisit = (cfg && cfg->armoryPromoteInitiateMaxPerVisit > 0) ? cfg->armoryPromoteInitiateMaxPerVisit : 10;
    int cost = (cfg && cfg->armoryPromoteInitiateCost > 0) ? cfg->armoryPromoteInitiateCost : 15;

    int remainingVisit = maxPerVisit - ctx->promotedInitiates;
    if (remainingVisit <= 0) return 0;

    int maxByArmy = army->iniciados;
    int maxByGold = army->monedas / cost;

    int limit = remainingVisit;
    if (maxByArmy < limit) limit = maxByArmy;
    if (maxByGold < limit) limit = maxByGold;
    return (limit > 0) ? limit : 0;
}

int City_GetMaxPromoteSoldiers(const CityContext* ctx, const Army* army) {
    if (!ctx || !army) return 0;
    const BalanceConfig* cfg = Balance_GetGlobal();
    int maxPerVisit = (cfg && cfg->armoryPromoteSoldierMaxPerVisit > 0) ? cfg->armoryPromoteSoldierMaxPerVisit : 2;
    int cost = (cfg && cfg->armoryPromoteSoldierCost > 0) ? cfg->armoryPromoteSoldierCost : 60;
    int ratio = (cfg && cfg->armorySergeantPerSoldiers > 0) ? cfg->armorySergeantPerSoldiers : 15;

    int remainingVisit = maxPerVisit - ctx->promotedSoldiers;
    if (remainingVisit <= 0) return 0;

    int maxByArmy = army->soldados;
    int maxByGold = army->monedas / cost;

    int maxByRatio = 0;
    for (int n = 1; n <= remainingVisit && n <= maxByArmy; n++) {
        if ((army->sargentos + n) * ratio <= (army->soldados - n)) {
            maxByRatio = n;
        } else {
            break;
        }
    }

    int limit = remainingVisit;
    if (maxByArmy < limit) limit = maxByArmy;
    if (maxByGold < limit) limit = maxByGold;
    if (maxByRatio < limit) limit = maxByRatio;
    return (limit > 0) ? limit : 0;
}

bool City_PromoteInitiates(CityContext* ctx, Army* army, int amount) {
    if (!ctx || !army || amount <= 0) return false;
    int maxAllowed = City_GetMaxPromoteInitiates(ctx, army);
    if (amount > maxAllowed) return false;

    const BalanceConfig* cfg = Balance_GetGlobal();
    int cost = amount * ((cfg && cfg->armoryPromoteInitiateCost > 0) ? cfg->armoryPromoteInitiateCost : 15);

    army->iniciados -= amount;
    army->soldados += amount;
    army->monedas -= cost;
    ctx->promotedInitiates += amount;
    if (cfg && cfg->armoryTrainingTakesDay) ctx->daySpent = true;
    return true;
}

bool City_PromoteSoldiers(CityContext* ctx, Army* army, int amount) {
    if (!ctx || !army || amount <= 0) return false;
    int maxAllowed = City_GetMaxPromoteSoldiers(ctx, army);
    if (amount > maxAllowed) return false;

    const BalanceConfig* cfg = Balance_GetGlobal();
    int cost = amount * ((cfg && cfg->armoryPromoteSoldierCost > 0) ? cfg->armoryPromoteSoldierCost : 60);

    army->soldados -= amount;
    army->sargentos += amount;
    army->monedas -= cost;
    ctx->promotedSoldiers += amount;
    if (cfg && cfg->armoryTrainingTakesDay) ctx->daySpent = true;
    return true;
}

int City_GetHospitalCost(const CityContext* ctx, const Army* army, int pointsToHeal) {
    (void)ctx;
    (void)army;
    const BalanceConfig* cfg = Balance_GetGlobal();
    int costPerPoint = (cfg && cfg->servicesHealGoldPerSalud > 0) ? cfg->servicesHealGoldPerSalud : 2;
    return pointsToHeal * costPerPoint;
}

bool City_HealArmy(CityContext* ctx, Army* army, int pointsToHeal) {
    if (!ctx || !army || pointsToHeal <= 0) return false;
    int missing = (int)ceilf(100.0f - army->salud);
    if (missing <= 0 || pointsToHeal > missing) return false;

    int cost = City_GetHospitalCost(ctx, army, pointsToHeal);
    if (army->monedas < cost) return false;

    army->monedas -= cost;
    army->salud += (float)pointsToHeal;
    if (army->salud > 100.0f) army->salud = 100.0f;
    return true;
}

int City_GetTempleCurseRemoveCost(const CityContext* ctx, const Army* army) {
    const BalanceConfig* cfg = Balance_GetGlobal();
    int base = (cfg && cfg->servicesTempleCurseRemoveCost > 0) ? cfg->servicesTempleCurseRemoveCost : 40;
    int count = army ? army->cursesRemovedCount : (ctx ? ctx->cursesRemoved : 0);
    return base + count * 20;
}

bool City_RemoveCurse(CityContext* ctx, Army* army) {
    if (!ctx || !army || army->curseCount <= 0) return false;
    int cost = City_GetTempleCurseRemoveCost(ctx, army);
    if (army->monedas < cost) return false;

    army->monedas -= cost;
    army->curseCount--;
    army->cursesRemovedCount++;
    ctx->cursesRemoved++;
    return true;
}

int City_GetInnCost(const CityContext* ctx) {
    const BalanceConfig* cfg = Balance_GetGlobal();
    // Aliados: descanso gratis (9.4)
    if (ctx && cfg && ctx->factionOpinion >= cfg->factionsThresholdAllied) {
        return 0;
    }
    return (cfg && cfg->servicesInnRestCost > 0) ? cfg->servicesInnRestCost : 10;
}

bool City_RestAtInn(CityContext* ctx, Army* army) {
    if (!ctx || !army) return false;
    int cost = City_GetInnCost(ctx);
    if (army->monedas < cost) return false;

    const BalanceConfig* cfg = Balance_GetGlobal();
    float moralRec = (cfg && cfg->servicesInnRecoverMoral > 0.0f) ? cfg->servicesInnRecoverMoral : 15.0f;

    army->monedas -= cost;
    army->moral += moralRec;
    if (army->moral > 100.0f) army->moral = 100.0f;
    ctx->daySpent = true;
    return true;
}

bool City_HasMarket(const CityContext* ctx) {
    if (!ctx || ctx->isBesieged) return false;
    const BalanceConfig* cfg = Balance_GetGlobal();
    if (ctx->factionAlertLevel >= 1) return false; // Alerta: puertas cerradas
    if (cfg && ctx->factionOpinion <= cfg->factionsThresholdHostile) return false; // Hostil
    return (ctx->structType == STRUCT_TOWN_HOUSES || ctx->structType == STRUCT_TOWN_MARKET);
}

bool City_HasArmory(const CityContext* ctx) {
    if (!ctx || ctx->isBesieged) return false;
    const BalanceConfig* cfg = Balance_GetGlobal();
    if (ctx->factionAlertLevel >= 1) return false;
    if (cfg && ctx->factionOpinion <= cfg->factionsThresholdSuspicious) return false; // Desconfiados limitan armas
    return (ctx->structType == STRUCT_TOWN_CASTLE || ctx->structType == STRUCT_TOWN_ARMORY || ctx->structType == STRUCT_TOWN_MARKET);
}

bool City_HasHospital(const CityContext* ctx) {
    if (!ctx || ctx->isBesieged) return false;
    const BalanceConfig* cfg = Balance_GetGlobal();
    if (ctx->factionAlertLevel >= 1) return false;
    if (cfg && ctx->factionOpinion <= cfg->factionsThresholdHostile) return false;
    return (ctx->structType == STRUCT_TOWN_MARKET || ctx->structType == STRUCT_TOWN_CASTLE);
}

bool City_HasTemple(const CityContext* ctx) {
    if (!ctx || ctx->isBesieged) return false;
    const BalanceConfig* cfg = Balance_GetGlobal();
    if (ctx->factionAlertLevel >= 1) return false;
    if (cfg && ctx->factionOpinion <= cfg->factionsThresholdHostile) return false;
    return (ctx->structType == STRUCT_TOWN_MARKET || ctx->structType == STRUCT_ALTAR);
}

bool City_HasInn(const CityContext* ctx) {
    if (!ctx || ctx->isBesieged) return false;
    const BalanceConfig* cfg = Balance_GetGlobal();
    if (ctx->factionAlertLevel >= 1) return false;
    if (cfg && ctx->factionOpinion <= cfg->factionsThresholdHostile) return false;
    return (ctx->structType == STRUCT_TOWN_HOUSES || ctx->structType == STRUCT_TOWN_MARKET);
}

bool City_CanSiege(const CityContext* ctx) {
    if (!ctx || ctx->isBesieged) return false;
    return (ctx->structType == STRUCT_TOWN_CASTLE || 
           (ctx->structType == STRUCT_TOWN_MARKET && ctx->settlementType == SETTLEMENT_PRIMARY));
}
