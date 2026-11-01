#include "views/ration_view.h"
#include "presentation/ui_theme.h"
#include "presentation/ui_widgets.h"
#include "presentation/audio_manager.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Enrutamiento condicional al Mock si RATION_UI_MOCK está definido */
#ifdef RATION_UI_MOCK
#include "views/ration_mock.h"
#define V_Ration_GetNeeds       RationMock_GetNeeds
#define V_Ration_GetStock       RationMock_GetStock
#define V_Ration_GetGroupState  RationMock_GetGroupState
#define V_Ration_Auto           RationMock_Auto
#define V_Ration_Repeat         RationMock_Repeat
#define V_Ration_Validate       RationMock_Validate
#define V_Ration_Preview        RationMock_Preview
#define V_Ration_NeedsDecision  RationMock_NeedsDecision
#else
#include "models/ration.h"
#define V_Ration_GetNeeds       Ration_GetNeeds
#define V_Ration_GetStock       Ration_GetStock
#define V_Ration_GetGroupState  Ration_GetGroupState
#define V_Ration_Auto           Ration_Auto
#define V_Ration_Repeat         Ration_Repeat
#define V_Ration_Validate       Ration_Validate
#define V_Ration_Preview        Ration_Preview
#define V_Ration_NeedsDecision  Ration_NeedsDecision
#endif

/* ============================================================
   TEMA Y ESTÉTICA ANTIGUA GRECIA / EDAD DEL BRONCE (#define)
   ============================================================ */

/* Paleta de Colores de Fondo y Estructura */
#define RATION_COLOR_BG_OVERLAY         (Color){ 12, 11, 10, 220 }    /* Telón oscuro exterior */
#define RATION_COLOR_MODAL_BG           (Color){ 24, 21, 20, 252 }    /* Pizarra piedra cálida */
#define RATION_COLOR_PANEL_STONE        (Color){ 34, 30, 28, 255 }    /* Tarjeta de grupo (piedra cálida) */
#define RATION_COLOR_PANEL_STONE_DARK   (Color){ 22, 19, 18, 255 }    /* Cavidad / caja interior */
#define RATION_COLOR_FORECAST_BG        (Color){ 25, 23, 22, 250 }    /* Caja inferior de pronóstico */
#define RATION_COLOR_HEADER_BG          (Color){ 44, 38, 34, 255 }    /* Cabecera de grupo */

/* Bordes y Filetes de Bronce Antiguo */
#define RATION_COLOR_BRONZE_OLD         (Color){ 140, 115, 75, 255 }   /* Bronce viejo envejecido */
#define RATION_COLOR_BRONZE_LIGHT       (Color){ 195, 165, 115, 255 }  /* Bronce dorado pulido */
#define RATION_COLOR_BRONZE_DARK        (Color){ 70, 55, 38, 255 }     /* Sombra de bronce */
#define RATION_COLOR_STONE_BORDER       (Color){ 90, 78, 65, 200 }     /* Marco de piedra labrada */

/* Recursos Desaturados y Armónicos */
#define RATION_COLOR_WATER              (Color){ 90, 145, 175, 255 }   /* Azul celeste apagado */
#define RATION_COLOR_WATER_FILL         (Color){ 90, 145, 175, 175 }   /* Relleno slider agua */
#define RATION_COLOR_WINE               (Color){ 165, 75, 85, 255 }    /* Rojo borgoña / terracota suave */
#define RATION_COLOR_WINE_FILL          (Color){ 165, 75, 85, 175 }    /* Relleno slider vino */
#define RATION_COLOR_FOOD               (Color){ 185, 155, 80, 255 }   /* Ocre / trigo dorado */
#define RATION_COLOR_FOOD_FILL          (Color){ 185, 155, 80, 175 }   /* Relleno slider comida */
#define RATION_COLOR_MEAT               (Color){ 175, 95, 60, 255 }    /* Terracota cobrizo */
#define RATION_COLOR_MEAT_FILL          (Color){ 175, 95, 60, 175 }    /* Relleno slider carne */

/* Barras de Estado (Moral, Salud, Satisfacción) */
#define RATION_COLOR_BAR_BG             (Color){ 20, 18, 17, 255 }     /* Fondo de barra */
#define RATION_COLOR_HEALTH             (Color){ 95, 150, 75, 255 }    /* Verde laurel apagado */
#define RATION_COLOR_MORAL              (Color){ 115, 165, 70, 255 }   /* Verde oliva suave */
#define RATION_COLOR_SATISFACTION       (Color){ 210, 175, 85, 255 }   /* Miel dorada */

/* Indicadores de Estado y Alertas */
#define RATION_COLOR_STATUS_GOOD        (Color){ 125, 175, 95, 255 }   /* Verde oliva óptimo */
#define RATION_COLOR_STATUS_WARN        (Color){ 210, 145, 55, 255 }   /* Alerta ámbar */
#define RATION_COLOR_STATUS_DANGER      (Color){ 195, 65, 55, 255 }    /* Terracota / peligro */
#define RATION_COLOR_WARNING_BG         (Color){ 65, 40, 20, 230 }     /* Fondo badge advertencia */

/* Botones y Controles */
#define RATION_COLOR_BTN_BG             (Color){ 45, 38, 34, 255 }     /* Fondo botón estándar */
#define RATION_COLOR_BTN_HOVER          (Color){ 68, 58, 50, 255 }     /* Hover botón */
#define RATION_COLOR_CONFIRM_BG         (Color){ 48, 85, 44, 255 }     /* Verde laurel confirmación */
#define RATION_COLOR_CONFIRM_HOVER      (Color){ 62, 110, 56, 255 }

/* Tipografía */
#define RATION_COLOR_TEXT_TITLE         (Color){ 242, 228, 200, 255 }  /* Pergamino claro */
#define RATION_COLOR_TEXT_MAIN          (Color){ 220, 212, 198, 255 }  /* Texto principal */
#define RATION_COLOR_TEXT_MUTED         (Color){ 165, 152, 138, 255 }  /* Texto secundario */
#define RATION_COLOR_TEXT_GOLD          (Color){ 230, 195, 120, 255 }  /* Acento dorado */

/* Tamaños de Fuente */
#define RATION_FONT_MODAL_TITLE         22
#define RATION_FONT_MODAL_SUB           13
#define RATION_FONT_GROUP_TITLE         18
#define RATION_FONT_STOCK_BIG           20
#define RATION_FONT_RESOURCE_LABEL      13
#define RATION_FONT_BAR_TEXT            12
#define RATION_FONT_CHIP                13
#define RATION_FONT_FORECAST_HEAD       12
#define RATION_FONT_FORECAST_TEXT       12
#define RATION_FONT_FOOTER              14

/* Dimensiones y Espaciados Generales */
#define RATION_MODAL_PAD_X              16.0f
#define RATION_HEADER_HEIGHT            44.0f
#define RATION_TOP_CARD_HEIGHT          66.0f
#define RATION_GROUP_GAP                10.0f
#define RATION_FOOTER_HEIGHT            54.0f

/* Alturas y Espaciados de Paneles de Grupo */
#define RATION_GROUP_HEAD_HEIGHT        34.0f

/* Barras de Estado (Moral / Salud / Satisfacción) */
#define RATION_BAR_HEIGHT_SD            18.0f
#define RATION_BAR_HEIGHT_HD            22.0f
#define RATION_BAR_GAP_SD               6.0f
#define RATION_BAR_GAP_HD               8.0f

/* Sliders de Recursos */
#define RATION_SLIDER_HEIGHT_SD         32.0f
#define RATION_SLIDER_HEIGHT_HD         36.0f
#define RATION_SLIDER_BTN_SIZE_SD       26.0f
#define RATION_SLIDER_BTN_SIZE_HD       28.0f
#define RATION_SLIDER_GAP_SD            6.0f
#define RATION_SLIDER_GAP_HD            8.0f

/* Estandarte y Pronóstico Inferior */
#define RATION_BANNER_HEIGHT_SD         28.0f
#define RATION_BANNER_HEIGHT_HD         32.0f
#define RATION_FORECAST_HEIGHT_SD       54.0f
#define RATION_FORECAST_HEIGHT_HD       60.0f

static const Color s_resourceColors[RS_COUNT] = {
    RATION_COLOR_WATER,
    RATION_COLOR_WINE,
    RATION_COLOR_FOOD,
    RATION_COLOR_MEAT
};

static const Color s_resourceFillColors[RS_COUNT] = {
    RATION_COLOR_WATER_FILL,
    RATION_COLOR_WINE_FILL,
    RATION_COLOR_FOOD_FILL,
    RATION_COLOR_MEAT_FILL
};

static const UIIconId s_resourceIcons[RS_COUNT] = {
    UI_ICON_AGUA,
    UI_ICON_VINO,
    UI_ICON_COMIDA,
    UI_ICON_CARNE
};

static const char* s_resourceNames[RS_COUNT] = {
    "Agua", "Vino", "Comida", "Carne"
};

static const char* s_groupNames[RG_COUNT] = {
    "Iniciados", "Soldados", "Sargentos", "Ganado"
};

/* ============================================================
   ESTRUCTURA DE LAYOUT COMPARTIDO (1280x720, 1600x900, etc.)
   ============================================================ */

typedef struct {
    Rectangle modalBounds;
    Rectangle headerRect;

    /* Pilas de suministros */
    Rectangle supplyCardRects[RS_COUNT];
    Rectangle slaughterBoxRect;
    Rectangle slaughterMinusBtn;
    Rectangle slaughterPlusBtn;
    Rectangle slaughterActionBtn;

    /* 4 Paneles de Grupo */
    Rectangle groupPanelRects[RG_COUNT];
    Rectangle groupResSlots[RG_COUNT][RS_COUNT];
    Rectangle groupMinusBtns[RG_COUNT][RS_COUNT];
    Rectangle groupPlusBtns[RG_COUNT][RS_COUNT];

    /* Pie y Botones */
    Rectangle footerRect;
    Rectangle btnAuto;
    Rectangle btnRepeat;
    Rectangle btnReset;
    Rectangle btnConfirm;

    /* Modo Compacto (Abundancia) */
    Rectangle compactBounds;
    Rectangle compactConfirmBtn;
    Rectangle compactExpandBtn;

    /* Página 2: Reporte */
    Rectangle reportBounds;
    Rectangle reportContinueBtn;
} RationViewLayout;

static void ComputeLayout(RationViewLayout* l, int sw, int sh, bool isCompact, bool isReport) {
    memset(l, 0, sizeof(RationViewLayout));

    /* 1. Modo Compacto (Abundancia) */
    if (isCompact) {
        float cw = (sw >= 1600) ? 680.0f : 580.0f;
        float ch = 320.0f;
        l->compactBounds = (Rectangle){ (sw - cw) / 2.0f, (sh - ch) / 2.0f, cw, ch };
        float btnW = (cw - 60.0f) / 2.0f;
        l->compactConfirmBtn = (Rectangle){ l->compactBounds.x + 20.0f, l->compactBounds.y + ch - 60.0f, btnW, 40.0f };
        l->compactExpandBtn  = (Rectangle){ l->compactBounds.x + 40.0f + btnW, l->compactBounds.y + ch - 60.0f, btnW, 40.0f };
        return;
    }

    /* 2. Página 2: Reporte del Día */
    if (isReport) {
        float rw = (sw >= 1600) ? 840.0f : 700.0f;
        float rh = (sh >= 900)  ? 540.0f : 460.0f;
        l->reportBounds = (Rectangle){ (sw - rw) / 2.0f, (sh - rh) / 2.0f, rw, rh };
        l->reportContinueBtn = (Rectangle){ l->reportBounds.x + (rw - 320.0f) / 2.0f, l->reportBounds.y + rh - 56.0f, 320.0f, 40.0f };
        return;
    }

    /* 3. Modal Principal de Reparto de Raciones (Centrado y equilibrado) */
    float pad = (sw >= 1600) ? 30.0f : 16.0f;
    float mw = (sw >= 1600) ? 1360.0f : ((sw >= 1280) ? 1180.0f : ((float)sw - (pad * 2.0f)));
    float mh = (sh >= 900)  ? 720.0f  : ((sh >= 720)  ? 640.0f : ((float)sh - (pad * 2.0f)));
    float mx = ((float)sw - mw) / 2.0f;
    float my = ((float)sh - mh) / 2.0f;
    l->modalBounds = (Rectangle){ mx, my, mw, mh };

    /* Cabecera */
    l->headerRect = (Rectangle){ mx + RATION_MODAL_PAD_X, my + 8.0f, mw - (RATION_MODAL_PAD_X * 2.0f), RATION_HEADER_HEIGHT };

    /* Pilas superiores de suministros (Agua, Vino, Comida, Carne + Sacrificio) */
    float topY = my + 52.0f;
    float cardW = (mw - (RATION_MODAL_PAD_X * 2.0f) - 40.0f) / 5.0f;
    float cardH = RATION_TOP_CARD_HEIGHT;

    for (int r = 0; r < RS_COUNT; r++) {
        l->supplyCardRects[r] = (Rectangle){ mx + RATION_MODAL_PAD_X + (float)r * (cardW + 10.0f), topY, cardW, cardH };
    }

    /* Caja de Sacrificar Ganado (a la derecha de Carne) */
    float slX = mx + RATION_MODAL_PAD_X + 4.0f * (cardW + 10.0f);
    float slW = cardW;
    l->slaughterBoxRect = (Rectangle){ slX, topY, slW, cardH };
    float slBtnSize = 24.0f;
    l->slaughterMinusBtn  = (Rectangle){ slX + 10.0f, topY + 34.0f, slBtnSize, slBtnSize };
    l->slaughterPlusBtn   = (Rectangle){ slX + 10.0f + slBtnSize + 48.0f, topY + 34.0f, slBtnSize, slBtnSize };
    l->slaughterActionBtn = (Rectangle){ slX + slW - 74.0f, topY + 34.0f,  66.0f, slBtnSize };

    /* 4 Paneles de Grupo */
    float groupY = topY + cardH + 10.0f;
    float footerH = RATION_FOOTER_HEIGHT;
    float groupH = mh - (groupY - my) - footerH - 12.0f;
    float groupW = (mw - (RATION_MODAL_PAD_X * 2.0f) - 30.0f) / 4.0f;

    bool isHD = (sh >= 900);
    float slotH = isHD ? RATION_SLIDER_HEIGHT_HD : RATION_SLIDER_HEIGHT_SD;
    float btnSize = isHD ? RATION_SLIDER_BTN_SIZE_HD : RATION_SLIDER_BTN_SIZE_SD;
    float slotGap = isHD ? RATION_SLIDER_GAP_HD : RATION_SLIDER_GAP_SD;

    float div1Y_rel = isHD ? 134.0f : 116.0f;
    float drinkSlotStartY = groupY + div1Y_rel + 26.0f;
    float div2Y_rel = isHD ? 258.0f : 221.0f;
    float foodSlotStartY  = groupY + div2Y_rel + 26.0f;

    for (int g = 0; g < RG_COUNT; g++) {
        float gx = mx + RATION_MODAL_PAD_X + (float)g * (groupW + RATION_GROUP_GAP);
        l->groupPanelRects[g] = (Rectangle){ gx, groupY, groupW, groupH };

        for (int r = 0; r < RS_COUNT; r++) {
            float sy = (r < 2) ? (drinkSlotStartY + (float)r * (slotH + slotGap))
                               : (foodSlotStartY  + (float)(r - 2) * (slotH + slotGap));
            l->groupResSlots[g][r] = (Rectangle){ gx + 10.0f, sy, groupW - 20.0f, slotH };
            l->groupPlusBtns[g][r]  = (Rectangle){ gx + groupW - 10.0f - btnSize, sy + (slotH - btnSize) / 2.0f, btnSize, btnSize };
            l->groupMinusBtns[g][r] = (Rectangle){ l->groupPlusBtns[g][r].x - btnSize - 4.0f, sy + (slotH - btnSize) / 2.0f, btnSize, btnSize };
        }
    }

    /* Pie de página y Botones */
    float footY = my + mh - footerH - 6.0f;
    l->footerRect = (Rectangle){ mx + RATION_MODAL_PAD_X, footY, mw - (RATION_MODAL_PAD_X * 2.0f), footerH };

    float btnH = 38.0f;
    float by = footY + (footerH - btnH) / 2.0f;
    float btnW = (sw >= 1600) ? 170.0f : 150.0f;
    float confirmW = (sw >= 1600) ? 260.0f : 230.0f;

    l->btnConfirm = (Rectangle){ mx + mw - RATION_MODAL_PAD_X - confirmW, by, confirmW, btnH };
    l->btnRepeat  = (Rectangle){ l->btnConfirm.x - btnW - 12.0f, by, btnW, btnH };
    l->btnAuto    = (Rectangle){ l->btnRepeat.x - btnW - 12.0f, by, btnW, btnH };
    l->btnReset   = (Rectangle){ 0, 0, 0, 0 };
}

/* ============================================================
   TARJETA DE PASTOREO NATURAL (CON SLOT 9-SLICE E ICONO)
   ============================================================ */

static void DrawPastureGrass(Rectangle bounds) {
    UI_DrawSlot(bounds, (Color){ 65, 105, 55, 255 }, UI_ICON_NONE);

    UI_DrawIcon(UI_ICON_GANADO, (Vector2){ bounds.x + 10.0f, bounds.y + 8.0f }, 16.0f, WHITE);
    UI_DrawText("PASTOREO NATURAL:", (Vector2){ bounds.x + 30.0f, bounds.y + 8.0f }, 12, RATION_COLOR_BRONZE_LIGHT);
    UI_DrawText("0 comida de reserva requerida", (Vector2){ bounds.x + 12.0f, bounds.y + 26.0f }, 12, RATION_COLOR_STATUS_GOOD);

    float cx1 = bounds.x + bounds.width * 0.35f;
    float cx2 = bounds.x + bounds.width * 0.72f;
    float baseY = bounds.y + bounds.height - 12.0f;

    /* Montículo 1 */
    DrawEllipse((int)cx1, (int)(baseY + 2), 24, 7, (Color){ 62, 50, 36, 255 });
    DrawEllipseLines((int)cx1, (int)(baseY + 2), 24, 7, (Color){ 85, 70, 50, 255 });

    Color gCol1 = (Color){ 115, 175, 80, 255 };
    Color gCol2 = (Color){ 80, 135, 55, 255 };
    DrawLineEx((Vector2){ cx1 - 10, baseY + 2 }, (Vector2){ cx1 - 14, baseY - 12 }, 2.0f, gCol1);
    DrawLineEx((Vector2){ cx1 - 5, baseY + 2 },  (Vector2){ cx1 - 6,  baseY - 18 }, 2.0f, gCol2);
    DrawLineEx((Vector2){ cx1, baseY + 2 },      (Vector2){ cx1 + 1,  baseY - 20 }, 2.5f, gCol1);
    DrawLineEx((Vector2){ cx1 + 6, baseY + 2 },  (Vector2){ cx1 + 8,  baseY - 16 }, 2.0f, gCol2);
    DrawLineEx((Vector2){ cx1 + 12, baseY + 2 }, (Vector2){ cx1 + 16, baseY - 10 }, 2.0f, gCol1);

    /* Montículo 2 */
    DrawEllipse((int)cx2, (int)(baseY + 2), 28, 8, (Color){ 58, 46, 34, 255 });
    DrawEllipseLines((int)cx2, (int)(baseY + 2), 28, 8, (Color){ 80, 65, 46, 255 });

    DrawLineEx((Vector2){ cx2 - 12, baseY + 2 }, (Vector2){ cx2 - 16, baseY - 14 }, 2.0f, gCol2);
    DrawLineEx((Vector2){ cx2 - 6, baseY + 2 },  (Vector2){ cx2 - 7,  baseY - 22 }, 2.5f, gCol1);
    DrawLineEx((Vector2){ cx2, baseY + 2 },      (Vector2){ cx2 + 2,  baseY - 24 }, 2.5f, gCol2);
    DrawLineEx((Vector2){ cx2 + 7, baseY + 2 },  (Vector2){ cx2 + 10, baseY - 19 }, 2.0f, gCol1);
    DrawLineEx((Vector2){ cx2 + 14, baseY + 2 }, (Vector2){ cx2 + 18, baseY - 12 }, 2.0f, gCol2);
}

/* ============================================================
   INICIALIZACIÓN Y CICLO DE VIDA DE LA VISTA
   ============================================================ */

void RationView_Init(RationViewState* state, const Army* army, const Map* map, HexCoord pos) {
    if (!state || !army) return;
    memset(state, 0, sizeof(RationViewState));

    RationNeeds needs = V_Ration_GetNeeds(army, map, pos);

    /* Siempre abrir en la vista principal de reparto para permitir decidir entre auto y gestionar */
    state->isCompactMode = false;

    /* Plan inicial: intentar repetir el de ayer ajustado; si no, usar automático */
    RationPlan repPlan = V_Ration_Repeat(army);
    char err[64];
    if (V_Ration_Validate(army, &repPlan, err, sizeof(err))) {
        state->draftPlan = repPlan;
    } else {
        state->draftPlan = V_Ration_Auto(army, &needs);
    }
    state->initialPlan = state->draftPlan;

    UIDragDrop_Init(&state->dragDrop);
    state->cattleToSlaughter = (army->ganado > 0) ? 1 : 0;
    state->showingReport = false;
    state->initialized = true;

    state->isValid = V_Ration_Validate(army, &state->draftPlan, state->validationError, sizeof(state->validationError));
}

void RationView_ShowReport(RationViewState* state, const DayReport* report) {
    if (!state) return;
    state->showingReport = true;
    if (report) state->lastReport = *report;
}

/* ============================================================
   UPDATE (PROCESAMIENTO DE ENTRADA Y ACCIONES MVC)
   ============================================================ */

RationViewAction RationView_Update(RationViewState* state, const Army* army, const Map* map, HexCoord pos, int screenWidth, int screenHeight) {
    RationViewAction action = { RATION_ACTION_NONE, {0}, 0 };
    if (!state || !army) return action;

    Vector2 mouse = GetMousePosition();
    RationViewLayout layout;
    ComputeLayout(&layout, screenWidth, screenHeight, state->isCompactMode, state->showingReport);

    /* 1. Página 2: Reporte del Día */
    if (state->showingReport) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
           (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, layout.reportContinueBtn))) {
            Audio_PlaySfx(SFX_UI_CONFIRM);
            action.type = RATION_ACTION_CLOSE_REPORT;
            return action;
        }
        return action;
    }

    /* 2. Modo Compacto (Abundancia) */
    if (state->isCompactMode) {
        if (IsKeyPressed(KEY_ENTER) ||
           (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, layout.compactConfirmBtn))) {
            Audio_PlaySfx(SFX_UI_CONFIRM);
            action.type = RATION_ACTION_SUBMIT_PLAN;
            action.plan = state->draftPlan;
            return action;
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, layout.compactExpandBtn)) {
            Audio_PlaySfx(SFX_UI_CLICK);
            state->isCompactMode = false;
            return action;
        }
        return action;
    }

    /* 3. Drag and Drop: Actualización */
    UIDragDrop_Update(&state->dragDrop, mouse);

    /* Iniciar arrastre desde pilas de recursos */
    if (!state->dragDrop.active && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        for (int r = 0; r < RS_COUNT; r++) {
            if (CheckCollisionPointRec(mouse, layout.supplyCardRects[r])) {
                float totalStock = V_Ration_GetStock(army, (RationResource)r);
                float assigned = 0.0f;
                for (int g = 0; g < RG_COUNT; g++) assigned += state->draftPlan.amount[g][r];
                float available = (totalStock >= assigned) ? (totalStock - assigned) : 0.0f;

                if (available > 0.0f) {
                    float packet = 5.0f;
                    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) packet = 20.0f;
                    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) packet = 1.0f;
                    if (packet > available) packet = available;

                    UIDragDrop_Start(&state->dragDrop, r, -1, packet, mouse, s_resourceNames[r], s_resourceColors[r]);
                    Audio_PlaySfx(SFX_UI_CLICK);
                    break;
                }
            }
        }
    }

    /* Finalizar arrastre (soltar) */
    if (state->dragDrop.active && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        int srcKind, srcGroup;
        float amount;
        if (UIDragDrop_Finish(&state->dragDrop, &srcKind, &srcGroup, &amount)) {
            int droppedGroup = -1;
            for (int g = 0; g < RG_COUNT; g++) {
                if (CheckCollisionPointRec(mouse, layout.groupPanelRects[g])) {
                    droppedGroup = g;
                    break;
                }
            }

            if (droppedGroup >= 0) {
                if (droppedGroup == RG_GANADO && srcKind != RS_AGUA) {
                    Audio_PlaySfx(SFX_DICE_LAND); /* Sonido de advertencia: el ganado solo bebe agua */
                } else {
                    if (srcGroup >= 0) {
                        state->draftPlan.amount[srcGroup][srcKind] -= amount;
                        if (state->draftPlan.amount[srcGroup][srcKind] < 0.0f) state->draftPlan.amount[srcGroup][srcKind] = 0.0f;
                    }
                    state->draftPlan.amount[droppedGroup][srcKind] += amount;
                    Audio_PlaySfx(SFX_UI_CLICK);
                }
            } else if (srcGroup >= 0) {
                state->draftPlan.amount[srcGroup][srcKind] -= amount;
                if (state->draftPlan.amount[srcGroup][srcKind] < 0.0f) state->draftPlan.amount[srcGroup][srcKind] = 0.0f;
                Audio_PlaySfx(SFX_UI_CLICK);
            }
        }
    }

    /* 4. Interacción con Sliders de Recursos (Click/Arrastre directo, Botones +/- y Rueda) */
    RationNeeds needs = V_Ration_GetNeeds(army, map, pos);

    for (int g = 0; g < RG_COUNT; g++) {
        for (int r = 0; r < RS_COUNT; r++) {
            if (g == RG_GANADO && r != RS_AGUA) continue;

            Rectangle slot = layout.groupResSlots[g][r];
            float labelW = (screenWidth >= 1600 || screenHeight >= 900) ? 78.0f : 66.0f;
            float btnW = layout.groupPlusBtns[g][r].width;
            float trackX = slot.x + labelW;
            float trackW = slot.width - labelW - (btnW * 2.0f + 8.0f);
            Rectangle trackRect = { trackX, slot.y + 2.0f, trackW, slot.height - 4.0f };

            /* Arrastre / Click directo sobre la barra interactiva (Slider) */
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, trackRect) && !state->dragDrop.active) {
                float totalReq = (r < 2) ? needs.drink[g] : needs.food[g];
                if (totalReq > 0.0f) {
                    float clickRatio = (mouse.x - trackRect.x) / trackRect.width;
                    if (clickRatio < 0.0f) clickRatio = 0.0f;
                    if (clickRatio > 1.0f) clickRatio = 1.0f;
                    float desired = roundf(clickRatio * totalReq);

                    float totalStock = V_Ration_GetStock(army, (RationResource)r);
                    float otherAssigned = 0.0f;
                    for (int og = 0; og < RG_COUNT; og++) {
                        if (og != g) otherAssigned += state->draftPlan.amount[og][r];
                    }
                    float maxAvail = (totalStock >= otherAssigned) ? (totalStock - otherAssigned) : 0.0f;
                    if (desired > maxAvail) desired = maxAvail;
                    state->draftPlan.amount[g][r] = desired;
                }
            }

            float step = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) ? 5.0f : 1.0f;
            bool hoveredSlot = CheckCollisionPointRec(mouse, slot);

            /* Botón Menos (-) */
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, layout.groupMinusBtns[g][r])) {
                state->draftPlan.amount[g][r] -= step;
                if (state->draftPlan.amount[g][r] < 0.0f) state->draftPlan.amount[g][r] = 0.0f;
                Audio_PlaySfx(SFX_UI_CLICK);
            }

            /* Botón Más (+) */
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, layout.groupPlusBtns[g][r])) {
                float totalStock = V_Ration_GetStock(army, (RationResource)r);
                float assigned = 0.0f;
                for (int og = 0; og < RG_COUNT; og++) assigned += state->draftPlan.amount[og][r];
                float available = (totalStock >= assigned) ? (totalStock - assigned) : 0.0f;
                if (available > 0.0f) {
                    float toAdd = (available >= step) ? step : available;
                    state->draftPlan.amount[g][r] += toAdd;
                    Audio_PlaySfx(SFX_UI_CLICK);
                }
            }

            /* Rueda del ratón */
            if (hoveredSlot) {
                float wheel = GetMouseWheelMove();
                if (wheel > 0.1f) {
                    float totalStock = V_Ration_GetStock(army, (RationResource)r);
                    float assigned = 0.0f;
                    for (int og = 0; og < RG_COUNT; og++) assigned += state->draftPlan.amount[og][r];
                    if (totalStock > assigned) {
                        state->draftPlan.amount[g][r] += 1.0f;
                        Audio_PlaySfx(SFX_UI_CLICK);
                    }
                } else if (wheel < -0.1f) {
                    if (state->draftPlan.amount[g][r] > 0.0f) {
                        state->draftPlan.amount[g][r] -= 1.0f;
                        if (state->draftPlan.amount[g][r] < 0.0f) state->draftPlan.amount[g][r] = 0.0f;
                        Audio_PlaySfx(SFX_UI_CLICK);
                    }
                }
            }
        }
    }

    /* Control de Sacrificio de Ganado (+ / - y botón Sacrificar) */
    if (army->ganado > 0) {
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, layout.slaughterMinusBtn)) {
            if (state->cattleToSlaughter > 1) {
                state->cattleToSlaughter--;
                Audio_PlaySfx(SFX_UI_CLICK);
            }
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, layout.slaughterPlusBtn)) {
            if (state->cattleToSlaughter < army->ganado) {
                state->cattleToSlaughter++;
                Audio_PlaySfx(SFX_UI_CLICK);
            }
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, layout.slaughterActionBtn)) {
            Audio_PlaySfx(SFX_UI_CONFIRM);
            action.type = RATION_ACTION_SLAUGHTER_CATTLE;
            action.cattleHeads = state->cattleToSlaughter;
            return action;
        }
    }

    /* Asignar Automático (A) */
    if (IsKeyPressed(KEY_A) || (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, layout.btnAuto))) {
        state->draftPlan = V_Ration_Auto(army, &needs);
        Audio_PlaySfx(SFX_UI_CLICK);
    }

    /* Repetir de Ayer (R) */
    if (IsKeyPressed(KEY_R) || (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, layout.btnRepeat))) {
        state->draftPlan = V_Ration_Repeat(army);
        Audio_PlaySfx(SFX_UI_CLICK);
    }

    /* Reiniciar */
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, layout.btnReset)) {
        state->draftPlan = state->initialPlan;
        Audio_PlaySfx(SFX_UI_CLICK);
    }

    /* Validación del plan */
    state->isValid = V_Ration_Validate(army, &state->draftPlan, state->validationError, sizeof(state->validationError));

    /* Confirmar Reparto (ENTER) */
    if (state->isValid && (IsKeyPressed(KEY_ENTER) || (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, layout.btnConfirm)))) {
        Audio_PlaySfx(SFX_UI_CONFIRM);
        action.type = RATION_ACTION_SUBMIT_PLAN;
        action.plan = state->draftPlan;
        return action;
    }

    return action;
}

/* ============================================================
   DRAW (RENDERIZADO Y PRESENTACIÓN DE LA VISTA)
   ============================================================ */

void RationView_Draw(const RationViewState* state, const Army* army, const Map* map, HexCoord pos, int screenWidth, int screenHeight) {
    if (!state || !army) return;

    Vector2 mouse = GetMousePosition();
    RationViewLayout l;
    ComputeLayout(&l, screenWidth, screenHeight, state->isCompactMode, state->showingReport);

    /* Fondo modal oscurecido */
    DrawRectangle(0, 0, screenWidth, screenHeight, RATION_COLOR_BG_OVERLAY);

    /* 1. Página 2: Reporte del Día tras Confirmar */
    if (state->showingReport) {
        UI_DrawPanelStyled(l.reportBounds, UI_STYLE_PANEL_DARK);

        int dayNum = (state->lastReport.dayNumber > 0) ? state->lastReport.dayNumber : 1;
        UI_DrawText(TextFormat("< RESUMEN DEL CIERRE DE JORNADA (DIA %d) >", dayNum), (Vector2){ l.reportBounds.x + 24.0f, l.reportBounds.y + 14.0f }, 20, RATION_COLOR_TEXT_TITLE);

        int totalBajas = state->lastReport.deadInitiates + state->lastReport.deadSoldiers + state->lastReport.deadSergeants;
        int totalDesert = state->lastReport.desertedInitiates + state->lastReport.desertedSoldiers + state->lastReport.desertedSergeants;
        char subBuf[128];
        snprintf(subBuf, sizeof(subBuf), "Consumo de reservas: %.0f Comida, %.0f Agua  |  Bajas totales: %d muertos, %d desertores",
                 state->lastReport.foodConsumed, state->lastReport.waterConsumed, totalBajas, totalDesert);
        UI_DrawText(subBuf, (Vector2){ l.reportBounds.x + 24.0f, l.reportBounds.y + 38.0f }, 12, (totalBajas > 0 || totalDesert > 0) ? RATION_COLOR_STATUS_DANGER : RATION_COLOR_TEXT_MUTED);

        DrawLine((int)(l.reportBounds.x + 20), (int)(l.reportBounds.y + 56), (int)(l.reportBounds.x + l.reportBounds.width - 20), (int)(l.reportBounds.y + 56), Fade(RATION_COLOR_BRONZE_OLD, 0.6f));

        float cardTopY = l.reportBounds.y + 68.0f;
        float cardH = l.reportBounds.height - 145.0f;
        if (state->stragglersRegrouped) {
            Rectangle regroupChip = { l.reportBounds.x + 24.0f, l.reportBounds.y + 62.0f, l.reportBounds.width - 48.0f, 26.0f };
            UISkin_DrawGlow(regroupChip, (Color){ 120, 235, 110, 255 }, 0.45f, 0);
            UI_DrawStatusChip(regroupChip,
                              "[REAGRUPAMIENTO]: Los soldados dispersos regresan al campamento (+3 Iniciados, +2 Soldados, Salud reorganizada a 35+, +10 Moral).",
                              UI_STYLE_CHIP_OK, UI_ICON_MORAL);
            cardTopY += 28.0f;
            cardH -= 28.0f;
        }

        float cardW = (l.reportBounds.width - 48.0f - 30.0f) / 4.0f;

        for (int g = 0; g < RG_COUNT; g++) {
            Rectangle card = { l.reportBounds.x + 24.0f + (float)g * (cardW + 10.0f), cardTopY, cardW, cardH };
            UI_DrawPanelStyled(card, UI_STYLE_PANEL_DARK);

            UI_DrawText(s_groupNames[g], (Vector2){ card.x + 12.0f, card.y + 10.0f }, 16, RATION_COLOR_BRONZE_LIGHT);
            DrawLine((int)(card.x + 10), (int)(card.y + 32), (int)(card.x + cardW - 10), (int)(card.y + 32), Fade(RATION_COLOR_STONE_BORDER, 0.6f));

            GroupState gs = V_Ration_GetGroupState(army, (RationGroup)g);
            float curY = card.y + 40.0f;

            if (g != RG_GANADO) {
                UI_DrawLabeledBar((Rectangle){ card.x + 10.0f, curY, card.width - 20.0f, 18.0f }, gs.morale / 100.0f, TextFormat("Moral: %.0f%%", gs.morale), (gs.morale < 35.0f) ? RATION_COLOR_STATUS_DANGER : RATION_COLOR_MORAL, UI_ICON_MORAL);
                curY += 24.0f;
            }
            UI_DrawLabeledBar((Rectangle){ card.x + 10.0f, curY, card.width - 20.0f, 18.0f }, gs.health / 100.0f, TextFormat("Salud: %.0f%%", gs.health), (gs.health < 40.0f) ? RATION_COLOR_STATUS_DANGER : RATION_COLOR_HEALTH, UI_ICON_SALUD);
            curY += 24.0f;

            if (g == RG_SARGENTOS) {
                float sRatio = (gs.satisfactionMax > 0.0f) ? (gs.satisfaction / gs.satisfactionMax) : 0.0f;
                UI_DrawLabeledBar((Rectangle){ card.x + 10.0f, curY, card.width - 20.0f, 18.0f }, sRatio, TextFormat("Sat: %.0f/%.0f", gs.satisfaction, gs.satisfactionMax), RATION_COLOR_SATISFACTION, UI_ICON_SATISFACCION);
                curY += 26.0f;
            }

            float statusY = curY + 8.0f;
            if (gs.daysUnfed > 0) {
                UI_DrawIcon(UI_ICON_PELIGRO, (Vector2){ card.x + 10.0f, statusY - 2.0f }, 16.0f, WHITE);
                UI_DrawText(TextFormat("Dias sin raciones: %d", gs.daysUnfed), (Vector2){ card.x + 30.0f, statusY }, 11, RATION_COLOR_STATUS_DANGER);
            } else {
                UI_DrawIcon(UI_ICON_OK, (Vector2){ card.x + 10.0f, statusY - 2.0f }, 16.0f, WHITE);
                UI_DrawText("Suministro correcto", (Vector2){ card.x + 30.0f, statusY }, 11, RATION_COLOR_STATUS_GOOD);
            }
        }

        UI_DrawButtonStyledSfx(l.reportContinueBtn, "[ CONTINUAR AL NUEVO DIA (ENTER) ]", UI_STYLE_BTN_PRIMARY, false, "ui_confirm");
        return;
    }

    /* 2. Modo Compacto (Abundancia) */
    if (state->isCompactMode) {
        UI_DrawPanelStyled(l.compactBounds, UI_STYLE_PANEL_DARK);

        UI_DrawText("< PROVISIONES SUFICIENTES (ABUNDANCIA) >", (Vector2){ l.compactBounds.x + 24.0f, l.compactBounds.y + 20.0f }, 18, RATION_COLOR_TEXT_TITLE);
        UI_DrawTextWrapped("Las reservas del ejercito satisfacen integramente todas las raciones de la tropa sin faltantes.",
                           (Vector2){ l.compactBounds.x + 24.0f, l.compactBounds.y + 55.0f }, 13, l.compactBounds.width - 48.0f, RATION_COLOR_TEXT_MUTED);

        RationPreview prev = V_Ration_Preview(army, &state->draftPlan);
        float ry = l.compactBounds.y + 110.0f;
        for (int r = 0; r < RS_COUNT; r++) {
            char rBuf[64];
            snprintf(rBuf, sizeof(rBuf), "%s: %.0fu (%.1fd)", s_resourceNames[r], V_Ration_GetStock(army, (RationResource)r), prev.daysLeft[r]);
            Rectangle pill = { l.compactBounds.x + 24.0f + (float)r * 130.0f, ry, 120.0f, 36.0f };
            UI_DrawStatusChip(pill, rBuf, (prev.daysLeft[r] < 2.0f) ? UI_STYLE_CHIP_WARN : UI_STYLE_CHIP_OK, s_resourceIcons[r]);
        }

        UI_DrawButtonStyledSfx(l.compactConfirmBtn, "[ ENTER ] Confirmar Todo", UI_STYLE_BTN_PRIMARY, false, "ui_confirm");
        UI_DrawButtonStyledSfx(l.compactExpandBtn, "Ajustar Manualmente...", UI_STYLE_BTN_DEFAULT, false, "ui_click");
        return;
    }

    /* 3. Modal Completo de Reparto */
    UI_DrawPanelStyled(l.modalBounds, UI_STYLE_PANEL_DARK);

    /* Encabezado */
    const char* headTitle = "< CIERRE DEL DIA: REPARTO DE RACIONES Y PROVISIONES >";
    Vector2 htSz = UI_MeasureText(headTitle, RATION_FONT_MODAL_TITLE);
    UI_DrawText(headTitle, (Vector2){ l.modalBounds.x + (l.modalBounds.width - htSz.x) / 2.0f, l.headerRect.y + 2.0f }, RATION_FONT_MODAL_TITLE, RATION_COLOR_TEXT_TITLE);

    const char* headSub = state->stragglersRegrouped
        ? "[REAGRUPAMIENTO]: Los soldados dispersos regresan al campamento (+3 Iniciados, +2 Soldados, Salud reorganizada a 35+, +10 Moral)."
        : "Pulsa [ENTER] para confirmar el reparto automatico o gestiona manualmente (+/- o arrastrar).";
    Vector2 hsSz = UI_MeasureText(headSub, RATION_FONT_MODAL_SUB);
    UI_DrawText(headSub, (Vector2){ l.modalBounds.x + (l.modalBounds.width - hsSz.x) / 2.0f, l.headerRect.y + 26.0f }, RATION_FONT_MODAL_SUB,
                state->stragglersRegrouped ? RATION_COLOR_STATUS_GOOD : RATION_COLOR_TEXT_MUTED);

    /* Pilas de Suministros Superiores */
    RationPreview preview = V_Ration_Preview(army, &state->draftPlan);

    for (int r = 0; r < RS_COUNT; r++) {
        Rectangle cr = l.supplyCardRects[r];
        bool hov = CheckCollisionPointRec(mouse, cr);

        UI_DrawPanelStyled(cr, UI_STYLE_PANEL_DARK);
        if (hov) {
            UISkin_DrawHighlight(cr, 0.35f, 0);
        }

        /* Icono 16x16 escalado */
        UI_DrawIcon(s_resourceIcons[r], (Vector2){ cr.x + 10.0f, cr.y + 8.0f }, 16.0f, WHITE);

        /* Etiqueta y Stock Principal en fuente grande */
        float curStock = V_Ration_GetStock(army, (RationResource)r);
        char lblBuf[32];
        snprintf(lblBuf, sizeof(lblBuf), "%s:", s_resourceNames[r]);
        UI_DrawText(lblBuf, (Vector2){ cr.x + 30.0f, cr.y + 10.0f }, RATION_FONT_RESOURCE_LABEL, RATION_COLOR_TEXT_MAIN);

        Vector2 lblSz = UI_MeasureText(lblBuf, RATION_FONT_RESOURCE_LABEL);
        char stockNum[32];
        snprintf(stockNum, sizeof(stockNum), "%.0fu", curStock);
        UI_DrawText(stockNum, (Vector2){ cr.x + 30.0f + lblSz.x + 6.0f, cr.y + 7.0f }, RATION_FONT_STOCK_BIG, RATION_COLOR_TEXT_TITLE);

        /* Icono de advertencia si el stock restante es crítico (< 2 días) */
        if (preview.daysLeft[r] < 2.0f && curStock > 0.0f) {
            UI_DrawIcon(UI_ICON_AVISO, (Vector2){ cr.x + cr.width - 26.0f, cr.y + 8.0f }, 16.0f, WHITE);
        }

        /* Barra de progreso 9-slice de reserva (Stock consumido hoy vs restante) */
        Rectangle barTrack = { cr.x + 12.0f, cr.y + 28.0f, cr.width - 24.0f, 12.0f };
        float stockRatio = (curStock > 0.0f) ? (preview.stockAfter[r] / curStock) : 0.0f;
        UISkin_DrawBar(barTrack, stockRatio, s_resourceColors[r], (Color){ 190, 180, 160, 255 }, 1);

        /* Línea inferior: Resta y días */
        char remBuf[64];
        snprintf(remBuf, sizeof(remBuf), "Resta: %.0fu (%.1f dias)", preview.stockAfter[r], preview.daysLeft[r]);
        Color remCol = (preview.daysLeft[r] < 2.0f) ? RATION_COLOR_STATUS_WARN : RATION_COLOR_TEXT_MUTED;
        UI_DrawText(remBuf, (Vector2){ cr.x + 12.0f, cr.y + 44.0f }, 12, remCol);
    }

    /* Caja de Sacrificar Ganado (a la derecha de Carne) */
    UI_DrawPanelStyled(l.slaughterBoxRect, UI_STYLE_PANEL_DARK);
    UI_DrawIcon(UI_ICON_GANADO, (Vector2){ l.slaughterBoxRect.x + 10.0f, l.slaughterBoxRect.y + 8.0f }, 16.0f, WHITE);
    UI_DrawText(TextFormat("Sacrificar (%d disp):", army->ganado), (Vector2){ l.slaughterBoxRect.x + 30.0f, l.slaughterBoxRect.y + 10.0f }, 13, RATION_COLOR_BRONZE_LIGHT);

    UI_DrawButtonStyled(l.slaughterMinusBtn, "-", UI_STYLE_BTN_DEFAULT, (state->cattleToSlaughter <= 1 || army->ganado <= 0));

    char cabBuf[32];
    snprintf(cabBuf, sizeof(cabBuf), "%d cab", state->cattleToSlaughter);
    Vector2 cabSz = UI_MeasureText(cabBuf, 13);
    float cabMidX = (l.slaughterMinusBtn.x + l.slaughterMinusBtn.width + l.slaughterPlusBtn.x) / 2.0f;
    UI_DrawText(cabBuf, (Vector2){ cabMidX - cabSz.x / 2.0f, l.slaughterBoxRect.y + 38.0f }, 13, RATION_COLOR_TEXT_MAIN);

    UI_DrawButtonStyled(l.slaughterPlusBtn, "+", UI_STYLE_BTN_DEFAULT, (state->cattleToSlaughter >= army->ganado || army->ganado <= 0));

    UI_DrawButtonStyledSfx(l.slaughterActionBtn, "+Carne", UI_STYLE_BTN_DANGER, (army->ganado <= 0 || state->cattleToSlaughter <= 0), "ui_click");

    /* 4 Paneles de Grupo */
    RationNeeds needs = V_Ration_GetNeeds(army, map, pos);
    bool isHD = (screenHeight >= 900);

    for (int g = 0; g < RG_COUNT; g++) {
        Rectangle gp = l.groupPanelRects[g];
        bool isStarving = (preview.group[g].level == RATION_STARVING);

        /* Resplandor de peligro si el grupo está en hambruna crítica */
        if (isStarving) {
            UISkin_DrawGlow(gp, RATION_COLOR_STATUS_DANGER, 0.65f, 0);
        }

        /* Tarjeta 9-slice en piedra oscura con marco metálico */
        UI_DrawPanelStyled(gp, UI_STYLE_PANEL_DARK);

        /* Cabecera de grupo con separador */
        Rectangle gHead = { gp.x + 6.0f, gp.y + 6.0f, gp.width - 12.0f, RATION_GROUP_HEAD_HEIGHT - 4.0f };
        DrawLineEx((Vector2){ gHead.x + 4.0f, gHead.y + gHead.height }, (Vector2){ gHead.x + gHead.width - 4.0f, gHead.y + gHead.height }, 1.5f, Fade(RATION_COLOR_BRONZE_OLD, 0.7f));

        int troopCount = 0;
        if (g == RG_INICIADOS) troopCount = army->iniciados;
        else if (g == RG_SOLDADOS) troopCount = army->soldados;
        else if (g == RG_SARGENTOS) troopCount = army->sargentos;
        else if (g == RG_GANADO) troopCount = army->ganado;

        char gTitle[48];
        snprintf(gTitle, sizeof(gTitle), "%s (%d)", s_groupNames[g], troopCount);
        Vector2 gtSz = UI_MeasureText(gTitle, RATION_FONT_GROUP_TITLE);
        float gtX = gHead.x + (gHead.width - gtSz.x) / 2.0f;
        if (g == RG_GANADO) {
            UI_DrawIcon(UI_ICON_GANADO, (Vector2){ gtX - 20.0f, gHead.y + 6.0f }, 16.0f, WHITE);
        }
        UI_DrawText(gTitle, (Vector2){ gtX, gHead.y + 5.0f }, RATION_FONT_GROUP_TITLE, RATION_COLOR_TEXT_TITLE);

        GroupState gs = V_Ration_GetGroupState(army, (RationGroup)g);

        /* ============================================================
           SECCIÓN SUPERIOR: BARRAS DE MORAL, SALUD Y SATISFACCIÓN
           ============================================================ */
        float barStartY = gp.y + RATION_GROUP_HEAD_HEIGHT + 8.0f;
        float barW = gp.width - 24.0f;
        float barH = isHD ? RATION_BAR_HEIGHT_HD : RATION_BAR_HEIGHT_SD;
        float barGap = isHD ? RATION_BAR_GAP_HD : RATION_BAR_GAP_SD;
        float barY = barStartY;

        if (g != RG_GANADO) {
            /* Barra de Moral (9-slice + icono) */
            Rectangle mBar = { gp.x + 12.0f, barY, barW, barH };
            UI_DrawLabeledBar(mBar, gs.morale / 100.0f, TextFormat("Moral: %.0f%%", gs.morale), (gs.morale < 35.0f) ? RATION_COLOR_STATUS_DANGER : RATION_COLOR_MORAL, UI_ICON_MORAL);

            barY += barH + barGap;

            /* Barra de Salud (9-slice + icono) */
            Rectangle hBar = { gp.x + 12.0f, barY, barW, barH };
            UI_DrawLabeledBar(hBar, gs.health / 100.0f, TextFormat("Salud: %.0f%%", gs.health), (gs.health < 40.0f) ? RATION_COLOR_STATUS_DANGER : RATION_COLOR_HEALTH, UI_ICON_SALUD);

            barY += barH + barGap;

            /* Medidor de Satisfacción (Exclusivo Sargentos) */
            if (g == RG_SARGENTOS) {
                Rectangle sBar = { gp.x + 12.0f, barY, barW, barH };
                float sRatio = (gs.satisfactionMax > 0.0f) ? (gs.satisfaction / gs.satisfactionMax) : 0.0f;
                UI_DrawLabeledBar(sBar, sRatio, TextFormat("Satisfaccion: %.0f / %.0f", gs.satisfaction, gs.satisfactionMax), RATION_COLOR_SATISFACTION, UI_ICON_SATISFACCION);
            }
        } else {
            /* Ganado: Subtítulo y Barra de Salud */
            UI_DrawText("Animales de tiro, carga o abasto", (Vector2){ gp.x + 14.0f, barY + 2.0f }, 12, RATION_COLOR_TEXT_MUTED);
            barY += isHD ? 24.0f : 20.0f;

            Rectangle hBar = { gp.x + 12.0f, barY, barW, barH };
            UI_DrawLabeledBar(hBar, gs.health / 100.0f, TextFormat("Salud: %.0f%%", gs.health), (gs.health < 40.0f) ? RATION_COLOR_STATUS_DANGER : RATION_COLOR_HEALTH, UI_ICON_SALUD);
        }

        /* Divisor horizontal 1 */
        float div1Y = gp.y + (isHD ? 134.0f : 116.0f);
        DrawLine((int)(gp.x + 12), (int)div1Y, (int)(gp.x + gp.width - 12), (int)div1Y, Fade(RATION_COLOR_BRONZE_OLD, 0.4f));

        /* ============================================================
           SECCIÓN MEDIA: SELECTORES DE RECURSOS COMO BARRAS DE RELLENO
           ============================================================ */
        float labelW = isHD ? 78.0f : 66.0f;

        if (g == RG_GANADO) {
            /* GANADO: AGUA REQUERIDA (SLIDER INTERACTIVO) + PASTOREO NATURAL */
            float drinkGot = state->draftPlan.amount[g][RS_AGUA];
            float dRatio = (needs.drink[g] > 0.0f) ? (drinkGot / needs.drink[g]) : 1.0f;
            Color drinkCol = (dRatio >= 0.99f) ? RATION_COLOR_STATUS_GOOD : (dRatio > 0.01f ? RATION_COLOR_STATUS_WARN : RATION_COLOR_STATUS_DANGER);
            UI_DrawText(TextFormat("Agua Requerida: %.0f / %.0fu", drinkGot, needs.drink[g]), (Vector2){ gp.x + 12.0f, div1Y + 7.0f }, 13, drinkCol);

            /* Slot / Slider de Agua */
            Rectangle sRect = l.groupResSlots[g][RS_AGUA];
            float btnW = l.groupPlusBtns[g][RS_AGUA].width;
            float trackX = sRect.x + labelW;
            float trackW = sRect.width - labelW - (btnW * 2.0f + 8.0f);
            Rectangle trackRect = { trackX, sRect.y + 4.0f, trackW, sRect.height - 8.0f };

            /* Icono 16x16 y etiqueta */
            UI_DrawIcon(UI_ICON_AGUA, (Vector2){ sRect.x + 2.0f, sRect.y + (sRect.height - 16.0f) / 2.0f }, 16.0f, WHITE);
            UI_DrawText(s_resourceNames[RS_AGUA], (Vector2){ sRect.x + 20.0f, sRect.y + (sRect.height - 13.0f) / 2.0f }, RATION_FONT_RESOURCE_LABEL, RATION_COLOR_TEXT_MAIN);

            /* Pista interactiva 9-slice */
            float fillRatio = (needs.drink[g] > 0.0f) ? (state->draftPlan.amount[g][RS_AGUA] / needs.drink[g]) : 0.0f;
            char amtStr[32];
            snprintf(amtStr, sizeof(amtStr), "%.0fu", state->draftPlan.amount[g][RS_AGUA]);
            UI_DrawLabeledBar(trackRect, fillRatio, amtStr, s_resourceColors[RS_AGUA], UI_ICON_NONE);

            if (CheckCollisionPointRec(mouse, trackRect)) {
                UISkin_DrawHighlight(trackRect, 0.35f, 1);
            }

            /* Botones - y + */
            float totalStock = V_Ration_GetStock(army, RS_AGUA);
            float assigned = 0.0f;
            for (int og = 0; og < RG_COUNT; og++) assigned += state->draftPlan.amount[og][RS_AGUA];
            bool canAdd = (totalStock > assigned);

            UI_DrawButtonStyled(l.groupMinusBtns[g][RS_AGUA], "-", UI_STYLE_BTN_DEFAULT, (state->draftPlan.amount[g][RS_AGUA] <= 0.0f));
            UI_DrawButtonStyled(l.groupPlusBtns[g][RS_AGUA], "+", UI_STYLE_BTN_DEFAULT, !canAdd);

            /* Divisor entre Agua y Pastoreo */
            float divPastureY = sRect.y + sRect.height + 8.0f;
            DrawLine((int)(gp.x + 12), (int)divPastureY, (int)(gp.x + gp.width - 12), (int)divPastureY, Fade(RATION_COLOR_BRONZE_OLD, 0.4f));

            /* Tarjeta de Pastoreo Natural con ilustración */
            float div3Y_ref = gp.y + (isHD ? 382.0f : 326.0f);
            float pastureY = divPastureY + 8.0f;
            float pastureH = div3Y_ref - pastureY - 6.0f;
            Rectangle pastureCard = { gp.x + 12.0f, pastureY, gp.width - 24.0f, pastureH };
            DrawPastureGrass(pastureCard);
        } else {
            /* HUMANOS: BEBIDA (AGUA Y VINO) Y COMIDA (COMIDA Y CARNE) */
            float drinkGot = state->draftPlan.amount[g][RS_AGUA] + state->draftPlan.amount[g][RS_VINO];
            float dRatio = (needs.drink[g] > 0.0f) ? (drinkGot / needs.drink[g]) : 1.0f;
            Color drinkCol = (dRatio >= 0.99f) ? RATION_COLOR_STATUS_GOOD : (dRatio > 0.01f ? RATION_COLOR_STATUS_WARN : RATION_COLOR_STATUS_DANGER);
            UI_DrawText(TextFormat("Bebida: %.0f / %.0fu", drinkGot, needs.drink[g]), (Vector2){ gp.x + 12.0f, div1Y + 7.0f }, 13, drinkCol);

            /* Sliders de Agua y Vino */
            for (int r = 0; r < 2; r++) {
                Rectangle sRect = l.groupResSlots[g][r];
                float btnW = l.groupPlusBtns[g][r].width;
                float trackX = sRect.x + labelW;
                float trackW = sRect.width - labelW - (btnW * 2.0f + 8.0f);
                Rectangle trackRect = { trackX, sRect.y + 4.0f, trackW, sRect.height - 8.0f };

                UI_DrawIcon(s_resourceIcons[r], (Vector2){ sRect.x + 2.0f, sRect.y + (sRect.height - 16.0f) / 2.0f }, 16.0f, WHITE);
                UI_DrawText(s_resourceNames[r], (Vector2){ sRect.x + 20.0f, sRect.y + (sRect.height - 13.0f) / 2.0f }, RATION_FONT_RESOURCE_LABEL, RATION_COLOR_TEXT_MAIN);

                float fillRatio = (needs.drink[g] > 0.0f) ? (state->draftPlan.amount[g][r] / needs.drink[g]) : 0.0f;
                char amtStr[32];
                snprintf(amtStr, sizeof(amtStr), "%.0fu", state->draftPlan.amount[g][r]);
                UI_DrawLabeledBar(trackRect, fillRatio, amtStr, s_resourceColors[r], UI_ICON_NONE);

                if (CheckCollisionPointRec(mouse, trackRect)) {
                    UISkin_DrawHighlight(trackRect, 0.35f, 1);
                }

                float totalStock = V_Ration_GetStock(army, (RationResource)r);
                float assigned = 0.0f;
                for (int og = 0; og < RG_COUNT; og++) assigned += state->draftPlan.amount[og][r];
                bool canAdd = (totalStock > assigned);

                UI_DrawButtonStyled(l.groupMinusBtns[g][r], "-", UI_STYLE_BTN_DEFAULT, (state->draftPlan.amount[g][r] <= 0.0f));
                UI_DrawButtonStyled(l.groupPlusBtns[g][r], "+", UI_STYLE_BTN_DEFAULT, !canAdd);
            }

            /* Divisor horizontal 2 */
            float div2Y = gp.y + (isHD ? 258.0f : 221.0f);
            DrawLine((int)(gp.x + 12), (int)div2Y, (int)(gp.x + gp.width - 12), (int)div2Y, Fade(RATION_COLOR_BRONZE_OLD, 0.4f));

            /* Resumen de necesidades y asignación de Comida */
            float foodGot = state->draftPlan.amount[g][RS_COMIDA] + state->draftPlan.amount[g][RS_CARNE];
            float fRatio = (needs.food[g] > 0.0f) ? (foodGot / needs.food[g]) : 1.0f;
            Color foodCol = (fRatio >= 0.99f) ? RATION_COLOR_STATUS_GOOD : (fRatio > 0.01f ? RATION_COLOR_STATUS_WARN : RATION_COLOR_STATUS_DANGER);
            UI_DrawText(TextFormat("Comida: %.0f / %.0fu", foodGot, needs.food[g]), (Vector2){ gp.x + 12.0f, div2Y + 7.0f }, 13, foodCol);

            /* Sliders de Comida y Carne */
            for (int r = 2; r < RS_COUNT; r++) {
                Rectangle sRect = l.groupResSlots[g][r];
                float btnW = l.groupPlusBtns[g][r].width;
                float trackX = sRect.x + labelW;
                float trackW = sRect.width - labelW - (btnW * 2.0f + 8.0f);
                Rectangle trackRect = { trackX, sRect.y + 4.0f, trackW, sRect.height - 8.0f };

                UI_DrawIcon(s_resourceIcons[r], (Vector2){ sRect.x + 2.0f, sRect.y + (sRect.height - 16.0f) / 2.0f }, 16.0f, WHITE);
                UI_DrawText(s_resourceNames[r], (Vector2){ sRect.x + 20.0f, sRect.y + (sRect.height - 13.0f) / 2.0f }, RATION_FONT_RESOURCE_LABEL, RATION_COLOR_TEXT_MAIN);

                float fillRatio = (needs.food[g] > 0.0f) ? (state->draftPlan.amount[g][r] / needs.food[g]) : 0.0f;
                char amtStr[32];
                snprintf(amtStr, sizeof(amtStr), "%.0fu", state->draftPlan.amount[g][r]);
                UI_DrawLabeledBar(trackRect, fillRatio, amtStr, s_resourceColors[r], UI_ICON_NONE);

                if (CheckCollisionPointRec(mouse, trackRect)) {
                    UISkin_DrawHighlight(trackRect, 0.35f, 1);
                }

                float totalStock = V_Ration_GetStock(army, (RationResource)r);
                float assigned = 0.0f;
                for (int og = 0; og < RG_COUNT; og++) assigned += state->draftPlan.amount[og][r];
                bool canAdd = (totalStock > assigned);

                UI_DrawButtonStyled(l.groupMinusBtns[g][r], "-", UI_STYLE_BTN_DEFAULT, (state->draftPlan.amount[g][r] <= 0.0f));
                UI_DrawButtonStyled(l.groupPlusBtns[g][r], "+", UI_STYLE_BTN_DEFAULT, !canAdd);
            }
        }

        /* Divisor antes del estandarte */
        float div3Y = gp.y + (isHD ? 382.0f : 326.0f);
        DrawLine((int)(gp.x + 12), (int)div3Y, (int)(gp.x + gp.width - 12), (int)div3Y, Fade(RATION_COLOR_BRONZE_OLD, 0.4f));

        /* ============================================================
           SECCIÓN INFERIOR: CHIP / SELLO DE ESTADO (9-SLICE UI_SKIN_CHIP)
           ============================================================ */
        const char* bannerTxt = "RACIÓN COMPLETA";
        UIChipStyle chipStyle = UI_STYLE_CHIP_OK;
        UIIconId chipIcon = UI_ICON_OK;

        if (preview.group[g].level == RATION_REDUCED) {
            bannerTxt = "RACIÓN REDUCIDA";
            chipStyle = UI_STYLE_CHIP_WARN;
            chipIcon = UI_ICON_AVISO;
        } else if (preview.group[g].level == RATION_STARVING) {
            bannerTxt = "HAMBRUNA CRÍTICA";
            chipStyle = UI_STYLE_CHIP_BAD;
            chipIcon = UI_ICON_PELIGRO;
        }

        float bannerH = isHD ? RATION_BANNER_HEIGHT_HD : RATION_BANNER_HEIGHT_SD;
        Rectangle bannerRect = { gp.x + 12.0f, div3Y + 8.0f, gp.width - 24.0f, bannerH };
        UI_DrawStatusChip(bannerRect, bannerTxt, chipStyle, chipIcon);

        /* ============================================================
           CAJA RECESADA (SLOT 9-SLICE): PRONÓSTICO DIARIO
           ============================================================ */
        float foreH = isHD ? RATION_FORECAST_HEIGHT_HD : RATION_FORECAST_HEIGHT_SD;
        Rectangle foreBox = { gp.x + 12.0f, bannerRect.y + bannerRect.height + 6.0f, gp.width - 24.0f, foreH };
        UI_DrawSlot(foreBox, (Color){0, 0, 0, 0}, UI_ICON_NONE);

        UI_DrawText("PRONÓSTICO DIARIO", (Vector2){ foreBox.x + 10.0f, foreBox.y + 5.0f }, RATION_FONT_FORECAST_HEAD, RATION_COLOR_BRONZE_LIGHT);

        char foreBuf[96];
        if (g == RG_GANADO) {
            snprintf(foreBuf, sizeof(foreBuf), "Salud esperada: %+.0f%%", preview.group[g].healthDelta);
        } else if (g == RG_SARGENTOS) {
            snprintf(foreBuf, sizeof(foreBuf), "Moral:%+.0f%%  Salud:%+.0f%%  Sat:%+.0f", preview.group[g].moraleDelta, preview.group[g].healthDelta, preview.group[g].satisfactionDelta);
        } else {
            snprintf(foreBuf, sizeof(foreBuf), "Moral: %+.0f%%   Salud: %+.0f%%", preview.group[g].moraleDelta, preview.group[g].healthDelta);
        }
        UI_DrawText(foreBuf, (Vector2){ foreBox.x + 10.0f, foreBox.y + 20.0f }, RATION_FONT_FORECAST_TEXT, RATION_COLOR_TEXT_MAIN);

        float iconY = foreBox.y + 37.0f;
        if (preview.group[g].risksDesertion) {
            UI_DrawIcon(UI_ICON_PELIGRO, (Vector2){ foreBox.x + 10.0f, iconY - 3.0f }, 16.0f, WHITE);
            const char* warn = (g == RG_GANADO) ? "Riesgo de deshidratacion" : "Riesgo de desercion";
            UI_DrawText(warn, (Vector2){ foreBox.x + 30.0f, iconY }, 11, RATION_COLOR_STATUS_DANGER);
        } else {
            UI_DrawIcon(UI_ICON_OK, (Vector2){ foreBox.x + 10.0f, iconY - 3.0f }, 16.0f, WHITE);
            const char* ok = (g == RG_GANADO) ? "Rebano con pastoreo pleno" : "Tropa estable y satisfecha";
            UI_DrawText(ok, (Vector2){ foreBox.x + 30.0f, iconY }, 11, RATION_COLOR_STATUS_GOOD);
        }
    }

    /* Pie de Página */
    UI_DrawPanelStyled(l.footerRect, UI_STYLE_PANEL_DARK);

    /* Mensaje de validación con icono 16x16 */
    float footMidY = l.footerRect.y + l.footerRect.height / 2.0f;
    if (!state->isValid) {
        UI_DrawIcon(UI_ICON_PELIGRO, (Vector2){ l.footerRect.x + 14.0f, footMidY - 8.0f }, 16.0f, WHITE);
        UI_DrawText(TextFormat("ERROR: %s", state->validationError), (Vector2){ l.footerRect.x + 36.0f, footMidY - 7.0f }, RATION_FONT_FOOTER, RATION_COLOR_STATUS_DANGER);
    } else {
        UI_DrawIcon(UI_ICON_OK, (Vector2){ l.footerRect.x + 14.0f, footMidY - 8.0f }, 16.0f, WHITE);
        UI_DrawText("Plan equilibrado y validado. Listo para confirmar.", (Vector2){ l.footerRect.x + 36.0f, footMidY - 7.0f }, RATION_FONT_FOOTER, RATION_COLOR_STATUS_GOOD);
    }

    /* Botones del Pie */
    UI_DrawButtonStyledSfx(l.btnAuto, "[A] Reparto Auto", UI_STYLE_BTN_DEFAULT, false, "ui_click");
    UI_DrawButtonStyledSfx(l.btnRepeat, "[R] Repetir Ayer", UI_STYLE_BTN_DEFAULT, false, "ui_click");
    UI_DrawButtonStyledSfx(l.btnConfirm, "[ENTER] Confirmar Todo", UI_STYLE_BTN_PRIMARY, !state->isValid, "ui_confirm");

    /* Dibuja elemento flotante arrastrado si está activo */
    UIDragDrop_DrawFloating(&state->dragDrop);
}
