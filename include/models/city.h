#ifndef CITY_H
#define CITY_H

#include "models/hex_tile.h"
#include "models/army.h"
#include "models/balance_config.h"
#include "models/rng.h"
#include <stdbool.h>

typedef enum {
    FACTION_HOSTILE = 0,
    FACTION_SUSPICIOUS,
    FACTION_NEUTRAL,
    FACTION_FRIENDLY,
    FACTION_ALLIED
} FactionOpinionLevel;

typedef struct {
    int id;
    char name[32];
    int temperament; /* -1 odia la infamia, +1 la respeta, 0 neutral */
    int opinion;     /* -100..100 */
    int alertLevel;  /* 0: calma, 1: alerta, 2: asedio */
    int alertDays;   /* dias restantes de alerta */
} Faction;

void Faction_Init(Faction* f, int id, const char* name, int temperament, float currentInfamy, const BalanceConfig* cfg);
FactionOpinionLevel Faction_GetLevel(int opinion, const BalanceConfig* cfg);
const char* Faction_GetLevelName(FactionOpinionLevel level);
float Faction_GetPriceMultiplier(int opinion, const BalanceConfig* cfg);
void Faction_ModifyOpinion(Faction* f, int delta);
void Faction_SetAlert(Faction* f, int alertLevel, int days);
void Faction_TickDay(Faction* f);

typedef enum {
    CITY_TAB_MAIN = 0,
    CITY_TAB_MARKET,
    CITY_TAB_ARMORY,
    CITY_TAB_HOSPITAL,
    CITY_TAB_TEMPLE,
    CITY_TAB_INN
} CityTab;

typedef struct {
    HexCoord tileCoord;
    StructureType structType;
    SettlementType settlementType;
    int settlementId;
    char name[64];

    /* Datos de Facción y Asedio (Fase 5) */
    int factionId;
    char factionName[32];
    int factionOpinion;
    float factionPriceMult;
    int factionAlertLevel;
    bool isBesieged;
    bool requestSiege;
    
    // Stock disponible en la tienda durante esta visita
    int stockFood;
    int stockWater;
    int stockCattle;
    int stockInitiates;
    
    // Contadores de ascensos realizados en esta visita
    int promotedInitiates;
    int promotedSoldiers;
    
    // Purificaciones de maldición en el templo
    int cursesRemoved;

    // Estado del menú
    CityTab currentTab;
    bool daySpent; // Si alguna acción consumió la jornada
} CityContext;

/* Inicialización de la visita a una estructura */
void City_InitVisit(CityContext* ctx, HexCoord coord, StructureType structType, SettlementType settlementType, int settlementId, int currentDay);
void City_SetFaction(CityContext* ctx, const Faction* faction, bool isBesieged);

/* Mercado: Consultas de precios unitarios */
int City_GetBuyPriceFood(const CityContext* ctx);
int City_GetSellPriceFood(const CityContext* ctx);
int City_GetBuyPriceWater(const CityContext* ctx);
int City_GetSellPriceWater(const CityContext* ctx);
int City_GetBuyPriceCattle(const CityContext* ctx);
int City_GetSellPriceCattle(const CityContext* ctx);
int City_GetBuyPriceInitiate(const CityContext* ctx);

/* Mercado: Transacciones de compra y venta */
bool City_BuyFood(CityContext* ctx, Army* army, int amount);
bool City_SellFood(CityContext* ctx, Army* army, int amount);
bool City_BuyWater(CityContext* ctx, Army* army, int amount);
bool City_SellWater(CityContext* ctx, Army* army, int amount);
bool City_BuyCattle(CityContext* ctx, Army* army, int amount);
bool City_SellCattle(CityContext* ctx, Army* army, int amount);
bool City_RecruitInitiates(CityContext* ctx, Army* army, int amount);

/* Armería: Ascensos y topes */
int  City_GetMaxPromoteInitiates(const CityContext* ctx, const Army* army);
int  City_GetMaxPromoteSoldiers(const CityContext* ctx, const Army* army);
bool City_PromoteInitiates(CityContext* ctx, Army* army, int amount);
bool City_PromoteSoldiers(CityContext* ctx, Army* army, int amount);

/* Servicios: Hospital, Templo, Posada */
int  City_GetHospitalCost(const CityContext* ctx, const Army* army, int pointsToHeal);
bool City_HealArmy(CityContext* ctx, Army* army, int pointsToHeal);

int  City_GetTempleCurseRemoveCost(const CityContext* ctx);
bool City_RemoveCurse(CityContext* ctx, Army* army);

int  City_GetInnCost(const CityContext* ctx);
bool City_RestAtInn(CityContext* ctx, Army* army);

/* Verificación de funciones disponibles según el tipo de estructura y facción (9.1, 9.4, 9.5) */
bool City_HasMarket(const CityContext* ctx);
bool City_HasArmory(const CityContext* ctx);
bool City_HasHospital(const CityContext* ctx);
bool City_HasTemple(const CityContext* ctx);
bool City_HasInn(const CityContext* ctx);
bool City_CanSiege(const CityContext* ctx);

#endif /* CITY_H */
