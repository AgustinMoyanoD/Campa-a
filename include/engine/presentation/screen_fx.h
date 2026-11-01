#ifndef ENGINE_SCREEN_FX_H
#define ENGINE_SCREEN_FX_H

#include "raylib.h"

/* ============================================================
   ENGINE SCREEN FX: SACUDIDAS DE PANTALLA, FLASH, FADES Y PARTÍCULAS
   ============================================================ */

void ScreenFX_Init(void);
void ScreenFX_Update(float dt);

void ScreenFX_FadeToBlack(float duration);
void ScreenFX_FadeFromBlack(float duration);
void ScreenFX_Shake(float duration, float intensity);
void ScreenFX_Flash(float duration, Color color);

void ScreenFX_ApplyShakeToCamera(Camera2D* cam);
Vector2 ScreenFX_GetShakeOffset(void);
void ScreenFX_DrawOverlays(void);

void ScreenFX_SpawnMarchDust(Vector2 worldPos, Color terrainTint);
void ScreenFX_DrawMapParticles(void);
void ScreenFX_DrawNightAtmosphere(int screenWidth, int screenHeight);

#endif /* ENGINE_SCREEN_FX_H */
