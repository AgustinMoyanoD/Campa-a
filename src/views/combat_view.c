#include "views/combat_view.h"
#include "controllers/game_master.h"
#include "presentation/sequence.h"
#include "presentation/tween.h"
#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"
#include "presentation/audio_manager.h"
#include <stdio.h>
#include <string.h>

static bool combatInitialized = false;
static bool isRollingDice = false;
static float rollTimer = 0.0f;
static int visualDice1 = 1;
static int visualDice2 = 1;
static float endTimer = 0.0f;
static bool s_combatHintDismissed = false;

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
    for(int i=0; i<sergeants; i++) { DrawRectangle((int)pos.x + (drawn%maxPerRow)*spacing, (int)pos.y + (drawn/maxPerRow)*spacing, 10, 10, color); drawn++; }
    for(int i=0; i<soldiers; i++) { DrawRectangle((int)pos.x + (drawn%maxPerRow)*spacing, (int)pos.y + (drawn/maxPerRow)*spacing, 8, 8, Fade(color, 0.8f)); drawn++; }
    for(int i=0; i<initiates; i++) { DrawRectangle((int)pos.x + (drawn%maxPerRow)*spacing, (int)pos.y + (drawn/maxPerRow)*spacing, 6, 6, Fade(color, 0.5f)); drawn++; }
}

/* Tarjeta individual de plan táctico (Sec B2) */
static bool DrawPlanCard(const CombatPlanInfo* info, Rectangle bounds) {
    if (!info) return false;

    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);
    bool clicked = false;

    Color bg;
    Color border;
    if (!info->isUnlocked) {
        bg = (Color){ 28, 28, 34, 230 };
        border = (Color){ 65, 65, 75, 255 };
    } else {
        if (hovered) {
            bg = (Color){ 46, 52, 68, 255 };
            border = (Color){ 240, 190, 80, 255 };
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                clicked = true;
            }
        } else {
            bg = (Color){ 36, 40, 52, 240 };
            border = (Color){ 85, 95, 120, 255 };
        }
    }

    DrawRectangleRec(bounds, bg);
    DrawRectangleLinesEx(bounds, (hovered && info->isUnlocked) ? 2.0f : 1.0f, border);

    /* 1. Cabecera: Nombre y Estado de Bloqueo / Tirada */
    float headerY = bounds.y + 7.0f;
    Color nameColor = info->isUnlocked ? (hovered ? (Color){ 255, 225, 120, 255 } : WHITE) : (Color){ 140, 140, 150, 255 };
    DrawText(info->name, (int)(bounds.x + 12.0f), (int)headerY, 15, nameColor);

    if (!info->isUnlocked) {
        const char* lockMsg = (info->plan == PLAN_FLANK) ? "[BLOQUEADO: Falta Flanco Expuesto]" : TextFormat("[BLOQUEADO: Falta %d]", info->pointsNeeded);
        float lockW = (float)MeasureText(lockMsg, 11);
        DrawRectangleRec((Rectangle){ bounds.x + bounds.width - lockW - 14.0f, headerY, lockW + 6.0f, 17.0f }, (Color){ 60, 25, 25, 220 });
        DrawRectangleLinesEx((Rectangle){ bounds.x + bounds.width - lockW - 14.0f, headerY, lockW + 6.0f, 17.0f }, 1.0f, (Color){ 180, 50, 50, 255 });
        DrawText(lockMsg, (int)(bounds.x + bounds.width - lockW - 11.0f), (int)(headerY + 2.0f), 11, (Color){ 240, 120, 120, 255 });
    } else if (info->requiredRoll > 0) {
        const char* okMsg = TextFormat("[REQ %d+ OK]", info->requiredRoll);
        float okW = (float)MeasureText(okMsg, 11);
        DrawRectangleRec((Rectangle){ bounds.x + bounds.width - okW - 14.0f, headerY, okW + 6.0f, 17.0f }, (Color){ 25, 55, 30, 220 });
        DrawRectangleLinesEx((Rectangle){ bounds.x + bounds.width - okW - 14.0f, headerY, okW + 6.0f, 17.0f }, 1.0f, (Color){ 60, 160, 70, 255 });
        DrawText(okMsg, (int)(bounds.x + bounds.width - okW - 11.0f), (int)(headerY + 2.0f), 11, (Color){ 120, 230, 130, 255 });
    }

    /* 2. Chips de Efecto y Estado con Tooltip del Glosario */
    float chipX = bounds.x + 12.0f;
    float chipY = bounds.y + 28.0f;

    if (info->rollModifier != 0) {
        const char* rollLbl = TextFormat("Tirada %+d", info->rollModifier);
        Rectangle cr = UI_DrawChipEx(rollLbl, "TIRADA", (Rectangle){ chipX, chipY, 0, 18 }, (Color){ 45, 45, 55, 230 }, (Color){ 220, 200, 140, 255 });
        chipX += cr.width + 6.0f;
    }

    if (info->clashMultiplier != 1.0f) {
        const char* clashLbl = (info->clashMultiplier < 1.0f) ? TextFormat("Choque -%.0f%%", (1.0f - info->clashMultiplier)*100.0f) : TextFormat("Choque +%.0f%%", (info->clashMultiplier - 1.0f)*100.0f);
        Rectangle cr = UI_DrawChipEx(clashLbl, "CHOQUE", (Rectangle){ chipX, chipY, 0, 18 }, (Color){ 50, 40, 45, 230 }, (Color){ 240, 160, 120, 255 });
        chipX += cr.width + 6.0f;
    }

    if (info->hasAdvantage) {
        Rectangle cr = UI_DrawChipEx("^ VENTAJA", "VENTAJA", (Rectangle){ chipX, chipY, 0, 18 }, (Color){ 25, 65, 35, 240 }, (Color){ 100, 240, 120, 255 });
        chipX += cr.width + 6.0f;
    } else if (info->hasDisadvantage) {
        Rectangle cr = UI_DrawChipEx("v DESVENTAJA", "DESVENTAJA", (Rectangle){ chipX, chipY, 0, 18 }, (Color){ 70, 25, 25, 240 }, (Color){ 250, 110, 110, 255 });
        chipX += cr.width + 6.0f;
    }

    if (info->isFatigued) {
        Rectangle cr = UI_DrawChipEx("CANSADO", "FATIGADO", (Rectangle){ chipX, chipY, 0, 18 }, (Color){ 65, 45, 20, 240 }, (Color){ 250, 180, 80, 255 });
        chipX += cr.width + 6.0f;
    }

    /* 3. Descripción concisa (1 línea máxima) */
    DrawText(info->description, (int)(bounds.x + 12.0f), (int)(bounds.y + 51.0f), 11, (Color){ 175, 175, 185, 255 });

    return clicked;
}

void CombatView_Draw(CombatContext* ctx, int SW, int SH) {
    if (!ctx) return;
    int panelW = 960; int panelH = 600;
    int px = (SW - panelW) / 2; int py = (SH - panelH) / 2;
    
    UI_DrawPanel((Rectangle){ (float)px, (float)py, (float)panelW, (float)panelH });
    UI_DrawText(TextFormat("Combate Tactico - Ronda %d", ctx->round), (Vector2){ (float)(px + 30), (float)(py + 18) }, 24, WHITE);
    
    int tx = px + 30; int ty = py + 60;
    
    // Barras de Salud y Tropas Propias
    UI_DrawText(TextFormat("Tus Fuerzas (Salud: %.0f)", ctx->playerHealth), (Vector2){ (float)tx, (float)ty }, 17, GREEN);
    UI_DrawChipEx("SALUD", "SALUD", (Rectangle){ (float)(tx + 205), (float)ty, 0, 18 }, (Color){ 25, 45, 30, 230 }, (Color){ 120, 220, 130, 255 });
    UI_DrawBar((Rectangle){ (float)tx, (float)(ty + 22), 260, 12 }, ctx->playerInitialHealth > 0 ? (ctx->playerHealth / ctx->playerInitialHealth) : 0.0f, GREEN, RED);
    DrawArmyIcons((Vector2){ (float)tx, (float)(ty + 40) }, ctx->playerArmy->iniciados - ctx->outOfActionInitiates, ctx->playerArmy->soldados - ctx->outOfActionSoldiers, ctx->playerArmy->sargentos - ctx->outOfActionSergeants, GREEN);
    
    // Enemigo
    int rightX = px + panelW - 320;
    UI_DrawText(TextFormat("Enemigo (Salud: %.0f)", ctx->enemyHealth), (Vector2){ (float)rightX, (float)ty }, 17, RED);
    UI_DrawChipEx("SALUD", "SALUD", (Rectangle){ (float)(rightX + 175), (float)ty, 0, 18 }, (Color){ 50, 25, 25, 230 }, (Color){ 240, 130, 130, 255 });
    UI_DrawBar((Rectangle){ (float)rightX, (float)(ty + 22), 260, 12 }, ctx->enemyInitialHealth > 0 ? (ctx->enemyHealth / ctx->enemyInitialHealth) : 0.0f, RED, GREEN);
    
    if (ctx->enemy->infoLevel >= 1) {
        UI_DrawText(TextFormat("Fuerza Base: %d", ctx->enemy->baseStrength), (Vector2){ (float)rightX, (float)(ty + 40) }, 15, GRAY);
    } else {
        UI_DrawText("Fuerza Base: ???", (Vector2){ (float)rightX, (float)(ty + 40) }, 15, GRAY);
    }
    
    if (ctx->enemy->infoLevel >= 2) {
        int tyo = 60;
        int chipXPos = rightX;
        if (ctx->enemy->tags & TAG_CAVALRY) {
            UI_DrawChip("CABALLERIA", (Rectangle){ (float)chipXPos, (float)(ty + tyo), 0, 18 });
            chipXPos += 85;
        }
        if (ctx->enemy->tags & TAG_AMBUSH) {
            UI_DrawChip("EMBOSCADA", (Rectangle){ (float)chipXPos, (float)(ty + tyo), 0, 18 });
            chipXPos += 85;
        }
        if (ctx->enemy->tags & TAG_EXPOSED_FLANK) {
            UI_DrawChip("EXPUESTO", (Rectangle){ (float)chipXPos, (float)(ty + tyo), 0, 18 });
            chipXPos += 85;
        }
        if (ctx->enemy->tags & TAG_HEAVILY_ARMORED) {
            UI_DrawChip("FORTIFICADO", (Rectangle){ (float)chipXPos, (float)(ty + tyo), 0, 18 });
            chipXPos += 95;
        }
        if (ctx->enemy->tags & TAG_DISORGANIZED) {
            UI_DrawChip("DESORGANIZADOS", (Rectangle){ (float)chipXPos, (float)(ty + tyo), 0, 18 });
        }
        if (chipXPos == rightX) UI_DrawText("Sin Tags", (Vector2){ (float)rightX, (float)(ty + tyo) }, 14, GRAY);
    } else {
        UI_DrawText("Tags: ??? (Requiere Evaluacion)", (Vector2){ (float)rightX, (float)(ty + 60) }, 14, GRAY);
    }
    
    int middleY = py + 235;
    
    // Feedback de tirada
    if (isRollingDice) {
        visualDice1 = GetRandomValue(1, 6);
        visualDice2 = GetRandomValue(1, 6);
        UI_DrawText(TextFormat("Tirando dados: [%d] [%d]", visualDice1, visualDice2), (Vector2){ (float)tx, (float)(middleY - 35) }, 26, GOLD);
    } else if (ctx->state == COMBAT_STATE_ROUND_CLOSE || ctx->state == COMBAT_STATE_END || ctx->state == COMBAT_STATE_RECOUNT || ctx->state == COMBAT_STATE_DONE) {
        if (ctx->margin > 0) {
            UI_DrawText(TextFormat("!Perdiste el choque! (Margen: -%d)", ctx->margin), (Vector2){ (float)tx, (float)(middleY - 45) }, 22, RED);
        } else if (ctx->margin < 0) {
            UI_DrawText(TextFormat("!Ganaste el choque! (Margen: +%d)", -ctx->margin), (Vector2){ (float)tx, (float)(middleY - 45) }, 22, GREEN);
        } else {
            UI_DrawText("!Empate en el choque!", (Vector2){ (float)tx, (float)(middleY - 45) }, 22, GRAY);
        }
        UI_DrawChipEx("MARGEN", "MARGEN", (Rectangle){ (float)(tx + 360), (float)(middleY - 43), 0, 18 }, (Color){ 45, 45, 55, 230 }, (Color){ 220, 200, 140, 255 });
        UI_DrawText(TextFormat("Tu Fuerza: %d  vs  Fuerza Enemiga: %d", ctx->playerScore, ctx->enemyScore), (Vector2){ (float)tx, (float)(middleY - 18) }, 17, LIGHTGRAY);
        UI_DrawChipEx("PUNTAJE", "PUNTAJE", (Rectangle){ (float)(tx + 330), (float)(middleY - 18), 0, 18 }, (Color){ 45, 45, 55, 230 }, (Color){ 220, 200, 140, 255 });

        if (ctx->breakdown.count > 0) {
            char bdText[256] = "Cartas Activas: ";
            for (int i=0; i<ctx->breakdown.count; i++) {
                char tmp[64];
                if (ctx->breakdown.entries[i].percent != 0) {
                    snprintf(tmp, sizeof(tmp), "%s%+d%% ", ctx->breakdown.entries[i].source, ctx->breakdown.entries[i].percent);
                } else {
                    snprintf(tmp, sizeof(tmp), "%s%+d ", ctx->breakdown.entries[i].source, ctx->breakdown.entries[i].flat);
                }
                int blen = (int)strlen(bdText);
                int tlen = (int)strlen(tmp);
                if (blen + tlen < 250) {
                    strcat(bdText, tmp);
                }
            }
            UI_DrawText(bdText, (Vector2){ (float)tx, (float)(middleY - 65) }, 15, GOLD);
        }
        if (ctx->criticalHit) {
            UI_DrawText("!! CRITICO EXCEPCIONAL !! (+5 Fuerza, x1.5 Dano)", (Vector2){ (float)tx, (float)(middleY - 88) }, 20, GOLD);
        } else if (ctx->criticalFumble) {
            UI_DrawText("... DESASTRE TACTICO ... (-5 Fuerza, x0.5 Dano)", (Vector2){ (float)tx, (float)(middleY - 88) }, 20, RED);
        }
    }
    
    if (Sequence_IsBusy()) return;
    
    if (ctx->state == COMBAT_STATE_EVALUATION) {
        UI_DrawText("Fase de Evaluacion", (Vector2){ (float)tx, (float)middleY }, 20, LIGHTGRAY);
        UI_DrawButton((Rectangle){ (float)tx, (float)(middleY+35), 240, 42 }, "Evaluar Terreno", false);
    }
    else if (ctx->state == COMBAT_STATE_PLANNING) {
        if (ctx->round == 1) {
            if (ctx->ambushed) {
                UI_DrawText("!EMBOSCADA! Sin evaluacion previa (-2 a Planificacion)", (Vector2){ (float)tx, (float)(middleY - 35) }, 20, RED);
            } else if (ctx->evalRoll >= 10) {
                UI_DrawText(TextFormat("Evaluacion Excelente (%d). !Todo revelado!", ctx->evalRoll), (Vector2){ (float)tx, (float)(middleY - 35) }, 20, GREEN);
            } else if (ctx->evalRoll >= 8) {
                UI_DrawText(TextFormat("Evaluacion Buena (%d). Fuerza revelada.", ctx->evalRoll), (Vector2){ (float)tx, (float)(middleY - 35) }, 20, GOLD);
            } else {
                UI_DrawText(TextFormat("Evaluacion Pobre (%d). El enemigo es un misterio.", ctx->evalRoll), (Vector2){ (float)tx, (float)(middleY - 35) }, 20, RED);
            }
        }
        UI_DrawText("Fase de Planificacion", (Vector2){ (float)tx, (float)middleY }, 20, LIGHTGRAY);
        if (ctx->intentRevealed) {
            const char* intents[] = {"Carga (Agresivo)", "Asedio (Defensivo)", "Repliegue (Tactico)"};
            UI_DrawText(TextFormat("Intencion Enemiga revelada: %s", intents[ctx->enemyIntent]), (Vector2){ (float)tx, (float)(middleY + 24) }, 16, ORANGE);
            UI_DrawChipEx("INTENCION", "INTENCION", (Rectangle){ (float)(tx + MeasureText(TextFormat("Intencion Enemiga revelada: %s", intents[ctx->enemyIntent]), 16) + 10), (float)(middleY + 22), 0, 18 }, (Color){ 60, 40, 20, 230 }, (Color){ 250, 180, 80, 255 });
        } else {
            UI_DrawText("Intencion Enemiga: ???", (Vector2){ (float)tx, (float)(middleY + 24) }, 16, GRAY);
        }
        UI_DrawButton((Rectangle){ (float)tx, (float)(middleY+45), 240, 42 }, "Tirar Dados de Mando", false);
    }
    else if (ctx->state == COMBAT_STATE_EXECUTION) {
        UI_DrawText(TextFormat("Planificacion (Tirada: %d) (Mando: %d). Elige tu Plan:", ctx->planRoll, ctx->playerArmy->commandTokens), (Vector2){ (float)tx, (float)(middleY - 5) }, 20, GOLD);
        UI_DrawChipEx("TIRADA", "TIRADA", (Rectangle){ (float)(tx + 360), (float)(middleY - 5), 0, 18 }, (Color){ 45, 45, 55, 230 }, (Color){ 220, 200, 140, 255 });
        if (ctx->intentRevealed) {
            const char* intents[] = {"Carga (Agresivo)", "Asedio (Defensivo)", "Repliegue (Tactico)"};
            UI_DrawText(TextFormat("Intencion Enemiga: %s", intents[ctx->enemyIntent]), (Vector2){ (float)(tx + 460), (float)(middleY - 5) }, 15, ORANGE);
            UI_DrawChipEx("INTENCION", "INTENCION", (Rectangle){ (float)(tx + 460 + MeasureText(TextFormat("Intencion Enemiga: %s", intents[ctx->enemyIntent]), 15) + 6), (float)(middleY - 5), 0, 18 }, (Color){ 60, 40, 20, 230 }, (Color){ 250, 180, 80, 255 });
        }

        // Fila de acciones auxiliares: Huir y Mando
        UI_DrawButton((Rectangle){ (float)tx, (float)(middleY + 22), 120, 32 }, "Huir", false);
        if (ctx->playerArmy->commandTokens > 0) {
            UI_DrawChipEx("MANDO", "MANDO", (Rectangle){ (float)(tx + 130), (float)(middleY + 28), 0, 20 }, (Color){ 65, 45, 20, 240 }, (Color){ 255, 215, 0, 255 });
            UI_DrawButton((Rectangle){ (float)(tx + 200), (float)(middleY + 22), 105, 32 }, "Mando: +2", false);
            UI_DrawButton((Rectangle){ (float)(tx + 312), (float)(middleY + 22), 115, 32 }, "Mando: Reroll", false);
        }

        // Tarjetas de Planes Tácticos en cuadrícula 2x2 (Sec B2)
        float cardW = (panelW - 60 - 16) / 2.0f;
        float cardH = 70.0f;
        Rectangle rSk = { (float)tx, (float)(middleY + 62), cardW, cardH };
        Rectangle rPk = { (float)(tx + cardW + 16), (float)(middleY + 62), cardW, cardH };
        Rectangle rAs = { (float)tx, (float)(middleY + 62 + cardH + 8), cardW, cardH };
        Rectangle rFl = { (float)(tx + cardW + 16), (float)(middleY + 62 + cardH + 8), cardW, cardH };

        CombatPlanInfo infoSk = Combat_GetPlanInfo(ctx, PLAN_SKIRMISH);
        CombatPlanInfo infoPk = Combat_GetPlanInfo(ctx, PLAN_PIKES);
        CombatPlanInfo infoAs = Combat_GetPlanInfo(ctx, PLAN_ASSAULT);
        CombatPlanInfo infoFl = Combat_GetPlanInfo(ctx, PLAN_FLANK);

        DrawPlanCard(&infoSk, rSk);
        DrawPlanCard(&infoPk, rPk);
        DrawPlanCard(&infoAs, rAs);
        DrawPlanCard(&infoFl, rFl);

        // Modificadores / Cartas activas
        ActiveMod ams[16];
        int amCount = Modifier_CollectActive(&ctx->gm->modRegistry, ctx->playerArmy, ctx->enemy, ams, 16);
        int cx = tx;
        int cy = middleY + 218;
        for (int i=0; i<amCount; i++) {
            if (ams[i].def->kind == MOD_KIND_CARD || ams[i].def->kind == MOD_KIND_CURSE) {
                Color c = (ams[i].def->kind == MOD_KIND_CURSE) ? RED : GOLD;
                UI_DrawText(TextFormat("[%s]", ams[i].def->name), (Vector2){ (float)cx, (float)cy }, 14, c);
                cx += 170;
                if (cx > tx + 500) { cx = tx; cy += 20; }
            }
        }
    }
    else if (ctx->state == COMBAT_STATE_ROUND_CLOSE) {
        UI_DrawButton((Rectangle){ (float)tx, (float)(middleY+40), 240, 42 }, "Cerrar Ronda", false);
    }
    else if (ctx->state == COMBAT_STATE_END || ctx->state == COMBAT_STATE_RECOUNT || ctx->state == COMBAT_STATE_DONE) {
        UI_DrawText("Fin del Combate", (Vector2){ (float)tx, (float)(middleY+15) }, 24, WHITE);
        
        Color resCol = GRAY;
        if (ctx->endReason == COMBAT_END_VICTORY) resCol = GOLD;
        if (ctx->endReason == COMBAT_END_DEFEAT || ctx->endReason == COMBAT_END_FLEE) resCol = RED;
        
        UI_DrawText(TextFormat("Resultado: %s", 
            ctx->endReason == COMBAT_END_VICTORY ? "VICTORIA" : 
            ctx->endReason == COMBAT_END_DEFEAT ? "DERROTA" : 
            ctx->endReason == COMBAT_END_FLEE ? "HUIDA" : "EMPATE"), 
            (Vector2){ (float)tx, (float)(middleY+48) }, 20, resCol);
            
        UI_DrawText(TextFormat("Fuera de combate: %d Iniciados, %d Soldados, %d Sargentos", ctx->outOfActionInitiates, ctx->outOfActionSoldiers, ctx->outOfActionSergeants), (Vector2){ (float)tx, (float)(middleY+78) }, 16, RED);
        UI_DrawChipEx("FUERA DE COMBATE", "FUERA_DE_COMBATE", (Rectangle){ (float)(tx + 450), (float)(middleY+76), 0, 18 }, (Color){ 55, 25, 25, 230 }, (Color){ 240, 120, 120, 255 });
        
        if (ctx->state == COMBAT_STATE_END) {
            UI_DrawButton((Rectangle){ (float)tx, (float)(middleY+115), 240, 42 }, "Hacer Recuento y Salir", false);
        } else if (ctx->state == COMBAT_STATE_DONE) {
            UI_DrawText(TextFormat("Muertes Confirmadas: %d Iniciados, %d Soldados, %d Sargentos", ctx->deadInitiates, ctx->deadSoldiers, ctx->deadSergeants), (Vector2){ (float)tx, (float)(middleY+110) }, 18, RED);
        }
    }

    // Pista descartable de primer uso (Sec B3)
    if (!s_combatHintDismissed) {
        Rectangle hintBounds = { (float)(px + 30), (float)(py + panelH - 30), (float)(panelW - 60), 20.0f };
        bool hintHover = CheckCollisionPointRec(GetMousePosition(), hintBounds);
        DrawRectangleRec(hintBounds, hintHover ? (Color){ 35, 45, 60, 240 } : (Color){ 25, 30, 42, 220 });
        DrawRectangleLinesEx(hintBounds, 1.0f, (Color){ 90, 120, 170, 200 });
        const char* hintMsg = "[PISTA] Pasa el raton sobre chips y tarjetas para ver el glosario. Clic aqui para cerrar.";
        DrawText(hintMsg, px + 38, py + panelH - 26, 11, (Color){ 200, 220, 255, 255 });
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
                act.duration = 0.5f;
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
    int tx = px + 30; int middleY = py + 235;

    // Descarte de pista con clic
    if (!s_combatHintDismissed) {
        Rectangle hintBounds = { (float)(px + 30), (float)(py + panelH - 30), (float)(panelW - 60), 20.0f };
        if (CheckCollisionPointRec(GetMousePosition(), hintBounds) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            s_combatHintDismissed = true;
        }
    }
    
    if (ctx->state == COMBAT_STATE_EVALUATION) {
        if (UI_ButtonClicked((Rectangle){ (float)tx, (float)(middleY+35), 240, 42 })) {
            Combat_RollEvaluation(ctx, GetRandomValue(1,6), GetRandomValue(1,6));
        }
    }
    else if (ctx->state == COMBAT_STATE_PLANNING) {
        if (UI_ButtonClicked((Rectangle){ (float)tx, (float)(middleY+45), 240, 42 })) {
            Combat_RollPlanning(ctx, GetRandomValue(1,6), GetRandomValue(1,6));
        }
    }
    else if (ctx->state == COMBAT_STATE_EXECUTION) {
        // Huir
        if (UI_ButtonClicked((Rectangle){ (float)tx, (float)(middleY + 22), 120, 32 })) {
            Combat_SelectPlan(ctx, PLAN_FLEE);
            Combat_RollExecution(ctx, GetRandomValue(1,6), GetRandomValue(1,6));
        }

        // Fichas de Mando
        if (ctx->playerArmy->commandTokens > 0) {
            if (UI_ButtonClicked((Rectangle){ (float)(tx + 200), (float)(middleY + 22), 105, 32 })) {
                Combat_UseCommandToken_Add(ctx);
            }
            if (UI_ButtonClicked((Rectangle){ (float)(tx + 312), (float)(middleY + 22), 115, 32 })) {
                Combat_UseCommandToken_Reroll(ctx, GetRandomValue(1,6), GetRandomValue(1,6));
            }
        }

        // Tarjetas de Planes Tácticos
        float cardW = (panelW - 60 - 16) / 2.0f;
        float cardH = 70.0f;
        Rectangle rSk = { (float)tx, (float)(middleY + 62), cardW, cardH };
        Rectangle rPk = { (float)(tx + cardW + 16), (float)(middleY + 62), cardW, cardH };
        Rectangle rAs = { (float)tx, (float)(middleY + 62 + cardH + 8), cardW, cardH };
        Rectangle rFl = { (float)(tx + cardW + 16), (float)(middleY + 62 + cardH + 8), cardW, cardH };

        CombatPlanInfo infoSk = Combat_GetPlanInfo(ctx, PLAN_SKIRMISH);
        CombatPlanInfo infoPk = Combat_GetPlanInfo(ctx, PLAN_PIKES);
        CombatPlanInfo infoAs = Combat_GetPlanInfo(ctx, PLAN_ASSAULT);
        CombatPlanInfo infoFl = Combat_GetPlanInfo(ctx, PLAN_FLANK);

        if (infoSk.isUnlocked && UI_ButtonClicked(rSk)) {
            Combat_SelectPlan(ctx, PLAN_SKIRMISH);
            Combat_RollExecution(ctx, GetRandomValue(1,6), GetRandomValue(1,6));
        }
        else if (infoPk.isUnlocked && UI_ButtonClicked(rPk)) {
            Combat_SelectPlan(ctx, PLAN_PIKES);
            Combat_RollExecution(ctx, GetRandomValue(1,6), GetRandomValue(1,6));
        }
        else if (infoAs.isUnlocked && UI_ButtonClicked(rAs)) {
            Combat_SelectPlan(ctx, PLAN_ASSAULT);
            Combat_RollExecution(ctx, GetRandomValue(1,6), GetRandomValue(1,6));
        }
        else if (infoFl.isUnlocked && UI_ButtonClicked(rFl)) {
            Combat_SelectPlan(ctx, PLAN_FLANK);
            Combat_RollExecution(ctx, GetRandomValue(1,6), GetRandomValue(1,6));
        }
    }
    else if (ctx->state == COMBAT_STATE_ROUND_CLOSE) {
        if (UI_ButtonClicked((Rectangle){ (float)tx, (float)(middleY+40), 240, 42 })) {
            Combat_RoundClose(ctx);
        }
    }
    else if (ctx->state == COMBAT_STATE_END) {
        if (UI_ButtonClicked((Rectangle){ (float)tx, (float)(middleY+115), 240, 42 })) {
            Combat_Recount(ctx);
        }
    }
    
    return false;
}
