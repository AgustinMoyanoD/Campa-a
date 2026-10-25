#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "models/general.h"
#include "models/army.h"
#include "models/modifier.h"
#include "models/combat.h"
#include "controllers/game_master.h"
#include "raylib.h"
#include "presentation/ui_widgets.h"

int main() {
    printf("==================================================\n");
    printf("PRUEBAS UNITARIAS: SISTEMA DE CARTAS Y MALDICIONES\n");
    printf("==================================================\n");

    // 1. Cargar Modificadores
    ModifierRegistry reg = {0};
    reg.count = Modifier_LoadAll("assets/modifiers", reg.defs, MAX_MODIFIERS);
    printf("[TEST] Cargando modificadores desde assets/modifiers... Total: %d\n", reg.count);
    assert(reg.count >= 25);

    // 2. Crear General y Ejercito
    General gen = General_Create("Leonidas", 40, 3, 4, 3, 3, 5);
    Army army = {0};
    army.general = gen;
    army.commander = Commander_FromGeneral(&army.general);
    army.iniciados = 20;
    army.soldados = 30;
    army.sargentos = 6;
    army.salud = 100.0f;
    army.moral = 100.0f;

    // Verificar ranuras iniciales
    assert(Commander_GetCardCount(army.commander) == 0);
    assert(Commander_GetCurseCount(army.commander) == 0);
    assert(Commander_GetActiveModifierCount(army.commander) == 0);

    // 3. Equipar una Maldicion (Pacto Sangriento)
    ModifierDef* curseDef = Modifier_GetById(reg.defs, reg.count, "curse_blindness");
    assert(curseDef != NULL);
    assert(curseDef->kind == MOD_KIND_CURSE);

    EquipResult resCurse = Army_EquipModifier(&army, curseDef);
    printf("resCurse: %d, curseDef->target: %d, curseDef->kind: %d, army.curseCount: %d, g->curseCount: %d\n",
           resCurse, curseDef->target, curseDef->kind, army.curseCount, army.general.curseCount);
    fflush(stdout);
    assert(resCurse == EQUIP_OK);

    // ¡TEST CRUCIAL! La maldición NUNCA debe aparecer como carta divina
    printf("[TEST] Comprobando separacion estricta entre Cartas y Maldiciones...\n");
    assert(Commander_GetCardCount(army.commander) == 0); // ¡Cero cartas divinas!
    assert(Commander_GetActiveModifierCount(army.commander) == 0); // ¡Cero cartas divinas!
    assert(Commander_GetCurseCount(army.commander) == 1); // 1 Maldición registrada
    assert(strcmp(Commander_GetCurse(army.commander, 0), "curse_blindness") == 0);
    printf("  -> OK: La maldicion no ocupa ranura de carta divina.\n");

    // 4. Equipar una Carta Divina (Rayo de Zeus)
    ModifierDef* zeusDef = Modifier_GetById(reg.defs, reg.count, "zeus_rayo");
    assert(zeusDef != NULL);
    assert(zeusDef->kind == MOD_KIND_CARD);

    EquipResult resCard = Army_EquipModifier(&army, zeusDef);
    assert(resCard == EQUIP_OK);

    assert(Commander_GetCardCount(army.commander) == 1);
    assert(Commander_GetActiveModifierCount(army.commander) == 1);
    assert(strcmp(Commander_GetCard(army.commander, 0), "zeus_rayo") == 0);
    assert(Commander_GetCurseCount(army.commander) == 1);
    printf("  -> OK: Carta divina Zeus equipada en slot 0, maldicion preservada aparte.\n");

    // 5. Test de Recoleccion de Modificadores Activos
    printf("[TEST] Comprobando Modifier_CollectActive y Modifier_Query...\n");
    ActiveMod activeMods[16];
    int nActive = Modifier_CollectActive(&reg, &army, NULL, activeMods, 16);
    assert(nActive == 2); // 1 carta + 1 maldición

    ModContext mCtx = {
        .reg = &reg,
        .army = &army,
        .enemy = NULL,
        .combat = NULL,
        .timeOfDay = 0,
        .infamy = 0.0f
    };

    // zeus_rayo otorga execution:+4
    int execScore = Modifier_Query(HOOK_EXEC_ROLL, EFFECT_EXECUTION, EFFECT_SIDE_SELF, &mCtx, 10, NULL);
    printf("  -> Ejecucion base 10 + Zeus Rayo (+4) = %d\n", execScore);
    assert(execScore == 14);

    // curse_blindness resta evaluation:-4
    int evalScore = Modifier_Query(HOOK_EVAL_ROLL, EFFECT_EVALUATION, EFFECT_SIDE_SELF, &mCtx, 10, NULL);
    printf("  -> Evaluacion base 10 + Ceguera (-4) = %d\n", evalScore);
    assert(evalScore == 6);

    printf("==================================================\n");
    printf("[EXITO TOTAL] Todas las pruebas del modelo pasaron!\n");
    printf("==================================================\n");
    return 0;
}
