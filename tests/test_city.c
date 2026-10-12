#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "models/army.h"
#include "models/city.h"
#include "models/balance_config.h"
#include "models/general.h"

static void test_market_buy_sell_conservation_and_prices(void) {
    printf("[TEST 6.1] Mercado: Compra, Venta, Conservacion y Precios...\n");
    BalanceConfig cfg;
    Balance_InitDefaults(&cfg);

    General gen = General_Create("Comandante Test", 35, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 0}, gen, 10, 50, 100, 500.0f, 0.0f, 100, 0);
    army.monedas = 200;
    army.comida_disponible = 20.0f;
    army.agua_disponible = 20.0f;
    army.ganado = 2;
    army.iniciados = 10;

    CityContext city;
    City_InitVisit(&city, (HexCoord){5, 5}, STRUCT_TOWN_MARKET, SETTLEMENT_PRIMARY, 1, 1);

    int buyFood = City_GetBuyPriceFood(&city);
    int sellFood = City_GetSellPriceFood(&city);
    int buyWater = City_GetBuyPriceWater(&city);
    int sellWater = City_GetSellPriceWater(&city);
    int buyCattle = City_GetBuyPriceCattle(&city);
    int sellCattle = City_GetSellPriceCattle(&city);
    int buyInit = City_GetBuyPriceInitiate(&city);

    // Precios de venta deben ser estrictamente menores que los de compra
    assert(sellFood < buyFood);
    assert(sellWater < buyWater);
    assert(sellCattle < buyCattle);
    printf("  Precios OK: Comida Compra=%d Venta=%d, Agua Compra=%d Venta=%d, Ganado Compra=%d Venta=%d\n",
           buyFood, sellFood, buyWater, sellWater, buyCattle, sellCattle);

    // Compra de comida
    int initialGold = army.monedas;
    float initialFood = army.comida_disponible;
    int initialStockFood = city.stockFood;
    assert(City_BuyFood(&city, &army, 10));
    assert(army.monedas == initialGold - (10 * buyFood));
    assert(army.comida_disponible == initialFood + 10.0f);
    assert(city.stockFood == initialStockFood - 10);
    printf("  Compra de Comida OK: Oro %d -> %d, Comida %.0f -> %.0f, Stock %d -> %d\n",
           initialGold, army.monedas, initialFood, army.comida_disponible, initialStockFood, city.stockFood);

    // Venta de comida: el jugador nunca puede ganar oro en un ciclo de compra y venta
    initialGold = army.monedas;
    initialFood = army.comida_disponible;
    assert(City_SellFood(&city, &army, 10));
    assert(army.monedas == initialGold + (10 * sellFood));
    assert(army.comida_disponible == initialFood - 10.0f);
    assert(army.monedas < 200); // Se perdió oro por el spread compra/venta
    printf("  Venta de Comida OK: Oro %d -> %d (spread compra/venta respetado)\n", initialGold, army.monedas);

    // Compra de agua
    initialGold = army.monedas;
    float initialWater = army.agua_disponible;
    int initialStockWater = city.stockWater;
    assert(City_BuyWater(&city, &army, 5));
    assert(army.monedas == initialGold - (5 * buyWater));
    assert(army.agua_disponible == initialWater + 5.0f);
    assert(city.stockWater == initialStockWater - 5);
    printf("  Compra de Agua OK: Oro %d -> %d, Agua %.0f -> %.0f\n", initialGold, army.monedas, initialWater, army.agua_disponible);

    // Venta de agua
    initialGold = army.monedas;
    initialWater = army.agua_disponible;
    assert(City_SellWater(&city, &army, 5));
    assert(army.monedas == initialGold + (5 * sellWater));
    assert(army.agua_disponible == initialWater - 5.0f);
    printf("  Venta de Agua OK\n");

    // Compra y venta de ganado
    int initialCattle = army.ganado;
    int initialStockCattle = city.stockCattle;
    initialGold = army.monedas;
    assert(City_BuyCattle(&city, &army, 1));
    assert(army.ganado == initialCattle + 1);
    assert(army.monedas == initialGold - buyCattle);
    assert(city.stockCattle == initialStockCattle - 1);
    
    initialGold = army.monedas;
    assert(City_SellCattle(&city, &army, 1));
    assert(army.ganado == initialCattle);
    assert(army.monedas == initialGold + sellCattle);
    printf("  Compra y Venta de Ganado OK\n");

    // Reclutar iniciados
    int initialInit = army.iniciados;
    int initialStockInit = city.stockInitiates;
    initialGold = army.monedas;
    assert(City_RecruitInitiates(&city, &army, 3));
    assert(army.iniciados == initialInit + 3);
    assert(army.monedas == initialGold - (3 * buyInit));
    assert(city.stockInitiates == initialStockInit - 3);
    printf("  Reclutamiento de Iniciados OK: Iniciados %d -> %d, Stock %d -> %d\n",
           initialInit, army.iniciados, initialStockInit, city.stockInitiates);

    // Prevencion de underflow por oro insuficiente
    army.monedas = 1;
    assert(!City_BuyFood(&city, &army, 10)); // Cuesta 20
    assert(army.monedas == 1);
    assert(!City_BuyCattle(&city, &army, 1));
    assert(army.monedas == 1);
    assert(!City_RecruitInitiates(&city, &army, 1));
    assert(army.monedas == 1);
    printf("  Prevencion de Underflow de Oro OK\n");

    // Prevencion de underflow por recursos insuficientes al vender
    army.ganado = 0;
    assert(!City_SellCattle(&city, &army, 1));
    army.comida_disponible = 2.0f;
    assert(!City_SellFood(&city, &army, 10));
    printf("  Prevencion de Venta sin Recursos OK\n");

    // Prevencion de compra excediendo stock
    army.monedas = 10000;
    assert(!City_BuyCattle(&city, &army, city.stockCattle + 10));
    assert(!City_RecruitInitiates(&city, &army, city.stockInitiates + 10));
    printf("  Prevencion de Exceso de Stock OK\n");
}

static void test_armory_promotions_and_caps(void) {
    printf("\n[TEST 6.2] Armeria: Ascensos, Topes por Visita y Proporcion Sargentos...\n");
    General gen = General_Create("Comandante Test", 35, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 0}, gen, 10, 50, 100, 500.0f, 0.0f, 100, 0);
    army.monedas = 500;
    army.iniciados = 20;
    army.soldados = 14;
    army.sargentos = 0;

    CityContext city;
    City_InitVisit(&city, (HexCoord){5, 5}, STRUCT_TOWN_ARMORY, SETTLEMENT_PRIMARY, 1, 1);

    int totalTroopsBefore = army.iniciados + army.soldados + army.sargentos;

    // Ascenso de Iniciados a Soldados (max 10 por visita)
    assert(City_PromoteInitiates(&city, &army, 5));
    assert(army.iniciados == 15);
    assert(army.soldados == 19);
    assert(army.sargentos == 0);
    assert(army.iniciados + army.soldados + army.sargentos == totalTroopsBefore); // Conservacion
    assert(city.promotedInitiates == 5);
    assert(city.daySpent == true);
    printf("  Ascenso 5 Iniciados -> Soldados OK (Conservacion total: %d)\n", totalTroopsBefore);

    // Intentar ascender 6 mas (superaria el tope de 10 por visita: 5 + 6 = 11 > 10)
    assert(!City_PromoteInitiates(&city, &army, 6));
    assert(city.promotedInitiates == 5);
    // Ascender exactamente 5 mas (llega al limite de 10)
    assert(City_PromoteInitiates(&city, &army, 5));
    assert(city.promotedInitiates == 10);
    // Intentar 1 mas debe fallar por tope de visita
    assert(!City_PromoteInitiates(&city, &army, 1));
    printf("  Tope de 10 ascensos de Iniciados por visita OK\n");

    // Proporcion de Sargentos <= Soldados / 15
    // Actualmente soldados = 24, sargentos = 0.
    // Si ascendemos 1 soldado: nuevos soldados = 23, nuevos sargentos = 1.
    // Condicion: 1 <= 23 / 15 = 1 (1 <= 1 es TRUE).
    assert(City_PromoteSoldiers(&city, &army, 1));
    assert(army.soldados == 23);
    assert(army.sargentos == 1);
    assert(city.promotedSoldiers == 1);
    assert(army.iniciados + army.soldados + army.sargentos == totalTroopsBefore); // Conservacion
    printf("  Ascenso 1 Soldado -> Sargento OK (Soldados: %d, Sargentos: %d)\n", army.soldados, army.sargentos);

    // Intentar ascender 1 soldado mas:
    // Nuevos soldados = 22, nuevos sargentos = 2.
    // Condicion: 2 <= 22 / 15 = 1 (2 <= 1 es FALSE). Debe ser RECHAZADO por ratio!
    assert(!City_PromoteSoldiers(&city, &army, 1));
    assert(army.soldados == 23);
    assert(army.sargentos == 1);
    printf("  Tope de Ratio 1:15 Sargentos/Soldados respetado: rechazo ascenso a 2 sargentos con 22 soldados\n");

    // Dar mas soldados para probar el segundo ascenso y el tope de 2 por visita
    army.soldados = 40; // total troops artificially increased for testing limit of 2/visit
    totalTroopsBefore = army.iniciados + army.soldados + army.sargentos;
    // Ahora con 40 soldados y 1 sargento: (1 + 1) <= (40 - 1)/15 => 2 <= 39/15 = 2. TRUE!
    assert(City_PromoteSoldiers(&city, &army, 1));
    assert(city.promotedSoldiers == 2);
    assert(army.sargentos == 2);
    assert(army.soldados == 39);
    assert(army.iniciados + army.soldados + army.sargentos == totalTroopsBefore);

    // Intentar un 3er ascenso en la misma visita: debe fallar por max 2 por visita
    assert(!City_PromoteSoldiers(&city, &army, 1));
    printf("  Tope de 2 ascensos a Sargento por visita OK\n");
}

static void test_services_hospital_temple_inn(void) {
    printf("\n[TEST 6.3] Servicios: Hospital, Templo y Posada...\n");
    General gen = General_Create("Comandante Test", 35, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 0}, gen, 10, 50, 100, 500.0f, 0.0f, 100, 0);
    army.monedas = 200;
    army.salud = 80.0f;
    army.moral = 50.0f;

    CityContext city;
    City_InitVisit(&city, (HexCoord){5, 5}, STRUCT_TOWN_MARKET, SETTLEMENT_PRIMARY, 1, 1);

    // Hospital: curar 15 salud (cuesta 15 * 2 = 30 oro)
    assert(City_HealArmy(&city, &army, 15));
    assert(army.salud == 95.0f);
    assert(army.monedas == 170);
    printf("  Hospital OK: Salud 80 -> 95, Oro 200 -> 170\n");

    // Curar mas alla del faltante (faltan 5, pedir 10 debe ser rechazado por rebasar)
    assert(!City_HealArmy(&city, &army, 10));
    // Curar exactamente los 5 faltantes
    assert(City_HealArmy(&city, &army, 5));
    assert(army.salud == 100.0f);
    assert(army.monedas == 160);
    printf("  Hospital Cap 100 OK: Salud 100, Oro 160\n");

    // Templo: agregar una maldición en Army
    army.curseCount = 1;

    int costCurse1 = City_GetTempleCurseRemoveCost(&city, &army);
    assert(City_RemoveCurse(&city, &army));
    assert(army.curseCount == 0);
    assert(army.monedas == 160 - costCurse1);
    assert(city.cursesRemoved == 1);
    int costCurse2 = City_GetTempleCurseRemoveCost(&city, &army);
    assert(costCurse2 > costCurse1); // El costo escala con cada purificacion
    printf("  Templo OK: Maldicion removida, costo base %d pagado, proximo costo escalo a %d\n", costCurse1, costCurse2);

    // Posada: descansar
    int initialGold = army.monedas;
    float initialMoral = army.moral;
    city.daySpent = false;
    assert(City_RestAtInn(&city, &army));
    assert(army.monedas == initialGold - 10);
    assert(army.moral == initialMoral + 15.0f);
    assert(city.daySpent == true);
    printf("  Posada OK: Moral %.0f -> %.0f, consumio dia\n", initialMoral, army.moral);
}

static void test_stock_generation_determinism(void) {
    printf("\n[TEST 6.4] Determinismo de Generacion de Stock...\n");
    CityContext c1, c2, c3;
    City_InitVisit(&c1, (HexCoord){10, 20}, STRUCT_TOWN_MARKET, SETTLEMENT_PRIMARY, 1, 3);
    City_InitVisit(&c2, (HexCoord){10, 20}, STRUCT_TOWN_MARKET, SETTLEMENT_PRIMARY, 1, 3);
    City_InitVisit(&c3, (HexCoord){10, 20}, STRUCT_TOWN_MARKET, SETTLEMENT_PRIMARY, 1, 4); // Dia siguiente

    assert(c1.stockFood == c2.stockFood);
    assert(c1.stockWater == c2.stockWater);
    assert(c1.stockCattle == c2.stockCattle);
    assert(c1.stockInitiates == c2.stockInitiates);
    printf("  Determinismo OK: Misma posicion y dia producen stock identico (Comida: %d, Agua: %d, Ganado: %d, Iniciados: %d)\n",
           c1.stockFood, c1.stockWater, c1.stockCattle, c1.stockInitiates);
}

int main(void) {
    printf("====================================================\n");
    printf("PRUEBA DE ACEPTACION 6: MERCADO, ARMERIA Y CIUDADES\n");
    printf("====================================================\n");

    test_market_buy_sell_conservation_and_prices();
    test_armory_promotions_and_caps();
    test_services_hospital_temple_inn();
    test_stock_generation_determinism();

    printf("\n>>> TODAS LAS PRUEBAS DE ACEPTACION 6 PASARON CON EXITO [100%%] <<<\n");
    return 0;
}
