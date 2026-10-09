#include "models/event.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>

static void TrimWhitespace(char* str) {
    char *p = str;
    int l = strlen(p);
    while(l > 0 && isspace((unsigned char)p[l - 1])) p[--l] = 0;
    while(*p && isspace((unsigned char)*p)) ++p, --l;
    memmove(str, p, l + 1);
}


static void AddLegacyEffect(EventResult* res, EffectType type, float value) {
    if (value == 0.0f) return;
    if (res->effectCount >= MAX_EFFECTS) return;
    EffectDef* e = &res->effects[res->effectCount++];
    e->type = type;
    e->mode = EFFECT_MODE_ABSOLUTE;
    e->value = value;
}

static void ParseEffectString(EventResult* res, const char* val, const char* filepath, int lineNum) {
    if (res->effectCount >= MAX_EFFECTS) {
        printf("WARNING [%s:%d]: Limite de efectos alcanzado (MAX=%d)\n", filepath, lineNum, MAX_EFFECTS);
        return;
    }
    
    EffectDef tempEffect;
    if (Effect_ParseString(val, &tempEffect)) {
        res->effects[res->effectCount] = tempEffect;
        res->effectCount++;
    } else {
        printf("WARNING [%s:%d]: Error al procesar efecto '%s'\n", filepath, lineNum, val);
    }
}

static EventTrigger ParseTrigger(const char* str) {
    if (strcmp(str, "RANDOM") == 0) return TRIGGER_RANDOM;
    if (strcmp(str, "ROAD") == 0) return TRIGGER_ROAD;
    if (strcmp(str, "BATTLE_START") == 0) return TRIGGER_BATTLE_START;
    if (strcmp(str, "BATTLE_END") == 0) return TRIGGER_BATTLE_END;
    if (strcmp(str, "CITY") == 0) return TRIGGER_CITY;
    if (strcmp(str, "MARKET") == 0) return TRIGGER_MARKET;
    if (strcmp(str, "HOUSE") == 0) return TRIGGER_HOUSE;
    if (strcmp(str, "CASTLE") == 0) return TRIGGER_CASTLE;
    if (strcmp(str, "ARMORY") == 0) return TRIGGER_ARMORY;
    if (strcmp(str, "NIGHT") == 0) return TRIGGER_NIGHT;
    return TRIGGER_NONE;
}

static int ParseTime(const char* str) {
    if (strcmp(str, "MORNING") == 0) return 0;
    if (strcmp(str, "MIDDAY") == 0) return 1;
    if (strcmp(str, "AFTERNOON") == 0) return 2;
    if (strcmp(str, "NIGHT") == 0) return 3;
    return -1;
}

static int ParseBiome(const char* str) {
    if (strcmp(str, "PLAINS") == 0) return 0;
    if (strcmp(str, "FOREST") == 0) return 1;
    if (strcmp(str, "DESERT") == 0) return 2;
    if (strcmp(str, "JUNGLE") == 0) return 3;
    if (strcmp(str, "MOUNTAIN") == 0) return 4;
    if (strcmp(str, "WATER") == 0) return 5;
    return -1;
}

static bool ParseCommaList(bool* arrayDest, bool* hasKey, const char* val, const char* filepath, int type) {
    *hasKey = true;
    char buffer[256]; strncpy(buffer, val, sizeof(buffer)-1);
    char* token = strtok(buffer, ",");
    bool foundValid = false;
    while (token) {
        TrimWhitespace(token);
        if (strlen(token) > 0) {
            int idx = -1;
            if (type == 0) idx = ParseTrigger(token);
            else if (type == 1) idx = ParseTime(token);
            else if (type == 2) idx = ParseBiome(token);
            
            if (idx == -1 || (type == 0 && idx == 0 && strcmp(token, "NONE") != 0)) {
                printf("WARNING [%s]: Token invalido '%s'. Descartando evento.\n", filepath, token); return false;
            } else {
                arrayDest[idx] = true;
                foundValid = true;
            }
        }
        token = strtok(NULL, ",");
    }
    return foundValid;
}

static bool LoadEventFromFile(const char* filepath, EventDef* ev) {
    FILE* f = fopen(filepath, "r");
    if (!f) return false;

    memset(ev, 0, sizeof(EventDef));
    ev->maxInfamy = 100.0f; /* Valor por defecto */
    char line[512];
    int currentOption = -1;

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        char* start = line;
        if ((unsigned char)start[0] == 0xEF && (unsigned char)start[1] == 0xBB && (unsigned char)start[2] == 0xBF) {
            start += 3;
        }

        if (strlen(start) == 0 || start[0] == '#') continue;

        if (strncmp(start, "[EVENT]", 7) == 0) {
            currentOption = -1;
            continue;
        }
        if (strncmp(start, "[OPTION_", 8) == 0) {
            currentOption++;
            if (currentOption < MAX_OPTIONS) {
                ev->optionCount = currentOption + 1;
            }
            continue;
        }

        char* sep = strchr(start, '=');
        if (sep) {
            *sep = '\0';
            char* key = start;
            char* val = sep + 1;
            TrimWhitespace(key);
            TrimWhitespace(val);

            if (currentOption == -1) {
                if (strcmp(key, "Id") == 0) strncpy(ev->id, val, sizeof(ev->id)-1);
                else if (strcmp(key, "Name") == 0) strncpy(ev->name, val, sizeof(ev->name)-1);
                else if (strcmp(key, "Description") == 0) {
                    char buffer[MAX_TEXT_LEN] = {0};
                    int b_idx = 0;
                    for (int j=0; val[j] && b_idx < MAX_TEXT_LEN - 1; j++) {
                        if (val[j] == '\\' && val[j+1] == 'n') { buffer[b_idx++] = '\n'; j++; }
                        else { buffer[b_idx++] = val[j]; }
                    }
                    strncpy(ev->description, buffer, sizeof(ev->description)-1);
                }
                else if (strcmp(key, "Trigger") == 0) { if (!ParseCommaList(ev->triggers, &ev->hasTriggerKey, val, filepath, 0)) return false; }
                else if (strcmp(key, "Time") == 0) { if (!ParseCommaList(ev->timesOfDay, &ev->hasTimeKey, val, filepath, 1)) return false; }
                else if (strcmp(key, "Biome") == 0) { if (!ParseCommaList(ev->biomes, &ev->hasBiomeKey, val, filepath, 2)) return false; }
                else if (strcmp(key, "Probability") == 0) ev->probability = atoi(val);
                else if (strcmp(key, "MinInfamy") == 0) ev->minInfamy = (float)atof(val);
                else if (strcmp(key, "MaxInfamy") == 0) ev->maxInfamy = (float)atof(val);
                else if (strcmp(key, "MinGold") == 0) ev->minGold = atoi(val);
                else if (strcmp(key, "MinFood") == 0) ev->minFood = atoi(val);
                else if (strcmp(key, "MinWater") == 0) ev->minWater = atoi(val);
                else if (strcmp(key, "MinArmySize") == 0) ev->minArmySize = atoi(val);
            } else if (currentOption >= 0 && currentOption < MAX_OPTIONS) {
                EventOption* opt = &ev->options[currentOption];
                if (strcmp(key, "Text") == 0) strncpy(opt->text, val, sizeof(opt->text)-1);
                else if (strcmp(key, "Result_Battle") == 0) opt->startsBattle = true;
                else if (strcmp(key, "Result_pool") == 0) {
                    if (strcmp(val, "NONE") == 0) {
                        opt->resultPoolCount = 0;
                    } else {
                        char* token = strtok(val, ",");
                        while (token && opt->resultPoolCount < 5) {
                            TrimWhitespace(token);
                            if (strlen(token) > 0) {
                                strncpy(opt->resultPool[opt->resultPoolCount++], token, 31);
                            }
                            token = strtok(NULL, ",");
                        }
                    }
                }
                else if (strncmp(key, "Result_", 7) == 0) {
                    printf("WARNING [%s]: Clave legacy %s en opcion. Usar .res\n", filepath, key);
                }
            }
        }
    }
    fclose(f);
    
    // Validar opciones sin Result_pool
    for (int i=0; i<ev->optionCount; i++) {
        if (ev->options[i].resultPoolCount == 0) {
            printf("WARNING [%s]: Option %d no tiene Result_pool o es NONE.\n", filepath, i+1);
        }
    }

    return true;
}

int Event_LoadAll(const char* directory, EventDef* outEvents, int maxEvents) {
    DIR* dir = opendir(directory);
    if (!dir) return 0;

    struct dirent* entry;
    int count = 0;

    while ((entry = readdir(dir)) != NULL && count < maxEvents) {
        if (strstr(entry->d_name, ".evt")) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", directory, entry->d_name);
            
            if (LoadEventFromFile(filepath, &outEvents[count])) {
                count++;
            }
        }
    }

    closedir(dir);
    return count;
}

static int LoadResultsFromFile(const char* filepath, EventResult* outResults, int maxResults, int currentCount) {
    FILE* f = fopen(filepath, "r");
    if (!f) return currentCount;

    char line[512];
    EventResult* res = NULL;

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        char* start = line;
        if ((unsigned char)start[0] == 0xEF && (unsigned char)start[1] == 0xBB && (unsigned char)start[2] == 0xBF) {
            start += 3;
        }

        if (strlen(start) == 0 || start[0] == '#') continue;

        if (strncmp(start, "[RESULT]", 8) == 0) {
            if (currentCount < maxResults) {
                res = &outResults[currentCount];
                memset(res, 0, sizeof(EventResult));
                currentCount++;
            } else {
                res = NULL;
            }
            continue;
        }

        if (!res) continue;

        char* sep = strchr(start, '=');
        if (sep) {
            *sep = '\0';
            char* key = start;
            char* val = sep + 1;
            TrimWhitespace(key);
            TrimWhitespace(val);

            if (strcmp(key, "Id") == 0) strncpy(res->id, val, sizeof(res->id)-1);
            else if (strcmp(key, "Name") == 0) strncpy(res->name, val, sizeof(res->name)-1);
            else if (strcmp(key, "Description") == 0) {
                char buffer[MAX_TEXT_LEN] = {0};
                int b_idx = 0;
                for (int j = 0; val[j] && b_idx < MAX_TEXT_LEN - 1; j++) {
                    if (val[j] == '\\' && val[j+1] == 'n') {
                        buffer[b_idx++] = '\n';
                        j++;
                    } else {
                        buffer[b_idx++] = val[j];
                    }
                }
                strncpy(res->description, buffer, sizeof(res->description)-1);
            }
            else if (strcmp(key, "Effect") == 0) ParseEffectString(res, val, filepath, 0);
            else if (strcmp(key, "Result_Battle") == 0) {
                res->startsBattle = true;
                strncpy(res->battleEnemy, val, sizeof(res->battleEnemy)-1);
            }
            // --- BACKWARDS COMPATIBILITY ---
            else if (strncmp(key, "Result_", 7) == 0 || strncmp(key, "Loss_", 5) == 0) {
                printf("WARNING: Archivo deprecado usa claves legacy '%s' en '%s'. Usar Effect=stat:delta en su lugar.\n", key, filepath);
                if (strcmp(key, "Result_Gold") == 0) AddLegacyEffect(res, EFFECT_GOLD, (float)atof(val));
                else if (strcmp(key, "Result_Infamy") == 0) AddLegacyEffect(res, EFFECT_INFAMY, (float)atof(val));
                else if (strcmp(key, "Result_Moral") == 0) AddLegacyEffect(res, EFFECT_MORAL, (float)atof(val));
                else if (strcmp(key, "Result_Food") == 0) AddLegacyEffect(res, EFFECT_FOOD, (float)atof(val));
                else if (strcmp(key, "Result_Water") == 0) AddLegacyEffect(res, EFFECT_WATER, (float)atof(val));
                else if (strcmp(key, "Loss_Initiates") == 0) AddLegacyEffect(res, EFFECT_INITIATES, -(float)atof(val));
                else if (strcmp(key, "Loss_Soldiers") == 0) AddLegacyEffect(res, EFFECT_SOLDIERS, -(float)atof(val));
                else if (strcmp(key, "Loss_Sergeants") == 0) AddLegacyEffect(res, EFFECT_SERGEANTS, -(float)atof(val));
            }
        }
    }
    fclose(f);
    return currentCount;
}

int Result_LoadAll(const char* directory, EventResult* outResults, int maxResults) {
    int count = 0;
    DIR* dir = opendir(directory);
    if (!dir) return 0;
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL && count < maxResults) {
        if (strstr(ent->d_name, ".res") || strstr(ent->d_name, ".evt")) {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", directory, ent->d_name);
            count = LoadResultsFromFile(path, outResults, maxResults, count);
        }
    }
    closedir(dir);
    return count;
}

#include "../models/army.h"
#include <stdio.h>
#include <math.h>

void Event_ApplyResult(const EventResult* res, Army* army, float* gmInfamy) {
    if (!res || !army || !gmInfamy) return;
    
    for (int i = 0; i < res->effectCount; i++) {
        const EffectDef* eff = &res->effects[i];
        
        switch (eff->type) {
            case EFFECT_GOLD:
                army->monedas += (int)eff->value;
                if (army->monedas < 0) army->monedas = 0;
                break;
            case EFFECT_INFAMY:
                *gmInfamy += eff->value;
                if (*gmInfamy < 0.0f) *gmInfamy = 0.0f;
                if (*gmInfamy > 100.0f) *gmInfamy = 100.0f;
                break;
            case EFFECT_MORAL:
                army->moral += eff->value;
                if (army->moral < 0.0f) army->moral = 0.0f;
                if (army->moral > 100.0f) army->moral = 100.0f;
                break;
            case EFFECT_FOOD:
                army->comida_disponible += eff->value;
                if (army->comida_disponible < 0.0f) army->comida_disponible = 0.0f;
                break;
            case EFFECT_WATER:
                army->agua_disponible += eff->value;
                if (army->agua_disponible < 0.0f) army->agua_disponible = 0.0f;
                break;
            case EFFECT_INITIATES: {
                if (eff->mode == EFFECT_MODE_ALL) army->iniciados = 0;
                else if (eff->mode == EFFECT_MODE_PERCENTAGE) {
                    army->iniciados += (int)roundf(army->iniciados * (eff->value / 100.0f));
                } else {
                    army->iniciados += (int)eff->value;
                }
                if (army->iniciados < 0) army->iniciados = 0;
                break;
            }
            case EFFECT_SOLDIERS: {
                if (eff->mode == EFFECT_MODE_ALL) army->soldados = 0;
                else if (eff->mode == EFFECT_MODE_PERCENTAGE) {
                    army->soldados += (int)roundf(army->soldados * (eff->value / 100.0f));
                } else {
                    army->soldados += (int)eff->value;
                }
                if (army->soldados < 0) army->soldados = 0;
                break;
            }
            case EFFECT_SERGEANTS: {
                if (eff->mode == EFFECT_MODE_ALL) army->sargentos = 0;
                else if (eff->mode == EFFECT_MODE_PERCENTAGE) {
                    army->sargentos += (int)roundf(army->sargentos * (eff->value / 100.0f));
                } else {
                    army->sargentos += (int)eff->value;
                }
                if (army->sargentos < 0) army->sargentos = 0;
                break;
            }
        }
    }
}









