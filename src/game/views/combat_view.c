#include "views/combat_view.h"
#include "controllers/game_master.h"
#include "presentation/sequence.h"
#include "presentation/tween.h"
#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"
#include "presentation/audio_manager.h"
#include "presentation/screen_fx.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static bool combatInitialized = false;

typedef enum {
    CROLL_NONE = 0,
    CROLL_EVALUATION,
    CROLL_PLANNING,
    CROLL_REROLL,
    CROLL_EXECUTION
} CombatRollType;

static CombatRollType s_rollType = CROLL_NONE;
static float s_rollTimer = 0.0f;
static float s_rollDuration = 0.80f;
static float s_soundTickTimer = 0.0f;
static int visualDice1 = 1;
static int visualDice2 = 1;
static int s_targetD1 = 1;
static int s_targetD2 = 1;
static CombatPlan s_pendingPlan = PLAN_NONE;

static bool s_hasRolledDice = false;
static int s_lastD1 = 1;
static int s_lastD2 = 1;

static float s_displayPlayerHealth = -1.0f;
static float s_displayEnemyHealth = -1.0f;

static float endTimer = 0.0f;
static bool s_combatHintDismissed = false;

#define MAX_FLOATING_TEXTS 8
typedef struct {
    char text[64];
    Vector2 pos;
    Color color;
    float timer;
    float maxTime;
    int fontSize;
    bool active;
} CombatFloatText;

static CombatFloatText s_floatTexts[MAX_FLOATING_TEXTS] = {0};

static void AddCombatFloatingText(const char* text, Vector2 pos, Color color, int fontSize) {
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        if (!s_floatTexts[i].active) {
            strncpy(s_floatTexts[i].text, text, sizeof(s_floatTexts[i].text)-1);
            s_floatTexts[i].pos = pos;
            s_floatTexts[i].color = color;
            s_floatTexts[i].timer = 1.5f;
            s_floatTexts[i].maxTime = 1.5f;
            s_floatTexts[i].fontSize = fontSize;
            s_floatTexts[i].active = true;
            return;
        }
    }
}

static void UpdateAndDrawFloatingTexts(float dt) {
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        if (!s_floatTexts[i].active) continue;
        s_floatTexts[i].timer -= dt;
        if (s_floatTexts[i].timer <= 0.0f) {
            s_floatTexts[i].active = false;
            continue;
        }
        s_floatTexts[i].pos.y -= 28.0f * dt;
        float alpha = s_floatTexts[i].timer / s_floatTexts[i].maxTime;
        if (alpha > 1.0f) alpha = 1.0f;
        Color c = s_floatTexts[i].color;
        c.a = (unsigned char)(255 * alpha);
        // Shadow
        UI_DrawText(s_floatTexts[i].text, (Vector2){ s_floatTexts[i].pos.x + 2.0f, s_floatTexts[i].pos.y + 2.0f }, s_floatTexts[i].fontSize, (Color){ 0, 0, 0, (unsigned char)(200 * alpha) });
        // Text
        UI_DrawText(s_floatTexts[i].text, s_floatTexts[i].pos, s_floatTexts[i].fontSize, c);
    }
}

static void TriggerCombatFeedback(CombatContext* ctx, int px, int py) {
    if (!ctx) return;
    int tx = px + 30;
    int rightX = px + 980 - 340;
    int ty = py + 60;

    // Detectar dano exacto registrado en el log
    float lostDmg = 0.0f;
    int dmgSide = -1; // 0 = Player, 1 = Enemy
    for (int i = 0; i < ctx->log.count; i++) {
        if (ctx->log.steps[i].type == STEP_DAMAGE) {
            lostDmg = ctx->log.steps[i].as.damage.lostHealth;
            dmgSide = ctx->log.steps[i].as.damage.side;
            break;
        }
    }

    // Si no se encontro STEP_DAMAGE pero el margen determina dano:
    if (dmgSide == -1) {
        if (ctx->margin > 0) dmgSide = 0;
        else dmgSide = 1;
    }

    // Reproduccion garantizada de SFX de impacto segun quien recibe dano
    if (dmgSide == 1) {
        // El jugador inflige dano al enemigo
        Audio_PlaySfx(SFX_DAMAGE_DEALT);
        if (ctx->criticalHit) {
            Audio_PlaySfx(SFX_CRITICAL_HIT);
        }
    } else if (dmgSide == 0) {
        // El jugador recibe dano
        Audio_PlaySfx(SFX_DAMAGE_TAKEN);
    }

    // Efectos visuales en pantalla y textos flotantes
    if (ctx->criticalHit) {
        ScreenFX_Shake(0.5f, 15.0f);
        ScreenFX_Flash(0.25f, (Color){ 255, 230, 80, 150 });
        AddCombatFloatingText("! IMPACTO CRITICO !", (Vector2){ (float)(px + 280), (float)(py + 180) }, GOLD, 26);
    } else if (ctx->criticalFumble) {
        ScreenFX_Shake(0.5f, 15.0f);
        ScreenFX_Flash(0.28f, (Color){ 220, 20, 20, 150 });
        AddCombatFloatingText("! DESASTRE TACTICO !", (Vector2){ (float)(px + 280), (float)(py + 180) }, RED, 26);
    } else if (ctx->margin > 0) { // Player lost clash
        ScreenFX_Shake(0.35f, 9.0f);
        ScreenFX_Flash(0.18f, (Color){ 180, 25, 25, 110 });
        AddCombatFloatingText(TextFormat("! CHOQUE PERDIDO (-%d) !", ctx->margin), (Vector2){ (float)(px + 280), (float)(py + 180) }, (Color){ 255, 110, 110, 255 }, 24);
    } else if (ctx->margin < 0) { // Player won clash
        ScreenFX_Shake(0.25f, 7.0f);
        ScreenFX_Flash(0.15f, (Color){ 220, 190, 50, 90 });
        AddCombatFloatingText(TextFormat("! CHOQUE GANADO (+%d) !", -ctx->margin), (Vector2){ (float)(px + 280), (float)(py + 180) }, (Color){ 120, 240, 130, 255 }, 24);
    } else {
        ScreenFX_Shake(0.15f, 4.0f);
        AddCombatFloatingText("! EMPATE EN EL CHOQUE !", (Vector2){ (float)(px + 280), (float)(py + 180) }, LIGHTGRAY, 22);
    }

    if (lostDmg > 0.0f) {
        if (dmgSide == 0) {
            AddCombatFloatingText(TextFormat("-%.0f Salud", lostDmg), (Vector2){ (float)(tx + 80), (float)(ty + 6) }, (Color){ 255, 75, 75, 255 }, 22);
        } else if (dmgSide == 1) {
            AddCombatFloatingText(TextFormat("-%.0f Salud", lostDmg), (Vector2){ (float)(rightX + 60), (float)(ty + 6) }, (Color){ 255, 120, 60, 255 }, 22);
        }
    }
}

static void DrawArmyIcons(Vector2 pos, int initiates, int soldiers, int sergeants, Color color) {
    int maxPerRow = 20;
    int spacing = 12;
    int drawn = 0;
    for(int i=0; i<sergeants; i++) { DrawRectangle((int)pos.x + (drawn%maxPerRow)*spacing, (int)pos.y + (drawn/maxPerRow)*spacing, 10, 10, color); drawn++; }
    for(int i=0; i<soldiers; i++) { DrawRectangle((int)pos.x + (drawn%maxPerRow)*spacing, (int)pos.y + (drawn/maxPerRow)*spacing, 8, 8, Fade(color, 0.8f)); drawn++; }
    for(int i=0; i<initiates; i++) { DrawRectangle((int)pos.x + (drawn%maxPerRow)*spacing, (int)pos.y + (drawn/maxPerRow)*spacing, 6, 6, Fade(color, 0.5f)); drawn++; }
}

/* Tarjeta individual de plan tÃ¡ctico (Sec B2) */
static bool DrawPlanCard(const CombatPlanInfo* info, Rectangle bounds) {
    if (!info) return false;

    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);
    bool clicked = false;

    UI_DrawPanelStyled(bounds, UI_STYLE_PANEL_DARK);
    if (hovered && info->isUnlocked) {
        UISkin_DrawHighlight(bounds, 0.45f, 0);
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            clicked = true;
        }
    }

    /* 1. Cabecera: Nombre y Estado de Bloqueo / Tirada */
    float headerY = bounds.y + 7.0f;
    Color nameColor = info->isUnlocked ? (hovered ? (Color){ 255, 225, 120, 255 } : WHITE) : (Color){ 140, 140, 150, 255 };
    UI_DrawText(info->name, (Vector2){ bounds.x + 12.0f, headerY }, 16, nameColor);

    if (!info->isUnlocked) {
        const char* lockMsg = (info->plan == PLAN_FLANK) ? "BLOQUEADO: Falta Flanco" : TextFormat("BLOQUEADO: Falta %d", info->pointsNeeded);
        Vector2 lockSz = UI_MeasureText(lockMsg, 12);
        float lockW = lockSz.x + 32.0f;
        UI_DrawStatusChip((Rectangle){ bounds.x + bounds.width - lockW - 10.0f, headerY - 1.0f, lockW, 20.0f }, lockMsg, UI_STYLE_CHIP_BAD, UI_ICON_PELIGRO);
    } else if (info->requiredRoll > 0) {
        const char* okMsg = TextFormat("REQ %d+ OK", info->requiredRoll);
        Vector2 okSz = UI_MeasureText(okMsg, 12);
        float okW = okSz.x + 32.0f;
        UI_DrawStatusChip((Rectangle){ bounds.x + bounds.width - okW - 10.0f, headerY - 1.0f, okW, 20.0f }, okMsg, UI_STYLE_CHIP_OK, UI_ICON_OK);
    }

    /* 2. Chips de Efecto y Estado con Tooltip del Glosario */
    float chipX = bounds.x + 12.0f;
    float chipY = bounds.y + 30.0f;

    if (info->rollModifier != 0) {
        const char* rollLbl = TextFormat("Tirada %+d", info->rollModifier);
        Rectangle cr = UI_DrawChipEx(rollLbl, "TIRADA", (Rectangle){ chipX, chipY, 0, 20 }, (Color){ 45, 45, 55, 230 }, (Color){ 220, 200, 140, 255 });
        chipX += cr.width + 6.0f;
    }

    if (info->clashMultiplier != 1.0f) {
        const char* clashLbl = (info->clashMultiplier < 1.0f) ? TextFormat("Choque -%.0f%%", (1.0f - info->clashMultiplier)*100.0f) : TextFormat("Choque +%.0f%%", (info->clashMultiplier - 1.0f)*100.0f);
        Rectangle cr = UI_DrawChipEx(clashLbl, "CHOQUE", (Rectangle){ chipX, chipY, 0, 20 }, (Color){ 50, 40, 45, 230 }, (Color){ 240, 160, 120, 255 });
        chipX += cr.width + 6.0f;
    }

    if (info->hasAdvantage) {
        Rectangle cr = UI_DrawChipEx("^ VENTAJA", "VENTAJA", (Rectangle){ chipX, chipY, 0, 20 }, (Color){ 25, 65, 35, 240 }, (Color){ 100, 240, 120, 255 });
        chipX += cr.width + 6.0f;
    } else if (info->hasDisadvantage) {
        Rectangle cr = UI_DrawChipEx("v DESVENTAJA", "DESVENTAJA", (Rectangle){ chipX, chipY, 0, 20 }, (Color){ 70, 25, 25, 240 }, (Color){ 250, 110, 110, 255 });
        chipX += cr.width + 6.0f;
    }

    if (info->isFatigued) {
        Rectangle cr = UI_DrawChipEx("CANSADO", "FATIGADO", (Rectangle){ chipX, chipY, 0, 20 }, (Color){ 65, 45, 20, 240 }, (Color){ 250, 180, 80, 255 });
        chipX += cr.width + 6.0f;
    }

    /* 3. DescripciÃ³n concisa */
    UI_DrawText(info->description, (Vector2){ bounds.x + 12.0f, bounds.y + 54.0f }, 12, (Color){ 175, 175, 185, 255 });

    return clicked;
}

void CombatView_Draw(CombatContext* ctx, int SW, int SH) {
    if (!ctx) return;
    int panelW = 980; int panelH = 630;
    Vector2 sOff = ScreenFX_GetShakeOffset();
    int px = (SW - panelW) / 2 + (int)sOff.x;
    int py = (SH - panelH) / 2 + (int)sOff.y;
    
    UI_DrawPanel((Rectangle){ (float)px, (float)py, (float)panelW, (float)panelH });
    UI_DrawText(TextFormat("Combate Tactico - Ronda %d", ctx->round), (Vector2){ (float)(px + 30), (float)(py + 18) }, 26, WHITE);
    
    int tx = px + 30; int ty = py + 60;
    
    // Barras de Salud y Tropas Propias
    float dispPH = (s_displayPlayerHealth >= 0.0f) ? s_displayPlayerHealth : ctx->playerHealth;
    float dispEH = (s_displayEnemyHealth >= 0.0f) ? s_displayEnemyHealth : ctx->enemyHealth;

    UI_DrawText(TextFormat("Tus Fuerzas (Salud: %.0f)", dispPH), (Vector2){ (float)tx, (float)ty }, 19, GREEN);
    UI_DrawChipEx("SALUD", "SALUD", (Rectangle){ (float)(tx + 225), (float)ty, 0, 20 }, (Color){ 25, 45, 30, 230 }, (Color){ 120, 220, 130, 255 });
    UI_DrawBar((Rectangle){ (float)tx, (float)(ty + 24), 280, 16 }, ctx->playerInitialHealth > 0 ? (dispPH / ctx->playerInitialHealth) : 0.0f, GREEN, RED);
    DrawArmyIcons((Vector2){ (float)tx, (float)(ty + 46) }, ctx->playerArmy->iniciados - ctx->outOfActionInitiates, ctx->playerArmy->soldados - ctx->outOfActionSoldiers, ctx->playerArmy->sargentos - ctx->outOfActionSergeants, GREEN);
    
    // Enemigo
    int rightX = px + panelW - 340;
    UI_DrawText(TextFormat("Enemigo (Salud: %.0f)", dispEH), (Vector2){ (float)rightX, (float)ty }, 19, RED);
    UI_DrawChipEx("SALUD", "SALUD", (Rectangle){ (float)(rightX + 195), (float)ty, 0, 20 }, (Color){ 50, 25, 25, 230 }, (Color){ 240, 130, 130, 255 });
    UI_DrawBar((Rectangle){ (float)rightX, (float)(ty + 22), 260, 12 }, ctx->enemyInitialHealth > 0 ? (dispEH / ctx->enemyInitialHealth) : 0.0f, RED, GREEN);
    
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
    
    int middleY = py + 245;
    
    // Feedback de tirada animada con dados reales de marfil
    if (s_rollType != CROLL_NONE) {
        int dieSize = 54;
        int diceCenterX = px + panelW / 2;
        int diceY = middleY - 50;

        // Vibracion / Jitter durante el lanzamiento
        int jitter1 = (int)(sinf((float)GetTime() * 45.0f) * 3.0f);
        int jitter2 = (int)(cosf((float)GetTime() * 40.0f) * 3.0f);

        UI_DrawSingleDie(diceCenterX - 36, diceY + jitter1, dieSize, visualDice1);
        UI_DrawSingleDie(diceCenterX + 36, diceY + jitter2, dieSize, visualDice2);

        const char* rollTitle = (s_rollType == CROLL_EXECUTION) ? "Â¡LANZANDO DADOS DE CHOQUE!" :
                                ((s_rollType == CROLL_PLANNING) ? "Lanzando Dados de Tactica..." :
                                ((s_rollType == CROLL_EVALUATION) ? "Lanzando Dados de Evaluacion..." : "Relanzando Dados..."));
        Vector2 rsz = UI_MeasureText(rollTitle, 22.0f);
        UI_DrawText(rollTitle, (Vector2){ (float)(diceCenterX - rsz.x / 2.0f), (float)(diceY - 38) }, 22.0f, UI_COLOR_GOLD_LIGHT);
    } else if (ctx->state == COMBAT_STATE_ROUND_CLOSE || ctx->state == COMBAT_STATE_END || ctx->state == COMBAT_STATE_RECOUNT || ctx->state == COMBAT_STATE_DONE) {
        // Dados asentados del choque en la esquina derecha del panel central
        if (s_hasRolledDice) {
            int dieSize = 42;
            int diceBaseX = px + panelW - 165;
            int diceBaseY = middleY - 54;
            UI_DrawSingleDie(diceBaseX, diceBaseY, dieSize, s_lastD1);
            UI_DrawSingleDie(diceBaseX + 50, diceBaseY, dieSize, s_lastD2);
            char diceLbl[64];
            snprintf(diceLbl, sizeof(diceLbl), "Tirada: %d + %d = %d", s_lastD1, s_lastD2, s_lastD1 + s_lastD2);
            Vector2 dsz = UI_MeasureText(diceLbl, 13.0f);
            UI_DrawText(diceLbl, (Vector2){ (float)(diceBaseX + 46 - dsz.x / 2.0f), (float)(diceBaseY + dieSize/2 + 8) }, 13.0f, UI_COLOR_GOLD_LIGHT);
        }

        if (ctx->margin > 0) {
            UI_DrawText(TextFormat("!Perdiste el choque! (Margen: -%d)", ctx->margin), (Vector2){ (float)tx, (float)(middleY - 45) }, 24, RED);
        } else if (ctx->margin < 0) {
            UI_DrawText(TextFormat("!Ganaste el choque! (Margen: +%d)", -ctx->margin), (Vector2){ (float)tx, (float)(middleY - 45) }, 24, GREEN);
        } else {
            UI_DrawText("!Empate en el choque!", (Vector2){ (float)tx, (float)(middleY - 45) }, 24, GRAY);
        }
        UI_DrawChipEx("MARGEN", "MARGEN", (Rectangle){ (float)(tx + 380), (float)(middleY - 43), 0, 20 }, (Color){ 45, 45, 55, 230 }, (Color){ 220, 200, 140, 255 });
        if (ctx->enemyRoll > 0) {
            UI_DrawText(TextFormat("Tu Fuerza: %d  vs  Fuerza Enemiga: %d (Dado: %d)", ctx->playerScore, ctx->enemyScore, ctx->enemyRoll), (Vector2){ (float)tx, (float)(middleY - 18) }, 18, LIGHTGRAY);
        } else {
            UI_DrawText(TextFormat("Tu Fuerza: %d  vs  Fuerza Enemiga: %d", ctx->playerScore, ctx->enemyScore), (Vector2){ (float)tx, (float)(middleY - 18) }, 18, LIGHTGRAY);
        }
        UI_DrawChipEx("PUNTAJE", "PUNTAJE", (Rectangle){ (float)(tx + 410), (float)(middleY - 18), 0, 20 }, (Color){ 45, 45, 55, 230 }, (Color){ 220, 200, 140, 255 });

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
            UI_DrawText(bdText, (Vector2){ (float)tx, (float)(middleY - 68) }, 16, GOLD);
        }
        if (ctx->criticalHit) {
            UI_DrawText("!! CRITICO EXCEPCIONAL !! (+5 Fuerza, x1.5 Dano)", (Vector2){ (float)tx, (float)(middleY - 92) }, 22, GOLD);
        } else if (ctx->criticalFumble) {
            UI_DrawText("... DESASTRE TACTICO ... (-5 Fuerza, x0.5 Dano)", (Vector2){ (float)tx, (float)(middleY - 92) }, 22, RED);
        }
    }
    
    if (s_rollType != CROLL_NONE) return;
    if (Sequence_IsBusy()) return;
    
    if (ctx->state == COMBAT_STATE_EVALUATION) {
        UI_DrawText("Fase de Evaluacion", (Vector2){ (float)tx, (float)middleY }, 22, LIGHTGRAY);
        UI_DrawButtonSfx((Rectangle){ (float)tx, (float)(middleY+38), 260, 44 }, "Evaluar Terreno", false, NULL);
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
        UI_DrawText("Fase de Planificacion", (Vector2){ (float)tx, (float)middleY }, 22, LIGHTGRAY);
        if (ctx->intentRevealed) {
            const char* intents[] = {"Carga (Agresivo)", "Asedio (Defensivo)", "Repliegue (Tactico)"};
            const char* iText = TextFormat("Intencion Enemiga revelada: %s", intents[ctx->enemyIntent]);
            UI_DrawText(iText, (Vector2){ (float)tx, (float)(middleY + 26) }, 16, ORANGE);
            UI_DrawChipEx("INTENCION", "INTENCION", (Rectangle){ (float)(tx + UI_MeasureText(iText, 16).x + 10), (float)(middleY + 24), 0, 20 }, (Color){ 60, 40, 20, 230 }, (Color){ 250, 180, 80, 255 });
        } else {
            UI_DrawText("Intencion Enemiga: ???", (Vector2){ (float)tx, (float)(middleY + 26) }, 16, GRAY);
        }
        UI_DrawButtonSfx((Rectangle){ (float)tx, (float)(middleY+50), 260, 44 }, "Tirar Dados de Mando", false, NULL);
    }
    else if (ctx->state == COMBAT_STATE_EXECUTION) {
        const char* planHdr = TextFormat("Planificacion (Tirada: %d) (Mando: %d). Elige tu Plan:", ctx->planRoll, ctx->playerArmy->commandTokens);
        Vector2 planHdrSz = UI_MeasureText(planHdr, 20);
        UI_DrawText(planHdr, (Vector2){ (float)tx, (float)(middleY - 5) }, 20, GOLD);
        UI_DrawChipEx("TIRADA", "TIRADA", (Rectangle){ (float)(tx + planHdrSz.x + 12), (float)(middleY - 5), 0, 20 }, (Color){ 45, 45, 55, 230 }, (Color){ 220, 200, 140, 255 });
        if (ctx->intentRevealed) {
            const char* intents[] = {"Carga (Agresivo)", "Asedio (Defensivo)", "Repliegue (Tactico)"};
            const char* iText2 = TextFormat("Intencion: %s", intents[ctx->enemyIntent]);
            float intentX = tx + planHdrSz.x + 95.0f;
            UI_DrawText(iText2, (Vector2){ intentX, (float)(middleY - 5) }, 16, ORANGE);
            UI_DrawChipEx("INTENCION", "INTENCION", (Rectangle){ intentX + UI_MeasureText(iText2, 16).x + 8, (float)(middleY - 5), 0, 20 }, (Color){ 60, 40, 20, 230 }, (Color){ 250, 180, 80, 255 });
        }

        // Fila de acciones auxiliares: Huir y Mando
        UI_DrawButtonSfx((Rectangle){ (float)tx, (float)(middleY + 24), 130, 34 }, "Huir", false, NULL);
        if (ctx->playerArmy->commandTokens > 0) {
            UI_DrawChipEx("MANDO", "MANDO", (Rectangle){ (float)(tx + 140), (float)(middleY + 30), 0, 22 }, (Color){ 65, 45, 20, 240 }, (Color){ 255, 215, 0, 255 });
            UI_DrawButtonSfx((Rectangle){ (float)(tx + 215), (float)(middleY + 24), 115, 34 }, "Mando: +2", false, NULL);
            UI_DrawButtonSfx((Rectangle){ (float)(tx + 335), (float)(middleY + 24), 125, 34 }, "Mando: Reroll", false, NULL);
        }

        // Tarjetas de Planes TÃ¡cticos en cuadrÃ­cula 2x2 (Sec B2)
        float cardW = (panelW - 60 - 16) / 2.0f;
        float cardH = 76.0f;
        Rectangle rSk = { (float)tx, (float)(middleY + 66), cardW, cardH };
        Rectangle rPk = { (float)(tx + cardW + 16), (float)(middleY + 66), cardW, cardH };
        Rectangle rAs = { (float)tx, (float)(middleY + 66 + cardH + 10), cardW, cardH };
        Rectangle rFl = { (float)(tx + cardW + 16), (float)(middleY + 66 + cardH + 10), cardW, cardH };

        CombatPlanInfo infoSk = Combat_GetPlanInfo(ctx, PLAN_SKIRMISH);
        CombatPlanInfo infoPk = Combat_GetPlanInfo(ctx, PLAN_PIKES);
        CombatPlanInfo infoAs = Combat_GetPlanInfo(ctx, PLAN_ASSAULT);
        CombatPlanInfo infoFl = Combat_GetPlanInfo(ctx, PLAN_FLANK);

        DrawPlanCard(&infoSk, rSk);
        DrawPlanCard(&infoPk, rPk);
        DrawPlanCard(&infoAs, rAs);
        DrawPlanCard(&infoFl, rFl);

        // Invocación Divina activa si el comandante tiene una carta divina no agotada
        int availableDivineSlot = -1;
        const ModifierDef* availableDivineDef = NULL;
        if (ctx->playerArmy && ctx->playerArmy->commander.ptr && ctx->gm) {
            for (int c = 0; c < 8; c++) {
                ModifierInstance* inst = Commander_GetCardInstance(ctx->playerArmy->commander, c);
                if (!inst) break;
                if (!inst->isExhausted) {
                    const ModifierDef* def = Modifier_GetById(ctx->gm->modRegistry.defs, ctx->gm->modRegistry.count, inst->id);
                    if (def && def->hasInvocation) {
                        availableDivineSlot = c;
                        availableDivineDef = def;
                        break;
                    }
                }
            }
        }

        int modY = middleY + 236;
        if (availableDivineDef != NULL) {
            Rectangle rInv = { (float)tx, (float)(middleY + 66 + (cardH + 10) * 2), (float)(panelW - 60), 66.0f };
            UI_DrawPanelStyled(rInv, UI_STYLE_PANEL_DARK);
            UISkin_DrawGlow(rInv, UI_COLOR_GOLD, 0.38f, 0);
            UISkin_DrawHighlight(rInv, 0.75f, 1);

            /* Relieve sacro / rúnico decorativo de fondo */
            DrawPolyLinesEx((Vector2){ rInv.x + rInv.width - 28.0f, rInv.y + 33.0f }, 4, 18.0f, 0.0f, 1.5f, Fade(UI_COLOR_GOLD, 0.22f));
            DrawPolyLinesEx((Vector2){ rInv.x + rInv.width - 28.0f, rInv.y + 33.0f }, 4, 11.0f, 45.0f, 1.0f, Fade(UI_COLOR_GOLD, 0.16f));

            Vector2 mouse = GetMousePosition();
            bool hovered = CheckCollisionPointRec(mouse, rInv);
            if (hovered) UISkin_DrawHighlight(rInv, 0.95f, 0);

            const char* invTitle = TextFormat("[INVOCACION DIVINA] %s (%s)", availableDivineDef->invocationName, availableDivineDef->name);
            UI_DrawText(invTitle, (Vector2){ rInv.x + 12.0f, rInv.y + 5.0f }, 15.0f, UI_COLOR_GOLD_LIGHT);

            float chipX = rInv.x + 12.0f;
            float chipY = rInv.y + 24.0f;

            if (availableDivineDef->invocationRollMod != 0) {
                Rectangle cr = UI_DrawChipEx(TextFormat("[TIRADA %+d]", availableDivineDef->invocationRollMod), "TIRADA", (Rectangle){ chipX, chipY, 0, 18 }, (Color){ 45, 45, 55, 230 }, (Color){ 220, 200, 140, 255 });
                chipX += cr.width + 6.0f;
            }
            if (availableDivineDef->invocationClashMult != 1.0f) {
                const char* clashLbl = (availableDivineDef->invocationClashMult < 1.0f) ?
                    TextFormat("[CHOQUE -%.0f%%]", (1.0f - availableDivineDef->invocationClashMult)*100.0f) :
                    TextFormat("[CHOQUE +%.0f%%]", (availableDivineDef->invocationClashMult - 1.0f)*100.0f);
                Rectangle cr = UI_DrawChipEx(clashLbl, "CHOQUE", (Rectangle){ chipX, chipY, 0, 18 }, (Color){ 50, 40, 45, 230 }, (Color){ 240, 160, 120, 255 });
                chipX += cr.width + 6.0f;
            }
            if (availableDivineDef->invocationHighVariance) {
                Rectangle cr = UI_DrawChipEx("[ALTA VARIANZA]", "VARIANZA", (Rectangle){ chipX, chipY, 0, 18 }, (Color){ 75, 40, 20, 230 }, (Color){ 255, 180, 80, 255 });
                if (CheckCollisionPointRec(mouse, cr)) {
                    UI_RequestTooltip("ALTA VARIANZA DIVINA", "Milagro inestable: dados pares suman +4 a la tirada, dados impares restan -3.", mouse);
                }
                chipX += cr.width + 6.0f;
            }
            if (ctx->intentRevealed && availableDivineDef->invocationCounterIntent >= 0 && availableDivineDef->invocationCounterIntent == ctx->enemyIntent) {
                Rectangle cr = UI_DrawChipEx("[^ VENTAJA]", "VENTAJA", (Rectangle){ chipX, chipY, 0, 18 }, (Color){ 25, 65, 35, 240 }, (Color){ 100, 240, 120, 255 });
                if (CheckCollisionPointRec(mouse, cr)) {
                    UI_RequestTooltip("VENTAJA DIVINA", "La intencion enemiga revelada coincide con la postura que contrarresta esta invocacion (+4 tirada, x1.3 dano).", mouse);
                }
                chipX += cr.width + 6.0f;
            }
            Rectangle cr = UI_DrawChipEx("[AGOTA LA CARTA]", "AGOTA", (Rectangle){ chipX, chipY, 0, 18 }, (Color){ 65, 25, 25, 230 }, (Color){ 255, 130, 130, 255 });
            if (CheckCollisionPointRec(mouse, cr)) {
                UI_RequestTooltip("AGOTA LA CARTA", "La carta divina quedara agotada tras el choque (2 batallas o descanso/templo para reconsagrar).", mouse);
            }

            UI_DrawText(availableDivineDef->invocationDesc, (Vector2){ rInv.x + 12.0f, rInv.y + 46.0f }, 12.0f, (Color){ 215, 215, 225, 255 });

            modY = (int)(rInv.y + rInv.height + 6.0f);
        }

        // Modificadores / Cartas activas
        ActiveMod ams[16];
        int amCount = Modifier_CollectActive(&ctx->gm->modRegistry, ctx->playerArmy, ctx->enemy, ams, 16);
        int cx = tx;
        int cy = modY;
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
        UI_DrawButtonSfx((Rectangle){ (float)tx, (float)(middleY+45), 260, 44 }, "Cerrar Ronda", false, NULL);
    }
    else if (ctx->state == COMBAT_STATE_END || ctx->state == COMBAT_STATE_RECOUNT || ctx->state == COMBAT_STATE_DONE) {
        UI_DrawText("Fin del Combate", (Vector2){ (float)tx, (float)(middleY+12) }, 26, WHITE);
        
        Color resCol = GRAY;
        if (ctx->endReason == COMBAT_END_VICTORY) resCol = GOLD;
        if (ctx->endReason == COMBAT_END_DEFEAT || ctx->endReason == COMBAT_END_FLEE) resCol = RED;
        
        if (ctx->endReason != COMBAT_END_DEFEAT) {
            UI_DrawText(TextFormat("Resultado: %s", 
                ctx->endReason == COMBAT_END_VICTORY ? "VICTORIA" : 
                ctx->endReason == COMBAT_END_FLEE ? "HUIDA" : "EMPATE"), 
                (Vector2){ (float)tx, (float)(middleY+44) }, 22, resCol);
        }

        if (ctx->endReason == COMBAT_END_VICTORY) {
            // Tarjeta destacada de Botín Ganado (Feedback visual de Oro, Comida, Agua, Vino y Carne)
            float cardW = (ctx->lootMeat > 0) ? 680.0f : ((ctx->lootWine > 0) ? 570.0f : 470.0f);
            Rectangle lootCard = { (float)tx, (float)(middleY + 72), cardW, 40.0f };
            UI_DrawPanelStyled(lootCard, UI_STYLE_PANEL_DARK);
            UISkin_DrawGlow(lootCard, UI_COLOR_GOLD, 0.25f, 0);

            UI_DrawText("BOTIN GANADO:", (Vector2){ lootCard.x + 12, lootCard.y + 12 }, 15.0f, UI_COLOR_GOLD);

            char bBuf[64];
            snprintf(bBuf, sizeof(bBuf), "+%d Oro", ctx->lootGold);
            UI_DrawText(bBuf, (Vector2){ lootCard.x + 140, lootCard.y + 11 }, 16.0f, UI_COLOR_GOLD_LIGHT);

            snprintf(bBuf, sizeof(bBuf), "+%d Comida", ctx->lootFood);
            UI_DrawText(bBuf, (Vector2){ lootCard.x + 235, lootCard.y + 11 }, 16.0f, (Color){ 140, 215, 100, 255 });

            snprintf(bBuf, sizeof(bBuf), "+%d Agua", ctx->lootWater);
            UI_DrawText(bBuf, (Vector2){ lootCard.x + 345, lootCard.y + 11 }, 16.0f, (Color){ 110, 195, 255, 255 });

            if (ctx->lootWine > 0) {
                snprintf(bBuf, sizeof(bBuf), "+%d Vino", ctx->lootWine);
                UI_DrawText(bBuf, (Vector2){ lootCard.x + 450, lootCard.y + 11 }, 16.0f, (Color){ 230, 160, 255, 255 });
            }

            if (ctx->lootMeat > 0) {
                snprintf(bBuf, sizeof(bBuf), "+%d Carne", ctx->lootMeat);
                UI_DrawText(bBuf, (Vector2){ lootCard.x + 560, lootCard.y + 11 }, 16.0f, (Color){ 255, 180, 140, 255 });
            }

            UI_DrawText(TextFormat("Fuera de combate: %d Iniciados, %d Soldados, %d Sargentos", ctx->outOfActionInitiates, ctx->outOfActionSoldiers, ctx->outOfActionSergeants), (Vector2){ (float)tx, (float)(middleY+120) }, 16, (Color){ 240, 130, 130, 255 });
            UI_DrawChipEx("FUERA DE COMBATE", "FUERA_DE_COMBATE", (Rectangle){ (float)(tx + 470), (float)(middleY+118), 0, 20 }, (Color){ 55, 25, 25, 230 }, (Color){ 240, 120, 120, 255 });
            
            if (ctx->state == COMBAT_STATE_END) {
                UI_DrawButtonSfx((Rectangle){ (float)tx, (float)(middleY+148), 260, 42 }, "Reclamar Botin y Salir", false, NULL);
            } else if (ctx->state == COMBAT_STATE_DONE) {
                UI_DrawText(TextFormat("Muertes Confirmadas: %d Iniciados, %d Soldados, %d Sargentos (Clic para continuar)", ctx->deadInitiates, ctx->deadSoldiers, ctx->deadSergeants), (Vector2){ (float)tx, (float)(middleY+148) }, 16, RED);
            }
        } else if (ctx->endReason == COMBAT_END_FLEE) {
            UI_DrawText(TextFormat("Perdidas al huir: -%d Comida, -%d Moral", ctx->fleeCostFood, ctx->fleeCostMorale), (Vector2){ (float)tx, (float)(middleY + 76) }, 16, (Color){ 240, 130, 130, 255 });
            UI_DrawText(TextFormat("Fuera de combate: %d Iniciados, %d Soldados, %d Sargentos", ctx->outOfActionInitiates, ctx->outOfActionSoldiers, ctx->outOfActionSergeants), (Vector2){ (float)tx, (float)(middleY+104) }, 16, RED);
            if (ctx->state == COMBAT_STATE_END) {
                UI_DrawButtonSfx((Rectangle){ (float)tx, (float)(middleY+135), 260, 42 }, "Hacer Recuento y Salir", false, NULL);
            } else if (ctx->state == COMBAT_STATE_DONE) {
                UI_DrawText(TextFormat("Muertes Confirmadas: %d Iniciados, %d Soldados, %d Sargentos", ctx->deadInitiates, ctx->deadSoldiers, ctx->deadSergeants), (Vector2){ (float)tx, (float)(middleY+135) }, 16, RED);
            }
        } else {
            if (ctx->endReason == COMBAT_END_DEFEAT) {
                /* 5.1 Banner de Derrota Táctica y Ruptura de la Espiral de Derrota */
                Rectangle defeatBanner = { (float)tx, (float)(middleY + 42), (float)(panelW - 60), 30.0f };
                UI_DrawStatusChip(defeatBanner, "DERROTA TACTICA", UI_STYLE_CHIP_BAD, UI_ICON_PELIGRO);

                UI_DrawText("El ejercito se repliega 2 posiciones. Las fuerzas enemigas se detienen a saquear el campo de batalla.",
                            (Vector2){ (float)tx, (float)(middleY + 78) }, 14.5f, (Color){ 245, 185, 170, 255 });

                Rectangle moraleChip = { (float)tx, (float)(middleY + 100), 430.0f, 22.0f };
                UI_DrawStatusChip(moraleChip, "Moral: Iniciados -24 | Soldados -18 | Sargentos -14", UI_STYLE_CHIP_WARN, UI_ICON_MORAL);

                UI_DrawText(TextFormat("Fuera de combate: %d Iniciados, %d Soldados, %d Sargentos", ctx->outOfActionInitiates, ctx->outOfActionSoldiers, ctx->outOfActionSergeants), (Vector2){ (float)tx, (float)(middleY + 128) }, 15.5f, RED);
                UI_DrawChipEx("FUERA DE COMBATE", "FUERA_DE_COMBATE", (Rectangle){ (float)(tx + 470), (float)(middleY + 126), 0, 20 }, (Color){ 55, 25, 25, 230 }, (Color){ 240, 120, 120, 255 });
                if (ctx->state == COMBAT_STATE_END) {
                    UI_DrawButtonSfx((Rectangle){ (float)tx, (float)(middleY + 154), 260, 42 }, "Hacer Recuento y Salir", false, NULL);
                } else if (ctx->state == COMBAT_STATE_DONE) {
                    UI_DrawText(TextFormat("Muertes Confirmadas: %d Iniciados, %d Soldados, %d Sargentos", ctx->deadInitiates, ctx->deadSoldiers, ctx->deadSergeants), (Vector2){ (float)tx, (float)(middleY + 154) }, 16, RED);
                }
            } else {
                UI_DrawText(TextFormat("Fuera de combate: %d Iniciados, %d Soldados, %d Sargentos", ctx->outOfActionInitiates, ctx->outOfActionSoldiers, ctx->outOfActionSergeants), (Vector2){ (float)tx, (float)(middleY+76) }, 16, RED);
                UI_DrawChipEx("FUERA DE COMBATE", "FUERA_DE_COMBATE", (Rectangle){ (float)(tx + 470), (float)(middleY+74), 0, 20 }, (Color){ 55, 25, 25, 230 }, (Color){ 240, 120, 120, 255 });
                if (ctx->state == COMBAT_STATE_END) {
                    UI_DrawButtonSfx((Rectangle){ (float)tx, (float)(middleY+115), 260, 42 }, "Hacer Recuento y Salir", false, NULL);
                } else if (ctx->state == COMBAT_STATE_DONE) {
                    UI_DrawText(TextFormat("Muertes Confirmadas: %d Iniciados, %d Soldados, %d Sargentos", ctx->deadInitiates, ctx->deadSoldiers, ctx->deadSergeants), (Vector2){ (float)tx, (float)(middleY+115) }, 16, RED);
                }
            }
        }
    }

    // Pista descartable de primer uso (Sec B3)
    if (!s_combatHintDismissed) {
        Rectangle hintBounds = { (float)(px + 30), (float)(py + panelH - 32), (float)(panelW - 60), 24.0f };
        bool hintHover = CheckCollisionPointRec(GetMousePosition(), hintBounds);
        UI_DrawStatusChip(hintBounds, "Pasa el raton sobre chips y tarjetas para ver el glosario. Clic aqui para cerrar.", hintHover ? UI_STYLE_CHIP_WARN : UI_STYLE_CHIP_NEUTRAL, UI_ICON_AVISO);
    }

    // Dibujar textos flotantes e impactos
    UpdateAndDrawFloatingTexts(GetFrameTime());
}

bool CombatView_Update(CombatContext* ctx, int SW, int SH) {
    if (!ctx) return true;
    
    static int lastCombatState = -1;

    if (!combatInitialized) {
        combatInitialized = true;
        endTimer = 0.0f;
        s_rollType = CROLL_NONE;
        s_rollTimer = 0.0f;
        s_soundTickTimer = 0.0f;
        s_hasRolledDice = false;
        s_displayPlayerHealth = ctx->playerHealth;
        s_displayEnemyHealth = ctx->enemyHealth;
        Audio_SetMusicContext(MUSIC_CTX_COMBAT);
        Audio_PlaySfxByName("combat_start");
        lastCombatState = ctx->state;
    }

    float dt = GetFrameTime();

    // Drenado suave de salud en las barras tras recibir impacto
    if (s_displayPlayerHealth < 0.0f) s_displayPlayerHealth = ctx->playerHealth;
    if (s_displayEnemyHealth < 0.0f) s_displayEnemyHealth = ctx->enemyHealth;

    if (s_displayPlayerHealth > ctx->playerHealth) {
        s_displayPlayerHealth -= (s_displayPlayerHealth - ctx->playerHealth) * 7.0f * dt;
        if (s_displayPlayerHealth - ctx->playerHealth < 0.5f) s_displayPlayerHealth = ctx->playerHealth;
    } else if (s_displayPlayerHealth < ctx->playerHealth) {
        s_displayPlayerHealth = ctx->playerHealth;
    }

    if (s_displayEnemyHealth > ctx->enemyHealth) {
        s_displayEnemyHealth -= (s_displayEnemyHealth - ctx->enemyHealth) * 7.0f * dt;
        if (s_displayEnemyHealth - ctx->enemyHealth < 0.5f) s_displayEnemyHealth = ctx->enemyHealth;
    } else if (s_displayEnemyHealth < ctx->enemyHealth) {
        s_displayEnemyHealth = ctx->enemyHealth;
    }

    int panelW = 980; int panelH = 630;
    int px = (SW - panelW) / 2; int py = (SH - panelH) / 2;
    int tx = px + 30; int middleY = py + 245;

    // Procesar tirada de dados en progreso (la tirada va PRIMERO, el daÃ±o y estado DESPUÃ‰S)
    if (s_rollType != CROLL_NONE) {
        s_rollTimer -= dt;
        s_soundTickTimer -= dt;

        // Dados cambian rÃ¡pidamente de cara mientras giran
        visualDice1 = GetRandomValue(1, 6);
        visualDice2 = GetRandomValue(1, 6);

        if (s_soundTickTimer <= 0.0f) {
            Audio_PlaySfx(SFX_DICE_LAND);
            s_soundTickTimer = 0.09f;
        }

        if (s_rollTimer <= 0.0f) {
            // Fin del lanzamiento: clavar valores finales
            visualDice1 = s_targetD1;
            visualDice2 = s_targetD2;
            s_lastD1 = s_targetD1;
            s_lastD2 = s_targetD2;
            s_hasRolledDice = true;
            Audio_PlaySfx(SFX_DICE_LAND);

            CombatRollType completed = s_rollType;
            s_rollType = CROLL_NONE;

            if (completed == CROLL_EVALUATION) {
                Combat_RollEvaluation(ctx, s_targetD1, s_targetD2);
                Audio_PlaySfxByName("ui_confirm");
                ctx->log.count = 0;
            }
            else if (completed == CROLL_PLANNING) {
                Combat_RollPlanning(ctx, s_targetD1, s_targetD2);
                Audio_PlaySfxByName("ui_confirm");
                ctx->log.count = 0;
            }
            else if (completed == CROLL_REROLL) {
                Combat_UseCommandToken_Reroll(ctx, s_targetD1, s_targetD2);
                Audio_PlaySfxByName("ui_confirm");
                ctx->log.count = 0;
            }
            else if (completed == CROLL_EXECUTION) {
                Combat_SelectPlan(ctx, s_pendingPlan);
                Combat_RollExecution(ctx, s_targetD1, s_targetD2);
                // AHORA que los dados cayeron, se aplica el impacto, sonido y textos flotantes
                TriggerCombatFeedback(ctx, px, py);
                ctx->log.count = 0;
            }
        }
        return false;
    }

    if (Sequence_IsBusy()) return false;

    if (ctx->state == COMBAT_STATE_DONE) {
        endTimer += dt;
        if (endTimer > 4.0f || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Combat_Commit(ctx);
            combatInitialized = false;
            lastCombatState = -1;
            s_hasRolledDice = false;
            s_displayPlayerHealth = -1.0f;
            s_displayEnemyHealth = -1.0f;
            return true;
        }
        return false;
    }

    // Descarte de pista con clic
    if (!s_combatHintDismissed) {
        Rectangle hintBounds = { (float)(px + 30), (float)(py + panelH - 30), (float)(panelW - 60), 22.0f };
        if (CheckCollisionPointRec(GetMousePosition(), hintBounds) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            s_combatHintDismissed = true;
        }
    }

    // Detectar transicion a pantalla final de combate (Victoria / Derrota / Huida)
    if (ctx->state == COMBAT_STATE_END && lastCombatState != COMBAT_STATE_END) {
        Audio_PlayCombatEnd(ctx->endReason);
        lastCombatState = COMBAT_STATE_END;
    }
    
    if (ctx->state == COMBAT_STATE_EVALUATION) {
        if (UI_ButtonClicked((Rectangle){ (float)tx, (float)(middleY+38), 260, 44 })) {
            s_targetD1 = GetRandomValue(1, 6);
            s_targetD2 = GetRandomValue(1, 6);
            s_rollType = CROLL_EVALUATION;
            s_rollTimer = 0.70f;
            s_soundTickTimer = 0.0f;
            Audio_PlaySfx(SFX_DICE_ROLL);
        }
    }
    else if (ctx->state == COMBAT_STATE_PLANNING) {
        if (UI_ButtonClicked((Rectangle){ (float)tx, (float)(middleY+50), 260, 44 })) {
            s_targetD1 = GetRandomValue(1, 6);
            s_targetD2 = GetRandomValue(1, 6);
            s_rollType = CROLL_PLANNING;
            s_rollTimer = 0.70f;
            s_soundTickTimer = 0.0f;
            Audio_PlaySfx(SFX_DICE_ROLL);
        }
    }
    else if (ctx->state == COMBAT_STATE_EXECUTION) {
        // Huir
        if (UI_ButtonClicked((Rectangle){ (float)tx, (float)(middleY + 24), 130, 34 })) {
            s_pendingPlan = PLAN_FLEE;
            s_targetD1 = GetRandomValue(1, 6);
            s_targetD2 = GetRandomValue(1, 6);
            s_rollType = CROLL_EXECUTION;
            s_rollTimer = 0.85f;
            s_soundTickTimer = 0.0f;
            Audio_PlaySfx(SFX_DICE_ROLL);
        }

        // Fichas de Mando
        if (ctx->playerArmy->commandTokens > 0) {
            if (UI_ButtonClicked((Rectangle){ (float)(tx + 215), (float)(middleY + 24), 115, 34 })) {
                Combat_UseCommandToken_Add(ctx);
                Audio_PlaySfxByName("ui_confirm");
            }
            if (UI_ButtonClicked((Rectangle){ (float)(tx + 335), (float)(middleY + 24), 125, 34 })) {
                s_targetD1 = GetRandomValue(1, 6);
                s_targetD2 = GetRandomValue(1, 6);
                s_rollType = CROLL_REROLL;
                s_rollTimer = 0.70f;
                s_soundTickTimer = 0.0f;
                Audio_PlaySfx(SFX_DICE_ROLL);
            }
        }

        // Tarjetas de Planes TÃ¡cticos
        float cardW = (panelW - 60 - 16) / 2.0f;
        float cardH = 76.0f;
        Rectangle rSk = { (float)tx, (float)(middleY + 66), cardW, cardH };
        Rectangle rPk = { (float)(tx + cardW + 16), (float)(middleY + 66), cardW, cardH };
        Rectangle rAs = { (float)tx, (float)(middleY + 66 + cardH + 10), cardW, cardH };
        Rectangle rFl = { (float)(tx + cardW + 16), (float)(middleY + 66 + cardH + 10), cardW, cardH };

        CombatPlanInfo infoSk = Combat_GetPlanInfo(ctx, PLAN_SKIRMISH);
        CombatPlanInfo infoPk = Combat_GetPlanInfo(ctx, PLAN_PIKES);
        CombatPlanInfo infoAs = Combat_GetPlanInfo(ctx, PLAN_ASSAULT);
        CombatPlanInfo infoFl = Combat_GetPlanInfo(ctx, PLAN_FLANK);

        if (infoSk.isUnlocked && UI_ButtonClicked(rSk)) {
            s_pendingPlan = PLAN_SKIRMISH;
            s_targetD1 = GetRandomValue(1, 6);
            s_targetD2 = GetRandomValue(1, 6);
            s_rollType = CROLL_EXECUTION;
            s_rollTimer = 0.85f;
            s_soundTickTimer = 0.0f;
            Audio_PlaySfx(SFX_DICE_ROLL);
        }
        else if (infoPk.isUnlocked && UI_ButtonClicked(rPk)) {
            s_pendingPlan = PLAN_PIKES;
            s_targetD1 = GetRandomValue(1, 6);
            s_targetD2 = GetRandomValue(1, 6);
            s_rollType = CROLL_EXECUTION;
            s_rollTimer = 0.85f;
            s_soundTickTimer = 0.0f;
            Audio_PlaySfx(SFX_DICE_ROLL);
        }
        else if (infoAs.isUnlocked && UI_ButtonClicked(rAs)) {
            s_pendingPlan = PLAN_ASSAULT;
            s_targetD1 = GetRandomValue(1, 6);
            s_targetD2 = GetRandomValue(1, 6);
            s_rollType = CROLL_EXECUTION;
            s_rollTimer = 0.85f;
            s_soundTickTimer = 0.0f;
            Audio_PlaySfx(SFX_DICE_ROLL);
        }
        else if (infoFl.isUnlocked && UI_ButtonClicked(rFl)) {
            s_pendingPlan = PLAN_FLANK;
            s_targetD1 = GetRandomValue(1, 6);
            s_targetD2 = GetRandomValue(1, 6);
            s_rollType = CROLL_EXECUTION;
            s_rollTimer = 0.85f;
            s_soundTickTimer = 0.0f;
            Audio_PlaySfx(SFX_DICE_ROLL);
        }

        // Clic en Invocación Divina
        int availableDivineSlot = -1;
        const ModifierDef* availableDivineDef = NULL;
        if (ctx->playerArmy && ctx->playerArmy->commander.ptr && ctx->gm) {
            for (int c = 0; c < 8; c++) {
                ModifierInstance* inst = Commander_GetCardInstance(ctx->playerArmy->commander, c);
                if (!inst) break;
                if (!inst->isExhausted) {
                    const ModifierDef* def = Modifier_GetById(ctx->gm->modRegistry.defs, ctx->gm->modRegistry.count, inst->id);
                    if (def && def->hasInvocation) {
                        availableDivineSlot = c;
                        availableDivineDef = def;
                        break;
                    }
                }
            }
        }

        if (availableDivineDef != NULL) {
            Rectangle rInv = { (float)tx, (float)(middleY + 66 + (cardH + 10) * 2), (float)(panelW - 60), 66.0f };
            if (UI_ButtonClicked(rInv)) {
                Combat_SelectDivineInvocation(ctx, availableDivineSlot);
                s_pendingPlan = PLAN_DIVINE_INVOCATION;
                s_targetD1 = GetRandomValue(1, 6);
                s_targetD2 = GetRandomValue(1, 6);
                s_rollType = CROLL_EXECUTION;
                s_rollTimer = 0.85f;
                s_soundTickTimer = 0.0f;
                Audio_PlaySfx(SFX_DICE_ROLL);
            }
        }
    }
    else if (ctx->state == COMBAT_STATE_ROUND_CLOSE) {
        if (UI_ButtonClicked((Rectangle){ (float)tx, (float)(middleY+45), 260, 44 })) {
            Combat_RoundClose(ctx);
            if (ctx->state == COMBAT_STATE_END) {
                Audio_PlayCombatEnd(ctx->endReason);
                lastCombatState = COMBAT_STATE_END;
            } else {
                Audio_PlaySfxByName("ui_confirm");
            }
        }
    }
    else if (ctx->state == COMBAT_STATE_END) {
        float btnY = (ctx->endReason == COMBAT_END_VICTORY) ? (middleY + 148) :
                     ((ctx->endReason == COMBAT_END_FLEE) ? (middleY + 135) :
                     ((ctx->endReason == COMBAT_END_DEFEAT) ? (middleY + 154) : (middleY + 115)));
        if (UI_ButtonClicked((Rectangle){ (float)tx, (float)btnY, 260, 42 })) {
            Combat_Recount(ctx);
            Audio_PlaySfxByName("ui_confirm");
        }
    }
    
    lastCombatState = ctx->state;
    return false;
}
