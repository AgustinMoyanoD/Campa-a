#include "presentation/ui_cards.h"
#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"
#include "presentation/ui_skin.h"
#include "models/modifier.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static Texture2D s_texCardDivine = {0};
static Texture2D s_texCardCurse = {0};
static bool s_cardTexturesLoaded = false;

void UICards_Init(void) {
    if (!IsWindowReady()) return;

    const char* pDivine = "assets/modifiers_placeholder/card_frame_divine.png";
    const char* pCurse  = "assets/modifiers_placeholder/card_frame_curse.png";
    if (!FileExists(pDivine)) {
        pDivine = TextFormat("%sassets/modifiers_placeholder/card_frame_divine.png", GetApplicationDirectory());
    }
    if (!FileExists(pCurse)) {
        pCurse = TextFormat("%sassets/modifiers_placeholder/card_frame_curse.png", GetApplicationDirectory());
    }
    if (FileExists(pDivine)) {
        s_texCardDivine = LoadTexture(pDivine);
        SetTextureFilter(s_texCardDivine, TEXTURE_FILTER_BILINEAR);
    }
    if (FileExists(pCurse)) {
        s_texCardCurse = LoadTexture(pCurse);
        SetTextureFilter(s_texCardCurse, TEXTURE_FILTER_BILINEAR);
    }
    if (s_texCardDivine.id > 0 && s_texCardCurse.id > 0) {
        s_cardTexturesLoaded = true;
        TraceLog(LOG_INFO, "[UICards] Marcos de cartas cargados exitosamente (512x768).");
    }
}

void UICards_Shutdown(void) {
    if (IsWindowReady()) {
        if (s_texCardDivine.id > 0) {
            UnloadTexture(s_texCardDivine);
            s_texCardDivine = (Texture2D){0};
        }
        if (s_texCardCurse.id > 0) {
            UnloadTexture(s_texCardCurse);
            s_texCardCurse = (Texture2D){0};
        }
    }
    s_cardTexturesLoaded = false;
}

static void UI_DrawCardIllustration(const ModifierDef* def, Rectangle box, bool isCurse) {
    BeginScissorMode((int)box.x, (int)box.y, (int)box.width, (int)box.height);
    DrawRectangleRec(box, isCurse ? (Color){ 20, 12, 28, 255 } : (Color){ 18, 22, 34, 255 });

    float cx = box.x + box.width / 2.0f;
    float cy = box.y + box.height / 2.0f;
    float rGrad = box.height * 0.46f;
    const char* src = def->source;

    if (isCurse) {
        DrawCircleGradient((Vector2){ cx, cy }, rGrad, (Color){ 75, 18, 95, 220 }, (Color){ 16, 8, 24, 255 });
        DrawCircle((int)cx, (int)(cy - 8.0f), box.height * 0.24f, (Color){ 45, 15, 60, 255 });
        DrawCircleLines((int)cx, (int)(cy - 8.0f), box.height * 0.24f, (Color){ 190, 80, 255, 255 });
        DrawCircle((int)(cx - box.width * 0.08f), (int)(cy - 10.0f), box.height * 0.045f, (Color){ 255, 60, 140, 255 });
        DrawCircle((int)(cx + box.width * 0.08f), (int)(cy - 10.0f), box.height * 0.045f, (Color){ 255, 60, 140, 255 });
        DrawRectangle((int)(cx - box.width * 0.10f), (int)(cy + 8.0f), (int)(box.width * 0.20f), (int)(box.height * 0.12f), (Color){ 45, 15, 60, 255 });
        DrawRectangleLines((int)(cx - box.width * 0.10f), (int)(cy + 8.0f), (int)(box.width * 0.20f), (int)(box.height * 0.12f), (Color){ 190, 80, 255, 255 });
        for (int i = 0; i < 6; i++) {
            float ang = (float)i * (2.0f * 3.14159f / 6.0f);
            float x1 = cx + cosf(ang) * (box.height * 0.28f);
            float y1 = cy + sinf(ang) * (box.height * 0.28f);
            float x2 = cx + cosf(ang) * (box.height * 0.44f);
            float y2 = cy + sinf(ang) * (box.height * 0.44f);
            DrawLineEx((Vector2){ x1, y1 }, (Vector2){ x2, y2 }, 2.5f, (Color){ 220, 80, 255, 200 });
        }
    } else if (strcmp(src, "ZEUS") == 0) {
        DrawCircleGradient((Vector2){ cx, cy }, rGrad, (Color){ 30, 48, 95, 220 }, (Color){ 14, 18, 38, 255 });
        Vector2 p1 = { cx - 35, box.y + 15 };
        Vector2 p2 = { cx + 15, cy - 12 };
        Vector2 p3 = { cx - 10, cy + 2 };
        Vector2 p4 = { cx + 28, box.y + box.height - 18 };
        DrawLineEx(p1, p2, 4.0f, UI_COLOR_GOLD);
        DrawLineEx(p2, p3, 3.5f, (Color){ 255, 245, 160, 255 });
        DrawLineEx(p3, p4, 5.0f, UI_COLOR_GOLD);
        DrawCircle((int)p2.x, (int)p2.y, 5.0f, RAYWHITE);
        DrawCircle((int)p4.x, (int)p4.y, 7.0f, UI_COLOR_GOLD);
    } else if (strcmp(src, "ARES") == 0) {
        DrawCircleGradient((Vector2){ cx, cy }, rGrad, (Color){ 75, 18, 20, 220 }, (Color){ 24, 10, 12, 255 });
        DrawLineEx((Vector2){ box.x + 20, box.y + box.height - 20 }, (Vector2){ box.x + box.width - 20, box.y + 20 }, 3.5f, (Color){ 180, 110, 45, 255 });
        DrawLineEx((Vector2){ box.x + box.width - 20, box.y + box.height - 20 }, (Vector2){ box.x + 20, box.y + 20 }, 3.5f, (Color){ 180, 110, 45, 255 });
        DrawCircle((int)cx, (int)cy, 20.0f, (Color){ 200, 45, 45, 255 });
        DrawCircleLines((int)cx, (int)cy, 20.0f, UI_COLOR_GOLD);
        DrawRectangle((int)(cx - 4.0f), (int)(cy - 26.0f), 8, 14, UI_COLOR_GOLD);
    } else if (strcmp(src, "ATENEA") == 0) {
        DrawCircleGradient((Vector2){ cx, cy }, rGrad, (Color){ 25, 45, 80, 220 }, (Color){ 12, 18, 36, 255 });
        DrawCircle((int)cx, (int)cy, box.height * 0.32f, (Color){ 35, 30, 50, 255 });
        DrawCircleLines((int)cx, (int)cy, box.height * 0.32f, UI_COLOR_GOLD);
        DrawCircleLines((int)cx, (int)cy, box.height * 0.27f, (Color){ 220, 180, 75, 180 });
        DrawCircle((int)(cx - 10), (int)(cy - 2), 5.0f, UI_COLOR_GOLD);
        DrawCircle((int)(cx + 10), (int)(cy - 2), 5.0f, UI_COLOR_GOLD);
        DrawCircle((int)(cx - 10), (int)(cy - 2), 2.0f, RAYWHITE);
        DrawCircle((int)(cx + 10), (int)(cy - 2), 2.0f, RAYWHITE);
    } else if (strcmp(src, "APOLO") == 0) {
        DrawCircleGradient((Vector2){ cx, cy }, rGrad, (Color){ 80, 55, 15, 220 }, (Color){ 24, 16, 10, 255 });
        DrawCircle((int)cx, (int)cy, 24.0f, (Color){ 255, 220, 80, 255 });
        DrawCircleLines((int)cx, (int)cy, 24.0f, RAYWHITE);
        for (int i = 0; i < 8; i++) {
            float ang = (float)i * (2.0f * 3.14159f / 8.0f);
            float x1 = cx + cosf(ang) * 30.0f;
            float y1 = cy + sinf(ang) * 30.0f;
            float x2 = cx + cosf(ang) * 50.0f;
            float y2 = cy + sinf(ang) * 50.0f;
            DrawLineEx((Vector2){ x1, y1 }, (Vector2){ x2, y2 }, 2.5f, (Color){ 255, 205, 60, 220 });
        }
    } else if (strcmp(src, "ARTEMISA") == 0) {
        DrawCircleGradient((Vector2){ cx, cy }, rGrad, (Color){ 16, 45, 32, 220 }, (Color){ 10, 20, 16, 255 });
        DrawCircle((int)(cx - 5), (int)cy, 26.0f, (Color){ 220, 230, 245, 255 });
        DrawCircle((int)(cx + 5), (int)(cy - 3), 22.0f, (Color){ 16, 45, 32, 255 });
        DrawLineEx((Vector2){ cx - 20, cy + 18 }, (Vector2){ cx + 30, cy - 20 }, 2.5f, (Color){ 200, 220, 240, 255 });
    } else if (strcmp(src, "POSEIDON") == 0) {
        DrawCircleGradient((Vector2){ cx, cy }, rGrad, (Color){ 15, 48, 65, 220 }, (Color){ 8, 20, 30, 255 });
        DrawLineEx((Vector2){ cx, box.y + 18 }, (Vector2){ cx, box.y + box.height - 18 }, 4.0f, UI_COLOR_GOLD);
        DrawLineEx((Vector2){ cx - 20, box.y + 26 }, (Vector2){ cx - 20, box.y + 52 }, 3.0f, UI_COLOR_GOLD);
        DrawLineEx((Vector2){ cx + 20, box.y + 26 }, (Vector2){ cx + 20, box.y + 52 }, 3.0f, UI_COLOR_GOLD);
        DrawLineEx((Vector2){ cx - 20, box.y + 52 }, (Vector2){ cx + 20, box.y + 52 }, 3.0f, UI_COLOR_GOLD);
    } else if (strcmp(src, "HADES") == 0) {
        DrawCircleGradient((Vector2){ cx, cy }, rGrad, (Color){ 35, 18, 40, 220 }, (Color){ 14, 8, 18, 255 });
        DrawLineEx((Vector2){ cx, box.y + 18 }, (Vector2){ cx, box.y + box.height - 18 }, 4.0f, (Color){ 160, 130, 190, 255 });
        DrawLineEx((Vector2){ cx - 18, box.y + 26 }, (Vector2){ cx - 18, box.y + 52 }, 3.0f, (Color){ 160, 130, 190, 255 });
        DrawLineEx((Vector2){ cx + 18, box.y + 26 }, (Vector2){ cx + 18, box.y + 52 }, 3.0f, (Color){ 160, 130, 190, 255 });
        DrawLineEx((Vector2){ cx - 18, box.y + 52 }, (Vector2){ cx + 18, box.y + 52 }, 3.0f, (Color){ 160, 130, 190, 255 });
        DrawCircle((int)cx, (int)(cy + 12), 4.5f, (Color){ 100, 255, 200, 220 });
    } else if (strcmp(src, "HEFESTO") == 0) {
        DrawCircleGradient((Vector2){ cx, cy }, rGrad, (Color){ 65, 25, 12, 220 }, (Color){ 24, 10, 8, 255 });
        DrawRectangle((int)(cx - 26), (int)(cy - 8), 52, 20, (Color){ 160, 90, 45, 255 });
        DrawRectangle((int)(cx - 16), (int)(cy + 12), 32, 16, (Color){ 120, 65, 30, 255 });
        DrawRectangleLines((int)(cx - 26), (int)(cy - 8), 52, 20, UI_COLOR_GOLD);
        DrawCircle((int)(cx - 12), (int)(cy - 16), 3.5f, (Color){ 255, 160, 40, 255 });
        DrawCircle((int)(cx + 15), (int)(cy - 20), 2.5f, (Color){ 255, 200, 60, 255 });
    } else if (strcmp(src, "HERMES") == 0) {
        DrawCircleGradient((Vector2){ cx, cy }, rGrad, (Color){ 20, 50, 40, 220 }, (Color){ 10, 24, 18, 255 });
        DrawLineEx((Vector2){ cx, box.y + 18 }, (Vector2){ cx, box.y + box.height - 18 }, 3.5f, UI_COLOR_GOLD);
        DrawCircle((int)cx, (int)(box.y + 20), 6.0f, UI_COLOR_GOLD);
        DrawCircle((int)(cx - 20), (int)(cy - 8), 14.0f, Fade(UI_COLOR_GOLD, 0.7f));
        DrawCircle((int)(cx + 20), (int)(cy - 8), 14.0f, Fade(UI_COLOR_GOLD, 0.7f));
    } else if (strcmp(src, "DEMETER") == 0) {
        DrawCircleGradient((Vector2){ cx, cy }, rGrad, (Color){ 60, 50, 15, 220 }, (Color){ 22, 18, 8, 255 });
        DrawLineEx((Vector2){ cx - 12, box.y + box.height - 18 }, (Vector2){ cx + 12, box.y + 24 }, 3.0f, UI_COLOR_GOLD);
        DrawLineEx((Vector2){ cx + 12, box.y + box.height - 18 }, (Vector2){ cx - 12, box.y + 24 }, 3.0f, UI_COLOR_GOLD);
        for (int i = 0; i < 5; i++) {
            DrawCircle((int)(cx + (i % 2 == 0 ? -8 : 8)), (int)(box.y + 32 + i * 12), 5.0f, (Color){ 255, 220, 90, 255 });
        }
    } else if (strcmp(src, "DIONISIO") == 0) {
        DrawCircleGradient((Vector2){ cx, cy }, rGrad, (Color){ 52, 18, 55, 220 }, (Color){ 20, 8, 24, 255 });
        DrawCircle((int)cx, (int)(cy - 8), 20.0f, (Color){ 140, 25, 45, 255 });
        DrawCircleLines((int)cx, (int)(cy - 8), 20.0f, UI_COLOR_GOLD);
        DrawRectangle((int)(cx - 3), (int)(cy + 12), 6, 16, UI_COLOR_GOLD);
        DrawRectangle((int)(cx - 14), (int)(cy + 28), 28, 5, UI_COLOR_GOLD);
    } else if (strcmp(src, "HESTIA") == 0) {
        DrawCircleGradient((Vector2){ cx, cy }, rGrad, (Color){ 60, 30, 15, 220 }, (Color){ 24, 12, 8, 255 });
        DrawRectangle((int)(cx - 20), (int)(cy + 8), 40, 16, (Color){ 140, 75, 40, 255 });
        DrawRectangleLines((int)(cx - 20), (int)(cy + 8), 40, 16, UI_COLOR_GOLD);
        DrawCircle((int)cx, (int)(cy - 6), 14.0f, (Color){ 255, 140, 30, 255 });
        DrawCircle((int)cx, (int)(cy - 11), 8.0f, (Color){ 255, 230, 80, 255 });
    } else {
        DrawCircleGradient((Vector2){ cx, cy }, rGrad, (Color){ 35, 40, 60, 220 }, (Color){ 16, 18, 28, 255 });
        DrawCircle((int)cx, (int)cy, 22.0f, (Color){ 30, 32, 50, 255 });
        DrawCircleLines((int)cx, (int)cy, 22.0f, UI_COLOR_GOLD);
        DrawCircleLines((int)cx, (int)cy, 15.0f, (Color){ 220, 180, 75, 180 });
    }

    DrawRectangleLinesEx(box, 1.5f, Fade(isCurse ? (Color){ 160, 60, 220, 255 } : UI_COLOR_GOLD, 0.45f));
    EndScissorMode();
}

static void UI_FormatRuleBonus(const ModifierRule* rule, char* out, int cap) {
    if (!rule || !out || cap <= 0) return;
    out[0] = '\0';
    if (rule->effectCount == 0) return;

    const EffectDef* eff = &rule->effects[0];
    float val = eff->value;

    switch (rule->hook) {
        case HOOK_EXEC_ROLL:
            snprintf(out, cap, "%+.0f a tirada de Choque (Ejecucion)", val);
            break;
        case HOOK_EXEC_DAMAGE:
            snprintf(out, cap, "%+.0f%% dano de choque infligido", val);
            break;
        case HOOK_HEALTH_DAMAGE_RECEIVED:
            snprintf(out, cap, "%+.0f%% bajas recibidas en combate", val);
            break;
        case HOOK_RECOUNT_DEATH:
            snprintf(out, cap, "%+.0f%% muertes definitivas tras batalla", val);
            break;
        case HOOK_MAP_BASE_PA:
            snprintf(out, cap, "%+.0f Puntos de Accion (PA) por dia", val);
            break;
        case HOOK_MAP_MOVE_COST:
            snprintf(out, cap, "%+.0f coste de movimiento en mapa", val);
            break;
        case HOOK_MAP_VISION:
            snprintf(out, cap, "%+.0f radio de vision en mapa", val);
            break;
        case HOOK_EVAL_ROLL:
            snprintf(out, cap, "%+.0f a Evaluacion enemiga", val);
            break;
        case HOOK_PLAN_ROLL:
            snprintf(out, cap, "%+.0f a Planificacion tactica", val);
            break;
        case HOOK_AMBUSH_ROLL:
            snprintf(out, cap, "%+.0f deteccion de emboscadas", val);
            break;
        case HOOK_PLAN_OPTIONS:
            snprintf(out, cap, "Desbloquea tactica: Retirada Ordenada");
            break;
        case HOOK_FLEE_COST:
            snprintf(out, cap, "Huida de combate sin penalizacion");
            break;
        case HOOK_COMBAT_END:
            if (eff->type == EFFECT_GOLD) snprintf(out, cap, "+%.0f Oro de botin tras victoria", val);
            else if (eff->type == EFFECT_FOOD) snprintf(out, cap, "+%.0f Comida de botin tras victoria", val);
            else if (eff->type == EFFECT_WATER) snprintf(out, cap, "+%.0f Agua de botin tras victoria", val);
            else if (eff->type == EFFECT_WINE) snprintf(out, cap, "+%.0f Vino sagrado tras victoria", val);
            else if (eff->type == EFFECT_MEAT) snprintf(out, cap, "+%.0f Carne fresca tras victoria", val);
            else if (eff->type == EFFECT_MORAL) snprintf(out, cap, "+%.0f Moral de tropa tras victoria", val);
            else snprintf(out, cap, "Bonificacion especial tras victoria");
            break;
        default:
            if (eff->type == EFFECT_EXECUTION) snprintf(out, cap, "%+.0f a Ejecucion", val);
            else if (eff->type == EFFECT_PLANNING) snprintf(out, cap, "%+.0f a Planificacion", val);
            else if (eff->type == EFFECT_EVALUATION) snprintf(out, cap, "%+.0f a Evaluacion", val);
            else snprintf(out, cap, "%+.0f efecto pasivo", val);
            break;
    }
}

void UI_DrawModifierCard(const struct ModifierDef* def, Rectangle bounds) {
    UI_DrawModifierCardEx(def, bounds, false, 0);
}

void UI_DrawModifierCardEx(const struct ModifierDef* def, Rectangle bounds, bool isExhausted, int battlesUntilRecharge) {
    if (bounds.width <= 0 || bounds.height <= 0) return;

    if (!def) {
        UISkin_DrawNineSliceDefault(UI_SKIN_SLOT, 0, bounds, (Color){ 150, 145, 130, 255 });
        UI_DrawText("Ranura Vacia", (Vector2){ bounds.x + bounds.width * 0.25f, bounds.y + bounds.height * 0.45f }, 15, UI_COLOR_TEXT_DIM);
        return;
    }

    float sX = bounds.width / 512.0f;
    float sY = bounds.height / 768.0f;
    bool isCurse = (def->kind == MOD_KIND_CURSE);

    Color bgTint = isExhausted ? (Color){ 22, 23, 26, 255 } : (isCurse ? (Color){ 18, 12, 26, 255 } : (Color){ 18, 20, 30, 255 });
    DrawRectangleRec(bounds, bgTint);

    Texture2D frameTex = isCurse ? s_texCardCurse : s_texCardDivine;
    Color frameMod = isExhausted ? (Color){ 115, 118, 125, 255 } : WHITE;
    if (frameTex.id > 0) {
        DrawTexturePro(frameTex,
                       (Rectangle){ 0.0f, 0.0f, (float)frameTex.width, (float)frameTex.height },
                       bounds,
                       (Vector2){ 0.0f, 0.0f },
                       0.0f,
                       frameMod);
    } else {
        Color borderCol = isExhausted ? (Color){ 110, 112, 118, 255 } : (isCurse ? (Color){ 170, 70, 230, 255 } : UI_COLOR_GOLD);
        DrawRectangleLinesEx(bounds, 3.0f, borderCol);
    }

    /* 1. Círculo Dios / Emblema superior izquierdo */
    float gcx = bounds.x + 64.0f * sX;
    float gcy = bounds.y + 64.0f * sY;
    float gr  = 26.0f * sX;
    Color accentGold = isExhausted ? (Color){ 135, 138, 145, 255 } : UI_COLOR_GOLD;
    DrawCircle((int)gcx, (int)gcy, gr, isExhausted ? (Color){ 32, 34, 38, 255 } : (isCurse ? (Color){ 45, 16, 62, 255 } : (Color){ 28, 30, 48, 255 }));
    DrawCircleLines((int)gcx, (int)gcy, gr, isCurse ? (Color){ 195, 80, 255, 255 } : accentGold);

    const char* sigil = "?";
    if (isCurse) sigil = "M";
    else if (strcmp(def->source, "ZEUS") == 0) sigil = "Z";
    else if (strcmp(def->source, "ARES") == 0) sigil = "AR";
    else if (strcmp(def->source, "ATENEA") == 0) sigil = "AT";
    else if (strcmp(def->source, "APOLO") == 0) sigil = "AP";
    else if (strcmp(def->source, "ARTEMISA") == 0) sigil = "AR";
    else if (strcmp(def->source, "POSEIDON") == 0) sigil = "P";
    else if (strcmp(def->source, "HADES") == 0) sigil = "H";
    else if (strcmp(def->source, "HEFESTO") == 0) sigil = "HF";
    else if (strcmp(def->source, "HERMES") == 0) sigil = "HR";
    else if (strcmp(def->source, "DEMETER") == 0) sigil = "D";
    else if (strcmp(def->source, "DIONISIO") == 0) sigil = "DI";
    else if (strcmp(def->source, "HESTIA") == 0) sigil = "HS";
    else if (strcmp(def->source, "SELENE") == 0) sigil = "S";
    else if (strcmp(def->source, "HECATE") == 0) sigil = "HC";
    else if (strcmp(def->source, "ERIS") == 0) sigil = "E";
    else if (def->name[0] != '\0') {
        static char tempInit[4] = {0};
        tempInit[0] = def->name[0];
        tempInit[1] = '\0';
        sigil = tempInit;
    }
    Vector2 sigilSz = UI_MeasureText(sigil, gr * 0.95f);
    UI_DrawText(sigil, (Vector2){ gcx - sigilSz.x / 2.0f, gcy - sigilSz.y / 2.0f }, gr * 0.95f, isCurse ? (Color){ 245, 180, 255, 255 } : accentGold);

    /* 2. Banner de Nombre */
    float nx = bounds.x + 110.0f * sX;
    float ny = bounds.y + 38.0f * sY;
    float nw = 362.0f * sX;
    float nh = 54.0f * sY;
    float nameFont = fmaxf(18.0f, 22.0f * sY);
    Vector2 nameSz = UI_MeasureText(def->name, nameFont);
    while (nameSz.x > nw - 10.0f && nameFont > 14.0f) {
        nameFont -= 1.0f;
        nameSz = UI_MeasureText(def->name, nameFont);
    }
    UI_DrawText(def->name,
                (Vector2){ nx + (nw - nameSz.x) / 2.0f, ny + (nh - nameSz.y) / 2.0f },
                nameFont,
                isCurse ? (Color){ 245, 210, 255, 255 } : accentGold);

    /* 3. Cuadro de Ilustración */
    Rectangle imgBox = {
        bounds.x + 40.0f * sX,
        bounds.y + 110.0f * sY,
        432.0f * sX,
        298.0f * sY
    };
    UI_DrawCardIllustration(def, imgBox, isCurse);

    if (isExhausted) {
        /* Desaturación del 60% con tinte gris ceniza sobre la ilustración */
        DrawRectangleRec(imgBox, (Color){ 36, 38, 44, 165 });
        DrawRectangleLinesEx(imgBox, 1.5f, (Color){ 110, 114, 122, 220 });

        /* Sello / Cinta de Estado: [AGOTADA: X BATALLAS] o [AGOTADA: EN REPOSO] */
        const char* sealTxt = (battlesUntilRecharge > 0)
            ? TextFormat("[AGOTADA: %d BATALLAS]", battlesUntilRecharge)
            : "[AGOTADA: EN REPOSO]";
        float sealH = fmaxf(26.0f, 44.0f * sY);
        Rectangle sealRect = {
            imgBox.x + 12.0f * sX,
            imgBox.y + (imgBox.height - sealH) * 0.5f,
            imgBox.width - 24.0f * sX,
            sealH
        };
        DrawRectangleRec(sealRect, (Color){ 24, 22, 26, 235 });
        DrawRectangleLinesEx(sealRect, 1.5f, (Color){ 195, 115, 95, 240 });
        float sealFont = fmaxf(12.5f, 17.0f * sY);
        Vector2 sealSz = UI_MeasureText(sealTxt, sealFont);
        while (sealSz.x > sealRect.width - 10.0f && sealFont > 10.5f) {
            sealFont -= 0.5f;
            sealSz = UI_MeasureText(sealTxt, sealFont);
        }
        UI_DrawText(sealTxt,
                    (Vector2){ sealRect.x + (sealRect.width - sealSz.x) * 0.5f, sealRect.y + (sealRect.height - sealSz.y) * 0.5f },
                    sealFont,
                    (Color){ 235, 165, 140, 255 });
    }

    /* 4. Cinta Tipo • Dios */
    float rx = bounds.x + 40.0f * sX;
    float ry = bounds.y + 420.0f * sY;
    float rw = 432.0f * sX;
    float rh = 32.0f * sY;
    const char* godStr = (def->source[0] != '\0') ? def->source : (isCurse ? "TARTARO" : "OLIMPO");
    const char* typeStr = isCurse ? "MALDICION" : (isExhausted ? "BENDICION AGOTADA" : "BENDICION DIVINA");
    const char* ribTxt = TextFormat("%s  -  %s", typeStr, godStr);
    float ribFont = fmaxf(13.5f, 15.0f * sY);
    Vector2 ribSz = UI_MeasureText(ribTxt, ribFont);
    UI_DrawText(ribTxt,
                (Vector2){ rx + (rw - ribSz.x) / 2.0f, ry + (rh - ribSz.y) / 2.0f },
                ribFont,
                isExhausted ? (Color){ 155, 158, 166, 255 } : (isCurse ? (Color){ 230, 190, 255, 255 } : (Color){ 255, 235, 175, 255 }));

    /* 5. Cuadro de Efectos y Atributos */
    float ex = bounds.x + 40.0f * sX;
    float ey = bounds.y + 456.0f * sY;
    float ew = 432.0f * sX;
    float eh = 192.0f * sY;

    DrawRectangleRec((Rectangle){ ex + 6.0f * sX, ey + 2.0f * sY, ew - 12.0f * sX, eh - 4.0f * sY }, (Color){ 12, 14, 22, 175 });

    float bonusFont = fmaxf(13.5f, 15.0f * sY);
    float curY = ey + 5.0f * sY;
    int maxRulesToShow = (def->ruleCount > 2) ? 2 : def->ruleCount;
    for (int r = 0; r < maxRulesToShow; r++) {
        char bonusTxt[96];
        UI_FormatRuleBonus(&def->rules[r], bonusTxt, sizeof(bonusTxt));
        if (bonusTxt[0] != '\0') {
            const char* fullBonus = TextFormat("%s %s", isCurse ? "[-]" : "[+]", bonusTxt);
            Color ruleCol = isExhausted
                ? (Color){ 115, 118, 126, 230 }
                : (isCurse ? (Color){ 245, 110, 130, 255 } : (Color){ 130, 230, 110, 255 });
            Vector2 rulePos = { ex + 14.0f * sX, curY };
            float lineFont = bonusFont;
            float maxLineW = ew - 28.0f * sX;
            Vector2 ruleSz = UI_MeasureText(fullBonus, lineFont);
            while (ruleSz.x > maxLineW && lineFont > 10.0f) {
                lineFont -= 0.5f;
                ruleSz = UI_MeasureText(fullBonus, lineFont);
            }
            UI_DrawText(fullBonus, rulePos, lineFont, ruleCol);
            if (isExhausted) {
                float strikeY = rulePos.y + ruleSz.y * 0.52f;
                DrawLineEx((Vector2){ rulePos.x, strikeY },
                           (Vector2){ fminf(rulePos.x + ruleSz.x, ex + ew - 14.0f * sX), strikeY },
                           1.5f,
                           (Color){ 165, 110, 110, 230 });
            }
            curY += bonusFont + 3.0f;
        }
    }

    if (def->hasInvocation && def->invocationName[0] != '\0') {
        const char* invLine = TextFormat("[INVOCACION] %s", def->invocationName);
        Color invCol = isExhausted ? (Color){ 115, 118, 126, 220 } : UI_COLOR_GOLD;
        Vector2 invPos = { ex + 14.0f * sX, curY };
        UI_DrawText(invLine, invPos, bonusFont - 0.5f, invCol);
        if (isExhausted) {
            Vector2 invSz = UI_MeasureText(invLine, bonusFont - 0.5f);
            float strikeY = invPos.y + invSz.y * 0.52f;
            DrawLineEx((Vector2){ invPos.x, strikeY },
                       (Vector2){ fminf(invPos.x + invSz.x, ex + ew - 14.0f * sX), strikeY },
                       1.5f,
                       (Color){ 165, 110, 110, 230 });
        }
        curY += bonusFont + 2.0f;
    }

    if (maxRulesToShow > 0 || def->hasInvocation) {
        DrawLineEx((Vector2){ ex + 14.0f * sX, curY + 2.0f },
                   (Vector2){ ex + ew - 14.0f * sX, curY + 2.0f },
                   1.0f,
                   Fade(UI_COLOR_PANEL_BORDER, 0.6f));
        curY += 5.0f;
    }

    if (def->description[0] != '\0') {
        float remainingH = (ey + eh - 2.0f) - curY;
        if (remainingH > 16.0f) {
            float descFont = (maxRulesToShow >= 2 || def->hasInvocation) ? fmaxf(12.5f, 13.5f * sY) : fmaxf(13.5f, 15.0f * sY);
            UI_DrawTextWrappedMaxH(def->description,
                                   (Vector2){ ex + 14.0f * sX, curY },
                                   descFont,
                                   ew - 28.0f * sX,
                                   remainingH,
                                   isExhausted ? (Color){ 135, 138, 146, 230 } : (Color){ 218, 220, 230, 255 });
        }
    }

    /* 6. Cinta Etiquetas • Duración */
    float tx = bounds.x + 40.0f * sX;
    float ty = bounds.y + 658.0f * sY;
    float tw = 432.0f * sX;
    float th = 36.0f * sY;

    char tagBuf[64] = {0};
    if (def->tagCount > 0) {
        snprintf(tagBuf, sizeof(tagBuf), "[%s", def->tags[0]);
        for (int t = 1; t < def->tagCount && t < 2; t++) {
            size_t len = strlen(tagBuf);
            snprintf(tagBuf + len, sizeof(tagBuf) - len, ", %s", def->tags[t]);
        }
        size_t len = strlen(tagBuf);
        snprintf(tagBuf + len, sizeof(tagBuf) - len, "]");
    } else {
        snprintf(tagBuf, sizeof(tagBuf), "[PASIVA]");
    }

    const char* durStr = "PERMANENTE";
    if (isExhausted) {
        durStr = (battlesUntilRecharge > 0)
            ? TextFormat("RECARGA: %d BAT.", battlesUntilRecharge)
            : "EN REPOSO";
    } else if (def->durationType == DURATION_BATTLES) {
        durStr = TextFormat("%d BATALLAS", def->durationValue);
    } else if (def->durationType == DURATION_DAYS) {
        durStr = TextFormat("%d DIAS", def->durationValue);
    }
    const char* bottomTxt = TextFormat("%s  -  %s", tagBuf, durStr);
    float btmFont = fmaxf(12.5f, 14.0f * sY);
    Vector2 btmSz = UI_MeasureText(bottomTxt, btmFont);
    while (btmSz.x > tw - 18.0f * sX && btmFont > 9.5f) {
        btmFont -= 0.5f;
        btmSz = UI_MeasureText(bottomTxt, btmFont);
    }
    UI_DrawText(bottomTxt,
                (Vector2){ tx + (tw - btmSz.x) / 2.0f, ty + (th - btmSz.y) / 2.0f },
                btmFont,
                isExhausted ? (Color){ 150, 152, 160, 255 } : (Color){ 200, 205, 218, 255 });

    /* 7. Círculo Rareza inferior */
    float rcx = bounds.x + 256.0f * sX;
    float rcy = bounds.y + 728.0f * sY;
    float rcr = 15.0f * sX;

    Color gemColor = (Color){ 195, 205, 215, 255 };
    if (isExhausted) {
        gemColor = (Color){ 110, 112, 120, 255 };
    } else if (def->rarityWeight <= 5) {
        gemColor = (Color){ 245, 50, 65, 255 };
    } else if (def->rarityWeight <= 15) {
        gemColor = (Color){ 185, 60, 240, 255 };
    } else if (def->rarityWeight <= 35) {
        gemColor = (Color){ 45, 160, 245, 255 };
    }

    DrawCircle((int)rcx, (int)rcy, rcr, gemColor);
    DrawCircleLines((int)rcx, (int)rcy, rcr, accentGold);
    DrawCircle((int)(rcx - rcr * 0.35f), (int)(rcy - rcr * 0.35f), rcr * 0.32f, Fade(WHITE, isExhausted ? 0.35f : 0.85f));

    /* Tooltip de Recarga al hacer hover sobre una carta agotada */
    if (isExhausted && CheckCollisionPointRec(GetMousePosition(), bounds)) {
        UI_RequestTooltip("CARTA AGOTADA",
                          "Esta carta gasto su poder divino en una invocacion. Reconsagrala en un Templo urbano, en un Altar sagrado o reposa en campamento.",
                          GetMousePosition());
    }
}
