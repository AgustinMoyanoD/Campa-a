#include "views/combat_view.h"
#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"
#include "presentation/sequence.h"
#include "presentation/tween.h"
#include "presentation/screen_fx.h"
#include "presentation/audio_manager.h"
#include <stdio.h>
#include <stdlib.h>

static int GetRandomDice() { return GetRandomValue(1, 6); }

static void DrawArmyIcons(Vector2 startPos, int initiates, int soldiers, int sergeants, Color color) {
    int maxPerRow = 15;
    int drawn = 0;
    int spacing = 12;
    Vector2 pos = startPos;
    
    // Sergeants
    for(int i=0; i<sergeants; i++) {
        DrawRectangle(pos.x + (drawn%maxPerRow)*spacing, pos.y + (drawn/maxPerRow)*spacing, 8, 8, UI_COLOR_GOLD);
        drawn++;
    }
    // Soldiers
    for(int i=0; i<soldiers; i++) {
        DrawRectangle(pos.x + (drawn%maxPerRow)*spacing, pos.y + (drawn/maxPerRow)*spacing, 8, 8, color);
        drawn++;
    }
    // Initiates
    for(int i=0; i<initiates; i++) {
        DrawRectangle(pos.x + (drawn%maxPerRow)*spacing, pos.y + (drawn/maxPerRow)*spacing, 6, 6, Fade(color, 0.5f));
        drawn++;
    }
}

void CombatView_Draw(CombatContext* ctx, int SW, int SH) {
    if (!ctx) return;
    
    int panelW = 900;
    int panelH = 600;
    int px = (SW - panelW) / 2;
    int py = (SH - panelH) / 2;
    
    UI_DrawPanel((Rectangle){px, py, panelW, panelH});
    
    UI_DrawText("Combate Tactico", (Vector2){px + 30, py + 20}, 24, UI_COLOR_TEXT_TITLE);
    
    int tx = px + 30;
    int ty = py + 70;
    
    // Draw the armies
    UI_DrawText("Tus Fuerzas", (Vector2){tx, ty}, 18, UI_COLOR_GOOD);
    DrawArmyIcons((Vector2){tx, ty+25}, ctx->playerArmy->iniciados, ctx->playerArmy->soldados, ctx->playerArmy->sargentos, UI_COLOR_GOOD);
    
    int rightX = px + panelW - 250;
    UI_DrawText("Fuerzas Enemigas", (Vector2){rightX, ty}, 18, UI_COLOR_BAD);
    
    // Approximate enemy counts just for visual
    int e_str = ctx->enemy->baseStrength;
    DrawArmyIcons((Vector2){rightX, ty+25}, e_str, e_str/2, e_str/5, UI_COLOR_BAD);
    
    // Draw states
    int middleY = py + 250;
    
    if (ctx->state == COMBAT_STATE_SELECT_PLAN) {
        UI_DrawText("Fase de Planificacion", (Vector2){tx, middleY}, 22, UI_COLOR_TEXT_MAIN);
        
        UI_DrawButton((Rectangle){tx, middleY+40, 250, 40}, "Huir", false);
        UI_DrawButton((Rectangle){tx, middleY+90, 250, 40}, "Escaramuza", !ctx->canSkirmish);
        UI_DrawButton((Rectangle){tx, middleY+140, 250, 40}, "Linea de Picas (8+)", !ctx->canPikes);
        UI_DrawButton((Rectangle){tx, middleY+190, 250, 40}, "Asalto Frontal (6+)", !ctx->canAssault);
        UI_DrawButton((Rectangle){tx, middleY+240, 250, 40}, "Flanqueo (9+ / Expuesto)", !ctx->canFlank);
    }
    else if (ctx->state == COMBAT_STATE_RESOLUTION || ctx->state == COMBAT_STATE_FINISHED) {
        UI_DrawText("Reporte de Bajas", (Vector2){tx, middleY}, 22, UI_COLOR_TEXT_MAIN);
        UI_DrawText(TextFormat("Bajas Propias: -%d Iniciados, -%d Soldados, -%d Sargentos", ctx->casualtiesInitiates, ctx->casualtiesSoldiers, ctx->casualtiesSergeants), (Vector2){tx, middleY+40}, 18, UI_COLOR_BAD);
        
        if (ctx->endReason == COMBAT_END_VICTORY_BREAK) UI_DrawText("VICTORIA: El enemigo se ha quebrado.", (Vector2){tx, middleY+80}, 20, UI_COLOR_GOLD);
        else if (ctx->endReason == COMBAT_END_PLAYER_ROUT) UI_DrawText("DERROTA: Tus tropas huyen en panico.", (Vector2){tx, middleY+80}, 20, UI_COLOR_BAD);
        
        UI_DrawButton((Rectangle){tx, middleY+140, 200, 40}, "Continuar", false);
    }
    else {
        UI_DrawText("Calculando...", (Vector2){tx, middleY}, 22, UI_COLOR_TEXT_DIM);
        UI_DrawButton((Rectangle){tx, middleY+140, 200, 40}, "Avanzar", false);
    }
}

static void OnCasualtyStart(void* unused) {
    ScreenFX_Shake(0.6f, 15.0f);
}

bool CombatView_Update(CombatContext* ctx, int SW, int SH) {
    if (!ctx) return true;
    
    int panelW = 900;
    int panelH = 600;
    int px = (SW - panelW) / 2;
    int py = (SH - panelH) / 2;
    int tx = px + 30;
    int middleY = py + 250;
    
        // Avoid double processing while sequencer is animating the combat log
    if (Sequence_IsBusy()) return false;
    
    // If we have unplayed steps, enqueue them!
    if (ctx->log.count > 0) {
        for (int i = 0; i < ctx->log.count; i++) {
            SeqAction act = {0};
            act.duration = 0.8f;
            if (ctx->log.steps[i].type == STEP_CASUALTIES) { act.duration = 1.0f; act.on_start = OnCasualtyStart; }
            Sequence_Push(act);
        }
        // Actually, for simplicity right now we just clear the log and let the sequencer run dummy waits.
        // We can bind visual changes inside the on_end callbacks.
        ctx->log.count = 0;
        return false;
    }
    
    if (ctx->state == COMBAT_STATE_SELECT_PLAN) {
        if (UI_DrawButton((Rectangle){tx, middleY+40, 250, 40}, "Huir", false)) Combat_SelectPlan(ctx, PLAN_FLEE);
        if (ctx->canSkirmish && UI_DrawButton((Rectangle){tx, middleY+90, 250, 40}, "Escaramuza", false)) Combat_SelectPlan(ctx, PLAN_SKIRMISH);
        if (ctx->canPikes && UI_DrawButton((Rectangle){tx, middleY+140, 250, 40}, "Linea de Picas (8+)", false)) Combat_SelectPlan(ctx, PLAN_PIKES);
        if (ctx->canAssault && UI_DrawButton((Rectangle){tx, middleY+190, 250, 40}, "Asalto Frontal (6+)", false)) Combat_SelectPlan(ctx, PLAN_ASSAULT);
        if (ctx->canFlank && UI_DrawButton((Rectangle){tx, middleY+240, 250, 40}, "Flanqueo (9+ / Expuesto)", false)) Combat_SelectPlan(ctx, PLAN_FLANK);
    }
    else if (ctx->state == COMBAT_STATE_RESOLUTION || ctx->state == COMBAT_STATE_FINISHED) {
        if (UI_DrawButton((Rectangle){tx, middleY+140, 200, 40}, "Continuar", false)) {
            if (ctx->state == COMBAT_STATE_FINISHED) return true;
        }
    }
    else {
        if (UI_DrawButton((Rectangle){tx, middleY+140, 200, 40}, "Avanzar", false)) {
            if (ctx->state == COMBAT_STATE_EVALUATION) Combat_RollEvaluation(ctx, GetRandomDice(), GetRandomDice());
            else if (ctx->state == COMBAT_STATE_PLANNING) Combat_RollPlanning(ctx, GetRandomDice(), GetRandomDice());
            else if (ctx->state == COMBAT_STATE_EXECUTION) Combat_RollExecution(ctx, GetRandomDice(), GetRandomDice());
        }
    }
    return false;
}
