#include "asset_manager.h"
#include "raylib.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

/* ============================================================
   CONSTANTES DE GEOMETRÍA PIXEL-ART ISOMÉTRICA (2:1)
   - Base hexagonal matemática: 64 x 37 px (proporción √3 : 1)
   - Franja de relieve superior (overhang): 23 px arriba
   - Lienzo total de cada tile: 64 x 60 px
   ============================================================ */

#define HEX_BASE_W      64
#define HEX_BASE_H      37
#define HEX_OVERHANG_H  23
#define TEX_W           64
#define TEX_H           (HEX_BASE_H + HEX_OVERHANG_H) /* 60 px */

#define STRUCT_W        72
#define STRUCT_H        72

#define WATER_FRAMES    4

/* ============================================================
   PRNG DETERMINISTA PARA DETALLE PIXEL-ART REPRODUCIBLE
   ============================================================ */

static uint32_t s_pixelSeed = 0xA341316Cu;

static void PixelSeed(uint32_t seed) {
    s_pixelSeed = seed ? seed : 0xA341316Cu;
}

static uint32_t PixelRand(void) {
    uint32_t x = s_pixelSeed;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_pixelSeed = x;
    return x;
}

static int PixelRange(int minV, int maxV) {
    if (maxV <= minV) return minV;
    return minV + (int)(PixelRand() % (uint32_t)(maxV - minV + 1));
}

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
   MÁSCARA HEXAGONAL MATEMÁTICA EXACTA (POINTY-TOP 2:1)
   Evalúa si el píxel (x, y) del lienzo (TEX_W x TEX_H) cae dentro
   de la cara superior del hexágono ubicada en y = [HEX_OVERHANG_H .. TEX_H-1].
   ============================================================ */

static bool IsInsideHexBase(int x, int y, float* outEdgeDist)
{
    int localY = y - HEX_OVERHANG_H;
    if (localY < 0 || localY >= HEX_BASE_H) return false;

    float cx = (HEX_BASE_W - 1) * 0.5f;
    float cy = (HEX_BASE_H - 1) * 0.5f;
    float nx = ((float)x - cx) / (HEX_BASE_W * 0.5f);
    float ny = ((float)localY - cy) / (HEX_BASE_H * 0.5f);

    float ax = fabsf(nx);
    float ay = fabsf(ny);

    /* Un hexágono pointy-top inscrito en [-1, 1] x [-1, 1] cumple:
     * ax <= 1.0  y  0.5 * ax + ay <= 1.0 */
    float dSide = 1.0f - ax;
    float dDiag = 1.0f - (0.5f * ax + ay);
    if (dSide < 0.0f || dDiag < 0.0f) return false;

    if (outEdgeDist) {
        *outEdgeDist = (dSide < dDiag) ? dSide : dDiag;
    }
    return true;
}

/* Dibuja la base hexagonal con bisel perimetral e iluminación cenital NO -> SE */
static void FillHexBaseWithBevel(Image* img, Color baseCol, Color lightCol, Color shadowCol, Color rimCol)
{
    for (int y = HEX_OVERHANG_H; y < TEX_H; y++) {
        for (int x = 0; x < TEX_W; x++) {
            float edgeDist = 0.0f;
            if (!IsInsideHexBase(x, y, &edgeDist)) continue;

            int localY = y - HEX_OVERHANG_H;
            /* Gradiente sutil noroeste (luz) a sureste (sombra) */
            float grad = ((float)(x - TEX_W / 2) / (float)TEX_W) * 0.5f +
                         ((float)(localY - HEX_BASE_H / 2) / (float)HEX_BASE_H) * 0.5f;

            Color c = baseCol;
            if (((x + y) & 1) == 0 && grad < -0.10f) {
                c = lightCol;
            } else if (((x + y) & 1) == 1 && grad > 0.12f) {
                c = shadowCol;
            }

            /* Borde exterior de 1px para encaje nítido entre hexágonos */
            if (edgeDist < 0.045f) {
                c = rimCol;
            } else if (edgeDist < 0.095f && localY < HEX_BASE_H / 2) {
                c = lightCol;
            }

            ImageDrawPixel(img, x, y, c);
        }
    }
}

/* Dibuja un píxel sólo si cae dentro de la base hexagonal */
static void PutPixelInHex(Image* img, int x, int y, Color c)
{
    if (IsInsideHexBase(x, y, NULL)) {
        ImageDrawPixel(img, x, y, c);
    }
}

/* Dibuja un píxel en cualquier parte del lienzo (incluyendo overhang superior),
   pero sin salirse por debajo ni por los lados de la base hexagonal */
static void PutPixelWithOverhang(Image* img, int x, int y, Color c)
{
    if (x < 1 || x >= TEX_W - 1 || y < 1 || y >= TEX_H) return;
    if (y >= HEX_OVERHANG_H + HEX_BASE_H / 2) {
        /* En la mitad inferior del hexágono, recortar estrictamente a la máscara */
        if (!IsInsideHexBase(x, y, NULL)) return;
    } else if (y >= HEX_OVERHANG_H) {
        /* En la mitad superior de la base, permitir sobresalir hacia arriba pero no fuera de X */
        float cx = (HEX_BASE_W - 1) * 0.5f;
        if (fabsf((float)x - cx) > (HEX_BASE_W * 0.48f)) return;
    }
    ImageDrawPixel(img, x, y, c);
}

/* ============================================================
   1. LLANURA HELÉNICA (plains.png)
   Estepa dorada-oliva con matas de hierba, piedras calizas y flores ocres
   ============================================================ */

static Texture2D GenTilePlains(void)
{
    PixelSeed(0x101A2B3Cu);
    Image img = GenImageColor(TEX_W, TEX_H, BLANK);

    Color cBase   = (Color){ 134, 152,  78, 255 }; /* Verde oliva seco */
    Color cLight  = (Color){ 152, 168,  92, 255 }; /* Trigo verdoso claro */
    Color cShadow = (Color){ 112, 130,  62, 255 }; /* Oliva musgo oscuro */
    Color cRim    = (Color){  88, 104,  48, 255 }; /* Borde de encaje */

    FillHexBaseWithBevel(&img, cBase, cLight, cShadow, cRim);

    /* Parches orgánicos de pasto cálido y tierra seca */
    for (int p = 0; p < 18; p++) {
        int px = PixelRange(8, TEX_W - 9);
        int py = PixelRange(HEX_OVERHANG_H + 4, TEX_H - 5);
        int rx = PixelRange(3, 6);
        int ry = PixelRange(2, 3);
        Color patchCol = (p % 3 == 0) ? (Color){ 162, 166, 94, 255 }
                                      : (Color){ 120, 138, 68, 255 };
        for (int dy = -ry; dy <= ry; dy++) {
            for (int dx = -rx; dx <= rx; dx++) {
                if ((dx * dx) * (ry * ry) + (dy * dy) * (rx * rx) <= (rx * rx) * (ry * ry)) {
                    if (((dx + dy) & 1) == 0) {
                        PutPixelInHex(&img, px + dx, py + dy, patchCol);
                    }
                }
            }
        }
    }

    /* Matas de hierba pixel-art (3-5 píxeles en abanico) */
    Color bladeDark  = (Color){  78,  96,  42, 255 };
    Color bladeMid   = (Color){ 106, 128,  56, 255 };
    Color bladeTip   = (Color){ 184, 178,  96, 255 };
    for (int i = 0; i < 28; i++) {
        int gx = PixelRange(6, TEX_W - 7);
        int gy = PixelRange(HEX_OVERHANG_H + 4, TEX_H - 4);
        if (!IsInsideHexBase(gx, gy, NULL)) continue;
        PutPixelInHex(&img, gx,     gy,     bladeDark);
        PutPixelInHex(&img, gx - 1, gy,     bladeDark);
        PutPixelInHex(&img, gx - 1, gy - 1, bladeMid);
        PutPixelInHex(&img, gx,     gy - 1, bladeTip);
        PutPixelInHex(&img, gx + 1, gy - 1, bladeMid);
    }

    /* Guijarros calizos y pequeñas flores silvestres */
    for (int i = 0; i < 10; i++) {
        int sx = PixelRange(10, TEX_W - 11);
        int sy = PixelRange(HEX_OVERHANG_H + 5, TEX_H - 5);
        if (i < 6) {
            PutPixelInHex(&img, sx,     sy, (Color){ 182, 174, 156, 255 });
            PutPixelInHex(&img, sx + 1, sy, (Color){ 136, 128, 112, 255 });
        } else {
            PutPixelInHex(&img, sx, sy, (Color){ 224, 186, 84, 255 });
        }
    }

    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return tex;
}

/* ============================================================
   2. BOSQUE DE OLIVOS Y CIPRESES (forest.png)
   Sotobosque oscuro + copas de árboles pixel-art con relieve overhang
   ============================================================ */

static void DrawPixelTree(Image* img, int cx, int baseY, int radius, int height, bool isCypress)
{
    /* Sombra proyectada en el suelo */
    for (int dy = -1; dy <= 2; dy++) {
        for (int dx = -radius; dx <= radius + 1; dx++) {
            PutPixelInHex(img, cx + dx + 1, baseY + dy, (Color){ 26, 38, 22, 255 });
        }
    }

    /* Tronco nudoso */
    PutPixelWithOverhang(img, cx,     baseY,     (Color){ 68, 46, 30, 255 });
    PutPixelWithOverhang(img, cx,     baseY - 1, (Color){ 88, 60, 38, 255 });
    PutPixelWithOverhang(img, cx - 1, baseY,     (Color){ 52, 34, 22, 255 });

    Color outline = (Color){  22,  34,  20, 255 };
    Color dark    = (Color){  42,  66,  36, 255 };
    Color mid     = (Color){  66,  96,  52, 255 };
    Color light   = (Color){  98, 132,  72, 255 };
    Color hi      = (Color){ 132, 162,  94, 255 };

    if (isCypress) {
        /* Ciprés mediterráneo cónico y esbelto */
        for (int y = 0; y < height; y++) {
            float t = (float)y / (float)height;
            int halfW = (int)roundf((1.0f - t * 0.78f) * (float)radius);
            int py = baseY - 2 - y;
            for (int dx = -halfW - 1; dx <= halfW + 1; dx++) {
                Color col;
                if (abs(dx) > halfW || y == height - 1) col = outline;
                else if (dx < 0 && y > height / 2) col = light;
                else if (dx <= 0) col = mid;
                else col = dark;
                PutPixelWithOverhang(img, cx + dx, py, col);
            }
        }
    } else {
        /* Copa frondosa de olivo / roble griego */
        int cy = baseY - 2 - radius;
        int r2 = radius * radius;
        for (int dy = -radius - 1; dy <= radius + 1; dy++) {
            for (int dx = -radius - 1; dx <= radius + 1; dx++) {
                int d2 = dx * dx + dy * dy;
                if (d2 > (radius + 1) * (radius + 1)) continue;
                Color col;
                if (d2 >= r2 - 1) {
                    col = outline;
                } else {
                    float lightFactor = (-dx * 0.65f - dy * 0.75f) / (float)radius;
                    if (lightFactor > 0.45f) col = hi;
                    else if (lightFactor > 0.05f) col = light;
                    else if (lightFactor > -0.35f) col = mid;
                    else col = dark;
                }
                PutPixelWithOverhang(img, cx + dx, cy + dy, col);
            }
        }
    }
}

static Texture2D GenTileForest(void)
{
    PixelSeed(0x202B3C4Du);
    Image img = GenImageColor(TEX_W, TEX_H, BLANK);

    Color cBase   = (Color){  56,  82,  44, 255 };
    Color cLight  = (Color){  72, 100,  56, 255 };
    Color cShadow = (Color){  42,  64,  34, 255 };
    Color cRim    = (Color){  30,  46,  24, 255 };

    FillHexBaseWithBevel(&img, cBase, cLight, cShadow, cRim);

    /* Textura de hojarasca y musgo en el suelo */
    for (int i = 0; i < 45; i++) {
        int x = PixelRange(5, TEX_W - 6);
        int y = PixelRange(HEX_OVERHANG_H + 2, TEX_H - 3);
        PutPixelInHex(&img, x, y, (i % 2 == 0) ? (Color){ 82, 112, 58, 255 } : (Color){ 36, 54, 28, 255 });
    }

    /* Árboles ordenados de norte a sur (Y menor a Y mayor) con overhang superior */
    struct { int x; int y; int r; int h; bool cypress; } trees[] = {
        { 31, HEX_OVERHANG_H +  4, 4, 12, true  },
        { 22, HEX_OVERHANG_H +  7, 6,  9, false },
        { 41, HEX_OVERHANG_H +  8, 6,  9, false },
        { 14, HEX_OVERHANG_H + 14, 5,  8, false },
        { 32, HEX_OVERHANG_H + 14, 7, 10, false },
        { 49, HEX_OVERHANG_H + 15, 4, 11, true  },
        { 20, HEX_OVERHANG_H + 22, 6,  9, false },
        { 43, HEX_OVERHANG_H + 22, 6,  9, false },
        { 31, HEX_OVERHANG_H + 27, 5,  8, false }
    };
    int treeCount = (int)(sizeof(trees) / sizeof(trees[0]));
    for (int i = 0; i < treeCount; i++) {
        DrawPixelTree(&img, trees[i].x, trees[i].y, trees[i].r, trees[i].h, trees[i].cypress);
    }

    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return tex;
}

/* ============================================================
   3. DESIERTO DE ARENA OCRE Y ROCA TERRACOTA (desert.png)
   Dunas onduladas con sombreado pixel-art y peñascos áridos
   ============================================================ */

static Texture2D GenTileDesert(void)
{
    PixelSeed(0x303C4D5Eu);
    Image img = GenImageColor(TEX_W, TEX_H, BLANK);

    Color cSandBase   = (Color){ 208, 166,  92, 255 };
    Color cSandLight  = (Color){ 228, 190, 116, 255 };
    Color cSandShadow = (Color){ 178, 134,  68, 255 };
    Color cSandRim    = (Color){ 142, 102,  48, 255 };

    FillHexBaseWithBevel(&img, cSandBase, cSandLight, cSandShadow, cSandRim);

    /* Ondas de dunas isométricas con cresta iluminada y vertiente en sombra */
    for (int y = HEX_OVERHANG_H + 2; y < TEX_H - 2; y++) {
        int localY = y - HEX_OVERHANG_H;
        for (int x = 3; x < TEX_W - 3; x++) {
            float edgeDist = 0.0f;
            if (!IsInsideHexBase(x, y, &edgeDist) || edgeDist < 0.05f) continue;

            float wave = sinf((float)x * 0.22f + (float)localY * 0.35f);
            if (wave > 0.65f) {
                ImageDrawPixel(&img, x, y, cSandLight);
            } else if (wave < -0.55f) {
                ImageDrawPixel(&img, x, y, cSandShadow);
            }
        }
    }

    /* Afloramientos de roca rojiza/terracota que sobresalen ligeramente */
    struct { int cx; int baseY; int w; int h; } rocks[] = {
        { 36, HEX_OVERHANG_H + 10, 12, 9 },
        { 20, HEX_OVERHANG_H + 18, 10, 7 },
        { 44, HEX_OVERHANG_H + 23,  8, 6 }
    };
    Color rOutline = (Color){  64,  38,  24, 255 };
    Color rDark    = (Color){ 124,  74,  44, 255 };
    Color rMid     = (Color){ 164, 104,  62, 255 };
    Color rLight   = (Color){ 204, 142,  88, 255 };

    for (int r = 0; r < 3; r++) {
        int cx = rocks[r].cx;
        int by = rocks[r].baseY;
        int rw = rocks[r].w;
        int rh = rocks[r].h;
        for (int dy = 0; dy <= rh; dy++) {
            int halfW = (int)((1.0f - (float)dy / (float)(rh + 2)) * (rw * 0.5f));
            int py = by - dy;
            for (int dx = -halfW - 1; dx <= halfW + 1; dx++) {
                Color col;
                if (abs(dx) > halfW || dy == rh) col = rOutline;
                else if (dx < 0) col = (dy > rh / 2) ? rLight : rMid;
                else col = rDark;
                PutPixelWithOverhang(&img, cx + dx, py, col);
            }
        }
    }

    /* Matorrales secos espinosos */
    for (int i = 0; i < 8; i++) {
        int bx = PixelRange(10, TEX_W - 11);
        int by = PixelRange(HEX_OVERHANG_H + 6, TEX_H - 6);
        PutPixelInHex(&img, bx,     by,     (Color){ 108, 86, 44, 255 });
        PutPixelInHex(&img, bx - 1, by - 1, (Color){ 134, 110, 56, 255 });
        PutPixelInHex(&img, bx + 1, by - 1, (Color){ 134, 110, 56, 255 });
    }

    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return tex;
}

/* ============================================================
   4. ESPESURA FRONDOSA / JUNGLA PANTANOSA (jungle.png)
   Vegetación densa esmeralda-turquesa con charcos y frondas altas
   ============================================================ */

static Texture2D GenTileJungle(void)
{
    PixelSeed(0x404D5E6Fu);
    Image img = GenImageColor(TEX_W, TEX_H, BLANK);

    Color cBase   = (Color){  36,  72,  58, 255 };
    Color cLight  = (Color){  48,  92,  72, 255 };
    Color cShadow = (Color){  26,  54,  44, 255 };
    Color cRim    = (Color){  18,  38,  30, 255 };

    FillHexBaseWithBevel(&img, cBase, cLight, cShadow, cRim);

    /* Pequeñas lagunas / charcos pantanosos en la base */
    for (int dy = -3; dy <= 3; dy++) {
        for (int dx = -7; dx <= 7; dx++) {
            if (dx * dx + dy * dy * 4 <= 45) {
                PutPixelInHex(&img, 32 + dx, HEX_OVERHANG_H + 20 + dy,
                              (dy < 0) ? (Color){ 46, 108, 116, 255 } : (Color){ 64, 134, 142, 255 });
            }
        }
    }

    /* Copas frondosas densas con tonos verde jade y esmeralda */
    struct { int x; int y; int r; } canopies[] = {
        { 26, HEX_OVERHANG_H +  5, 7 },
        { 39, HEX_OVERHANG_H +  6, 7 },
        { 16, HEX_OVERHANG_H + 12, 6 },
        { 32, HEX_OVERHANG_H + 11, 8 },
        { 48, HEX_OVERHANG_H + 13, 6 },
        { 19, HEX_OVERHANG_H + 21, 6 },
        { 44, HEX_OVERHANG_H + 21, 6 },
        { 31, HEX_OVERHANG_H + 26, 5 }
    };

    Color jOut   = (Color){  16,  32,  26, 255 };
    Color jDark  = (Color){  28,  62,  48, 255 };
    Color jMid   = (Color){  46,  96,  72, 255 };
    Color jLight = (Color){  74, 134,  92, 255 };
    Color jHi    = (Color){ 114, 168, 106, 255 };

    for (int c = 0; c < 8; c++) {
        int cx = canopies[c].x;
        int cy = canopies[c].y - canopies[c].r + 2;
        int r  = canopies[c].r;
        for (int dy = -r - 1; dy <= r + 1; dy++) {
            for (int dx = -r - 1; dx <= r + 1; dx++) {
                int d2 = dx * dx + dy * dy;
                if (d2 > (r + 1) * (r + 1)) continue;
                Color col;
                if (d2 >= r * r - 1) col = jOut;
                else {
                    float lf = (-dx * 0.6f - dy * 0.8f) / (float)r;
                    if (lf > 0.45f) col = jHi;
                    else if (lf > 0.05f) col = jLight;
                    else if (lf > -0.35f) col = jMid;
                    else col = jDark;
                }
                PutPixelWithOverhang(&img, cx + dx, cy + dy, col);
            }
        }
    }

    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return tex;
}

/* ============================================================
   5. CORDILLERA CALIZA GRIEGA (mountain.png)
   Macizo rocoso facetado en pixel-art que se eleva en el overhang
   ============================================================ */

static void DrawMountainPeak(Image* img, int apexX, int apexY, int baseY, int halfWidth)
{
    Color cOut     = (Color){  28,  24,  22, 255 };
    Color cShadow2 = (Color){  68,  62,  56, 255 };
    Color cShadow1 = (Color){  94,  86,  78, 255 };
    Color cMid     = (Color){ 128, 118, 106, 255 };
    Color cLight   = (Color){ 166, 154, 138, 255 };
    Color cSnow    = (Color){ 218, 210, 196, 255 };

    int totalH = baseY - apexY;
    if (totalH <= 0) return;

    for (int y = apexY; y <= baseY; y++) {
        float t = (float)(y - apexY) / (float)totalH;
        int curHalfW = (int)roundf(t * (float)halfWidth);
        /* Cresta central quebrada en zig-zag sutil */
        int ridgeOffset = (int)(sinf((float)(y - apexY) * 0.45f) * 2.0f);

        for (int dx = -curHalfW - 1; dx <= curHalfW + 1; dx++) {
            int px = apexX + dx;
            Color col;
            if (abs(dx) >= curHalfW || y == apexY) {
                col = cOut;
            } else if (dx == ridgeOffset) {
                col = (t < 0.35f) ? cSnow : cLight;
            } else if (dx < ridgeOffset) {
                /* Cara occidental iluminada */
                if (t < 0.26f) col = cSnow;
                else if (((px + y) % 5) == 0) col = cMid;
                else col = cLight;
            } else {
                /* Cara oriental en sombra con estratos */
                if (t < 0.20f && dx < ridgeOffset + 3) col = cLight;
                else if ((y % 4) == 0) col = cShadow2;
                else col = cShadow1;
            }
            PutPixelWithOverhang(img, px, y, col);
        }
    }
}

static Texture2D GenTileMountain(void)
{
    PixelSeed(0x505E6F70u);
    Image img = GenImageColor(TEX_W, TEX_H, BLANK);

    Color cBase   = (Color){ 114, 106,  96, 255 };
    Color cLight  = (Color){ 136, 126, 114, 255 };
    Color cShadow = (Color){  88,  82,  74, 255 };
    Color cRim    = (Color){  58,  52,  46, 255 };

    FillHexBaseWithBevel(&img, cBase, cLight, cShadow, cRim);

    /* Tres cumbres escalonadas de norte a sur */
    DrawMountainPeak(&img, 22,  9, HEX_OVERHANG_H + 22, 16);
    DrawMountainPeak(&img, 42, 11, HEX_OVERHANG_H + 24, 15);
    DrawMountainPeak(&img, 32,  3, HEX_OVERHANG_H + 29, 21);

    /* Matorrales de montaña en la falda rocosa */
    for (int i = 0; i < 12; i++) {
        int mx = PixelRange(12, TEX_W - 13);
        int my = PixelRange(HEX_OVERHANG_H + 22, TEX_H - 5);
        PutPixelInHex(&img, mx,     my, (Color){ 76, 94, 52, 255 });
        PutPixelInHex(&img, mx + 1, my, (Color){ 54, 70, 38, 255 });
    }

    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return tex;
}

/* ============================================================
   6. MAR EGEO ANIMADO (water_sheet.png — 4 frames)
   Azul mediterráneo profundo con ondas y espuma pixel-art
   ============================================================ */

static Image GenWaterFrame(int frame)
{
    Image img = GenImageColor(TEX_W, HEX_BASE_H, BLANK);

    Color cDeep   = (Color){  42,  86, 116, 255 };
    Color cMid    = (Color){  56, 108, 142, 255 };
    Color cLight  = (Color){  78, 136, 172, 255 };
    Color cFoam   = (Color){ 176, 214, 232, 255 };
    Color cRim    = (Color){  30,  64,  88, 255 };

    for (int y = 0; y < HEX_BASE_H; y++) {
        for (int x = 0; x < TEX_W; x++) {
            float edgeDist = 0.0f;
            if (!IsInsideHexBase(x, y + HEX_OVERHANG_H, &edgeDist)) continue;

            float phase = (float)frame * (3.14159265f * 0.5f);
            float w1 = sinf((float)x * 0.28f + (float)y * 0.45f + phase);
            float w2 = cosf((float)x * 0.18f - (float)y * 0.35f + phase);
            float v = (w1 + w2) * 0.5f;

            Color c = cMid;
            if (edgeDist < 0.045f) {
                c = cRim;
            } else if (v > 0.68f && (y % 3 == 0)) {
                c = cFoam;
            } else if (v > 0.25f) {
                c = cLight;
            } else if (v < -0.30f) {
                c = cDeep;
            }
            ImageDrawPixel(&img, x, y, c);
        }
    }
    return img;
}

static AnimatedSprite GenWaterAnimated(void)
{
    Image sheet = GenImageColor(TEX_W * WATER_FRAMES, HEX_BASE_H, BLANK);
    for (int f = 0; f < WATER_FRAMES; f++) {
        Image frame = GenWaterFrame(f);
        Rectangle dst = { (float)(f * TEX_W), 0, (float)TEX_W, (float)HEX_BASE_H };
        ImageDraw(&sheet, frame, (Rectangle){ 0, 0, (float)TEX_W, (float)HEX_BASE_H }, dst, WHITE);
        UnloadImage(frame);
    }
    AnimatedSprite spr = { 0 };
    spr.sheet         = LoadTextureFromImage(sheet);
    SetTextureFilter(spr.sheet, TEXTURE_FILTER_POINT);
    spr.frameCount    = WATER_FRAMES;
    spr.currentFrame  = 0;
    spr.frameDuration = 0.28f;
    spr.frameW        = TEX_W;
    spr.frameH        = HEX_BASE_H;
    UnloadImage(sheet);
    return spr;
}

/* ============================================================
   PROCESADOR DE ESTRUCTURAS PIXEL-ART
   Quita el pedestal circular gris de las estructuras de alta resolución,
   escala a STRUCT_W x STRUCT_H con Nearest-Neighbor, cuantiza paleta
   y aplica contorno pixel-art limpio.
   ============================================================ */

static void CleanAndPixelateStructure(Image* img, bool removeCircularPedestal)
{
    if (!img || !img->data) return;

    int srcW = img->width;
    int srcH = img->height;

    if (removeCircularPedestal && srcW > 128) {
        /* Los sprites de estructuras de 512x512 tienen un pedestal circular gris en la base:
         * Centro de la elipse del pedestal ~ (0.50 * W, 0.72 * H), radios (0.40 * W, 0.21 * H).
         * Conservamos el edificio central y eliminamos el anillo exterior de bloques grises. */
        for (int y = 0; y < srcH; y++) {
            float ny = (float)y / (float)srcH;
            for (int x = 0; x < srcW; x++) {
                float nx = (float)x / (float)srcW;
                Color px = GetImageColor(*img, x, y);
                if (px.a < 20) continue;

                /* Distancia normalizada al centro de la base circular */
                float dx = (nx - 0.50f) / 0.36f;
                float dy = (ny - 0.71f) / 0.19f;
                float ellDist = sqrtf(dx * dx + dy * dy);

                /* Detectar si el píxel es piedra grisácea/musgosa del borde del pedestal */
                int maxC = (px.r > px.g) ? ((px.r > px.b) ? px.r : px.b) : ((px.g > px.b) ? px.g : px.b);
                int minC = (px.r < px.g) ? ((px.r < px.b) ? px.r : px.b) : ((px.g < px.b) ? px.g : px.b);
                int sat = maxC - minC;
                bool isGrayOrMoss = (sat < 38) || (px.g > px.r + 8 && px.g > px.b + 12 && ny > 0.65f);

                /* Recortar el anillo exterior del pedestal (ellDist > 0.76 en la parte baja) */
                if (ny > 0.83f || (ny > 0.58f && ellDist > 0.80f && isGrayOrMoss) || (ny > 0.76f && ellDist > 0.68f)) {
                    ImageDrawPixel(img, x, y, BLANK);
                }
            }
        }
    }

    /* Redimensionar con Nearest-Neighbor a STRUCT_W x STRUCT_H */
    ImageResizeNN(img, STRUCT_W, STRUCT_H);

    /* Cuantización de color (posterización suave a 24 niveles por canal) y umbral alfa nítido */
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            Color c = GetImageColor(*img, x, y);
            if (c.a < 96) {
                ImageDrawPixel(img, x, y, BLANK);
            } else {
                c.r = (unsigned char)((c.r / 12) * 12 + 6);
                c.g = (unsigned char)((c.g / 12) * 12 + 6);
                c.b = (unsigned char)((c.b / 12) * 12 + 6);
                c.a = 255;
                ImageDrawPixel(img, x, y, c);
            }
        }
    }

    /* Añadir contorno exterior oscuro de 1px (#1A1614) para cohesión total con los iconos de UI */
    Image copy = ImageCopy(*img);
    Color outlineCol = (Color){ 26, 22, 20, 255 };
    for (int y = 1; y < img->height - 1; y++) {
        for (int x = 1; x < img->width - 1; x++) {
            Color c = GetImageColor(copy, x, y);
            if (c.a > 0) continue;
            if (GetImageColor(copy, x - 1, y).a > 0 ||
                GetImageColor(copy, x + 1, y).a > 0 ||
                GetImageColor(copy, x, y - 1).a > 0 ||
                GetImageColor(copy, x, y + 1).a > 0) {
                ImageDrawPixel(img, x, y, outlineCol);
            }
        }
    }
    UnloadImage(copy);
}

/* Fallbacks procedurales si no existiera el archivo */
static Texture2D GenStructFallback(Color mainCol, Color roofCol)
{
    Image img = GenImageColor(STRUCT_W, STRUCT_H, BLANK);
    ImageDrawRectangle(&img, 18, 30, 36, 32, mainCol);
    ImageDrawTriangle(&img, (Vector2){ 36, 12 }, (Vector2){ 14, 32 }, (Vector2){ 58, 32 }, roofCol);
    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return tex;
}

static Texture2D LoadAndProcessStructure(const char* path, bool removePedestal, Color fbMain, Color fbRoof)
{
    if (FileExists(path)) {
        Image img = LoadImage(path);
        if (img.data != NULL) {
            /* Si todavía está en alta resolución (> 128px), limpiamos pedestal, pixelamos y guardamos */
            if (img.width > 128 || img.height > 128) {
                CleanAndPixelateStructure(&img, removePedestal);
                ExportImage(img, path);
            }
            Texture2D tex = LoadTextureFromImage(img);
            SetTextureFilter(tex, TEXTURE_FILTER_POINT);
            UnloadImage(img);
            return tex;
        }
    }
    return GenStructFallback(fbMain, fbRoof);
}

/* ============================================================
   TOKENS DE EJÉRCITO Y ENEMIGO (PIXEL-ART)
   ============================================================ */

static Texture2D GenEnemyToken(void)
{
    Image img = GenImageColor(48, 48, BLANK);
    ImageDrawCircle(&img, 24, 24, 20, (Color){ 155, 35, 35, 255 });
    ImageDrawCircleLines(&img, 24, 24, 20, (Color){ 26, 22, 20, 255 });
    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return tex;
}

static Texture2D GenArmyToken(void)
{
    Image img = GenImageColor(48, 48, BLANK);
    /* Escudo hoplita de bronce */
    ImageDrawCircle(&img, 24, 26, 16, (Color){ 26, 22, 20, 255 });
    ImageDrawCircle(&img, 24, 26, 15, (Color){ 196, 154, 68, 255 });
    ImageDrawCircle(&img, 24, 26, 12, (Color){ 154,  52, 42, 255 });
    /* Casco corintio con penacho rojo */
    ImageDrawCircle(&img, 24, 14,  7, (Color){ 216, 174, 84, 255 });
    ImageDrawRectangle(&img, 20, 6, 8, 4, (Color){ 195, 48, 42, 255 });
    /* Lanza dorada */
    ImageDrawLine(&img, 36, 6, 36, 42, (Color){ 220, 210, 185, 255 });
    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return tex;
}

static Texture2D GenerateAndSaveTile(const char* path, Texture2D (*genFn)(void))
{
    /* Si ya existe un tile con exactamente nuestras dimensiones canónicas (TEX_W x TEX_H), cargarlo */
    if (FileExists(path)) {
        Image existing = LoadImage(path);
        if (existing.data != NULL && existing.width == TEX_W && existing.height == TEX_H) {
            Texture2D tex = LoadTextureFromImage(existing);
            SetTextureFilter(tex, TEXTURE_FILTER_POINT);
            UnloadImage(existing);
            return tex;
        }
        if (existing.data != NULL) UnloadImage(existing);
    }

    Texture2D tex = genFn();
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

    /* Biomas Pixel-Art con máscara hexagonal exacta (64x37) + overhang superior (23px) */
    assets->tilePlains   = GenerateAndSaveTile("assets/tiles/plains.png",   GenTilePlains);
    assets->tileForest   = GenerateAndSaveTile("assets/tiles/forest.png",   GenTileForest);
    assets->tileDesert   = GenerateAndSaveTile("assets/tiles/desert.png",   GenTileDesert);
    assets->tileJungle   = GenerateAndSaveTile("assets/tiles/jungle.png",   GenTileJungle);
    assets->tileMountain = GenerateAndSaveTile("assets/tiles/mountain.png", GenTileMountain);

    /* Agua animada Pixel-Art (4 frames de 64x37) */
    bool needWaterGen = true;
    if (FileExists("assets/tiles/water_sheet.png")) {
        Image sheet = LoadImage("assets/tiles/water_sheet.png");
        if (sheet.data != NULL && sheet.width == TEX_W * WATER_FRAMES && sheet.height == HEX_BASE_H) {
            assets->waterAnim.sheet         = LoadTextureFromImage(sheet);
            SetTextureFilter(assets->waterAnim.sheet, TEXTURE_FILTER_POINT);
            assets->waterAnim.frameCount    = WATER_FRAMES;
            assets->waterAnim.frameW        = TEX_W;
            assets->waterAnim.frameH        = HEX_BASE_H;
            assets->waterAnim.frameDuration = 0.28f;
            needWaterGen = false;
        }
        if (sheet.data != NULL) UnloadImage(sheet);
    }
    if (needWaterGen) {
        assets->waterAnim = GenWaterAnimated();
        Image sheet = LoadImageFromTexture(assets->waterAnim.sheet);
        ExportImage(sheet, "assets/tiles/water_sheet.png");
        UnloadImage(sheet);
    }
    assets->tileWater = assets->waterAnim.sheet;

    /* Estructuras Pixel-Art sin pedestal circular */
    assets->structAltar      = LoadAndProcessStructure("assets/structures/altar.png",      true,  (Color){210,200,185,255}, (Color){215,175,75,255});
    assets->structCastle     = LoadAndProcessStructure("assets/structures/castle.png",     true,  (Color){175,165,150,255}, (Color){165,75,55,255});
    assets->structMarket     = LoadAndProcessStructure("assets/structures/market.png",     true,  (Color){165,125,80,255},  (Color){195,85,65,255});
    assets->structArmory     = LoadAndProcessStructure("assets/structures/armory.png",     true,  (Color){145,135,125,255}, (Color){155,65,50,255});
    assets->structHouses     = LoadAndProcessStructure("assets/structures/houses.png",     true,  (Color){195,180,155,255}, (Color){175,75,55,255});
    assets->structWatchtower = LoadAndProcessStructure("assets/structures/watchtower.png", true,  (Color){165,155,140,255}, (Color){155,70,50,255});
    assets->structBridge     = LoadAndProcessStructure("assets/structures/bridge.png",     false, (Color){165,145,115,255}, (Color){125,110,90,255});

    /* Tokens de unidades */
    if (FileExists("assets/units/army_token.png")) {
        assets->armyToken = LoadTexture("assets/units/army_token.png");
        SetTextureFilter(assets->armyToken, TEXTURE_FILTER_POINT);
    } else {
        assets->armyToken = GenArmyToken();
    }
    if (FileExists("assets/units/enemy_token.png")) {
        assets->enemyToken = LoadTexture("assets/units/enemy_token.png");
        SetTextureFilter(assets->enemyToken, TEXTURE_FILTER_POINT);
    } else {
        assets->enemyToken = GenEnemyToken();
    }

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
