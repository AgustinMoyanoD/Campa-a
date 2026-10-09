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


/* ============================================================
   ESTRUCTURA DEL GENERAL
   - Sus estadísticas nunca pueden ser negativas (mínimo 0).
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
    int vision;         /* Puntos de visión en niebla de guerra */
    int leadership;     /* Puntos de dirigencia (movimiento base por turno) */
    int evaluation;     /* Estimación de tropas y composición enemiga */
    int planning;       /* Capacidad táctica y selección de estrategia */
    int execution;      /* Fuerza de mando y efectividad en combate */

} General;


/* ============================================================
   ESTRUCTURA DEL TENIENTE
   - Puede tener atributos reducidos o incluso NEGATIVOS.
   - Cuenta únicamente con 1 carta divina pasiva.
   - Posee lealtad (0 a 100), factor clave en motines y traición.
   ============================================================ */

typedef struct {
        char name[64];
    int age;

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
   REFERENCIA POLIMÓRFICA AL COMANDANTE EN JEFE
   Permite que la tropa esté dirigida tanto por el General
   como por cualquiera de los Tenientes mediante una interfaz unificada.
   ============================================================ */

typedef struct {
    OfficerType type;
    void* ptr;                  /* Apunta a General* o Lieutenant* */
} CommanderRef;


/* ============================================================
   FUNCIONES DE CONSTRUCCIÓN Y GESTIÓN DE OFICIALES
   ============================================================ */

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
    const char* name, const char* patronCardId,
    int age,
    int vision,
    int leadership,
    int evaluation,
    int planning,
    int execution,
    int loyalty
);

/* Métodos polimórficos de consulta sobre el oficial al mando */
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



int Commander_GetActiveModifierCount(CommanderRef cmd);
const char* Commander_GetActiveModifier(CommanderRef cmd, int index);
bool Commander_AddActiveModifier(CommanderRef cmd, const char* modId);
#endif /* GENERAL_H */




