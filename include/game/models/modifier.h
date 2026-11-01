#ifndef MODIFIER_H
#define MODIFIER_H

#include "models/effect.h"
#include "models/army.h"
#include "models/enemy.h"

// MAXIMOS
#define MAX_MODIFIERS 128
#define MAX_RULES_PER_MODIFIER 8
#define MAX_EFFECTS_PER_RULE 4

typedef enum {
    MOD_KIND_CARD = 0,
    MOD_KIND_CURSE,
    MOD_KIND_TRAIT,
    MOD_KIND_EQUIPMENT
} ModifierKind;

typedef enum {
    HOOK_PASSIVE = 0,
    HOOK_COMBAT_START,
    HOOK_EVAL_ROLL,
    HOOK_EVAL_REVEAL,
    HOOK_PLAN_ROLL,
    HOOK_PLAN_OPTIONS,
    HOOK_FLEE_COST,
    HOOK_EXEC_ROLL,
    HOOK_EXEC_DAMAGE,
    HOOK_HEALTH_DAMAGE_RECEIVED,
    HOOK_RECOUNT_DEATH,
    HOOK_COMBAT_END,
    HOOK_MAP_BASE_PA,
    HOOK_MAP_MOVE_COST,
    HOOK_MAP_VISION,
    HOOK_DAILY_CONSUMPTION,
    HOOK_NIGHT_FALL,
    HOOK_NIGHT_PENALTY,
    HOOK_AMBUSH_ROLL,
    HOOK_NIGHT_ENCOUNTER_CHANCE,
    HOOK_EVENT_FILTER,
    HOOK_EVENT_RESULT,
    HOOK_DAY_START,
    HOOK_FORAGE_YIELD
} ModifierHook;

typedef enum { MOD_TARGET_COMMANDER = 0, MOD_TARGET_ARMY, MOD_TARGET_ENEMY } ModifierTarget;
typedef enum DurationType { DURATION_PERMANENT = 0, DURATION_BATTLES, DURATION_DAYS } DurationType;

typedef enum { COND_PLAN, COND_ENEMY_TAG, COND_KNOWN_TAG, COND_EQUIPPED_TAG,
               COND_MORAL, COND_INFAMY, COND_TIME, COND_EVAL_TIER, COND_ORIGIN } CondLhs;
typedef enum { OP_EQ, OP_NE, OP_GE, OP_LE, OP_GT, OP_LT } CondOp;

typedef struct { CondLhs lhs; char arg[16]; CondOp op; int rhsInt; char rhsStr[24]; } CondTerm;
typedef struct { CondTerm terms[4]; int termCount; } Condition;

typedef struct {
    ModifierHook hook;
    Condition condition;
    EffectDef effects[MAX_EFFECTS_PER_RULE];
    int effectCount;
} ModifierRule;

typedef struct ModifierDef {
    char id[32];
    char name[64];
    char description[160];
    ModifierKind kind;
    ModifierTarget target;
    char source[32];
    int  rarityWeight;
    char tags[8][16];
    int  tagCount;
    DurationType durationType;
    int  durationValue;
    ModifierRule rules[MAX_RULES_PER_MODIFIER];
    int  ruleCount;
    
    // Invocación activa como plan de combate
    bool hasInvocation;
    char invocationName[48];
    char invocationDesc[128];
    int  invocationRollMod;
    float invocationClashMult;
    int  invocationCounterIntent; // 0=CHARGE, 1=HOLD, 2=MANEUVER, -1=ANY
    bool invocationHighVariance;  // Varianza extrema
    char rivalGod[32];            // ID/Nombre del dios rival
} ModifierDef;

typedef struct {
    ModifierDef defs[MAX_MODIFIERS];
    int count;
} ModifierRegistry;

typedef enum { SIDE_PLAYER = 0, SIDE_ENEMY } ModSide;

typedef struct {
    const ModifierDef* def;
    ModSide owner;
} ActiveMod;

typedef struct {
    char source[48];
    int  flat;
    int  percent;
} ModBreakdownEntry;

typedef struct {
    int base;
    ModBreakdownEntry entries[12];
    int count;
    int final;
} ModBreakdown;

typedef struct GameMaster GameMaster;
typedef struct CombatContext CombatContext;



typedef struct {
    const ModifierRegistry* reg;
    const Army*      army;
    const EnemySquad* enemy;
    const CombatContext* combat;
    int timeOfDay;
    float infamy;
} ModContext;

typedef struct {
    EffectType type;
    char arg[32];
    int  value;
    const ModifierDef* source;
} ModAction;

int Modifier_LoadAll(const char* directory, ModifierDef* outMods, int maxMods);
ModifierDef* Modifier_GetById(const ModifierDef* registry, int registryCount, const char* id);

int Modifier_CollectActive(const ModifierRegistry* reg, const Army* army, const EnemySquad* enemy, ActiveMod* out, int max);
int Modifier_Query(ModifierHook hook, EffectType stat, ModSide side, const ModContext* ctx, int base, ModBreakdown* outOpt);
int Modifier_CollectActions(ModifierHook hook, const ModContext* ctx, ModAction* out, int max);
bool Modifier_CheckRivals(const ModifierRegistry* reg, const Army* army, char* outConflict, int maxLen);



#endif


