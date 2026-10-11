#include "hud.h"
#include "presentation/audio_manager.h"
#include "models/balance_config.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

HudState Hud_Init(void)
{
    HudState hud = {0};
    hud.statsExpanded = false;
    hud.blinkTimer = 0.0f;
    hud.dice.valor1 = 1;
    hud.dice.valor2 = 1;
    hud.dice.rolling = false;
    hud.dice.rollTimer = 0.0f;
    hud.dice.rollDuration = 0.5f;
    
    hud.cmdPanelRect = (Rectangle){0, 0, 0, 0};
    hud.rollBtnRect = (Rectangle){0, 0, 0, 0};
    hud.forageBtnRect = (Rectangle){0, 0, 0, 0};
    hud.endTurnBtnRect = (Rectangle){0, 0, 0, 0};
    
    return hud;
}

void Hud_Update(HudState* hud, float deltaTime)
{
    if (hud == NULL) return;
    
    hud->blinkTimer += deltaTime;
    if (hud->blinkTimer >= 10.0f) {
        hud->blinkTimer -= 10.0f;
    }
    
    if (hud->dice.rolling) {
        hud->dice.rollTimer += deltaTime;
        
        /* Animaci�n r�pida de cambio de caras mientras gira */
        if ((int)(hud->dice.rollTimer * 20.0f) % 2 == 0) {
            hud->dice.valor1 = (rand() % 6) + 1;
            hud->dice.valor2 = (rand() % 6) + 1;
            Audio_PlaySfx(SFX_DICE_LAND);
        }
        
        if (hud->dice.rollTimer >= hud->dice.rollDuration) {
            hud->dice.rolling = false;
            hud->dice.rollTimer = 0.0f;
            /* Valores finales */
            hud->dice.valor1 = (rand() % 6) + 1;
            hud->dice.valor2 = (rand() % 6) + 1;
            Audio_PlaySfx(SFX_DICE_LAND);
        }
    }
}

void Hud_RollDice(HudState* hud)
{
    if (hud && !hud->dice.rolling) {
        
        Audio_PlaySfx(SFX_DICE_ROLL);
        hud->dice.rolling = true;
        hud->dice.rollTimer = 0.0f;
    }
}

void Hud_HandleInput(HudState* hud, GameMaster* gm, Army* army, Map* map)
{
    if (hud == NULL || gm == NULL) return;
    
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        
        if (CheckCollisionPointRec(mouse, hud->cmdPanelRect)) {
            hud->statsExpanded = !hud->statsExpanded;
        }
        
        if (CheckCollisionPointRec(mouse, hud->rollBtnRect) && !gm->hasRolledThisTurn && !hud->dice.rolling) {
            Hud_RollDice(hud);
        }

        if (CheckCollisionPointRec(mouse, hud->forageBtnRect) && army && map && !gm->inCombat && gm->phase == PHASE_PLAYER && !gm->hasForagedToday && gm->timeOfDay != TIME_NIGHT && !gm->showCampDecision) {
            if (GameMaster_Forage(gm, army, map)) {
                Map_UpdateVision(map, army->position, Army_GetVision(army) + GameMaster_GetVisionModifier(gm, army));
            }
        }

        if (CheckCollisionPointRec(mouse, hud->endTurnBtnRect) && army && map && !gm->inCombat && gm->phase == PHASE_PLAYER && !gm->hasDayReport && !gm->showCampDecision) {
            GameMaster_EndTurn(gm, map, army);
            Map_UpdateVision(map, army->position, Army_GetVision(army) + GameMaster_GetVisionModifier(gm, army));
        }
    }

    if (IsKeyPressed(KEY_F) && army && map && !gm->inCombat && gm->phase == PHASE_PLAYER && !gm->hasDayReport && !gm->hasForagedToday && gm->timeOfDay != TIME_NIGHT && !gm->showCampDecision) {
        if (GameMaster_Forage(gm, army, map)) {
            Map_UpdateVision(map, army->position, Army_GetVision(army) + GameMaster_GetVisionModifier(gm, army));
        }
    }
}

static void Hud_DrawDivineCardSlot(int x, int y, int w, int h, bool hasCard, const char* cardId, int remaining, const GameMaster* gm)
{
    Color border = hasCard ? GOLD : DARKGRAY;
    Color bg     = hasCard ? (Color){ 50, 40, 65, 230 }
                           : (Color){ 30, 30, 35, 180 };
    DrawRectangle(x, y, w, h, bg);
    DrawRectangleLines(x, y, w, h, border);
    if (hasCard) {
        char initial = '?';
        if (gm) {
            for (int m = 0; m < gm->modRegistry.count; m++) {
                if (strcmp(gm->modRegistry.defs[m].id, cardId) == 0) {
                    if (gm->modRegistry.defs[m].source[0] != '\0') {
                        initial = gm->modRegistry.defs[m].source[0];
                    } else if (gm->modRegistry.defs[m].name[0] != '\0') {
                        initial = gm->modRegistry.defs[m].name[0];
                    }
                    break;
                }
            }
        }
        DrawText(TextFormat("%c", initial), x+4, y+4, 14, GOLD);
        DrawCircle(x + w/2, y + h/2, w/5, (Color){ 180, 120, 255, 200 });
    } else {
        DrawText("?", x + w/2 - 4, y + h/2 - 6, 12, DARKGRAY);
    }
}

static void Hud_DrawCommanderStats(const Army* army, int x, int y, int w, int h)
{
    DrawRectangle(x, y, w, h, (Color){ 30, 30, 40, 230 });
    DrawRectangleLines(x, y, w, h, (Color){ 100, 100, 120, 255 });
    
    CommanderRef cmd = army->commander;
    int ty = y + 8;
    DrawText(TextFormat("Vision:        %d", Commander_GetVision(cmd)),     x+16, ty,    12, LIGHTGRAY);
    DrawText(TextFormat("Liderazgo:     %d", Commander_GetLeadership(cmd)), x+16, ty+16, 12, LIGHTGRAY);
    DrawText(TextFormat("Evaluacion:    %d", Commander_GetEvaluation(cmd)), x+16, ty+32, 12, LIGHTGRAY);
    DrawText(TextFormat("Planificacion: %d", Commander_GetPlanning(cmd)),   x+16, ty+48, 12, LIGHTGRAY);
    DrawText(TextFormat("Ejecucion:     %d", Commander_GetExecution(cmd)),  x+16, ty+64, 12, LIGHTGRAY);
    
    if (army->commander.type == OFFICER_LIEUTENANT) {
        int loy = Commander_GetLoyalty(cmd);
        Color loyColor = (loy < 50) ? RED : ((loy < 75) ? ORANGE : GREEN);
        DrawText(TextFormat("Lealtad:       %d", loy), x+16, ty+80, 12, loyColor);
    }
}

void Hud_DrawCommanderPanel(const Army* army, const GameMaster* gm, HudState* hud, int x, int y, int w)
{
    int hBase = 90;
    
    /* Guardamos el rect para el click (solo la parte superior) */
    hud->cmdPanelRect = (Rectangle){ x, y, w, hBase };
    
    DrawRectangle(x, y, w, hBase, (Color){ 20, 20, 25, 210 });
    DrawRectangleLines(x, y, w, hBase, (Color){ 120, 120, 140, 255 });
    
    /* Espacio para el retrato (cara del comandante en blanco) */
    int portraitSize = 64;
    int portraitX = x + 12;
    int portraitY = y + 12;
    DrawRectangle(portraitX, portraitY, portraitSize, portraitSize, (Color){ 40, 40, 45, 255 });
    DrawRectangleLines(portraitX, portraitY, portraitSize, portraitSize, LIGHTGRAY);
    DrawText("[Sprite]", portraitX + 6, portraitY + 28, 12, DARKGRAY);
    
    /* Nombre y tipo de oficial */
    CommanderRef cmd = army->commander;
    const char* name = Commander_GetName(cmd);
    const char* rank = (cmd.type == OFFICER_GENERAL) ? "General" : "Teniente";
    
    DrawText(name, portraitX + portraitSize + 12, y + 16, 16, WHITE);
    DrawText(rank, portraitX + portraitSize + 12, y + 36, 12, (cmd.type == OFFICER_GENERAL) ? GOLD : LIGHTGRAY);
    
    /* Cartas Divinas (a la derecha) */
    int slots = (cmd.type == OFFICER_GENERAL) ? MAX_GENERAL_DIVINE_CARDS : MAX_LIEUTENANT_DIVINE_CARDS;
    int cardCount = Commander_GetActiveModifierCount(cmd);
    
    int cardW = 36;
    int cardH = 48;
    int cardX = x + w - (slots * (cardW + 6)) - 10;
    int cardY = y + 16;
    
    for (int i = 0; i < slots; i++) {
        bool hasCard = (i < cardCount);
        const char* cardId = "";
        int remaining = -1;
        if (hasCard) {
            cardId = Commander_GetActiveModifier(cmd, i);
            if (cmd.type == OFFICER_GENERAL) {
                remaining = ((General*)cmd.ptr)->cards[i].remaining;
            } else if (cmd.type == OFFICER_LIEUTENANT) {
                remaining = ((Lieutenant*)cmd.ptr)->patronCard.remaining;
            }
        }
        Hud_DrawDivineCardSlot(cardX + i * (cardW + 6), cardY, cardW, cardH, hasCard, cardId, remaining, gm);
    }
    
    /* Indicador de men� desplegable */
    const char* toggleText = hud->statsExpanded ? "? Ocultar Stats" : "? Mostrar Stats";
    DrawText(toggleText, portraitX + portraitSize + 12, y + 64, 11, GRAY);
    
    /* Dibujar panel desplegable si est� expandido */
    if (hud->statsExpanded) {
        Hud_DrawCommanderStats(army, x, y + hBase, w, 100);
    }
}

void Hud_DrawArmyStatsPanel(const Army* army, const GameMaster* gm, int x, int y, int w, int h)
{
    DrawRectangle(x, y, w, h, (Color){ 20, 18, 25, 230 });
    DrawRectangleLines(x, y, w, h, (Color){ 160, 120, 60, 255 });

    int total = Army_GetTotalSoldiers(army);
    float daysSupply = Army_DaysOfSupply(army);
    const BalanceConfig* cfg = Balance_GetGlobal();
    float lowThreshold = cfg ? cfg->lowSupplyDays : 2.0f;

    /* Columna izquierda: tropas */
    DrawText(TextFormat("EJERCITO (PA: %d/%d)", gm->currentActionPoints, gm->maxActionPoints), x+12, y+10, 13, (Color){ 200, 170, 80, 255 });
    DrawLine(x+12, y+24, x+w/2-10, y+24, (Color){ 160, 120, 60, 100 });
    
    DrawText(TextFormat("Sargentos: %d",  army->sargentos), x+12, y+30, 11, LIGHTGRAY);
    DrawText(TextFormat("Soldados:  %d",  army->soldados),  x+12, y+45, 11, LIGHTGRAY);
    DrawText(TextFormat("Iniciados: %d",  army->iniciados), x+12, y+60, 11, LIGHTGRAY);
    DrawText(TextFormat("Total:     %d",  total),           x+12, y+75, 11, WHITE);

    /* Columna derecha: recursos */
    int rx = x + w/2 + 10;
    
    DrawText("RECURSOS", rx, y+10, 13, (Color){ 200, 170, 80, 255 });
    DrawLine(rx, y+24, x+w-12, y+24, (Color){ 160, 120, 60, 100 });

    /* Indicador de estado de comida y agua (6.3) */
    const char* waterStatus = (army->daysWithoutWater > 0) ? TextFormat("Sed d%d", army->daysWithoutWater) : ((daysSupply < lowThreshold) ? "Escasa" : "OK");
    Color waterColor = (army->daysWithoutWater > 0) ? RED : ((daysSupply < lowThreshold) ? ORANGE : SKYBLUE);

    const char* foodStatus = (army->daysWithoutFood > 0) ? TextFormat("Hambre d%d", army->daysWithoutFood) : ((daysSupply < lowThreshold) ? "Escasa" : "OK");
    Color foodColor = (army->daysWithoutFood > 0) ? RED : ((daysSupply < lowThreshold) ? YELLOW : ORANGE);

    DrawText(TextFormat("Agua:   %.0f [%s]", army->agua_disponible, waterStatus), rx, y+28, 11, waterColor);
    DrawText(TextFormat("Comida: %.0f [%s]", army->comida_disponible, foodStatus),  rx, y+42, 11, foodColor);

    Color supplyColor = (daysSupply < lowThreshold) ? RED : RAYWHITE;
    DrawText(TextFormat("Raciones: %.1fd %s", daysSupply, (daysSupply < lowThreshold) ? "[! AVISO]" : ""), rx, y+56, 11, supplyColor);
    DrawText(TextFormat("Oro: %d  Ganado: %d", army->monedas, army->ganado), rx, y+70, 11, GOLD);

    /* Barras de moral y salud */
    int bx = x+12, bw = w-24, bh = 11;
    /* Moral */
    DrawRectangle(bx, y+92, bw, bh, (Color){ 50, 30, 30, 255 });
    DrawRectangle(bx, y+92, (int)(bw * army->moral / 100.0f), bh,
                  (army->moral < 30) ? RED : (army->moral < 60) ? ORANGE : GREEN);
    DrawText(TextFormat("Moral %.0f%%", army->moral), bx+4, y+93, 10, WHITE);
    DrawRectangleLines(bx, y+92, bw, bh, BLACK);

    /* Salud */
    DrawRectangle(bx, y+107, bw, bh, (Color){ 30, 50, 30, 255 });
    DrawRectangle(bx, y+107, (int)(bw * army->salud / 100.0f), bh, GREEN);
    DrawText(TextFormat("Salud %.0f%%", army->salud), bx+4, y+108, 10, WHITE);
    DrawRectangleLines(bx, y+107, bw, bh, BLACK);

    /* Advertencia de hambruna si aplica */
    if (army->daysWithoutFood > 0 || army->daysWithoutWater > 0) {
        DrawText("[!] TROPA EN CONDICION CRITICA DE SUPERVIVENCIA", bx+4, y+124, 11, RED);
    }
}

static void Hud_DrawSingleDie(int cx, int cy, int size, int value)
{
    int hs = size/2;
    DrawRectangle(cx - hs, cy - hs, size, size, WHITE);
    DrawRectangleLines(cx - hs, cy - hs, size, size, BLACK);
    
    int dotR = size/10;
    int padding = size/4;
    Color dotCol = BLACK;
    
    if (value == 1 || value == 3 || value == 5) {
        DrawCircle(cx, cy, dotR, dotCol); /* Centro */
    }
    if (value > 1) {
        DrawCircle(cx - padding, cy - padding, dotR, dotCol); /* Arriba Izq */
        DrawCircle(cx + padding, cy + padding, dotR, dotCol); /* Abajo Der */
    }
    if (value > 3) {
        DrawCircle(cx + padding, cy - padding, dotR, dotCol); /* Arriba Der */
        DrawCircle(cx - padding, cy + padding, dotR, dotCol); /* Abajo Izq */
    }
    if (value == 6) {
        DrawCircle(cx - padding, cy, dotR, dotCol); /* Medio Izq */
        DrawCircle(cx + padding, cy, dotR, dotCol); /* Medio Der */
    }
}

void Hud_DrawDicePanel(HudState* hud, const GameMaster* gm, int x, int y, int w, int h)
{
    DrawRectangle(x, y, w, h, (Color){ 28, 22, 26, 230 });
    DrawRectangleLines(x, y, w, h, (Color){ 160, 80, 60, 255 });
    DrawText(TextFormat("ACCIONES (PA: %d)", gm ? gm->currentActionPoints : 0), x + w/2 - 55, y + 6, 12, GOLD);

    /* Dados */
    int dieSize = 28;
    Hud_DrawSingleDie(x + w/4 + 4,   y + 36, dieSize, hud->dice.valor1);
    Hud_DrawSingleDie(x + 3*w/4 - 4, y + 36, dieSize, hud->dice.valor2);

    /* Boton 1: ROLL (SPACE) */
    hud->rollBtnRect = (Rectangle){ (float)(x+8), (float)(y+58), (float)(w-16), 24.0f };
    bool rollHover = CheckCollisionPointRec(GetMousePosition(), hud->rollBtnRect);
    
    bool canRoll = (gm && !gm->hasRolledThisTurn && !hud->dice.rolling && !gm->showCampDecision);
    Color rollCol = GRAY;
    if (canRoll) {
        float pulse = (sinf(GetTime() * 5.0f) + 1.0f) * 0.5f;
        Color baseCol = rollHover ? (Color){180,50,50,255} : (Color){140,30,30,255};
        rollCol = (Color){ baseCol.r + (unsigned char)(50*pulse), baseCol.g, baseCol.b, 255 };
    }

    DrawRectangleRec(hud->rollBtnRect, rollCol);
    DrawRectangleLinesEx(hud->rollBtnRect, 1, BLACK);
    
    if (canRoll) {
        DrawText("[ TIRAR (SPACE) ]", x + w/2 - MeasureText("[ TIRAR (SPACE) ]", 10)/2, y+64, 10, WHITE);
    } else {
        const char* rollMsg = hud->dice.rolling ? "[ TIRANDO... ]" : "[ YA TIRASTE ]";
        DrawText(rollMsg, x + w/2 - MeasureText(rollMsg, 10)/2, y+64, 10, LIGHTGRAY);
    }

    /* Boton 2: FORRAJE (F) */
    hud->forageBtnRect = (Rectangle){ (float)(x+8), (float)(y+87), (float)(w-16), 24.0f };
    bool forageHover = CheckCollisionPointRec(GetMousePosition(), hud->forageBtnRect);
    bool canForage = (gm && !gm->inCombat && gm->phase == PHASE_PLAYER && !gm->hasDayReport && !gm->hasForagedToday && gm->timeOfDay != TIME_NIGHT && !gm->showCampDecision);

    Color forageCol;
    const char* forageTxt;
    if (gm && gm->hasForagedToday) {
        forageCol = (Color){ 45, 45, 45, 200 };
        forageTxt = "[ YA FORRAJEASTE ]";
    } else if (gm && gm->timeOfDay == TIME_NIGHT) {
        forageCol = (Color){ 40, 40, 45, 200 };
        forageTxt = "[ NOCHE: SIN FORRAJE ]";
    } else {
        forageCol = canForage ? (forageHover ? (Color){ 45, 130, 60, 255 } : (Color){ 30, 95, 45, 255 }) : (Color){ 50, 50, 50, 200 };
        forageTxt = "[ FORRAJEAR (F) ]";
    }

    DrawRectangleRec(hud->forageBtnRect, forageCol);
    DrawRectangleLinesEx(hud->forageBtnRect, 1, BLACK);
    DrawText(forageTxt, x + w/2 - MeasureText(forageTxt, 10)/2, y+93, 10, canForage ? WHITE : GRAY);

    /* Boton 3: PASAR TURNO / ATARDECER (ENTER) */
    hud->endTurnBtnRect = (Rectangle){ (float)(x+8), (float)(y+116), (float)(w-16), 24.0f };
    bool endTurnHover = CheckCollisionPointRec(GetMousePosition(), hud->endTurnBtnRect);
    bool isAfternoon = (gm && gm->timeOfDay == TIME_AFTERNOON);

    Color endTurnCol;
    const char* endTurnTxt;
    if (isAfternoon) {
        endTurnCol = endTurnHover ? (Color){ 190, 115, 40, 255 } : (Color){ 145, 80, 25, 255 };
        endTurnTxt = "[ ATARDECER (ENTER) ]";
    } else {
        endTurnCol = endTurnHover ? (Color){ 65, 95, 145, 255 } : (Color){ 45, 65, 105, 255 };
        endTurnTxt = "[ PASAR TURNO (ENTER) ]";
    }

    DrawRectangleRec(hud->endTurnBtnRect, endTurnCol);
    DrawRectangleLinesEx(hud->endTurnBtnRect, 1, isAfternoon ? GOLD : (Color){ 100, 140, 200, 255 });
    DrawText(endTurnTxt, x + w/2 - MeasureText(endTurnTxt, 10)/2, y+122, 10, WHITE);
}

void Hud_DrawTopBar(const GameMaster* gm, int screenWidth) {
    if (!gm) return;
    int h = 30;
    DrawRectangle(0, 0, screenWidth, h, (Color){ 15, 15, 20, 240 });
    DrawRectangleLines(0, 0, screenWidth, h, (Color){ 100, 80, 40, 255 });

    const char* timeStr = "Manana";
    switch(gm->timeOfDay) {
        case TIME_MORNING: timeStr = "Manana"; break;
        case TIME_MIDDAY: timeStr = "Mediodia"; break;
        case TIME_AFTERNOON: timeStr = "Tarde"; break;
        case TIME_NIGHT: timeStr = "Noche"; break;
    }

    DrawText(TextFormat("Dia %d - %s (Turno %d)", gm->currentDay, timeStr, gm->currentTurn), 20, 8, 14, RAYWHITE);
    DrawText(TextFormat("Infamia: %.1f%%", gm->infamy), screenWidth / 2 - 40, 8, 14, RED);

    if (gm->timeOfDay == TIME_AFTERNOON) {
        DrawText("[ENTER] Atardecer / Noche", screenWidth - 190, 8, 14, GOLD);
    } else {
        DrawText("[ENTER] Pasar Turno", screenWidth - 160, 8, 14, YELLOW);
    }
}

void Hud_DrawDayReportModal(const DayReport* report, int screenWidth, int screenHeight)
{
    if (!report) return;

    /* Fondo semitransparente oscuro */
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.75f));

    int mw = 480;
    int mh = 330;
    int mx = (screenWidth - mw) / 2;
    int my = (screenHeight - mh) / 2;

    /* Marco del modal */
    DrawRectangle(mx, my, mw, mh, (Color){ 24, 22, 28, 250 });
    DrawRectangleLinesEx((Rectangle){ mx, my, mw, mh }, 2, (Color){ 180, 140, 60, 255 });

    /* Titulo */
    const char* title = TextFormat("--- RESUMEN DEL DIA %d ---", report->dayNumber);
    int titleW = MeasureText(title, 18);
    DrawText(title, mx + (mw - titleW) / 2, my + 16, 18, GOLD);
    DrawLine(mx + 20, my + 42, mx + mw - 20, my + 42, (Color){ 180, 140, 60, 150 });

    /* Consumo del dia */
    int ty = my + 52;
    DrawText("CONSUMO DE RECURSOS:", mx + 24, ty, 13, (Color){ 200, 170, 80, 255 });
    ty += 18;
    DrawText(TextFormat("Comida consumida: %.0f raciones", report->foodConsumed), mx + 36, ty, 12, LIGHTGRAY);
    ty += 16;
    DrawText(TextFormat("Agua consumida:   %.0f unidades", report->waterConsumed), mx + 36, ty, 12, LIGHTGRAY);
    ty += 22;

    /* Estado de Hambruna / Deshidratacion */
    DrawText("ESTADO DE SUPERVIVENCIA:", mx + 24, ty, 13, (Color){ 200, 170, 80, 255 });
    ty += 18;
    if (report->foodShortage) {
        DrawText(TextFormat("[!] ESCASEZ DE COMIDA: %d dia(s) en hambruna", report->daysWithoutFood), mx + 36, ty, 12, RED);
    } else {
        DrawText("[OK] Raciones de comida suficientes", mx + 36, ty, 12, GREEN);
    }
    ty += 16;
    if (report->waterShortage) {
        DrawText(TextFormat("[!] ESCASEZ DE AGUA: %d dia(s) en sed extrema", report->daysWithoutWater), mx + 36, ty, 12, RED);
    } else {
        DrawText("[OK] Reserva de agua suficiente", mx + 36, ty, 12, GREEN);
    }
    ty += 22;

    /* Impacto en la tropa */
    DrawText("IMPACTO EN LA TROPA:", mx + 24, ty, 13, (Color){ 200, 170, 80, 255 });
    ty += 18;
    Color statCol = (report->moralDelta < 0 || report->saludDelta < 0) ? ORANGE : RAYWHITE;
    DrawText(TextFormat("Moral: %+.0f  |  Salud: %+.0f", report->moralDelta, report->saludDelta), mx + 36, ty, 12, statCol);
    ty += 16;

    int totalDeserters = report->desertedInitiates + report->desertedSoldiers + report->desertedSergeants;
    int totalDeaths = report->deadInitiates + report->deadSoldiers + report->deadSergeants;
    if (totalDeserters > 0 || totalDeaths > 0) {
        DrawText(TextFormat("Bajas por hambre: %d muertos | %d desertores", totalDeaths, totalDeserters), mx + 36, ty, 12, RED);
    } else {
        DrawText("Sin bajas ni deserciones registradas.", mx + 36, ty, 12, LIGHTGRAY);
    }

    /* Boton Continuar */
    Rectangle okBtn = { mx + mw / 2 - 85, my + mh - 42, 170, 28 };
    bool hover = CheckCollisionPointRec(GetMousePosition(), okBtn);
    DrawRectangleRec(okBtn, hover ? (Color){ 160, 120, 50, 255 } : (Color){ 110, 80, 30, 255 });
    DrawRectangleLinesEx(okBtn, 1, GOLD);
    const char* btnTxt = "[ CONTINUAR (ENTER) ]";
    int bw = MeasureText(btnTxt, 11);
    DrawText(btnTxt, okBtn.x + (okBtn.width - bw) / 2, okBtn.y + 8, 11, WHITE);
}

bool Hud_HandleDayReportInput(GameMaster* gm, int screenWidth, int screenHeight)
{
    if (!gm || !gm->hasDayReport) return false;

    int mw = 480;
    int mh = 330;
    int mx = (screenWidth - mw) / 2;
    int my = (screenHeight - mh) / 2;
    Rectangle okBtn = { mx + mw / 2 - 85, my + mh - 42, 170, 28 };

    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), okBtn);
    bool key = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);

    if (clicked || key) {
        gm->hasDayReport = false;
        return true;
    }
    return false;
}

void Hud_DrawCampDecisionModal(const GameMaster* gm, const Army* army, const Map* map, int screenWidth, int screenHeight)
{
    if (!gm || !gm->showCampDecision || !army || !map) return;

    /* Fondo oscurecido para enfocar la decision */
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 0, 0, 0, 160 });

    int mw = 560;
    int mh = 390;
    int mx = (screenWidth - mw) / 2;
    int my = (screenHeight - mh) / 2;

    /* Ventana Modal */
    DrawRectangle(mx, my, mw, mh, (Color){ 20, 22, 28, 250 });
    DrawRectangleLinesEx((Rectangle){ (float)mx, (float)my, (float)mw, (float)mh }, 2, (Color){ 180, 140, 60, 255 });

    /* Titulo */
    DrawText("DECISION DE LA TARDE", mx + 24, my + 16, 18, GOLD);
    DrawText("El sol se oculta tras el horizonte. Elige como procedera tu ejercito.", mx + 24, my + 40, 12, LIGHTGRAY);
    DrawLine(mx + 24, my + 58, mx + mw - 24, my + 58, (Color){ 80, 70, 50, 255 });

    /* Evaluacion del terreno actual */
    HexTile* tile = Map_GetTile(map, army->position);
    const BalanceConfig* cfg = Balance_GetGlobal();
    int cover = Balance_GetTerrainCover(cfg, tile ? (int)tile->biome : 0, tile ? (int)tile->structure.type : 0);
    
    BiomeProperties bProps = Biome_GetProperties(tile ? tile->biome : BIOME_PLAINS);
    const char* terrainDesc;
    if (tile && tile->structure.type != STRUCT_NONE) {
        terrainDesc = TextFormat("%s (%s, +%d Cobertura)", bProps.name, Structure_GetName(tile->structure.type), cover);
    } else {
        terrainDesc = TextFormat("%s (+%d Cobertura)", bProps.name, cover);
    }

    int evalStat = (int)Army_CalculateEvaluation(army);
    float daysSupply = Army_DaysOfSupply(army);

    /* Riesgo de emboscada */
    const char* riskTxt;
    Color riskCol;
    if (cover >= 90) {
        riskTxt = "NULO (Posicion fortificada)";
        riskCol = GREEN;
    } else if (evalStat + cover >= 4) {
        riskTxt = "BAJO (~0-3%)";
        riskCol = GREEN;
    } else if (evalStat + cover >= 2) {
        riskTxt = "MEDIO (~8-17%)";
        riskCol = ORANGE;
    } else {
        riskTxt = "ALTO (>25%)";
        riskCol = RED;
    }

    Color supplyCol = (daysSupply < 2.0f) ? RED : (daysSupply < 4.0f ? ORANGE : GREEN);

    DrawText(TextFormat("Terreno: %s", terrainDesc), mx + 24, my + 68, 12, RAYWHITE);
    DrawText(TextFormat("Oficial al mando: %s  |  Evaluacion: %d", army->general.name, evalStat), mx + 24, my + 86, 12, RAYWHITE);
    DrawText(TextFormat("Riesgo estimado de emboscada: %s", riskTxt), mx + 24, my + 104, 12, riskCol);
    DrawText(TextFormat("Suministros restantes: %.1f dias (Comida: %.0f | Agua: %.0f)", daysSupply, army->comida_disponible, army->agua_disponible), mx + 24, my + 122, 12, supplyCol);

    /* Tarjetas de Opciones */
    int cardW = (mw - 56) / 2;
    int cardH = 175;
    int cardY = my + 148;

    /* Tarjeta A: Acampar */
    Rectangle cardA = { (float)(mx + 20), (float)cardY, (float)cardW, (float)cardH };
    DrawRectangleRec(cardA, (Color){ 24, 30, 24, 220 });
    DrawRectangleLinesEx(cardA, 1, (Color){ 70, 130, 70, 255 });
    DrawText("OPCION A: ACAMPAR", cardA.x + 12, cardA.y + 10, 13, GREEN);
    DrawText("+ Recupera +6 Moral y +5 Salud", cardA.x + 12, cardA.y + 32, 11, RAYWHITE);
    DrawText("+ Descanso seguro hasta la manana", cardA.x + 12, cardA.y + 48, 11, RAYWHITE);
    DrawText("- Consume 1 dia de suministros", cardA.x + 12, cardA.y + 64, 11, LIGHTGRAY);
    DrawText("- Riesgo de emboscada segun", cardA.x + 12, cardA.y + 80, 11, LIGHTGRAY);
    DrawText("  cobertura y evaluacion", cardA.x + 12, cardA.y + 94, 11, LIGHTGRAY);
    if (daysSupply < 2.0f) {
        DrawText("! ATENCION: Reservas muy bajas", cardA.x + 12, cardA.y + 116, 11, RED);
    }

    /* Tarjeta B: Marcha Nocturna */
    Rectangle cardB = { (float)(mx + 36 + cardW), (float)cardY, (float)cardW, (float)cardH };
    DrawRectangleRec(cardB, (Color){ 32, 24, 26, 220 });
    DrawRectangleLinesEx(cardB, 1, (Color){ 150, 70, 70, 255 });
    DrawText("OPCION B: MARCHA NOCTURNA", cardB.x + 12, cardB.y + 10, 13, (Color){ 230, 110, 100, 255 });
    DrawText("+ Continua el avance de noche", cardB.x + 12, cardB.y + 32, 11, RAYWHITE);
    DrawText("- Coste movimiento x1.5 PA", cardB.x + 12, cardB.y + 48, 11, LIGHTGRAY);
    DrawText("- Desvelo: -2 Moral y -2 Salud/turno", cardB.x + 12, cardB.y + 64, 11, LIGHTGRAY);
    DrawText("- Peligro de encuentros hostiles (35%)", cardB.x + 12, cardB.y + 80, 11, LIGHTGRAY);
    if (army->consecutiveNightMarches > 0) {
        DrawText(TextFormat("! Marchas consecutivas: %d (+%.0f%%)", 
            army->consecutiveNightMarches, army->consecutiveNightMarches * 50.0f), 
            cardB.x + 12, cardB.y + 100, 11, ORANGE);
    }

    /* Botones de Accion */
    Rectangle btnA = { (float)(mx + 20), (float)(my + mh - 48), (float)cardW, 32.0f };
    Rectangle btnB = { (float)(mx + 36 + cardW), (float)(my + mh - 48), (float)cardW, 32.0f };

    bool hoverA = CheckCollisionPointRec(GetMousePosition(), btnA);
    bool hoverB = CheckCollisionPointRec(GetMousePosition(), btnB);

    DrawRectangleRec(btnA, hoverA ? (Color){ 50, 120, 50, 255 } : (Color){ 30, 80, 30, 255 });
    DrawRectangleLinesEx(btnA, 1, hoverA ? GREEN : (Color){ 100, 180, 100, 255 });
    const char* txtA = "[ ACAMPAR (C) ]";
    int twA = MeasureText(txtA, 11);
    DrawText(txtA, btnA.x + (btnA.width - twA) / 2, btnA.y + 10, 11, WHITE);

    DrawRectangleRec(btnB, hoverB ? (Color){ 140, 50, 50, 255 } : (Color){ 90, 30, 30, 255 });
    DrawRectangleLinesEx(btnB, 1, hoverB ? RED : (Color){ 180, 90, 90, 255 });
    const char* txtB = "[ MARCHA NOCTURNA (M) ]";
    int twB = MeasureText(txtB, 11);
    DrawText(txtB, btnB.x + (btnB.width - twB) / 2, btnB.y + 10, 11, WHITE);
}

bool Hud_HandleCampDecisionInput(GameMaster* gm, Map* map, Army* army, int screenWidth, int screenHeight)
{
    if (!gm || !gm->showCampDecision || !army || !map) return false;

    static float campModalTime = 0.0f;
    campModalTime += GetFrameTime();

    /* Debounce: no aceptar input en los primeros 250ms para garantizar que la tecla que abrió el atardecer no se consuma aquí */
    if (campModalTime < 0.25f) return false;

    int mw = 560;
    int mh = 390;
    int mx = (screenWidth - mw) / 2;
    int my = (screenHeight - mh) / 2;
    int cardW = (mw - 56) / 2;

    Rectangle btnA = { (float)(mx + 20), (float)(my + mh - 48), (float)cardW, 32.0f };
    Rectangle btnB = { (float)(mx + 36 + cardW), (float)(my + mh - 48), (float)cardW, 32.0f };

    bool clickA = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), btnA);
    bool clickB = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), btnB);

    bool keyCamp = IsKeyPressed(KEY_C) || IsKeyPressed(KEY_ENTER);
    bool keyMarch = IsKeyPressed(KEY_M);

    if (clickA || keyCamp) {
        campModalTime = 0.0f;
        GameMaster_ChooseCamp(gm, map, army);
        Map_UpdateVision(map, army->position, Army_GetVision(army) + GameMaster_GetVisionModifier(gm, army));
        return true;
    }
    if (clickB || keyMarch) {
        campModalTime = 0.0f;
        GameMaster_ChooseNightMarch(gm, map, army);
        Map_UpdateVision(map, army->position, Army_GetVision(army) + GameMaster_GetVisionModifier(gm, army));
        return true;
    }

    return false;
}






