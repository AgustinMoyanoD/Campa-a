#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "models/combat.h"
#include "models/army.h"
#include "models/enemy.h"

// Stubs for game master / modifier
struct GameMaster { ModifierRegistry modRegistry; };
ModifierDef* Modifier_GetById(const ModifierDef* defs, int count, const char* id) { return NULL; }
int Modifier_Query(ModifierHook hook, EffectType effectType, int side, ModContext* ctx, int baseValue, ModBreakdown* breakdown) { return baseValue; }
int Modifier_CollectActions(ModifierHook hook, const ModContext* ctx, ModAction* actionsOut, int maxActions) { return 0; }
bool Enemy_IsTagKnown(const EnemySquad* e, EnemyTag t) { return false; }
bool Enemy_HasTag(const EnemySquad* e, EnemyTag t) { return false; }

int main() {
    struct GameMaster gm = {0};
    Army army = {0};
    army.iniciados = 10;
    army.soldados = 10;
    army.sargentos = 2;
    army.salud = 100.0f;
    army.moral = 100.0f;
    
    EnemySquad enemy = {0};
    enemy.baseStrength = 100;
    enemy.maxStrength = 100;
    enemy.isActive = true;
    
    int initialWeight = Army_GetTotalWeight(&army);
    
    CombatContext ctx;
    Combat_Init(&ctx, &gm, &army, &enemy);
    assert(ctx.state == COMBAT_STATE_EVALUATION);
    assert(ctx.playerInitialWeight == initialWeight);
    
    Combat_RollEvaluation(&ctx, 3, 4);
    assert(ctx.state == COMBAT_STATE_PLANNING);
    
    Combat_RollPlanning(&ctx, 4, 4);
    assert(ctx.state == COMBAT_STATE_EXECUTION);
    
    Combat_SelectPlan(&ctx, PLAN_SKIRMISH);
    
    Combat_RollExecution(&ctx, 6, 6);
    assert(ctx.state == COMBAT_STATE_ROUND_CLOSE);
    
    Combat_RoundClose(&ctx);
    // Vuelve a planificacion porque el enemigo/nosotros no morimos (margin pequeno)
    assert(ctx.state == COMBAT_STATE_PLANNING || ctx.state == COMBAT_STATE_END);
    
    // Testear que el ejercito original no se modifico antes del commit (I2)
    assert(army.iniciados == 10);
    assert(army.soldados == 10);
    assert(army.sargentos == 2);
    assert(army.salud == 100.0f);
    
    printf("All combat tests passed!\n");
    return 0;
}
