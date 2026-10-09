#include "views/combat_view.h"
#include <stdio.h>
#include <stdlib.h>

static int GetRandomDice() { return GetRandomValue(1, 6); }

static const char* GetPlanName(CombatPlan plan) {
    switch (plan) {
        case PLAN_FLEE: return "Huir";
        case PLAN_ASSAULT: return "Asalto Frontal";
        case PLAN_SKIRMISH: return "Escaramuza";
        case PLAN_PIKES: return "Linea de Picas";
        case PLAN_FLANK: return "Flanqueo";
        
        
        default: return "Ninguno";
    }
}

static const char* GetCombatStateName(CombatState state) {
    switch(state) {
        case COMBAT_STATE_EVALUATION: return "FASE 1: EVALUACION";
        case COMBAT_STATE_PLANNING: return "FASE 2: PLANIFICACION";
        case COMBAT_STATE_SELECT_PLAN: return "FASE 2.5: SELECCION DE PLAN";
        case COMBAT_STATE_EXECUTION: return "FASE 3: EJECUCION";
        case COMBAT_STATE_RESOLUTION: return "FASE 4: RESOLUCION";
        default: return "";
    }
}

void CombatView_Draw(CombatContext* ctx, int SW, int SH) {
    if (!ctx) return;
    
    // Fondo oscuro translucido
    DrawRectangle(0, 0, SW, SH, (Color){ 10, 15, 25, 240 });
    
    // Panel Central
    int panelW = 800;
    int panelH = 500;
    int px = (SW - panelW) / 2;
    int py = (SH - panelH) / 2;
    
    DrawRectangle(px, py, panelW, panelH, (Color){ 245, 245, 250, 255 });
    DrawRectangleLinesEx((Rectangle){ px, py, panelW, panelH }, 4, DARKGRAY);
    
    // Titulo Estado
    DrawText(GetCombatStateName(ctx->state), px + 20, py + 20, 30, DARKBLUE);
    
    // Info Enemigo (Derecha)
    int ex = px + 450;
    int ey = py + 80;
    DrawText("ENEMIGO:", ex, ey, 20, MAROON);
    DrawText(ctx->enemy->name, ex, ey + 30, 24, BLACK);
    
    if (ctx->state >= COMBAT_STATE_PLANNING) {
        // En planificacion ya tiramos evaluacion, mostramos composicion
        DrawText(TextFormat("Fuerza: %d", ctx->enemy->baseStrength), ex, ey + 70, 18, DARKGRAY);
        DrawText(TextFormat("Movilidad: %d", ctx->enemy->mobility), ex, ey + 95, 18, DARKGRAY);
        
        // Mostrar tags revelados
        int ty = ey + 130;
        DrawText("Tags Revelados:", ex, ty, 18, MAROON);
        if (Enemy_IsTagKnown(ctx->enemy, TAG_CAVALRY)) { DrawText("- Caballeria", ex, ty+=20, 16, BLACK); }
        if (Enemy_IsTagKnown(ctx->enemy, TAG_AMBUSH)) { DrawText("- Emboscada", ex, ty+=20, 16, BLACK); }
        if (Enemy_IsTagKnown(ctx->enemy, TAG_EXPOSED_FLANK)) { DrawText("- Flanco Exp.", ex, ty+=20, 16, BLACK); }
    } else {
        DrawText("Fuerza: ???", ex, ey + 70, 18, DARKGRAY);
        DrawText("Composicion Desconocida", ex, ey + 95, 18, DARKGRAY);
    }
    
    // Info Jugador / Acciones (Izquierda)
    int tx = px + 30;
    int ty = py + 80;
    
    if (ctx->state == COMBAT_STATE_EVALUATION) {
        DrawText("Determina que tanta informacion obtienes", tx, ty, 16, DARKGRAY);
        DrawText("del escuadron enemigo.", tx, ty+20, 16, DARKGRAY);
        
        DrawText("[ESPACIO] para tirar Evaluacion (2d6)", tx, py + panelH - 60, 20, BLUE);
    }
    else if (ctx->state == COMBAT_STATE_PLANNING) {
        DrawText(TextFormat("Resultado Evaluacion: %d (Margen: %+d)", ctx->evalRoll, ctx->evalMargin), tx, ty, 20, BLACK);
        DrawText("Calculando los posibles planes...", tx, ty+40, 16, DARKGRAY);
        DrawText("[ESPACIO] Tirar Planificacion (2d6)", tx, py + panelH - 60, 20, BLUE);
    }
    else if (ctx->state == COMBAT_STATE_SELECT_PLAN) {
        DrawText(TextFormat("Tirada de Planificacion: %d", ctx->planRoll), tx, ty, 20, BLACK);
        if (ctx->planRoll >= 9) {
            DrawText("Planificacion Perfecta (Opciones avanzadas)", tx, ty+20, 16, DARKGREEN);
        } else if (ctx->planRoll >= 6) {
            DrawText("Planificacion Estandar", tx, ty+20, 16, DARKGRAY);
        } else {
            DrawText("Planificacion Deficiente (Limitado)", tx, ty+20, 16, RED);
        }
        
                DrawText("Selecciona tu plan:", tx, ty+45, 20, MAROON);
        
        Rectangle btnHuir = { tx, ty+80, 250, 40 };
        DrawRectangleRec(btnHuir, LIGHTGRAY); DrawRectangleLinesEx(btnHuir, 2, DARKGRAY);
        DrawText(TextFormat("Huir (Costo: %d Comida)", ctx->fleeCostFood), tx+10, ty+90, 16, BLACK);
        
        Rectangle btnSkirmish = { tx, ty+130, 250, 40 };
        DrawRectangleRec(btnSkirmish, ctx->canSkirmish ? LIGHTGRAY : DARKGRAY); DrawRectangleLinesEx(btnSkirmish, 2, BLACK);
        DrawText("Escaramuza", tx+10, ty+140, 16, ctx->canSkirmish ? BLACK : LIGHTGRAY);
        
        Rectangle btnPikes = { tx, ty+180, 250, 40 };
        DrawRectangleRec(btnPikes, ctx->canPikes ? LIGHTGRAY : DARKGRAY); DrawRectangleLinesEx(btnPikes, 2, BLACK);
        DrawText("Linea de Picas (8+)", tx+10, ty+190, 16, ctx->canPikes ? BLACK : LIGHTGRAY);
        
        Rectangle btnAsalto = { tx, ty+230, 250, 40 };
        DrawRectangleRec(btnAsalto, ctx->canAssault ? LIGHTGRAY : DARKGRAY); DrawRectangleLinesEx(btnAsalto, 2, BLACK);
        DrawText("Asalto Frontal (6+)", tx+10, ty+240, 16, ctx->canAssault ? BLACK : LIGHTGRAY);
        
        Rectangle btnFlank = { tx, ty+280, 250, 40 };
        DrawRectangleRec(btnFlank, ctx->canFlank ? LIGHTGRAY : DARKGRAY); DrawRectangleLinesEx(btnFlank, 2, BLACK);
        DrawText("Flanqueo (9+ y Flanco Expuesto)", tx+10, ty+290, 16, ctx->canFlank ? BLACK : LIGHTGRAY);
        
        DrawText("Haz click en un plan para avanzar", tx, py + panelH - 40, 16, BLUE);
    }
    else if (ctx->state == COMBAT_STATE_EXECUTION) {
        DrawText(TextFormat("Plan Seleccionado: %s", GetPlanName(ctx->selectedPlan)), tx, ty, 20, BLACK);
        DrawText("Aqui se decide el dano infligido y recibido.", tx, ty+40, 16, DARKGRAY);
        
        DrawText("[ESPACIO] para tirar Ejecucion (2d6)", tx, py + panelH - 60, 20, BLUE);
        }
    else if (ctx->state == COMBAT_STATE_RESOLUTION) {
        DrawText("REPORTE DE COMBATE", tx, ty, 24, MAROON);
        
        if (ctx->selectedPlan == PLAN_FLEE) {
            DrawText("El ejercito escapo.", tx, ty+40, 18, BLACK);
            DrawText(TextFormat("Comida perdida: %d", ctx->fleeCostFood), tx, ty+70, 18, RED);
        } else {
            Rectangle rollRect = { tx, ty+40, 250, 30 };
            DrawText(TextFormat("Puntuacion Combate: %d (Enemigo: %d)", ctx->playerScore, ctx->enemyScore), rollRect.x, rollRect.y, 18, BLACK);
            Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, rollRect)) {
                Rectangle tooltip = { mouse.x + 10, mouse.y + 10, 250, 60 + ctx->breakdown.count * 20 };
                DrawRectangleRec(tooltip, (Color){ 240, 240, 240, 255 });
                DrawRectangleLinesEx(tooltip, 1, DARKGRAY);
                DrawText("Desglose de Ejecucion:", tooltip.x + 10, tooltip.y + 10, 14, DARKBLUE);
                DrawText(TextFormat("Base (Dados + Ventaja + Plan): %d", ctx->breakdown.base), tooltip.x + 10, tooltip.y + 30, 14, BLACK);
                for (int i = 0; i < ctx->breakdown.count; i++) {
                    if (ctx->breakdown.entries[i].flat != 0) DrawText(TextFormat("%s: %+d", ctx->breakdown.entries[i].source, ctx->breakdown.entries[i].flat), tooltip.x + 10, tooltip.y + 50 + i * 20, 14, DARKGREEN);
                    else DrawText(TextFormat("%s: %+d%%", ctx->breakdown.entries[i].source, ctx->breakdown.entries[i].percent), tooltip.x + 10, tooltip.y + 50 + i * 20, 14, DARKGREEN);
                }
            }
            
            DrawText("Bajas Propias:", tx, ty+80, 20, RED);
            DrawText(TextFormat("Iniciados: -%d", ctx->casualtiesInitiates), tx, ty+110, 16, BLACK);
            DrawText(TextFormat("Soldados: -%d", ctx->casualtiesSoldiers), tx, ty+130, 16, BLACK);
            DrawText(TextFormat("Sargentos: -%d", ctx->casualtiesSergeants), tx, ty+150, 16, BLACK);
            
            DrawText("Botin Obtenido:", tx, ty+190, 20, GREEN);
            DrawText(TextFormat("Oro: +%d", ctx->lootGold), tx, ty+220, 16, BLACK);
            DrawText(TextFormat("Comida: +%d", ctx->lootFood), tx, ty+240, 16, BLACK);
        }
        
        DrawText("[ESPACIO] para Terminar", tx, py + panelH - 60, 20, BLUE);
    }
}

bool CombatView_Update(CombatContext* ctx, int SW, int SH) {
    if (!ctx) return true;
    
    int panelW = 800;
    int panelH = 500;
    int px = (SW - panelW) / 2;
    int py = (SH - panelH) / 2;
    int tx = px + 30;
    int ty = py + 80;
    
    if (IsKeyPressed(KEY_SPACE)) {
        if (ctx->state == COMBAT_STATE_EVALUATION) {
            Combat_RollEvaluation(ctx, GetRandomDice(), GetRandomDice());
        }
        else if (ctx->state == COMBAT_STATE_PLANNING) {
            Combat_RollPlanning(ctx, GetRandomDice(), GetRandomDice());
        }
        else if (ctx->state == COMBAT_STATE_EXECUTION) {
            Combat_RollExecution(ctx, GetRandomDice(), GetRandomDice());
        }
        else if (ctx->state == COMBAT_STATE_RESOLUTION) {
            Combat_ApplyResolution(ctx);
            if (ctx->state == COMBAT_STATE_FINISHED) return true;
        }
    }
    
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && ctx->state == COMBAT_STATE_SELECT_PLAN) {
        Vector2 mouse = GetMousePosition();
        Rectangle btnHuir = { tx, ty+80, 250, 40 };
        Rectangle btnSkirmish = { tx, ty+130, 250, 40 };
        Rectangle btnPikes = { tx, ty+180, 250, 40 };
        Rectangle btnAsalto = { tx, ty+230, 250, 40 };
        Rectangle btnFlank = { tx, ty+280, 250, 40 };
        
        if (CheckCollisionPointRec(mouse, btnHuir)) {
            Combat_SelectPlan(ctx, PLAN_FLEE);
        }
        else if (ctx->canSkirmish && CheckCollisionPointRec(mouse, btnSkirmish)) {
            Combat_SelectPlan(ctx, PLAN_SKIRMISH);
        }
        else if (ctx->canPikes && CheckCollisionPointRec(mouse, btnPikes)) {
            Combat_SelectPlan(ctx, PLAN_PIKES);
        }
        else if (ctx->canAssault && CheckCollisionPointRec(mouse, btnAsalto)) {
            Combat_SelectPlan(ctx, PLAN_ASSAULT);
        }
        else if (ctx->canFlank && CheckCollisionPointRec(mouse, btnFlank)) {
            Combat_SelectPlan(ctx, PLAN_FLANK);
        }
    }
    
    return false;
}
