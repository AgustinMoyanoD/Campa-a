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

static bool isRollingDice = false;
static float rollTimer = 0.0f;
static int visualDice1 = 1;
static int visualDice2 = 1;

static float visualWeight = 0.0f;
static bool combatInitialized = false;

static void OnCasualtyStart(void* unused) {
    ScreenFX_Shake(0.6f, 15.0f);
}

static void OnDiceStart(void* unused) {
    isRollingDice = true;
    rollTimer = 0.8f;
    Audio_PlaySfx(SFX_DICE_ROLL);
}
static void OnDiceEnd(void* unused) {
    isRollingDice = false;
    Audio_PlaySfx(SFX_DICE_LAND);
}

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
    
    UI_DrawText(TextFormat("Combate Tactico - Ronda %d", ctx->round), (Vector2){px + 30, py + 20}, 24, UI_COLOR_TEXT_TITLE);
    
    int tx = px + 30;
    int ty = py + 70;
    
    // Moral Bar
    UI_DrawText("Integridad del Ejercito", (Vector2){tx, ty}, 16, UI_COLOR_TEXT_DIM);
    UI_DrawBar((Rectangle){tx, ty + 20, 200, 10}, ctx->playerInitialWeight > 0 ? (visualWeight / (float)ctx->playerInitialWeight) : 0.0f, UI_COLOR_GOOD, UI_COLOR_BAD);
    
    ty += 50;
    
    UI_DrawText("Tus Fuerzas", (Vector2){tx, ty}, 18, UI_COLOR_GOOD);
    DrawArmyIcons((Vector2){tx, ty+25}, ctx->playerArmy->iniciados, ctx->playerArmy->soldados, ctx->playerArmy->sargentos, UI_COLOR_GOOD);
    
    int rightX = px + panelW - 250;
    UI_DrawText("Fuerzas Enemigas", (Vector2){rightX, ty}, 18, UI_COLOR_BAD);
    
    int e_str = ctx->enemy->baseStrength;
    DrawArmyIcons((Vector2){rightX, ty+25}, e_str, e_str/2, e_str/5, UI_COLOR_BAD);
    
    int middleY = py + 300;
    
    if (isRollingDice) {
        visualDice1 = GetRandomValue(1, 6);
        visualDice2 = GetRandomValue(1, 6);
        UI_DrawText(TextFormat("Rodando: [%d] [%d]", visualDice1, visualDice2), (Vector2){tx, middleY}, 28, UI_COLOR_GOLD);
    } else if (Sequence_IsBusy()) {
        UI_DrawText("Aplicando...", (Vector2){tx, middleY}, 22, UI_COLOR_TEXT_DIM);
    } else {
        if (ctx->state == COMBAT_STATE_EVALUATION) {
            UI_DrawText("Fase de Evaluacion (Viendo al enemigo)", (Vector2){tx, middleY}, 22, UI_COLOR_TEXT_MAIN);
            UI_DrawButton((Rectangle){tx, middleY+60, 200, 40}, "Avanzar", false);
        }
        else if (ctx->state == COMBAT_STATE_PLANNING) {
            UI_DrawText("Fase de Planificacion", (Vector2){tx, middleY}, 22, UI_COLOR_TEXT_MAIN);
            UI_DrawText(TextFormat("Tiraste un %d en Evaluacion.", ctx->evalRoll), (Vector2){tx, middleY+30}, 18, UI_COLOR_GOLD);
            UI_DrawButton((Rectangle){tx, middleY+80, 200, 40}, "Avanzar", false);
        }
        else if (ctx->state == COMBAT_STATE_SELECT_PLAN) {
            UI_DrawText(TextFormat("Planes Desbloqueados (Tiraste un %d)", ctx->planRoll), (Vector2){tx, middleY}, 22, UI_COLOR_GOLD);
            UI_DrawButton((Rectangle){tx, middleY+40, 250, 40}, "Huir", false);
            UI_DrawButton((Rectangle){tx, middleY+90, 250, 40}, "Escaramuza", !ctx->canSkirmish);
            UI_DrawButton((Rectangle){tx, middleY+140, 250, 40}, "Linea de Picas (8+)", !ctx->canPikes);
            UI_DrawButton((Rectangle){tx, middleY+190, 250, 40}, "Asalto Frontal (6+)", !ctx->canAssault);
            UI_DrawButton((Rectangle){tx, middleY+240, 250, 40}, "Flanqueo (9+ / Expuesto)", !ctx->canFlank);
        }
        else if (ctx->state == COMBAT_STATE_EXECUTION) {
            UI_DrawText("Fase de Ejecucion (Choque)", (Vector2){tx, middleY}, 22, UI_COLOR_TEXT_MAIN);
            UI_DrawButton((Rectangle){tx, middleY+60, 200, 40}, "Avanzar", false);
        }
        else if (ctx->state == COMBAT_STATE_RESOLUTION || ctx->state == COMBAT_STATE_FINISHED) {
            UI_DrawText("Reporte de Bajas", (Vector2){tx, middleY}, 22, UI_COLOR_TEXT_MAIN);
            UI_DrawText(TextFormat("Ejecucion: %d | Ventaja: %d | Total: %d", ctx->execRoll, ctx->forceAdvantage, ctx->playerScore), (Vector2){tx, middleY+30}, 18, UI_COLOR_GOLD);
            UI_DrawText(TextFormat("Bajas Propias: -%d Iniciados, -%d Soldados, -%d Sargentos", ctx->casualtiesInitiates, ctx->casualtiesSoldiers, ctx->casualtiesSergeants), (Vector2){tx, middleY+60}, 18, UI_COLOR_BAD);
            
            if (ctx->endReason == COMBAT_END_VICTORY_BREAK) UI_DrawText("VICTORIA: El enemigo se ha quebrado.", (Vector2){tx, middleY+100}, 20, UI_COLOR_GOLD);
            else if (ctx->endReason == COMBAT_END_PLAYER_ROUT) UI_DrawText("DERROTA: Tus tropas huyen en panico.", (Vector2){tx, middleY+100}, 20, UI_COLOR_BAD);
            else if (ctx->endReason == COMBAT_END_STALEMATE) UI_DrawText("EMPATE: El combate termina sin vencedor.", (Vector2){tx, middleY+100}, 20, UI_COLOR_TEXT_DIM);
            
            UI_DrawButton((Rectangle){tx, middleY+140, 200, 40}, "Continuar", false);
        }
    }
}

bool CombatView_Update(CombatContext* ctx, int SW, int SH) {
    if (!ctx) return true;
    
    if (!combatInitialized) {
        visualWeight = (float)Army_GetTotalWeight(ctx->playerArmy);
        combatInitialized = true;
    }
    
    if (isRollingDice) {
        rollTimer -= GetFrameTime();
        if (rollTimer <= 0) isRollingDice = false;
    }
    
    if (Sequence_IsBusy()) return false;
    
    if (ctx->log.count > 0) {
        for (int i = 0; i < ctx->log.count; i++) {
            SeqAction act = {0};
            act.duration = 0.5f;
            if (ctx->log.steps[i].type == STEP_ROLL) {
                act.duration = 0.8f;
                act.on_start = OnDiceStart;
                act.on_end = OnDiceEnd;
            } else if (ctx->log.steps[i].type == STEP_CASUALTIES) {
                act.duration = 1.0f;
                act.on_start = OnCasualtyStart;
            }
            Sequence_Push(act);
        }
        ctx->log.count = 0;
        
        float currentW = (float)Army_GetTotalWeight(ctx->playerArmy);
        if (visualWeight != currentW) {
            SeqAction mAct = {0};
            mAct.duration = 0.5f;
            Sequence_Push(mAct);
            Tween_Start(&visualWeight, currentW, 0.5f, Ease_OutCubic);
        }
        return false;
    }
    
    int panelW = 900;
    int panelH = 600;
    int px = (SW - panelW) / 2;
    int py = (SH - panelH) / 2;
    int tx = px + 30;
    int middleY = py + 300;
    
    if (ctx->state == COMBAT_STATE_SELECT_PLAN) {
        if (UI_DrawButton((Rectangle){tx, middleY+40, 250, 40}, "Huir", false)) Combat_SelectPlan(ctx, PLAN_FLEE);
        if (ctx->canSkirmish && UI_DrawButton((Rectangle){tx, middleY+90, 250, 40}, "Escaramuza", false)) Combat_SelectPlan(ctx, PLAN_SKIRMISH);
        if (ctx->canPikes && UI_DrawButton((Rectangle){tx, middleY+140, 250, 40}, "Linea de Picas (8+)", false)) Combat_SelectPlan(ctx, PLAN_PIKES);
        if (ctx->canAssault && UI_DrawButton((Rectangle){tx, middleY+190, 250, 40}, "Asalto Frontal (6+)", false)) Combat_SelectPlan(ctx, PLAN_ASSAULT);
        if (ctx->canFlank && UI_DrawButton((Rectangle){tx, middleY+240, 250, 40}, "Flanqueo (9+ / Expuesto)", false)) Combat_SelectPlan(ctx, PLAN_FLANK);
    }
    else if (ctx->state == COMBAT_STATE_RESOLUTION || ctx->state == COMBAT_STATE_FINISHED) {
        if (UI_DrawButton((Rectangle){tx, middleY+140, 200, 40}, "Continuar", false)) {
            if (ctx->state == COMBAT_STATE_FINISHED) {
                combatInitialized = false;
                return true;
            } else {
                ctx->state = COMBAT_STATE_SELECT_PLAN; // Or reset to EVALUATION if new round
            }
        }
    }
    else if (ctx->state == COMBAT_STATE_EVALUATION || ctx->state == COMBAT_STATE_EXECUTION) {
        if (UI_DrawButton((Rectangle){tx, middleY+60, 200, 40}, "Avanzar", false)) {
            if (ctx->state == COMBAT_STATE_EVALUATION) Combat_RollEvaluation(ctx, GetRandomDice(), GetRandomDice());
            else if (ctx->state == COMBAT_STATE_EXECUTION) Combat_RollExecution(ctx, GetRandomDice(), GetRandomDice());
        }
    }
    else if (ctx->state == COMBAT_STATE_PLANNING) {
        if (UI_DrawButton((Rectangle){tx, middleY+80, 200, 40}, "Avanzar", false)) {
            Combat_RollPlanning(ctx, GetRandomDice(), GetRandomDice());
        }
    }
    return false;
}
