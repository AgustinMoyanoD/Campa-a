#ifndef ENGINE_ASSET_CACHE_H
#define ENGINE_ASSET_CACHE_H

#include "raylib.h"
#include <stdbool.h>

/* ============================================================
   ENGINE ASSET CACHE: GESTOR DE TEXTURAS Y SPRITES ANIMADOS
   ============================================================ */

#define ASSET_CACHE_MAX_TEXTURES 64

typedef struct {
    Texture2D sheet;        /* Hoja de sprites (frames en fila horizontal) */
    int       frameCount;   /* Cuántos frames tiene */
    int       currentFrame; /* Frame que se está mostrando ahora */
    float     frameDuration;/* Duración de cada frame en segundos */
    float     timer;        /* Acumulador de tiempo */
    int       frameW;       /* Ancho de un frame individual */
    int       frameH;       /* Alto de un frame individual */
} AnimatedSprite;

void      AnimSprite_Init(AnimatedSprite* spr, Texture2D sheet, int frameCount, float frameDuration, int frameW, int frameH);
void      AnimSprite_Update(AnimatedSprite* spr, float deltaTime);
Rectangle AnimSprite_GetFrame(const AnimatedSprite* spr);

/* Caché de texturas por ruta/clave */
void      AssetCache_Init(void);
Texture2D AssetCache_LoadTexture(const char* filepath);
Texture2D AssetCache_GetTexture(const char* filepath);
void      AssetCache_UnloadAll(void);

#endif /* ENGINE_ASSET_CACHE_H */
