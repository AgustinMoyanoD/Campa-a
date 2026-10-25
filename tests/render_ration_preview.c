#include "raylib.h"
#include "presentation/ui_widgets.h"
#include "models/army.h"
#include "models/ration.h"
#include "models/hex_tile.h"
#include "models/map_generator.h"
#include "views/ration_view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    InitWindow(1600, 900, "Ration View Bronze Age Greek Refactor Preview");
    SetTargetFPS(60);
    UI_Init();

    /* Configurar ejército idéntico al mockup del usuario */
    Army army;
    memset(&army, 0, sizeof(Army));
    army.iniciados = 10;
    army.soldados = 51;
    army.sargentos = 8;
    army.ganado = 12;

    army.agua_disponible = 1642.0f;
    army.vino_disponible = 26.0f;
    army.comida_disponible = 2396.0f;
    army.carne_disponible = 26.0f;

    army.groupState[RG_INICIADOS].morale = 80.0f;
    army.groupState[RG_INICIADOS].health = 85.0f;
    army.groupState[RG_INICIADOS].daysUnfed = 0;

    army.groupState[RG_SOLDADOS].morale = 85.0f;
    army.groupState[RG_SOLDADOS].health = 82.0f;
    army.groupState[RG_SOLDADOS].daysUnfed = 0;

    army.groupState[RG_SARGENTOS].morale = 90.0f;
    army.groupState[RG_SARGENTOS].health = 88.0f;
    army.groupState[RG_SARGENTOS].satisfaction = 8.0f;
    army.groupState[RG_SARGENTOS].satisfactionMax = 10.0f;
    army.groupState[RG_SARGENTOS].daysUnfed = 0;

    army.groupState[RG_GANADO].morale = 100.0f;
    army.groupState[RG_GANADO].health = 100.0f;
    army.groupState[RG_GANADO].daysUnfed = 0;

    Map map;
    memset(&map, 0, sizeof(Map));
    HexCoord pos = (HexCoord){ 10, 10 };

    RationViewState state;
    RationView_Init(&state, &army, &map, pos);

    /* Asignar plan exacto para que todo calce con el diseño */
    state.draftPlan.amount[RG_INICIADOS][RS_AGUA]   = 10.0f;
    state.draftPlan.amount[RG_INICIADOS][RS_COMIDA] = 10.0f;
    state.draftPlan.amount[RG_SOLDADOS][RS_AGUA]    = 51.0f;
    state.draftPlan.amount[RG_SOLDADOS][RS_COMIDA]  = 51.0f;
    state.draftPlan.amount[RG_SARGENTOS][RS_VINO]   = 8.0f;
    state.draftPlan.amount[RG_SARGENTOS][RS_CARNE]  = 8.0f;
    state.draftPlan.amount[RG_GANADO][RS_AGUA]      = 18.0f;
    state.draftPlan.amount[RG_GANADO][RS_COMIDA]    = 0.0f;

    state.groupDrinkAnim[RG_INICIADOS] = 10.0f;
    state.groupFoodAnim[RG_INICIADOS]  = 10.0f;
    state.groupDrinkAnim[RG_SOLDADOS]  = 51.0f;
    state.groupFoodAnim[RG_SOLDADOS]   = 51.0f;
    state.groupDrinkAnim[RG_SARGENTOS] = 8.0f;
    state.groupFoodAnim[RG_SARGENTOS]  = 8.0f;
    state.groupDrinkAnim[RG_GANADO]    = 18.0f;
    state.groupFoodAnim[RG_GANADO]     = 0.0f;

    /* Renderizar a textura de alta resolución (1600x900) */
    RenderTexture2D target = LoadRenderTexture(1600, 900);
    BeginTextureMode(target);
    ClearBackground((Color){ 16, 14, 18, 255 });

    /* Dibujar fondo sutil simulando el juego detrás del modal */
    DrawRectangle(0, 0, 1600, 900, (Color){ 24, 22, 20, 255 });

    /* Dibujar la pantalla modal de raciones */
    RationView_Draw(&state, &army, &map, pos, 1600, 900);

    EndTextureMode();

    Image img = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&img);

    /* Exportar a ruta local y al directorio de artefactos */
    ExportImage(img, "ration_view_preview.png");
    ExportImage(img, "C:/Users/exaul/.gemini/antigravity/brain/1ec680a5-a412-4783-8263-ef76f34df73d/ration_view_preview.png");
    UnloadImage(img);
    UnloadRenderTexture(target);

    UI_Shutdown();
    CloseWindow();
    printf("[OK] ration_view_preview.png generado exitosamente.\n");
    return 0;
}
