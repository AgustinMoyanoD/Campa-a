#ifndef MOVEMENT_CONTROLLER_H
#define MOVEMENT_CONTROLLER_H

#include "game/controllers/game_master.h"
#include "game/models/army.h"
#include "game/models/map_generator.h"
#include "game/views/map_view.h"
#include "engine/presentation/camera_controller.h"

/**
 * Actualiza el avance paso a paso del ejército a lo largo de su ruta planificada.
 * Ejecuta animaciones con Tween, efectos de polvo, sonidos de paso por bioma,
 * actualización de niebla de guerra y detección de emboscadas con NPCs.
 */
void Movement_UpdateStep(GameMaster* gm, Map* map, Army* army, MapViewState* mapView);

/**
 * Resuelve la animación de retirada o posicionamiento tras finalizar un combate.
 */
void Movement_HandleCombatRetreat(GameMaster* gm, const Map* map, const Army* army, MapViewState* mapView, CameraController* cc);

#endif /* MOVEMENT_CONTROLLER_H */
