#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "models/combat.h"
#include "models/army.h"
#include "models/general.h"
#include "models/enemy.h"
#include "models/balance_config.h"

int main(void) {
    printf("====================================================\n");
    printf("PRUEBA A4: CORRECCIONES DE COMBATE (RONDA 1 Y LOOT)\n");
    printf("====================================================\n");

    BalanceConfig cfg;
    Balance_InitDefaults(&cfg);

    General gen = General_Create("Comandante Test", 30, 2, 2, 2, 2, 2);
    Army army = Army_Create((HexCoord){0, 0}, gen, 5, 20, 10, 200.0f, 200.0f, 100, 0);

    EnemySquad enemy = Enemy_Create("Guardia Test", (HexCoord){1, 0}, 40, 5, 0, 20, 5);

    CombatContext ctx;
    memset(&ctx, 0, sizeof(CombatContext));
    Combat_Init(&ctx, NULL, &army, &enemy);

    printf("[1] Verificando estado en Ronda 1...\n");
    printf("    Ronda: %d (Esperado: 1)\n", ctx.round);
    assert(ctx.round == 1);

    printf("    previousPlan: %d (Esperado: PLAN_NONE = %d)\n", ctx.previousPlan, PLAN_NONE);
    assert(ctx.previousPlan == PLAN_NONE);

    /* Verificar que ningun plan este fatigado en ronda 1 */
    assert(ctx.previousPlan != PLAN_ASSAULT);
    assert(ctx.previousPlan != PLAN_SKIRMISH);
    assert(ctx.previousPlan != PLAN_PIKES);
    assert(ctx.previousPlan != PLAN_FLANK);
    printf("-> [OK] Ningun plan esta fatigado en Ronda 1\n\n");

    printf("[2] Verificando multiplicador de botin en contexto de combate...\n");
    printf("    lootMultiplier inicial: %.1f (Esperado: 1.0)\n", ctx.lootMultiplier);
    assert(ctx.lootMultiplier == 1.0f);

    Rng rng;
    Rng_Init(&rng, 9999ULL);
    Combat_ComputeLoot(&ctx, &rng);
    int baseGold = ctx.lootGold;
    int baseFood = (int)ctx.lootFood;
    printf("    Botin base (mult 1.0): Oro=%d, Comida=%d\n", baseGold, baseFood);

    /* Probar multiplicador de asedio (x3.0) */
    ctx.lootMultiplier = 3.0f;
    Rng_Init(&rng, 9999ULL); // Misma semilla
    Combat_ComputeLoot(&ctx, &rng);
    printf("    Botin asedio (mult 3.0): Oro=%d, Comida=%d\n", ctx.lootGold, (int)ctx.lootFood);
    assert(ctx.lootGold == baseGold * 3);
    assert(abs((int)ctx.lootFood - baseFood * 3) <= 1);
    printf("-> [OK] Multiplicador de botin integrado en CombatContext y Combat_ComputeLoot\n\n");

    printf(">>> PRUEBA A4 PASO AL 100%% [OK] <<<\n");
    return 0;
}
