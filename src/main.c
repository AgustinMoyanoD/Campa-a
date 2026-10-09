#include "raylib.h"
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
#include "views/event_view.h"
#include "views/combat_view.h"
#include "views/enemy_view.h"
#include "views/draft_view.h"

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
        "Estrategia Táctica - Nivel 1 Procedural"
    );

    ToggleFullscreen();

    SetTargetFPS(60);


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
    Vector2 origin = center;
    CameraController cc;
    CameraController_Init(&cc, center);

    /* ============================================================
       MAPA
       ============================================================ */

    Map levelMap =
        Map_GenerateProcedural(
            mapRadius,
            landProbabilityPercent,
            3,
            provinceLevel
        );


    /* ============================================================
       INICIALIZAR HUD Y ARMY
       ============================================================ */
       
    HudState hudState = Hud_Init();

    General generalPrueba = General_Create(
        "Cayo Marcio", 38,
        1, 3, 3, 2, 5
    );

    /* Buscar el tile de inicio para spawnear el army */
    HexTile* startTile = NULL;
    for (int i = 0; i < levelMap.tileCount; i++) {
        if (levelMap.tiles[i].special == TILE_SPECIAL_START) {
            startTile = &levelMap.tiles[i];
            break;
        }
    }
    HexCoord startCoord = startTile ? startTile->coord : HexCoord_Create(0,0);

    Army playerArmy = Army_Create(
        startCoord,
        generalPrueba,
        8,      /* sargentos */
        40,     /* soldados */
        15,     /* iniciados */
        200.0f, /* agua */
        180.0f, /* comida */
        350,    /* monedas */
        12      /* ganado */
    );

    Lieutenant lt = Lieutenant_Create(
        "Publio Varro", NULL, 26,
        2, 1, 1, 0, 2,
        72 /* lealtad */
    );
    Army_AddLieutenant(&playerArmy, lt);

    /* REPARAR PUNTERO */
    Army_SetCommanderGeneral(&playerArmy);

    GameMaster gm = {0};
    DraftContext activeDraft = {0};
    GameMaster_Init(&gm);

    ModifierDef* def1 = Modifier_GetById(gm.modRegistry.defs, gm.modRegistry.count, "ares_marea_roja");
    if (def1) Army_EquipModifier(&playerArmy, def1);
    ModifierDef* def2 = Modifier_GetById(gm.modRegistry.defs, gm.modRegistry.count, "hermes_pie_alado");
    if (def2) Army_EquipModifier(&playerArmy, def2);
    ModifierDef* def3 = Modifier_GetById(gm.modRegistry.defs, gm.modRegistry.count, "zeus_rayo");
    if (def3) Army_EquipModifier(&playerArmy, def3);
    GameMaster_SpawnTestEnemies(&gm, &levelMap, playerArmy.position);
    GameMaster_StartTurn(&gm, &playerArmy);

    /* Inicializar visión del ejército sobre el mapa en el tile inicial */
    Map_UpdateVision(&levelMap, playerArmy.position, Army_GetVision(&playerArmy) + GameMaster_GetVisionModifier(&gm, &playerArmy));
    {
        Vector2 pos = HexToPixel(playerArmy.position, hexRadius);
        pos.x += origin.x;
        pos.y += origin.y;
        CameraController_FocusOn(&cc, pos);
    }


    bool debugRevealMap = false;


    /* ============================================================
       TILE SELECCIONADO
       ============================================================ */

        HexTile* selectedTile = NULL;
    bool isArmyMoving = false;
    Vector2 visualArmyPos = {0};
    bool firstFrame = true;
    
    float skyR = 255.0f, skyG = 255.0f, skyB = 255.0f;
    TimeOfDay lastTimeOfDay = gm.timeOfDay;


    /* ============================================================
       LOOP PRINCIPAL
       ============================================================ */

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
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
        if (firstFrame) { Vector2 p = HexToPixel(playerArmy.position, hexRadius); visualArmyPos = p; firstFrame = false; }
        /* ========================================================
           CAMBIO DE PROVINCIA Y REGENERACIÓN
           ======================================================== */

        bool regenerate = false;

        if (gm.phase != PHASE_EVENT && IsKeyPressed(KEY_ENTER)) {
            GameMaster_EndTurn(&gm, &levelMap, &playerArmy);
            Map_UpdateVision(&levelMap, playerArmy.position, Army_GetVision(&playerArmy) + GameMaster_GetVisionModifier(&gm, &playerArmy));
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
            
            /* Reposicionar ejército */
            for (int i = 0; i < levelMap.tileCount; i++) { if (levelMap.tiles[i].special == TILE_SPECIAL_START) { playerArmy.position = levelMap.tiles[i].coord; break; } } visualArmyPos = HexToPixel(playerArmy.position, hexRadius); Map_UpdateVision(&levelMap, playerArmy.position, Army_GetVision(&playerArmy) + GameMaster_GetVisionModifier(&gm, &playerArmy));
    {
        Vector2 pos = HexToPixel(playerArmy.position, hexRadius);
        pos.x += origin.x;
        pos.y += origin.y;
        CameraController_FocusOn(&cc, pos);
    }
        }


        /* ========================================================
           CÁMARA
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
        if (gm.phase != PHASE_EVENT) Hud_HandleInput(&hudState, &gm);
        
        if (IsKeyPressed(KEY_SPACE)) {
            Hud_RollDice(&hudState);
        }

        /* ========================================================
           MOUSE INPUT PARA MAPA
           ======================================================== */

        if (gm.phase != PHASE_EVENT && IsMouseButtonPressed(
                MOUSE_BUTTON_LEFT) && 
            !CheckCollisionPointRec(GetMousePosition(), hudState.cmdPanelRect) &&
            !CheckCollisionPointRec(GetMousePosition(), hudState.rollBtnRect))
        {
            /*
             * Posición del mouse en pantalla.
             */
            Vector2 mousePosition =
                GetMousePosition();

            /*
             * Convertir de coordenadas de pantalla
             * a coordenadas del mundo teniendo en cuenta
             * la cámara y el zoom.
             */
            Vector2 worldPosition =
                GetScreenToWorld2D(mousePosition, cc.camera);

            /*
             * MapView dibuja los hexágonos de esta forma:
             *
             * posición del hexágono =
             * HexToPixel(coord) + origin
             *
             * Por lo tanto, quitamos origin antes
             * de hacer PixelToHex().
             */
            worldPosition.x -= origin.x;
            worldPosition.y -= origin.y;

            /*
             * Convertir posición del mouse
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

        // Dibujar Enemigos
        for (int i = 0; i < gm.enemyCount; i++) {
            HexTile* t = Map_GetTile(&levelMap, gm.enemies[i].position);
            if (t && (t->visible || debugRevealMap)) {
                EnemyView_DrawOnMap(&gm.enemies[i], origin, hexRadius, &gameAssets);
            }
        }


                /*
         * Resaltar hover y tile seleccionado.
         */
        Vector2 mousePosition = GetMousePosition();
        Vector2 worldPosition = GetScreenToWorld2D(mousePosition, cc.camera);
        worldPosition.x -= origin.x;
        worldPosition.y -= origin.y;
        HexCoord hoverHex = PixelToHex(worldPosition, hexRadius);

        for (int i = 0; i < levelMap.tileCount; i++) {
            HexTile* t = &levelMap.tiles[i];
            if (t->coord.q == hoverHex.q && t->coord.r == hoverHex.r) {
                if (gm.hasRolledThisTurn && HexCoord_Distance(playerArmy.position, t->coord) == 1) {
                    MapView_DrawHexFillHighlight(t, hexRadius, origin, (Color){100, 200, 255, 80});
                }
                break;
            }
        }

        if (selectedTile != NULL)
        {
            if (!playerArmy.isMoving) {
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
                        // Resaltar suavemente solo la casilla destino
                        MapView_DrawHexHighlight(selectedTile, hexRadius, origin, pathColor);
                        MapView_DrawPath(path, pathLen, hexRadius, origin);
                        
                        // Centro de la ruta para el costo de PA
                        int midIdx = pathLen / 2;
                        Vector2 midPx = HexToPixel(path[midIdx]->coord, hexRadius);
                        midPx.x += origin.x; midPx.y += origin.y;
                        
                        DrawCircleV(midPx, 18.0f, DARKGRAY);
                        DrawCircleLines(midPx.x, midPx.y, 18.0f, pathColor);
                        const char* text = TextFormat("%d PA", cost);
                        int textW = MeasureText(text, 10);
                        DrawText(text, midPx.x - textW/2, midPx.y - 5, 10, WHITE);
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
        Hud_DrawArmyStatsPanel(&playerArmy, &gm, 10, SH - 160, 400, 150);
        Hud_DrawTopBar(&gm, SW);

        /* Panel dados (abajo centro) */
        Hud_DrawDicePanel(&hudState, &gm, 420, SH - 160, 160, 150);

        /* Minimapa (abajo derecha) */
        Rectangle minimapRect = { 590.0f, (float)(SH - 160), (float)(SW - 1000), 150.0f };
        DrawRectangleRec(minimapRect, (Color){ 15, 15, 20, 220 });
        DrawRectangleLinesEx(minimapRect, 2, (Color){ 140, 100, 50, 255 });
        MapView_DrawMinimap(&levelMap, minimapRect, playerArmy.position, hudState.blinkTimer, &gameAssets, debugRevealMap);

                /* Tooltip de Exploracion */
        if (selectedTile != NULL && !gm.inCombat && gm.phase != PHASE_EVENT)
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
        }        if (gm.inCombat) {
            if (CombatView_Update(&gm.activeCombat, SW, SH)) {
                gm.inCombat = false;
                if (Army_GetTotalWeight(&playerArmy) <= 0) {
                    gm.phase = PHASE_GAME_OVER;
                } else {
                    gm.phase = PHASE_PLAYER;
                    visualArmyPos = HexToPixel(playerArmy.position, hexRadius);
                }
            }
        } else if (gm.phase == PHASE_EVENT) { gm.eventTimer += GetFrameTime(); }
        if (gm.inCombat) {
            CombatView_Draw(&gm.activeCombat, SW, SH);
        } else if (gm.phase == PHASE_DRAFT) {
            if (activeDraft.choices[0] == NULL) {
                DraftView_Init(&activeDraft, &gm.modRegistry);
            }
            if (DraftView_UpdateDraw(&activeDraft, &playerArmy, SW, SH) == 1) {
                gm.phase = PHASE_PLAYER;
                activeDraft.choices[0] = NULL;
            }
        } else if (gm.phase == PHASE_EVENT && (gm.activeEvent || gm.activeResult)) {
            if (gm.activeResult) {
                ResultView_Draw(gm.activeResult, SW, SH);
                int r_opt = (gm.eventTimer > 1.0f) ? ResultView_HandleInput(gm.activeResult, SW, SH) : -1;
                if (r_opt == 0) {
                    Event_ApplyResult(gm.activeResult, &playerArmy, &gm.infamy);
                    if (gm.activeResult->startsBattle) {
                        gm.inCombat = true;
                        static EnemySquad dummyEnemy;
                        dummyEnemy = Enemy_Create(gm.activeResult->battleEnemy, (HexCoord){0,0}, 35, 12, 10, 5, 2);
                        Enemy_AddTag(&dummyEnemy, TAG_CAVALRY);
                        Combat_Init(&gm.activeCombat, &gm, &playerArmy, &dummyEnemy);
                    } else {
                        gm.phase = PHASE_PLAYER;
                    }
                    gm.activeResult = NULL;
                }
            } else if (gm.activeEvent) {
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

                if (gm.phase == PHASE_GAME_OVER) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.8f));
            DrawText("DERROTA TOTAL", screenWidth/2 - MeasureText("DERROTA TOTAL", 40)/2, screenHeight/2 - 40, 40, RED);
            DrawText("Tu ejercito ha sido aniquilado.", screenWidth/2 - MeasureText("Tu ejercito ha sido aniquilado.", 20)/2, screenHeight/2 + 20, 20, LIGHTGRAY);
        } else if (gm.phase == PHASE_VICTORY) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.8f));
            DrawText("VICTORIA", screenWidth/2 - MeasureText("VICTORIA", 40)/2, screenHeight/2 - 40, 40, GOLD);
            DrawText("Has conquistado el ultimo objetivo.", screenWidth/2 - MeasureText("Has conquistado el ultimo objetivo.", 20)/2, screenHeight/2 + 20, 20, LIGHTGRAY);
        }
        ScreenFX_DrawOverlays();
        EndDrawing();
    }


    /* ============================================================
       LIBERAR
       ============================================================ */

    Map_Free(
        &levelMap
    );

    Assets_Unload(&gameAssets);
    Audio_Shutdown();
    CloseWindow();

    return 0;
}





































