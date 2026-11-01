#ifndef SETUP_VIEW_H
#define SETUP_VIEW_H

#include "raylib.h"
#include "game/models/army.h"

typedef enum {
    SETUP_ACTION_NONE = 0,
    SETUP_ACTION_BACK,
    SETUP_ACTION_START
} SetupViewAction;

/**
 * Dibuja la pantalla de selección y preparación de la expedición.
 * Retorna la acción elegida por el jugador (Volver, Iniciar, o Ninguna si continúa en la pantalla).
 */
SetupViewAction SetupView_Draw(const ArmySetupConfig* setup, int sw, int sh);

#endif /* SETUP_VIEW_H */
