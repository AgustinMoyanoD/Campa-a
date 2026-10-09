#ifndef COMBAT_LOG_H
#define COMBAT_LOG_H

typedef enum {
    STEP_ROLL, 
    STEP_MOD_TRIGGERED, 
    STEP_TAG_REVEALED, 
    STEP_PLANS_UNLOCKED,
    STEP_ADVANTAGE, 
    STEP_CASUALTIES, 
    STEP_MORALE, 
    STEP_ROUND_END,
    STEP_COMBAT_END
} StepType;

typedef struct {
    StepType type;
    union {
        struct { int d1, d2, stat, modTotal, total; } roll;
        struct { char source[48]; int flat, percent; } mod; 
        struct { int side, initiates, soldiers, sergeants; } casualties;
        struct { int tagId; } tag;
        struct { int reason; } end;
        struct { int advantage; } advantage;
        struct { float moralDrop; } morale;
    } as;
} CombatStep;

#define MAX_STEPS 64

typedef struct {
    CombatStep steps[MAX_STEPS];
    int count;
} CombatLog;

#endif
