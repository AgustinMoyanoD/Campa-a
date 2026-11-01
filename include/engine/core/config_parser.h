#ifndef ENGINE_CONFIG_PARSER_H
#define ENGINE_CONFIG_PARSER_H

#include <stdbool.h>

/* ============================================================
   ENGINE CONFIG PARSER: LECTOR GENÉRICO INI / CFG
   Carga y consulta configuraciones organizadas en secciones y claves.
   ============================================================ */

#define CONFIG_MAX_ENTRIES 512
#define CONFIG_STR_LEN     128
#define CONFIG_VAL_LEN     256

typedef struct {
    char section[CONFIG_STR_LEN];
    char key[CONFIG_STR_LEN];
    char value[CONFIG_VAL_LEN];
} ConfigEntry;

typedef struct {
    ConfigEntry entries[CONFIG_MAX_ENTRIES];
    int count;
    char loadedPath[CONFIG_STR_LEN];
} ConfigFile;

/* Carga y parseo desde archivo */
bool Config_LoadFile(ConfigFile* cfg, const char* filepath);
bool Config_LoadString(ConfigFile* cfg, const char* content);
void Config_Init(ConfigFile* cfg);

/* Consultas tipadas */
const char* Config_GetString(const ConfigFile* cfg, const char* section, const char* key, const char* defaultVal);
int         Config_GetInt(const ConfigFile* cfg, const char* section, const char* key, int defaultVal);
float       Config_GetFloat(const ConfigFile* cfg, const char* section, const char* key, float defaultVal);
bool        Config_GetBool(const ConfigFile* cfg, const char* section, const char* key, bool defaultVal);
bool        Config_HasKey(const ConfigFile* cfg, const char* section, const char* key);

/* Utilidad para valores compuestos delimitados por espacios: "food:1.5 water:0.5 stealth:1" */
bool Config_GetSubValue(const char* compositeStr, const char* subKey, char* outValue, int maxOutLen);
int  Config_GetSubInt(const char* compositeStr, const char* subKey, int defaultVal);
float Config_GetSubFloat(const char* compositeStr, const char* subKey, float defaultVal);

#endif /* ENGINE_CONFIG_PARSER_H */
