#include "game/views/endscreen_view.h"
#include "engine/ui/ui_widgets.h"
#include "engine/ui/ui_skin.h"
#include "engine/ui/ui_theme.h"
#include <stdio.h>

EndscreenAction EndscreenView_DrawVictory(const CampaignEndStats* stats, int sw, int sh)
{
    EndscreenAction action = ENDSCREEN_ACTION_NONE;

    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.85f));

    float cardW = 600.0f;
    float cardH = 460.0f;
    Rectangle card = { sw / 2.0f - cardW / 2.0f, sh / 2.0f - cardH / 2.0f, cardW, cardH };

    UISkin_DrawNineSliceDefault(UI_SKIN_PANEL_DARK, 0, card, (Color){ 245, 235, 215, 255 });
    UISkin_DrawGlow(card, UI_COLOR_GOLD, 0.55f, 0);

    /* Estandarte de cabecera */
    Rectangle bannerRec = { card.x + 20.0f, card.y + 16.0f, cardW - 40.0f, 68.0f };
    UI_DrawTriumphalBanner(bannerRec, "¡VICTORIA EN LA PROVINCIA!", "Has conquistado el Bastión Central y asegurado el control de la isla.");

    float statY = card.y + 98.0f;
    Rectangle statBox = { card.x + 24.0f, statY, cardW - 48.0f, 260.0f };
    UISkin_DrawNineSliceDefault(UI_SKIN_SLOT, 0, statBox, (Color){ 160, 150, 135, 255 });

    /* Métricas de campaña */
    UI_DrawText("REGISTRO DE CAMPAÑA", (Vector2){ statBox.x + 20.0f, statBox.y + 14.0f }, 15, UI_COLOR_GOLD);
    DrawLine((int)(statBox.x + 20.0f), (int)(statBox.y + 34.0f), (int)(statBox.x + statBox.width - 20.0f), (int)(statBox.y + 34.0f), (Color){ 110, 95, 70, 180 });

    UI_DrawText(TextFormat("Días transcurridos en marcha:  %d días", stats->daysElapsed), (Vector2){ statBox.x + 24.0f, statBox.y + 46.0f }, 15, UI_COLOR_TEXT_MAIN);
    UI_DrawText(TextFormat("Bajas totales sufridas:       %d soldados", stats->totalCasualties), (Vector2){ statBox.x + 24.0f, statBox.y + 74.0f }, 15, (stats->totalCasualties > 20) ? UI_COLOR_BAD : UI_COLOR_TEXT_MAIN);
    UI_DrawText(TextFormat("Asentamientos conquistados:   %d / %d", stats->conqueredSettlements, stats->totalSettlements), (Vector2){ statBox.x + 24.0f, statBox.y + 102.0f }, 15, UI_COLOR_TEXT_MAIN);

    /* Chip de Conquista Total o Victoria Rápida */
    Rectangle statusChipRec = { statBox.x + 24.0f, statBox.y + 134.0f, statBox.width - 48.0f, 28.0f };
    if (stats->totalConquest) {
        UI_DrawStatusChip(statusChipRec, "¡CONQUISTA TOTAL (100%)! Todos los feudos han caído", UI_STYLE_CHIP_OK, UI_ICON_OK);
    } else {
        UI_DrawStatusChip(statusChipRec, "Bastión Principal Capturado (Feudos secundarios opcionales)", UI_STYLE_CHIP_NEUTRAL, UI_ICON_AVISO);
    }

    /* Insignia de Nivel y Semilla */
    Rectangle seedBox = { statBox.x + 24.0f, statBox.y + 172.0f, statBox.width - 48.0f, 32.0f };
    DrawRectangleRec(seedBox, (Color){ 25, 22, 20, 200 });
    DrawRectangleLinesEx(seedBox, 1.0f, (Color){ 80, 70, 55, 255 });
    UI_DrawText(TextFormat("NIVEL DE PROVINCIA: %d      SEMILLA DE ISLA: #%d", stats->provinceLevel, stats->seed), (Vector2){ seedBox.x + 16.0f, seedBox.y + 8.0f }, 13, UI_COLOR_TEXT_DIM);

    /* Botones de acción */
    Rectangle btnContinue = { card.x + 30.0f, card.y + cardH - 64.0f, 250.0f, 42.0f };
    if (UI_DrawButtonStyled(btnContinue, "CONTINUAR CAMPAÑA", UI_STYLE_BTN_PRIMARY, false) || IsKeyPressed(KEY_ENTER)) {
        action = ENDSCREEN_ACTION_CONTINUE;
    }

    Rectangle btnMenu = { card.x + cardW - 280.0f, card.y + cardH - 64.0f, 250.0f, 42.0f };
    if (UI_DrawButtonStyled(btnMenu, "VOLVER AL MENÚ", UI_STYLE_BTN_DEFAULT, false) || IsKeyPressed(KEY_ESCAPE)) {
        action = ENDSCREEN_ACTION_MENU;
    }

    return action;
}

EndscreenAction EndscreenView_DrawGameOver(int sw, int sh)
{
    EndscreenAction action = ENDSCREEN_ACTION_NONE;

    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.8f));
    Vector2 sz1 = UI_MeasureText("DERROTA TOTAL", 40);
    UI_DrawText("DERROTA TOTAL", (Vector2){ sw/2.0f - sz1.x/2.0f, sh/2.0f - 60.0f }, 40, RED);

    Vector2 sz2 = UI_MeasureText("Tu ejército ha sido aniquilado en campaña.", 20);
    UI_DrawText("Tu ejército ha sido aniquilado en campaña.", (Vector2){ sw/2.0f - sz2.x/2.0f, sh/2.0f }, 20, LIGHTGRAY);

    Rectangle btnMenu = { sw / 2.0f - 130.0f, sh / 2.0f + 60.0f, 260.0f, 42.0f };
    if (UI_DrawButtonStyled(btnMenu, "VOLVER AL MENÚ", UI_STYLE_BTN_DEFAULT, false) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
        action = ENDSCREEN_ACTION_MENU;
    }

    return action;
}
