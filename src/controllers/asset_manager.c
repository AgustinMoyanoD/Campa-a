#include "asset_manager.h"
#include "raylib.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================
   CONSTANTES DE TAMAÑO DE TEXTURA
   Podés escalar estos valores si querés más resolución.
   ============================================================ */

#define TEX_W 192   /* Ancho de cada tile (puntos de un hex 2:1 iso) */
#define TEX_H 128   /* Alto de cada tile */

#define STRUCT_W 96
#define STRUCT_H 128

#define WATER_FRAMES 4  /* Frames de animación del agua */

/* ============================================================
   ANIMATED SPRITE
   ============================================================ */

void AnimSprite_Update(AnimatedSprite* spr, float deltaTime)
{
    if (!spr || spr->frameCount <= 1) return;
    spr->timer += deltaTime;
    if (spr->timer >= spr->frameDuration) {
        spr->timer -= spr->frameDuration;
        spr->currentFrame = (spr->currentFrame + 1) % spr->frameCount;
    }
}

Rectangle AnimSprite_GetFrame(const AnimatedSprite* spr)
{
    Rectangle r = { 0, 0, 0, 0 };
    if (!spr) return r;
    r.x      = (float)(spr->currentFrame * spr->frameW);
    r.y      = 0;
    r.width  = (float)spr->frameW;
    r.height = (float)spr->frameH;
    return r;
}

/* ============================================================
   GENERACIÓN PROCEDURAL DE TEXTURAS
   Cada función dibuja sobre una Image, luego la sube a la GPU.
   El artista puede poner un PNG externo con el mismo nombre
   y la función lo carga directamente sin regenerar.
   ============================================================ */

/* Llena la imagen con el color base (ya no recortamos el hexágono aquí,
 * map_view.c se encarga de aplicarlo al polígono matemático perfecto usando UVs) */


/* Dibuja un hexagono perfecto que toca los bordes exactos de la imagen */
static void FillHexOnImage(Image* img, Color fill)
{
    int w = img->width;
    int h = img->height;
    float cx = w / 2.0f;
    float cy = h / 2.0f;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float nx = (x - cx) / cx;
            float ny = (y - cy) / cy;
            
            bool inside = (0.5f * fabsf(nx) + fabsf(ny) <= 1.0f) && (fabsf(nx) <= 1.0f);
            
            if (inside) {
                ImageDrawPixel(img, x, y, fill);
            } else {
                ImageDrawPixel(img, x, y, (Color){0,0,0,0});
            }
        }
    }
}

/* Genera una textura con detalle de hierba para las llanuras */
static Texture2D GenTilePlains(void)
{
    Image img = GenImageColor(TEX_W, TEX_H, BLANK);
    Color base = (Color){ 140, 205, 90, 255 };
    FillHexOnImage(&img, base);

    /* Manchas de hierba más oscura */
    for (int i = 0; i < 30; i++) {
        int x = GetRandomValue(TEX_W/4, 3*TEX_W/4);
        int y = GetRandomValue(TEX_H/4, 3*TEX_H/4);
        ImageDrawCircle(&img, x, y, GetRandomValue(2, 5),
                        (Color){ 100, 170, 60, 180 });
    }
    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img);
    return tex;
}

/* Bosque: verde oscuro con "copas" de árbol circulares */
static Texture2D GenTileForest(void)
{
    Image img = GenImageColor(TEX_W, TEX_H, BLANK);
    Color base = (Color){ 35, 115, 50, 255 };
    FillHexOnImage(&img, base);

    /* Círculos representando copas de árboles */
    Color canopy1 = (Color){ 25, 90, 40, 220 };
    Color canopy2 = (Color){ 45, 130, 60, 200 };
    for (int i = 0; i < 12; i++) {
        int x = GetRandomValue(TEX_W/5, 4*TEX_W/5);
        int y = GetRandomValue(TEX_H/5, 4*TEX_H/5);
        int r = GetRandomValue(8, 16);
        ImageDrawCircle(&img, x, y, r, (i % 2 == 0) ? canopy1 : canopy2);
    }
    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img);
    return tex;
}

/* Desierto: arena cálida con duna suave */
static Texture2D GenTileDesert(void)
{
    Image img = GenImageColor(TEX_W, TEX_H, BLANK);
    Color base = (Color){ 230, 200, 110, 255 };
    FillHexOnImage(&img, base);

    /* Gradiente de duna suave */
    for (int y = 0; y < TEX_H; y++) {
        for (int x = 0; x < TEX_W; x++) {
            float t = (float)y / TEX_H;
            Color px = GetImageColor(img, x, y);
            if (px.a > 0) {
                px.r = (unsigned char)(215 + 20 * t);
                px.g = (unsigned char)(185 + 20 * t);
                px.b = (unsigned char)(90 + 20 * t);
                ImageDrawPixel(&img, x, y, px);
            }
        }
    }
    /* Grietas */
    for (int i = 0; i < 6; i++) {
        int x1 = GetRandomValue(TEX_W/4, 3*TEX_W/4);
        int y1 = GetRandomValue(TEX_H/4, 3*TEX_H/4);
        ImageDrawLine(&img, x1, y1, x1 + GetRandomValue(-20, 20),
                      y1 + GetRandomValue(-10, 10),
                      (Color){ 160, 130, 60, 180 });
    }
    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img);
    return tex;
}

/* Jungla: verde exótico con hojas grandes */
static Texture2D GenTileJungle(void)
{
    Image img = GenImageColor(TEX_W, TEX_H, BLANK);
    Color base = (Color){ 20, 95, 75, 255 };
    FillHexOnImage(&img, base);

    for (int i = 0; i < 16; i++) {
        int x = GetRandomValue(TEX_W/6, 5*TEX_W/6);
        int y = GetRandomValue(TEX_H/6, 5*TEX_H/6);
        /* Hojas como elipses rotadas */
        ImageDrawCircle(&img, x, y, GetRandomValue(6, 14),
                        (Color){ 30 + GetRandomValue(0,30), 110, 70, 200 });
    }
    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img);
    return tex;
}

/* Montaña: gris con nieve en el pico */
static Texture2D GenTileMountain(void)
{
    Image img = GenImageColor(TEX_W, TEX_H, BLANK);
    Color base = (Color){ 155, 145, 138, 255 };
    FillHexOnImage(&img, base);

    /* Pico de montaña */
    int cx = TEX_W / 2;
    int cy = TEX_H / 2;
    /* Triángulo de roca */
    ImageDrawTriangle(&img,
        (Vector2){ (float)cx, (float)(cy - 40) },
        (Vector2){ (float)(cx - 35), (float)(cy + 10) },
        (Vector2){ (float)(cx + 35), (float)(cy + 10) },
        (Color){ 120, 110, 105, 255 });
    /* Nieve */
    ImageDrawTriangle(&img,
        (Vector2){ (float)cx, (float)(cy - 40) },
        (Vector2){ (float)(cx - 12), (float)(cy - 18) },
        (Vector2){ (float)(cx + 12), (float)(cy - 18) },
        WHITE);

    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img);
    return tex;
}

/* Agua: azul con ondas dibujadas a mano */
static Image GenWaterFrame(int frame)
{
    Image img = GenImageColor(TEX_W, TEX_H, BLANK);
    Color base = (Color){ 95, 175, 235, 255 };
    FillHexOnImage(&img, base);

    /* Líneas de ola desplazadas según el frame */
    float offset = (float)frame * (TEX_W / (float)WATER_FRAMES);
    for (int i = 0; i < 5; i++) {
        float y = TEX_H * 0.2f + i * (TEX_H * 0.14f);
        for (int x = 0; x < TEX_W; x++) {
            float wave = sinf((x + offset) * 0.2f) * 3.0f;
            int iy = (int)(y + wave);
            if (iy >= 0 && iy < TEX_H) {
                Color px = GetImageColor(img, x, iy);
                if (px.a > 0) {
                    ImageDrawPixel(&img, x, iy, (Color){ 160, 210, 255, 200 });
                }
            }
        }
    }
    return img;
}

/* Genera un spritesheet de agua con WATER_FRAMES frames en horizontal */
static AnimatedSprite GenWaterAnimated(void)
{
    Image sheet = GenImageColor(TEX_W * WATER_FRAMES, TEX_H, BLANK);
    for (int f = 0; f < WATER_FRAMES; f++) {
        Image frame = GenWaterFrame(f);
        Rectangle dst = { (float)(f * TEX_W), 0, (float)TEX_W, (float)TEX_H };
        ImageDraw(&sheet, frame, (Rectangle){ 0, 0, TEX_W, TEX_H }, dst, WHITE);
        UnloadImage(frame);
    }
    AnimatedSprite spr = { 0 };
    spr.sheet         = LoadTextureFromImage(sheet);
    spr.frameCount    = WATER_FRAMES;
    spr.currentFrame  = 0;
    spr.frameDuration = 0.22f;
    spr.frameW        = TEX_W;
    spr.frameH        = TEX_H;
    UnloadImage(sheet);
    return spr;
}

/* -- Estructuras ------------------------------------------- */

static Texture2D GenStructAltar(void)
{
    Image img = GenImageColor(STRUCT_W, STRUCT_H, BLANK);
    /* Base de piedra */
    ImageDrawRectangle(&img, 20, 80, 56, 35, (Color){ 90, 85, 100, 255 });
    /* Columnas */
    ImageDrawRectangle(&img, 24, 50, 10, 35, (Color){ 120, 115, 130, 255 });
    ImageDrawRectangle(&img, 62, 50, 10, 35, (Color){ 120, 115, 130, 255 });
    /* Llama mágica */
    ImageDrawCircle(&img, 48, 45, 14, (Color){ 180, 80, 255, 240 });
    ImageDrawCircle(&img, 48, 38, 9,  (Color){ 220, 160, 255, 220 });
    ImageDrawCircle(&img, 48, 33, 5,  WHITE);
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

static Texture2D GenStructCastle(void)
{
    Image img = GenImageColor(STRUCT_W, STRUCT_H, BLANK);
    /* Cuerpo */
    ImageDrawRectangle(&img, 16, 50, 64, 65, (Color){ 75, 75, 82, 255 });
    /* Almenas */
    for (int i = 0; i < 4; i++)
        ImageDrawRectangle(&img, 16 + i*17, 36, 10, 18, (Color){ 95, 95, 102, 255 });
    /* Torre central */
    ImageDrawRectangle(&img, 32, 18, 32, 40, (Color){ 60, 60, 68, 255 });
    /* Bandera */
    ImageDrawLine(&img, 48, 18, 48, 5, (Color){ 80, 80, 90, 255 });
    ImageDrawRectangle(&img, 48, 5, 14, 9, RED);
    /* Puerta */
    ImageDrawRectangle(&img, 38, 80, 20, 35, (Color){ 40, 30, 20, 255 });
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

static Texture2D GenStructMarket(void)
{
    Image img = GenImageColor(STRUCT_W, STRUCT_H, BLANK);
    /* Piso de tienda */
    ImageDrawRectangle(&img, 10, 70, 76, 45, (Color){ 200, 160, 90, 255 });
    /* Toldo */
    ImageDrawTriangle(&img,
        (Vector2){ 48, 30 }, (Vector2){ 4, 70 }, (Vector2){ 92, 70 },
        ORANGE);
    /* Franja del toldo */
    ImageDrawRectangle(&img, 8, 68, 80, 6, (Color){ 220, 100, 20, 255 });
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

static Texture2D GenStructArmory(void)
{
    Image img = GenImageColor(STRUCT_W, STRUCT_H, BLANK);
    ImageDrawRectangle(&img, 18, 60, 60, 55, (Color){ 70, 80, 100, 255 });
    /* Techo triangular */
    ImageDrawTriangle(&img,
        (Vector2){ 48, 30 }, (Vector2){ 12, 62 }, (Vector2){ 84, 62 },
        (Color){ 50, 60, 80, 255 });
    /* Espada cruzada */
    ImageDrawLine(&img, 30, 50, 66, 86, (Color){ 200, 200, 220, 255 });
    ImageDrawLine(&img, 66, 50, 30, 86, (Color){ 200, 200, 220, 255 });
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

static Texture2D GenStructHouses(void)
{
    Image img = GenImageColor(STRUCT_W, STRUCT_H, BLANK);
    /* Casa 1 */
    ImageDrawRectangle(&img, 8, 70, 36, 42, (Color){ 190, 150, 110, 255 });
    ImageDrawTriangle(&img,
        (Vector2){ 26, 45 }, (Vector2){ 6, 72 }, (Vector2){ 46, 72 },
        (Color){ 160, 60, 60, 255 });
    /* Casa 2 */
    ImageDrawRectangle(&img, 50, 76, 36, 36, (Color){ 180, 140, 105, 255 });
    ImageDrawTriangle(&img,
        (Vector2){ 68, 52 }, (Vector2){ 48, 78 }, (Vector2){ 88, 78 },
        (Color){ 140, 50, 50, 255 });
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

static Texture2D GenStructWatchtower(void)
{
    Image img = GenImageColor(STRUCT_W, STRUCT_H, BLANK);
    /* Torre */
    ImageDrawRectangle(&img, 38, 30, 20, 85, (Color){ 80, 75, 70, 255 });
    /* Plataforma */
    ImageDrawRectangle(&img, 28, 22, 40, 12, (Color){ 100, 90, 82, 255 });
    /* Tejado */
    ImageDrawTriangle(&img,
        (Vector2){ 48, 5 }, (Vector2){ 24, 24 }, (Vector2){ 72, 24 },
        DARKBROWN);
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

static Texture2D GenStructBridge(void)
{
    Image img = GenImageColor(STRUCT_W, STRUCT_H, BLANK);
    /* Tablones */
    for (int i = 0; i < 5; i++)
        ImageDrawRectangle(&img, 8, 50 + i * 10, 80, 7, (Color){ 140, 100, 55, 255 });
    /* Pilares */
    ImageDrawRectangle(&img, 12, 40, 8, 60, (Color){ 100, 80, 50, 255 });
    ImageDrawRectangle(&img, 76, 40, 8, 60, (Color){ 100, 80, 50, 255 });
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

/* Token del ejército */
static Texture2D GenEnemyToken(void)
{
    Image img = GenImageColor(64, 64, BLANK);
    ImageDrawCircle(&img, 32, 32, 28, MAROON);
    ImageDrawCircleLines(&img, 32, 32, 28, BLACK);
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

static Texture2D GenArmyToken(void)
{
    Image img = GenImageColor(64, 64, BLANK);
    ImageDrawCircle(&img, 32, 32, 28, (Color){ 200, 150, 50, 255 });
    ImageDrawCircle(&img, 32, 32, 24, (Color){ 160, 110, 30, 255 });
    /* Silueta de guerrero (simple) */
    /* Cabeza */
    ImageDrawCircle(&img, 32, 20, 7, (Color){ 220, 180, 130, 255 });
    /* Cuerpo */
    ImageDrawRectangle(&img, 26, 27, 12, 18, (Color){ 80, 100, 160, 255 });
    /* Lanza */
    ImageDrawLine(&img, 42, 14, 42, 46, (Color){ 180, 180, 180, 255 });
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

/* ============================================================
   CARGA/GENERACIÓN CON FALLBACK A PNG EXTERNO
   Si assets/tiles/<nombre>.png existe, lo carga.
   Si no, genera la textura procedural y la exporta.
   ============================================================ */

static Texture2D LoadOrGen(const char* path, Texture2D (*genFn)(void))
{
    if (FileExists(path)) {
        Texture2D tex = LoadTexture(path);
        if (tex.id != 0) return tex;
    }
    Texture2D tex = genFn();
    /* Exportar para reemplazos futuros */
    Image img = LoadImageFromTexture(tex);
    ExportImage(img, path);
    UnloadImage(img);
    return tex;
}

/* ============================================================
   INTERFAZ PÚBLICA
   ============================================================ */

void Assets_Init(GameAssets* assets)
{
    if (!assets) return;
    memset(assets, 0, sizeof(GameAssets));

    /* Biomas */
    assets->tilePlains   = LoadOrGen("assets/tiles/plains.png",   GenTilePlains);
    assets->tileForest   = LoadOrGen("assets/tiles/forest.png",   GenTileForest);
    assets->tileDesert   = LoadOrGen("assets/tiles/desert.png",   GenTileDesert);
    assets->tileJungle   = LoadOrGen("assets/tiles/jungle.png",   GenTileJungle);
    assets->tileMountain = LoadOrGen("assets/tiles/mountain.png", GenTileMountain);

    /* Agua animada */
    if (FileExists("assets/tiles/water_sheet.png")) {
        Image sheet = LoadImage("assets/tiles/water_sheet.png");
        assets->waterAnim.sheet        = LoadTextureFromImage(sheet);
        assets->waterAnim.frameCount   = WATER_FRAMES;
        assets->waterAnim.frameW       = sheet.width / WATER_FRAMES;
        assets->waterAnim.frameH       = sheet.height;
        assets->waterAnim.frameDuration = 0.22f;
        UnloadImage(sheet);
    } else {
        assets->waterAnim = GenWaterAnimated();
        Image sheet = LoadImageFromTexture(assets->waterAnim.sheet);
        ExportImage(sheet, "assets/tiles/water_sheet.png");
        UnloadImage(sheet);
    }
    assets->tileWater = assets->waterAnim.sheet; /* alias estático */

    /* Estructuras */
    assets->structAltar      = LoadOrGen("assets/structures/altar.png",      GenStructAltar);
    assets->structCastle     = LoadOrGen("assets/structures/castle.png",     GenStructCastle);
    assets->structMarket     = LoadOrGen("assets/structures/market.png",     GenStructMarket);
    assets->structArmory     = LoadOrGen("assets/structures/armory.png",     GenStructArmory);
    assets->structHouses     = LoadOrGen("assets/structures/houses.png",     GenStructHouses);
    assets->structWatchtower = LoadOrGen("assets/structures/watchtower.png", GenStructWatchtower);
    assets->structBridge     = LoadOrGen("assets/structures/bridge.png",     GenStructBridge);

    /* Token del ejército */
    assets->armyToken = LoadOrGen("assets/units/army_token.png", GenArmyToken);
    assets->enemyToken = LoadOrGen("assets/units/enemy_token.png", GenEnemyToken);

    assets->loaded = true;
}

void Assets_UpdateAnimations(GameAssets* assets, float deltaTime)
{
    if (!assets || !assets->loaded) return;
    AnimSprite_Update(&assets->waterAnim, deltaTime);
}

void Assets_Unload(GameAssets* assets)
{
    if (!assets || !assets->loaded) return;

    UnloadTexture(assets->tilePlains);
    UnloadTexture(assets->tileForest);
    UnloadTexture(assets->tileDesert);
    UnloadTexture(assets->tileJungle);
    UnloadTexture(assets->tileMountain);
    /* El agua animada tiene su propia hoja — tileWater es alias, no duplicar */
    UnloadTexture(assets->waterAnim.sheet);

    UnloadTexture(assets->structAltar);
    UnloadTexture(assets->structCastle);
    UnloadTexture(assets->structMarket);
    UnloadTexture(assets->structArmory);
    UnloadTexture(assets->structHouses);
    UnloadTexture(assets->structWatchtower);
    UnloadTexture(assets->structBridge);

    UnloadTexture(assets->armyToken);
    UnloadTexture(assets->enemyToken);

    assets->loaded = false;
}

Texture2D Assets_GetBiomeTile(const GameAssets* assets, BiomeType biome)
{
    switch (biome) {
        case BIOME_FOREST:   return assets->tileForest;
        case BIOME_DESERT:   return assets->tileDesert;
        case BIOME_JUNGLE:   return assets->tileJungle;
        case BIOME_MOUNTAIN: return assets->tileMountain;
        case BIOME_WATER:    return assets->waterAnim.sheet;
        case BIOME_PLAINS:
        default:             return assets->tilePlains;
    }
}



