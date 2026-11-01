#ifndef RATION_H
#define RATION_H

#include <stdbool.h>
#include "models/hex_tile.h"

/* Forward declarations */
struct Army;
typedef struct Army Army;

struct Map;
typedef struct Map Map;

struct DayReport;
typedef struct DayReport DayReport;

struct Rng;
typedef struct Rng Rng;

struct GameMaster;
typedef struct GameMaster GameMaster;

/* ============================================================
   CONTRATO DE INTERFAZ: SISTEMA DE RACIONES Y REPARTO DIARIO
   ============================================================ */

typedef enum {
    RG_INICIADOS = 0,
    RG_SOLDADOS,
    RG_SARGENTOS,
    RG_GANADO,
    RG_COUNT
} RationGroup;

typedef enum {
    RS_AGUA = 0,
    RS_VINO,
    RS_COMIDA,
    RS_CARNE,
    RS_COUNT
} RationResource;

typedef struct {
    float amount[RG_COUNT][RS_COUNT];
} RationPlan;

typedef struct {
    float drink[RG_COUNT];
    float food[RG_COUNT];
} RationNeeds;

typedef enum {
    RATION_FULL = 0,
    RATION_REDUCED,
    RATION_STARVING
} RationLevel;

typedef struct {
    RationLevel level;
    float moraleDelta;
    float healthDelta;
    float satisfactionDelta;
    int daysUnfed;
    bool risksDesertion;
} GroupPreview;

typedef struct {
    GroupPreview group[RG_COUNT];
    float stockAfter[RS_COUNT];
    float daysLeft[RS_COUNT];
} RationPreview;

typedef struct {
    float morale;
    float health;
    int daysUnfed;
    float satisfaction;
    float satisfactionMax;
} GroupState; /* satisfaccion: solo Sargentos */

/* ============================================================
   CONSULTAS PURAS (No modifican estado, aptas para UI y vistas)
   ============================================================ */

RationNeeds   Ration_GetNeeds(const Army* army, const Map* map, HexCoord pos);
float         Ration_GetStock(const Army* army, RationResource r);
GroupState    Ration_GetGroupState(const Army* army, RationGroup g);
RationPlan    Ration_Auto(const Army* army, const RationNeeds* needs);
RationPlan    Ration_Repeat(const Army* army);  /* reparto de ayer ajustado al stock actual */
bool          Ration_Validate(const Army* army, const RationPlan* plan, char* err, int cap);
RationPreview Ration_Preview(const Army* army, const RationPlan* plan);
bool          Ration_NeedsDecision(const Army* army, const Map* map, HexCoord pos); /* hay faltante */

/* ============================================================
   OPERACIONES DE MODELO (Solo las llama el controlador)
   ============================================================ */

bool Ration_Commit(Army* army, const RationPlan* plan, DayReport* report, Rng* rng);
int  Ration_SlaughterCattle(Army* army, int heads);  /* devuelve unidades de carne */

#endif /* RATION_H */
