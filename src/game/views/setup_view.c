#include "game/views/setup_view.h"
#include "engine/ui/ui_widgets.h"
#include "engine/ui/ui_skin.h"
#include "engine/ui/ui_theme.h"
#include <stdio.h>

SetupViewAction SetupView_Draw(const ArmySetupConfig* setup, int sw, int sh)
{
    SetupViewAction action = SETUP_ACTION_NONE;

    float panelW = 780.0f;
    float panelH = 560.0f;
    Rectangle panel = { sw / 2.0f - panelW / 2.0f, sh / 2.0f - panelH / 2.0f, panelW, panelH };

    /* Fondo estilizado helénico */
    UISkin_DrawNineSliceDefault(UI_SKIN_PANEL_DARK, 0, panel, (Color){ 245, 235, 215, 255 });
    UISkin_DrawGlow(panel, UI_COLOR_GOLD, 0.50f, 0);

    /* Estandarte de cabecera */
    Rectangle bannerRec = { panel.x + 20.0f, panel.y + 16.0f, panelW - 40.0f, 64.0f };
    UI_DrawTriumphalBanner(bannerRec, "PREPARACIÓN DE LA EXPEDICIÓN", "Configura tu Estado Mayor y recursos iniciales antes del desembarco");

    float colW = (panelW - 60.0f) * 0.5f;
    float colY = panel.y + 94.0f;
    float colH = 370.0f;

    /* --- Columna Izquierda: Comandante y Estado Mayor --- */
    Rectangle leftBox = { panel.x + 20.0f, colY, colW, colH };
    UISkin_DrawNineSliceDefault(UI_SKIN_SLOT, 0, leftBox, (Color){ 160, 150, 135, 255 });

    UI_DrawText("ESTADO MAYOR & COMANDANTE", (Vector2){ leftBox.x + 18.0f, leftBox.y + 14.0f }, 15, UI_COLOR_GOLD);
    DrawLine((int)(leftBox.x + 18.0f), (int)(leftBox.y + 34.0f), (int)(leftBox.x + leftBox.width - 18.0f), (int)(leftBox.y + 34.0f), (Color){ 110, 95, 70, 180 });

    const GeneralConfig* gen = &setup->general;
    UI_DrawText(TextFormat("Comandante: %s", gen->name), (Vector2){ leftBox.x + 18.0f, leftBox.y + 46.0f }, 16, UI_COLOR_TEXT_MAIN);
    UI_DrawText(TextFormat("Edad: %d años | Rango: General", gen->age), (Vector2){ leftBox.x + 18.0f, leftBox.y + 70.0f }, 13, UI_COLOR_TEXT_DIM);

    /* Atributos del Comandante */
    UI_DrawText(TextFormat("Planificación:   %d", gen->stats.planning), (Vector2){ leftBox.x + 24.0f, leftBox.y + 104.0f }, 14, UI_COLOR_TEXT_MAIN);
    UI_DrawText(TextFormat("Evaluación:      %d", gen->stats.evaluation), (Vector2){ leftBox.x + 24.0f, leftBox.y + 130.0f }, 14, UI_COLOR_TEXT_MAIN);
    UI_DrawText(TextFormat("Ejecución:       %d", gen->stats.execution), (Vector2){ leftBox.x + 24.0f, leftBox.y + 156.0f }, 14, UI_COLOR_TEXT_MAIN);

    /* Tenientes asignados */
    UI_DrawText("Tenientes de Vanguardia:", (Vector2){ leftBox.x + 18.0f, leftBox.y + 196.0f }, 14, UI_COLOR_GOLD);
    for (int lt = 0; lt < setup->lieutenantCount && lt < 3; lt++) {
        UI_DrawText(TextFormat("• %s (%s, Dios: %s)", setup->lieutenants[lt].name,
                               OfficerRole_GetName(setup->lieutenants[lt].role),
                               setup->lieutenants[lt].patronGod),
                    (Vector2){ leftBox.x + 24.0f, leftBox.y + 224.0f + lt * 24.0f }, 13, UI_COLOR_TEXT_DIM);
    }

    Rectangle chipInfo = { leftBox.x + 16.0f, leftBox.y + colH - 46.0f, leftBox.width - 32.0f, 28.0f };
    UI_DrawStatusChip(chipInfo, "Perfil balanceado para la 1ra Campaña", UI_STYLE_CHIP_NEUTRAL, UI_ICON_OK);

    /* --- Columna Derecha: Tropa y Recursos --- */
    Rectangle rightBox = { panel.x + 30.0f + colW, colY, colW, colH };
    UISkin_DrawNineSliceDefault(UI_SKIN_SLOT, 0, rightBox, (Color){ 160, 150, 135, 255 });

    UI_DrawText("DOTACIÓN Y RECURSOS INICIALES", (Vector2){ rightBox.x + 18.0f, rightBox.y + 14.0f }, 15, UI_COLOR_GOLD);
    DrawLine((int)(rightBox.x + 18.0f), (int)(rightBox.y + 34.0f), (int)(rightBox.x + rightBox.width - 18.0f), (int)(rightBox.y + 34.0f), (Color){ 110, 95, 70, 180 });

    UI_DrawText(TextFormat("Iniciados de leva:   %d hombres", setup->iniciados), (Vector2){ rightBox.x + 24.0f, rightBox.y + 50.0f }, 14, UI_COLOR_TEXT_MAIN);
    UI_DrawText(TextFormat("Soldados de línea:   %d hoplitas", setup->soldados), (Vector2){ rightBox.x + 24.0f, rightBox.y + 80.0f }, 14, UI_COLOR_TEXT_MAIN);
    UI_DrawText(TextFormat("Sargentos veteranos: %d oficiales", setup->sargentos), (Vector2){ rightBox.x + 24.0f, rightBox.y + 110.0f }, 14, UI_COLOR_TEXT_MAIN);
    UI_DrawText(TextFormat("Ganado para marcha:  %d reses", setup->ganado), (Vector2){ rightBox.x + 24.0f, rightBox.y + 140.0f }, 14, UI_COLOR_TEXT_MAIN);
    UI_DrawText(TextFormat("Raciones de comida:  %.0f medidas", setup->comida), (Vector2){ rightBox.x + 24.0f, rightBox.y + 170.0f }, 14, UI_COLOR_GOOD);
    UI_DrawText(TextFormat("Odres de agua dulce: %.0f medidas", setup->agua), (Vector2){ rightBox.x + 24.0f, rightBox.y + 200.0f }, 14, UI_COLOR_GOOD);
    UI_DrawText(TextFormat("Monedas de tesorería: %d oro", setup->monedas), (Vector2){ rightBox.x + 24.0f, rightBox.y + 230.0f }, 14, UI_COLOR_GOLD);

    Rectangle chipSupply = { rightBox.x + 16.0f, rightBox.y + colH - 46.0f, rightBox.width - 32.0f, 28.0f };
    UI_DrawStatusChip(chipSupply, "Suministros para aprox. 3,5 días de marcha", UI_STYLE_CHIP_OK, UI_ICON_AVISO);

    /* --- Botonera Inferior --- */
    Rectangle btnBack = { panel.x + 30.0f, panel.y + panelH - 58.0f, 200.0f, 40.0f };
    if (UI_DrawButtonStyled(btnBack, "VOLVER AL MENÚ", UI_STYLE_BTN_DEFAULT, false) || IsKeyPressed(KEY_ESCAPE)) {
        action = SETUP_ACTION_BACK;
    }

    Rectangle btnStart = { panel.x + panelW - 270.0f, panel.y + panelH - 58.0f, 240.0f, 40.0f };
    if (UI_DrawButtonStyled(btnStart, "INICIAR EXPEDICIÓN", UI_STYLE_BTN_PRIMARY, false) || IsKeyPressed(KEY_ENTER)) {
        action = SETUP_ACTION_START;
    }

    return action;
}
