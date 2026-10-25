#include "raylib.h"
#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"
#include "models/modifier.h"
#include "models/army.h"
#include "controllers/game_master.h"
#include "models/hex_tile.h"
#include "views/draft_view.h"
#include "views/hud.h"
#include "views/city_view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    InitWindow(1600, 900, "Cards & HUD Showcase");
    SetTargetFPS(60);
    UI_Init();

    ModifierRegistry reg = {0};
    reg.count = Modifier_LoadAll("assets/modifiers", reg.defs, MAX_MODIFIERS);
    printf("Loaded %d modifiers for preview.\n", reg.count);

    ModifierDef* zeus = Modifier_GetById(reg.defs, reg.count, "zeus_rayo");
    ModifierDef* ares = Modifier_GetById(reg.defs, reg.count, "ares_marea_roja");
    ModifierDef* atenea = Modifier_GetById(reg.defs, reg.count, "atenea_escudo");
    ModifierDef* cDoom = Modifier_GetById(reg.defs, reg.count, "curse_doom");

    // PASS 1: Render 4 Showcase Cards (Large & Crisp)
    RenderTexture2D target1 = LoadRenderTexture(1600, 900);
    BeginTextureMode(target1);
    ClearBackground((Color){ 12, 14, 20, 255 });

    Vector2 titleSz = UI_MeasureText("SISTEMA DE CARTAS DIVINAS Y MALDICIONES", 32);
    UI_DrawText("SISTEMA DE CARTAS DIVINAS Y MALDICIONES", (Vector2){ (1600 - titleSz.x)/2.0f, 30 }, 32, UI_COLOR_GOLD);
    const char* sub = "Plantilla visual con fuentes legibles y balance de escala mejorado";
    Vector2 subSz = UI_MeasureText(sub, 16);
    UI_DrawText(sub, (Vector2){ (1600 - subSz.x)/2.0f, 75 }, 16, (Color){ 180, 185, 200, 255 });

    int cardW = 320;
    int cardH = 480;
    int spacing = 32;
    int totalW = 4 * cardW + 3 * spacing;
    int startX = (1600 - totalW) / 2;
    int cardY = 125;

    if (zeus) UI_DrawModifierCard(zeus, (Rectangle){ (float)(startX + 0 * (cardW + spacing)), (float)cardY, (float)cardW, (float)cardH });
    if (ares) UI_DrawModifierCard(ares, (Rectangle){ (float)(startX + 1 * (cardW + spacing)), (float)cardY, (float)cardW, (float)cardH });
    if (atenea) UI_DrawModifierCard(atenea, (Rectangle){ (float)(startX + 2 * (cardW + spacing)), (float)cardY, (float)cardW, (float)cardH });
    if (cDoom) UI_DrawModifierCard(cDoom, (Rectangle){ (float)(startX + 3 * (cardW + spacing)), (float)cardY, (float)cardW, (float)cardH });

    const char* ftr = "Fuentes de alta definicion (minimo 14px-18px) para nombres, estadisticas, atributos y narrativa de cartas.";
    Vector2 ftrSz = UI_MeasureText(ftr, 16);
    UI_DrawText(ftr, (Vector2){ (1600 - ftrSz.x)/2.0f, (float)(cardY + cardH + 30) }, 16, (Color){ 140, 230, 140, 255 });

    EndTextureMode();
    Image img1 = LoadImageFromTexture(target1.texture);
    ImageFlipVertical(&img1);
    ExportImage(img1, "cards_showcase_preview.png");
    UnloadImage(img1);
    UnloadRenderTexture(target1);

    // PASS 2: Render Altar State Reveal with pulled card
    DraftContext draft = {0};
    DraftView_Init(&draft, &reg);
    draft.state = ALTAR_STATE_REVEAL;
    draft.pulledCard = zeus;

    Army army = {0};
    army.general = General_Create("Leonidas", 40, 3, 4, 3, 3, 5);
    army.commander = Commander_FromGeneral(&army.general);
    army.iniciados = 20;
    army.soldados = 30;
    army.sargentos = 6;
    army.monedas = 180;
    army.ganado = 4;
    army.agua_disponible = 250.0f;
    army.comida_disponible = 320.0f;
    army.vino_disponible = 30.0f;
    army.carne_disponible = 25.0f;
    army.moral = 85.0f;
    army.salud = 92.0f;
    army.curseCount = 1;
    strncpy(army.curses[0].id, "curse_doom", 31);

    // Equipar 2 cartas divinas al general
    if (ares) Army_EquipModifier(&army, ares);
    if (atenea) Army_EquipModifier(&army, atenea);

    RenderTexture2D target2 = LoadRenderTexture(1600, 900);
    BeginTextureMode(target2);
    ClearBackground((Color){ 15, 18, 25, 255 });
    DraftView_UpdateDraw(&draft, &army, 1600, 900);
    EndTextureMode();
    Image img2 = LoadImageFromTexture(target2.texture);
    ImageFlipVertical(&img2);
    ExportImage(img2, "altar_card_reveal_preview.png");
    UnloadImage(img2);
    UnloadRenderTexture(target2);

    // PASS 3: Render HUD with Commander Panel (NO OVERLAP) and compact Army Stats Panel (height 165)
    GameMaster gm = {0};
    gm.currentActionPoints = 3;
    gm.maxActionPoints = 4;
    gm.modRegistry = reg;
    gm.timeOfDay = TIME_AFTERNOON;

    Map map = {0};
    HudState hud = {0};

    RenderTexture2D target3 = LoadRenderTexture(1600, 900);
    BeginTextureMode(target3);
    ClearBackground((Color){ 20, 24, 34, 255 });

    // Background placeholder for map
    DrawRectangleGradientV(0, 0, 1600, 900, (Color){ 25, 30, 45, 255 }, (Color){ 14, 16, 22, 255 });
    UI_DrawText("VISTA DEL MAPA TACTICO (Fondo)", (Vector2){ 600, 400 }, 24, (Color){ 70, 80, 100, 255 });

    int bottomH = 165;
    int bottomY = 900 - bottomH - 12;

    Hud_DrawCommanderPanel(&army, &gm, &hud, 14, 48, 430);
    Hud_DrawArmyStatsPanel(&army, &gm, &map, &hud, 14, bottomY, 450, bottomH);
    Hud_DrawTopBar(&gm, 1600);

    int diceW = 215;
    int diceX = 14 + 450 + 12;
    Hud_DrawDicePanel(&hud, &gm, &army, &map, diceX, bottomY, diceW, bottomH);

    EndTextureMode();
    Image img3 = LoadImageFromTexture(target3.texture);
    ImageFlipVertical(&img3);
    ExportImage(img3, "hud_preview.png");
    UnloadImage(img3);
    UnloadRenderTexture(target3);

    // PASS 4: Render City Main Tab
    CityContext cityCtx = {0};
    strncpy(cityCtx.name, "Esparta", sizeof(cityCtx.name) - 1);
    strncpy(cityCtx.factionName, "Legion del Sol", sizeof(cityCtx.factionName) - 1);
    cityCtx.structType = STRUCT_TOWN_CASTLE;
    cityCtx.settlementType = SETTLEMENT_PRIMARY;
    cityCtx.factionOpinion = 15;
    cityCtx.factionPriceMult = 1.0f;
    cityCtx.factionAlertLevel = 0;
    cityCtx.currentTab = CITY_TAB_MAIN;
    cityCtx.stockFood = 120;
    cityCtx.stockWater = 95;
    cityCtx.stockInitiates = 8;
    cityCtx.stockCattle = 5;

    RenderTexture2D target4 = LoadRenderTexture(1600, 900);
    BeginTextureMode(target4);
    ClearBackground((Color){ 10, 12, 16, 255 });
    CityView_UpdateDraw(&cityCtx, &army, &gm, &map, 1600, 900);
    EndTextureMode();
    Image img4 = LoadImageFromTexture(target4.texture);
    ImageFlipVertical(&img4);
    ExportImage(img4, "city_main_preview.png");
    UnloadImage(img4);
    UnloadRenderTexture(target4);

    // PASS 5: Render City Armory Tab
    cityCtx.currentTab = CITY_TAB_ARMORY;
    RenderTexture2D target5 = LoadRenderTexture(1600, 900);
    BeginTextureMode(target5);
    ClearBackground((Color){ 10, 12, 16, 255 });
    CityView_UpdateDraw(&cityCtx, &army, &gm, &map, 1600, 900);
    EndTextureMode();
    Image img5 = LoadImageFromTexture(target5.texture);
    ImageFlipVertical(&img5);
    ExportImage(img5, "city_armory_preview.png");
    UnloadImage(img5);
    UnloadRenderTexture(target5);

    UI_Shutdown();
    CloseWindow();
    printf("All 5 previews rendered successfully!\n");
    return 0;
}
