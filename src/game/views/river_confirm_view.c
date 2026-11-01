#include "game/views/river_confirm_view.h"
#include "engine/ui/ui_widgets.h"
#include "engine/ui/ui_skin.h"
#include "engine/ui/ui_theme.h"
#include <stdio.h>

RiverConfirmAction RiverConfirmView_Draw(const HexTile* targetTile, const BalanceConfig* bcfg, int sw, int sh)
{
    RiverConfirmAction action = RIVER_ACTION_NONE;

    if (!targetTile || !bcfg) return RIVER_ACTION_CANCEL;

    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.70f));

    float modalW = 580.0f;
    float modalH = 340.0f;
    Rectangle modal = { sw / 2.0f - modalW / 2.0f, sh / 2.0f - modalH / 2.0f, modalW, modalH };

    /* Enmarcado con UISkin y brillo temático */
    UISkin_DrawNineSliceDefault(UI_SKIN_PANEL_DARK, 0, modal, (Color){ 245, 235, 215, 255 });
    UISkin_DrawGlow(modal, (Color){ 220, 120, 50, 255 }, 0.40f, 0);

    /* Listón de cabecera en rojo terracota */
    Rectangle headRibbon = { modal.x + 12.0f, modal.y + 12.0f, modal.width - 24.0f, 34.0f };
    DrawRectangleRec(headRibbon, (Color){ 125, 38, 30, 230 });
    DrawRectangleLinesEx(headRibbon, 1.5f, (Color){ 220, 140, 60, 255 });

    Vector2 tSz = UI_MeasureText("PELIGRO: CRUCE DE RÍO", 20);
    UI_DrawText("PELIGRO: CRUCE DE RÍO", (Vector2){ modal.x + (modal.width - tSz.x) * 0.5f, headRibbon.y + 7.0f }, 20, (Color){ 255, 210, 130, 255 });

    UI_DrawText("La ruta planificada atraviesa aguas de río sin puente.", (Vector2){ modal.x + 32.0f, modal.y + 60.0f }, 15, UI_COLOR_TEXT_MAIN);
    UI_DrawText("Cruzar implica riesgo para suministros y tropa (Tirada: 2d6 + Evaluación):", (Vector2){ modal.x + 32.0f, modal.y + 82.0f }, 14, UI_COLOR_TEXT_DIM);

    Rectangle riskBox = { modal.x + 28.0f, modal.y + 108.0f, modalW - 56.0f, 104.0f };
    UISkin_DrawNineSliceDefault(UI_SKIN_SLOT, 0, riskBox, (Color){ 160, 150, 135, 255 });

    /* Indicadores de riesgo calculados desde BalanceConfig (Hallazgo 1) */
    int intermediateLossPct = (int)(bcfg->riverLossIntermediateFoodWater * 100.0f + 0.5f);
    int badLossPct = (int)(bcfg->riverLossBadFoodWater * 100.0f + 0.5f);
    int badSoldierPct = (int)(bcfg->riverLossBadSoldiers * 100.0f + 0.5f);

    DrawCircle((int)(riskBox.x + 20.0f), (int)(riskBox.y + 20.0f), 4.0f, UI_COLOR_GOOD);
    UI_DrawText("Tirada >= 8 : Cruce seguro (sin pérdidas de comida ni bajas)",
                (Vector2){ riskBox.x + 34.0f, riskBox.y + 12.0f }, 14, UI_COLOR_GOOD);

    DrawCircle((int)(riskBox.x + 20.0f), (int)(riskBox.y + 46.0f), 4.0f, UI_COLOR_GOLD);
    UI_DrawText(TextFormat("Tirada 5 a 7 : Corriente brava (~%d%% de provisiones perdidas)",
                           intermediateLossPct),
                (Vector2){ riskBox.x + 34.0f, riskBox.y + 38.0f }, 14, UI_COLOR_GOLD);

    DrawCircle((int)(riskBox.x + 20.0f), (int)(riskBox.y + 72.0f), 4.0f, UI_COLOR_BAD);
    UI_DrawText(TextFormat("Tirada <= 4 : Desastre (~%d%% provisiones y ~%d%% de bajas)",
                           badLossPct,
                           badSoldierPct),
                (Vector2){ riskBox.x + 34.0f, riskBox.y + 64.0f }, 14, UI_COLOR_BAD);

    if (targetTile->isFord) {
        Rectangle chipRec = { modal.x + 28.0f, modal.y + 224.0f, modalW - 56.0f, 26.0f };
        UI_DrawStatusChip(chipRec, TextFormat("Vado Natural detectado: Bono de +%d a la tirada de cruce", bcfg->riverFordRollBonus), UI_STYLE_CHIP_NEUTRAL, UI_ICON_AVISO);
    }

    /* 3 Botones de acción: Cruzar, Buscar otra ruta, Cancelar (Hallazgo 1) */
    float btnW = 165.0f;
    float btnH = 38.0f;
    float btnY = modal.y + modalH - 54.0f;

    Rectangle btnConfirm = { modal.x + 24.0f, btnY, btnW, btnH };
    if (UI_DrawButtonStyled(btnConfirm, "CRUZAR (Enter)", UI_STYLE_BTN_PRIMARY, false) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        action = RIVER_ACTION_CONFIRM;
    }

    Rectangle btnReroute = { modal.x + 24.0f + btnW + 12.0f, btnY, btnW + 10.0f, btnH };
    if (UI_DrawButtonStyled(btnReroute, "OTRA RUTA", UI_STYLE_BTN_DEFAULT, false)) {
        action = RIVER_ACTION_REROUTE_SAFE;
    }

    Rectangle btnCancel = { modal.x + modalW - btnW - 24.0f, btnY, btnW, btnH };
    if (UI_DrawButtonStyled(btnCancel, "CANCELAR (Esc)", UI_STYLE_BTN_DEFAULT, false) || IsKeyPressed(KEY_ESCAPE)) {
        action = RIVER_ACTION_CANCEL;
    }

    return action;
}
