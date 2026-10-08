#ifndef EVENT_H
#define EVENT_H

#include <stdbool.h>

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
    TRIGGER_NIGHT
} EventTrigger;

typedef struct {
    char id[32];
    char name[64];
    char description[MAX_TEXT_LEN];
    
    int resultGold;
    float resultInfamy;
    int resultMoral;
    float resultFood;
    float resultWater;
    
    int lossInitiates;
    int lossSoldiers;
    int lossSergeants;
    
    bool startsBattle;
    char battleEnemy[32];
} EventResult;

typedef struct {
    char text[MAX_TEXT_LEN];
    int resultGold;
    float resultInfamy;
    int resultMoral;
    bool startsBattle;
    
    int resultPoolCount;
    char resultPool[5][32]; // IDs de los posibles resultados
} EventOption;

typedef struct {
    char id[32];
    char name[64];
    char description[MAX_TEXT_LEN];
    EventTrigger trigger;
    int probability;
    float minInfamy;
    float maxInfamy;
    int timeOfDay;
    int minGold;
    int minFood;
    int minWater;
    int minArmySize;
    int optionCount;
    EventOption options[MAX_OPTIONS];
} EventDef;

// Carga todos los archivos .evt de un directorio
int Event_LoadAll(const char* directory, EventDef* outEvents, int maxEvents);

int Result_LoadAll(const char* directory, EventResult* outResults, int maxResults);

#endif /* EVENT_H */




