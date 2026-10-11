#include "views/combat_view.h"
#include "controllers/game_master.h"
#include "presentation/sequence.h"
#include "presentation/tween.h"
#include "presentation/ui_widgets.h"
#include "presentation/audio_manager.h"
#include <stdio.h>

static bool combatInitialized = false;
static bool isRollingDice = false;
static float rollTimer = 0.0f;
static int visualDice1 = 1;
static int visualDice2 = 1;
static float endTimer = 0.0f;

static void OnDiceStart() {
    isRollingDice = true;
    rollTimer = 0.8f;
    Audio_PlaySfx(SFX_DICE_ROLL);
}
static void OnDiceEnd() {
    isRollingDice = false;
    Audio_PlaySfx(SFX_DICE_LAND);
}
static void OnCasualtyStart() {
    // Flash effect
}

static void DrawArmyIcons(Vector2 pos, int initiates, int soldiers, int sergeants, Color color) {
    int maxPerRow = 20;
    int spacing = 12;
    int drawn = 0;
    for(int i=0; i<sergeants; i++) { DrawRectangle(pos.x + (drawn%maxPerRow)*spacing, pos.y + (drawn/maxPerRow)*spacing, 10, 10, color); drawn++; }
    for(int i=0; i<soldiers; i++) { DrawRectangle(pos.x + (drawn%maxPerRow)*spacing, pos.y + (drawn/maxPerRow)*spacing, 8, 8, Fade(color, 0.8f)); drawn++; }
    for(int i=0; i<initiates; i++) { DrawRectangle(pos.x + (drawn%maxPerRow)*spacing, pos.y + (drawn/maxPerRow)*spacing, 6, 6, Fade(color, 0.5f)); drawn++; }
}

void CombatView_Draw(CombatContext* ctx, int SW, int SH) {
    if (!ctx) return;
    int panelW = 960; int panelH = 600;
    int px = (SW - panelW) / 2; int py = (SH - panelH) / 2;
    
    UI_DrawPanel((Rectangle){px, py, panelW, panelH});
    UI_DrawText(TextFormat("Combate Tactico - Ronda %d", ctx->round), (Vector2){px + 30, py + 20}, 24, WHITE);
    
    int tx = px + 30; int ty = py + 70;
    
    // Health Bars
    UI_DrawText(TextFormat("Tus Fuerzas (Salud: %.0f)", ctx->playerHealth), (Vector2){tx, ty}, 18, GREEN);
    UI_DrawBar((Rectangle){tx, ty + 25, 250, 12}, ctx->playerInitialHealth > 0 ? (ctx->playerHealth / ctx->playerInitialHealth) : 0.0f, GREEN, RED);
    DrawArmyIcons((Vector2){tx, ty+45}, ctx->playerArmy->iniciados - ctx->outOfActionInitiates, ctx->playerArmy->soldados - ctx->outOfActionSoldiers, ctx->playerArmy->sargentos - ctx->outOfActionSergeants, GREEN);
    
    int rightX = px + panelW - 300;
    UI_DrawText(TextFormat("Enemigo (Salud: %.0f)", ctx->enemyHealth), (Vector2){rightX, ty}, 18, RED);
    UI_DrawBar((Rectangle){rightX, ty + 25, 250, 12}, ctx->enemyInitialHealth > 0 ? (ctx->enemyHealth / ctx->enemyInitialHealth) : 0.0f, RED, GREEN);
    
    if (ctx->enemy->infoLevel >= 1) {
        UI_DrawText(TextFormat("Fuerza Base: %d", ctx->enemy->baseStrength), (Vector2){rightX, ty + 45}, 16, GRAY);
    } else {
        UI_DrawText("Fuerza Base: ???", (Vector2){rightX, ty + 45}, 16, GRAY);
    }
    
    if (ctx->enemy->infoLevel >= 2) {
        int tyo = 65;
        if (ctx->enemy->tags & TAG_CAVALRY) { UI_DrawText("[Caballeria]", (Vector2){rightX, ty + tyo}, 16, GOLD); tyo += 20; }
        if (ctx->enemy->tags & TAG_AMBUSH) { UI_DrawText("[Emboscada]", (Vector2){rightX, ty + tyo}, 16, GOLD); tyo += 20; }
        if (ctx->enemy->tags & TAG_EXPOSED_FLANK) { UI_DrawText("[Flanco Expuesto]", (Vector2){rightX, ty + tyo}, 16, GOLD); tyo += 20; }
        if (ctx->enemy->tags & TAG_HEAVILY_ARMORED) { UI_DrawText("[Acorazados]", (Vector2){rightX, ty + tyo}, 16, GOLD); tyo += 20; }
        if (ctx->enemy->tags & TAG_DISORGANIZED) { UI_DrawText("[Desorganizados]", (Vector2){rightX, ty + tyo}, 16, GOLD); tyo += 20; }
        if (tyo == 65) UI_DrawText("Sin Tags", (Vector2){rightX, ty + tyo}, 16, GRAY);
    } else {
        UI_DrawText("Tags: ???", (Vector2){rightX, ty + 65}, 16, GRAY);
    }
    
    int middleY = py + 260;
    
    // Draw roll feedback
    if (isRollingDice) {
        visualDice1 = GetRandomValue(1, 6);
        visualDice2 = GetRandomValue(1, 6);
        UI_DrawText(TextFormat("Tirando dados: [%d] [%d]", visualDice1, visualDice2), (Vector2){tx, middleY - 40}, 28, GOLD);
    } else if (ctx->state == COMBAT_STATE_ROUND_CLOSE || ctx->state == COMBAT_STATE_END || ctx->state == COMBAT_STATE_RECOUNT || ctx->state == COMBAT_STATE_DONE) {
        if (ctx->margin > 0) {
            UI_DrawText(TextFormat("!Perdiste el choque! (Margen: -%d)", ctx->margin), (Vector2){tx, middleY - 50}, 24, RED);
        } else if (ctx->margin < 0) {
            UI_DrawText(TextFormat("!Ganaste el choque! (Margen: +%d)", -ctx->margin), (Vector2){tx, middleY - 50}, 24, GREEN);
        } else {
            UI_DrawText("!Empate en el choque!", (Vector2){tx, middleY - 50}, 24, GRAY);
        }
        UI_DrawText(TextFormat("Tu Fuerza: %d  vs  Fuerza Enemiga: %d", ctx->playerScore, ctx->enemyScore), (Vector2){tx, middleY - 20}, 18, LIGHTGRAY);
        if (ctx->breakdown.count > 0) {
            char bdText[256] = "Cartas Activas: ";
            for (int i=0; i<ctx->breakdown.count; i++) {
                char tmp[64];
                if (ctx->breakdown.entries[i].percent != 0) {
                    sprintf(tmp, "%s%+d%% ", ctx->breakdown.entries[i].source, ctx->breakdown.entries[i].percent);
                } else {
                    sprintf(tmp, "%s%+d ", ctx->breakdown.entries[i].source, ctx->breakdown.entries[i].flat);
                }
                
                // strcat
                int blen = 0;
                while(bdText[blen] != '\0') blen++;
                int tlen = 0;
                while(tmp[tlen] != '\0') bdText[blen++] = tmp[tlen++];
                bdText[blen] = '\0';
            }
            UI_DrawText(bdText, (Vector2){tx, middleY - 40}, 16, GOLD);
        }
        if (ctx->criticalHit) {
            UI_DrawText("!! CRITICO EXCEPCIONAL !! (+5 Fuerza, x1.5 Dano)", (Vector2){tx, middleY - 80}, 22, GOLD);
        } else if (ctx->criticalFumble) {
            UI_DrawText("... DESASTRE TACTICO ... (-5 Fuerza, x0.5 Dano)", (Vector2){tx, middleY - 80}, 22, RED);
        }
    }
    
    if (Sequence_IsBusy()) return; // Don't draw buttons while animating
    
    if (ctx->state == COMBAT_STATE_EVALUATION) {
        UI_DrawText("Fase de Evaluacion", (Vector2){tx, middleY}, 22, LIGHTGRAY);
        UI_DrawButton((Rectangle){tx, middleY+40, 250, 45}, "Evaluar Terreno", false);
    }
    else if (ctx->state == COMBAT_STATE_PLANNING) {
        if (ctx->round == 1) {
            if (ctx->ambushed) {
                UI_DrawText("!EMBOSCADA! Sin evaluacion previa (-2 a Planificacion)", (Vector2){tx, middleY - 40}, 22, RED);
            } else if (ctx->evalRoll >= 10) {
                UI_DrawText(TextFormat("Evaluacion Excelente (%d). !Todo revelado!", ctx->evalRoll), (Vector2){tx, middleY - 40}, 22, GREEN);
            } else if (ctx->evalRoll >= 8) {
                UI_DrawText(TextFormat("Evaluacion Buena (%d). Fuerza revelada.", ctx->evalRoll), (Vector2){tx, middleY - 40}, 22, GOLD);
            } else {
                UI_DrawText(TextFormat("Evaluacion Pobre (%d). El enemigo es un misterio.", ctx->evalRoll), (Vector2){tx, middleY - 40}, 22, RED);
            }
        }
        UI_DrawText("Fase de Planificacion", (Vector2){tx, middleY}, 22, LIGHTGRAY);
        if (ctx->intentRevealed) {
            const char* intents[] = {"Carga (Agresivo)", "Asedio (Defensivo)", "Repliegue (Tactico)"};
            UI_DrawText(TextFormat("Intencion Enemiga revelada: %s", intents[ctx->enemyIntent]), (Vector2){tx, middleY + 25}, 18, ORANGE);
        } else {
            UI_DrawText("Intencion Enemiga: ???", (Vector2){tx, middleY + 25}, 18, GRAY);
        }
        UI_DrawButton((Rectangle){tx, middleY+40, 250, 45}, "Tirar Dados de Mando", false);
    }
    else if (ctx->state == COMBAT_STATE_EXECUTION) {
        UI_DrawText(TextFormat("Planificacion (Tirada: %d) (Mando: %d). Elige tu Plan:", ctx->planRoll, ctx->playerArmy->commandTokens), (Vector2){tx, middleY}, 22, GOLD);
        if (ctx->intentRevealed) {
            const char* intents[] = {"Carga (Agresivo)", "Asedio (Defensivo)", "Repliegue (Tactico)"};
            UI_DrawText(TextFormat("Intencion Enemiga revelada: %s", intents[ctx->enemyIntent]), (Vector2){tx, middleY + 22}, 18, ORANGE);
        }
        UI_DrawButton((Rectangle){tx, middleY+40, 250, 40}, "Huir", false);
if (ctx->playerArmy->commandTokens > 0) {
    UI_DrawButton((Rectangle){tx + 260, middleY+40, 115, 40}, "Mando: +2", false);
    UI_DrawButton((Rectangle){tx + 385, middleY+40, 115, 40}, "Mando: Reroll", false);
}
        const char* txtSk = (ctx->intentRevealed && ctx->enemyIntent == 1) ? "Escaramuza (Dano -30%) [CONTRA ASEDIO]" : "Escaramuza (Dano -30%)";
        const char* txtPk = (ctx->intentRevealed && ctx->enemyIntent == 0) ? "Linea de Picas (8+) (Tirada -1) [CONTRA CARGA]" : "Linea de Picas (8+) (Tirada -1)";
        const char* txtAs = (ctx->intentRevealed && ctx->enemyIntent == 2) ? "Asalto Frontal (6+) (Enemigo +2) [CONTRA REPLIEGUE]" : "Asalto Frontal (6+) (Enemigo +2)";
        const char* txtFl = (ctx->intentRevealed && ctx->enemyIntent == 1) ? "Flanqueo (9+ / Expuesto) (Tirada +4) [CONTRA ASEDIO]" : "Flanqueo (9+ / Expuesto) (Tirada +4)";

        if (ctx->previousPlan == PLAN_SKIRMISH) txtSk = TextFormat("%s [FATIGADO]", txtSk);
        if (ctx->previousPlan == PLAN_PIKES) txtPk = TextFormat("%s [FATIGADO]", txtPk);
        if (ctx->previousPlan == PLAN_ASSAULT) txtAs = TextFormat("%s [FATIGADO]", txtAs);
        if (ctx->previousPlan == PLAN_FLANK) txtFl = TextFormat("%s [FATIGADO]", txtFl);

                UI_DrawButton((Rectangle){tx, middleY+85, 550, 35}, txtSk, !ctx->canSkirmish);
        UI_DrawButton((Rectangle){tx, middleY+125, 550, 35}, txtPk, !ctx->canPikes);
        UI_DrawButton((Rectangle){tx, middleY+165, 550, 35}, txtAs, !ctx->canAssault);
        UI_DrawButton((Rectangle){tx, middleY+205, 550, 35}, txtFl, !ctx->canFlank);

        // Dibujar cartas activas debajo
        ActiveMod ams[16];
        int amCount = Modifier_CollectActive(&ctx->gm->modRegistry, ctx->playerArmy, ctx->enemy, ams, 16);
        int cx = tx;
        int cy = middleY + 250;
        for (int i=0; i<amCount; i++) {
            if (ams[i].def->kind == MOD_KIND_CARD || ams[i].def->kind == MOD_KIND_CURSE) {
                Color c = (ams[i].def->kind == MOD_KIND_CURSE) ? RED : GOLD;
                UI_DrawText(TextFormat("[%s]", ams[i].def->name), (Vector2){cx, cy}, 16, c);
                cx += 180;
                if (cx > tx + 360) { cx = tx; cy += 25; }
            }
        }
    }
    else if (ctx->state == COMBAT_STATE_ROUND_CLOSE) {
        UI_DrawButton((Rectangle){tx, middleY+40, 250, 45}, "Cerrar Ronda", false);
    }
    else if (ctx->state == COMBAT_STATE_END || ctx->state == COMBAT_STATE_RECOUNT || ctx->state == COMBAT_STATE_DONE) {
        UI_DrawText("Fin del Combate", (Vector2){tx, middleY+20}, 26, WHITE);
        
        Color resCol = GRAY;
        if (ctx->endReason == COMBAT_END_VICTORY) resCol = GOLD;
        if (ctx->endReason == COMBAT_END_DEFEAT || ctx->endReason == COMBAT_END_FLEE) resCol = RED;
        
        UI_DrawText(TextFormat("Resultado: %s", 
            ctx->endReason == COMBAT_END_VICTORY ? "VICTORIA" : 
            ctx->endReason == COMBAT_END_DEFEAT ? "DERROTA" : 
            ctx->endReason == COMBAT_END_FLEE ? "HUIDA" : "EMPATE"), 
            (Vector2){tx, middleY+55}, 22, resCol);
            
        UI_DrawText(TextFormat("Fuera de combate: %d Iniciados, %d Soldados, %d Sargentos", ctx->outOfActionInitiates, ctx->outOfActionSoldiers, ctx->outOfActionSergeants), (Vector2){tx, middleY+90}, 18, RED);
        
        if (ctx->state == COMBAT_STATE_END) {
            UI_DrawButton((Rectangle){tx, middleY+130, 250, 45}, "Hacer Recuento y Salir", false);
        } else if (ctx->state == COMBAT_STATE_DONE) {
            UI_DrawText(TextFormat("Muertes Confirmadas: %d Iniciados, %d Soldados, %d Sargentos", ctx->deadInitiates, ctx->deadSoldiers, ctx->deadSergeants), (Vector2){tx, middleY+120}, 20, RED);
        }
    }
}

bool CombatView_Update(CombatContext* ctx, int SW, int SH) {
    if (!ctx) return true;
    
    static int lastCombatState = -1;

    if (!combatInitialized) {
        combatInitialized = true;
        endTimer = 0.0f;
        
        Audio_PlayCombatMusic(0, 1.0f);
        Audio_PlayCombatSFX(0);
        
        lastCombatState = ctx->state;
    } else {
        if (ctx->state != lastCombatState) {
            Audio_PlayCombatMusic(ctx->state, 1.0f);
            Audio_PlayCombatSFX(ctx->state);
            lastCombatState = ctx->state;
        }
    }
    
    if (isRollingDice) {
        rollTimer -= GetFrameTime();
        if (rollTimer <= 0) isRollingDice = false;
    }
    
    if (Sequence_IsBusy()) return false;
    
    if (ctx->log.count > 0) {
        for (int i = 0; i < ctx->log.count; i++) {
            SeqAction act = {0};
            act.duration = 0.4f;
            if (ctx->log.steps[i].type == STEP_ROLL) {
                act.duration = 0.8f;
                act.on_start = OnDiceStart;
                act.on_end = OnDiceEnd;
            } else if (ctx->log.steps[i].type == STEP_OUT_OF_ACTION) {
                act.duration = 0.8f;
                act.on_start = OnCasualtyStart;
            } else if (ctx->log.steps[i].type == STEP_RECOUNT) {
                act.duration = 0.5f; // Faster recount
            }
            Sequence_Push(act);
        }
        ctx->log.count = 0;
        return false;
    }

    if (ctx->state == COMBAT_STATE_DONE) {
        endTimer += GetFrameTime();
        if (endTimer > 4.0f || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Combat_Commit(ctx);
            combatInitialized = false;
            return true;
        }
        return false;
    }

    int panelW = 960; int panelH = 600;
    int px = (SW - panelW) / 2; int py = (SH - panelH) / 2;
    int tx = px + 30; int middleY = py + 260;
    
    if (ctx->state == COMBAT_STATE_EVALUATION) {
        if (UI_DrawButton((Rectangle){tx, middleY+40, 250, 45}, "Evaluar Terreno", false)) { Combat_RollEvaluation(ctx, GetRandomValue(1,6), GetRandomValue(1,6)); }
    }
    else if (ctx->state == COMBAT_STATE_PLANNING) {
        if (UI_DrawButton((Rectangle){tx, middleY+40, 250, 45}, "Tirar Dados de Mando", false)) { Combat_RollPlanning(ctx, GetRandomValue(1,6), GetRandomValue(1,6)); }
    }
    else if (ctx->state == COMBAT_STATE_EXECUTION) {
        if (UI_DrawButton((Rectangle){tx, middleY+40, 250, 40}, "Huir", false)) { Combat_SelectPlan(ctx, PLAN_FLEE); Combat_RollExecution(ctx, GetRandomValue(1,6), GetRandomValue(1,6)); }

        if (ctx->playerArmy->commandTokens > 0) {
            if (UI_DrawButton((Rectangle){tx + 260, middleY+40, 115, 40}, "Mando: +2", false)) { Combat_UseCommandToken_Add(ctx); }
            if (UI_DrawButton((Rectangle){tx + 385, middleY+40, 115, 40}, "Mando: Reroll", false)) { Combat_UseCommandToken_Reroll(ctx, GetRandomValue(1,6), GetRandomValue(1,6)); }
        }
        const char* txtSk2 = (ctx->intentRevealed && ctx->enemyIntent == 1) ? "Escaramuza (Dano -30%) [CONTRA ASEDIO]" : "Escaramuza (Dano -30%)";
        const char* txtPk2 = (ctx->intentRevealed && ctx->enemyIntent == 0) ? "Linea de Picas (8+) (Tirada -1) [CONTRA CARGA]" : "Linea de Picas (8+) (Tirada -1)";
        const char* txtAs2 = (ctx->intentRevealed && ctx->enemyIntent == 2) ? "Asalto Frontal (6+) (Enemigo +2) [CONTRA REPLIEGUE]" : "Asalto Frontal (6+) (Enemigo +2)";
        const char* txtFl2 = (ctx->intentRevealed && ctx->enemyIntent == 1) ? "Flanqueo (9+ / Expuesto) (Tirada +4) [CONTRA ASEDIO]" : "Flanqueo (9+ / Expuesto) (Tirada +4)";

        if (ctx->previousPlan == PLAN_SKIRMISH) txtSk2 = TextFormat("%s [FATIGADO]", txtSk2);
        if (ctx->previousPlan == PLAN_PIKES) txtPk2 = TextFormat("%s [FATIGADO]", txtPk2);
        if (ctx->previousPlan == PLAN_ASSAULT) txtAs2 = TextFormat("%s [FATIGADO]", txtAs2);
        if (ctx->previousPlan == PLAN_FLANK) txtFl2 = TextFormat("%s [FATIGADO]", txtFl2);

                if (ctx->canSkirmish && UI_DrawButton((Rectangle){tx, middleY+85, 550, 35}, txtSk2, false)) { Combat_SelectPlan(ctx, PLAN_SKIRMISH); Combat_RollExecution(ctx, GetRandomValue(1,6), GetRandomValue(1,6)); }
        if (ctx->canPikes && UI_DrawButton((Rectangle){tx, middleY+125, 550, 35}, txtPk2, false)) { Combat_SelectPlan(ctx, PLAN_PIKES); Combat_RollExecution(ctx, GetRandomValue(1,6), GetRandomValue(1,6)); }
        if (ctx->canAssault && UI_DrawButton((Rectangle){tx, middleY+165, 550, 35}, txtAs2, false)) { Combat_SelectPlan(ctx, PLAN_ASSAULT); Combat_RollExecution(ctx, GetRandomValue(1,6), GetRandomValue(1,6)); }
        if (ctx->canFlank && UI_DrawButton((Rectangle){tx, middleY+205, 550, 35}, txtFl2, false)) { Combat_SelectPlan(ctx, PLAN_FLANK); Combat_RollExecution(ctx, GetRandomValue(1,6), GetRandomValue(1,6)); }    }
    else if (ctx->state == COMBAT_STATE_ROUND_CLOSE) {
        if (UI_DrawButton((Rectangle){tx, middleY+40, 250, 45}, "Cerrar Ronda", false)) { Combat_RoundClose(ctx); }
    }
    else if (ctx->state == COMBAT_STATE_END) {
        if (UI_DrawButton((Rectangle){tx, middleY+130, 250, 45}, "Hacer Recuento y Salir", false)) {
            Combat_Recount(ctx);
        }
    }
    
    return false;
}
