#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "models/army.h"
#include "models/city.h"
#include "models/balance_config.h"
#include "models/general.h"
#include "models/hex_tile.h"
#include "models/map_generator.h"
#include "controllers/game_master.h"

/* Helper para crear un mapa mínimo */
static Map CreateSimpleTestMap(void) {
    Map map;
    map.tileCount = 5;
    map.tiles = (HexTile*)calloc(map.tileCount, sizeof(HexTile));
    for (int i = 0; i < map.tileCount; i++) {
        map.tiles[i].coord = (HexCoord){i, 0};
        map.tiles[i].terrain = TERRAIN_LAND;
        map.tiles[i].biome = BIOME_PLAINS;
        map.tiles[i].visible = true;
        map.tiles[i].visited = true;
        map.tiles[i].structure.type = STRUCT_NONE;
    }
    // Asignar una ciudad y un castillo
    map.tiles[1].structure.type = STRUCT_TOWN_MARKET;
    map.tiles[1].structure.settlementId = 1;
    map.tiles[1].structure.settlementType = SETTLEMENT_PRIMARY;
    map.tiles[1].structure.factionId = 0;

    map.tiles[2].structure.type = STRUCT_TOWN_CASTLE;
    map.tiles[2].structure.settlementId = 1;
    map.tiles[2].structure.settlementType = SETTLEMENT_PRIMARY;
    map.tiles[2].structure.factionId = 0;

    return map;
}

static void test_faction_temperament_and_opinion(void) {
    printf("[TEST 8.1] Facciones: Temperamento, Infamia y Niveles de Opinion...\n");
    BalanceConfig cfg;
    Balance_InitDefaults(&cfg);

    // Infamia baja (10)
    Faction fHates, fNeutral, fRespects;
    Faction_Init(&fHates, 0, "Gremio", -1, 10.0f, &cfg);
    Faction_Init(&fNeutral, 1, "Legion", 0, 10.0f, &cfg);
    Faction_Init(&fRespects, 2, "Hierro", 1, 10.0f, &cfg);

    // Con infamia 10 (baja):
    // fHates: 0 + (-1)*(10-50)*0.5 = +20 (Neutral/Amistoso)
    // fNeutral: 0
    // fRespects: 0 + 1*(10-50)*0.5 = -20 (Desconfiado)
    assert(fHates.opinion == 20);
    assert(fNeutral.opinion == 0);
    assert(fRespects.opinion == -20);
    printf("  Infamia Baja (10): OdiaInfamia=%+d, Neutral=%+d, RespetaInfamia=%+d OK\n",
           fHates.opinion, fNeutral.opinion, fRespects.opinion);

    // Infamia alta (90)
    Faction_Init(&fHates, 0, "Gremio", -1, 90.0f, &cfg);
    Faction_Init(&fRespects, 2, "Hierro", 1, 90.0f, &cfg);
    // fHates: 0 + (-1)*(90-50)*0.5 = -20
    // fRespects: 0 + 1*(90-50)*0.5 = +20
    assert(fHates.opinion == -20);
    assert(fRespects.opinion == 20);
    printf("  Infamia Alta (90): OdiaInfamia=%+d, RespetaInfamia=%+d OK\n",
           fHates.opinion, fRespects.opinion);

    // Verificación de niveles de opinión
    assert(Faction_GetLevel(-60, &cfg) == FACTION_HOSTILE);
    assert(Faction_GetLevel(-25, &cfg) == FACTION_SUSPICIOUS);
    assert(Faction_GetLevel(0, &cfg) == FACTION_NEUTRAL);
    assert(Faction_GetLevel(35, &cfg) == FACTION_FRIENDLY);
    assert(Faction_GetLevel(85, &cfg) == FACTION_ALLIED);
    printf("  Niveles de Opinion (Hostil, Desconfiado, Neutral, Amistoso, Aliado) OK\n");
}

static void test_faction_pricing_and_services(void) {
    printf("\n[TEST 8.2] Precios segun Opinion y Servicios Disponibles...\n");
    BalanceConfig cfg;
    Balance_InitDefaults(&cfg);

    General gen = General_Create("Comandante Test", 35, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 0}, gen, 10, 50, 100, 500.0f, 0.0f, 100, 0);

    // 1. Facción Desconfiada (-30 de opinión)
    Faction fSuspicious;
    fSuspicious.id = 0;
    strncpy(fSuspicious.name, "Desconfiados", 32);
    fSuspicious.opinion = -30;
    fSuspicious.alertLevel = 0;
    fSuspicious.alertDays = 0;

    CityContext citySusp;
    City_InitVisit(&citySusp, (HexCoord){1, 0}, STRUCT_TOWN_MARKET, SETTLEMENT_PRIMARY, 1, 1);
    City_SetFaction(&citySusp, &fSuspicious, false);

    // Precio base de comida es 2. Con Desconfiado (*1.5) debe ser 3.
    int buyFoodSusp = City_GetBuyPriceFood(&citySusp);
    assert(buyFoodSusp == 3);
    // Armería debe estar deshabilitada para facciones desconfiadas
    assert(!City_HasArmory(&citySusp));
    // Mercado civil sigue abierto
    assert(City_HasMarket(&citySusp));
    printf("  Desconfiado OK: Precios *1.5 (Comida: %d), Armeria cerrada\n", buyFoodSusp);

    // 2. Facción Aliada (+85 de opinión)
    Faction fAllied;
    fAllied.id = 1;
    strncpy(fAllied.name, "Aliados", 32);
    fAllied.opinion = 85;
    fAllied.alertLevel = 0;
    fAllied.alertDays = 0;

    CityContext cityAllied;
    City_InitVisit(&cityAllied, (HexCoord){1, 0}, STRUCT_TOWN_MARKET, SETTLEMENT_PRIMARY, 1, 1);
    City_SetFaction(&cityAllied, &fAllied, false);

    // Precio base ganado = 25. Con Aliado (*0.70) = 17 o 18.
    int buyCattleAllied = City_GetBuyPriceCattle(&cityAllied);
    assert(buyCattleAllied < 25);
    // Posada en facción aliada debe ser GRATIS (costo 0)
    int innCostAllied = City_GetInnCost(&cityAllied);
    assert(innCostAllied == 0);
    printf("  Aliado OK: Precios con descuento (Ganado: %d), Posada GRATIS (%d Oro)\n", buyCattleAllied, innCostAllied);

    // 3. Facción Hostil (-70 de opinión)
    Faction fHostile;
    fHostile.id = 2;
    strncpy(fHostile.name, "Hostiles", 32);
    fHostile.opinion = -70;
    fHostile.alertLevel = 0;
    fHostile.alertDays = 0;

    CityContext cityHostile;
    City_InitVisit(&cityHostile, (HexCoord){1, 0}, STRUCT_TOWN_MARKET, SETTLEMENT_PRIMARY, 1, 1);
    City_SetFaction(&cityHostile, &fHostile, false);

    // Puertas cerradas: ningún servicio civil disponible
    assert(!City_HasMarket(&cityHostile));
    assert(!City_HasArmory(&cityHostile));
    assert(!City_HasHospital(&cityHostile));
    assert(!City_HasTemple(&cityHostile));
    assert(!City_HasInn(&cityHostile));
    // Pero se permite asediar
    assert(City_CanSiege(&cityHostile));
    printf("  Hostil OK: Puertas cerradas para todos los servicios, Asedio permitido\n");
}

static void test_siege_mechanics_and_acceptance(void) {
    printf("\n[TEST 8.3] Mecanica de Asedio (Prueba de Aceptacion 8)...\n");
    BalanceConfig cfg;
    Balance_InitDefaults(&cfg);

    Map map = CreateSimpleTestMap();
    General gen = General_Create("Comandante Test", 35, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 0}, gen, 10, 50, 100, 500.0f, 0.0f, 100, 0);

    GameMaster gm;
    GameMaster_Init(&gm);
    gm.infamy = 20.0f;

    // Faccion 0 de la ciudad
    gm.factions[0].opinion = 10; // Neutral
    gm.factions[0].alertLevel = 0;
    gm.factions[0].alertDays = 0;

    // A. Proponer el Asedio: La alerta de la guardia sube (Prueba de Aceptación 8)
    HexCoord castleCoord = (HexCoord){2, 0};
    GameMaster_StartSiege(&gm, &map, &army, castleCoord);

    assert(gm.inSiegeCombat == true);
    assert(gm.factions[0].alertLevel == 2); // Alerta sube a Asedio
    assert(gm.factions[0].alertDays == cfg.siegeAlertDays);
    printf("  A. Alerta de la guardia sube a %d (dias: %d) OK\n",
           gm.factions[0].alertLevel, gm.factions[0].alertDays);

    // B. Verificación de guarnición enemiga fortificada
    assert(gm.activeCombat.enemy != NULL);
    assert(Enemy_HasTag(gm.activeCombat.enemy, TAG_FORTIFIED));
    assert(gm.activeCombat.enemyHealth == (float)cfg.siegeGarrisonHealth);
    assert(gm.activeCombat.maxRounds == cfg.siegeMaxRounds);
    printf("  B. Guarnicion Fortificada OK: Tag FORTIFIED, Salud=%.0f, RondasMax=%d\n",
           gm.activeCombat.enemyHealth, gm.activeCombat.maxRounds);

    // C. Victoria del Asedio: Acreditar botin * 3, infamia +15, opinion -40, alerta 5 dias
    int initialGold = army.monedas;
    float initialInfamy = gm.infamy;
    int initialOpinion = gm.factions[0].opinion;

    // Simular botín base generado por combate
    gm.activeCombat.state = COMBAT_STATE_DONE;
    gm.activeCombat.endReason = COMBAT_END_VICTORY;
    gm.activeCombat.lootGold = 40;
    gm.activeCombat.lootFood = 20;
    Combat_Commit(&gm.activeCombat);

    GameMaster_ResolveSiegeVictory(&gm, &map, &army, castleCoord);

    // Botín triplicado (base 40 + 2*40 extra = 120 oro)
    assert(army.monedas == initialGold + 120);
    // Infamia +15
    assert(gm.infamy == initialInfamy + (float)cfg.siegeInfamyGain);
    // Opinión cae en 40
    assert(gm.factions[0].opinion == initialOpinion - cfg.siegeOpinionLossOnSiege);
    // Estructura ocupada
    assert(map.tiles[2].structure.isBesieged == true);
    // Alerta se mantiene activa en 1 (Alerta de guardia) por AlertDays
    assert(gm.factions[0].alertLevel == 1);
    assert(gm.factions[0].alertDays == cfg.siegeAlertDays);
    assert(gm.inSiegeCombat == false);

    printf("  C. Victoria de Asedio OK: Botin x%.1f (%d Oro acreditado), Infamia %.0f -> %.0f, Opinion %d -> %d, isBesieged=true\n",
           cfg.siegeLootMultiplier, army.monedas - initialGold, initialInfamy, gm.infamy, initialOpinion, gm.factions[0].opinion);

    free(map.tiles);
}

static void test_faction_alert_decay_over_days(void) {
    printf("\n[TEST 8.4] Decaimiento de Alerta a lo largo de los dias...\n");
    Faction f;
    f.id = 0;
    strncpy(f.name, "Test", 32);
    f.opinion = 0;
    f.alertLevel = 1;
    f.alertDays = 3;

    // Simular 3 días
    Faction_TickDay(&f);
    assert(f.alertDays == 2);
    assert(f.alertLevel == 1);

    Faction_TickDay(&f);
    assert(f.alertDays == 1);
    assert(f.alertLevel == 1);

    Faction_TickDay(&f);
    assert(f.alertDays == 0);
    assert(f.alertLevel == 0); // Vuelve a la calma
    printf("  Decaimiento OK: Tras 3 dias, la faccion vuelve al estado de Calma (alertLevel=0)\n");
}

int main(void) {
    printf("====================================================\n");
    printf("PRUEBA DE ACEPTACION 8: FACCIONES Y ASEDIO (FASE 5)\n");
    printf("====================================================\n");

    test_faction_temperament_and_opinion();
    test_faction_pricing_and_services();
    test_siege_mechanics_and_acceptance();
    test_faction_alert_decay_over_days();

    printf("\n>>> TODAS LAS PRUEBAS DE ACEPTACION 8 PASARON CON EXITO [100%%] <<<\n");
    return 0;
}
