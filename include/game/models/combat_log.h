#ifndef COMBAT_LOG_H
#define COMBAT_LOG_H

#include "models/modifier.h"
#include "models/enemy.h"

typedef enum {
    STEP_DEPLOY = 0,
    STEP_ROLL,
    STEP_MOD_TRIGGERED,
    STEP_REVEAL,
    STEP_TAG_REVEALED,
    STEP_PLANS_UNLOCKED,
    STEP_PLAN_CHOSEN,
    STEP_ADVANTAGE,
    STEP_DAMAGE,
    STEP_OUT_OF_ACTION,
    STEP_ROUND_END,
    STEP_COMBAT_END,
    STEP_RECOUNT
} CombatStepType;

typedef struct {
    CombatStepType type;
    
    union {
        struct {
            int totalTroops;
            float initialHealth;
        } deploy;
        
        struct {
            int d1;
            int d2;
            int total;
        } roll;
        
        struct {
            char modifierId[32];
            int delta;
        } mod_trigger;
        
        struct {
            int infoLevel;
        } reveal;
        
        struct {
            EnemyTag tag;
        } tag_reveal;
        
        struct {
            unsigned int unlockedMask;
        } plans;
        
        struct {
            int plan;
        } plan_chosen;
        
        struct {
            int advantage;
            int playerForce;
            int enemyForce;
        } advantage;
        
        struct {
            int side; // 0=player, 1=enemy
            float lostHealth;
            float newHealth;
        } damage;
        
        struct {
            int side; // 0=player, 1=enemy
            int countInitiates;
            int countSoldiers;
            int countSergeants;
        } out_of_action;
        
        struct {
            int round;
        } round_end;
        
        struct {
            int reason;
        } combat_end;
        
        struct {
            int rank; // 0=Initiate, 1=Soldier, 2=Sergeant
            int outOfAction;
            int died;
            int survived;
        } recount;
        
    } as;
    
} CombatStep;

#define MAX_STEPS 64

typedef struct {
    CombatStep steps[MAX_STEPS];
    int count;
} CombatLog;

#endif
