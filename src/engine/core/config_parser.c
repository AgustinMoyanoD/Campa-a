#include "engine/core/config_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void Trim(char* str) {
    if (!str) return;
    int len = (int)strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = 0;
    }
    char* start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

void Config_Init(ConfigFile* cfg) {
    if (!cfg) return;
    memset(cfg, 0, sizeof(ConfigFile));
}

bool Config_LoadFile(ConfigFile* cfg, const char* filepath) {
    if (!cfg || !filepath) return false;
    Config_Init(cfg);
    strncpy(cfg->loadedPath, filepath, sizeof(cfg->loadedPath) - 1);

    FILE* f = fopen(filepath, "r");
    if (!f) return false;

    char line[512];
    char currentSection[CONFIG_STR_LEN] = "GLOBAL";

    while (fgets(line, sizeof(line), f)) {
        // Remover comentario
        char* comment = strchr(line, ';');
        if (comment) *comment = 0;
        char* commentHash = strchr(line, '#');
        if (commentHash) *commentHash = 0;

        Trim(line);
        if (line[0] == 0) continue;

        // Sección: [SECTION]
        if (line[0] == '[' && line[strlen(line) - 1] == ']') {
            line[strlen(line) - 1] = 0;
            strncpy(currentSection, line + 1, sizeof(currentSection) - 1);
            Trim(currentSection);
            continue;
        }

        // Par Clave = Valor
        char* eq = strchr(line, '=');
        if (eq && cfg->count < CONFIG_MAX_ENTRIES) {
            *eq = 0;
            char* k = line;
            char* v = eq + 1;
            Trim(k);
            Trim(v);

            if (k[0] != 0) {
                ConfigEntry* entry = &cfg->entries[cfg->count++];
                strncpy(entry->section, currentSection, sizeof(entry->section) - 1);
                strncpy(entry->key, k, sizeof(entry->key) - 1);
                strncpy(entry->value, v, sizeof(entry->value) - 1);
            }
        }
    }

    fclose(f);
    return true;
}

bool Config_LoadString(ConfigFile* cfg, const char* content) {
    if (!cfg || !content) return false;
    Config_Init(cfg);

    char currentSection[CONFIG_STR_LEN] = "GLOBAL";
    const char* p = content;

    while (*p) {
        char line[512];
        int idx = 0;
        while (*p && *p != '\n' && *p != '\r' && idx < 510) {
            line[idx++] = *p++;
        }
        line[idx] = 0;
        if (*p == '\r') p++;
        if (*p == '\n') p++;

        char* comment = strchr(line, ';');
        if (comment) *comment = 0;
        Trim(line);
        if (line[0] == 0) continue;

        if (line[0] == '[' && line[strlen(line) - 1] == ']') {
            line[strlen(line) - 1] = 0;
            strncpy(currentSection, line + 1, sizeof(currentSection) - 1);
            Trim(currentSection);
            continue;
        }

        char* eq = strchr(line, '=');
        if (eq && cfg->count < CONFIG_MAX_ENTRIES) {
            *eq = 0;
            char* k = line;
            char* v = eq + 1;
            Trim(k);
            Trim(v);
            if (k[0] != 0) {
                ConfigEntry* entry = &cfg->entries[cfg->count++];
                strncpy(entry->section, currentSection, sizeof(entry->section) - 1);
                strncpy(entry->key, k, sizeof(entry->key) - 1);
                strncpy(entry->value, v, sizeof(entry->value) - 1);
            }
        }
    }
    return true;
}

const char* Config_GetString(const ConfigFile* cfg, const char* section, const char* key, const char* defaultVal) {
    if (!cfg || !key) return defaultVal;
    const char* sec = (section && section[0]) ? section : "GLOBAL";

    for (int i = 0; i < cfg->count; i++) {
        if (strcmp(cfg->entries[i].section, sec) == 0 && strcmp(cfg->entries[i].key, key) == 0) {
            return cfg->entries[i].value;
        }
    }
    return defaultVal;
}

int Config_GetInt(const ConfigFile* cfg, const char* section, const char* key, int defaultVal) {
    const char* str = Config_GetString(cfg, section, key, NULL);
    if (!str) return defaultVal;
    return atoi(str);
}

float Config_GetFloat(const ConfigFile* cfg, const char* section, const char* key, float defaultVal) {
    const char* str = Config_GetString(cfg, section, key, NULL);
    if (!str) return defaultVal;
    return (float)atof(str);
}

bool Config_GetBool(const ConfigFile* cfg, const char* section, const char* key, bool defaultVal) {
    const char* str = Config_GetString(cfg, section, key, NULL);
    if (!str) return defaultVal;
    if (strcmp(str, "true") == 0 || strcmp(str, "TRUE") == 0 || strcmp(str, "1") == 0 || strcmp(str, "yes") == 0) {
        return true;
    }
    if (strcmp(str, "false") == 0 || strcmp(str, "FALSE") == 0 || strcmp(str, "0") == 0 || strcmp(str, "no") == 0) {
        return false;
    }
    return defaultVal;
}

bool Config_HasKey(const ConfigFile* cfg, const char* section, const char* key) {
    return Config_GetString(cfg, section, key, NULL) != NULL;
}

bool Config_GetSubValue(const char* compositeStr, const char* subKey, char* outValue, int maxOutLen) {
    if (!compositeStr || !subKey || !outValue || maxOutLen <= 0) return false;
    outValue[0] = 0;

    int keyLen = (int)strlen(subKey);
    const char* p = compositeStr;

    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        // Comprobar si empieza por subKey:
        if (strncmp(p, subKey, keyLen) == 0 && p[keyLen] == ':') {
            const char* valStart = p + keyLen + 1;
            const char* valEnd = valStart;
            while (*valEnd && !isspace((unsigned char)*valEnd)) valEnd++;

            int len = (int)(valEnd - valStart);
            if (len >= maxOutLen) len = maxOutLen - 1;
            strncpy(outValue, valStart, len);
            outValue[len] = 0;
            return true;
        }

        // Avanzar a la siguiente palabra
        while (*p && !isspace((unsigned char)*p)) p++;
    }
    return false;
}

int Config_GetSubInt(const char* compositeStr, const char* subKey, int defaultVal) {
    char buf[64];
    if (Config_GetSubValue(compositeStr, subKey, buf, sizeof(buf))) {
        return atoi(buf);
    }
    return defaultVal;
}

float Config_GetSubFloat(const char* compositeStr, const char* subKey, float defaultVal) {
    char buf[64];
    if (Config_GetSubValue(compositeStr, subKey, buf, sizeof(buf))) {
        return (float)atof(buf);
    }
    return defaultVal;
}
