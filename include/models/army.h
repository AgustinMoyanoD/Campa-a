#ifndef ARMY_H
#define ARMY_H

#include "hex_tile.h"
#include "general.h"
#include "models/rng.h"
#include <stdbool.h>

#define MAX_ARMY_LIEUTENANTS 6

/* Constante por defecto B para la fórmula de ejecución (peso relativo de tropa regular) */
#define DEFAULT_EXECUTION_B 2.0f

/* ============================================================
   REPORTE DE CIERRE DEL DÍA (I7)
   Registra todos los efectos de consumo, desgaste y supervivencia.
   ============================================================ */

typedef struct {
    int dayNumber;
    float foodConsumed;
    float waterConsumed;
    bool foodShortage;
    bool waterShortage;
    int daysWithoutFood;
    int daysWithoutWater;

    float moralDelta;
    float saludDelta;

    int desertedInitiates;
    int desertedSoldiers;
    int desertedSergeants;

    int deadInitiates;
    int deadSoldiers;
    int deadSergeants;
} DayReport;

/* ============================================================
   ESTRUCTURA DE LA ARMY (EJÉRCITO / DIVISIÓN)
   ============================================================ */

typedef struct {
    /* Posición en el mapa hexagonal */
    HexCoord position;
    HexCoord previousPosition;

    /* Oficiales */
    General general;
    Lieutenant lieutenants[MAX_ARMY_LIEUTENANTS];
    int lieutenantCount;

    /* Oficial actualmente al mando de esta tropa */
    CommanderRef commander;

    /* Tropas por categoría de experiencia */
    int sargentos;  /* Soldados veteranos experimentados: alto valor táctico */
    int soldados;   /* Soldados regulares de línea */
    int iniciados;  /* Reclutas novatos recién alistados */

    /* Recursos logísticos de utilidad */
    float agua_disponible;
    float comida_disponible;
    int monedas;
    int ganado;     /* Animales vivos: reserva de emergencia y carga */
    int commandTokens; /* Fichas de mando para repetir tiradas o sumar +2 */

    ModifierInstance curses[8];
    int curseCount;
    int cursesRemovedCount; /* Total historico de maldiciones eliminadas en templos (Sec A5) */

    /* Indicadores vitales */
    float moral;    /* 0.0f a 100.0f */
    float salud;    /* 0.0f a 100.0f */
    int infamia;    /* Escala con saqueos y agresiones */

    /* Supervivencia y Noche (arreglo_exploracion.md) */
    int daysWithoutFood;
    int daysWithoutWater;
    int consecutiveNightMarches;

    /* Animacion de Movimiento */
    HexCoord movePath[128];
    int pathLength;
    int currentPathIndex;
    float moveTimer;
    bool isMoving;

} Army;


/* ============================================================
   ESTADÍSTICAS CALCULADAS DE LA ARMY
   ============================================================ */

typedef struct {
    float vision;
    float movilidad;    /* Puntos de dirigencia / movimiento por turno */
    float evaluacion;   /* Etapa 1 del combate: reconocimiento enemigo */
    float planificacion;/* Etapa 2 del combate: estrategia táctica */
    float ejecucion;    /* Etapa 3 del combate: poder de choque */
} ArmyStats;


/* ============================================================
   FUNCIONES DE CONSTRUCCIÓN Y GESTIÓN
   ============================================================ */

Army Army_Create(
    HexCoord startPos,
    General general,
    int sargentos,
    int soldados,
    int iniciados,
    float agua,
    float comida,
    int monedas,
    int ganado
);

/* Totales y recuento de tropas */
int Army_GetTotalSoldiers(const Army* army);
int Army_GetTotalWeight(const Army* army);

/* Gestión de oficiales al mando */
bool Army_AddLieutenant(Army* army, Lieutenant lt);
void Army_SetCommanderGeneral(Army* army);
bool Army_SetCommanderLieutenant(Army* army, int lieutenantIndex);

/* Consumo logístico por turno */
float Army_CalculateWaterUsage(int cantSoldados, int cantGanado);
float Army_CalculateFoodUsage(int cantSoldados, int cantGanado);
void Army_ConsumeTurnResources(Army* army);

/* Supervivencia y Cierre de Día (6.1, E1) */
void  Army_EndDay(Army* army, DayReport* report, Rng* rng);
float Army_DaysOfSupply(const Army* army);

/* Pronóstico unificado de suministros (Sec A3) */
struct Map;
typedef struct Map Map;
typedef struct {
    float campDays;
    float marchDays;
    float nightMarchDays;
} SupplyForecast;

SupplyForecast Army_ForecastSupply(const Army* army, const Map* map, HexCoord pos);

/* ============================================================
   FÓRMULAS DE ESTADÍSTICAS GLOBALES DE LA ARMY
   ============================================================ */

/*
    Fórmula de Ejecución solicitada:
    f_ejecucion = ejecucion_comandante + sargentos + (soldados / B) + f(iniciados)
    Donde tener pocos iniciados ayuda ligeramente, pero tener demasiados perjudica.
*/
float Army_CalculateExecution(const Army* army, float B);
float Army_CalculateMobility(const Army* army);
float Army_CalculateVision(const Army* army);
float Army_CalculateEvaluation(const Army* army);
float Army_CalculatePlanning(const Army* army);

ArmyStats Army_GetStats(const Army* army, float B);

/* ============================================================
   SISTEMA DE MOTÍN Y TRAICIÓN
   Evalúa el riesgo de motín si la moral cae y hay tenientes con baja lealtad.
   ============================================================ */

float Army_CalculateMutinyRisk(const Army* army);
bool Army_CheckMutiny(Army* army, int* outRebelLieutenantIndex);

typedef struct ModifierDef ModifierDef;

typedef enum { EQUIP_OK, EQUIP_SLOTS_FULL, EQUIP_DUPLICATE, EQUIP_INVALID } EquipResult;

EquipResult Army_EquipModifier(Army* army, const ModifierDef* def);
bool        Army_ReplaceCard(Army* army, int slot, const ModifierDef* def);
bool        Army_RemoveModifier(Army* army, const char* id);
enum DurationType;
void        Army_TickModifiers(Army* army, enum DurationType type);
bool        Army_HasModifier(const Army* army, const char* id);

#endif /* ARMY_H */


int Army_GetVision(const Army* army);



