#ifndef RATION_VIEW_H
#define RATION_VIEW_H

#include <stdbool.h>
#include "raylib.h"
#include "models/army.h"
#include "models/hex_tile.h"
#include "models/day_clock.h"
#include "models/ration.h"
#include "presentation/ui_widgets.h"

/* ============================================================
   ACCIONES DEVUELTAS POR LA VISTA DE RACIONES (MVC Puro)
   La vista NO modifica el juego; devuelve una acción al controlador.
   ============================================================ */

typedef enum {
    RATION_ACTION_NONE = 0,
    RATION_ACTION_SUBMIT_PLAN,        /* Jugador confirmó el plan de reparto */
    RATION_ACTION_SLAUGHTER_CATTLE,   /* Petición de sacrificar cabezas de ganado */
    RATION_ACTION_CLOSE_REPORT        /* Cierre del reporte de fin de día */
} RationViewActionType;

typedef struct {
    RationViewActionType type;
    RationPlan plan;                  /* Plan para RATION_ACTION_SUBMIT_PLAN */
    int cattleHeads;                  /* Cantidad para RATION_ACTION_SLAUGHTER_CATTLE */
} RationViewAction;

/* ============================================================
   ESTADO DE LA VISTA DE RACIONES (Vive en la capa de vista)
   ============================================================ */

typedef struct {
    bool initialized;
    RationPlan draftPlan;             /* Borrador editable del reparto */
    RationPlan initialPlan;           /* Plan de referencia para reiniciar */
    UIDragDrop dragDrop;              /* Componente de arrastrar y soltar */

    int cattleToSlaughter;            /* Contador para sacrificio (+/-) */
    bool isCompactMode;               /* Abundancia: confirmación rápida */
    bool showingReport;               /* Página 2: reporte tras confirmar */
    DayReport lastReport;             /* Copia del reporte para página 2 */

    /* Interpolaciones animadas de barras */
    float groupDrinkAnim[RG_COUNT];
    float groupFoodAnim[RG_COUNT];

    /* Validación actual */
    bool isValid;
    char validationError[128];
    bool stragglersRegrouped;         /* TRUE si se reagruparon rezagados al acampar tras derrota */
} RationViewState;

/* Inicializa el estado de la vista al abrirse el reparto */
void RationView_Init(RationViewState* state, const Army* army, const Map* map, HexCoord pos);

/* Actualiza la entrada del usuario y lógica de interacción (devuelve acción al controlador) */
RationViewAction RationView_Update(RationViewState* state, const Army* army, const Map* map, HexCoord pos, int screenWidth, int screenHeight);

/* Dibuja la pantalla modal de reparto de raciones (o la página de reporte) */
void RationView_Draw(const RationViewState* state, const Army* army, const Map* map, HexCoord pos, int screenWidth, int screenHeight);

/* Activa la página 2 del reporte tras confirmar el reparto */
void RationView_ShowReport(RationViewState* state, const DayReport* report);

#endif /* RATION_VIEW_H */
