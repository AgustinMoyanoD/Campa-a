#include "map_view.h"
#include "controllers/game_master.h"
#include "controllers/camera_controller.h"
#include "presentation/ui_widgets.h"
#include "presentation/tween.h"
#include "presentation/sequence.h"
#include "presentation/audio_manager.h"
#include "presentation/screen_fx.h"
#include "raymath.h"
#include "rlgl.h"
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

/* ============================================================
   PERSPECTIVA ISOMï¿½TRICA
   Factor de aplastamiento 2:1. El eje Y se multiplica por 0.5
   para dar la ilusiï¿½n de profundidad.
   ============================================================ */

#define ISO_SCALE_Y 0.5f

Vector2 HexToPixel(HexCoord coord, float radius)
{
    Vector2 position;
    position.x = radius * sqrtf(3.0f) * ((float)coord.q + (float)coord.r / 2.0f);
    position.y = (radius * 1.5f * (float)coord.r) * ISO_SCALE_Y;
    return position;
}

HexCoord PixelToHex(Vector2 position, float radius)
{
    /* Revertir el aplastamiento del eje Y antes de la conversiï¿½n inversa */
    float unscaledY = position.y / ISO_SCALE_Y;

    float q = (sqrtf(3.0f) / 3.0f * position.x - 1.0f / 3.0f * unscaledY) / radius;
    float r = (2.0f / 3.0f * unscaledY) / radius;

    float x = q, z = r, y = -x - z;

    int rx = (int)roundf(x);
    int ry = (int)roundf(y);
    int rz = (int)roundf(z);

    float dx = fabsf((float)rx - x);
    float dy = fabsf((float)ry - y);
    float dz = fabsf((float)rz - z);

    if      (dx > dy && dx > dz) rx = -ry - rz;
    else if (dy > dz)            ry = -rx - rz;
    else                         rz = -rx - ry;

    HexCoord coord = { rx, rz };
    return coord;
}

/* ============================================================
   HELPERS DE BIOMA / ALTURA
   ============================================================ */

float GetBiomeHeight(BiomeType biome)
{
    switch (biome) {
        case BIOME_WATER:    return  2.0f;
        case BIOME_PLAINS:   return  9.0f;
        case BIOME_DESERT:   return 10.0f;
        case BIOME_JUNGLE:   return 11.0f;
        case BIOME_FOREST:   return 12.0f;
        case BIOME_MOUNTAIN: return 20.0f;
        default:             return  9.0f;
    }
}

/* Color de cara lateral (acorde con la paleta de Edad del Bronce y Grecia ClÃ¡sica) */
static Color GetBiomeSideColor(BiomeType biome)
{
    switch (biome) {
        case BIOME_WATER:    return (Color){  36,  72,  98, 255 };
        case BIOME_FOREST:   return (Color){  62,  76,  48, 255 };
        case BIOME_DESERT:   return (Color){ 168, 122,  68, 255 };
        case BIOME_JUNGLE:   return (Color){  42,  68,  52, 255 };
        case BIOME_MOUNTAIN: return (Color){ 102,  94,  86, 255 };
        default:             return (Color){ 116, 102,  66, 255 };
    }
}

/* Color de cara superior (fallback sin textura y minimapa) */
static Color GetBiomeTopColor(BiomeType biome)
{
    switch (biome) {
        case BIOME_WATER:    return (Color){  56, 108, 142, 255 };
        case BIOME_FOREST:   return (Color){  56,  82,  44, 255 };
        case BIOME_DESERT:   return (Color){ 208, 166,  92, 255 };
        case BIOME_JUNGLE:   return (Color){  36,  72,  58, 255 };
        case BIOME_MOUNTAIN: return (Color){ 128, 118, 106, 255 };
        default:             return (Color){ 134, 152,  78, 255 };
    }
}

/* ============================================================
   DIBUJO DEL BLOQUE ISOMï¿½TRICO (caras laterales + cara superior)
   ============================================================ */

/*
 * Vï¿½rtices del hexï¿½gono "pointy-top" aplastado:
 *
 *        0
 *      /   \
 *    5       1
 *    |       |
 *    4       2
 *      \   /
 *        3
 *
 * Con ISO_SCALE_Y las coordenadas Y se comprimen al 50%.
 * Los lados visibles desde la perspectiva sur son: 4-5-0, 0-1-2.
 * Dibujamos los quads 4?5?0 y 0?1?2 como caras laterales.
 */

static void ComputeHexVertices(Vector2 center, float radius, Vector2 pts[6])
{
    for (int i = 0; i < 6; i++) {
        float angle = (30.0f + i * 60.0f) * DEG2RAD;
        pts[i].x = center.x + radius * cosf(angle);
        pts[i].y = center.y + (radius * sinf(angle)) * ISO_SCALE_Y;
    }
}

static void DrawHexTop(Vector2 center, float radius,
                        Color topColor, Texture2D tex, Rectangle srcFrame,
                        bool useTexture, Color tint)
{
    Vector2 pts[6];
    ComputeHexVertices(center, radius, pts);

    if (useTexture && tex.id != 0) {
        float minX =  1e9f, maxX = -1e9f;
        float minY =  1e9f, maxY = -1e9f;
        for (int i = 0; i < 6; i++) {
            if (pts[i].x < minX) minX = pts[i].x;
            if (pts[i].x > maxX) maxX = pts[i].x;
            if (pts[i].y < minY) minY = pts[i].y;
            if (pts[i].y > maxY) maxY = pts[i].y;
        }

        float hexW = maxX - minX;
        float hexH = maxY - minY;

        /* Soporte de relieve superior (Overhang):
         * La base hexagonal canÃ³nica mide 64x37 px. Si la textura es mÃ¡s alta (ej. 64x60),
         * los pÃ­xeles extra corresponden a copas de Ã¡rboles o cumbres que sobresalen hacia arriba. */
        float baseTexH = srcFrame.width * (37.0f / 64.0f);
        float overhangTexH = (srcFrame.height > baseTexH + 0.5f) ? (srcFrame.height - baseTexH) : 0.0f;
        float overhangScreenH = (baseTexH > 0.0f) ? (overhangTexH * (hexH / baseTexH)) : 0.0f;

        Rectangle dest = {
            roundf(minX),
            roundf(minY - overhangScreenH),
            roundf(hexW),
            roundf(hexH + overhangScreenH)
        };
        DrawTexturePro(tex, srcFrame, dest, (Vector2){ 0, 0 }, 0.0f, tint);

        /* Aristas inferiores sutiles (2->1 y 1->0) para separar la cara superior del bloque lateral */
        Color edgeCol = (Color){ 26, 22, 20, 145 };
        DrawLineV(pts[2], pts[1], edgeCol);
        DrawLineV(pts[1], pts[0], edgeCol);
    } else {
        rlBegin(RL_TRIANGLES);
        Color finalCol = { (unsigned char)(topColor.r * tint.r / 255.0f),
                           (unsigned char)(topColor.g * tint.g / 255.0f),
                           (unsigned char)(topColor.b * tint.b / 255.0f),
                           (unsigned char)(topColor.a * tint.a / 255.0f) };
        rlColor4ub(finalCol.r, finalCol.g, finalCol.b, finalCol.a);

        for (int i = 0; i < 4; i++) {
            rlVertex2f(center.x, center.y);
            rlVertex2f(pts[i].x, pts[i].y);
            rlVertex2f(pts[i + 1].x, pts[i + 1].y);
        }
        rlVertex2f(center.x, center.y);
        rlVertex2f(pts[4].x, pts[4].y);
        rlVertex2f(pts[5].x, pts[5].y);
        rlVertex2f(center.x, center.y);
        rlVertex2f(pts[5].x, pts[5].y);
        rlVertex2f(pts[0].x, pts[0].y);
        rlEnd();

        for (int i = 0; i < 6; i++)
            DrawLineV(pts[i], pts[(i + 1) % 6], (Color){ 26, 22, 20, 60 });
    }
}

/* Dibuja el cuerpo lateral (prisma) del bloque isomï¿½trico */
/* Dibuja un cuadrilÃ¡tero relleno usando dos triÃ¡ngulos en ambos sentidos de giro
   para evitar que el backface culling de rlgl descarte caras laterales */
static void DrawQuadSolid(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color col)
{
    DrawTriangle(a, b, c, col);
    DrawTriangle(a, c, b, col);
    DrawTriangle(a, c, d, col);
    DrawTriangle(a, d, c, col);
}

static void DrawHexSides(Vector2 baseCenter, float radius,
                          float height, Color sideColor)
{
    if (height <= 0.0f) return;

    Vector2 base[6], top[6], lip[6];
    ComputeHexVertices(baseCenter, radius, base);
    float lipH = (height > 6.0f) ? 3.5f : (height * 0.4f);
    for (int i = 0; i < 6; i++) {
        top[i].x = base[i].x;
        top[i].y = base[i].y - height;
        lip[i].x = base[i].x;
        lip[i].y = top[i].y + lipH;
    }

    /* En nuestro sistema de Ã¡ngulos (30Â° + i*60Â° con Y hacia abajo):
     * VÃ©rtice 0 = 30Â°  (abajo-derecha)
     * VÃ©rtice 1 = 90Â°  (abajo-centro / punta sur)
     * VÃ©rtice 2 = 150Â° (abajo-izquierda)
     * Por lo tanto, las dos caras visibles desde el sur son:
     *   - Cara frontal izquierda: vÃ©rtices 2 -> 1
     *   - Cara frontal derecha:   vÃ©rtices 1 -> 0 */
    Color lipLeft  = (Color){ (unsigned char)(sideColor.r * 0.94f),
                              (unsigned char)(sideColor.g * 0.94f),
                              (unsigned char)(sideColor.b * 0.94f), 255 };
    Color lipRight = (Color){ (unsigned char)(sideColor.r * 0.78f),
                              (unsigned char)(sideColor.g * 0.78f),
                              (unsigned char)(sideColor.b * 0.78f), 255 };
    Color rockLeft = (Color){ (unsigned char)(sideColor.r * 0.74f),
                              (unsigned char)(sideColor.g * 0.70f),
                              (unsigned char)(sideColor.b * 0.66f), 255 };
    Color rockRight= (Color){ (unsigned char)(sideColor.r * 0.58f),
                              (unsigned char)(sideColor.g * 0.55f),
                              (unsigned char)(sideColor.b * 0.52f), 255 };

    /* Cara frontal izquierda (2 -> 1) */
    DrawQuadSolid(top[2],  top[1],  lip[1],  lip[2],  lipLeft);
    DrawQuadSolid(lip[2],  lip[1],  base[1], base[2], rockLeft);

    /* Cara frontal derecha (1 -> 0) */
    DrawQuadSolid(top[1],  top[0],  lip[0],  lip[1],  lipRight);
    DrawQuadSolid(lip[1],  lip[0],  base[0], base[1], rockRight);

    /* Aristas de contorno en piedra oscura cÃ¡lida (#1A1614) */
    Color outline = (Color){ 26, 22, 20, 230 };
    DrawLineV(top[2],  base[2],  outline);
    DrawLineV(top[1],  base[1],  outline);
    DrawLineV(top[0],  base[0],  outline);
    DrawLineV(base[2], base[1],  outline);
    DrawLineV(base[1], base[0],  outline);
}

/* ============================================================
   DIBUJO DE ESTRUCTURAS CON TEXTURA (o fallback geomï¿½trico)
   ============================================================ */

static void DrawStructure(StructureType type, Vector2 topPos, float radius, bool isPrimary, bool isGuarded, const GameAssets* assets, Color tint) {
    Texture2D tex = { 0 };
    bool hasTex = (assets && assets->loaded);

    /* Seleccionar textura */
    if (hasTex) {
        switch (type) {
            case STRUCT_ALTAR:        tex = assets->structAltar;      break;
            case STRUCT_TOWN_CASTLE:  tex = assets->structCastle;     break;
            case STRUCT_TOWN_MARKET:  tex = assets->structMarket;     break;
            case STRUCT_TOWN_ARMORY:  tex = assets->structArmory;     break;
            case STRUCT_TOWN_HOUSES:  tex = assets->structHouses;     break;
            case STRUCT_WATCHTOWER:   tex = assets->structWatchtower; break;
            case STRUCT_BRIDGE:       tex = assets->structBridge;     break;
            default: break;
        }
    }

    if (tex.id != 0) {
        float tw = (float)tex.width;
        float th = (float)tex.height;

        if (type == STRUCT_BRIDGE) {
            /* El puente se dibuja plano SOBRE la cara superior del tile,
             * centrado tanto horizontal como verticalmente. */
            float scale = (radius * 1.35f) / tw;
            float dw = roundf(tw * scale);
            float dh = roundf(th * scale);
            Rectangle src = { 0, 0, tw, th };
            Rectangle dst = { roundf(topPos.x - dw / 2.0f),
                              roundf(topPos.y - dh / 2.0f),
                              dw, dh };
            DrawTexturePro(tex, src, dst, (Vector2){ 0, 0 }, 0.0f, tint);
        } else {
            /* Sombra elÃ­ptica cÃ¡lida para asentar la estructura sin pedestal circular */
            DrawEllipse((int)topPos.x, (int)(topPos.y + 1.0f), radius * 0.48f, radius * 0.20f, (Color){ 18, 14, 12, 135 });

            /* Estructuras que se "paran" sobre el tile (torres, casas, etc.) */
            float scale = (radius * 1.55f) / tw;
            float dw = roundf(tw * scale);
            float dh = roundf(th * scale);
            Rectangle src = { 0, 0, tw, th };
            Rectangle dst = { roundf(topPos.x - dw / 2.0f), roundf(topPos.y - dh + radius * 0.22f), dw, dh };
            DrawTexturePro(tex, src, dst, (Vector2){ 0, 0 }, 0.0f, tint);
        }

    } else {
        /* Fallback geomï¿½trico del sistema anterior */
        switch (type) {
            case STRUCT_ALTAR:
                DrawRectangle((int)(topPos.x - radius*0.22f), (int)(topPos.y - radius*0.4f),
                              (int)(radius*0.44f), (int)(radius*0.4f), (Color){90,85,100,255});
                DrawCircle((int)topPos.x, (int)(topPos.y - radius*0.5f), radius*0.15f, PURPLE);
                break;
            case STRUCT_TOWN_CASTLE:
                DrawRectangle((int)(topPos.x - radius*0.3f), (int)(topPos.y - radius*0.6f),
                              (int)(radius*0.6f), (int)(radius*0.6f), (Color){75,75,82,255});
                if (isPrimary)
                    DrawTriangle((Vector2){topPos.x, topPos.y - radius*0.9f},
                                 (Vector2){topPos.x - radius*0.1f, topPos.y - radius*0.6f},
                                 (Vector2){topPos.x + radius*0.1f, topPos.y - radius*0.6f}, GOLD);
                break;
            case STRUCT_TOWN_MARKET:
                DrawTriangle((Vector2){topPos.x, topPos.y - radius*0.5f},
                             (Vector2){topPos.x - radius*0.25f, topPos.y},
                             (Vector2){topPos.x + radius*0.25f, topPos.y}, ORANGE);
                break;
            case STRUCT_TOWN_ARMORY:
                DrawPoly((Vector2){topPos.x, topPos.y - radius*0.2f}, 3, radius*0.3f, 180.0f,
                         (Color){65,85,125,255});
                break;
            case STRUCT_TOWN_HOUSES:
                DrawRectangle((int)(topPos.x - radius*0.2f), (int)(topPos.y - radius*0.3f),
                              (int)(radius*0.4f), (int)(radius*0.3f), (Color){190,150,110,255});
                DrawTriangle((Vector2){topPos.x, topPos.y - radius*0.5f},
                             (Vector2){topPos.x - radius*0.2f, topPos.y - radius*0.3f},
                             (Vector2){topPos.x + radius*0.2f, topPos.y - radius*0.3f}, MAROON);
                break;
            case STRUCT_WATCHTOWER:
                DrawRectangle((int)(topPos.x - radius*0.1f), (int)(topPos.y - radius*0.7f),
                              (int)(radius*0.2f), (int)(radius*0.7f), (Color){80,75,70,255});
                DrawTriangle((Vector2){topPos.x, topPos.y - radius*0.9f},
                             (Vector2){topPos.x - radius*0.15f, topPos.y - radius*0.7f},
                             (Vector2){topPos.x + radius*0.15f, topPos.y - radius*0.7f}, DARKBROWN);
                break;
            case STRUCT_SIGNPOST:
                /* Poste vertical de madera */
                DrawRectangle((int)(topPos.x - radius * 0.05f), (int)(topPos.y - radius * 0.5f),
                              (int)(radius * 0.1f), (int)(radius * 0.5f), (Color){110, 75, 45, 255});
                /* Cartel / flecha horizontal de madera */
                DrawRectangle((int)(topPos.x - radius * 0.25f), (int)(topPos.y - radius * 0.48f),
                              (int)(radius * 0.5f), (int)(radius * 0.18f), (Color){185, 140, 85, 255});
                /* Punta de flecha indicadora */
                DrawTriangle((Vector2){topPos.x + radius * 0.38f, topPos.y - radius * 0.39f},
                             (Vector2){topPos.x + radius * 0.25f, topPos.y - radius * 0.48f},
                             (Vector2){topPos.x + radius * 0.25f, topPos.y - radius * 0.30f}, (Color){185, 140, 85, 255});
                /* Contorno */
                DrawRectangleLines((int)(topPos.x - radius * 0.25f), (int)(topPos.y - radius * 0.48f),
                                   (int)(radius * 0.5f), (int)(radius * 0.18f), (Color){80, 50, 30, 255});
                break;
            default: break;
        }
    }

    /* Indicador de custodia */
    if (isGuarded)
        DrawCircle((int)(topPos.x + radius*0.3f), (int)(topPos.y - radius*0.5f),
                   radius*0.1f, RED);
}

/* ============================================================
   Z-SORTING  (Algoritmo del Pintor)
   ============================================================ */

typedef struct {
    const HexTile* tile;
    Vector2        basePos; /* Centro del bloque a nivel del suelo */
    float          height;
} RenderNode;

static int CompareRenderNodes(const void* a, const void* b)
{
    const RenderNode* na = (const RenderNode*)a;
    const RenderNode* nb = (const RenderNode*)b;
    if (na->basePos.y < nb->basePos.y) return -1;
    if (na->basePos.y > nb->basePos.y) return  1;
    if (na->basePos.x < nb->basePos.x) return -1;
    if (na->basePos.x > nb->basePos.x) return  1;
    return 0;
}

static int HexDistance(HexCoord a, HexCoord b)
{
    int dq = abs(a.q - b.q);
    int dr = abs(a.r - b.r);
    int ds = abs((a.q + a.r) - (b.q + b.r));
    return (dq + dr + ds) / 2;
}

static bool IsTileInRange(const HexTile* tile, HexCoord center, int renderRadius)
{
    if (renderRadius <= 0) return true;
    return HexDistance(tile->coord, center) <= renderRadius;
}

/* ============================================================
   FUNCIï¿½N PRINCIPAL DE DIBUJADO
   ============================================================ */

void MapView_Draw(const Map* map, float radius, Vector2 origin, bool drawConnections, HexCoord viewCenter, int renderRadius, Vector2 armyPos, const GameAssets* assets, bool debugRevealMap) {
    if (!map) return;

    bool hasAssets = (assets && assets->loaded);

    /* 1. Recolectar tiles visibles */
    RenderNode* nodes = (RenderNode*)malloc(sizeof(RenderNode) * map->tileCount);
    if (!nodes) return;
    int count = 0;

    for (int i = 0; i < map->tileCount; i++) {
        if (!IsTileInRange(&map->tiles[i], viewCenter, renderRadius)) continue;
        nodes[count].tile    = &map->tiles[i];
        Vector2 pos = HexToPixel(map->tiles[i].coord, radius);
        pos.x += origin.x;
        pos.y += origin.y;
        nodes[count].basePos = pos;
        nodes[count].height  = GetBiomeHeight(map->tiles[i].biome);
        count++;
    }

    /* 2. Ordenar atrï¿½s?adelante (Y menor = mï¿½s al norte = se dibuja antes) */
    qsort(nodes, count, sizeof(RenderNode), CompareRenderNodes);

    /* 3. Dibujar cada tile */
    for (int i = 0; i < count; i++) {
        const HexTile* t  = nodes[i].tile;
        if (!t->visited && !debugRevealMap) continue;

        Vector2 basePos   = nodes[i].basePos;
        float   h         = nodes[i].height;
        Vector2 topPos    = { basePos.x, basePos.y - h };
        Color   sideColor = GetBiomeSideColor(t->biome);
        Color   topColor  = GetBiomeTopColor(t->biome);

        Color tint = WHITE;
        if (!t->visible && !debugRevealMap) {
            tint = (Color){ 80, 80, 85, 255 };
            sideColor.r = (unsigned char)(sideColor.r * 0.4f);
            sideColor.g = (unsigned char)(sideColor.g * 0.4f);
            sideColor.b = (unsigned char)(sideColor.b * 0.4f);
            topColor.r = (unsigned char)(topColor.r * 0.4f);
            topColor.g = (unsigned char)(topColor.g * 0.4f);
            topColor.b = (unsigned char)(topColor.b * 0.4f);
        }

        /* --- Cara lateral --- */
        DrawHexSides(basePos, radius, h, sideColor);

        /* --- Cara superior --- */
        bool isWater = (t->biome == BIOME_WATER);
        Texture2D tileTex = { 0 };
        Rectangle srcFrame = { 0, 0, 0, 0 };

        if (hasAssets) {
            if (isWater) {
                tileTex  = assets->waterAnim.sheet;
                srcFrame = AnimSprite_GetFrame(&assets->waterAnim);
            } else {
                tileTex  = Assets_GetBiomeTile(assets, t->biome);
                srcFrame = (Rectangle){ 0, 0,
                    (float)tileTex.width, (float)tileTex.height };
            }
        }

        DrawHexTop(topPos, radius, topColor, tileTex, srcFrame, hasAssets && tileTex.id != 0, tint);

        /* -- Rios entre vecinos -- */
        if (t->river == RIVER_PRESENT) {
            DrawCircleV(topPos, radius * 0.10f, (Color){ 62, 118, 154, 255 });
            for (int d = 0; d < 6; d++) {
                const HexTile* nb = t->neighbors[d];
                if (!nb || nb->river != RIVER_PRESENT) continue;
                Vector2 nBase = HexToPixel(nb->coord, radius);
                nBase.x += origin.x; nBase.y += origin.y;
                Vector2 nTop = { nBase.x, nBase.y - GetBiomeHeight(nb->biome) };
                Vector2 mid  = { (topPos.x + nTop.x) * 0.5f,
                                 (topPos.y + nTop.y) * 0.5f };

                /* Ribera oscura + cauce azul Egeo + reflejo central */
                DrawLineEx(topPos, mid, radius * 0.28f, (Color){  34,  58,  74, 255 });
                DrawLineEx(topPos, mid, radius * 0.18f, (Color){  62, 118, 154, 255 });
                DrawLineEx(topPos, mid, radius * 0.07f, (Color){ 136, 186, 212, 220 });
            }
        }

        /* -- Caminos entre vecinos -- */
        if (t->isRoad) {
            bool isTileMain = (t->road == ROAD_MAIN);
            float hubR      = isTileMain ? (radius * 0.24f * 0.5f) : (radius * 0.14f * 0.5f);
            Color hubBorder = isTileMain ? (Color){  68,  48,  32, 255 } : (Color){  84,  62,  40, 240 };
            Color hubFill   = isTileMain ? (Color){ 168, 136,  92, 255 } : (Color){ 186, 152, 102, 255 };

            /* Hub de union en el centro del hexagono para intersecciones y giros limpios */
            DrawCircleV(topPos, hubR * 1.35f, hubBorder);
            DrawCircleV(topPos, hubR, hubFill);

            for (int d = 0; d < 6; d++) {
                const HexTile* nb = t->neighbors[d];
                if (!nb || !nb->isRoad) continue;
                Vector2 nBase = HexToPixel(nb->coord, radius);
                nBase.x += origin.x; nBase.y += origin.y;
                Vector2 nTop = { nBase.x, nBase.y - GetBiomeHeight(nb->biome) };
                Vector2 mid  = { (topPos.x + nTop.x) * 0.5f,
                                 (topPos.y + nTop.y) * 0.5f };

                bool isMain = (t->road == ROAD_MAIN && nb->road == ROAD_MAIN);
                if (isMain) {
                    /* Calzada real empedrada: bordillo oscuro + superficie de piedra calida + eje central */
                    DrawLineEx(topPos, mid, radius * 0.26f, (Color){  68,  48,  32, 255 });
                    DrawLineEx(topPos, mid, radius * 0.18f, (Color){ 168, 136,  92, 255 });
                    DrawLineEx(topPos, mid, radius * 0.07f, (Color){ 212, 184, 136, 255 });
                } else {
                    /* Sendero secundario de tierra apisonada / grava */
                    DrawLineEx(topPos, mid, radius * 0.16f, (Color){  84,  62,  40, 240 });
                    DrawLineEx(topPos, mid, radius * 0.11f, (Color){ 186, 152, 102, 255 });
                }
            }
        }
        /* -- Estructuras -- */
        if (t->structure.type != STRUCT_NONE)
            DrawStructure(t->structure.type, topPos, radius, t->structure.settlementType == SETTLEMENT_PRIMARY, t->structure.isGuarded, assets, tint);

        /* -- Marcadores de inicio / fin -- */
        if (t->special == TILE_SPECIAL_START) {
            DrawCircleV(topPos, radius * 0.26f, (Color){ 26, 22, 20, 220 });
            DrawCircleV(topPos, radius * 0.20f, (Color){ 216, 174, 72, 255 });
        } else if (t->special == TILE_SPECIAL_END) {
            DrawCircleV(topPos, radius * 0.26f, (Color){ 26, 22, 20, 220 });
            DrawCircleV(topPos, radius * 0.20f, (Color){ 186, 56, 48, 255 });
        }

                /* -- Token removido del loop -- */
    }

    /* -- Token del ejï¿½rcito Interpolado -- */
    Vector2 topPos = armyPos;
    topPos.x += origin.x;
    topPos.y += origin.y;
    
    float currentHeight = 0.0f;
    for (int i=0; i<map->tileCount; i++) {
        Vector2 p = HexToPixel(map->tiles[i].coord, radius);
        float dx = armyPos.x - p.x;
        float dy = armyPos.y - p.y;
        if (dx*dx + dy*dy < radius*radius) {
            currentHeight = GetBiomeHeight(map->tiles[i].biome);
            break;
        }
    }
    topPos.y -= currentHeight;

    /* Sombra de suelo para asentar el token tridimensionalmente */
    DrawEllipse((int)topPos.x, (int)(topPos.y - 2.0f), radius * 0.44f, radius * 0.20f, (Color){ 0, 0, 0, 110 });

    if (hasAssets && assets->armyToken.id != 0) {
        float tw = (float)assets->armyToken.width;
        float th = (float)assets->armyToken.height;
        float breath = sinf((float)GetTime() * 3.5f) * 0.035f;
        float scale = (radius * (1.6f + breath)) / tw;
        Rectangle src = { 0, 0, tw, th };
        Rectangle dst = { topPos.x - tw*scale/2.0f, topPos.y - th*scale - radius*0.1f, tw*scale, th*scale };
        DrawTexturePro(assets->armyToken, src, dst, (Vector2){0,0}, 0.0f, WHITE);
    } else {
        DrawCircleV((Vector2){ topPos.x, topPos.y - radius*0.4f }, radius * 0.4f, (Color){ 200,150,50,255 });
        DrawCircleLines((int)topPos.x, (int)(topPos.y - radius*0.4f), radius * 0.4f, BLACK);
        Vector2 asz = UI_MeasureText("ARMY", 10);
        UI_DrawText("ARMY", (Vector2){ topPos.x - asz.x / 2.0f, topPos.y - radius*0.4f - 5.0f }, 10, WHITE);
    }

    /* -- Rumores de asentamientos no descubiertos (D2) -- */
    for (int i = 0; i < map->tileCount; i++) {
        const HexTile* t = &map->tiles[i];
        if (t->isRumor && !t->visited) {
            Vector2 rBase = HexToPixel(t->coord, radius);
            rBase.x += origin.x;
            rBase.y += origin.y - GetBiomeHeight(t->biome);

            /* Icono de rumor: sello ambar con destello */
            DrawCircleV(rBase, radius * 0.42f, (Color){ 230, 160, 20, 200 });
            DrawCircleLines((int)rBase.x, (int)rBase.y, radius * 0.42f, GOLD);
            DrawPoly((Vector2){rBase.x, rBase.y}, 4, radius * 0.25f, 45.0f, WHITE);

            /* Etiqueta con distancia estimada en dias */
            HexCoord armyHex = PixelToHex((Vector2){armyPos.x - origin.x, armyPos.y - origin.y}, radius);
            float distDays = (float)HexDistance(armyHex, t->coord) / 9.0f;
            const char* rText = TextFormat("Rumor %.1fd", distDays);
            Vector2 tw = UI_MeasureText(rText, 10);
            DrawRectangle((int)(rBase.x - tw.x / 2.0f - 4), (int)rBase.y + (int)(radius * 0.38f), (int)tw.x + 8, 14, (Color){15, 15, 25, 210});
            DrawRectangleLines((int)(rBase.x - tw.x / 2.0f - 4), (int)rBase.y + (int)(radius * 0.38f), (int)tw.x + 8, 14, GOLD);
            UI_DrawText(rText, (Vector2){ rBase.x - tw.x / 2.0f, rBase.y + radius * 0.38f + 2.0f }, 10, GOLD);
        }
    }

    free(nodes);
}

/* ============================================================
   HIGHLIGHT DE TILE SELECCIONADO
   ============================================================ */

void MapView_DrawHexHighlight(const HexTile* tile, float radius, Vector2 origin, Color color) {
    if (!tile) return;
    Vector2 center = HexToPixel(tile->coord, radius);
    center.x += origin.x;
    center.y += origin.y - GetBiomeHeight(tile->biome);
    Vector2 corners[7];
    for(int i = 0; i <= 6; i++) {
        float angle_deg = 60.0f * i - 30.0f;
        float angle_rad = PI / 180.0f * angle_deg;
        corners[i].x = center.x + radius * cosf(angle_rad);
        corners[i].y = center.y + (radius * sinf(angle_rad)) * ISO_SCALE_Y;
    }

    float pulse = (sinf((float)GetTime() * 4.5f) + 1.0f) * 0.5f;
    Color fillCol = color;
    fillCol.a = (unsigned char)(25 + (int)(35 * pulse));

    /* Relleno sutil con blending */
    rlBegin(RL_TRIANGLES);
    rlColor4ub(fillCol.r, fillCol.g, fillCol.b, fillCol.a);
    for (int i = 0; i < 6; i++) {
        rlVertex2f(center.x, center.y);
        rlVertex2f(corners[i].x, corners[i].y);
        rlVertex2f(corners[i+1].x, corners[i+1].y);
    }
    rlEnd();

    Color lineCol = color;
    lineCol.a = (unsigned char)(170 + (int)(85 * pulse));
    float thick = 3.0f + pulse * 1.5f;
    for(int i = 0; i < 6; i++) {
        DrawLineEx(corners[i], corners[i+1], thick, lineCol);
    }
}
/* ============================================================
   MINIMAPA
   ============================================================ */

void MapView_DrawPath(HexTile** path, int length, float radius, Vector2 origin) {
    if (!path || length < 2) return;
    
    for (int i = 0; i < length - 1; i++) {
        Vector2 p1 = HexToPixel(path[i]->coord, radius);
        p1.x += origin.x;
        p1.y += origin.y - GetBiomeHeight(path[i]->biome);
        
        Vector2 p2 = HexToPixel(path[i+1]->coord, radius);
        p2.x += origin.x;
        p2.y += origin.y - GetBiomeHeight(path[i+1]->biome);
        
        DrawLineEx(p1, p2, 4.0f, Fade(GOLD, 0.7f));
    }
}

void MapView_DrawMinimap(const Map* map, Rectangle dest,
                          HexCoord armyPos, float blinkTimer,
                          const GameAssets* assets, bool debugRevealMap)
{
    if (!map || map->tileCount == 0) return;

    /* Bounding box con radio = 1 */
    float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
    for (int i = 0; i < map->tileCount; i++) {
        Vector2 p = HexToPixel(map->tiles[i].coord, 1.0f);
        if (p.x < minX) minX = p.x;
        if (p.x > maxX) maxX = p.x;
        if (p.y < minY) minY = p.y;
        if (p.y > maxY) maxY = p.y;
    }

    float spanX    = maxX - minX + 2.0f;
    float spanY    = maxY - minY + 2.0f;
    float padding  = 4.0f;
    float scaleX   = (dest.width  - padding * 2.0f) / spanX;
    float scaleY   = (dest.height - padding * 2.0f) / spanY;
    float miniR    = (scaleX < scaleY) ? scaleX : scaleY;
    if (miniR < 0.5f) miniR = 0.5f;

    Vector2 origin = {
        dest.x + dest.width  / 2.0f - (minX + spanX / 2.0f) * miniR,
        dest.y + dest.height / 2.0f - (minY + spanY / 2.0f) * miniR
    };

    bool hasAssets = (assets && assets->loaded);

    for (int i = 0; i < map->tileCount; i++) {
        const HexTile* t = &map->tiles[i];
        Vector2 pos = HexToPixel(t->coord, miniR);
        pos.x += origin.x;
        pos.y += origin.y;

        if (!t->visited && !debugRevealMap) continue;

        Color topCol = GetBiomeTopColor(t->biome);
        Color tint = WHITE;
        if (!t->visible && !debugRevealMap) {
            tint = (Color){ 80, 80, 85, 255 };
            topCol.r = (unsigned char)(topCol.r * 0.4f);
            topCol.g = (unsigned char)(topCol.g * 0.4f);
            topCol.b = (unsigned char)(topCol.b * 0.4f);
        }

        /* En el minimapa usamos la cara plana coloreada para nitidez absoluta a baja escala */
        (void)hasAssets;
        DrawHexTop(pos, miniR, topCol, (Texture2D){ 0 }, (Rectangle){ 0 }, false, tint);
    }

    /* Rumores en el minimapa (D2) */
    for (int i = 0; i < map->tileCount; i++) {
        const HexTile* t = &map->tiles[i];
        if (t->isRumor && !t->visited) {
            Vector2 rPos = HexToPixel(t->coord, miniR);
            rPos.x += origin.x;
            rPos.y += origin.y;
            DrawPoly(rPos, 4, miniR * 2.2f, 45.0f, ORANGE);
            DrawPolyLines(rPos, 4, miniR * 2.2f, 45.0f, GOLD);
        }
    }

    /* Pen parpadeante */
    if ((int)(blinkTimer * 2.0f) % 2 == 0) {
        Vector2 armyPixel = HexToPixel(armyPos, miniR);
        armyPixel.x += origin.x;
        armyPixel.y += origin.y;
        DrawCircle((int)armyPixel.x, (int)armyPixel.y, miniR * 1.8f, GOLD);
    }
}









void MapView_DrawHexFillHighlight(const HexTile* tile, float radius, Vector2 origin, Color color) {
    if (!tile) return;
    Vector2 center = HexToPixel(tile->coord, radius);
    center.x += origin.x;
    center.y += origin.y - GetBiomeHeight(tile->biome);
    Vector2 points[8];
    points[0] = center;
    for(int i = 0; i <= 6; i++) {
        float angle_deg = 60.0f * i - 30.0f;
        float angle_rad = PI / 180.0f * angle_deg;
        points[i+1].x = center.x + radius * cosf(angle_rad);
        points[i+1].y = center.y + (radius * sinf(angle_rad)) * ISO_SCALE_Y;
    }
    DrawTriangleFan(points, 8, color);
}

/* ============================================================
   BLOQUE D3: INDICADOR DIRECCIONAL DE RUMORES FUERA DE VISTA
   ============================================================ */

void MapView_DrawOffscreenRumors(
    const Map*  map,
    Camera2D    camera,
    float       radius,
    Vector2     origin,
    HexCoord    playerPos
) {
    if (!map || map->tileCount <= 0) return;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    Vector2 center = { (float)sw / 2.0f, (float)sh / 2.0f };

    float minX = 70.0f;
    float maxX = (float)sw - 70.0f;
    float minY = 70.0f;
    float maxY = (float)sh - 180.0f; /* Margen para no solapar con el HUD */

    for (int i = 0; i < map->tileCount; i++) {
        const HexTile* t = &map->tiles[i];
        if (!t->isRumor || t->visited) continue;

        Vector2 worldPos = HexToPixel(t->coord, radius);
        worldPos.x += origin.x;
        worldPos.y += origin.y - GetBiomeHeight(t->biome);

        Vector2 screenPos = GetWorldToScreen2D(worldPos, camera);

        /* Si el rumor esta dentro de la pantalla, no dibujar indicador de borde */
        if (screenPos.x >= 20.0f && screenPos.x <= (float)sw - 20.0f &&
            screenPos.y >= 20.0f && screenPos.y <= (float)sh - 20.0f) {
            continue;
        }

        /* Direccion hacia la posicion del rumor */
        float dx = screenPos.x - center.x;
        float dy = screenPos.y - center.y;
        if (fabsf(dx) < 0.001f && fabsf(dy) < 0.001f) continue;

        /* Interseccion de rayo con bordes rectangulares */
        float tIntersect = 1e9f;
        if (dx > 0) {
            float tx = (maxX - center.x) / dx;
            if (tx < tIntersect) tIntersect = tx;
        } else if (dx < 0) {
            float tx = (minX - center.x) / dx;
            if (tx < tIntersect) tIntersect = tx;
        }

        if (dy > 0) {
            float ty = (maxY - center.y) / dy;
            if (ty < tIntersect) tIntersect = ty;
        } else if (dy < 0) {
            float ty = (minY - center.y) / dy;
            if (ty < tIntersect) tIntersect = ty;
        }

        Vector2 edgePos = { center.x + tIntersect * dx, center.y + tIntersect * dy };

        /* Distancia en dias hacia el asentamiento rumor */
        float distDays = (float)HexDistance(playerPos, t->coord) / 9.0f;
        const char* badgeText = TextFormat("Rumor: %.1fd", distDays);
        int textW = (int)UI_MeasureText(badgeText, 11).x;

        /* Flecha direccional apuntando hacia el rumor fuera de pantalla */
        float angle = atan2f(dy, dx);
        Vector2 dir = { cosf(angle), sinf(angle) };
        Vector2 perp = { -sinf(angle), cosf(angle) };
        Vector2 tip = { edgePos.x + dir.x * 16.0f, edgePos.y + dir.y * 16.0f };
        Vector2 left = { edgePos.x - dir.x * 4.0f + perp.x * 8.0f, edgePos.y - dir.y * 4.0f + perp.y * 8.0f };
        Vector2 right = { edgePos.x - dir.x * 4.0f - perp.x * 8.0f, edgePos.y - dir.y * 4.0f - perp.y * 8.0f };

        DrawTriangle(tip, left, right, GOLD);
        DrawTriangleLines(tip, left, right, DARKGRAY);

        /* Insignia con texto */
        DrawRectangle((int)edgePos.x - textW / 2 - 8, (int)edgePos.y - 12, textW + 16, 24, (Color){ 20, 20, 30, 230 });
        DrawRectangleLines((int)edgePos.x - textW / 2 - 8, (int)edgePos.y - 12, textW + 16, 24, GOLD);
        UI_DrawText(badgeText, (Vector2){ (float)(edgePos.x - textW / 2), (float)(edgePos.y - 6) }, 11, GOLD);
    }
}

void MapView_Init(MapViewState* state) {
    if (!state) return;
    memset(state, 0, sizeof(MapViewState));
    state->hexRadius = 35.0f;
    state->origin = (Vector2){ 0.0f, 0.0f };
    state->firstFrame = true;
    state->skyR = 255.0f;
    state->skyG = 255.0f;
    state->skyB = 255.0f;
    state->lastTimeOfDay = TIME_MORNING;
    state->debugRevealMap = false;
    state->selectedTile = NULL;
    for (int i = 0; i < 64; i++) {
        state->enemyFirstFrame[i] = true;
        state->enemyIsTweening[i] = false;
    }
}

void MapView_SyncToArmy(MapViewState* state, HexCoord armyPos, CameraController* cc) {
    if (!state) return;
    state->visualArmyPos = HexToPixel(armyPos, state->hexRadius);
    state->selectedTile = NULL;
    state->firstFrame = false;
    for (int i = 0; i < 64; i++) {
        state->enemyFirstFrame[i] = true;
        state->enemyIsTweening[i] = false;
    }
    if (cc) {
        Vector2 focusPos = {
            state->visualArmyPos.x + state->origin.x,
            state->visualArmyPos.y + state->origin.y
        };
        CameraController_FocusOn(cc, focusPos);
    }
}

void MapView_UpdateWorldVisuals(
    MapViewState* state,
    const struct GameMaster* gm,
    const struct Army* army,
    CameraController* cc,
    float dt
)
{
    if (!state || !gm || !army) return;

    if (state->lastTimeOfDay != (int)gm->timeOfDay) {
        Audio_SetAmbientTimeOfDay(gm->timeOfDay);
        if (gm->timeOfDay == TIME_NIGHT) {
            Audio_PlaySfxByName("night_fall");
        }
        float targetR = 255.0f, targetG = 255.0f, targetB = 255.0f;
        switch (gm->timeOfDay) {
            case TIME_MORNING:   targetR = 255.0f; targetG = 240.0f; targetB = 220.0f; break;
            case TIME_MIDDAY:    targetR = 255.0f; targetG = 255.0f; targetB = 255.0f; break;
            case TIME_AFTERNOON: targetR = 255.0f; targetG = 200.0f; targetB = 150.0f; break;
            case TIME_NIGHT:     targetR = 100.0f; targetG = 120.0f; targetB = 180.0f; break;
        }
        Tween_Start(&state->skyR, targetR, 1.5f, Ease_Linear);
        Tween_Start(&state->skyG, targetG, 1.5f, Ease_Linear);
        Tween_Start(&state->skyB, targetB, 1.5f, Ease_Linear);
        state->lastTimeOfDay = (int)gm->timeOfDay;
    }

    Tween_Update(dt);
    Sequence_Update(dt);
    ScreenFX_Update(dt);

    if (army->isMoving || Sequence_IsBusy()) {
        if (cc) cc->camera.target = state->visualArmyPos;
    }

    if (state->firstFrame) {
        state->visualArmyPos = HexToPixel(army->position, state->hexRadius);
        state->firstFrame = false;
    }

    bool anyEnemyMoving = false;
    for (int i = 0; i < gm->enemyCount && i < 64; i++) {
        if (!gm->enemies[i].isActive) continue;
        Vector2 target = HexToPixel(gm->enemies[i].position, state->hexRadius);
        if (state->enemyFirstFrame[i]) {
            state->visualEnemyPos[i] = target;
            state->enemyFirstFrame[i] = false;
        }
        float dist = Vector2Distance(state->visualEnemyPos[i], target);
        if (dist > 1.0f) {
            if (!state->enemyIsTweening[i]) {
                Tween_Start(&state->visualEnemyPos[i].x, target.x, 0.4f, Ease_OutCubic);
                Tween_Start(&state->visualEnemyPos[i].y, target.y, 0.4f, Ease_OutCubic);
                state->enemyIsTweening[i] = true;
                anyEnemyMoving = true;
            }
        } else {
            state->enemyIsTweening[i] = false;
        }
    }

    if (anyEnemyMoving) {
        SeqAction enemyMoveAct = {0};
        enemyMoveAct.duration = 0.45f;
        Sequence_Push(enemyMoveAct);
    }
}
