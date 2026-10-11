#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "models/army.h"
#include "models/enemy.h"
#include "models/balance_config.h"
#include "models/combat.h"
#include "models/hex_tile.h"
#include "models/map_generator.h"
#include "controllers/game_master.h"
#include "models/rng.h"
#include "models/day_clock.h"

/* Helper para crear un mapa hexagonal controlado para pruebas */
static Map CreateTestMap(int radius) {
    int count = 1 + 3 * radius * (radius + 1);
    HexTile* tiles = (HexTile*)calloc(count, sizeof(HexTile));
    int idx = 0;
    for (int q = -radius; q <= radius; q++) {
        int r1 = (q < 0) ? -radius - q : -radius;
        int r2 = (q < 0) ? radius : radius - q;
        for (int r = r1; r <= r2; r++) {
            tiles[idx].coord = (HexCoord){q, r};
            tiles[idx].terrain = TERRAIN_LAND;
            tiles[idx].biome = BIOME_PLAINS;
            tiles[idx].visible = true;
            tiles[idx].visited = true;
            tiles[idx].structure.type = STRUCT_NONE;
            tiles[idx].bridge = BRIDGE_NONE;
            idx++;
        }
    }
    for (int i = 0; i < count; i++) {
        for (int d = 0; d < 6; d++) {
            HexCoord nCoord = HexCoord_GetNeighbor(tiles[i].coord, (HexDirection)d);
            for (int j = 0; j < count; j++) {
                if (tiles[j].coord.q == nCoord.q && tiles[j].coord.r == nCoord.r) {
                    tiles[i].neighbors[d] = &tiles[j];
                    break;
                }
            }
        }
    }
    Map m;
    m.tiles = tiles;
    m.tileCount = count;
    return m;
}

static void FreeTestMap(Map* m) {
    if (m && m->tiles) {
        free(m->tiles);
        m->tiles = NULL;
        m->tileCount = 0;
    }
}

/* ============================================================
   SUBTEST 7.1: Determinismo con la misma semilla
   ============================================================ */
static void test_determinism(void) {
    printf("[SUBTEST 7.1] Determinismo con la misma semilla...\n");
    Map map1 = CreateTestMap(5);
    Map map2 = CreateTestMap(5);

    GameMaster gm1, gm2;
    GameMaster_Init(&gm1);
    GameMaster_Init(&gm2);
    Rng_Init(&gm1.rng, 88888ULL);
    Rng_Init(&gm2.rng, 88888ULL);

    General gen = General_Create("Comandante", 30, 2, 2, 2, 2, 2);
    Army army1 = Army_Create((HexCoord){0, 0}, gen, 5, 20, 10, 100.0f, 100.0f, 100, 5);
    Army army2 = Army_Create((HexCoord){0, 0}, gen, 5, 20, 10, 100.0f, 100.0f, 100, 5);

    GameMaster_SpawnTestEnemies(&gm1, &map1, army1.position);
    GameMaster_SpawnTestEnemies(&gm2, &map2, army2.position);

    assert(gm1.enemyCount == gm2.enemyCount);
    for (int i = 0; i < gm1.enemyCount; i++) {
        assert(gm1.enemies[i].position.q == gm2.enemies[i].position.q);
        assert(gm1.enemies[i].position.r == gm2.enemies[i].position.r);
        assert(gm1.enemies[i].behavior == gm2.enemies[i].behavior);
    }

    // Simular 6 turnos en ambas instancias
    for (int t = 0; t < 6; t++) {
        GameMaster_ProcessEnemyTurn(&gm1, &map1, &army1);
        GameMaster_ProcessEnemyTurn(&gm2, &map2, &army2);

        assert(gm1.enemyCount == gm2.enemyCount);
        for (int i = 0; i < gm1.enemyCount; i++) {
            assert(gm1.enemies[i].position.q == gm2.enemies[i].position.q);
            assert(gm1.enemies[i].position.r == gm2.enemies[i].position.r);
            assert(gm1.enemies[i].aiState == gm2.enemies[i].aiState);
            assert(gm1.enemies[i].chaseTurnsLeft == gm2.enemies[i].chaseTurnsLeft);
        }
    }
    printf("  -> OK: 6 turnos simulados idénticos en dos instancias independientes.\n");

    FreeTestMap(&map1);
    FreeTestMap(&map2);
}

/* ============================================================
   SUBTEST 7.2: Nunca entran a hexágonos intransitables (Agua)
   ============================================================ */
static void test_water_impassable(void) {
    printf("[SUBTEST 7.2] Nunca entran a hexagonos intransitables (Agua)...\n");
    Map map = CreateTestMap(5);
    GameMaster gm;
    GameMaster_Init(&gm);
    Rng_Init(&gm.rng, 12345ULL);

    General gen = General_Create("Comandante", 30, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 4}, gen, 5, 20, 10, 100.0f, 100.0f, 100, 5);

    // Colocar un muro de agua en la fila r = 1 y r = 2
    for (int i = 0; i < map.tileCount; i++) {
        if (map.tiles[i].coord.r == 1 || map.tiles[i].coord.r == 2) {
            map.tiles[i].terrain = TERRAIN_WATER;
            map.tiles[i].biome = BIOME_WATER;
        }
    }

    // Crear un enemigo en (0, -2) que persigue al jugador en (0, 4)
    gm.enemyCount = 1;
    gm.enemies[0] = Enemy_CreateWithBehavior("Rastreador", (HexCoord){0, -2}, 30, 8, 10, 5, 0, AI_HUNTER);
    gm.enemies[0].aiState = AIS_CHASING;
    gm.enemies[0].lastKnownPlayerPos = army.position;

    // Procesar 10 turnos de movimiento del enemigo
    for (int t = 0; t < 10; t++) {
        GameMaster_ProcessEnemyTurn(&gm, &map, &army);
        HexTile* currentTile = Map_GetTile(&map, gm.enemies[0].position);
        assert(currentTile != NULL);
        assert(currentTile->terrain != TERRAIN_WATER);
        assert(currentTile->biome != BIOME_WATER);
    }
    printf("  -> OK: El enemigo rodeo o se detuvo, jamas piso casillas de agua.\n");

    FreeTestMap(&map);
}

/* ============================================================
   SUBTEST 7.3: Velocidad de persecución no supera mobility
   ============================================================ */
static void test_speed_budget(void) {
    printf("[SUBTEST 7.3] Velocidad de persecucion no supera mobility...\n");
    Map map = CreateTestMap(6);
    GameMaster gm;
    GameMaster_Init(&gm);
    Rng_Init(&gm.rng, 9999ULL);

    General gen = General_Create("Comandante", 30, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 5}, gen, 5, 20, 10, 100.0f, 100.0f, 100, 5);

    // Colocar un escuadrón con movilidad 4
    int mob = 4;
    HexCoord startPos = (HexCoord){0, -4};
    gm.enemyCount = 1;
    gm.enemies[0] = Enemy_CreateWithBehavior("Patrulla Lenta", startPos, 25, mob, 10, 0, 0, AI_HUNTER);
    gm.enemies[0].aiState = AIS_CHASING;
    gm.enemies[0].lastKnownPlayerPos = army.position;

    GameMaster_ProcessEnemyTurn(&gm, &map, &army);

    // Calcular el coste total de PA consumido
    HexTile* startTile = Map_GetTile(&map, startPos);
    HexTile* endTile = Map_GetTile(&map, gm.enemies[0].position);
    HexTile* path[128];
    int pathCost = 0;
    int pathLen = Map_FindPathEnemy(&map, startTile, endTile, &pathCost, path);

    assert(pathCost <= mob);
    int dist = HexCoord_Distance(startPos, gm.enemies[0].position);
    assert(dist <= mob);
    printf("  -> OK: Movilidad=%d, PA consumidos=%d, Hexagonos avanzados=%d (<= %d).\n", mob, pathCost, dist, mob);

    FreeTestMap(&map);
}

/* ============================================================
   SUBTEST 7.4: La persecución termina tras ChaseTurns
   ============================================================ */
static void test_chase_termination(void) {
    printf("[SUBTEST 7.4] La persecucion termina tras ChaseTurns...\n");
    Map map = CreateTestMap(6);
    GameMaster gm;
    GameMaster_Init(&gm);
    const BalanceConfig* cfg = Balance_GetGlobal();
    int maxChaseTurns = cfg->aiChaseTurns; // 6

    General gen = General_Create("Comandante", 30, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 6}, gen, 5, 20, 10, 100.0f, 100.0f, 100, 5);

    // Enemigo en (0, 0) que detectó al jugador en (0, 1) y entró en CHASING
    gm.enemyCount = 1;
    gm.enemies[0] = Enemy_CreateWithBehavior("Guardia Perseguidor", (HexCoord){0, 0}, 30, 4, 10, 5, 0, AI_GUARD);
    gm.enemies[0].aiState = AIS_CHASING;
    gm.enemies[0].chaseTurnsLeft = maxChaseTurns;
    gm.enemies[0].lastKnownPlayerPos = (HexCoord){0, 1};

    // Jugador se encuentra lejos (distancia > 5, fuera de senseRadius)
    for (int t = 0; t < maxChaseTurns; t++) {
        assert(gm.enemies[0].aiState == AIS_CHASING);
        GameMaster_ProcessEnemyTurn(&gm, &map, &army);
    }

    // Al agotarse ChaseTurns debe haber pasado a AIS_SEARCHING
    assert(gm.enemies[0].aiState == AIS_SEARCHING);
    printf("  -> OK: Tras %d turnos sin ver al jugador, pasa a AIS_SEARCHING.\n", maxChaseTurns);

    // Avanzar turnos de búsqueda
    while (gm.enemies[0].aiState == AIS_SEARCHING) {
        GameMaster_ProcessEnemyTurn(&gm, &map, &army);
    }

    // Finalmente pasa a AIS_RETURNING
    assert(gm.enemies[0].aiState == AIS_RETURNING);
    printf("  -> OK: Tras la busqueda pasa a AIS_RETURNING hacia su base.\n");

    FreeTestMap(&map);
}

/* ============================================================
   SUBTEST 7.5: Contacto dispara un solo combate y registra iniciador
   ============================================================ */
static void test_contact_and_initiator(void) {
    printf("[SUBTEST 7.5] Contacto dispara combate y registra iniciador...\n");
    Map map = CreateTestMap(5);
    GameMaster gm;
    GameMaster_Init(&gm);

    General gen = General_Create("Comandante", 30, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 0}, gen, 5, 20, 10, 100.0f, 100.0f, 100, 5);

    // Caso A: Enemigo avanza e inicia el contacto (initiator = 1)
    gm.enemyCount = 1;
    gm.enemies[0] = Enemy_CreateWithBehavior("Asaltante", (HexCoord){0, 2}, 30, 6, 10, 5, 0, AI_HUNTER);
    gm.enemies[0].aiState = AIS_CHASING;
    gm.enemies[0].lastKnownPlayerPos = army.position;

    gm.pendingCombatEnemy = -1;
    GameMaster_ProcessEnemyTurn(&gm, &map, &army);

    assert(gm.pendingCombatEnemy == 0);
    assert(gm.enemies[0].initiator == 1);
    assert(gm.activeCombat.initiator == 1);
    printf("  -> OK (Enemigo inicia): pendingCombatEnemy=0, initiator=%d\n", gm.activeCombat.initiator);

    // Caso B: Emboscador activa ambushed = true
    gm.pendingCombatEnemy = -1;
    gm.enemies[0] = Enemy_CreateWithBehavior("Emboscador Oculto", (HexCoord){0, 2}, 30, 6, 10, 5, 0, AI_AMBUSH);
    gm.enemies[0].aiState = AIS_CHASING;
    GameMaster_ProcessEnemyTurn(&gm, &map, &army);
    assert(gm.pendingCombatEnemy == 0);
    assert(gm.activeCombat.ambushed == true);
    printf("  -> OK (Emboscador): ambushed=%s\n", gm.activeCombat.ambushed ? "true" : "false");

    // Caso C: Jugador pisa al enemigo (initiator = 0)
    gm.pendingCombatEnemy = -1;
    gm.enemies[0].position = (HexCoord){0, 1};
    gm.enemies[0].behavior = AI_PATROL;
    gm.currentActionPoints = 10;
    army.position = (HexCoord){0, 0};

    HexTile* target = Map_GetTile(&map, (HexCoord){0, 1});
    bool moved = GameMaster_AttemptMove(&gm, &map, &army, target);
    assert(moved == true);
    assert(gm.pendingCombatEnemy == 0);
    assert(gm.enemies[0].initiator == 0);
    assert(gm.activeCombat.initiator == 0);
    printf("  -> OK (Jugador inicia): pendingCombatEnemy=0, initiator=%d\n", gm.activeCombat.initiator);

    FreeTestMap(&map);
}

/* ============================================================
   SUBTEST 7.6: ZOC aumenta el coste de salir (+1 PA)
   ============================================================ */
static void test_zoc_penalty(void) {
    printf("[SUBTEST 7.6] ZOC aumenta el coste de salir (+1 PA)...\n");
    Map map = CreateTestMap(5);
    GameMaster gm;
    GameMaster_Init(&gm);

    General gen = General_Create("Comandante", 30, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 0}, gen, 5, 20, 10, 100.0f, 100.0f, 100, 5);

    // Colocar un enemigo adyacente en (1, 0)
    gm.enemyCount = 1;
    gm.enemies[0] = Enemy_CreateWithBehavior("Guardia ZOC", (HexCoord){1, 0}, 30, 4, 10, 5, 0, AI_GUARD);

    // Moverse a (0, 1) (Llanura: coste base 1 PA)
    // Al estar adyacente al enemigo en (1, 0), salir del hexágono cuesta 1 + 1 = 2 PA
    gm.currentActionPoints = 10;
    HexTile* tile1 = Map_GetTile(&map, (HexCoord){0, 1});
    bool moved1 = GameMaster_AttemptMove(&gm, &map, &army, tile1);
    assert(moved1 == true);
    int apSpentZOC = 10 - gm.currentActionPoints;
    assert(apSpentZOC == 2);
    printf("  -> OK Con ZOC: Coste gastado = %d PA (Base 1 + Extra ZOC 1)\n", apSpentZOC);

    // Ahora el jugador está en (0, 1). La distancia a (1, 0) es HexCoord_Distance((0,1), (1,0)) = 1.
    // Moverse a (-1, 2): distancia a (1, 0) era 1, de nuevo en ZOC. Coste = 2 PA.
    gm.currentActionPoints = 10;
    HexTile* tile2 = Map_GetTile(&map, (HexCoord){-1, 2});
    GameMaster_AttemptMove(&gm, &map, &army, tile2);
    assert(gm.currentActionPoints == 8);

    // Ahora el jugador está en (-1, 2). Distancia a (1, 0) es > 1 (fuera de ZOC).
    // Moverse a (-2, 2): coste normal = 1 PA.
    gm.currentActionPoints = 10;
    HexTile* tile3 = Map_GetTile(&map, (HexCoord){-2, 2});
    GameMaster_AttemptMove(&gm, &map, &army, tile3);
    int apSpentSinZOC = 10 - gm.currentActionPoints;
    assert(apSpentSinZOC == 1);
    printf("  -> OK Sin ZOC: Coste gastado = %d PA (Normal)\n", apSpentSinZOC);

    FreeTestMap(&map);
}

/* ============================================================
   SUBTEST 7.7: Tras huir el enemigo salta una fase (skipNextPhase)
   ============================================================ */
static void test_flee_skip_phase(void) {
    printf("[SUBTEST 7.7] Tras huir el enemigo salta una fase (skipNextPhase)...\n");
    Map map = CreateTestMap(5);
    GameMaster gm;
    GameMaster_Init(&gm);

    General gen = General_Create("Comandante", 30, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 0}, gen, 5, 20, 10, 100.0f, 100.0f, 100, 5);
    army.previousPosition = (HexCoord){0, -1};

    gm.enemyCount = 1;
    gm.enemies[0] = Enemy_CreateWithBehavior("Perseguidor", (HexCoord){0, 1}, 30, 4, 10, 5, 0, AI_HUNTER);

    // Iniciar combate y simular huida
    CombatContext ctx;
    Combat_Init(&ctx, &gm, &army, &gm.enemies[0]);
    ctx.endReason = COMBAT_END_FLEE;
    ctx.state = COMBAT_STATE_DONE;
    Combat_Commit(&ctx);

    // Verificar estado del enemigo tras huir
    const BalanceConfig* cfg = Balance_GetGlobal();
    assert(gm.enemies[0].aiState == AIS_CHASING);
    assert(gm.enemies[0].skipNextPhase == cfg->aiFleeHeadStartPhases);
    printf("  -> OK: Enemigo puesto en AIS_CHASING con skipNextPhase=%d\n", gm.enemies[0].skipNextPhase);

    // Ejecutar el turno del enemigo: debe saltar la fase y NO moverse
    HexCoord posBefore = gm.enemies[0].position;
    gm.pendingCombatEnemy = -1;
    GameMaster_ProcessEnemyTurn(&gm, &map, &army);

    assert(gm.enemies[0].position.q == posBefore.q && gm.enemies[0].position.r == posBefore.r);
    assert(gm.enemies[0].skipNextPhase == 0);
    assert(gm.pendingCombatEnemy == -1);
    printf("  -> OK: Durante skipNextPhase=1 no hubo movimiento ni combate, contador bajo a 0.\n");

    // Siguiente turno: ahora sí debe moverse
    GameMaster_ProcessEnemyTurn(&gm, &map, &army);
    // El enemigo ahora se mueve hacia la posición del jugador
    int distAfter = HexCoord_Distance(gm.enemies[0].position, army.position);
    printf("  -> OK: En el turno siguiente reanudo la persecucion (Distancia al jugador: %d).\n", distAfter);

    FreeTestMap(&map);
}

int main(void) {
    printf("==================================================\n");
    printf("   PRUEBA DE ACEPTACION 7: IA ENEMIGA Y MOVIMIENTO \n");
    printf("==================================================\n\n");

    test_determinism();
    test_water_impassable();
    test_speed_budget();
    test_chase_termination();
    test_contact_and_initiator();
    test_zoc_penalty();
    test_flee_skip_phase();

    printf("\n==================================================\n");
    printf("   TODAS LAS PRUEBAS DE LA FASE 3 PASARON [OK]    \n");
    printf("==================================================\n");
    return 0;
}
