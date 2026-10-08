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

static EventTrigger ParseTrigger(const char* str) {
    if (strstr(str, "RANDOM")) return TRIGGER_RANDOM;
    if (strstr(str, "ROAD")) return TRIGGER_ROAD;
    if (strstr(str, "BATTLE_START")) return TRIGGER_BATTLE_START;
    if (strstr(str, "BATTLE_END")) return TRIGGER_BATTLE_END;
    if (strstr(str, "CITY")) return TRIGGER_CITY;
    if (strstr(str, "MARKET")) return TRIGGER_MARKET;
    if (strstr(str, "HOUSE")) return TRIGGER_HOUSE;
    if (strstr(str, "CASTLE")) return TRIGGER_CASTLE;
    if (strstr(str, "ARMORY")) return TRIGGER_ARMORY;
    if (strstr(str, "NIGHT")) return TRIGGER_NIGHT;
    return TRIGGER_NONE;
}

static int ParseTime(const char* str) {
    if (strstr(str, "MORNING")) return 0;
    if (strstr(str, "MIDDAY")) return 1;
    if (strstr(str, "AFTERNOON")) return 2;
    if (strstr(str, "NIGHT")) return 3;
    return -1;
}

static bool LoadEventFromFile(const char* filepath, EventDef* ev) {
    FILE* f = fopen(filepath, "r");
    if (!f) return false;

    memset(ev, 0, sizeof(EventDef));
    ev->maxInfamy = 100.0f; /* Valor por defecto */
    ev->timeOfDay = -1; /* ANY por defecto */
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
                        if (val[j] == '\\' && val[j+1] == 'n') {
                            buffer[b_idx++] = '\n';
                            j++;
                        } else {
                            buffer[b_idx++] = val[j];
                        }
                    }
                    strncpy(ev->description, buffer, sizeof(ev->description)-1);
                }
                else if (strcmp(key, "Trigger") == 0) ev->trigger = ParseTrigger(val);
                else if (strcmp(key, "Probability") == 0) ev->probability = atoi(val);
                else if (strcmp(key, "MinInfamy") == 0) ev->minInfamy = (float)atof(val);
                else if (strcmp(key, "MaxInfamy") == 0) ev->maxInfamy = (float)atof(val);
                else if (strcmp(key, "Time") == 0) ev->timeOfDay = ParseTime(val);
                else if (strcmp(key, "MinGold") == 0) ev->minGold = atoi(val);
                else if (strcmp(key, "MinFood") == 0) ev->minFood = atoi(val);
                else if (strcmp(key, "MinWater") == 0) ev->minWater = atoi(val);
                else if (strcmp(key, "MinArmySize") == 0) ev->minArmySize = atoi(val);
            } else if (currentOption >= 0 && currentOption < MAX_OPTIONS) {
                EventOption* opt = &ev->options[currentOption];
                if (strcmp(key, "Text") == 0) strncpy(opt->text, val, sizeof(opt->text)-1);
                else if (strcmp(key, "Result_Gold") == 0) opt->resultGold = atoi(val);
                else if (strcmp(key, "Result_Infamy") == 0) opt->resultInfamy = (float)atof(val);
                else if (strcmp(key, "Result_Moral") == 0) opt->resultMoral = atoi(val);
                else if (strcmp(key, "Result_Battle") == 0) opt->startsBattle = true;
                else if (strcmp(key, "Result_pool") == 0) {
                    char* token = strtok(val, ",");
                    while (token && opt->resultPoolCount < 5) {
                        TrimWhitespace(token);
                        strncpy(opt->resultPool[opt->resultPoolCount++], token, 31);
                        token = strtok(NULL, ",");
                    }
                }
            }
        }
    }

    fclose(f);
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
            else if (strcmp(key, "Result_Gold") == 0) res->resultGold = atoi(val);
            else if (strcmp(key, "Result_Infamy") == 0) res->resultInfamy = (float)atof(val);
            else if (strcmp(key, "Result_Moral") == 0) res->resultMoral = atoi(val);
            else if (strcmp(key, "Result_Food") == 0) res->resultFood = (float)atof(val);
            else if (strcmp(key, "Result_Water") == 0) res->resultWater = (float)atof(val);
            else if (strcmp(key, "Loss_Initiates") == 0) res->lossInitiates = atoi(val);
            else if (strcmp(key, "Loss_Soldiers") == 0) res->lossSoldiers = atoi(val);
            else if (strcmp(key, "Loss_Sergeants") == 0) res->lossSergeants = atoi(val);
            else if (strcmp(key, "Result_Battle") == 0) {
                res->startsBattle = true;
                strncpy(res->battleEnemy, val, sizeof(res->battleEnemy)-1);
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
