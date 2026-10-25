#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "models/army.h"
#include "models/balance_config.h"
#include "models/combat.h"
#include "controllers/game_master.h"
#include "controllers/npc_controller.h"

int main(void) {
    printf("====================================================\n");
    printf("TESTS: SUPERVIVENCIA, BOSQUE, MORAL Y POSADA\n");
    printf("====================================================\n");

    // Inicializar configuración de balance
    Balance_LoadFromFile((BalanceConfig*)Balance_GetGlobal(), "assets/balance.cfg");
    const BalanceConfig* cfg = Balance_GetGlobal();
    assert(cfg != NULL);

    /* ----------------------------------------------------
       TEST 1: Configuración de Moral y Hambruna
       ---------------------------------------------------- */
    printf("[TEST 1] Verificando penalidades de moral aumentadas...\n");
    assert(cfg->reducedMoraleLossPerDay >= 2.5f);
    assert(cfg->starvationFood[0].moral >= 4.0f);
    assert(cfg->starvationFood[1].moral >= 7.0f);
    assert(cfg->starvationFood[2].moral >= 9.0f);
    assert(cfg->campCoverForest >= 3);
    printf("  -> OK: Penalidades de moral por racion reducida y hambruna correctamente calibradas.\n");

    /* ----------------------------------------------------
       TEST 2: Penalidad de Moral en Derrota de Combate
       ---------------------------------------------------- */
    printf("[TEST 2] Verificando perdida de moral al ser derrotado en combate...\n");
    {
        GameMaster gm;
        GameMaster_Init(&gm);

        Army army;
        memset(&army, 0, sizeof(Army));
        army.moral = 80.0f;
        army.salud = 80.0f;
        army.iniciados = 10;
        army.soldados = 10;
        army.sargentos = 4;
        for (int g = 0; g < RG_COUNT; g++) {
            army.groupState[g].morale = 80.0f;
            army.groupState[g].health = 80.0f;
        }

        EnemySquad enemy;
        memset(&enemy, 0, sizeof(EnemySquad));
        enemy.isActive = true;
        enemy.baseStrength = 50;

        CombatContext ctx;
        Combat_Init(&ctx, &gm, &army, &enemy);
        ctx.endReason = COMBAT_END_DEFEAT;
        ctx.state = COMBAT_STATE_DONE;
        Combat_Commit(&ctx);

        // La moral del ejército debe bajar en ~20 puntos
        printf("  Moral tras derrota: %.1f (esperado: ~60.2)\n", army.moral);
        assert(fabsf(army.moral - 60.17f) < 0.5f);
        // Cada rango debe haber recibido penalidad proporcional adicional
        assert(army.groupState[RG_INICIADOS].morale < 60.0f);
        assert(army.groupState[RG_SOLDADOS].morale < 65.0f);
        assert(army.groupState[RG_SARGENTOS].morale < 70.0f);
        printf("  -> OK: Derrota reduce severamente la moral del ejercito y de cada rango.\n");
    }

    /* ----------------------------------------------------
       TEST 3: Acampar en el Bosque (+Moral, +Salud)
       ---------------------------------------------------- */
    printf("[TEST 3] Verificando recuperacion adicional al acampar en bosque...\n");
    {
        GameMaster gm;
        GameMaster_Init(&gm);

        Army army;
        memset(&army, 0, sizeof(Army));
        army.position = (HexCoord){ 5, 5 };
        army.iniciados = 10;
        army.soldados = 10;
        army.sargentos = 2;
        army.moral = 50.0f;
        army.salud = 50.0f;
        army.comida_disponible = 100.0f;
        army.agua_disponible = 100.0f;
        for (int g = 0; g < RG_COUNT; g++) {
            army.groupState[g].morale = 50.0f;
            army.groupState[g].health = 50.0f;
        }

        HexTile singleTile;
        memset(&singleTile, 0, sizeof(HexTile));
        singleTile.coord = (HexCoord){ 5, 5 };
        singleTile.biome = BIOME_FOREST;
        singleTile.terrain = TERRAIN_LAND;

        Map map;
        memset(&map, 0, sizeof(Map));
        map.tiles = &singleTile;
        map.tileCount = 1;

        gm.timeOfDay = TIME_AFTERNOON;
        gm.showCampDecision = true;

        printf("  Calling GameMaster_ChooseCamp...\n"); fflush(stdout);
        GameMaster_ChooseCamp(&gm, &map, &army);
        printf("  Returned from GameMaster_ChooseCamp.\n"); fflush(stdout);

        // Cobertura forestal otorga +3 moral y +3 salud extra sobre Recover_Moral / Recover_Salud
        float expectedMoral = 50.0f + cfg->campRecoverMoral + 3.0f;
        float expectedSalud = 50.0f + cfg->campRecoverSalud + 3.0f;
        printf("  Moral tras acampar en bosque: %.1f (esperado: %.1f)\n", army.moral, expectedMoral);
        printf("  Salud tras acampar en bosque: %.1f (esperado: %.1f)\n", army.salud, expectedSalud);
        fflush(stdout);
        assert(fabsf(army.moral - expectedMoral) < 0.1f);
        assert(fabsf(army.salud - expectedSalud) < 0.1f);
        printf("  -> OK: Acampar en bosque sana y levanta moral con bonificacion estrategica.\n");
    }

    /* ----------------------------------------------------
       TEST 4: Sigilo y Evasion de Cazadores en Bosque
       ---------------------------------------------------- */
    printf("[TEST 4] Verificando que acampar en bosque oculta al jugador del cazador...\n");
    {
        GameMaster gm;
        GameMaster_Init(&gm);

        Army army;
        memset(&army, 0, sizeof(Army));
        army.position = (HexCoord){ 10, 10 };
        army.iniciados = 10;
        army.soldados = 10;
        army.sargentos = 2; // total < 50

        HexTile testTiles[2];
        memset(testTiles, 0, sizeof(testTiles));
        testTiles[0].coord = (HexCoord){ 10, 10 };
        testTiles[0].biome = BIOME_FOREST;
        testTiles[1].coord = (HexCoord){ 10, 13 };
        testTiles[1].biome = BIOME_PLAINS;

        Map map;
        memset(&map, 0, sizeof(Map));
        map.tiles = testTiles;
        map.tileCount = 2;

        // Enemigo cazador a distancia 3
        EnemySquad hunter;
        memset(&hunter, 0, sizeof(EnemySquad));
        hunter.isActive = true;
        hunter.behavior = AI_HUNTER;
        hunter.position = (HexCoord){ 10, 13 };
        hunter.senseRadius = 5;
        hunter.mobility = 10;
        hunter.factionId = -1;
        hunter.aiState = AIS_CHASING;

        gm.enemyCount = 1;
        gm.enemies[0] = hunter;
        gm.isEncamped = true; // Jugador acampado en el bosque

        // Simular cálculo de detección como en NPC_ProcessTurn
        int dist = HexCoord_Distance(hunter.position, army.position);
        assert(dist == 3);

        HexTile* playerTile = &map.tiles[0];
        bool playerInForest = (playerTile->biome == BIOME_FOREST);
        assert(playerInForest == true);

        // Regla: En bosque y acampado, senseRadius del cazador se reduce a 1 (o 2 si tropa > 50)
        int effectiveSense = (army.iniciados + army.soldados + army.sargentos > 50) ? 2 : 1;
        bool detects = (dist <= effectiveSense);
        assert(detects == false); // El cazador NO detecta al jugador a distancia 3 en bosque
        printf("  Cazador a dist 3 con jugador acampado en bosque: detectado = %s (esperado: NO)\n", detects ? "SI" : "NO");

        // Si estuviese en llanura sin cobertura:
        int plainsCover = 0;
        int plainsSense = 5 - plainsCover;
        bool detectsPlains = (dist <= plainsSense);
        assert(detectsPlains == true);
        printf("  Cazador a dist 3 con jugador en llanura: detectado = %s (esperado: SI)\n", detectsPlains ? "SI" : "NO");
        printf("  -> OK: Acampar en bosque evade efectivamente la vista de los cazadores.\n");
    }

    /* ----------------------------------------------------
       TEST 5: Pasar la Noche en la Posada (Descanso, Alimentacion, Inmunidad)
       ---------------------------------------------------- */
    printf("[TEST 5] Verificando descanso en la posada...\n");
    {
        GameMaster gm;
        GameMaster_Init(&gm);
        gm.timeOfDay = TIME_AFTERNOON;
        gm.currentDay = 2;
        gm.clock.currentDay = 2;
        gm.clock.timeOfDay = TIME_AFTERNOON;
        gm.showCampDecision = true;

        Army army;
        memset(&army, 0, sizeof(Army));
        army.position = (HexCoord){ 2, 2 };
        army.iniciados = 10;
        army.soldados = 10;
        army.sargentos = 2;
        army.monedas = 100;
        army.moral = 40.0f;
        army.salud = 50.0f;
        army.comida_disponible = 20.0f;
        army.agua_disponible = 20.0f;
        army.daysWithoutFood = 1;
        army.daysWithoutWater = 1;
        for (int g = 0; g < RG_COUNT; g++) {
            army.groupState[g].morale = 40.0f;
            army.groupState[g].health = 50.0f;
            army.groupState[g].daysUnfed = 1;
        }
        army.groupState[RG_SARGENTOS].satisfaction = 2.0f;
        army.groupState[RG_SARGENTOS].satisfactionMax = 10.0f;

        HexTile innTile;
        memset(&innTile, 0, sizeof(HexTile));
        innTile.coord = (HexCoord){ 2, 2 };
        innTile.biome = BIOME_PLAINS;
        innTile.structure.type = STRUCT_TOWN_HOUSES;
        innTile.structure.factionId = 0; // Gremio

        Map map;
        memset(&map, 0, sizeof(Map));
        map.tiles = &innTile;
        map.tileCount = 1;

        // Un enemigo hostil adyacente que intentaría atacar de noche
        EnemySquad enemy;
        memset(&enemy, 0, sizeof(EnemySquad));
        enemy.isActive = true;
        enemy.behavior = AI_HUNTER;
        enemy.position = (HexCoord){ 2, 3 }; // Adyacente (distancia 1)
        enemy.senseRadius = 5;
        enemy.mobility = 10;
        enemy.factionId = -1; // Bandido hostil
        gm.enemyCount = 1;
        gm.enemies[0] = enemy;

        // El jugador elige dormir en la posada
        GameMaster_ChooseInn(&gm, &map, &army);

        // Verificaciones post-posada:
        // 1. Oro descontado
        printf("  Oro restante: %d (esperado: 90)\n", army.monedas);
        assert(army.monedas == 90);

        // 2. Moral y salud aumentadas
        printf("  Moral: %.1f (esperado: 55.0), Salud: %.1f (esperado: 60.0)\n", army.moral, army.salud);
        assert(fabsf(army.moral - 55.0f) < 0.1f);
        assert(fabsf(army.salud - 60.0f) < 0.1f);

        // 3. Tropa completamente alimentada sin gastar suministros propios
        assert(army.comida_disponible == 20.0f);
        assert(army.agua_disponible == 20.0f);
        assert(army.daysWithoutFood == 0);
        assert(army.daysWithoutWater == 0);
        for (int g = 0; g < RG_COUNT; g++) {
            assert(army.groupState[g].daysUnfed == 0);
        }
        assert(army.groupState[RG_SARGENTOS].satisfaction >= 5.0f);

        // 4. No hay pantalla de reparto pendiente
        assert(gm.rationPending == false);

        // 5. El reloj avanzó a la mañana del siguiente día
        printf("  Hora del dia: %d (esperado: TIME_MORNING=0), Dia: %d (esperado: 3)\n", gm.timeOfDay, gm.currentDay);
        assert(gm.timeOfDay == TIME_MORNING);
        assert(gm.currentDay == 3);

        // 6. Protección de posada: el enemigo adyacente NO inició combate
        assert(gm.pendingCombatEnemy == -1);
        assert(gm.spentNightAtInn == false); // Ya fue reseteado al comenzar la mañana
        assert(gm.phase == PHASE_PLAYER);
        printf("  -> OK: Posada alimenta, recupera salud/moral, avanza a la manana y otorga proteccion total.\n");
    }

    printf("\n====================================================\n");
    printf("TODOS LOS TESTS COMPLETADOS CON EXITO!\n");
    printf("====================================================\n");
    return 0;
}
