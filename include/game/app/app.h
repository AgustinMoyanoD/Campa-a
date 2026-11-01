#ifndef GAME_APP_H
#define GAME_APP_H

#include "raylib.h"
#include <stdbool.h>

/* Headers de Motor */
#include "engine/presentation/camera_controller.h"

/* Headers de Juego */
#include "game/models/balance_config.h"
#include "game/models/hex_tile.h"
#include "game/models/map_generator.h"
#include "game/models/general.h"
#include "game/models/army.h"
#include "game/models/enemy.h"
#include "game/models/combat.h"
#include "game/models/modifier.h"
#include "game/models/event.h"
#include "game/models/ration.h"
#include "game/models/city.h"
#include "game/models/menu.h"
#include "game/controllers/asset_manager.h"
#include "game/controllers/game_master.h"
#include "game/controllers/npc_controller.h"
#include "game/controllers/event_flow.h"
#include "game/controllers/movement_controller.h"
#include "game/presentation/audio_manager.h"
#include "game/views/map_view.h"
#include "game/views/menu_view.h"
#include "game/views/hud.h"
#include "game/views/event_view.h"
#include "game/views/combat_view.h"
#include "game/views/enemy_view.h"
#include "game/views/npc_view.h"
#include "game/views/ration_view.h"
#include "game/views/draft_view.h"
#include "game/views/city_view.h"
#include "game/views/ui_cards.h"
#include "game/views/setup_view.h"
#include "game/views/endscreen_view.h"
#include "game/views/river_confirm_view.h"

/* ============================================================================
   ESTRUCTURAS DE CONTEXTO DE LA APLICACIÓN (Bloque 9)
   ============================================================================ */

typedef struct AppEngine {
    int screenWidth;
    int screenHeight;
    CameraController cameraController;
} AppEngine;

typedef struct GameContext {
    GameMaster gm;
    Map levelMap;
    Army playerArmy;
    ArmySetupConfig armySetup;
    int provinceLevel;
    GameAssets gameAssets;
    MapViewState mapView;
    HudState hudState;
    MainMenu mainMenu;
    RationViewState rationView;
    DraftContext activeDraft;
    EventFlowState eventFlowState;

    /* Diálogo y estado interactivo */
    bool showRiverConfirmDialog;
    HexTile* pendingTargetTile;
    bool telemetryWritten;
    GamePhase lastPhase;
} GameContext;

typedef struct AppContext {
    AppEngine engine;
    GameContext game;
    bool shouldClose;
} AppContext;

/* ============================================================================
   CICLO DE VIDA DE LA APLICACIÓN
   ============================================================================ */

bool App_Init(AppContext* app, int width, int height, const char* title);
bool App_ShouldClose(const AppContext* app);
void App_Update(AppContext* app, float dt);
void App_Draw(AppContext* app);
void App_Shutdown(AppContext* app);

#endif /* GAME_APP_H */
