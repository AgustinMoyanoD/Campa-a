#include "views/draft_view.h"
#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"
#include "presentation/audio_manager.h"
#include <stdlib.h>
#include <string.h>

void DraftView_Init(DraftContext* ctx, const ModifierRegistry* reg) {
    if (!ctx || !reg) return;
    ctx->registry = reg;
    ctx->state = ALTAR_STATE_OFFER;
    ctx->pulledCard = NULL;
    ctx->hiddenCurse = NULL;
    
    // Preparar el pool de Cartas y Maldiciones
    ModifierDef* cards[64];
    ModifierDef* curses[64];
    int cardCount = 0, curseCount = 0;
    
    for (int i=0; i<reg->count; i++) {
        if (reg->defs[i].kind == MOD_KIND_CARD) {
            cards[cardCount++] = (ModifierDef*)&reg->defs[i];
        } else if (reg->defs[i].kind == MOD_KIND_CURSE) {
            curses[curseCount++] = (ModifierDef*)&reg->defs[i];
        }
    }
    
    if (cardCount > 0) ctx->pulledCard = cards[rand() % cardCount];
    if (curseCount > 0) ctx->hiddenCurse = curses[rand() % curseCount];
}

int DraftView_UpdateDraw(DraftContext* ctx, Army* army, int SW, int SH) {
    if (!ctx || !army) return 1;
    
    DrawRectangle(0, 0, SW, SH, Fade(BLACK, 0.82f));
    
    int cx = SW / 2;
    int cy = SH / 2;
    
    int cardH = (SH >= 900) ? 600 : ((SH >= 760) ? 570 : 530);
    int py = (SH - cardH - 100) / 2 + 42;

    Vector2 titleSz = UI_MeasureText("Altar de los Dioses", 38);
    float titleY = (ctx->state == ALTAR_STATE_OFFER) ? (cy - 250.0f) : (float)(py - 74);
    UI_DrawText("Altar de los Dioses", (Vector2){cx - titleSz.x/2.0f, titleY}, 38, UI_COLOR_GOLD);
    
    if (ctx->state == ALTAR_STATE_OFFER) {
        Rectangle offerPanel = { (float)(cx - 310), (float)(cy - 210), 620.0f, 430.0f };
        UI_DrawPanelStyled(offerPanel, UI_STYLE_PANEL_DARK);

        const char* msg = "Los dioses exigen un tributo a cambio de su favor.";
        Vector2 msgSz = UI_MeasureText(msg, 20);
        UI_DrawText(msg, (Vector2){cx - msgSz.x/2.0f, cy - 175.0f}, 20, RAYWHITE);
        UI_DrawStatusChip((Rectangle){ (float)(cx - 210), (float)(cy - 135), 420.0f, 26.0f },
                          "El aura del Altar ha reconsagrado tus cartas divinas",
                          UI_STYLE_CHIP_OK, UI_ICON_OK);
        
        bool canPayGold = army->monedas >= 50;
        
        if (UI_DrawButtonStyledSfx((Rectangle){cx - 260, cy - 50, 520, 54}, TextFormat("Ofrendar Oro (50 Monedas) [%d disponibles]", army->monedas), UI_STYLE_BTN_DEFAULT, !canPayGold, "altar_offer")) {
            army->monedas -= 50;
            ctx->state = ALTAR_STATE_REVEAL;
        }
        
        if (UI_DrawButtonStyledSfx((Rectangle){cx - 260, cy + 30, 520, 54}, "Aceptar Maldicion (Pacto Sangriento)", UI_STYLE_BTN_CURSE, false, NULL)) {
            if (ctx->hiddenCurse) {
                // Agregar maldicion a la armada/general sin ocupar ranuras de cartas divinas
                Army_EquipModifier(army, ctx->hiddenCurse);
                Audio_PlaySfxByName("curse_acquired");
            }
            ctx->state = ALTAR_STATE_REVEAL;
        }
        
        if (UI_DrawButtonStyled((Rectangle){cx - 130, cy + 150, 260, 46}, "Alejarse del Altar", UI_STYLE_BTN_DEFAULT, false)) {
            return 1;
        }
    } 
    else if (ctx->state == ALTAR_STATE_REVEAL) {
        int cardH = (SH >= 900) ? 600 : ((SH >= 760) ? 570 : 530);
        int cardW = (cardH * 2) / 3;
        int px = cx - cardW / 2;
        int py = (SH - cardH - 100) / 2 + 42;

        const char* respMsg = "Los dioses han respondido con su bendicion...";
        Vector2 respSz = UI_MeasureText(respMsg, 20);
        UI_DrawText(respMsg, (Vector2){cx - respSz.x/2.0f, (float)(py - 28)}, 20, UI_COLOR_GOOD);
        
        if (ctx->pulledCard) {
            Rectangle cardBounds = { (float)px, (float)py, (float)cardW, (float)cardH };
            UISkin_DrawGlow(cardBounds, (Color){ 255, 220, 140, 255 }, 0.75f, 0);
            UI_DrawModifierCard(ctx->pulledCard, cardBounds);
            
            if (UI_DrawButtonStyledSfx((Rectangle){cx - 140, py + cardH + 16, 280, 46}, "Aceptar Bendicion", UI_STYLE_BTN_PRIMARY, false, NULL)) {
                if (army->general.cardCount >= 3) {
                    Audio_PlaySfxByName("altar_offer");
                    ctx->state = ALTAR_STATE_REPLACE;
                } else {
                    Army_ReplaceCard(army, army->general.cardCount, ctx->pulledCard);
                    Audio_PlaySfxByName("divine_card_acquired");
                    return 1;
                }
            }
        } else {
            UI_DrawText("No hay dioses escuchando (Error de pool).", (Vector2){cx - 150, cy}, 20, RED);
            if (UI_DrawButtonStyled((Rectangle){cx - 100, cy + 100, 200, 44}, "Continuar", UI_STYLE_BTN_DEFAULT, false)) return 1;
        }
    }
    else if (ctx->state == ALTAR_STATE_REPLACE) {
        int cardH = (SH >= 900) ? 450 : ((SH >= 760) ? 410 : 370);
        int cardW = (cardH * 2) / 3;
        int gap = 24;
        int startX = cx - (cardW * 3 + gap * 2) / 2;
        int py = (SH - cardH - 100) / 2 + 42;

        const char* replMsg = "Tus 3 ranuras estan llenas. Elige una bendicion para REEMPLAZAR:";
        Vector2 replSz = UI_MeasureText(replMsg, 20);
        UI_DrawText(replMsg, (Vector2){cx - replSz.x/2.0f, (float)(py - 28)}, 20, (Color){245, 90, 80, 255});
        
        for (int i=0; i<3; i++) {
            if (i >= army->general.cardCount) continue;
            int px = startX + i * (cardW + gap);
            Rectangle cardRec = { (float)px, (float)py, (float)cardW, (float)cardH };
            bool hover = CheckCollisionPointRec(GetMousePosition(), cardRec);
            
            if (hover) {
                UISkin_DrawGlow(cardRec, (Color){ 245, 60, 60, 255 }, 0.85f, 0);
            }

            ModifierDef* d = Modifier_GetById(ctx->registry->defs, ctx->registry->count, army->general.cards[i].id);
            if (d) {
                UI_DrawModifierCardEx(d, cardRec, army->general.cards[i].isExhausted, army->general.cards[i].battlesUntilRecharge);
            }
            
            if (hover) {
                UI_DrawStatusChip((Rectangle){ (float)(px + 12), (float)(py + cardH - 42), (float)(cardW - 24), 32.0f }, "SACRIFICAR", UI_STYLE_CHIP_BAD, UI_ICON_PELIGRO);
            }
            
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hover) {
                Audio_PlaySfxByName("card_sacrificed");
                Army_ReplaceCard(army, i, ctx->pulledCard);
                Audio_PlaySfxByName("divine_card_acquired");
                return 1; // Finished
            }
        }
        
        if (UI_DrawButtonStyled((Rectangle){cx - 150, py + cardH + 18, 300, 44}, "Cancelar (Conservar Actuales)", UI_STYLE_BTN_DEFAULT, false)) {
            return 1;
        }
    }
    
    return 0;
}
