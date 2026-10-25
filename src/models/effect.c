#include "models/effect.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Auxiliar para limpiar espacios
static void Effect_TrimWhitespace(char* str) {
    if (!str) return;
    int len = strlen(str);
    while (len > 0 && (str[len-1] == ' ' || str[len-1] == '\r' || str[len-1] == '\n' || str[len-1] == '\t')) {
        str[len-1] = '\0';
        len--;
    }
    char* start = str;
    while (*start == ' ' || *start == '\t') start++;
    if (start != str) {
        memmove(str, start, len - (start - str) + 1);
    }
}

static EffectType ParseEffectType(const char* statStr) {
    if (strcmp(statStr, "gold") == 0) return EFFECT_GOLD;
    if (strcmp(statStr, "infamy") == 0) return EFFECT_INFAMY;
    if (strcmp(statStr, "moral") == 0) return EFFECT_MORAL;
    if (strcmp(statStr, "food") == 0) return EFFECT_FOOD;
    if (strcmp(statStr, "water") == 0) return EFFECT_WATER;
    if (strcmp(statStr, "initiates") == 0) return EFFECT_INITIATES;
    if (strcmp(statStr, "soldiers") == 0) return EFFECT_SOLDIERS;
    if (strcmp(statStr, "sergeants") == 0) return EFFECT_SERGEANTS;
    if (strcmp(statStr, "cattle") == 0) return EFFECT_CATTLE;
    if (strcmp(statStr, "salud") == 0) return EFFECT_SALUD;
    if (strcmp(statStr, "wine") == 0 || strcmp(statStr, "vino") == 0) return EFFECT_WINE;
    if (strcmp(statStr, "meat") == 0 || strcmp(statStr, "carne") == 0) return EFFECT_MEAT;
    if (strcmp(statStr, "group_moral") == 0) return EFFECT_GROUP_MORALE;
    if (strcmp(statStr, "group_salud") == 0) return EFFECT_GROUP_HEALTH;
    if (strcmp(statStr, "sargentos_satisfaction") == 0 || strcmp(statStr, "satisfaccion_sargentos") == 0) return EFFECT_SARGENTOS_SATISFACTION;
    
    // Nuevos
    if (strcmp(statStr, "addmod") == 0) return EFFECT_ADD_MOD;
    if (strcmp(statStr, "removemod") == 0) return EFFECT_REMOVE_MOD;
    if (strcmp(statStr, "drawmod") == 0) return EFFECT_DRAW_MOD;
    if (strcmp(statStr, "loyalty") == 0) return EFFECT_LOYALTY;
    if (strcmp(statStr, "flee_cost") == 0) return EFFECT_FLEE_COST;
    if (strcmp(statStr, "pa") == 0) return EFFECT_PA;
    if (strcmp(statStr, "unlock_plan") == 0) return EFFECT_UNLOCK_PLAN;
    if (strcmp(statStr, "casualties") == 0) return EFFECT_CASUALTIES;
    if (strcmp(statStr, "vision") == 0) return EFFECT_VISION;
    if (strcmp(statStr, "execution") == 0) return EFFECT_EXECUTION;
    if (strcmp(statStr, "evaluation") == 0) return EFFECT_EVALUATION;
    if (strcmp(statStr, "planning") == 0) return EFFECT_PLANNING;
    if (strcmp(statStr, "move_cost") == 0) return EFFECT_MOVE_COST;
    if (strcmp(statStr, "faction_opinion") == 0) return EFFECT_FACTION_OPINION;
    if (strcmp(statStr, "set_flag") == 0) return EFFECT_SET_FLAG;
    if (strcmp(statStr, "clear_flag") == 0) return EFFECT_CLEAR_FLAG;
    
    return (EffectType)-1;
}

bool Effect_ParseString(const char* effectStr, EffectDef* outEffect) {
    if (!effectStr || !outEffect) return false;
    
    char buffer[128] = {0};
    strncpy(buffer, effectStr, sizeof(buffer)-1);
    
    char* colon = strchr(buffer, ':');
    if (!colon) {
        printf("WARNING: Efecto mal formado '%s'\n", effectStr);
        return false;
    }
    
    *colon = '\0';
    char* statStr = buffer;
    char* deltaStr = colon + 1;
    
    Effect_TrimWhitespace(statStr);
    Effect_TrimWhitespace(deltaStr);
    
    outEffect->side = EFFECT_SIDE_SELF;
    if (strncmp(statStr, "enemy.", 6) == 0) {
        outEffect->side = EFFECT_SIDE_OPPONENT;
        statStr += 6;
    }
    
    EffectType type = ParseEffectType(statStr);
    if ((int)type == -1) {
        printf("WARNING: Estadistica o Hook desconocido '%s'\n", statStr);
        return false;
    }
    
    outEffect->type = type;
    memset(outEffect->stringValue, 0, sizeof(outEffect->stringValue));
    
    // Manejo de valores string (addmod, unlock_plan, set_flag, clear_flag, etc)
    if (type == EFFECT_ADD_MOD || type == EFFECT_REMOVE_MOD || type == EFFECT_DRAW_MOD || 
        type == EFFECT_UNLOCK_PLAN || type == EFFECT_SET_FLAG || type == EFFECT_CLEAR_FLAG) {
        outEffect->mode = EFFECT_MODE_ABSOLUTE;
        outEffect->value = 0.0f;
        strncpy(outEffect->stringValue, deltaStr, sizeof(outEffect->stringValue)-1);
        return true;
    }

    if (type == EFFECT_FACTION_OPINION) {
        outEffect->mode = EFFECT_MODE_ABSOLUTE;
        char* subColon = strchr(deltaStr, ':');
        if (subColon) {
            *subColon = '\0';
            char* targetStr = deltaStr;
            char* valStr = subColon + 1;
            Effect_TrimWhitespace(targetStr);
            Effect_TrimWhitespace(valStr);
            strncpy(outEffect->stringValue, targetStr, sizeof(outEffect->stringValue)-1);
            outEffect->value = (float)atof(valStr);
            return true;
        } else {
            printf("WARNING: Efecto faction_opinion mal formado '%s'. Uso esperado: faction_opinion:<target>:<delta>\n", deltaStr);
            return false;
        }
    }

    if (type == EFFECT_LOYALTY) {
        outEffect->mode = EFFECT_MODE_ABSOLUTE;
        char* subColon = strchr(deltaStr, ':');
        if (subColon) {
            *subColon = '\0';
            char* roleStr = deltaStr;
            char* valStr = subColon + 1;
            Effect_TrimWhitespace(roleStr);
            Effect_TrimWhitespace(valStr);
            strncpy(outEffect->stringValue, roleStr, sizeof(outEffect->stringValue)-1);
            outEffect->value = (float)atof(valStr);
        } else {
            outEffect->stringValue[0] = '\0';
            outEffect->value = (float)atof(deltaStr);
        }
        return true;
    }

    if (type == EFFECT_GROUP_MORALE || type == EFFECT_GROUP_HEALTH) {
        outEffect->mode = EFFECT_MODE_ABSOLUTE;
        char* subColon = strchr(deltaStr, ':');
        if (subColon) {
            *subColon = '\0';
            char* grpStr = deltaStr;
            char* valStr = subColon + 1;
            Effect_TrimWhitespace(grpStr);
            Effect_TrimWhitespace(valStr);
            strncpy(outEffect->stringValue, grpStr, sizeof(outEffect->stringValue)-1);
            outEffect->value = (float)atof(valStr);
            return true;
        } else {
            printf("WARNING: Efecto de grupo mal formado '%s'. Uso esperado: group_moral:<grupo>:<delta>\n", deltaStr);
            return false;
        }
    }
    
    // Manejo de modos tradicionales (ABSOLUTO, PORCENTAJE, ALL)
    if (strcmp(deltaStr, "-ALL") == 0 || strcmp(deltaStr, "ALL") == 0) {
        outEffect->mode = EFFECT_MODE_ALL;
        outEffect->value = 0.0f;
        if (type < EFFECT_INITIATES) {
            printf("WARNING: ALL solo es valido para tropas. Ignorando efecto '%s'.\n", effectStr);
            return false;
        }
    } else if (strchr(deltaStr, '%')) {
        outEffect->mode = EFFECT_MODE_PERCENTAGE;
        outEffect->value = (float)atof(deltaStr);
        // % is now valid for casualties, flee_cost, etc.
        // if (type < EFFECT_INITIATES) { ... } // Removed strict restriction for generic modifiers
    } else {
        outEffect->mode = EFFECT_MODE_ABSOLUTE;
        outEffect->value = (float)atof(deltaStr);
    }
    
    return true;
}
