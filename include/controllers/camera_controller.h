#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include "raylib.h"

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

#endif
