#ifndef ENGINE_CAMERA_CONTROLLER_H
#define ENGINE_CAMERA_CONTROLLER_H

#include "raylib.h"

/* ============================================================
   ENGINE CAMERA CONTROLLER: CONTROLADOR 2D SUAVE CON ZOOM
   ============================================================ */

typedef struct {
    Camera2D camera;
    float panSpeed;
    float minZoom;
    float maxZoom;
    float zoomSpeed;
} CameraController;

void CameraController_Init(CameraController* cc, Vector2 offset);
void CameraController_Update(CameraController* cc, float deltaTime);
void CameraController_FocusOn(CameraController* cc, Vector2 targetPos);

#endif /* ENGINE_CAMERA_CONTROLLER_H */
