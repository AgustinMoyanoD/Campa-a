#ifndef EFFECT_H
#define EFFECT_H

#include <stdbool.h>

#define MAX_EFFECTS 16

typedef enum {
    EFFECT_GOLD = 0,
    EFFECT_INFAMY,
    EFFECT_MORAL,
    EFFECT_FOOD,
    EFFECT_WATER,
    EFFECT_INITIATES,
    EFFECT_SOLDIERS,
    EFFECT_SERGEANTS,
    EFFECT_CATTLE,
    EFFECT_SALUD,
    
    // Novedades Roguelike (a implementar luego)
    EFFECT_ADD_MOD,
    EFFECT_REMOVE_MOD,
    EFFECT_DRAW_MOD,
    
    // Efectos dinamicos para ganchos (Hooks)
    EFFECT_FLEE_COST,
    EFFECT_PA,
    EFFECT_UNLOCK_PLAN,
    EFFECT_CASUALTIES,
    EFFECT_VISION,
    EFFECT_EXECUTION,
    EFFECT_EVALUATION,
    EFFECT_PLANNING,
    EFFECT_MOVE_COST
} EffectType;

typedef enum {
    EFFECT_MODE_ABSOLUTE = 0,
    EFFECT_MODE_PERCENTAGE,
    EFFECT_MODE_ALL
} EffectMode;

typedef struct {
    char id[32];
    int remaining;
} ModifierInstance;

typedef enum {
    EFFECT_SIDE_SELF = 0,
    EFFECT_SIDE_OPPONENT
} EffectTargetSide;

typedef struct {
    EffectType type;
    EffectTargetSide side;
    EffectMode mode;
    float value; // Para ABSOLUTE y PERCENTAGE
    char stringValue[32]; // Para identificadores como "addmod:hermes_pie_alado" o "unlock_plan:FLEE"
} EffectDef;

// Declaracion de la funcion parseadora. Retorna true si pudo leer el efecto.
bool Effect_ParseString(const char* effectStr, EffectDef* outEffect);

#endif
