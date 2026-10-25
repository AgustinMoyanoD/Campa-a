#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "raylib.h"
#include "presentation/ui_widgets.h"
#include "views/ration_mock.h"
#include "views/ration_view.h"

/* ============================================================
   HELPER DE GEOMETRÍA: DETECCIÓN DE SOLAPAMIENTO DE RECTÁNGULOS
   ============================================================ */

static bool RectanglesOverlap(Rectangle a, Rectangle b) {
    if (a.x + a.width <= b.x || b.x + b.width <= a.x) return false;
    if (a.y + a.height <= b.y || b.y + b.height <= a.y) return false;
    return true;
}

static bool RectangleContains(Rectangle outer, Rectangle inner) {
    return (inner.x >= outer.x - 0.01f &&
            inner.y >= outer.y - 0.01f &&
            inner.x + inner.width <= outer.x + outer.width + 0.01f &&
            inner.y + inner.height <= outer.y + outer.height + 0.01f);
}

/* ============================================================
   TEST 1: MÁQUINA DE ESTADOS DEL ARRASTRE (UI_DragDrop)
   ============================================================ */

static void Test_DragDrop_StateMachine(void) {
    printf("[TEST] UIDragDrop State Machine...\n");

    UIDragDrop dd;
    UIDragDrop_Init(&dd);
    assert(!dd.active);
    assert(dd.sourceGroup == -1);
    assert(dd.amount == 0.0f);

    /* 1. Intento de inicio inválido con cantidad 0 */
    bool okStart0 = UIDragDrop_Start(&dd, 0, -1, 0.0f, (Vector2){100, 100}, "Agua", BLUE);
    assert(!okStart0);
    assert(!dd.active);

    /* 2. Inicio válido desde pila general */
    bool okStart = UIDragDrop_Start(&dd, RS_AGUA, -1, 5.0f, (Vector2){100, 100}, "Agua", BLUE);
    assert(okStart);
    assert(dd.active);
    assert(dd.sourceKind == RS_AGUA);
    assert(dd.sourceGroup == -1);
    assert(dd.amount == 5.0f);
    assert(strcmp(dd.label, "Agua") == 0);

    /* 3. Actualización de posición */
    UIDragDrop_Update(&dd, (Vector2){150, 200});
    assert(dd.active);
    assert(dd.currentPos.x == 150 && dd.currentPos.y == 200);

    /* 4. Cancelación */
    UIDragDrop_Cancel(&dd);
    assert(!dd.active);
    assert(dd.amount == 0.0f);

    /* 5. Inicio desde grupo y finalización (drop) */
    okStart = UIDragDrop_Start(&dd, RS_VINO, RG_SARGENTOS, 2.0f, (Vector2){200, 250}, "Vino", RED);
    assert(okStart);
    assert(dd.active);
    assert(dd.sourceKind == RS_VINO);
    assert(dd.sourceGroup == RG_SARGENTOS);

    int outKind = -1, outGroup = -1;
    float outAmt = 0.0f;
    bool finished = UIDragDrop_Finish(&dd, &outKind, &outGroup, &outAmt);
    assert(finished);
    assert(outKind == RS_VINO);
    assert(outGroup == RG_SARGENTOS);
    assert(outAmt == 2.0f);
    assert(!dd.active);

    /* 6. Intento de finish cuando está inactivo */
    assert(!UIDragDrop_Finish(&dd, NULL, NULL, NULL));

    printf("  -> OK: Inicio, movimiento, cancelacion y entrega verificados.\n");
}

/* ============================================================
   TEST 2: LAYOUT NO-SUPERPUESTO EN 1280x720 Y 1600x900
   ============================================================ */

/* Replicamos la función interna de cálculo de layout para prueba aislada */
typedef struct {
    Rectangle modalBounds;
    Rectangle headerRect;
    Rectangle supplyCardRects[RS_COUNT];
    Rectangle slaughterBoxRect;
    Rectangle slaughterMinusBtn;
    Rectangle slaughterPlusBtn;
    Rectangle slaughterActionBtn;
    Rectangle groupPanelRects[RG_COUNT];
    Rectangle groupResSlots[RG_COUNT][RS_COUNT];
    Rectangle groupMinusBtns[RG_COUNT][RS_COUNT];
    Rectangle groupPlusBtns[RG_COUNT][RS_COUNT];
    Rectangle footerRect;
    Rectangle btnAuto;
    Rectangle btnRepeat;
    Rectangle btnConfirm;
} TestLayout;

static void ComputeTestLayout(TestLayout* l, int sw, int sh) {
    float pad = (sw >= 1600) ? 30.0f : 16.0f;
    float mw = (sw >= 1600) ? 1340.0f : ((sw >= 1280) ? 1160.0f : ((float)sw - (pad * 2.0f)));
    float mh = (sh >= 900)  ? 720.0f  : ((sh >= 720)  ? 640.0f : ((float)sh - (pad * 2.0f)));
    float mx = ((float)sw - mw) / 2.0f;
    float my = ((float)sh - mh) / 2.0f;
    l->modalBounds = (Rectangle){ mx, my, mw, mh };
    l->headerRect = (Rectangle){ mx + 16.0f, my + 10.0f, mw - 32.0f, 40.0f };

    float topY = my + 52.0f;
    float cardW = (mw - 32.0f - 40.0f) / 5.0f;
    float cardH = 64.0f;

    for (int r = 0; r < RS_COUNT; r++) {
        l->supplyCardRects[r] = (Rectangle){ mx + 16.0f + (float)r * (cardW + 10.0f), topY, cardW, cardH };
    }

    float slX = mx + 16.0f + 4.0f * (cardW + 10.0f);
    float slW = cardW;
    l->slaughterBoxRect = (Rectangle){ slX, topY, slW, cardH };
    l->slaughterActionBtn = (Rectangle){ slX + slW - 54.0f, topY + 7.0f,  46.0f, 24.0f };
    l->slaughterMinusBtn  = (Rectangle){ slX + 8.0f,        topY + 33.0f, 26.0f, 26.0f };
    l->slaughterPlusBtn   = (Rectangle){ slX + slW - 34.0f, topY + 33.0f, 26.0f, 26.0f };

    float groupY = topY + cardH + 10.0f;
    float footerH = 58.0f;
    float groupH = mh - (groupY - my) - footerH - 12.0f;
    float groupW = (mw - 32.0f - 30.0f) / 4.0f;

    for (int g = 0; g < RG_COUNT; g++) {
        float gx = mx + 16.0f + (float)g * (groupW + 10.0f);
        l->groupPanelRects[g] = (Rectangle){ gx, groupY, groupW, groupH };

        float slotH = (sh >= 900) ? 36.0f : 28.0f;
        float btnW = (sh >= 900) ? 28.0f : 24.0f;
        float btnH = (sh >= 900) ? 28.0f : 24.0f;
        float drinkSlotStartY = (sh >= 900) ? (groupY + 158.0f) : (groupY + 120.0f);
        float foodSlotStartY  = (sh >= 900) ? (groupY + 276.0f) : (groupY + 218.0f);
        float slotGap = (sh >= 900) ? 8.0f : 6.0f;

        for (int r = 0; r < RS_COUNT; r++) {
            float sy = (r < 2) ? (drinkSlotStartY + (float)r * (slotH + slotGap))
                               : (foodSlotStartY  + (float)(r - 2) * (slotH + slotGap));
            l->groupResSlots[g][r] = (Rectangle){ gx + 10.0f, sy, groupW - 20.0f, slotH };
            l->groupPlusBtns[g][r]  = (Rectangle){ gx + groupW - 10.0f - btnW, sy + (slotH - btnH) / 2.0f, btnW, btnH };
            l->groupMinusBtns[g][r] = (Rectangle){ l->groupPlusBtns[g][r].x - btnW - 4.0f, sy + (slotH - btnH) / 2.0f, btnW, btnH };
        }
    }

    float footY = my + mh - footerH - 6.0f;
    l->footerRect = (Rectangle){ mx + 16.0f, footY, mw - 32.0f, footerH };

    float btnH = 40.0f;
    float by = footY + (footerH - btnH) / 2.0f;
    float btnW = (sw >= 1600) ? 170.0f : 150.0f;
    float confirmW = (sw >= 1600) ? 260.0f : 230.0f;

    l->btnConfirm = (Rectangle){ mx + mw - 16.0f - confirmW, by, confirmW, btnH };
    l->btnRepeat  = (Rectangle){ l->btnConfirm.x - btnW - 12.0f, by, btnW, btnH };
    l->btnAuto    = (Rectangle){ l->btnRepeat.x - btnW - 12.0f, by, btnW, btnH };
}

static void Test_Layout_Resolution(int sw, int sh) {
    printf("[TEST] Layout Verification at %dx%d...\n", sw, sh);

    TestLayout l;
    ComputeTestLayout(&l, sw, sh);

    /* 1. Modal contenido en la pantalla */
    assert(RectangleContains((Rectangle){0, 0, (float)sw, (float)sh}, l.modalBounds));

    /* 2. Pilas de suministros no se solapan entre sí */
    for (int i = 0; i < RS_COUNT; i++) {
        assert(RectangleContains(l.modalBounds, l.supplyCardRects[i]));
        for (int j = i + 1; j < RS_COUNT; j++) {
            assert(!RectanglesOverlap(l.supplyCardRects[i], l.supplyCardRects[j]));
        }
        /* No se solapan con la caja de sacrificio */
        assert(!RectanglesOverlap(l.supplyCardRects[i], l.slaughterBoxRect));
    }

    /* 3. Botones de caja de sacrificio contenidos y sin solapamiento */
    assert(RectangleContains(l.slaughterBoxRect, l.slaughterMinusBtn));
    assert(RectangleContains(l.slaughterBoxRect, l.slaughterPlusBtn));
    assert(RectangleContains(l.slaughterBoxRect, l.slaughterActionBtn));
    assert(!RectanglesOverlap(l.slaughterMinusBtn, l.slaughterPlusBtn));
    assert(!RectanglesOverlap(l.slaughterPlusBtn, l.slaughterActionBtn));

    /* 4. Paneles de grupo no se solapan entre sí */
    for (int i = 0; i < RG_COUNT; i++) {
        assert(RectangleContains(l.modalBounds, l.groupPanelRects[i]));
        for (int j = i + 1; j < RG_COUNT; j++) {
            assert(!RectanglesOverlap(l.groupPanelRects[i], l.groupPanelRects[j]));
        }
        /* No se solapan con las pilas superiores ni el footer */
        assert(!RectanglesOverlap(l.groupPanelRects[i], l.slaughterBoxRect));
        assert(!RectanglesOverlap(l.groupPanelRects[i], l.footerRect));

        /* Slots internos no se solapan entre sí */
        for (int r1 = 0; r1 < RS_COUNT; r1++) {
            assert(RectangleContains(l.groupPanelRects[i], l.groupResSlots[i][r1]));
            for (int r2 = r1 + 1; r2 < RS_COUNT; r2++) {
                assert(!RectanglesOverlap(l.groupResSlots[i][r1], l.groupResSlots[i][r2]));
            }
            /* Botones +/- contenidos y sin solapamiento */
            assert(RectangleContains(l.groupResSlots[i][r1], l.groupMinusBtns[i][r1]));
            assert(RectangleContains(l.groupResSlots[i][r1], l.groupPlusBtns[i][r1]));
            assert(!RectanglesOverlap(l.groupMinusBtns[i][r1], l.groupPlusBtns[i][r1]));
        }
    }

    /* 5. Botones del footer no se solapan */
    assert(!RectanglesOverlap(l.btnAuto, l.btnRepeat));
    assert(!RectanglesOverlap(l.btnRepeat, l.btnConfirm));

    assert(RectangleContains(l.modalBounds, l.btnAuto));
    assert(RectangleContains(l.modalBounds, l.btnConfirm));

    printf("  -> OK: Sin solapamientos en %dx%d (slots y botones internos verificados).\n", sw, sh);
}

/* ============================================================
   TEST 3: ESCENARIOS DEL MOCK DE RACIONES
   ============================================================ */

static void Test_RationMock_Scenarios(void) {
    printf("[TEST] Ration Mock Scenarios...\n");

    /* Escenario 1: Abundancia */
    RationMock_SetScenario(MOCK_SCENARIO_ABUNDANCE);
    assert(!RationMock_NeedsDecision(NULL, NULL, (HexCoord){0,0}));
    RationNeeds nAbund = RationMock_GetNeeds(NULL, NULL, (HexCoord){0,0});
    RationPlan pAbund = RationMock_Auto(NULL, &nAbund);
    char err[64];
    assert(RationMock_Validate(NULL, &pAbund, err, sizeof(err)));
    RationPreview prAbund = RationMock_Preview(NULL, &pAbund);
    for (int g = 0; g < RG_COUNT; g++) {
        assert(prAbund.group[g].level == RATION_FULL);
        assert(!prAbund.group[g].risksDesertion);
    }

    /* Escenario 2: Faltante */
    RationMock_SetScenario(MOCK_SCENARIO_SHORTAGE);
    assert(RationMock_NeedsDecision(NULL, NULL, (HexCoord){0,0}));

    /* Escenario 3: Hambre en grupo */
    RationMock_SetScenario(MOCK_SCENARIO_STARVING_GROUP);
    assert(RationMock_NeedsDecision(NULL, NULL, (HexCoord){0,0}));
    GroupState stIni = RationMock_GetGroupState(NULL, RG_INICIADOS);
    assert(stIni.daysUnfed == 2);
    assert(stIni.morale <= 25.0f);

    /* Escenario 4: Satisfacción cero en Sargentos */
    RationMock_SetScenario(MOCK_SCENARIO_ZERO_SATISFACTION);
    GroupState stSgt = RationMock_GetGroupState(NULL, RG_SARGENTOS);
    assert(stSgt.satisfaction == 0.0f);

    /* Validaciones negativas del contrato */
    RationPlan invalidPlan = pAbund;
    invalidPlan.amount[RG_GANADO][RS_VINO] = 5.0f; /* Ganado no toma vino */
    assert(!RationMock_Validate(NULL, &invalidPlan, err, sizeof(err)));

    invalidPlan = pAbund;
    invalidPlan.amount[RG_GANADO][RS_COMIDA] = 5.0f; /* Ganado no consume comida de inventario */
    assert(!RationMock_Validate(NULL, &invalidPlan, err, sizeof(err)));

    invalidPlan = pAbund;
    invalidPlan.amount[RG_SOLDADOS][RS_AGUA] = 9999.0f; /* Excede stock */
    assert(!RationMock_Validate(NULL, &invalidPlan, err, sizeof(err)));

    printf("  -> OK: Escenarios de abundancia, faltante, hambruna y validaciones verificados.\n");
}

/* ============================================================
   MAIN
   ============================================================ */

int main(void) {
    printf("==================================================\n");
    printf("EJECUTANDO PRUEBAS UNITARIAS DE UI DE RACIONES\n");
    printf("==================================================\n");

    Test_DragDrop_StateMachine();
    Test_Layout_Resolution(1280, 720);
    Test_Layout_Resolution(1600, 900);
    Test_Layout_Resolution(1024, 768);
    Test_RationMock_Scenarios();

    printf("==================================================\n");
    printf("[EXITO TOTAL] Todas las pruebas de UI pasaron.\n");
    printf("==================================================\n");
    return 0;
}
