#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"
#include <math.h>

#include "map_generator.h"
#include "map_view.h"
#include "hud.h"
#include "army.h"
#include "general.h"
#include "asset_manager.h"
#include "presentation/audio_manager.h"
#include "presentation/screen_fx.h"
#include "presentation/tween.h"
#include "presentation/sequence.h"
#include "presentation/ui_widgets.h"
#include "camera_controller.h"
#include "controllers/game_master.h"
#include "controllers/npc_controller.h"
#include "models/menu.h"
#include "views/menu_view.h"
#include "views/event_view.h"
#include "views/combat_view.h"
#include "views/enemy_view.h"
#include "views/npc_view.h"
#include "views/ration_view.h"
#include "views/draft_view.h"
#include "views/city_view.h"

#include <stdlib.h>
#include <time.h>


int main(void)
{
    ChangeDirectory(GetApplicationDirectory());

    const int screenWidth = 1024;
    const int screenHeight = 768;


    /* ============================================================
       VENTANA
       ============================================================ */

    InitWindow(
        screenWidth,
        screenHeight,
        "Estrategia T�ctica - Nivel 1 Procedural"
    );

    ToggleFullscreen();
    SetTargetFPS(60);
    srand((unsigned int)time(NULL));

    Audio_Init();
    UI_Init();

    GameAssets gameAssets;
    Assets_Init(&gameAssets);

    Vector2 center = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    CameraController cc;
    CameraController_Init(&cc, center);

    int provinceLevel = 1;

    MainMenu mainMenu;
    Menu_Init(&mainMenu);
    MenuView_Init();

    ArmySetupConfig armySetup;
    ArmySetup_InitDefault(&armySetup);

    Map levelMap = {0};
    MapViewState mapView;
    MapView_Init(&mapView);

    HudState hudState;
    Hud_InitState(&hudState);

    RationViewState rationView = {0};
    DraftContext activeDraft = {0};
    Army playerArmy = {0};
    GameMaster gm = {0};
    gm.phase = PHASE_MENU;

    /* ============================================================
       LOOP PRINCIPAL
       ============================================================ */

    GamePhase lastPhase = gm.phase;
    Audio_PlayPhaseMusic(gm.phase, 1.0f);
    Audio_PlayPhaseSFX(gm.phase);

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        Audio_Update(dt);

        if (gm.phase != lastPhase) {
            Audio_PlayPhaseMusic(gm.phase, 2.0f);
            Audio_PlayPhaseSFX(gm.phase);
            ScreenFX_FadeFromBlack(0.3f);
            lastPhase = gm.phase;
        }

        if (gm.phase == PHASE_MENU) {
            BeginDrawing();
            MenuView_Draw(&mainMenu, GetScreenWidth(), GetScreenHeight());
            EndDrawing();

            if (mainMenu.startGame) {
                mainMenu.startGame = false;
                ArmySetup_InitDefault(&armySetup);
                gm.phase = PHASE_SETUP;
            }
            if (mainMenu.quitGame) {
                break;
            }
            continue;
        }

        if (gm.phase == PHASE_SETUP) {
            /* Punto de enganche para la futura ventana de elección de personajes:
               La vista modificará 'armySetup' (General, Tenientes y stats) y al
               confirmar invocará GameMaster_StartCampaign. */
            GameMaster_StartCampaign(&gm, &levelMap, &playerArmy, &armySetup, provinceLevel);
            Hud_InitState(&hudState);
            MapView_SyncToArmy(&mapView, playerArmy.position, &cc);
            Audio_SetAmbientTimeOfDay(gm.timeOfDay);
            continue;
        }

        MapView_UpdateWorldVisuals(&mapView, &gm, &playerArmy, &cc, dt);
        /* ========================================================
           CAMBIO DE PROVINCIA Y REGENERACIN
           ======================================================== */

        bool regenerate = false;

        if (gm.phase != PHASE_EVENT && gm.phase != PHASE_CITY && !gm.hasDayReport && !gm.showCampDecision && !hudState.showOfficersPanel && !hudState.showGroupsPanel && !GameMaster_RationPending(&gm) && !rationView.showingReport && !hudState.dice.rolling && !playerArmy.isMoving && IsKeyPressed(KEY_ENTER)) {
            Audio_PlaySfxByName("turn_end");
            GameMaster_EndTurn(&gm, &levelMap, &playerArmy);
            Map_UpdateVision(&levelMap, playerArmy.position, Army_GetVision(&playerArmy) + GameMaster_GetVisionModifier(&gm, &playerArmy));
        }

        if (gm.canInteractStructure && gm.phase == PHASE_PLAYER && !gm.hasDayReport && !gm.showCampDecision && !hudState.showOfficersPanel && !hudState.showGroupsPanel && !GameMaster_RationPending(&gm) && !rationView.showingReport) {
            if (IsKeyPressed(KEY_E)) {
                GameMaster_InteractStructure(&gm, &levelMap, &playerArmy);
            }
        }

        if (gm.phase == PHASE_PLAYER && !gm.hasDayReport && !gm.showCampDecision && !hudState.showOfficersPanel && !hudState.showGroupsPanel && !GameMaster_RationPending(&gm) && !rationView.showingReport) {
            int attackableNpcIdx = NPC_FindAttackableSquadNear(&gm, playerArmy.position);
            if (attackableNpcIdx >= 0 && IsKeyPressed(KEY_X)) {
                NPC_PlayerVoluntaryAttack(&gm, &playerArmy, attackableNpcIdx);
                playerArmy.isMoving = false;
            }
        }

        if (IsKeyPressed(KEY_F1)) mapView.debugRevealMap = !mapView.debugRevealMap;

        if (IsKeyPressed(KEY_R))
            regenerate = true;

        if (IsKeyPressed(KEY_ONE) && provinceLevel != 1)
        {
            provinceLevel = 1;
            regenerate = true;
        }
        else if (IsKeyPressed(KEY_TWO) && provinceLevel != 2)
        {
            provinceLevel = 2;
            regenerate = true;
        }
        else if (IsKeyPressed(KEY_THREE) && provinceLevel != 3)
        {
            provinceLevel = 3;
            regenerate = true;
        }
        else if (IsKeyPressed(KEY_FOUR) && provinceLevel != 4)
        {
            provinceLevel = 4;
            regenerate = true;
        }

        if (regenerate)
        {
            GameMaster_TransitionProvince(&gm, &levelMap, &playerArmy, provinceLevel);
            MapView_SyncToArmy(&mapView, playerArmy.position, &cc);
            
            /* Reposicionar ej�rcito */
        }


        /* ========================================================
           C�MARA
           ======================================================== */
        /* ========================================================
           MOVIMIENTO DEL EJERCITO
           ======================================================== */
                if (playerArmy.isMoving && gm.phase != PHASE_EVENT && !Sequence_IsBusy()) {
            if (playerArmy.currentPathIndex < playerArmy.pathLength) {
                HexCoord nextPos = playerArmy.movePath[playerArmy.currentPathIndex];
                HexTile* targetTile = NULL;
                for (int i = 0; i < levelMap.tileCount; i++) {
                    if (levelMap.tiles[i].coord.q == nextPos.q && levelMap.tiles[i].coord.r == nextPos.r) {
                        targetTile = &levelMap.tiles[i];
                        break;
                    }
                }
                
                if (targetTile && GameMaster_AttemptMove(&gm, &levelMap, &playerArmy, targetTile)) {
                    Map_UpdateVision(&levelMap, playerArmy.position, Army_GetVision(&playerArmy) + GameMaster_GetVisionModifier(&gm, &playerArmy));
                    playerArmy.currentPathIndex++;
                    
                    Vector2 targetPixel = HexToPixel(playerArmy.position, mapView.hexRadius);
                    
                    
                    SeqAction moveAct = {0};
                    moveAct.duration = 0.25f;
                    Sequence_Push(moveAct);
                    
                    Tween_Start(&mapView.visualArmyPos.x, targetPixel.x, 0.25f, Ease_Linear);
                    Tween_Start(&mapView.visualArmyPos.y, targetPixel.y, 0.25f, Ease_Linear);
                    Audio_PlayStepBiome(targetTile->biome);
                    ScreenFX_SpawnMarchDust((Vector2){ targetPixel.x + mapView.origin.x, targetPixel.y + mapView.origin.y }, (Color){ 195, 175, 130, 255 });
                    
                    for (int e = 0; e < gm.enemyCount; e++) {
                        if (gm.enemies[e].isActive && gm.enemies[e].position.q == playerArmy.position.q && gm.enemies[e].position.r == playerArmy.position.r) {
                            if (NPC_IsHostileToPlayer(&gm.enemies[e], &gm)) {
                                Audio_PlaySfxByName("enemy_alert");
                                bool ambushed = (gm.enemies[e].behavior == AI_AMBUSH || Enemy_HasTag(&gm.enemies[e], TAG_AMBUSH));
                                NPC_InitiateCombat(&gm, &playerArmy, &gm.enemies[e], 0, ambushed);
                                playerArmy.isMoving = false;
                                break;
                            }
                        }
                    }
                } else {
                    playerArmy.isMoving = false;
                }
            } else {
                playerArmy.isMoving = false;
            }
        }

        CameraController_Update(&cc, GetFrameTime());


        /* ========================================================
           ACTUALIZAR HUD Y DADOS
           ======================================================== */
           
        bool wasRolling = hudState.dice.rolling;
        Hud_Update(&hudState, GetFrameTime());
        if (wasRolling && !hudState.dice.rolling) {
            GameMaster_ApplyDiceRoll(&gm, &playerArmy, hudState.dice.valor1 + hudState.dice.valor2);
        }
        Assets_UpdateAnimations(&gameAssets, GetFrameTime());
        if (gm.phase != PHASE_EVENT && gm.phase != PHASE_CITY && !gm.hasDayReport && !gm.showCampDecision && !hudState.showOfficersPanel && !hudState.showGroupsPanel && !GameMaster_RationPending(&gm) && !rationView.showingReport) Hud_HandleInput(&hudState, &gm, &playerArmy, &levelMap);
        
        if (IsKeyPressed(KEY_SPACE) && gm.phase != PHASE_CITY && !gm.hasRolledThisTurn && !gm.hasDayReport && !gm.showCampDecision && !hudState.showOfficersPanel && !hudState.showGroupsPanel && !GameMaster_RationPending(&gm) && !rationView.showingReport) {
            Hud_RollDice(&hudState);
        }

        /* ========================================================
           MOUSE INPUT PARA MAPA
           ======================================================== */

        if (gm.phase != PHASE_EVENT && gm.phase != PHASE_CITY && !gm.hasDayReport && !gm.showCampDecision && !hudState.showOfficersPanel && !hudState.showGroupsPanel && !GameMaster_RationPending(&gm) && !rationView.showingReport && IsMouseButtonPressed(
                MOUSE_BUTTON_LEFT) && 
            !CheckCollisionPointRec(GetMousePosition(), hudState.cmdPanelRect) &&
            !CheckCollisionPointRec(GetMousePosition(), hudState.rollBtnRect) &&
            !CheckCollisionPointRec(GetMousePosition(), hudState.forageBtnRect) &&
            !CheckCollisionPointRec(GetMousePosition(), hudState.endTurnBtnRect))
        {
            /*
             * Posici�n del mouse en pantalla.
             */
            Vector2 mousePosition =
                GetMousePosition();

            /*
             * Convertir de coordenadas de pantalla
             * a coordenadas del mundo teniendo en cuenta
             * la c�mara y el zoom.
             */
            Vector2 worldPosition =
                GetScreenToWorld2D(mousePosition, cc.camera);

            /*
             * MapView dibuja los hex�gonos de esta forma:
             *
             * posici�n del hex�gono =
             * HexToPixel(coord) + origin
             *
             * Por lo tanto, quitamos origin antes
             * de hacer PixelToHex().
             */
            worldPosition.x -= mapView.origin.x;
            worldPosition.y -= mapView.origin.y;

            /*
             * Convertir posici�n del mouse
             * a coordenadas hexagonales.
             */
            HexCoord mouseHex =
                PixelToHex(
                    worldPosition,
                    mapView.hexRadius
                );


            /*
             * Buscar el HexTile correspondiente.
             */
                        for (int i = 0; i < levelMap.tileCount; i++) {
                            HexTile* tile = &levelMap.tiles[i];
            
                            if (tile->coord.q == mouseHex.q && tile->coord.r == mouseHex.r) {
                                if (tile == mapView.selectedTile && !playerArmy.isMoving) {
                                    if (gm.hasRolledThisTurn) {
                                        HexTile* startTile = Map_GetTile(&levelMap, playerArmy.position);
                                        if (startTile) {
                                            HexTile* path[128];
                                            int cost = 0;
                                            int pathLen = Map_FindPath(&levelMap, startTile, tile, &cost, path);
                                            if (pathLen > 0 && gm.currentActionPoints >= cost) {
                                                for (int p = 0; p < pathLen; p++) {
                                                    playerArmy.movePath[p] = path[p]->coord;
                                                }
                                                playerArmy.pathLength = pathLen;
                                                playerArmy.currentPathIndex = 0;
                                                playerArmy.moveTimer = 0.0f;
                                                playerArmy.isMoving = true;
                                                mapView.selectedTile = NULL;
                                            }
                                        }
                                    }
                                } else {
                                    mapView.selectedTile = tile;
                                }
                                break;
                            }
                        }
        }


        /* ========================================================
           DIBUJAR
           ======================================================== */

        BeginDrawing();

        ClearBackground(
            (Color){ 20, 24, 32, 255 }
        );


        Camera2D renderCam = cc.camera;
        ScreenFX_ApplyShakeToCamera(&renderCam);
        BeginMode2D(renderCam);


        Vector2 camCenterWorld = cc.camera.target;
        camCenterWorld.x -= mapView.origin.x;
        camCenterWorld.y -= mapView.origin.y;
        HexCoord viewCenter = PixelToHex(camCenterWorld, mapView.hexRadius);
        int dynamicRenderRadius = (int)(((GetScreenWidth() / cc.camera.zoom) / (mapView.hexRadius * 1.5f))) + 6;
        if (dynamicRenderRadius > 40) dynamicRenderRadius = 40;

        MapView_Draw(&levelMap, mapView.hexRadius, mapView.origin, false, viewCenter, dynamicRenderRadius, mapView.visualArmyPos, &gameAssets, mapView.debugRevealMap);

        Vector2 mousePosition = GetMousePosition();
        Vector2 worldPosition = GetScreenToWorld2D(mousePosition, cc.camera);
        worldPosition.x -= mapView.origin.x;
        worldPosition.y -= mapView.origin.y;
        HexCoord hoverHex = PixelToHex(worldPosition, mapView.hexRadius);

        // Dibujar NPCs / Facciones en el Mapa
        for (int i = 0; i < gm.enemyCount; i++) {
            HexTile* t = Map_GetTile(&levelMap, gm.enemies[i].position);
            if (t && (t->visible || mapView.debugRevealMap)) {
                bool isHovered = (hoverHex.q == gm.enemies[i].position.q && hoverHex.r == gm.enemies[i].position.r);
                NPCView_DrawOnMap(&gm.enemies[i], &gm, mapView.visualEnemyPos[i], mapView.origin, &gameAssets, isHovered, mapView.hexRadius);
            }
        }

        /*
         * Resaltar hover y tile seleccionado.
         */

        for (int i = 0; i < levelMap.tileCount; i++) {
            HexTile* t = &levelMap.tiles[i];
            if (t->coord.q == hoverHex.q && t->coord.r == hoverHex.r) {
                if (gm.hasRolledThisTurn && HexCoord_Distance(playerArmy.position, t->coord) == 1) {
                    MapView_DrawHexFillHighlight(t, mapView.hexRadius, mapView.origin, (Color){100, 200, 255, 80});
                }
                break;
            }
        }

        // Rutas y Token de Ejercito
        if (playerArmy.isMoving) {
            HexTile* pathTiles[128];
            int numTiles = 0;
            // Draw remaining path
            for (int i = playerArmy.currentPathIndex; i < playerArmy.pathLength; i++) {
                HexTile* pt = Map_GetTile(&levelMap, playerArmy.movePath[i]);
                if (pt) pathTiles[numTiles++] = pt;
            }
            if (numTiles > 0) {
                // Line from token to first remaining path node
                Vector2 p1 = mapView.visualArmyPos;
                p1.x += mapView.origin.x;
                float currentHeight = 0.0f;
                for (int k = 0; k < levelMap.tileCount; k++) {
                    Vector2 p = HexToPixel(levelMap.tiles[k].coord, mapView.hexRadius);
                    float dx = mapView.visualArmyPos.x - p.x;
                    float dy = mapView.visualArmyPos.y - p.y;
                    if (dx*dx + dy*dy < mapView.hexRadius*mapView.hexRadius) {
                        currentHeight = GetBiomeHeight(levelMap.tiles[k].biome);
                        break;
                    }
                }
                p1.y += mapView.origin.y - currentHeight;
                Vector2 p2 = HexToPixel(pathTiles[0]->coord, mapView.hexRadius);
                p2.x += mapView.origin.x;
                p2.y += mapView.origin.y - GetBiomeHeight(pathTiles[0]->biome);
                DrawLineEx(p1, p2, 4.0f, Fade(GOLD, 0.7f));
                
                if (numTiles > 1) {
                    MapView_DrawPath(pathTiles, numTiles, mapView.hexRadius, mapView.origin);
                }
            }
        } 
        else if (mapView.selectedTile != NULL && !gm.inCombat && gm.phase == PHASE_PLAYER)
        {
            if (!(mapView.selectedTile->coord.q == playerArmy.position.q && mapView.selectedTile->coord.r == playerArmy.position.r) && mapView.selectedTile->visited) {
                HexTile* startTile = Map_GetTile(&levelMap, playerArmy.position);
                if (startTile) {
                    HexTile* path[128];
                    int cost = 0;
                    int pathLen = Map_FindPath(&levelMap, startTile, mapView.selectedTile, &cost, path);
                    Color pathColor = (gm.currentActionPoints >= cost) ? GREEN : RED;
                    
                    if (pathLen > 0) {
                        MapView_DrawHexHighlight(mapView.selectedTile, mapView.hexRadius, mapView.origin, pathColor);
                        
                        // Line from token to first path node
                        Vector2 p1 = mapView.visualArmyPos;
                        p1.x += mapView.origin.x;
                        float currentHeight = 0.0f;
                        for (int k = 0; k < levelMap.tileCount; k++) {
                            Vector2 p = HexToPixel(levelMap.tiles[k].coord, mapView.hexRadius);
                            float dx = mapView.visualArmyPos.x - p.x;
                            float dy = mapView.visualArmyPos.y - p.y;
                            if (dx*dx + dy*dy < mapView.hexRadius*mapView.hexRadius) {
                                currentHeight = GetBiomeHeight(levelMap.tiles[k].biome);
                                break;
                            }
                        }
                        p1.y += mapView.origin.y - currentHeight;
                        Vector2 p2 = HexToPixel(path[0]->coord, mapView.hexRadius);
                        p2.x += mapView.origin.x;
                        p2.y += mapView.origin.y - GetBiomeHeight(path[0]->biome);
                        DrawLineEx(p1, p2, 4.0f, Fade(GOLD, 0.7f));
                        
                        MapView_DrawPath(path, pathLen, mapView.hexRadius, mapView.origin);
                        
                        int midIdx = pathLen / 2;
                        Vector2 midPx = HexToPixel(path[midIdx]->coord, mapView.hexRadius);
                        midPx.x += mapView.origin.x; midPx.y += mapView.origin.y - GetBiomeHeight(path[midIdx]->biome);
                        
                        DrawCircleV(midPx, 18.0f, DARKGRAY);
                        DrawCircleLines(midPx.x, midPx.y, 18.0f, pathColor);
                        const char* costTxt = TextFormat("%d PA", cost);
                        Vector2 costSz = UI_MeasureText(costTxt, 11);
                        UI_DrawText(costTxt, (Vector2){ midPx.x - costSz.x/2.0f, midPx.y - 6.0f }, 11, WHITE);
                    }
                }
            } else {
                MapView_DrawHexHighlight(mapView.selectedTile, mapView.hexRadius, mapView.origin, GREEN);
            }
        }

        ScreenFX_DrawMapParticles();

        EndMode2D();
        BeginBlendMode(BLEND_MULTIPLIED);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ (unsigned char)mapView.skyR, (unsigned char)mapView.skyG, (unsigned char)mapView.skyB, 255 });
        EndBlendMode();

        if (gm.clock.timeOfDay == TIME_NIGHT) {
            ScreenFX_DrawNightAtmosphere(GetScreenWidth(), GetScreenHeight());
        }

        /* Indicadores direccionales de rumores fuera de vista (D3) */
        MapView_DrawOffscreenRumors(&levelMap, cc.camera, mapView.hexRadius, mapView.origin, playerArmy.position);


        /* ========================================================
           NUEVO HUD DEL EJERCITO
           ======================================================== */
           
        int SW = GetScreenWidth();
        int SH = GetScreenHeight();

        int bottomH = 165;
        int bottomY = SH - bottomH - 12;

        /* Panel comandante (arriba izquierda) */
        Hud_DrawCommanderPanel(&playerArmy, &gm, &hudState, 12, 48, 430);

        /* Panel stats ejercito (abajo izquierda) */
        int statsW = 450;
        Hud_DrawArmyStatsPanel(&playerArmy, &gm, &levelMap, &hudState, 12, bottomY, statsW, bottomH);
        Hud_DrawTopBar(&gm, SW);

        /* Panel dados (abajo centro) */
        int diceW = 215;
        int diceX = 12 + statsW + 12;
        Hud_DrawDicePanel(&hudState, &gm, &playerArmy, &levelMap, diceX, bottomY, diceW, bottomH);

        /* Minimapa (abajo derecha) */
        int minimapStart = diceX + diceW + 12;
        float minimapW = (SW > 1024) ? 420.0f : (float)(SW - minimapStart - 12);
        if (minimapW < 180.0f) minimapW = 180.0f;
        float minimapX = (float)(SW - minimapW - 12);
        Rectangle minimapRect = { minimapX, (float)bottomY, minimapW, (float)bottomH };
        UI_DrawPanelStyled(minimapRect, UI_STYLE_PANEL_DARK);
        MapView_DrawMinimap(&levelMap, minimapRect, playerArmy.position, hudState.blinkTimer, &gameAssets, mapView.debugRevealMap);

        if (gm.canInteractStructure && gm.phase == PHASE_PLAYER && !gm.hasDayReport && !gm.showCampDecision) {
            const char* promptText = "[E] Interactuar con Asentamiento";
            Vector2 pSize = UI_MeasureText(promptText, 20);
            int px = SW / 2 - (int)pSize.x / 2;
            int py = bottomY - 50;
            DrawRectangle(px - 18, py - 7, (int)pSize.x + 36, 36, (Color){ 20, 25, 35, 235 });
            DrawRectangleLinesEx((Rectangle){ (float)(px - 18), (float)(py - 7), pSize.x + 36.0f, 36.0f }, 2.0f, GOLD);
            UI_DrawText(promptText, (Vector2){ (float)px, (float)py }, 20, GOLD);
        }

        /* Indicador contextual de Ataque Voluntario a NPC [X] */
        if (gm.phase == PHASE_PLAYER && !gm.hasDayReport && !gm.showCampDecision) {
            int attackNpcIdx = NPC_FindAttackableSquadNear(&gm, playerArmy.position);
            if (attackNpcIdx >= 0) {
                EnemySquad* targetNpc = &gm.enemies[attackNpcIdx];
                bool isHostile = NPC_IsHostileToPlayer(targetNpc, &gm);
                const char* promptText = TextFormat("[X] Atacar a %s%s", targetNpc->name, isHostile ? "" : " (No hostil)");
                Vector2 pSize = UI_MeasureText(promptText, 20);
                int px = SW / 2 - (int)pSize.x / 2;
                int py = gm.canInteractStructure ? (bottomY - 95) : (bottomY - 50);
                Rectangle btnRect = { (float)(px - 18), (float)(py - 7), pSize.x + 36.0f, 36.0f };
                Color boxBg = isHostile ? (Color){ 45, 20, 20, 235 } : (Color){ 35, 30, 20, 235 };
                Color borderCol = isHostile ? RED : GOLD;
                DrawRectangleRec(btnRect, boxBg);
                DrawRectangleLinesEx(btnRect, 2.0f, borderCol);
                UI_DrawText(promptText, (Vector2){ (float)px, (float)py }, 20, borderCol);

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), btnRect)) {
                    NPC_PlayerVoluntaryAttack(&gm, &playerArmy, attackNpcIdx);
                    playerArmy.isMoving = false;
                }
            }
        }

        /* Tooltip de Exploracion */
        if (mapView.selectedTile != NULL && !gm.inCombat && gm.phase != PHASE_EVENT && gm.phase != PHASE_CITY)
        {
            BiomeProperties props = Biome_GetProperties(mapView.selectedTile->biome);
            Vector2 mPos = GetMousePosition();
            
            int foodCost = props.actionPointCost * 2; // Ejemplo base
            
            int tw = 220;
            int th = 96;
            int px = mPos.x + 15;
            int py = mPos.y + 15;
            
            // Keep tooltip inside screen bounds
            if (px + tw > SW) px = mPos.x - tw - 15;
            if (py + th > SH) py = mPos.y - th - 15;
            
            DrawRectangle(px, py, tw, th, (Color){ 20, 20, 25, 235 });
            DrawRectangleLinesEx((Rectangle){ (float)px, (float)py, (float)tw, (float)th }, 2.0f, (Color){ 140, 105, 50, 255 });

            UI_DrawText(props.name, (Vector2){ (float)(px+12), (float)(py+10) }, 18, GOLD);
            if (!mapView.selectedTile->visited && !mapView.selectedTile->visible) {
                UI_DrawText("Desconocido", (Vector2){ (float)(px+12), (float)(py+38) }, 15, GRAY);
            } else {
                UI_DrawText(TextFormat("Costo: %d PA", props.actionPointCost), (Vector2){ (float)(px+12), (float)(py+38) }, 15, RAYWHITE);
                UI_DrawText(TextFormat("Consumo: %d Suministros", foodCost), (Vector2){ (float)(px+12), (float)(py+62) }, 15, RED);
            }
        }

        if (gm.pendingCombatEnemy >= 0 && !Sequence_IsBusy()) {
            Audio_PlaySfxByName("enemy_alert");
            int targetEnemyIdx = gm.pendingCombatEnemy;
            NPC_InitiateCombat(&gm, &playerArmy, &gm.enemies[targetEnemyIdx], 
                gm.enemies[targetEnemyIdx].initiator, gm.activeCombat.ambushed);
            playerArmy.isMoving = false;
            gm.pendingCombatEnemy = -1;
        }

        if (gm.inCombat) {
            if (CombatView_Update(&gm.activeCombat, SW, SH)) {
                gm.inCombat = false;
                if (Army_GetTotalWeight(&playerArmy) <= 0) {
                    gm.phase = PHASE_GAME_OVER;
                    Audio_PlaySfxByName("game_over");
                } else {
                    if (gm.inSiegeCombat) {
                        if (gm.activeCombat.endReason == COMBAT_END_VICTORY) {
                            Audio_PlaySfxByName("siege_victory");
                            GameMaster_ResolveSiegeVictory(&gm, &levelMap, &playerArmy, gm.siegeTargetCoord);
                        } else {
                            gm.inSiegeCombat = false;
                        }
                    }
                    gm.phase = PHASE_PLAYER;
                    Map_UpdateVision(&levelMap, playerArmy.position, Army_GetVision(&playerArmy) + GameMaster_GetVisionModifier(&gm, &playerArmy));
                    Vector2 retreatTarget = HexToPixel(playerArmy.position, mapView.hexRadius);
                    if (gm.activeCombat.endReason == COMBAT_END_DEFEAT || gm.activeCombat.endReason == COMBAT_END_FLEE) {
                        SeqAction retAct = {0};
                        retAct.duration = 0.45f;
                        Sequence_Push(retAct);
                        Tween_Start(&mapView.visualArmyPos.x, retreatTarget.x, 0.45f, Ease_OutCubic);
                        Tween_Start(&mapView.visualArmyPos.y, retreatTarget.y, 0.45f, Ease_OutCubic);
                    } else {
                        mapView.visualArmyPos = retreatTarget;
                        cc.camera.target = mapView.visualArmyPos;
                    }
                    Audio_SetMusicContext(MUSIC_CTX_MAIN);
                }
            }
        } else if (gm.phase == PHASE_EVENT) { gm.eventTimer += GetFrameTime(); }
        if (gm.inCombat) {
            CombatView_Draw(&gm.activeCombat, SW, SH);
        } else if (gm.phase == PHASE_DRAFT) {
            if (activeDraft.registry == NULL) {
                DraftView_Init(&activeDraft, &gm.modRegistry);
            }
            if (DraftView_UpdateDraw(&activeDraft, &playerArmy, SW, SH) == 1) {
                gm.phase = PHASE_PLAYER;
                activeDraft.registry = NULL;
            }
        } else if (gm.phase == PHASE_CITY) {
            if (CityView_UpdateDraw(&gm.activeCity, &playerArmy, &gm, &levelMap, SW, SH)) {
                GameMaster_CloseCity(&gm, &levelMap, &playerArmy);
            }
        } else if (gm.phase == PHASE_EVENT && (gm.activeEvent || gm.activeResult)) {
            static void* s_lastActiveEvent = NULL;
            if (gm.activeEvent != s_lastActiveEvent && gm.activeEvent != NULL) {
                Audio_PlaySfxByName("event_open");
                s_lastActiveEvent = gm.activeEvent;
            } else if (gm.activeEvent == NULL && gm.activeResult == NULL) {
                s_lastActiveEvent = NULL;
            }

            if (gm.activeResult) {
                ResultView_Draw(gm.activeResult, SW, SH);
                int r_opt = (gm.eventTimer > 1.0f) ? ResultView_HandleInput(gm.activeResult, SW, SH) : -1;
                if (r_opt == 0) {
                    Event_ApplyResult(gm.activeResult, &playerArmy, &gm);
                    for (int ei = 0; ei < gm.activeResult->effectCount; ei++) {
                        EffectDef* eff = &gm.activeResult->effects[ei];
                        if (eff->type == EFFECT_ADD_MOD && strlen(eff->stringValue) > 0) {
                            ModifierDef* def = Modifier_GetById(gm.modRegistry.defs, gm.modRegistry.count, eff->stringValue);
                            if (def) {
                                Army_EquipModifier(&playerArmy, def);
                                if (def->kind == MOD_KIND_CURSE) Audio_PlaySfxByName("curse_acquired");
                            }
                        } else if (eff->type == EFFECT_REMOVE_MOD && strlen(eff->stringValue) > 0) {
                            Army_RemoveModifier(&playerArmy, eff->stringValue);
                        }
                    }

                    if (gm.activeResult->startsBattle) {
                        gm.pendingCityAfterEvent = false;
                        static EnemySquad eventEnemy;
                        bool ambushed = false;
                        int initiator = 0;
                        eventEnemy = NPC_CreateEventSquad(gm.activeResult->battleEnemy, gm.activeResult->id, &ambushed, &initiator);
                        NPC_InitiateCombat(&gm, &playerArmy, &eventEnemy, initiator, ambushed);
                    } else {
                        /* Retorno del General depuesto (E1) */
                        if (strcmp(gm.activeResult->id, "res_general_regresa_si") == 0) {
                            Army_RestoreGeneral(&playerArmy);
                        } else if (strcmp(gm.activeResult->id, "res_general_regresa_no") == 0) {
                            Army_DismissGeneral(&playerArmy);
                        }

                        /* Transición tras evento disparado por presionar [E] en estructura (Prioridad 2) */
                        if (gm.pendingCityAfterEvent) {
                            gm.pendingCityAfterEvent = false;
                            HexTile* cTile = Map_GetTile(&levelMap, gm.pendingCityCoord);
                            int fId = cTile ? cTile->structure.factionId : -1;
                            bool isHostile = false;
                            if (fId >= 0 && fId < 3) {
                                const BalanceConfig* bcfg = Balance_GetGlobal();
                                isHostile = (Faction_GetLevel(gm.factions[fId].opinion, bcfg) == FACTION_HOSTILE);
                            }
                            if (gm.pendingCityBlocked || isHostile || GameMaster_HasFlag(&gm, "bloqueo_ciudad") || GameMaster_HasFlag(&gm, "puertas_cerradas")) {
                                gm.phase = PHASE_PLAYER;
                            } else {
                                GameMaster_OpenCity(&gm, &levelMap, &playerArmy);
                            }
                        } else {
                            gm.phase = PHASE_PLAYER;
                        }
                    }
                    gm.activeResult = NULL;
                }
            } else if (gm.activeEvent) {
                Event_EvaluateOptions(gm.activeEvent, &playerArmy, &gm);
                EventView_Draw(gm.activeEvent, SW, SH);
                
                int opt = (gm.eventTimer > 1.0f) ? EventView_HandleInput(gm.activeEvent, SW, SH) : -1;
                if (opt >= 0 && opt < gm.activeEvent->optionCount) {
                    EventOption* chosenOpt = &gm.activeEvent->options[opt];
                    
                    if (chosenOpt->resultPoolCount > 0) {
                        /* Sorteo de resultados ponderado por pesos (Prioridad 5) */
                        int totalWeight = 0;
                        for (int w = 0; w < chosenOpt->resultPoolCount; w++) {
                            totalWeight += (chosenOpt->resultWeights[w] > 0) ? chosenOpt->resultWeights[w] : 1;
                        }
                        int roll = GetRandomValue(1, totalWeight);
                        int accum = 0;
                        int chosenIdx = 0;
                        for (int w = 0; w < chosenOpt->resultPoolCount; w++) {
                            accum += (chosenOpt->resultWeights[w] > 0) ? chosenOpt->resultWeights[w] : 1;
                            if (roll <= accum) {
                                chosenIdx = w;
                                break;
                            }
                        }

                        const char* resId = chosenOpt->resultPool[chosenIdx];
                        EventResult* foundRes = GameMaster_FindResult(&gm, resId);
                        
                        if (foundRes) {
                            gm.activeResult = foundRes;
                            gm.activeEvent = NULL;
                            gm.eventTimer = 0.0f; 
                        } else {
                            gm.phase = PHASE_PLAYER;
                            gm.activeEvent = NULL;
                        }
                    } else {
                        gm.phase = PHASE_PLAYER;
                        gm.activeEvent = NULL;
                    }
                }
            }
        }

        static bool telemetryWritten = false;
        if ((gm.phase == PHASE_GAME_OVER || gm.phase == PHASE_VICTORY) && !telemetryWritten) {
            FILE* tel = fopen("telemetry_run.txt", "a");
            if (tel) {
                fprintf(tel, "--- RUN FINALIZADA ---\n");
                fprintf(tel, "Resultado: %s\n", gm.phase == PHASE_VICTORY ? "VICTORIA" : "DERROTA");
                fprintf(tel, "Dia: %d, Turno: %d\n", gm.currentDay, gm.currentTurn);
                fprintf(tel, "Infamia Final: %.1f\n", gm.infamy);
                fprintf(tel, "Oro: %d, Comida: %.0f, Agua: %.0f\n", playerArmy.monedas, playerArmy.comida_disponible, playerArmy.agua_disponible);
                fprintf(tel, "Supervivientes: Iniciados:%d Soldados:%d Sargentos:%d\n\n", playerArmy.iniciados, playerArmy.soldados, playerArmy.sargentos);
                fclose(tel);
            }
            telemetryWritten = true;
        }

        if (gm.phase == PHASE_GAME_OVER) {
            DrawRectangle(0, 0, SW, SH, Fade(BLACK, 0.8f));
            Vector2 sz1 = UI_MeasureText("DERROTA TOTAL", 40);
            UI_DrawText("DERROTA TOTAL", (Vector2){ SW/2.0f - sz1.x/2.0f, SH/2.0f - 40.0f }, 40, RED);
            Vector2 sz2 = UI_MeasureText("Tu ejercito ha sido aniquilado.", 20);
            UI_DrawText("Tu ejercito ha sido aniquilado.", (Vector2){ SW/2.0f - sz2.x/2.0f, SH/2.0f + 20.0f }, 20, LIGHTGRAY);
        } else if (gm.phase == PHASE_VICTORY) {
            DrawRectangle(0, 0, SW, SH, Fade(BLACK, 0.8f));
            Vector2 sz1 = UI_MeasureText("VICTORIA", 40);
            UI_DrawText("VICTORIA", (Vector2){ SW/2.0f - sz1.x/2.0f, SH/2.0f - 40.0f }, 40, GOLD);
            Vector2 sz2 = UI_MeasureText("Has conquistado el ultimo objetivo.", 20);
            UI_DrawText("Has conquistado el ultimo objetivo.", (Vector2){ SW/2.0f - sz2.x/2.0f, SH/2.0f + 20.0f }, 20, LIGHTGRAY);
        }

        /* Modal de Reparto de Raciones (cuando GameMaster_RationPending(&gm) o mostrando reporte de raciones) */
        if (GameMaster_RationPending(&gm) || rationView.showingReport) {
            if (!rationView.initialized && GameMaster_RationPending(&gm)) {
                RationView_Init(&rationView, &playerArmy, &levelMap, playerArmy.position);
                rationView.stragglersRegrouped = GameMaster_HasFlag(&gm, "reagrupamiento_reciente");
            }
            RationViewAction rAct = RationView_Update(&rationView, &playerArmy, &levelMap, playerArmy.position, SW, SH);
            if (rAct.type == RATION_ACTION_SUBMIT_PLAN) {
                GameMaster_SubmitRation(&gm, &playerArmy, &rAct.plan);
                RationView_ShowReport(&rationView, &gm.lastDayReport);
            } else if (rAct.type == RATION_ACTION_SLAUGHTER_CATTLE) {
                GameMaster_SlaughterCattle(&gm, &playerArmy, rAct.cattleHeads);
            } else if (rAct.type == RATION_ACTION_CLOSE_REPORT) {
                rationView.initialized = false;
                rationView.showingReport = false;
                gm.hasDayReport = false;
                GameMaster_ClearFlag(&gm, "reagrupamiento_reciente");

                if (gm.lastDayReport.foodShortage || gm.lastDayReport.waterShortage || Army_DaysOfSupply(&playerArmy) < 2.0f) {
                    Audio_PlaySfxByName("low_supply_warning");
                }
                /* Avisos telegrafiados de motín tras cerrar el reporte (E2) */
                if (gm.lastDayReport.triggerWarn2) {
                    Audio_PlaySfxByName("mutiny_warning");
                    EventDef* wEvt = Event_FindById(gm.events, gm.eventCount, "ev_mutiny_warn2");
                    if (wEvt) {
                        gm.phase = PHASE_EVENT;
                        gm.activeEvent = wEvt;
                        gm.eventTimer = 0.0f;
                    }
                } else if (gm.lastDayReport.triggerWarn1) {
                    Audio_PlaySfxByName("mutiny_warning");
                    EventDef* wEvt = Event_FindById(gm.events, gm.eventCount, "ev_mutiny_warn1");
                    if (wEvt) {
                        gm.phase = PHASE_EVENT;
                        gm.activeEvent = wEvt;
                        gm.eventTimer = 0.0f;
                    }
                } else if (playerArmy.lieutenantCount > 0 && GetRandomValue(1, 100) <= 25) {
                    /* Consejo de un teniente (E3) */
                    EventDef* pool[16]; int pc = 0;
                    for (int ei = 0; ei < gm.eventCount && pc < 16; ei++) {
                        if (gm.events[ei].hasTriggerKey && gm.events[ei].triggers[TRIGGER_ADVICE]) pool[pc++] = &gm.events[ei];
                    }
                    if (pc > 0) {
                        gm.phase = PHASE_EVENT;
                        gm.activeEvent = pool[GetRandomValue(0, pc - 1)];
                        gm.eventTimer = 0.0f;
                    }
                }
            }
            RationView_Draw(&rationView, &playerArmy, &levelMap, playerArmy.position, SW, SH);
        } else if (gm.hasDayReport) {
            Hud_DrawDayReportModal(&gm.lastDayReport, SW, SH);
            if (Hud_HandleDayReportInput(&gm, SW, SH)) {
                if (gm.lastDayReport.foodShortage || gm.lastDayReport.waterShortage || Army_DaysOfSupply(&playerArmy) < 2.0f) {
                    Audio_PlaySfxByName("low_supply_warning");
                }
                /* Avisos telegrafiados de motín tras cerrar el reporte (E2) */
                if (gm.lastDayReport.triggerWarn2) {
                    Audio_PlaySfxByName("mutiny_warning");
                    EventDef* wEvt = Event_FindById(gm.events, gm.eventCount, "ev_mutiny_warn2");
                    if (wEvt) {
                        gm.phase = PHASE_EVENT;
                        gm.activeEvent = wEvt;
                        gm.eventTimer = 0.0f;
                    }
                } else if (gm.lastDayReport.triggerWarn1) {
                    Audio_PlaySfxByName("mutiny_warning");
                    EventDef* wEvt = Event_FindById(gm.events, gm.eventCount, "ev_mutiny_warn1");
                    if (wEvt) {
                        gm.phase = PHASE_EVENT;
                        gm.activeEvent = wEvt;
                        gm.eventTimer = 0.0f;
                    }
                } else if (playerArmy.lieutenantCount > 0 && GetRandomValue(1, 100) <= 25) {
                    /* Consejo de un teniente (E3) */
                    EventDef* pool[16]; int pc = 0;
                    for (int ei = 0; ei < gm.eventCount && pc < 16; ei++) {
                        if (gm.events[ei].hasTriggerKey && gm.events[ei].triggers[TRIGGER_ADVICE]) pool[pc++] = &gm.events[ei];
                    }
                    if (pc > 0) {
                        gm.phase = PHASE_EVENT;
                        gm.activeEvent = pool[GetRandomValue(0, pc - 1)];
                        gm.eventTimer = 0.0f;
                    }
                }
            }
        }

        if (gm.showCampDecision) {
            Hud_DrawCampDecisionModal(&gm, &playerArmy, &levelMap, SW, SH);
            Hud_HandleCampDecisionInput(&gm, &levelMap, &playerArmy, SW, SH);
        }

        if (hudState.showOfficersPanel) {
            Hud_DrawOfficersPanel(&playerArmy, &gm, &hudState, SW, SH);
            Hud_HandleOfficersPanelInput(&hudState, SW, SH);
        }

        if (hudState.showGroupsPanel) {
            Hud_DrawGroupsPanel(&playerArmy, &gm, &hudState, SW, SH);
            Hud_HandleGroupsPanelInput(&hudState, SW, SH);
        }

        ScreenFX_DrawOverlays();
        UI_DrawPendingTooltips();
        EndDrawing();
    }


    /* ============================================================
       LIBERAR
       ============================================================ */

    Map_Free(&levelMap);

    Assets_Unload(&gameAssets);
    CityView_UnloadAssets();
    Audio_Shutdown();
    UI_Shutdown();
    CloseWindow();

    return 0;
}





































