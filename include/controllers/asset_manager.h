#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "raylib.h"
#include "hex_tile.h"

/* ============================================================
   SPRITE ANIMADO
   Contiene una textura de hoja de frames (spritesheet horizontal)
   y lleva la cuenta del frame actual.
   ============================================================ */

typedef struct {
    Texture2D sheet;        /* Hoja de sprites (frames en fila horizontal) */
    int       frameCount;   /* Cuántos frames tiene */
    int       currentFrame; /* Frame que se está mostrando ahora */
    float     frameDuration;/* Duración de cada frame en segundos */
    float     timer;        /* Acumulador de tiempo */
    int       frameW;       /* Ancho de un frame individual */
    int       frameH;       /* Alto de un frame individual */
} AnimatedSprite;

/* Avanza el timer y cambia de frame si corresponde */
void AnimSprite_Update(AnimatedSprite* spr, float deltaTime);

/* Retorna el rectángulo del frame actual para usar con DrawTexturePro */
Rectangle AnimSprite_GetFrame(const AnimatedSprite* spr);

/* ============================================================
   CONTENEDOR GLOBAL DE ASSETS
   ============================================================ */

typedef struct {
    /* Texturas de biomas (se usan en la cara superior del bloque iso) */
    Texture2D tilePlains;
    Texture2D tileForest;
    Texture2D tileDesert;
    Texture2D tileJungle;
    Texture2D tileMountain;
    Texture2D tileWater;  /* Frame estático (el animated es para las olas) */

    /* Animación del agua (spritesheet con 4 frames de oleaje) */
    AnimatedSprite waterAnim;

    /* Texturas de estructuras */
    Texture2D structAltar;
    Texture2D structCastle;
    Texture2D structMarket;
    Texture2D structArmory;
    Texture2D structHouses;
    Texture2D structWatchtower;
    Texture2D structBridge;

    /* Token del ejército (sprite estático de momento) */
    Texture2D armyToken;

    bool loaded; /* TRUE si los assets se cargaron correctamente */
} GameAssets;

/* Carga / genera todos los assets.
   Si existe assets/tiles/<nombre>.png lo carga desde disco.
   Si no existe, lo genera programáticamente y lo exporta a PNG
   para que el artista pueda reemplazarlo más tarde. */
void Assets_Init(GameAssets* assets);

/* Actualiza las animaciones (llamar cada frame con GetFrameTime()) */
void Assets_UpdateAnimations(GameAssets* assets, float deltaTime);

/* Libera toda la VRAM */
void Assets_Unload(GameAssets* assets);

/* Devuelve la textura de bioma correspondiente (sin animación) */
Texture2D Assets_GetBiomeTile(const GameAssets* assets, BiomeType biome);

#endif /* ASSET_MANAGER_H */
