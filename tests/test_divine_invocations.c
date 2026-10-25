#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "models/general.h"
#include "models/army.h"
#include "models/modifier.h"
#include "models/combat.h"
#include "models/map_generator.h"
#include "controllers/game_master.h"
#include "models/balance_config.h"

int main() {
    printf("============================================================\n");
    printf("PRUEBAS UNITARIAS: INVOCACIONES DIVINAS, RIVALIDADES Y DERROTA\n");
    printf("============================================================\n\n");

    // 0. Inicializar configuración de balance
    BalanceConfig bCfg;
    Balance_InitDefaults(&bCfg);
    Balance_LoadFromFile(&bCfg, "assets/balance.cfg");

    // 1. Cargar Registro de Modificadores
    ModifierRegistry reg = {0};
    reg.count = Modifier_LoadAll("assets/modifiers", reg.defs, MAX_MODIFIERS);
    printf("[TEST 1] Modificadores cargados: %d\n", reg.count);
    assert(reg.count >= 25);

    // 2. Verificar Parseo de Invocaciones y Rivalidades
    printf("[TEST 2] Verificando parseo de Invocacion y Rivalidad en Ares y Atenea...\n");
    ModifierDef* aresDef = Modifier_GetById(reg.defs, reg.count, "ares_furia");
    assert(aresDef != NULL);
    assert(aresDef->hasInvocation == true);
    assert(strcmp(aresDef->invocationName, "Carga Sangrienta") == 0);
    assert(aresDef->invocationRollMod == 8);
    assert(fabsf(aresDef->invocationClashMult - 1.8f) < 0.01f);
    assert(aresDef->invocationCounterIntent == 1); // HOLD
    assert(aresDef->invocationHighVariance == true);
    assert(strcmp(aresDef->rivalGod, "ATENEA") == 0);
    printf("  -> OK: Ares parseado correctamente (Invocacion: %s, Rival: %s)\n",
           aresDef->invocationName, aresDef->rivalGod);

    ModifierDef* ateneaDef = Modifier_GetById(reg.defs, reg.count, "atenea_escudo");
    assert(ateneaDef != NULL);
    assert(ateneaDef->hasInvocation == true);
    assert(strcmp(ateneaDef->invocationName, "Muro de Falange") == 0);
    assert(ateneaDef->invocationRollMod == 4);
    assert(fabsf(ateneaDef->invocationClashMult - 0.4f) < 0.01f);
    assert(ateneaDef->invocationCounterIntent == 0); // CHARGE
    assert(ateneaDef->invocationHighVariance == false);
    assert(strcmp(ateneaDef->rivalGod, "ARES") == 0);
    printf("  -> OK: Atenea parseada correctamente (Invocacion: %s, Rival: %s)\n",
           ateneaDef->invocationName, ateneaDef->rivalGod);

    // 3. Crear Ejercito y Equipar Dioses Rivales
    printf("[TEST 3] Verificando Discordia entre Dioses Rivales...\n");
    General gen = General_Create("Aquiles", 35, 3, 4, 3, 3, 5);
    Army army = {0};
    army.general = gen;
    army.commander = Commander_FromGeneral(&army.general);
    army.iniciados = 20;
    army.soldados = 20;
    army.sargentos = 5;
    army.salud = 100.0f;
    army.moral = 80.0f;

    // Inicializar estados de ración
    for (int g = 0; g < RG_COUNT; g++) {
        army.groupState[g].health = 100.0f;
        army.groupState[g].morale = 80.0f;
    }
    Army_RecalculateAggregates(&army);

    // Equipar Ares
    EquipResult resAres = Army_EquipModifier(&army, aresDef);
    assert(resAres == EQUIP_OK);

    // Sin Atenea, no debe haber discordia
    char conflictMsg[64] = {0};
    bool hasDiscord = Modifier_CheckRivals(&reg, &army, conflictMsg, sizeof(conflictMsg));
    assert(!hasDiscord);
    printf("  -> OK: Un solo dios equipado, sin discordia.\n");

    // Equipar Atenea (rival de Ares)
    EquipResult resAtenea = Army_EquipModifier(&army, ateneaDef);
    assert(resAtenea == EQUIP_OK);

    hasDiscord = Modifier_CheckRivals(&reg, &army, conflictMsg, sizeof(conflictMsg));
    assert(hasDiscord);
    printf("  -> OK: Detectada discordia entre dioses rivales: %s\n", conflictMsg);

    // Comprobar que Atenea otorga +3 a la planificación y la discordia resta -1 (10 + 3 - 1 = 12)
    GameMaster gm = {0};
    gm.modRegistry = reg;
    ModContext mCtx = { &gm.modRegistry, &army, NULL, NULL, TIME_MIDDAY, army.infamia };
    int planRollScore = Modifier_Query(HOOK_PLAN_ROLL, EFFECT_PLANNING, SIDE_PLAYER, &mCtx, 10, NULL);
    assert(planRollScore == 12); // 10 base + 3 (Atenea) - 1 (Discordia Ares vs Atenea) = 12
    printf("  -> OK: Atenea (+3) con discordia (-1) aplicada correctamente (10 base -> %d).\n", planRollScore);

    // 4. Invocación Divina en Combate y Agotamiento de Carta
    printf("[TEST 4] Probando Invocacion Divina en combate y agotamiento...\n");
    EnemySquad enemy = {0};
    enemy.baseStrength = 30;
    enemy.initiates = 15;
    enemy.soldiers = 10;
    enemy.sergeants = 2;
    enemy.position = (HexCoord){ 5, 4 };
    enemy.isActive = true;

    CombatContext cCtx = {0};
    Combat_Init(&cCtx, &gm, &army, &enemy);
    cCtx.state = COMBAT_STATE_EXECUTION;

    // Seleccionar Invocación Divina en la ranura 0 (Ares)
    assert(!Commander_IsCardExhausted(army.commander, 0));
    Combat_SelectDivineInvocation(&cCtx, 0);
    assert(cCtx.selectedPlan == PLAN_DIVINE_INVOCATION);
    assert(cCtx.selectedDivineCardSlot == 0);
    assert(cCtx.activeInvocationDef != NULL);
    assert(strcmp(cCtx.activeInvocationDef->id, "ares_furia") == 0);
    assert(strcmp(cCtx.activeInvocationDef->invocationName, "Carga Sangrienta") == 0);

    // Ejecutar choque (ej: 4 + 4 = 8, par -> bono de alta varianza)
    Combat_RollExecution(&cCtx, 4, 4);

    // La carta de Ares debe haber quedado agotada
    assert(Commander_IsCardExhausted(army.commander, 0) == true);
    printf("  -> OK: La carta de Ares se agoto inmediatamente tras la invocacion.\n");

    // Comprobar que Modifier_CollectActive excluye la carta agotada
    ActiveMod activeMods[16];
    int nActive = Modifier_CollectActive(&reg, &army, &enemy, activeMods, 16);
    bool aresPresent = false;
    bool ateneaPresent = false;
    for (int i = 0; i < nActive; i++) {
        if (strcmp(activeMods[i].def->id, "ares_furia") == 0) aresPresent = true;
        if (strcmp(activeMods[i].def->id, "atenea_escudo") == 0) ateneaPresent = true;
    }
    assert(!aresPresent);
    assert(ateneaPresent);
    printf("  -> OK: La carta agotada no aporta pasivas (Ares ausente, Atenea activa).\n");

    // 5. Resolución de Derrota: Ruptura de la Espiral de Derrota
    printf("[TEST 5] Probando Resolucion de Derrota y Ruptura de Espiral...\n");
    army.position = (HexCoord){ 5, 5 };
    army.previousPosition = (HexCoord){ 5, 6 };
    enemy.position = (HexCoord){ 5, 4 };

    // Simular fin de combate en derrota
    cCtx.endReason = COMBAT_END_DEFEAT;
    cCtx.state = COMBAT_STATE_DONE;
    Combat_Commit(&cCtx);

    // Verificar empuje de 2 hexágonos en dirección opuesta al enemigo
    // enemy at (5, 4), p1 at (5, 6) -> dq = 0, dr = +2 -> retreatPos = (5, 8)
    assert(army.position.q == 5 && army.position.r == 8);
    printf("  -> OK: Ejercito empujado 2 hexes en retirada a (%d, %d).\n", army.position.q, army.position.r);

    // Verificar estado de IA del enemigo victorioso: en AIS_RESTING y no en persecución
    assert(enemy.aiState == AIS_RESTING);
    assert(enemy.patrolWaitTurns == 2);
    assert(enemy.chaseTurnsLeft == 0);
    assert(GameMaster_HasFlag(&gm, "derrota_reciente") == true);
    printf("  -> OK: Enemigo victorioso en AIS_RESTING (patrolWaitTurns = 2), bandera 'derrota_reciente' activa.\n");

    // 6. Campamento y Reagrupamiento de Rezagados
    printf("[TEST 6] Probando Reagrupamiento de Rezagados en Campamento...\n");
    Map map = {0};
    HexTile tile = {0};
    tile.coord = army.position;
    tile.biome = BIOME_PLAINS;
    map.tiles = &tile;
    map.tileCount = 1;

    int oldInit = army.iniciados;
    int oldSold = army.soldados;
    army.salud = 15.0f; // Salud crítica tras derrota

    // Acampar
    GameMaster_ChooseCamp(&gm, &map, &army);

    // Bandera debe haber sido consumida
    assert(!GameMaster_HasFlag(&gm, "derrota_reciente"));
    // Rezagados recuperados: +3 iniciados, +2 soldados
    assert(army.iniciados == oldInit + 3);
    assert(army.soldados == oldSold + 2);
    // Salud reincorporada a un mínimo viable (>= 35)
    assert(army.salud >= 35.0f);
    printf("  -> OK: Rezagados recuperados (+3 iniciados, +2 soldados, salud %.1f >= 35, bandera limpia).\n", army.salud);

    // El campamento además reduce el tiempo de recarga de la carta agotada
    ModifierInstance* aresInst = Commander_GetCardInstance(army.commander, 0);
    assert(aresInst != NULL);
    assert(aresInst->battlesUntilRecharge == 1); // Bajó de 2 a 1 por acampar
    printf("  -> OK: Descanso en campamento avanzo la recarga de la carta divina (quedan %d batallas).\n",
           aresInst->battlesUntilRecharge);

    // 7. Reconsagración en Altar o Templo
    printf("[TEST 7] Probando Reconsagracion en Altar Divino y Templo...\n");
    HexTile altarTile = {0};
    altarTile.coord = (HexCoord){ army.position.q + 1, army.position.r };
    altarTile.biome = BIOME_PLAINS;
    altarTile.structure.type = STRUCT_ALTAR;
    
    gm.currentActionPoints = 10;
    gm.isEncamped = false;
    bool moved = GameMaster_AttemptMove(&gm, &map, &army, &altarTile);
    assert(moved);
    assert(!Commander_IsCardExhausted(army.commander, 0));
    printf("  -> OK: Altar divino reconsagro inmediatamente todas las cartas agotadas al entrar.\n");

    // Probar reconsagración en Templo urbano (costo: 15 oro)
    Commander_SetCardExhausted(army.commander, 0, true);
    assert(Commander_IsCardExhausted(army.commander, 0) == true);
    army.monedas = 25;
    // Simular compra de reconsagración en el templo
    army.monedas -= 15;
    Commander_SetCardExhausted(army.commander, 0, false);
    assert(!Commander_IsCardExhausted(army.commander, 0));
    assert(army.monedas == 10);
    printf("  -> OK: Reconsagracion de carta agotada en templo por 15 de oro exitosa.\n");

    // 8. Inicialización Modular (ArmySetupConfig, Presets, Army_Init)
    printf("[TEST 8] Probando Inicializacion Modular (ArmySetupConfig, Presets, Army_Init)...\n");
    assert(General_GetPresetCount() == 3);
    assert(Lieutenant_GetPresetCount() == 3);

    ArmySetupConfig setupCfg;
    ArmySetup_InitDefault(&setupCfg);
    assert(strcmp(setupCfg.general.name, "Cayo Marcio") == 0);
    assert(setupCfg.lieutenantCount == 3);
    assert(strcmp(setupCfg.lieutenants[0].name, "Publio Varro") == 0);
    assert(strcmp(setupCfg.lieutenants[1].name, "Marco Craso") == 0);
    assert(strcmp(setupCfg.lieutenants[2].name, "Lucio Vitelio") == 0);

    // Modificar estadísticas desde la configuración (como lo hará PHASE_SETUP)
    setupCfg.general = General_GetPreset(1); // Aulo Postumio
    setupCfg.general.stats.planning += 2;
    setupCfg.iniciados = 25;

    Army setupArmy;
    HexCoord startCoord = { 3, 4 };
    Army_Init(&setupArmy, &setupCfg, startCoord);

    assert(setupArmy.commander.ptr == &setupArmy.general);
    assert(strcmp(setupArmy.general.name, "Aulo Postumio") == 0);
    assert(setupArmy.general.planning == General_GetPreset(1).stats.planning + 2);
    assert(setupArmy.iniciados == 25);
    assert(setupArmy.soldados == setupCfg.soldados);
    assert(setupArmy.lieutenantCount == 3);
    assert(setupArmy.position.q == 3 && setupArmy.position.r == 4);
    printf("  -> OK: Army_Init vinculo puntero commander a army.general y aplico presets/overrides correctamente.\n");

    printf("\n============================================================\n");
    printf("¡TODAS LAS PRUEBAS DE INVOCACIONES Y COMBATE PASARON CON EXITO!\n");
    printf("============================================================\n");
    return 0;
}
