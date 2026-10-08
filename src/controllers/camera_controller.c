#include "camera_controller.h"

void CameraController_Init(CameraController* cc, Vector2 offset)
{
    cc->camera.target = (Vector2){ 0.0f, 0.0f };
    cc->camera.offset = offset;
    cc->camera.rotation = 0.0f;
    cc->camera.zoom = 1.0f;
    
    cc->panSpeed = 1000.0f; // Pixels per second at zoom=1.0
    cc->minZoom = 0.2f;
    cc->maxZoom = 3.0f;
    cc->zoomSpeed = 0.1f;
}

void CameraController_Update(CameraController* cc, float deltaTime)
{
    float currentPanSpeed = cc->panSpeed * deltaTime / cc->camera.zoom;
    
    // 1. WASD Panning
    if (IsKeyDown(KEY_D)) cc->camera.target.x += currentPanSpeed;
    if (IsKeyDown(KEY_A)) cc->camera.target.x -= currentPanSpeed;
    if (IsKeyDown(KEY_S)) cc->camera.target.y += currentPanSpeed;
    if (IsKeyDown(KEY_W)) cc->camera.target.y -= currentPanSpeed;

    // 2. Edge Panning (Mouse to screen edges)
    Vector2 mousePos = GetMousePosition();
    float edgeThreshold = 10.0f; // Pixels from the edge to trigger pan
    
    if (mousePos.x <= edgeThreshold) cc->camera.target.x -= currentPanSpeed;
    if (mousePos.x >= GetScreenWidth() - edgeThreshold) cc->camera.target.x += currentPanSpeed;
    if (mousePos.y <= edgeThreshold) cc->camera.target.y -= currentPanSpeed;
    if (mousePos.y >= GetScreenHeight() - edgeThreshold) cc->camera.target.y += currentPanSpeed;

    // 3. Zooming (Mouse Wheel)
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f)
    {
        // Panning to mouse position during zoom
        Vector2 mouseWorldPos = GetScreenToWorld2D(mousePos, cc->camera);
        
        cc->camera.zoom += wheel * cc->zoomSpeed;
        if (cc->camera.zoom < cc->minZoom) cc->camera.zoom = cc->minZoom;
        if (cc->camera.zoom > cc->maxZoom) cc->camera.zoom = cc->maxZoom;
        
        // Update target to keep mouse in the same world position
        Vector2 newMouseWorldPos = GetScreenToWorld2D(mousePos, cc->camera);
        cc->camera.target.x -= (newMouseWorldPos.x - mouseWorldPos.x);
        cc->camera.target.y -= (newMouseWorldPos.y - mouseWorldPos.y);
    }
}

void CameraController_FocusOn(CameraController* cc, Vector2 targetPos)
{
    cc->camera.target = targetPos;
}

