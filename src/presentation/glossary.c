#include "presentation/glossary.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static GlossaryEntry s_entries[GLOSSARY_MAX_ENTRIES];
static int s_entryCount = 0;
static bool s_initialized = false;

/* Lista de terminos obligatorios exigidos por la especificacion (Sec B1) */
static const char* const MANDATORY_TERMS[] = {
    "TIRADA",
    "PUNTAJE",
    "CHOQUE",
    "MARGEN",
    "INTENCION",
    "FATIGADO",
    "EXPUESTO",
    "FORTIFICADO",
    "DESORGANIZADOS",
    "CABALLERIA",
    "MANDO",
    "FUERA_DE_COMBATE",
    "VENTAJA",
    "DESVENTAJA",
    "MORAL",
    "SALUD",
    "RACIONES",
    "INFAMIA",
    "LEALTAD"
};
static const int MANDATORY_TERM_COUNT = sizeof(MANDATORY_TERMS) / sizeof(MANDATORY_TERMS[0]);

static void NormalizeString(const char* src, char* dst, size_t dstSize) {
    size_t i = 0;
    while (*src && i < dstSize - 1) {
        dst[i++] = (char)toupper((unsigned char)*src++);
    }
    dst[i] = '\0';
}

static char* Trim(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

bool Glossary_Init(const char* filepath) {
    Glossary_Shutdown();
    
    const char* path = filepath ? filepath : "assets/glossary.cfg";
    FILE* f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "[GLOSSARY ERROR] No se pudo abrir el archivo de glosario: %s\n", path);
        return false;
    }

    char line[512];
    GlossaryEntry current = {0};
    bool inSection = false;

    while (fgets(line, sizeof(line), f)) {
        char* trimmed = Trim(line);
        if (trimmed[0] == '\0' || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        if (trimmed[0] == '[' && trimmed[strlen(trimmed) - 1] == ']') {
            if (inSection && current.id[0] != '\0' && s_entryCount < GLOSSARY_MAX_ENTRIES) {
                s_entries[s_entryCount++] = current;
            }
            memset(&current, 0, sizeof(current));
            trimmed[strlen(trimmed) - 1] = '\0';
            char* rawId = Trim(trimmed + 1);
            NormalizeString(rawId, current.id, sizeof(current.id));
            inSection = true;
            continue;
        }

        if (!inSection) continue;

        char* eq = strchr(trimmed, '=');
        if (!eq) continue;
        *eq = '\0';
        char* key = Trim(trimmed);
        char* val = Trim(eq + 1);

        char keyUpper[32];
        NormalizeString(key, keyUpper, sizeof(keyUpper));

        if (strcmp(keyUpper, "TITLE") == 0) {
            strncpy(current.title, val, sizeof(current.title) - 1);
            current.title[sizeof(current.title) - 1] = '\0';
        } else if (strcmp(keyUpper, "TEXT") == 0) {
            strncpy(current.text, val, sizeof(current.text) - 1);
            current.text[sizeof(current.text) - 1] = '\0';
        }
    }

    if (inSection && current.id[0] != '\0' && s_entryCount < GLOSSARY_MAX_ENTRIES) {
        s_entries[s_entryCount++] = current;
    }

    fclose(f);
    s_initialized = true;
    return true;
}

void Glossary_Shutdown(void) {
    memset(s_entries, 0, sizeof(s_entries));
    s_entryCount = 0;
    s_initialized = false;
}

const GlossaryEntry* Glossary_Get(const char* id) {
    if (!s_initialized || !id) return NULL;
    char normId[GLOSSARY_ID_LEN];
    NormalizeString(id, normId, sizeof(normId));

    for (int i = 0; i < s_entryCount; i++) {
        if (strcmp(s_entries[i].id, normId) == 0) {
            return &s_entries[i];
        }
    }
    return NULL;
}

bool Glossary_Has(const char* id) {
    return Glossary_Get(id) != NULL;
}

bool Glossary_ValidateAll(void) {
    if (!s_initialized) {
        fprintf(stderr, "[GLOSSARY ERROR] Glossary_ValidateAll llamado antes de inicializar.\n");
        return false;
    }

    bool allPresent = true;
    for (int i = 0; i < MANDATORY_TERM_COUNT; i++) {
        const char* term = MANDATORY_TERMS[i];
        if (!Glossary_Has(term)) {
            fprintf(stderr, "[GLOSSARY VALIDATION ERROR] Falta el termino obligatorio: '%s' en assets/glossary.cfg\n", term);
            allPresent = false;
        }
    }

    return allPresent;
}

int Glossary_GetCount(void) {
    return s_entryCount;
}
