#include "map_view.h"
#include "rlgl.h"
#include <stddef.h>
#include <math.h>
#include <stdlib.h>

/* ============================================================
   PERSPECTIVA ISOMÉTRICA
   Factor de aplastamiento 2:1. El eje Y se multiplica por 0.5
   para dar la ilusión de profundidad.
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
    /* Revertir el aplastamiento del eje Y antes de la conversión inversa */
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

static float GetBiomeHeight(BiomeType biome)
{
    switch (biome) {
        case BIOME_WATER:    return  2.0f;
        case BIOME_PLAINS:   return 10.0f;
        case BIOME_DESERT:   return 12.0f;
        case BIOME_JUNGLE:   return 10.0f;
        case BIOME_FOREST:   return 14.0f;
        case BIOME_MOUNTAIN: return 35.0f;
        default:             return 10.0f;
    }
}

/* Color de cara lateral (fallback cuando no hay textura) */
static Color GetBiomeSideColor(BiomeType biome)
{
    switch (biome) {
        case BIOME_WATER:    return (Color){ 45, 105, 185, 255 };
        case BIOME_FOREST:   return (Color){ 18,  70,  30, 255 };
        case BIOME_DESERT:   return (Color){ 180, 150,  70, 255 };
        case BIOME_JUNGLE:   return (Color){ 10,  55,  42, 255 };
        case BIOME_MOUNTAIN: return (Color){ 105,  95,  90, 255 };
        default:             return (Color){ 80,  145,  50, 255 };
    }
}

/* Color de cara superior (fallback sin textura) */
static Color GetBiomeTopColor(BiomeType biome)
{
    switch (biome) {
        case BIOME_WATER:    return (Color){ 95, 175, 235, 255 };
        case BIOME_FOREST:   return (Color){ 35, 115,  50, 255 };
        case BIOME_DESERT:   return (Color){ 230, 200, 110, 255 };
        case BIOME_JUNGLE:   return (Color){ 20,  95,  75, 255 };
        case BIOME_MOUNTAIN: return (Color){ 155, 145, 138, 255 };
        default:             return (Color){ 140, 205,  90, 255 };
    }
}

/* ============================================================
   DIBUJO DEL BLOQUE ISOMÉTRICO (caras laterales + cara superior)
   ============================================================ */

/*
 * Vértices del hexágono "pointy-top" aplastado:
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
 * Dibujamos los quads 4→5→0 y 0→1→2 como caras laterales.
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
        
        Rectangle dest = { minX, minY, maxX - minX, maxY - minY };
        DrawTexturePro(tex, srcFrame, dest, (Vector2){ 0, 0 }, 0.0f, tint);

        /* Borde fino */
        for (int i = 0; i < 6; i++)
            DrawLineV(pts[i], pts[(i + 1) % 6], (Color){ 0, 0, 0, 80 });
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
            DrawLineV(pts[i], pts[(i + 1) % 6], BLACK);
    }
}

/* Dibuja el cuerpo lateral (prisma) del bloque isométrico */
static void DrawHexSides(Vector2 baseCenter, float radius,
                          float height, Color sideColor)
{
    if (height <= 0.0f) return;

    Vector2 base[6], top[6];
    ComputeHexVertices(baseCenter, radius, base);
    for (int i = 0; i < 6; i++) {
        top[i].x = base[i].x;
        top[i].y = base[i].y - height;
    }

    /* Solo las caras visibles desde el sur: derecha (0-1) y frontal (5-0) */
    /* Cara derecha: vértices 0, 1 (top) → 1, 0 (base) */
    rlBegin(RL_QUADS);
    Color dark = (Color){ (unsigned char)(sideColor.r * 0.7f),
                           (unsigned char)(sideColor.g * 0.7f),
                           (unsigned char)(sideColor.b * 0.7f), 255 };
    rlColor4ub(dark.r, dark.g, dark.b, dark.a);
    rlVertex2f(top[0].x,  top[0].y);
    rlVertex2f(top[1].x,  top[1].y);
    rlVertex2f(base[1].x, base[1].y);
    rlVertex2f(base[0].x, base[0].y);

    /* Cara frontal izquierda: 5-0 (top) → 0-5 (base) */
    Color mid = (Color){ (unsigned char)(sideColor.r * 0.85f),
                          (unsigned char)(sideColor.g * 0.85f),
                          (unsigned char)(sideColor.b * 0.85f), 255 };
    rlColor4ub(mid.r, mid.g, mid.b, mid.a);
    rlVertex2f(top[5].x,  top[5].y);
    rlVertex2f(top[0].x,  top[0].y);
    rlVertex2f(base[0].x, base[0].y);
    rlVertex2f(base[5].x, base[5].y);
    rlEnd();

    /* Aristas de contorno */
    DrawLineV(top[0],  base[0],  BLACK);
    DrawLineV(top[1],  base[1],  BLACK);
    DrawLineV(top[5],  base[5],  BLACK);
    DrawLineV(top[5],  top[0],   BLACK);
    DrawLineV(top[0],  top[1],   BLACK);
    DrawLineV(base[5], base[0],  BLACK);
    DrawLineV(base[0], base[1],  BLACK);
}

/* ============================================================
   DIBUJO DE ESTRUCTURAS CON TEXTURA (o fallback geométrico)
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
            float scale = (radius * 1.4f) / tw;
            float dw = tw * scale;
            float dh = th * scale;
            Rectangle src = { 0, 0, tw, th };
            Rectangle dst = { topPos.x - dw / 2.0f,
                               topPos.y - dh / 2.0f,
                               dw, dh };
            DrawTexturePro(tex, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
        } else {
            /* Estructuras que se "paran" sobre el tile (torres, casas, etc.)
             * se anclan en su base coincidiendo con el centro de la cara superior. */
            float scale = (radius * 1.8f) / tw;
            float dw = tw * scale;
            float dh = th * scale;
            Rectangle src = { 0, 0, tw, th };
            Rectangle dst = { topPos.x - dw / 2.0f, topPos.y - dh, dw, dh };
            DrawTexturePro(tex, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
        }

    } else {
        /* Fallback geométrico del sistema anterior */
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
   FUNCIÓN PRINCIPAL DE DIBUJADO
   ============================================================ */

void MapView_Draw(const Map* map, float radius, Vector2 origin, bool drawConnections, HexCoord viewCenter, int renderRadius, HexCoord armyPos, const GameAssets* assets, bool debugRevealMap) {
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

    /* 2. Ordenar atrás→adelante (Y menor = más al norte = se dibuja antes) */
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

        /* ── Ríos entre vecinos ── */
        for (int d = 0; d < 6; d++) {
            const HexTile* nb = t->neighbors[d];
            if (!nb) continue;
            Vector2 nBase = HexToPixel(nb->coord, radius);
            nBase.x += origin.x; nBase.y += origin.y;
            Vector2 nTop = { nBase.x, nBase.y - GetBiomeHeight(nb->biome) };
            Vector2 mid  = { (topPos.x + nTop.x) / 2.0f,
                              (topPos.y + nTop.y) / 2.0f };

            if (t->river == RIVER_PRESENT && nb->river == RIVER_PRESENT)
                DrawLineEx(topPos, mid, radius * 0.28f, SKYBLUE);

            if (t->isRoad && nb->isRoad) {
                bool  isMain = (t->road == ROAD_MAIN && nb->road == ROAD_MAIN);
                float rw     = isMain ? radius * 0.28f : radius * 0.14f;
                Color rc     = isMain ? (Color){ 105,70,40,255 }
                                      : (Color){ 185,140,85,255 };
                DrawLineEx(topPos, mid, rw, rc);
            }
        }

        /* ── Estructuras ── */
        if (t->structure.type != STRUCT_NONE)
            DrawStructure(t->structure.type, topPos, radius, t->structure.settlementType == SETTLEMENT_PRIMARY, t->structure.isGuarded, assets, tint);

        /* ── Marcadores de inicio / fin ── */
        if (t->special == TILE_SPECIAL_START)
            DrawCircleV(topPos, radius * 0.35f, GOLD);
        else if (t->special == TILE_SPECIAL_END)
            DrawCircleV(topPos, radius * 0.35f, RED);

        /* ── Token del ejército ── */
        if (t->coord.q == armyPos.q && t->coord.r == armyPos.r) {
            if (hasAssets && assets->armyToken.id != 0) {
                float tw = (float)assets->armyToken.width;
                float th = (float)assets->armyToken.height;
                float scale = (radius * 1.6f) / tw;
                Rectangle src = { 0, 0, tw, th };
                Rectangle dst = { topPos.x - tw*scale/2.0f,
                                   topPos.y - th*scale - radius*0.1f,
                                   tw*scale, th*scale };
                DrawTexturePro(assets->armyToken, src, dst,
                               (Vector2){ 0,0 }, 0.0f, WHITE);
            } else {
                DrawCircleV((Vector2){ topPos.x, topPos.y - radius*0.4f },
                            radius * 0.4f, (Color){ 200,150,50,255 });
                DrawCircleLines((int)topPos.x, (int)(topPos.y - radius*0.4f),
                                radius * 0.4f, BLACK);
                DrawText("ARMY", (int)(topPos.x - 12),
                         (int)(topPos.y - radius*0.4f - 4), 10, WHITE);
            }
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
    for(int i = 0; i < 6; i++) {
        DrawLineEx(corners[i], corners[i+1], 4.0f, color);
    }
}
/* ============================================================
   MINIMAPA
   ============================================================ */

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

        /* En el minimapa usamos solo la cara plana (sin volumen) para mayor claridad */
        if (hasAssets && t->biome != BIOME_WATER) {
            Texture2D tileTex = Assets_GetBiomeTile(assets, t->biome);
            Rectangle src = { 0, 0, (float)tileTex.width, (float)tileTex.height };
            DrawHexTop(pos, miniR, topCol, tileTex, src, true, tint);
        } else if (hasAssets && t->biome == BIOME_WATER) {
            Rectangle srcF = AnimSprite_GetFrame(&assets->waterAnim);
            DrawHexTop(pos, miniR, topCol, assets->waterAnim.sheet, srcF, true, tint);
        } else {
            DrawHexTop(pos, miniR, topCol, (Texture2D){ 0 }, (Rectangle){ 0 }, false, tint);
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
