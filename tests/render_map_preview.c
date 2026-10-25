#include "raylib.h"
#include "controllers/asset_manager.h"
#include "models/map_generator.h"
#include "views/map_view.h"
#include "views/hud.h"
#include "models/army.h"
#include "models/general.h"
#include "controllers/game_master.h"
#include "presentation/ui_widgets.h"
#include "presentation/ui_skin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(void)
{
    InitWindow(1600, 900, "Map Pixel-Art Tiles & Structures Showcase");
    SetTargetFPS(60);
    UI_Init();

    GameAssets assets = {0};
    Assets_Init(&assets);

    /* ============================================================
       PASS 1: CATÁLOGO DE BIOMAS Y ESTRUCTURAS PIXEL-ART
       ============================================================ */
    RenderTexture2D target1 = LoadRenderTexture(1600, 900);
    BeginTextureMode(target1);
    ClearBackground((Color){ 18, 16, 15, 255 });

    UI_DrawText("CATALOGO DE TILES Y ESTRUCTURAS PIXEL-ART ISOMETRICOS (2:1 + OVERHANG)", (Vector2){ 32, 22 }, 24, (Color){ 238, 214, 146, 255 });
    UI_DrawText("Mascara hexagonal matematica exacta (64x37 px base + 23 px de relieve superior = 64x60 px, TEXTURE_FILTER_POINT)", (Vector2){ 32, 54 }, 16, (Color){ 185, 175, 155, 255 });

    /* 1. Fila de los 6 Biomas individuales con su prisma 3D estratificado */
    Map demoRow = {0};
    demoRow.tileCount = 6;
    demoRow.tiles = (HexTile*)MemAlloc(sizeof(HexTile) * 6);
    BiomeType biomes[6] = { BIOME_PLAINS, BIOME_FOREST, BIOME_DESERT, BIOME_JUNGLE, BIOME_MOUNTAIN, BIOME_WATER };
    const char* biomeNames[6] = { "Llanura (Plains)", "Bosque (Forest)", "Desierto (Desert)", "Jungla (Jungle)", "Montana (Mountain)", "Mar Egeo (Water)" };

    for (int i = 0; i < 6; i++) {
        Rectangle card = { 32.0f + i * 258.0f, 92.0f, 242.0f, 240.0f };
        UI_DrawPanelStyled(card, UI_STYLE_PANEL_DARK);
        UI_DrawText(biomeNames[i], (Vector2){ card.x + 18, card.y + 14 }, 18, (Color){ 238, 214, 146, 255 });

        Map singleMap = {0};
        singleMap.tileCount = 1;
        HexTile ht = HexTile_Create((HexCoord){0, 0}, 1, (biomes[i] == BIOME_WATER) ? TERRAIN_WATER : TERRAIN_LAND);
        ht.biome = biomes[i];
        ht.visited = true;
        ht.visible = true;
        singleMap.tiles = &ht;

        Vector2 orig = { card.x + card.width * 0.5f, card.y + 162.0f };
        MapView_Draw(&singleMap, 68.0f, orig, false, (HexCoord){0,0}, 2, (Vector2){-9999, -9999}, &assets, true);

        const char* dimTxt = (biomes[i] == BIOME_WATER) ? "64x37 px (4 frames)" : "64x60 px (Overhang)";
        UI_DrawText(dimTxt, (Vector2){ card.x + 18, card.y + 206 }, 14, (Color){ 165, 155, 138, 255 });
    }

    /* 2. Fila de las 7 Estructuras sobre hexágonos (sin pedestal circular gris) */
    UI_DrawText("Estructuras Pixel-Art asentadas sobre tiles (sin pedestal circular gris, escala pixel-perfect)", (Vector2){ 32, 352 }, 18, (Color){ 238, 214, 146, 255 });

    StructureType structs[7] = {
        STRUCT_TOWN_CASTLE, STRUCT_ALTAR, STRUCT_TOWN_MARKET,
        STRUCT_TOWN_ARMORY, STRUCT_TOWN_HOUSES, STRUCT_WATCHTOWER, STRUCT_BRIDGE
    };
    const char* structNames[7] = {
        "Ciudadela", "Altar Divino", "Mercado",
        "Armeria", "Suburbio", "Atalaya", "Puente"
    };

    for (int i = 0; i < 7; i++) {
        Rectangle card = { 32.0f + i * 220.0f, 384.0f, 208.0f, 235.0f };
        UI_DrawPanelStyled(card, UI_STYLE_PANEL_DARK);
        UI_DrawText(structNames[i], (Vector2){ card.x + 16, card.y + 14 }, 17, (Color){ 238, 214, 146, 255 });

        Map singleMap = {0};
        singleMap.tileCount = 1;
        HexTile ht = HexTile_Create((HexCoord){0, 0}, 1, TERRAIN_LAND);
        ht.biome = (structs[i] == STRUCT_BRIDGE) ? BIOME_WATER : BIOME_PLAINS;
        ht.visited = true;
        ht.visible = true;
        ht.structure.type = structs[i];
        ht.structure.settlementType = (structs[i] == STRUCT_TOWN_CASTLE) ? SETTLEMENT_PRIMARY : SETTLEMENT_SECONDARY;
        singleMap.tiles = &ht;

        Vector2 orig = { card.x + card.width * 0.5f, card.y + 168.0f };
        MapView_Draw(&singleMap, 58.0f, orig, false, (HexCoord){0,0}, 2, (Vector2){-9999, -9999}, &assets, true);
    }

    /* 3. Demostración de encaje de cluster hexagonal (teselación perfecta sin huecos) */
    Rectangle clusterPanel = { 32.0f, 636.0f, 1536.0f, 240.0f };
    UI_DrawPanelStyled(clusterPanel, UI_STYLE_PANEL_DARK);
    UI_DrawText("Teselacion en Grilla Hexagonal (Encaje exacto entre vecinos + superposicion natural norte-sur)", (Vector2){ 52, 652 }, 18, (Color){ 238, 214, 146, 255 });

    HexTile clusterTiles[19];
    int cIdx = 0;
    for (int q = -2; q <= 2; q++) {
        for (int r = -2; r <= 2; r++) {
            if (abs(q + r) > 2) continue;
            HexTile t = HexTile_Create((HexCoord){ q, r }, 1, TERRAIN_LAND);
            int pattern = (abs(q * 3 + r * 5)) % 6;
            t.biome = biomes[pattern];
            t.visited = true;
            t.visible = true;
            if (q == 0 && r == 0) t.structure.type = STRUCT_TOWN_CASTLE;
            else if (q == -1 && r == 1) t.structure.type = STRUCT_WATCHTOWER;
            else if (q == 1 && r == 0) t.structure.type = STRUCT_ALTAR;
            clusterTiles[cIdx++] = t;
        }
    }
    Map clusterMap = {0};
    clusterMap.tileCount = cIdx;
    clusterMap.tiles = clusterTiles;
    MapView_Draw(&clusterMap, 39.0f, (Vector2){ 800.0f, 774.0f }, false, (HexCoord){0,0}, 5, (Vector2){-9999, -9999}, &assets, true);

    EndTextureMode();
    Image img1 = LoadImageFromTexture(target1.texture);
    ImageFlipVertical(&img1);
    ExportImage(img1, "map_tiles_showcase.png");
    ExportImage(img1, "C:/Users/exaul/.gemini/antigravity/brain/1ec680a5-a412-4783-8263-ef76f34df73d/map_tiles_showcase.png");
    UnloadImage(img1);
    UnloadRenderTexture(target1);
    MemFree(demoRow.tiles);

    /* ============================================================
       PASS 2: VISTA DEL MAPA TÁCTICO EN PARTIDA CON HUD Y CAMINOS
       ============================================================ */
    SetRandomSeed(42);
    srand(42);
    Map worldMap = Map_GenerateProcedural(22, 65, 3, 1);
    float sumX = 0.0f, sumY = 0.0f;
    for (int i = 0; i < worldMap.tileCount; i++) {
        worldMap.tiles[i].visited = true;
        worldMap.tiles[i].visible = true;
        Vector2 tp = HexToPixel(worldMap.tiles[i].coord, 38.0f);
        sumX += tp.x;
        sumY += tp.y;
    }

    Vector2 centroidPx = { sumX / (float)worldMap.tileCount, sumY / (float)worldMap.tileCount };
    HexCoord centerCoord = PixelToHex(centroidPx, 38.0f);

    /* Colocar algunos biomas variados alrededor del centro para mostrar la riqueza visual en partida */
    for (int i = 0; i < worldMap.tileCount; i++) {
        int dist = HexCoord_Distance(worldMap.tiles[i].coord, centerCoord);
        if (dist <= 6 && worldMap.tiles[i].biome == BIOME_PLAINS && worldMap.tiles[i].structure.type == STRUCT_NONE && !worldMap.tiles[i].isRoad) {
            int h = abs(worldMap.tiles[i].coord.q * 7 + worldMap.tiles[i].coord.r * 13) % 10;
            if (h == 0 || h == 1) worldMap.tiles[i].biome = BIOME_MOUNTAIN;
            else if (h == 2 || h == 3) worldMap.tiles[i].biome = BIOME_DESERT;
            else if (h == 4) worldMap.tiles[i].biome = BIOME_JUNGLE;
        }
    }

    General gen = General_Create("Leonidas", 40, 4, 4, 3, 3, 5);
    Army army = Army_Create(centerCoord, gen, 12, 45, 8, 950.0f, 1120.0f, 240, 10);
    GameMaster gm = {0};
    GameMaster_Init(&gm);
    HudState hud = Hud_Init();

    Vector2 centerPx = HexToPixel(centerCoord, 38.0f);
    Vector2 mapOrigin = { 800.0f - centroidPx.x, 380.0f - centroidPx.y };

    RenderTexture2D target2 = LoadRenderTexture(1600, 900);
    BeginTextureMode(target2);
    ClearBackground((Color){ 20, 24, 32, 255 });

    MapView_Draw(&worldMap, 38.0f, mapOrigin, false, centerCoord, 16, centerPx, &assets, true);

    int bottomH = 165;
    int bottomY = 900 - bottomH - 12;
    Hud_DrawCommanderPanel(&army, &gm, &hud, 12, 48, 430);
    Hud_DrawArmyStatsPanel(&army, &gm, &worldMap, &hud, 12, bottomY, 450, bottomH);
    Hud_DrawTopBar(&gm, 1600);
    Hud_DrawDicePanel(&hud, &gm, &army, &worldMap, 12 + 450 + 12, bottomY, 215, bottomH);

    Rectangle minimapRect = { 1600.0f - 420.0f - 12.0f, (float)bottomY, 420.0f, (float)bottomH };
    UI_DrawPanelStyled(minimapRect, UI_STYLE_PANEL_DARK);
    MapView_DrawMinimap(&worldMap, minimapRect, army.position, 0.0f, &assets, true);

    EndTextureMode();
    Image img2 = LoadImageFromTexture(target2.texture);
    ImageFlipVertical(&img2);
    ExportImage(img2, "map_tactical_preview.png");
    ExportImage(img2, "C:/Users/exaul/.gemini/antigravity/brain/1ec680a5-a412-4783-8263-ef76f34df73d/map_tactical_preview.png");
    UnloadImage(img2);
    UnloadRenderTexture(target2);

    Map_Free(&worldMap);
    Assets_Unload(&assets);
    UI_Shutdown();
    CloseWindow();
    printf("[OK] map_tiles_showcase.png y map_tactical_preview.png generados exitosamente.\n");
    return 0;
}
