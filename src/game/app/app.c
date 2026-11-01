#include "game/app/app.h"
#include "engine/ui/ui_widgets.h"
#include "engine/presentation/screen_fx.h"
#include "engine/presentation/sequence.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================
   INICIALIZACIÓN Y APAGADO
   ============================================================================ */

bool App_Init(AppContext* app, int width, int height, const char* title)
{
    if (!app) return false;
    memset(app, 0, sizeof(AppContext));

    ChangeDirectory(GetApplicationDirectory());

    /* 1. Subsistemas del Motor */
    app->engine.screenWidth = width;
    app->engine.screenHeight = height;

    InitWindow(width, height, title);
    ToggleFullscreen();
    SetTargetFPS(60);
    srand((unsigned int)time(NULL));

    Audio_Init();
    UI_Init();

    Vector2 center = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    CameraController_Init(&app->engine.cameraController, center);

    /* 2. Módulos de Juego */
    Assets_Init(&app->game.gameAssets);
    app->game.provinceLevel = 1;

    Menu_Init(&app->game.mainMenu);
    MenuView_Init();

    ArmySetup_InitDefault(&app->game.armySetup);
    MapView_Init(&app->game.mapView);
    Hud_InitState(&app->game.hudState);
    EventFlow_Init(&app->game.eventFlowState);

    app->game.gm.phase = PHASE_MENU;
    app->game.lastPhase = PHASE_MENU;
    app->game.pendingTargetTile = NULL;
    app->game.showRiverConfirmDialog = false;
    app->game.telemetryWritten = false;
    app->shouldClose = false;

    Audio_PlayPhaseMusic(app->game.gm.phase, 1.0f);
    Audio_PlayPhaseSFX(app->game.gm.phase);

    return true;
}

bool App_ShouldClose(const AppContext* app)
{
    if (!app) return true;
    return app->shouldClose || WindowShouldClose();
}

void App_Shutdown(AppContext* app)
{
    if (!app) return;

    Map_Free(&app->game.levelMap);
    Assets_Unload(&app->game.gameAssets);
    CityView_UnloadAssets();
    Audio_Shutdown();
    UI_Shutdown();
    CloseWindow();
}

static void App_StartCampaignSession(AppContext* app)
{
    GameMaster_StartCampaign(&app->game.gm, &app->game.levelMap, &app->game.playerArmy,
                             &app->game.armySetup, app->game.provinceLevel);

    Hud_InitState(&app->game.hudState);
    MapView_SyncToArmy(&app->game.mapView, app->game.playerArmy.position, &app->engine.cameraController);
    Audio_SetAmbientTimeOfDay(app->game.gm.timeOfDay);
    app->game.telemetryWritten = false;
}

/* ============================================================================
   LÓGICA DE ACTUALIZACIÓN (UPDATE)
   ============================================================================ */

static void App_UpdateAudioTransitions(AppContext* app, float dt)
{
    Audio_Update(dt);
    if (app->game.gm.phase != app->game.lastPhase) {
        Audio_PlayPhaseMusic(app->game.gm.phase, 2.0f);
        Audio_PlayPhaseSFX(app->game.gm.phase);
        ScreenFX_FadeFromBlack(0.3f);
        app->game.lastPhase = app->game.gm.phase;
    }
}

static void App_UpdateMenuPhase(AppContext* app)
{
    if (app->game.mainMenu.startGame) {
        app->game.mainMenu.startGame = false;
        ArmySetup_InitDefault(&app->game.armySetup);
        app->game.gm.phase = PHASE_SETUP;
    }
    if (app->game.mainMenu.quitGame) {
        app->shouldClose = true;
    }
}

static void App_UpdateSetupPhase(AppContext* app)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    SetupViewAction act = SetupView_Draw(&app->game.armySetup, sw, sh);
    if (act == SETUP_ACTION_BACK) {
        app->game.gm.phase = PHASE_MENU;
        Menu_Init(&app->game.mainMenu);
    } else if (act == SETUP_ACTION_START) {
        App_StartCampaignSession(app);
    }
}

static void App_HandlePlayerShortcuts(AppContext* app)
{
    GameContext* ctx = &app->game;
    bool modalBlocking = (ctx->gm.phase == PHASE_EVENT || ctx->gm.phase == PHASE_CITY ||
                          ctx->gm.phase == PHASE_DRAFT ||
                          ctx->gm.phase == PHASE_GAME_OVER || ctx->gm.phase == PHASE_VICTORY ||
                          ctx->gm.hasDayReport || ctx->gm.showCampDecision ||
                          ctx->hudState.showOfficersPanel || ctx->hudState.showGroupsPanel ||
                          GameMaster_RationPending(&ctx->gm) || ctx->rationView.showingReport ||
                          ctx->showRiverConfirmDialog);

    if (modalBlocking) return;

    /* Finalizar Turno con [Enter] */
    if (!ctx->hudState.dice.rolling && !ctx->playerArmy.isMoving && IsKeyPressed(KEY_ENTER)) {
        Audio_PlaySfxByName("turn_end");
        GameMaster_EndTurn(&ctx->gm, &ctx->levelMap, &ctx->playerArmy);
        Map_UpdateVision(&ctx->levelMap, ctx->playerArmy.position,
                         Army_GetVision(&ctx->playerArmy) + GameMaster_GetVisionModifier(&ctx->gm, &ctx->playerArmy));
    }

    /* Interactuar con Estructura con [E] */
    if (ctx->gm.canInteractStructure && ctx->gm.phase == PHASE_PLAYER && !ctx->playerArmy.isMoving && IsKeyPressed(KEY_E)) {
        GameMaster_InteractStructure(&ctx->gm, &ctx->levelMap, &ctx->playerArmy);
    }

    /* Ataque voluntario a NPC con [X] */
    if (ctx->gm.phase == PHASE_PLAYER) {
        int attackNpcIdx = NPC_FindAttackableSquadNear(&ctx->gm, ctx->playerArmy.position);
        if (attackNpcIdx >= 0 && IsKeyPressed(KEY_X)) {
            NPC_PlayerVoluntaryAttack(&ctx->gm, &ctx->playerArmy, attackNpcIdx);
            ctx->playerArmy.isMoving = false;
        }
    }

    /* Tirar dados con [Espacio] */
    if (ctx->gm.phase == PHASE_PLAYER && !ctx->gm.hasRolledThisTurn && IsKeyPressed(KEY_SPACE)) {
        Hud_RollDice(&ctx->hudState);
    }

    /* Modo de depuración F1 */
    if (IsKeyPressed(KEY_F1)) ctx->mapView.debugRevealMap = !ctx->mapView.debugRevealMap;

    /* Teclas de depuración: protegidas para operar sólo si debugRevealMap está activo (Hallazgo 4) */
    if (ctx->mapView.debugRevealMap) {
        bool regen = false;
        if (IsKeyPressed(KEY_R)) regen = true;
        if (IsKeyPressed(KEY_ONE) && ctx->provinceLevel != 1) { ctx->provinceLevel = 1; regen = true; }
        else if (IsKeyPressed(KEY_TWO) && ctx->provinceLevel != 2) { ctx->provinceLevel = 2; regen = true; }
        else if (IsKeyPressed(KEY_THREE) && ctx->provinceLevel != 3) { ctx->provinceLevel = 3; regen = true; }
        else if (IsKeyPressed(KEY_FOUR) && ctx->provinceLevel != 4) { ctx->provinceLevel = 4; regen = true; }

        if (regen) {
            GameMaster_TransitionProvince(&ctx->gm, &ctx->levelMap, &ctx->playerArmy, ctx->provinceLevel);
            MapView_SyncToArmy(&ctx->mapView, ctx->playerArmy.position, &app->engine.cameraController);
        }
    }
}

static void App_HandleMapMouseClick(AppContext* app)
{
    GameContext* ctx = &app->game;
    bool modalBlocking = (ctx->gm.phase == PHASE_EVENT || ctx->gm.phase == PHASE_CITY ||
                          ctx->gm.phase == PHASE_DRAFT ||
                          ctx->gm.phase == PHASE_GAME_OVER || ctx->gm.phase == PHASE_VICTORY ||
                          ctx->gm.hasDayReport || ctx->gm.showCampDecision ||
                          ctx->hudState.showOfficersPanel || ctx->hudState.showGroupsPanel ||
                          GameMaster_RationPending(&ctx->gm) || ctx->rationView.showingReport ||
                          ctx->showRiverConfirmDialog);

    if (modalBlocking) return;

    Hud_HandleInput(&ctx->hudState, &ctx->gm, &ctx->playerArmy, &ctx->levelMap);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !CheckCollisionPointRec(GetMousePosition(), ctx->hudState.cmdPanelRect) &&
        !CheckCollisionPointRec(GetMousePosition(), ctx->hudState.rollBtnRect) &&
        !CheckCollisionPointRec(GetMousePosition(), ctx->hudState.forageBtnRect) &&
        !CheckCollisionPointRec(GetMousePosition(), ctx->hudState.endTurnBtnRect))
    {
        Vector2 mousePos = GetMousePosition();
        Vector2 worldPos = GetScreenToWorld2D(mousePos, app->engine.cameraController.camera);
        worldPos.x -= ctx->mapView.origin.x;
        worldPos.y -= ctx->mapView.origin.y;

        HexCoord mouseHex = PixelToHex(worldPos, ctx->mapView.hexRadius);
        HexTile* tile = Map_GetTile(&ctx->levelMap, mouseHex);

        if (tile) {
            if (tile == ctx->mapView.selectedTile && !ctx->playerArmy.isMoving) {
                if (ctx->gm.hasRolledThisTurn) {
                    HexTile* startTile = Map_GetTile(&ctx->levelMap, ctx->playerArmy.position);
                    if (startTile) {
                        HexTile* path[128];
                        int cost = 0;
                        int pathLen = Map_FindPath(&ctx->levelMap, startTile, tile, &cost, path);
                        if (pathLen > 0 && ctx->gm.currentActionPoints >= cost) {
                            /* Detección estricta de transición tierra -> río sin puente (Hallazgo 1) */
                            bool hasUnbridgedRiver = false;
                            HexTile* prevStep = startTile;
                            for (int p = 0; p < pathLen; p++) {
                                if (prevStep->river != RIVER_PRESENT &&
                                    path[p]->river == RIVER_PRESENT &&
                                    path[p]->bridge == BRIDGE_NONE)
                                {
                                    hasUnbridgedRiver = true;
                                    break;
                                }
                                prevStep = path[p];
                            }
                            if (hasUnbridgedRiver) {
                                ctx->showRiverConfirmDialog = true;
                                ctx->pendingTargetTile = tile;
                            } else {
                                for (int p = 0; p < pathLen; p++) ctx->playerArmy.movePath[p] = path[p]->coord;
                                ctx->playerArmy.pathLength = pathLen;
                                ctx->playerArmy.currentPathIndex = 0;
                                ctx->playerArmy.moveTimer = 0.0f;
                                ctx->playerArmy.isMoving = true;
                                ctx->mapView.selectedTile = NULL;
                            }
                        }
                    }
                }
            } else {
                ctx->mapView.selectedTile = tile;
            }
        }
    }
}

static void App_UpdateCombatPhase(AppContext* app, int sw, int sh)
{
    GameContext* ctx = &app->game;
    if (ctx->gm.pendingCombatEnemy >= 0 && !Sequence_IsBusy()) {
        Audio_PlaySfxByName("enemy_alert");
        int targetEnemyIdx = ctx->gm.pendingCombatEnemy;
        NPC_InitiateCombat(&ctx->gm, &ctx->playerArmy, &ctx->gm.enemies[targetEnemyIdx],
                           ctx->gm.enemies[targetEnemyIdx].initiator, ctx->gm.activeCombat.ambushed);
        ctx->playerArmy.isMoving = false;
        ctx->gm.pendingCombatEnemy = -1;
    }

    if (ctx->gm.inCombat) {
        if (CombatView_Update(&ctx->gm.activeCombat, sw, sh)) {
            ctx->gm.inCombat = false;
            if (Army_GetTotalWeight(&ctx->playerArmy) <= 0) {
                ctx->gm.phase = PHASE_GAME_OVER;
                Audio_PlaySfxByName("game_over");
            } else {
                if (ctx->gm.inSiegeCombat) {
                    if (ctx->gm.activeCombat.endReason == COMBAT_END_VICTORY) {
                        Audio_PlaySfxByName("siege_victory");
                        GameMaster_ResolveSiegeVictory(&ctx->gm, &ctx->levelMap, &ctx->playerArmy, ctx->gm.siegeTargetCoord);
                    } else {
                        ctx->gm.inSiegeCombat = false;
                    }
                }
                ctx->gm.phase = PHASE_PLAYER;
                Map_UpdateVision(&ctx->levelMap, ctx->playerArmy.position,
                                 Army_GetVision(&ctx->playerArmy) + GameMaster_GetVisionModifier(&ctx->gm, &ctx->playerArmy));
                Movement_HandleCombatRetreat(&ctx->gm, &ctx->levelMap, &ctx->playerArmy, &ctx->mapView, &app->engine.cameraController);
                Audio_SetMusicContext(MUSIC_CTX_MAIN);
            }
        }
    }
}

static void App_UpdateInteractiveModals(AppContext* app, int sw, int sh)
{
    GameContext* ctx = &app->game;

    /* Eventos */
    if (ctx->gm.phase == PHASE_EVENT) {
        EventFlow_Update(&ctx->eventFlowState, &ctx->gm, &ctx->playerArmy, &ctx->levelMap, GetFrameTime());
    }

    /* Raciones */
    if (GameMaster_RationPending(&ctx->gm) || ctx->rationView.showingReport) {
        if (!ctx->rationView.initialized && GameMaster_RationPending(&ctx->gm)) {
            RationView_Init(&ctx->rationView, &ctx->playerArmy, &ctx->levelMap, ctx->playerArmy.position);
            ctx->rationView.stragglersRegrouped = GameMaster_HasFlag(&ctx->gm, "reagrupamiento_reciente");
        }
        RationViewAction rAct = RationView_Update(&ctx->rationView, &ctx->playerArmy, &ctx->levelMap, ctx->playerArmy.position, sw, sh);
        if (rAct.type == RATION_ACTION_SUBMIT_PLAN) {
            GameMaster_SubmitRation(&ctx->gm, &ctx->playerArmy, &rAct.plan);
            RationView_ShowReport(&ctx->rationView, &ctx->gm.lastDayReport);
        } else if (rAct.type == RATION_ACTION_SLAUGHTER_CATTLE) {
            GameMaster_SlaughterCattle(&ctx->gm, &ctx->playerArmy, rAct.cattleHeads);
        } else if (rAct.type == RATION_ACTION_CLOSE_REPORT) {
            ctx->rationView.initialized = false;
            ctx->rationView.showingReport = false;
            ctx->gm.hasDayReport = false;
            GameMaster_ClearFlag(&ctx->gm, "reagrupamiento_reciente");
            GameMaster_CheckPostDayReportEvents(&ctx->gm, &ctx->playerArmy);
        }
    } else if (ctx->gm.hasDayReport) {
        if (Hud_HandleDayReportInput(&ctx->gm, sw, sh)) {
            GameMaster_CheckPostDayReportEvents(&ctx->gm, &ctx->playerArmy);
        }
    }

    /* Campamento, Oficiales y Grupos */
    if (ctx->gm.showCampDecision) {
        Hud_HandleCampDecisionInput(&ctx->gm, &ctx->levelMap, &ctx->playerArmy, sw, sh);
    }
    if (ctx->hudState.showOfficersPanel) {
        Hud_HandleOfficersPanelInput(&ctx->hudState, sw, sh);
    }
    if (ctx->hudState.showGroupsPanel) {
        Hud_HandleGroupsPanelInput(&ctx->hudState, sw, sh);
    }

    /* Fin de partida */
    if (ctx->gm.phase == PHASE_GAME_OVER || ctx->gm.phase == PHASE_VICTORY) {
        if (!ctx->telemetryWritten) {
            GameMaster_LogTelemetry(&ctx->gm, &ctx->playerArmy, "telemetry_run.txt");
            ctx->telemetryWritten = true;
        }
    }
}

void App_Update(AppContext* app, float dt)
{
    if (!app) return;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    App_UpdateAudioTransitions(app, dt);

    if (app->game.gm.phase == PHASE_MENU) {
        App_UpdateMenuPhase(app);
        return;
    }
    if (app->game.gm.phase == PHASE_SETUP) {
        App_UpdateSetupPhase(app);
        return;
    }

    /* Movimiento, cámara y HUD */
    MapView_UpdateWorldVisuals(&app->game.mapView, &app->game.gm, &app->game.playerArmy, &app->engine.cameraController, dt);
    CameraController_Update(&app->engine.cameraController, dt);

    bool wasRolling = app->game.hudState.dice.rolling;
    Hud_Update(&app->game.hudState, dt);
    if (wasRolling && !app->game.hudState.dice.rolling) {
        GameMaster_ApplyDiceRoll(&app->game.gm, &app->game.playerArmy,
                                 app->game.hudState.dice.valor1 + app->game.hudState.dice.valor2);
    }
    Assets_UpdateAnimations(&app->game.gameAssets, dt);

    Movement_UpdateStep(&app->game.gm, &app->game.levelMap, &app->game.playerArmy, &app->game.mapView);
    App_HandlePlayerShortcuts(app);
    App_HandleMapMouseClick(app);
    App_UpdateCombatPhase(app, sw, sh);
    App_UpdateInteractiveModals(app, sw, sh);
}

/* ============================================================================
   RENDERIZADO (DRAW) - CERO MUTACIONES DE ESTADO (Meta 3)
   ============================================================================ */

static void App_DrawWorld(AppContext* app)
{
    GameContext* ctx = &app->game;
    Camera2D renderCam = app->engine.cameraController.camera;
    ScreenFX_ApplyShakeToCamera(&renderCam);
    BeginMode2D(renderCam);

    Vector2 camCenterWorld = app->engine.cameraController.camera.target;
    camCenterWorld.x -= ctx->mapView.origin.x;
    camCenterWorld.y -= ctx->mapView.origin.y;
    HexCoord viewCenter = PixelToHex(camCenterWorld, ctx->mapView.hexRadius);
    int dynamicRenderRadius = (int)(((GetScreenWidth() / app->engine.cameraController.camera.zoom) / (ctx->mapView.hexRadius * 1.5f))) + 6;
    if (dynamicRenderRadius > 40) dynamicRenderRadius = 40;

    MapView_Draw(&ctx->levelMap, ctx->mapView.hexRadius, ctx->mapView.origin, false, viewCenter, dynamicRenderRadius,
                 ctx->mapView.visualArmyPos, &ctx->gameAssets, ctx->mapView.debugRevealMap);

    Vector2 mousePos = GetMousePosition();
    Vector2 worldPos = GetScreenToWorld2D(mousePos, app->engine.cameraController.camera);
    worldPos.x -= ctx->mapView.origin.x;
    worldPos.y -= ctx->mapView.origin.y;
    HexCoord hoverHex = PixelToHex(worldPos, ctx->mapView.hexRadius);

    for (int i = 0; i < ctx->gm.enemyCount; i++) {
        HexTile* t = Map_GetTile(&ctx->levelMap, ctx->gm.enemies[i].position);
        if (t && (t->visible || ctx->mapView.debugRevealMap)) {
            bool isHovered = (hoverHex.q == ctx->gm.enemies[i].position.q && hoverHex.r == ctx->gm.enemies[i].position.r);
            NPCView_DrawOnMap(&ctx->gm.enemies[i], &ctx->gm, ctx->mapView.visualEnemyPos[i], ctx->mapView.origin,
                              &ctx->gameAssets, isHovered, ctx->mapView.hexRadius);
        }
    }

    HexTile* hoverTile = Map_GetTile(&ctx->levelMap, hoverHex);
    if (hoverTile && ctx->gm.hasRolledThisTurn && HexCoord_Distance(ctx->playerArmy.position, hoverTile->coord) == 1) {
        MapView_DrawHexFillHighlight(hoverTile, ctx->mapView.hexRadius, ctx->mapView.origin, (Color){100, 200, 255, 80});
    }

    /* Rutas del ejército con cálculo O(1) de altura (Hallazgo 3) */
    if (ctx->playerArmy.isMoving) {
        HexTile* pathTiles[128];
        int numTiles = 0;
        for (int i = ctx->playerArmy.currentPathIndex; i < ctx->playerArmy.pathLength; i++) {
            HexTile* pt = Map_GetTile(&ctx->levelMap, ctx->playerArmy.movePath[i]);
            if (pt) pathTiles[numTiles++] = pt;
        }
        if (numTiles > 0) {
            Vector2 p1 = ctx->mapView.visualArmyPos;
            p1.x += ctx->mapView.origin.x;
            HexCoord armyCoord = PixelToHex(ctx->mapView.visualArmyPos, ctx->mapView.hexRadius);
            HexTile* armyTile = Map_GetTile(&ctx->levelMap, armyCoord);
            float curH = armyTile ? GetBiomeHeight(armyTile->biome) : 0.0f;
            p1.y += ctx->mapView.origin.y - curH;

            Vector2 p2 = HexToPixel(pathTiles[0]->coord, ctx->mapView.hexRadius);
            p2.x += ctx->mapView.origin.x;
            p2.y += ctx->mapView.origin.y - GetBiomeHeight(pathTiles[0]->biome);
            DrawLineEx(p1, p2, 4.0f, Fade(GOLD, 0.7f));

            if (numTiles > 1) MapView_DrawPath(pathTiles, numTiles, ctx->mapView.hexRadius, ctx->mapView.origin);
        }
    } else if (ctx->mapView.selectedTile != NULL && !ctx->gm.inCombat && ctx->gm.phase == PHASE_PLAYER) {
        if (!(ctx->mapView.selectedTile->coord.q == ctx->playerArmy.position.q && ctx->mapView.selectedTile->coord.r == ctx->playerArmy.position.r) &&
            ctx->mapView.selectedTile->visited)
        {
            HexTile* startTile = Map_GetTile(&ctx->levelMap, ctx->playerArmy.position);
            if (startTile) {
                HexTile* path[128];
                int cost = 0;
                int pathLen = Map_FindPath(&ctx->levelMap, startTile, ctx->mapView.selectedTile, &cost, path);
                Color pathColor = (ctx->gm.currentActionPoints >= cost) ? GREEN : RED;
                if (pathLen > 0) {
                    MapView_DrawHexHighlight(ctx->mapView.selectedTile, ctx->mapView.hexRadius, ctx->mapView.origin, pathColor);

                    Vector2 p1 = ctx->mapView.visualArmyPos;
                    p1.x += ctx->mapView.origin.x;
                    HexCoord armyCoord = PixelToHex(ctx->mapView.visualArmyPos, ctx->mapView.hexRadius);
                    HexTile* armyTile = Map_GetTile(&ctx->levelMap, armyCoord);
                    float curH = armyTile ? GetBiomeHeight(armyTile->biome) : 0.0f;
                    p1.y += ctx->mapView.origin.y - curH;

                    Vector2 p2 = HexToPixel(path[0]->coord, ctx->mapView.hexRadius);
                    p2.x += ctx->mapView.origin.x;
                    p2.y += ctx->mapView.origin.y - GetBiomeHeight(path[0]->biome);
                    DrawLineEx(p1, p2, 4.0f, Fade(GOLD, 0.7f));
                    MapView_DrawPath(path, pathLen, ctx->mapView.hexRadius, ctx->mapView.origin);

                    int midIdx = pathLen / 2;
                    Vector2 midPx = HexToPixel(path[midIdx]->coord, ctx->mapView.hexRadius);
                    midPx.x += ctx->mapView.origin.x;
                    midPx.y += ctx->mapView.origin.y - GetBiomeHeight(path[midIdx]->biome);
                    DrawCircleV(midPx, 18.0f, DARKGRAY);
                    DrawCircleLines(midPx.x, midPx.y, 18.0f, pathColor);
                    const char* costTxt = TextFormat("%d PA", cost);
                    Vector2 costSz = UI_MeasureText(costTxt, 11);
                    UI_DrawText(costTxt, (Vector2){ midPx.x - costSz.x/2.0f, midPx.y - 6.0f }, 11, WHITE);
                }
            }
        } else {
            MapView_DrawHexHighlight(ctx->mapView.selectedTile, ctx->mapView.hexRadius, ctx->mapView.origin, GREEN);
        }
    }

    ScreenFX_DrawMapParticles();
    EndMode2D();

    BeginBlendMode(BLEND_MULTIPLIED);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                  (Color){ (unsigned char)ctx->mapView.skyR, (unsigned char)ctx->mapView.skyG, (unsigned char)ctx->mapView.skyB, 255 });
    EndBlendMode();

    if (ctx->gm.clock.timeOfDay == TIME_NIGHT) {
        ScreenFX_DrawNightAtmosphere(GetScreenWidth(), GetScreenHeight());
    }

    MapView_DrawOffscreenRumors(&ctx->levelMap, app->engine.cameraController.camera, ctx->mapView.hexRadius, ctx->mapView.origin, ctx->playerArmy.position);
}

static void App_DrawInterface(AppContext* app, int sw, int sh)
{
    GameContext* ctx = &app->game;
    int bottomH = 165;
    int bottomY = sh - bottomH - 12;

    /* Paneles HUD base */
    Hud_DrawCommanderPanel(&ctx->playerArmy, &ctx->gm, &ctx->hudState, 12, 48, 430);
    int statsW = 450;
    Hud_DrawArmyStatsPanel(&ctx->playerArmy, &ctx->gm, &ctx->levelMap, &ctx->hudState, 12, bottomY, statsW, bottomH);
    Hud_DrawTopBar(&ctx->gm, sw);

    int diceW = 215;
    int diceX = 12 + statsW + 12;
    Hud_DrawDicePanel(&ctx->hudState, &ctx->gm, &ctx->playerArmy, &ctx->levelMap, diceX, bottomY, diceW, bottomH);

    int minimapStart = diceX + diceW + 12;
    float minimapW = (sw > 1024) ? 420.0f : (float)(sw - minimapStart - 12);
    if (minimapW < 180.0f) minimapW = 180.0f;
    float minimapX = (float)(sw - minimapW - 12);
    Rectangle minimapRect = { minimapX, (float)bottomY, minimapW, (float)bottomH };
    UI_DrawPanelStyled(minimapRect, UI_STYLE_PANEL_DARK);
    MapView_DrawMinimap(&ctx->levelMap, minimapRect, ctx->playerArmy.position, ctx->hudState.blinkTimer, &ctx->gameAssets, ctx->mapView.debugRevealMap);

    /* Prompts [E]/[X] y Tooltips */
    Hud_DrawInteractionPrompts(&ctx->gm, &ctx->playerArmy, bottomY, sw);
    if (ctx->mapView.selectedTile != NULL && !ctx->gm.inCombat && ctx->gm.phase == PHASE_PLAYER) {
        Hud_DrawTileTooltip(ctx->mapView.selectedTile, Balance_GetGlobal(), GetMousePosition(), sw, sh);
    }

    /* Vistas activas de combate, draft, ciudad y eventos */
    if (ctx->gm.inCombat) {
        CombatView_Draw(&ctx->gm.activeCombat, sw, sh);
    } else if (ctx->gm.phase == PHASE_DRAFT) {
        if (ctx->activeDraft.registry == NULL) {
            DraftView_Init(&ctx->activeDraft, &ctx->gm.modRegistry);
        }
        if (DraftView_UpdateDraw(&ctx->activeDraft, &ctx->playerArmy, sw, sh) == 1) {
            ctx->gm.phase = PHASE_PLAYER;
            ctx->activeDraft.registry = NULL;
        }
    } else if (ctx->gm.phase == PHASE_CITY) {
        if (CityView_UpdateDraw(&ctx->gm.activeCity, &ctx->playerArmy, &ctx->gm, &ctx->levelMap, sw, sh)) {
            GameMaster_CloseCity(&ctx->gm, &ctx->levelMap, &ctx->playerArmy);
        }
    } else if (ctx->gm.phase == PHASE_EVENT) {
        EventFlow_Draw(&ctx->gm, sw, sh);
    }

    /* Modales de raciones, reportes y decisiones */
    if (GameMaster_RationPending(&ctx->gm) || ctx->rationView.showingReport) {
        RationView_Draw(&ctx->rationView, &ctx->playerArmy, &ctx->levelMap, ctx->playerArmy.position, sw, sh);
    } else if (ctx->gm.hasDayReport) {
        Hud_DrawDayReportModal(&ctx->gm.lastDayReport, sw, sh);
    }

    if (ctx->gm.showCampDecision) Hud_DrawCampDecisionModal(&ctx->gm, &ctx->playerArmy, &ctx->levelMap, sw, sh);
    if (ctx->hudState.showOfficersPanel) Hud_DrawOfficersPanel(&ctx->playerArmy, &ctx->gm, &ctx->hudState, sw, sh);
    if (ctx->hudState.showGroupsPanel) Hud_DrawGroupsPanel(&ctx->playerArmy, &ctx->gm, &ctx->hudState, sw, sh);

    /* Marcador diegético del Bastión y banner de río */
    if (ctx->gm.phase == PHASE_PLAYER && !ctx->rationView.showingReport && !ctx->gm.hasDayReport && !ctx->gm.showCampDecision) {
        MapView_DrawBastionObjectiveMarker(&ctx->levelMap, app->engine.cameraController.camera, ctx->mapView.hexRadius, ctx->mapView.origin, sw, sh);
    }
    if (ctx->gm.riverCrossTimer > 0.0f) {
        ctx->gm.riverCrossTimer -= GetFrameTime();
        Hud_DrawRiverCrossBanner(ctx->gm.riverCrossMessage, ctx->gm.riverCrossSeverity, sw);
    }

    /* Modal de cruce de río */
    if (ctx->showRiverConfirmDialog && ctx->pendingTargetTile != NULL) {
        RiverConfirmAction rAct = RiverConfirmView_Draw(ctx->pendingTargetTile, Balance_GetGlobal(), sw, sh);
        if (rAct == RIVER_ACTION_CONFIRM) {
            HexTile* startTile = Map_GetTile(&ctx->levelMap, ctx->playerArmy.position);
            if (startTile) {
                HexTile* path[128];
                int cost = 0;
                int pathLen = Map_FindPath(&ctx->levelMap, startTile, ctx->pendingTargetTile, &cost, path);
                if (pathLen > 0 && ctx->gm.currentActionPoints >= cost) {
                    for (int p = 0; p < pathLen; p++) ctx->playerArmy.movePath[p] = path[p]->coord;
                    ctx->playerArmy.pathLength = pathLen;
                    ctx->playerArmy.currentPathIndex = 0;
                    ctx->playerArmy.moveTimer = 0.0f;
                    ctx->playerArmy.isMoving = true;
                    ctx->mapView.selectedTile = NULL;
                }
            }
            ctx->showRiverConfirmDialog = false;
            ctx->pendingTargetTile = NULL;
        } else if (rAct == RIVER_ACTION_REROUTE_SAFE) {
            HexTile* startTile = Map_GetTile(&ctx->levelMap, ctx->playerArmy.position);
            if (startTile) {
                HexTile* safePath[128];
                int safeCost = 0;
                int safeLen = Map_FindPathEx(&ctx->levelMap, startTile, ctx->pendingTargetTile, false, &safeCost, safePath);
                if (safeLen > 0 && ctx->gm.currentActionPoints >= safeCost) {
                    for (int p = 0; p < safeLen; p++) ctx->playerArmy.movePath[p] = safePath[p]->coord;
                    ctx->playerArmy.pathLength = safeLen;
                    ctx->playerArmy.currentPathIndex = 0;
                    ctx->playerArmy.moveTimer = 0.0f;
                    ctx->playerArmy.isMoving = true;
                    ctx->mapView.selectedTile = NULL;
                }
            }
            ctx->showRiverConfirmDialog = false;
            ctx->pendingTargetTile = NULL;
        } else if (rAct == RIVER_ACTION_CANCEL) {
            ctx->showRiverConfirmDialog = false;
            ctx->pendingTargetTile = NULL;
        }
    }

    /* Pantallas de Fin de Partida */
    if (ctx->gm.phase == PHASE_GAME_OVER) {
        EndscreenAction act = EndscreenView_DrawGameOver(sw, sh);
        if (act == ENDSCREEN_ACTION_MENU) {
            ctx->gm.phase = PHASE_MENU;
            Menu_Init(&ctx->mainMenu);
        }
    } else if (ctx->gm.phase == PHASE_VICTORY) {
        CampaignEndStats endStats;
        GameMaster_GetCampaignStats(&ctx->gm, &ctx->levelMap, &endStats);
        EndscreenAction act = EndscreenView_DrawVictory(&endStats, sw, sh);
        if (act == ENDSCREEN_ACTION_CONTINUE) {
            GameMaster_TransitionProvince(&ctx->gm, &ctx->levelMap, &ctx->playerArmy, ctx->gm.provinceLevel + 1);
            MapView_SyncToArmy(&ctx->mapView, ctx->playerArmy.position, &app->engine.cameraController);
        } else if (act == ENDSCREEN_ACTION_MENU) {
            ctx->gm.phase = PHASE_MENU;
            Menu_Init(&ctx->mainMenu);
        }
    }

    ScreenFX_DrawOverlays();
    UI_DrawPendingTooltips();
}

void App_Draw(AppContext* app)
{
    if (!app) return;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    BeginDrawing();
    ClearBackground((Color){ 20, 24, 32, 255 });

    if (app->game.gm.phase == PHASE_MENU) {
        MenuView_Draw(&app->game.mainMenu, sw, sh);
    } else if (app->game.gm.phase == PHASE_SETUP) {
        SetupView_Draw(&app->game.armySetup, sw, sh);
    } else {
        App_DrawWorld(app);
        App_DrawInterface(app, sw, sh);
    }

    EndDrawing();
}
