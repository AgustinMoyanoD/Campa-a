#include "views/draft_view.h"
#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"
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
    
    DrawRectangle(0, 0, SW, SH, Fade(BLACK, 0.8f));
    
    int cx = SW / 2;
    int cy = SH / 2;
    
    UI_DrawText("Altar de los Dioses", (Vector2){cx - MeasureText("Altar de los Dioses", 34)/2, cy - 250}, 34, UI_COLOR_GOLD);
    
    if (ctx->state == ALTAR_STATE_OFFER) {
        UI_DrawText("Los dioses exigen un tributo a cambio de su favor.", (Vector2){cx - MeasureText("Los dioses exigen un tributo a cambio de su favor.", 20)/2, cy - 180}, 20, RAYWHITE);
        
        bool canPayGold = army->monedas >= 50;
        
        if (UI_DrawButton((Rectangle){cx - 200, cy - 50, 400, 50}, TextFormat("Ofrendar Oro (50 Monedas) [%d disponibles]", army->monedas), !canPayGold)) {
            army->monedas -= 50;
            ctx->state = ALTAR_STATE_REVEAL;
            Audio_PlaySfx(SFX_CARD_TRIGGER);
        }
        
        if (UI_DrawButton((Rectangle){cx - 200, cy + 30, 400, 50}, "Aceptar Maldicion (Pacto Sangriento)", false)) {
            if (ctx->hiddenCurse) {
                // Agregar maldicion oculta
                General* g = &army->general;
                if (g->curseCount < 4) {
                    strncpy(g->curses[g->curseCount].id, ctx->hiddenCurse->id, 31);
                    g->curses[g->curseCount].remaining = -1;
                    g->curseCount++;
                }
            }
            ctx->state = ALTAR_STATE_REVEAL;
            Audio_PlaySfx(SFX_CARD_TRIGGER); // Sonido ominoso podria ir aqui
        }
        
        if (UI_DrawButton((Rectangle){cx - 100, cy + 150, 200, 40}, "Alejarse del Altar", false)) {
            return 1;
        }
    } 
    else if (ctx->state == ALTAR_STATE_REVEAL) {
        UI_DrawText("Los dioses han respondido...", (Vector2){cx - MeasureText("Los dioses han respondido...", 22)/2, cy - 180}, 22, UI_COLOR_GOOD);
        
        if (ctx->pulledCard) {
            int cardW = 220, cardH = 320;
            int px = cx - cardW/2;
            int py = cy - 120;
            
            DrawRectangle(px, py, cardW, cardH, (Color){30,30,40,255});
            DrawRectangleLines(px, py, cardW, cardH, GOLD);
            UI_DrawText(ctx->pulledCard->name, (Vector2){px + 10, py + 10}, 18, UI_COLOR_GOOD);
            DrawTextEx(GetFontDefault(), ctx->pulledCard->description, (Vector2){px + 10, py + 50}, 14, 1, LIGHTGRAY);
            
            if (UI_DrawButton((Rectangle){cx - 100, py + cardH + 30, 200, 40}, "Aceptar Bendicion", false)) {
                if (army->general.cardCount >= 3) {
                    ctx->state = ALTAR_STATE_REPLACE;
                } else {
                    Army_ReplaceCard(army, army->general.cardCount, ctx->pulledCard);
                    Audio_PlaySfx(SFX_CARD_TRIGGER);
                    return 1;
                }
            }
        } else {
            UI_DrawText("No hay dioses escuchando (Error de pool).", (Vector2){cx - 150, cy}, 20, RED);
            if (UI_DrawButton((Rectangle){cx - 100, cy + 100, 200, 40}, "Continuar", false)) return 1;
        }
    }
    else if (ctx->state == ALTAR_STATE_REPLACE) {
        UI_DrawText("Tus ranuras estan llenas. Elige una bendicion para REEMPLAZAR:", (Vector2){cx - MeasureText("Tus ranuras estan llenas. Elige una bendicion para REEMPLAZAR:", 20)/2, cy - 200}, 20, RED);
        
        int cardW = 200;
        int cardH = 200;
        int startX = cx - (cardW*3 + 40)/2;
        int py = cy - 100;
        
        for (int i=0; i<3; i++) {
            if (i >= army->general.cardCount) continue;
            int px = startX + i*(cardW + 20);
            
            bool hover = CheckCollisionPointRec(GetMousePosition(), (Rectangle){px, py, cardW, cardH});
            DrawRectangle(px, py, cardW, cardH, hover ? (Color){60,30,30,255} : (Color){40,20,20,255});
            DrawRectangleLines(px, py, cardW, cardH, hover ? RED : DARKGRAY);
            
            ModifierDef* d = Modifier_GetById(ctx->registry->defs, ctx->registry->count, army->general.cards[i].id);
            UI_DrawText(d ? d->name : "Desconocida", (Vector2){px + 10, py + 10}, 16, UI_COLOR_TEXT_MAIN);
            
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hover) {
                Army_ReplaceCard(army, i, ctx->pulledCard);
                Audio_PlaySfx(SFX_CARD_TRIGGER);
                return 1; // Finished
            }
        }
        
        if (UI_DrawButton((Rectangle){cx - 100, cy + 150, 200, 40}, "Cancelar (Conservar Actuales)", false)) {
            return 1;
        }
    }
    
    return 0;
}
