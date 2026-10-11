#ifndef EVENT_H
#define EVENT_H

#include <stdbool.h>
#include "models/effect.h"

#define MAX_OPTIONS 5
#define MAX_TEXT_LEN 256


typedef enum {
    TRIGGER_NONE,
    TRIGGER_RANDOM,
    TRIGGER_ROAD,
    TRIGGER_BATTLE_START,
    TRIGGER_BATTLE_END,
    TRIGGER_CITY,
    TRIGGER_MARKET,
    TRIGGER_HOUSE,
    TRIGGER_CASTLE,
    TRIGGER_ARMORY,
    TRIGGER_NIGHT,
    TRIGGER_CAMP_AMBUSH
} EventTrigger;



typedef struct {
    char id[32];
    char name[64];
    char description[MAX_TEXT_LEN];
    
    EffectDef effects[MAX_EFFECTS];
    int effectCount;
    
    bool startsBattle;
    char battleEnemy[32];
} EventResult;

typedef struct {
    char text[MAX_TEXT_LEN];
    bool startsBattle;
    
    int resultPoolCount;
    char resultPool[5][32]; // IDs de los posibles resultados

    char requiresCondition[64];
    bool enabled;
    char disabledReason[64];
} EventOption;

typedef struct {
    char id[32];
    char name[64];
    char description[MAX_TEXT_LEN];
    
    bool hasTriggerKey;
    bool triggers[16]; // Mapeado a EventTrigger
    
    bool hasTimeKey;
    bool timesOfDay[4]; // MORNING=0, MIDDAY=1, AFTERNOON=2, NIGHT=3
    
    bool hasBiomeKey;
    bool biomes[8]; // Mapeado a BiomeType (0 a 5)

    int probability;
    float minInfamy;
    float maxInfamy;
    int minGold;
    int minFood;
    int minWater;
    int minArmySize;
    int minCattle;
    int daysWithoutFoodMin;
    int daysWithoutWaterMin;

    int optionCount;
    EventOption options[MAX_OPTIONS];
} EventDef;

#include "../models/army.h"

// Carga todos los archivos .evt de un directorio
int Event_LoadAll(const char* directory, EventDef* outEvents, int maxEvents);
void Event_EvaluateOptions(EventDef* event, const Army* army);

int Result_LoadAll(const char* directory, EventResult* outResults, int maxResults);

void Event_ApplyResult(const EventResult* res, Army* army, float* gmInfamy);

#endif /* EVENT_H */


