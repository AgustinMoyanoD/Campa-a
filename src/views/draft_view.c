#include "views/draft_view.h"
#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"
#include <stdlib.h>

void DraftView_Init(DraftContext* ctx, const ModifierRegistry* reg) {
    ctx->registry = reg;
    if (!ctx || !reg) return;
    ctx->selectedChoice = -1;
    ctx->wantsToReplace = false;
    ctx->replaceIndex = -1;
    
    // Pick 3 random modifiers that are NOT curses
    int validCount = 0;
    ModifierDef* valid[64];
    for (int i=0; i<reg->count; i++) {
        if (reg->defs[i].kind != MOD_KIND_CURSE) {
            valid[validCount++] = (ModifierDef*)&reg->defs[i];
        }
    }
    
    for (int i=0; i<3; i++) {
        if (validCount > 0) {
            int r = rand() % validCount;
            ctx->choices[i] = valid[r];
        } else {
            ctx->choices[i] = NULL;
        }
    }
}

int DraftView_UpdateDraw(DraftContext* ctx, Army* army, int SW, int SH) {
    if (!ctx || !army) return 1; // Done
    
    DrawRectangle(0, 0, SW, SH, Fade(BLACK, 0.8f));
    
    int cx = SW / 2;
    int cy = SH / 2;
    
    UI_DrawText("Un Altar Antiguo", (Vector2){cx - MeasureText("Un Altar Antiguo", 30)/2, cy - 250}, 30, UI_COLOR_GOLD);
    UI_DrawText("Elige una bendicion para tu ejercito:", (Vector2){cx - MeasureText("Elige una bendicion para tu ejercito:", 20)/2, cy - 200}, 20, RAYWHITE);
    
    if (!ctx->wantsToReplace) {
        int cardW = 200;
        int cardH = 300;
        int startX = cx - (cardW*3 + 40)/2;
        int py = cy - 100;
        
        for (int i=0; i<3; i++) {
            if (!ctx->choices[i]) continue;
            int px = startX + i*(cardW + 20);
            
            bool hover = CheckCollisionPointRec(GetMousePosition(), (Rectangle){px, py, cardW, cardH});
            Color bgCol = hover ? (Color){40,40,50,255} : (Color){20,20,25,255};
            if (ctx->selectedChoice == i) bgCol = (Color){60,60,40,255};
            
            DrawRectangle(px, py, cardW, cardH, bgCol);
            DrawRectangleLines(px, py, cardW, cardH, hover ? GOLD : DARKGRAY);
            
            UI_DrawText(ctx->choices[i]->name, (Vector2){px + 10, py + 10}, 16, UI_COLOR_GOOD);
            DrawTextEx(GetFontDefault(), ctx->choices[i]->description, (Vector2){px + 10, py + 40}, 12, 1, LIGHTGRAY);
            
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hover) {
                ctx->selectedChoice = i;
                Audio_PlaySfx(SFX_CARD_TRIGGER);
            }
        }
        
        if (ctx->selectedChoice != -1) {
            if (UI_DrawButton((Rectangle){cx - 100, cy + 230, 200, 40}, "Aceptar Bendicion", false)) {
                if (army->general.cardCount >= 3) {
                    ctx->wantsToReplace = true;
                } else {
                    Army_EquipModifier(army, ctx->choices[ctx->selectedChoice]);
                    return 1; // Finished
                }
            }
        }
    } else {
        UI_DrawText("Tus ranuras estan llenas. Elige una carta para reemplazar:", (Vector2){cx - MeasureText("Tus ranuras estan llenas. Elige una carta para reemplazar:", 20)/2, cy - 150}, 20, RED);
        
        int cardW = 200;
        int cardH = 200;
        int startX = cx - (cardW*3 + 40)/2;
        int py = cy - 50;
        
        for (int i=0; i<3; i++) {
            if (i >= army->general.cardCount) continue;
            int px = startX + i*(cardW + 20);
            
            bool hover = CheckCollisionPointRec(GetMousePosition(), (Rectangle){px, py, cardW, cardH});
            DrawRectangle(px, py, cardW, cardH, hover ? (Color){60,30,30,255} : (Color){40,20,20,255});
            DrawRectangleLines(px, py, cardW, cardH, hover ? RED : DARKGRAY);
            
                        ModifierDef* d = Modifier_GetById(ctx->registry->defs, ctx->registry->count, army->general.cards[i].id);
            UI_DrawText(d ? d->name : "Desconocida", (Vector2){px + 10, py + 10}, 16, UI_COLOR_TEXT_MAIN);
            
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hover) {
                // Reemplazar
                Army_ReplaceCard(army, i, ctx->choices[ctx->selectedChoice]);
                Audio_PlaySfx(SFX_CARD_TRIGGER);
                return 1; // Finished
            }
        }
        
        if (UI_DrawButton((Rectangle){cx - 100, cy + 180, 200, 40}, "Cancelar Reemplazo", false)) {
            ctx->wantsToReplace = false;
        }
    }
    
    // Skip button
    if (UI_DrawButton((Rectangle){cx - 100, cy + 280, 200, 40}, "Rechazar (Ignorar Altar)", false)) {
        return 1; // Finished without taking
    }
    
    return 0; // Still drafting
}
