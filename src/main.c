#include "raylib.h"

#include "map_generator.h"
#include "map_view.h"
#include "hud.h"
#include "army.h"
#include "general.h"
#include "asset_manager.h"
#include "camera_controller.h"
#include "controllers/game_master.h"
#include "views/event_view.h"

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
        4, 3, 3, 2, 5
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
        "Publio Varro", 26,
        2, 1, 1, 0, 2,
        72 /* lealtad */
    );
    Army_AddLieutenant(&playerArmy, lt);

    /* REPARAR PUNTERO */
    Army_SetCommanderGeneral(&playerArmy);

    GameMaster gm;
    GameMaster_Init(&gm);
    GameMaster_StartTurn(&gm, &playerArmy);

    /* Inicializar visión del ejército sobre el mapa en el tile inicial */
    Map_UpdateVision(&levelMap, playerArmy.position, Army_GetVision(&playerArmy) + GameMaster_GetVisionModifier(&gm));
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


    /* ============================================================
       LOOP PRINCIPAL
       ============================================================ */

    while (!WindowShouldClose())
    {
        /* ========================================================
           CAMBIO DE PROVINCIA Y REGENERACIÓN
           ======================================================== */

        bool regenerate = false;

        if (gm.phase != PHASE_EVENT && IsKeyPressed(KEY_ENTER)) {
            GameMaster_EndTurn(&gm, &levelMap, &playerArmy);
            Map_UpdateVision(&levelMap, playerArmy.position, Army_GetVision(&playerArmy) + GameMaster_GetVisionModifier(&gm));
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
            for (int i = 0; i < levelMap.tileCount; i++) { if (levelMap.tiles[i].special == TILE_SPECIAL_START) { playerArmy.position = levelMap.tiles[i].coord; break; } } Map_UpdateVision(&levelMap, playerArmy.position, Army_GetVision(&playerArmy) + GameMaster_GetVisionModifier(&gm));
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
        CameraController_Update(&cc, GetFrameTime());


        /* ========================================================
           ACTUALIZAR HUD Y DADOS
           ======================================================== */
           
        bool wasRolling = hudState.dice.rolling;
        Hud_Update(&hudState, GetFrameTime());
        if (wasRolling && !hudState.dice.rolling) {
            GameMaster_ApplyDiceRoll(&gm, hudState.dice.valor1 + hudState.dice.valor2);
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
                    if (tile == selectedTile) {
                        if (gm.hasRolledThisTurn && GameMaster_AttemptMove(&gm, &levelMap, &playerArmy, tile)) {
                            selectedTile = NULL;
                            Map_UpdateVision(&levelMap, playerArmy.position, Army_GetVision(&playerArmy) + GameMaster_GetVisionModifier(&gm));
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

        MapView_Draw(&levelMap, hexRadius, origin, false, viewCenter, dynamicRenderRadius, playerArmy.position, &gameAssets, debugRevealMap);


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
            MapView_DrawHexHighlight(
                selectedTile,
                hexRadius,
                origin,
                GREEN
            );
        }


        EndMode2D();


        /* ========================================================
           NUEVO HUD DEL EJERCITO
           ======================================================== */
           
        int SW = GetScreenWidth();
        int SH = GetScreenHeight();

        /* Panel comandante (arriba izquierda) */
        Hud_DrawCommanderPanel(&playerArmy, &hudState, 10, 10, 340);

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

        /* Panel Tile Seleccionado (Debug opcional, movido a la derecha superior) */
        if (selectedTile != NULL)
        {
            BiomeProperties props = Biome_GetProperties(selectedTile->biome);
            int px = SW - 310;
            int py = 10;
            
            DrawRectangle(px, py, 300, 130, (Color){ 245, 245, 248, 220 });
            DrawRectangleLines(px, py, 300, 130, (Color){ 160, 160, 175, 255 });

            DrawText(TextFormat("Tile: q=%d, r=%d", selectedTile->coord.q, selectedTile->coord.r), px+10, py+10, 14, BLACK);
            DrawText(TextFormat("Bioma: %s", props.name), px+10, py+30, 12, DARKBLUE);
            DrawText(TextFormat("PA: %d", props.actionPointCost), px+10, py+50, 11, DARKGRAY);
        }
        if (gm.phase == PHASE_EVENT) { gm.eventTimer += GetFrameTime(); }
        if (gm.phase == PHASE_EVENT && (gm.activeEvent || gm.activeResult)) {
            if (gm.activeResult) {
                ResultView_Draw(gm.activeResult, SW, SH);
                int r_opt = (gm.eventTimer > 1.0f) ? ResultView_HandleInput(gm.activeResult, SW, SH) : -1;
                if (r_opt == 0) {
                    ResultView_Apply(gm.activeResult, &gm, &playerArmy);
                    gm.phase = PHASE_PLAYER;
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
                        EventView_ApplyResult(chosenOpt, &gm, &playerArmy);
                        gm.phase = PHASE_PLAYER;
                        gm.activeEvent = NULL;
                    }
                }
            }
        }

        EndDrawing();
    }


    /* ============================================================
       LIBERAR
       ============================================================ */

    Map_Free(
        &levelMap
    );

    Assets_Unload(&gameAssets);
    CloseWindow();

    return 0;
}























