#ifndef GENERAL_H
#define GENERAL_H

#include <stdbool.h>
#include "models/effect.h"

#define MAX_GENERAL_DIVINE_CARDS 3
#define MAX_LIEUTENANT_DIVINE_CARDS 1

/* ============================================================
   TIPO DE OFICIAL AL MANDO
   ============================================================ */

typedef enum {
    OFFICER_GENERAL = 0,
    OFFICER_LIEUTENANT
} OfficerType;

typedef enum {
    ROLE_NONE = 0,
    ROLE_EXPLORADOR,
    ROLE_INTENDENTE,
    ROLE_CAPELLAN
} OfficerRole;

const char* OfficerRole_GetName(OfficerRole role);


/* ============================================================
   ESTRUCTURA DEL GENERAL
   - Sus estad�sticas nunca pueden ser negativas (m�nimo 0).
   - Puede portar hasta 3 cartas divinas pasivas.
   ============================================================ */

typedef struct {
        char name[64];
    int age;

    ModifierInstance cards[3];
    int cardCount;
    ModifierInstance curses[4];
    int curseCount;

    /* Puntos base de atributos (siempre >= 0) */
    int vision;         /* Puntos de visi�n en niebla de guerra */
    int leadership;     /* Puntos de dirigencia (movimiento base por turno) */
    int evaluation;     /* Estimaci�n de tropas y composici�n enemiga */
    int planning;       /* Capacidad t�ctica y selecci�n de estrategia */
    int execution;      /* Fuerza de mando y efectividad en combate */

} General;


/* ============================================================
   ESTRUCTURA DEL TENIENTE
   - Puede tener atributos reducidos o incluso NEGATIVOS.
   - Cuenta �nicamente con 1 carta divina pasiva.
   - Posee lealtad (0 a 100), factor clave en motines y traici�n.
   ============================================================ */

typedef struct {
    char name[64];
    int age;
    OfficerRole role;
    char patronGod[32];

    ModifierInstance patronCard;
    bool hasPatronCard;
    ModifierInstance curses[4];
    int curseCount;

    /* Puntos de atributos (pueden ser negativos por inexperiencia o defectos) */
    int vision;
    int leadership;
    int evaluation;
    int planning;
    int execution;

    int loyalty;                /* Lealtad hacia el general (0 = traidor, 100 = devoto) */

} Lieutenant;


/* ============================================================
   REFERENCIA POLIM�RFICA AL COMANDANTE EN JEFE
   Permite que la tropa est� dirigida tanto por el General
   como por cualquiera de los Tenientes mediante una interfaz unificada.
   ============================================================ */

typedef struct {
    OfficerType type;
    void* ptr;                  /* Apunta a General* o Lieutenant* */
} CommanderRef;


/* ============================================================
   FUNCIONES DE CONSTRUCCI�N Y GESTI�N DE OFICIALES
   ============================================================ */

typedef struct {
    int vision;
    int leadership;
    int evaluation;
    int planning;
    int execution;
} OfficerAttributes;

typedef struct {
    const char* name;
    int age;
    OfficerAttributes stats;
    const char* initialCards[MAX_GENERAL_DIVINE_CARDS];
    int initialCardCount;
} GeneralConfig;

typedef struct {
    const char* name;
    int age;
    OfficerRole role;
    const char* patronGod;
    const char* patronCardId;
    OfficerAttributes stats;
    int loyalty;
} LieutenantConfig;

void General_Init(General* general, const GeneralConfig* cfg);
void Lieutenant_Init(Lieutenant* lt, const LieutenantConfig* cfg);

/* Catálogo de presets de personajes para selección y configuración */
int General_GetPresetCount(void);
GeneralConfig General_GetPreset(int index);
int Lieutenant_GetPresetCount(void);
LieutenantConfig Lieutenant_GetPreset(int index);

General General_Create(
    const char* name,
    int age,
    int vision,
    int leadership,
    int evaluation,
    int planning,
    int execution
);

Lieutenant Lieutenant_Create(
    const char* name,
    OfficerRole role,
    const char* patronGod,
    const char* patronCardId,
    int age,
    int vision,
    int leadership,
    int evaluation,
    int planning,
    int execution,
    int loyalty
);

/* M�todos polim�rficos de consulta sobre el oficial al mando */
CommanderRef Commander_FromGeneral(General* general);
CommanderRef Commander_FromLieutenant(Lieutenant* lt);

bool Commander_IsValid(CommanderRef cmd);
const char* Commander_GetName(CommanderRef cmd);
int Commander_GetAge(CommanderRef cmd);
int Commander_GetVision(CommanderRef cmd);
int Commander_GetLeadership(CommanderRef cmd);
int Commander_GetEvaluation(CommanderRef cmd);
int Commander_GetPlanning(CommanderRef cmd);
int Commander_GetExecution(CommanderRef cmd);
int Commander_GetLoyalty(CommanderRef cmd);



int Commander_GetCardCount(CommanderRef cmd);
const char* Commander_GetCard(CommanderRef cmd, int index);
int Commander_GetCurseCount(CommanderRef cmd);
const char* Commander_GetCurse(CommanderRef cmd, int index);

int Commander_GetActiveModifierCount(CommanderRef cmd);
const char* Commander_GetActiveModifier(CommanderRef cmd, int index);
bool Commander_AddActiveModifier(CommanderRef cmd, const char* modId);

ModifierInstance* Commander_GetCardInstance(CommanderRef cmd, int index);
bool Commander_IsCardExhausted(CommanderRef cmd, int index);
void Commander_SetCardExhausted(CommanderRef cmd, int index, bool exhausted);

#endif /* GENERAL_H */




