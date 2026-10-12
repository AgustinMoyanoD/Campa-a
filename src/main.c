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
#include "models/menu.h"
#include "views/menu_view.h"
#include "views/event_view.h"
#include "views/combat_view.h"
#include "views/enemy_view.h"
#include "views/draft_view.h"
#include "views/city_view.h"

#include <stdlib.h>
#include <time.h>


int main(void)
{
    const int screenWidth = 1024;
    const int screenHeight = 768;

    const int mapRadius = 70;
    const int landProbabilityPercent = 55;

    const float hexRadius = 35.0f;


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
    Audio_Init();
    UI_Init();


    srand(
        (unsigned int)time(NULL)
    );


        /* ============================================================
       CARGAR ASSETS
       ============================================================ */

    GameAssets gameAssets = {0};
    Assets_Init(&gameAssets);


    /* ============================================================
       CENTRO DE LA PANTALLA
       ============================================================ */

    Vector2 center = {
        GetScreenWidth() / 2.0f,
        GetScreenHeight() / 2.0f
    };


    int provinceLevel = 1;
    Vector2 origin = (Vector2){ 0.0f, 0.0f };
    CameraController cc;
    CameraController_Init(&cc, center);

    /* ============================================================
       MAPA
       ============================================================ */

    Map levelMap = {0};
    HudState hudState = {0};
    Army playerArmy = {0};
    GameMaster gm = {0};
    DraftContext activeDraft = {0};
    
    gm.phase = PHASE_MENU;

    MainMenu mainMenu;
    Menu_Init(&mainMenu);
    MenuView_Init();
    Vector2 visualArmyPos = {0};
    Vector2 visualEnemyPos[64] = {0};
    bool enemyFirstFrame[64] = {0};
    bool enemyIsTweening[64] = {0};
    for (int i = 0; i < 64; i++) enemyFirstFrame[i] = true;
    bool firstFrame = true;
    
    float skyR = 255.0f, skyG = 255.0f, skyB = 255.0f;
    TimeOfDay lastTimeOfDay = gm.timeOfDay;


    /* ============================================================
       LOOP PRINCIPAL
       ============================================================ */

    bool debugRevealMap = false;
    HexTile* selectedTile = NULL;
    GamePhase lastPhase = gm.phase;
    Audio_PlayPhaseMusic(gm.phase, 1.0f);
    Audio_PlayPhaseSFX(gm.phase);

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        
        if (gm.phase != lastPhase) {
            Audio_PlayPhaseMusic(gm.phase, 2.0f);
            Audio_PlayPhaseSFX(gm.phase);
            lastPhase = gm.phase;
        }

        if (gm.phase == PHASE_MENU) {
            BeginDrawing();
            MenuView_Draw(&mainMenu, screenWidth, screenHeight);
            EndDrawing();

            if (mainMenu.startGame) {
                levelMap = Map_GenerateProcedural(mapRadius, landProbabilityPercent, 3, provinceLevel);
                hudState = Hud_Init();
                
                General generalPrueba = General_Create("Cayo Marcio", 38, 1, 3, 3, 2, 5);
                HexTile* startTile = NULL;
                for (int i = 0; i < levelMap.tileCount; i++) {
                    if (levelMap.tiles[i].special == TILE_SPECIAL_START) {
                        startTile = &levelMap.tiles[i]; break;
                    }
                }
                HexCoord startCoord = startTile ? startTile->coord : HexCoord_Create(0,0);
                
                playerArmy = Army_Create(startCoord, generalPrueba, 8, 40, 15, 200.0f, 180.0f, 350, 12);
                Lieutenant lt = Lieutenant_Create("Publio Varro", NULL, 26, 2, 1, 1, 0, 2, 72);
                Army_AddLieutenant(&playerArmy, lt);
                Army_SetCommanderGeneral(&playerArmy);
                
                GameMaster_Init(&gm);
                
                
                GameMaster_SpawnTestEnemies(&gm, &levelMap, playerArmy.position);
                GameMaster_StartTurn(&gm, &playerArmy);
                
                Map_UpdateVision(&levelMap, playerArmy.position, Army_GetVision(&playerArmy) + GameMaster_GetVisionModifier(&gm, &playerArmy));
                visualArmyPos = HexToPixel(playerArmy.position, hexRadius);
                cc.camera.target = visualArmyPos;
                mainMenu.startGame = false; // consume input
                // Note: GameMaster_Init sets gm.phase = PHASE_PLAYER implicitly
            }
            if (mainMenu.quitGame) {
                break;
            }
            continue;
        }

        if (lastTimeOfDay != gm.timeOfDay) {
            float targetR = 255.0f, targetG = 255.0f, targetB = 255.0f;
            switch(gm.timeOfDay) {
                case TIME_MORNING: targetR=255.0f; targetG=240.0f; targetB=220.0f; break; // Alba calida
                case TIME_MIDDAY:  targetR=255.0f; targetG=255.0f; targetB=255.0f; break; // Sol pleno
                case TIME_AFTERNOON:targetR=255.0f; targetG=200.0f; targetB=150.0f; break; // Atardecer naranja
                case TIME_NIGHT:   targetR=100.0f; targetG=120.0f; targetB=180.0f; break; // Noche azulada
            }
            Tween_Start(&skyR, targetR, 1.5f, Ease_Linear);
            Tween_Start(&skyG, targetG, 1.5f, Ease_Linear);
            Tween_Start(&skyB, targetB, 1.5f, Ease_Linear);
            lastTimeOfDay = gm.timeOfDay;
        }
        Tween_Update(dt);
        Sequence_Update(dt);
        ScreenFX_Update(dt);
        Audio_Update(dt);
        if (playerArmy.isMoving || Sequence_IsBusy()) {
            cc.camera.target = visualArmyPos;
        }
        if (firstFrame) { Vector2 p = HexToPixel(playerArmy.position, hexRadius); visualArmyPos = p; firstFrame = false; }
        
        bool anyEnemyMoving = false;
        for (int i = 0; i < gm.enemyCount; i++) {
            if (!gm.enemies[i].isActive) continue;
            Vector2 target = HexToPixel(gm.enemies[i].position, hexRadius);
            if (enemyFirstFrame[i]) {
                visualEnemyPos[i] = target;
                enemyFirstFrame[i] = false;
            }
            float dist = Vector2Distance(visualEnemyPos[i], target);
            if (dist > 1.0f) {
                if (!enemyIsTweening[i]) {
                    Tween_Start(&visualEnemyPos[i].x, target.x, 0.4f, Ease_OutCubic);
                    Tween_Start(&visualEnemyPos[i].y, target.y, 0.4f, Ease_OutCubic);
                    enemyIsTweening[i] = true;
                    anyEnemyMoving = true;
                }
            } else {
                enemyIsTweening[i] = false;
            }
        }
        if (anyEnemyMoving) {
            SeqAction enemyMoveAct = {0};
            enemyMoveAct.duration = 0.45f;
            Sequence_Push(enemyMoveAct);
        }
        /* ========================================================
           CAMBIO DE PROVINCIA Y REGENERACI�N
           ======================================================== */

        bool regenerate = false;

        if (gm.phase != PHASE_EVENT && gm.phase != PHASE_CITY && !gm.hasDayReport && !gm.showCampDecision && IsKeyPressed(KEY_ENTER)) {
            GameMaster_EndTurn(&gm, &levelMap, &playerArmy);
            Map_UpdateVision(&levelMap, playerArmy.position, Army_GetVision(&playerArmy) + GameMaster_GetVisionModifier(&gm, &playerArmy));
        }

        if (gm.canInteractStructure && gm.phase == PHASE_PLAYER && !gm.hasDayReport && !gm.showCampDecision) {
            if (IsKeyPressed(KEY_E)) {
                GameMaster_OpenCity(&gm, &levelMap, &playerArmy);
            }
        }

        if (IsKeyPressed(KEY_F1)) debugRevealMap = !debugRevealMap;

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
            Map_Free(&levelMap);

            levelMap =
                Map_GenerateProcedural(
                    mapRadius,
                    landProbabilityPercent,
                    3,
                    provinceLevel
                );

            selectedTile = NULL;
            
            /* Reposicionar ej�rcito */
            for (int i = 0; i < levelMap.tileCount; i++) { if (levelMap.tiles[i].special == TILE_SPECIAL_START) { playerArmy.position = levelMap.tiles[i].coord; break; } } visualArmyPos = HexToPixel(playerArmy.position, hexRadius); cc.camera.target = visualArmyPos; Map_UpdateVision(&levelMap, playerArmy.position, Army_GetVision(&playerArmy) + GameMaster_GetVisionModifier(&gm, &playerArmy));
    {
        Vector2 pos = HexToPixel(playerArmy.position, hexRadius);
        pos.x += origin.x;
        pos.y += origin.y;
        CameraController_FocusOn(&cc, pos);
    }
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
                    
                    Vector2 targetPixel = HexToPixel(playerArmy.position, hexRadius);
                    
                    
                    SeqAction moveAct = {0};
                    moveAct.duration = 0.25f;
                    Sequence_Push(moveAct);
                    
                    Tween_Start(&visualArmyPos.x, targetPixel.x, 0.25f, Ease_Linear);
                    Tween_Start(&visualArmyPos.y, targetPixel.y, 0.25f, Ease_Linear);
                    
                    for (int e = 0; e < gm.enemyCount; e++) {
                        if (gm.enemies[e].isActive && gm.enemies[e].position.q == playerArmy.position.q && gm.enemies[e].position.r == playerArmy.position.r) {
                            gm.inCombat = true;
                            gm.phase = PHASE_EVENT;
                            gm.enemies[e].initiator = 0;
                            gm.activeCombat.initiator = 0;
                            gm.activeCombat.ambushed = (gm.enemies[e].behavior == AI_AMBUSH || Enemy_HasTag(&gm.enemies[e], TAG_AMBUSH));
                            Combat_Init(&gm.activeCombat, &gm, &playerArmy, &gm.enemies[e]);
                            playerArmy.isMoving = false;
                            break;
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
        if (gm.phase != PHASE_EVENT && gm.phase != PHASE_CITY && !gm.hasDayReport && !gm.showCampDecision) Hud_HandleInput(&hudState, &gm, &playerArmy, &levelMap);
        
        if (IsKeyPressed(KEY_SPACE) && gm.phase != PHASE_CITY && !gm.hasRolledThisTurn && !gm.hasDayReport && !gm.showCampDecision) {
            Hud_RollDice(&hudState);
        }

        /* ========================================================
           MOUSE INPUT PARA MAPA
           ======================================================== */

        if (gm.phase != PHASE_EVENT && gm.phase != PHASE_CITY && !gm.hasDayReport && !gm.showCampDecision && IsMouseButtonPressed(
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
            worldPosition.x -= origin.x;
            worldPosition.y -= origin.y;

            /*
             * Convertir posici�n del mouse
             * a coordenadas hexagonales.
             */
            HexCoord mouseHex =
                PixelToHex(
                    worldPosition,
                    hexRadius
                );


            /*
             * Buscar el HexTile correspondiente.
             */
                        for (int i = 0; i < levelMap.tileCount; i++) {
                            HexTile* tile = &levelMap.tiles[i];
            
                            if (tile->coord.q == mouseHex.q && tile->coord.r == mouseHex.r) {
                                if (tile == selectedTile && !playerArmy.isMoving) {
                                    if (gm.hasRolledThisTurn) {
                                        HexTile* startTile = NULL;
                                        for (int k = 0; k < levelMap.tileCount; k++) {
                                            if (levelMap.tiles[k].coord.q == playerArmy.position.q && levelMap.tiles[k].coord.r == playerArmy.position.r) {
                                                startTile = &levelMap.tiles[k]; break;
                                            }
                                        }
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
                                                selectedTile = NULL;
                                            }
                                        }
                                    }
                                } else {
                                    selectedTile = tile;
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
            RAYWHITE
        );


        BeginMode2D(cc.camera);


        Vector2 camCenterWorld = cc.camera.target;
        camCenterWorld.x -= origin.x;
        camCenterWorld.y -= origin.y;
        HexCoord viewCenter = PixelToHex(camCenterWorld, hexRadius);
        int dynamicRenderRadius = (int)(((GetScreenWidth() / cc.camera.zoom) / (hexRadius * 1.5f))) + 6;
        if (dynamicRenderRadius > 40) dynamicRenderRadius = 40;

        MapView_Draw(&levelMap, hexRadius, origin, false, viewCenter, dynamicRenderRadius, visualArmyPos, &gameAssets, debugRevealMap);

        Vector2 mousePosition = GetMousePosition();
        Vector2 worldPosition = GetScreenToWorld2D(mousePosition, cc.camera);
        worldPosition.x -= origin.x;
        worldPosition.y -= origin.y;
        HexCoord hoverHex = PixelToHex(worldPosition, hexRadius);

        // Dibujar Enemigos
        for (int i = 0; i < gm.enemyCount; i++) {
            HexTile* t = Map_GetTile(&levelMap, gm.enemies[i].position);
            if (t && (t->visible || debugRevealMap)) {
                bool isHovered = (hoverHex.q == gm.enemies[i].position.q && hoverHex.r == gm.enemies[i].position.r);
                EnemyView_DrawOnMap(&gm.enemies[i], visualEnemyPos[i], origin, &gameAssets, isHovered, hexRadius);
            }
        }

        /*
         * Resaltar hover y tile seleccionado.
         */

        for (int i = 0; i < levelMap.tileCount; i++) {
            HexTile* t = &levelMap.tiles[i];
            if (t->coord.q == hoverHex.q && t->coord.r == hoverHex.r) {
                if (gm.hasRolledThisTurn && HexCoord_Distance(playerArmy.position, t->coord) == 1) {
                    MapView_DrawHexFillHighlight(t, hexRadius, origin, (Color){100, 200, 255, 80});
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
                for (int k = 0; k < levelMap.tileCount; k++) {
                    if (levelMap.tiles[k].coord.q == playerArmy.movePath[i].q && levelMap.tiles[k].coord.r == playerArmy.movePath[i].r) {
                        pathTiles[numTiles++] = &levelMap.tiles[k];
                        break;
                    }
                }
            }
            if (numTiles > 0) {
                // Line from token to first remaining path node
                Vector2 p1 = visualArmyPos;
                p1.x += origin.x;
                float currentHeight = 0.0f;
                for (int k = 0; k < levelMap.tileCount; k++) {
                    Vector2 p = HexToPixel(levelMap.tiles[k].coord, hexRadius);
                    float dx = visualArmyPos.x - p.x;
                    float dy = visualArmyPos.y - p.y;
                    if (dx*dx + dy*dy < hexRadius*hexRadius) {
                        currentHeight = GetBiomeHeight(levelMap.tiles[k].biome);
                        break;
                    }
                }
                p1.y += origin.y - currentHeight;
                Vector2 p2 = HexToPixel(pathTiles[0]->coord, hexRadius);
                p2.x += origin.x;
                p2.y += origin.y - GetBiomeHeight(pathTiles[0]->biome);
                DrawLineEx(p1, p2, 4.0f, Fade(GOLD, 0.7f));
                
                if (numTiles > 1) {
                    MapView_DrawPath(pathTiles, numTiles, hexRadius, origin);
                }
            }
        } 
        else if (selectedTile != NULL && !gm.inCombat && gm.phase == PHASE_PLAYER)
        {
            if (!(selectedTile->coord.q == playerArmy.position.q && selectedTile->coord.r == playerArmy.position.r) && selectedTile->visited) {
                HexTile* startTile = NULL;
                for (int k = 0; k < levelMap.tileCount; k++) {
                    if (levelMap.tiles[k].coord.q == playerArmy.position.q && levelMap.tiles[k].coord.r == playerArmy.position.r) {
                        startTile = &levelMap.tiles[k]; break;
                    }
                }
                if (startTile) {
                    HexTile* path[128];
                    int cost = 0;
                    int pathLen = Map_FindPath(&levelMap, startTile, selectedTile, &cost, path);
                    Color pathColor = (gm.currentActionPoints >= cost) ? GREEN : RED;
                    
                    if (pathLen > 0) {
                        MapView_DrawHexHighlight(selectedTile, hexRadius, origin, pathColor);
                        
                        // Line from token to first path node
                        Vector2 p1 = visualArmyPos;
                        p1.x += origin.x;
                        float currentHeight = 0.0f;
                        for (int k = 0; k < levelMap.tileCount; k++) {
                            Vector2 p = HexToPixel(levelMap.tiles[k].coord, hexRadius);
                            float dx = visualArmyPos.x - p.x;
                            float dy = visualArmyPos.y - p.y;
                            if (dx*dx + dy*dy < hexRadius*hexRadius) {
                                currentHeight = GetBiomeHeight(levelMap.tiles[k].biome);
                                break;
                            }
                        }
                        p1.y += origin.y - currentHeight;
                        Vector2 p2 = HexToPixel(path[0]->coord, hexRadius);
                        p2.x += origin.x;
                        p2.y += origin.y - GetBiomeHeight(path[0]->biome);
                        DrawLineEx(p1, p2, 4.0f, Fade(GOLD, 0.7f));
                        
                        MapView_DrawPath(path, pathLen, hexRadius, origin);
                        
                        int midIdx = pathLen / 2;
                        Vector2 midPx = HexToPixel(path[midIdx]->coord, hexRadius);
                        midPx.x += origin.x; midPx.y += origin.y - GetBiomeHeight(path[midIdx]->biome);
                        
                        DrawCircleV(midPx, 18.0f, DARKGRAY);
                        DrawCircleLines(midPx.x, midPx.y, 18.0f, pathColor);
                        DrawText(TextFormat("%d PA", cost), midPx.x - MeasureText(TextFormat("%d PA", cost), 10)/2, midPx.y - 5, 10, WHITE);
                    }
                }
            } else {
                MapView_DrawHexHighlight(selectedTile, hexRadius, origin, GREEN);
            }
        }

        EndMode2D();
        BeginBlendMode(BLEND_MULTIPLIED);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ (unsigned char)skyR, (unsigned char)skyG, (unsigned char)skyB, 255 });
        EndBlendMode();


        /* ========================================================
           NUEVO HUD DEL EJERCITO
           ======================================================== */
           
        int SW = GetScreenWidth();
        int SH = GetScreenHeight();

        /* Panel comandante (arriba izquierda) */
        Hud_DrawCommanderPanel(&playerArmy, &gm, &hudState, 10, 10, 340);

        /* Panel stats ejercito (abajo izquierda) */
        Hud_DrawArmyStatsPanel(&playerArmy, &gm, &levelMap, 10, SH - 160, 400, 150);
        Hud_DrawTopBar(&gm, SW);

        /* Panel dados (abajo centro) */
        Hud_DrawDicePanel(&hudState, &gm, &playerArmy, &levelMap, 420, SH - 160, 160, 150);

        /* Minimapa (abajo derecha) */
        Rectangle minimapRect = { 590.0f, (float)(SH - 160), (float)(SW - 1000), 150.0f };
        DrawRectangleRec(minimapRect, (Color){ 15, 15, 20, 220 });
        DrawRectangleLinesEx(minimapRect, 2, (Color){ 140, 100, 50, 255 });
        MapView_DrawMinimap(&levelMap, minimapRect, playerArmy.position, hudState.blinkTimer, &gameAssets, debugRevealMap);

                if (gm.canInteractStructure && gm.phase == PHASE_PLAYER && !gm.hasDayReport && !gm.showCampDecision) {
            const char* promptText = "[E] Interactuar con Asentamiento";
            int tw = MeasureText(promptText, 20);
            int px = SW / 2 - tw / 2;
            int py = SH - 210;
            DrawRectangle(px - 15, py - 6, tw + 30, 32, (Color){ 20, 25, 35, 230 });
            DrawRectangleLines(px - 15, py - 6, tw + 30, 32, GOLD);
            DrawText(promptText, px, py, 20, GOLD);
        }

        /* Tooltip de Exploracion */
        if (selectedTile != NULL && !gm.inCombat && gm.phase != PHASE_EVENT && gm.phase != PHASE_CITY)
        {
            BiomeProperties props = Biome_GetProperties(selectedTile->biome);
            Vector2 mPos = GetMousePosition();
            
            int foodCost = props.actionPointCost * 2; // Ejemplo base
            
            int tw = 200;
            int th = 85;
            int px = mPos.x + 15;
            int py = mPos.y + 15;
            
            // Keep tooltip inside screen bounds
            if (px + tw > SW) px = mPos.x - tw - 15;
            if (py + th > SH) py = mPos.y - th - 15;
            
            DrawRectangle(px, py, tw, th, (Color){ 20, 20, 25, 235 });
            DrawRectangleLines(px, py, tw, th, (Color){ 100, 80, 40, 255 });

            DrawText(props.name, px+10, py+10, 16, GOLD);
            if (!selectedTile->visited && !selectedTile->visible) {
                DrawText("Desconocido", px+10, py+35, 14, GRAY);
            } else {
                DrawText(TextFormat("Costo: %d PA", props.actionPointCost), px+10, py+35, 14, RAYWHITE);
                DrawText(TextFormat("Consumo: %d Suministros", foodCost), px+10, py+55, 14, RED);
            }
        }        if (gm.pendingCombatEnemy >= 0 && !Sequence_IsBusy()) {
            gm.inCombat = true;
            gm.phase = PHASE_EVENT;
            Combat_Init(&gm.activeCombat, &gm, &playerArmy, &gm.enemies[gm.pendingCombatEnemy]);
            playerArmy.isMoving = false;
            gm.pendingCombatEnemy = -1;
        }

        if (gm.inCombat) {
            if (CombatView_Update(&gm.activeCombat, SW, SH)) {
                gm.inCombat = false;
                if (Army_GetTotalWeight(&playerArmy) <= 0) {
                    gm.phase = PHASE_GAME_OVER;
                } else {
                    if (gm.inSiegeCombat) {
                        if (gm.activeCombat.endReason == COMBAT_END_VICTORY) {
                            GameMaster_ResolveSiegeVictory(&gm, &levelMap, &playerArmy, gm.siegeTargetCoord);
                        } else {
                            gm.inSiegeCombat = false;
                        }
                    }
                    gm.phase = PHASE_PLAYER;
                    visualArmyPos = HexToPixel(playerArmy.position, hexRadius);
                    cc.camera.target = visualArmyPos;
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
            if (CityView_UpdateDraw(&gm.activeCity, &playerArmy, &gm, SW, SH)) {
                GameMaster_CloseCity(&gm, &levelMap, &playerArmy);
            }
        } else if (gm.phase == PHASE_EVENT && (gm.activeEvent || gm.activeResult)) {
            if (gm.activeResult) {
                ResultView_Draw(gm.activeResult, SW, SH);
                int r_opt = (gm.eventTimer > 1.0f) ? ResultView_HandleInput(gm.activeResult, SW, SH) : -1;
                if (r_opt == 0) {
                    Event_ApplyResult(gm.activeResult, &playerArmy, &gm.infamy);
                    for (int ei = 0; ei < gm.activeResult->effectCount; ei++) {
                        EffectDef* eff = &gm.activeResult->effects[ei];
                        if (eff->type == EFFECT_ADD_MOD && strlen(eff->stringValue) > 0) {
                            ModifierDef* def = Modifier_GetById(gm.modRegistry.defs, gm.modRegistry.count, eff->stringValue);
                            if (def) Army_EquipModifier(&playerArmy, def);
                        } else if (eff->type == EFFECT_REMOVE_MOD && strlen(eff->stringValue) > 0) {
                            Army_RemoveModifier(&playerArmy, eff->stringValue);
                        }
                    }
                    if (gm.activeResult->startsBattle) {
                        gm.inCombat = true;
                        static EnemySquad dummyEnemy;
                        dummyEnemy = Enemy_Create(gm.activeResult->battleEnemy, (HexCoord){0,0}, 35, 12, 10, 5, 2);
                        Enemy_AddTag(&dummyEnemy, TAG_CAVALRY);
                        if (strcmp(gm.activeResult->id, "res_camp_luchar") == 0) {
                            gm.activeCombat.ambushed = true;
                        } else {
                            gm.activeCombat.ambushed = false;
                        }
                        Combat_Init(&gm.activeCombat, &gm, &playerArmy, &dummyEnemy);
                    } else {
                        gm.phase = PHASE_PLAYER;
                    }
                    gm.activeResult = NULL;
                }
            } else if (gm.activeEvent) {
                Event_EvaluateOptions(gm.activeEvent, &playerArmy);
                EventView_Draw(gm.activeEvent, SW, SH);
                
                int opt = (gm.eventTimer > 1.0f) ? EventView_HandleInput(gm.activeEvent, SW, SH) : -1;
                if (opt >= 0 && opt < gm.activeEvent->optionCount) {
                    EventOption* chosenOpt = &gm.activeEvent->options[opt];
                    
                    if (chosenOpt->resultPoolCount > 0) {
                        int randIdx = GetRandomValue(0, chosenOpt->resultPoolCount - 1);
                        const char* resId = chosenOpt->resultPool[randIdx];
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
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.8f));
            DrawText("DERROTA TOTAL", screenWidth/2 - MeasureText("DERROTA TOTAL", 40)/2, screenHeight/2 - 40, 40, RED);
            DrawText("Tu ejercito ha sido aniquilado.", screenWidth/2 - MeasureText("Tu ejercito ha sido aniquilado.", 20)/2, screenHeight/2 + 20, 20, LIGHTGRAY);
        } else if (gm.phase == PHASE_VICTORY) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.8f));
            DrawText("VICTORIA", screenWidth/2 - MeasureText("VICTORIA", 40)/2, screenHeight/2 - 40, 40, GOLD);
            DrawText("Has conquistado el ultimo objetivo.", screenWidth/2 - MeasureText("Has conquistado el ultimo objetivo.", 20)/2, screenHeight/2 + 20, 20, LIGHTGRAY);
        }

        if (gm.hasDayReport) {
            Hud_DrawDayReportModal(&gm.lastDayReport, SW, SH);
            Hud_HandleDayReportInput(&gm, SW, SH);
        }

        if (gm.showCampDecision) {
            Hud_DrawCampDecisionModal(&gm, &playerArmy, &levelMap, SW, SH);
            Hud_HandleCampDecisionInput(&gm, &levelMap, &playerArmy, SW, SH);
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
    Audio_Shutdown();
    UI_Shutdown();
    CloseWindow();

    return 0;
}





































