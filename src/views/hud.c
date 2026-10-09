#include "hud.h"
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
        
        /* Animación rápida de cambio de caras mientras gira */
        if ((int)(hud->dice.rollTimer * 20.0f) % 2 == 0) {
            hud->dice.valor1 = (rand() % 6) + 1;
            hud->dice.valor2 = (rand() % 6) + 1;
        }
        
        if (hud->dice.rollTimer >= hud->dice.rollDuration) {
            hud->dice.rolling = false;
            hud->dice.rollTimer = 0.0f;
            /* Valores finales */
            hud->dice.valor1 = (rand() % 6) + 1;
            hud->dice.valor2 = (rand() % 6) + 1;
        }
    }
}

void Hud_RollDice(HudState* hud)
{
    if (hud && !hud->dice.rolling) {
        hud->dice.rolling = true;
        hud->dice.rollTimer = 0.0f;
    }
}

void Hud_HandleInput(HudState* hud, GameMaster* gm)
{
    if (hud == NULL) return;
    
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        
        if (CheckCollisionPointRec(mouse, hud->cmdPanelRect)) {
            hud->statsExpanded = !hud->statsExpanded;
        }
        
        if (CheckCollisionPointRec(mouse, hud->rollBtnRect)) {
            Hud_RollDice(hud);
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
    
    /* Indicador de menú desplegable */
    const char* toggleText = hud->statsExpanded ? "? Ocultar Stats" : "? Mostrar Stats";
    DrawText(toggleText, portraitX + portraitSize + 12, y + 64, 11, GRAY);
    
    /* Dibujar panel desplegable si está expandido */
    if (hud->statsExpanded) {
        Hud_DrawCommanderStats(army, x, y + hBase, w, 100);
    }
}

void Hud_DrawArmyStatsPanel(const Army* army, const GameMaster* gm, int x, int y, int w, int h)
{
    DrawRectangle(x, y, w, h, (Color){ 20, 18, 25, 230 });
    DrawRectangleLines(x, y, w, h, (Color){ 160, 120, 60, 255 });

    ArmyStats stats = Army_GetStats(army, DEFAULT_EXECUTION_B);
    int total = Army_GetTotalSoldiers(army);

    /* Columna izquierda: tropas */
    DrawText(TextFormat("EJERCITO (PA: %d/%d)", gm->currentActionPoints, gm->maxActionPoints), x+12, y+10, 13, (Color){ 200, 170, 80, 255 });
    DrawLine(x+12, y+24, x+w/2-10, y+24, (Color){ 160, 120, 60, 100 });
    
    DrawText(TextFormat("Sargentos: %d",  army->sargentos), x+12, y+32, 12, LIGHTGRAY);
    DrawText(TextFormat("Soldados:  %d",  army->soldados),  x+12, y+48, 12, LIGHTGRAY);
    DrawText(TextFormat("Iniciados: %d",  army->iniciados), x+12, y+64, 12, LIGHTGRAY);
    DrawText(TextFormat("Total:     %d",  total),           x+12, y+80, 12, WHITE);

    /* Columna derecha: recursos */
    int rx = x + w/2 + 10;
    float waterUsage = Army_CalculateWaterUsage(total, army->ganado);
    float foodUsage  = Army_CalculateFoodUsage(total, army->ganado);
    
    DrawText("RECURSOS", rx, y+10, 13, (Color){ 200, 170, 80, 255 });
    DrawLine(rx, y+24, x+w-12, y+24, (Color){ 160, 120, 60, 100 });
    
    DrawText(TextFormat("Agua:   %.0f (-%.0f/t)", army->agua_disponible,   waterUsage), rx, y+32, 12, SKYBLUE);
    DrawText(TextFormat("Comida: %.0f (-%.0f/t)", army->comida_disponible, foodUsage),  rx, y+48, 12, ORANGE);
    DrawText(TextFormat("Monedas: %d",   army->monedas),  rx, y+64, 12, GOLD);
    DrawText(TextFormat("Ganado:  %d",   army->ganado),   rx, y+80, 12, BEIGE);

    /* Barras de moral y salud */
    int bx = x+12, bw = w-24, bh = 12;
    /* Moral */
    DrawRectangle(bx, y+105, bw, bh, (Color){ 50, 30, 30, 255 });
    DrawRectangle(bx, y+105, (int)(bw * army->moral / 100.0f), bh,
                  (army->moral < 30) ? RED : (army->moral < 60) ? ORANGE : GREEN);
    DrawText(TextFormat("Moral %.0f%%", army->moral), bx+4, y+106, 10, WHITE);
    DrawRectangleLines(bx, y+105, bw, bh, BLACK);

    /* Salud */
    DrawRectangle(bx, y+122, bw, bh, (Color){ 30, 50, 30, 255 });
    DrawRectangle(bx, y+122, (int)(bw * army->salud / 100.0f), bh, GREEN);
    DrawText(TextFormat("Salud %.0f%%", army->salud), bx+4, y+123, 10, WHITE);
    DrawRectangleLines(bx, y+122, bw, bh, BLACK);
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
    DrawRectangle(x, y, w, h, (Color){ 80, 25, 25, 230 });
    DrawRectangleLines(x, y, w, h, (Color){ 200, 60, 60, 255 });
    DrawText(TextFormat("ROLL (PA: %d)", gm ? gm->currentActionPoints : 0), x + w/2 - 35, y + 10, 14, WHITE);

    /* Dados */
    int dieSize = (w - 40) / 2;
    Hud_DrawSingleDie(x + w/4 + 4,   y + h/2 - 10, dieSize, hud->dice.valor1);
    Hud_DrawSingleDie(x + 3*w/4 - 4, y + h/2 - 10, dieSize, hud->dice.valor2);

    /* Boton ROLL */
    hud->rollBtnRect = (Rectangle){ x+12, y+h-34, w-24, 26 };
    bool hover = CheckCollisionPointRec(GetMousePosition(), hud->rollBtnRect);
    
    bool canRoll = (gm && !gm->hasRolledThisTurn && !hud->dice.rolling);
    Color btnCol = GRAY;
    if (canRoll) {
        // Pulsing effect
        float pulse = (sinf(GetTime() * 5.0f) + 1.0f) * 0.5f; // 0.0 to 1.0
        Color baseCol = hover ? (Color){180,50,50,255} : (Color){140,30,30,255};
        btnCol = (Color){ baseCol.r + (unsigned char)(50*pulse), baseCol.g, baseCol.b, 255 };
    }

    DrawRectangleRec(hud->rollBtnRect, btnCol);
    DrawRectangleLinesEx(hud->rollBtnRect, 1, BLACK);
    
    if (canRoll) {
        DrawText("[ TIRAR ]", x + w/2 - 32, y+h-28, 12, WHITE);
    } else {
        DrawText(hud->dice.rolling ? "[ TIRANDO... ]" : "[ YA TIRASTE ]", x + w/2 - 45, y+h-28, 12, LIGHTGRAY);
    }
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
    DrawText("[ENTER] Pasar Turno", screenWidth - 160, 8, 14, YELLOW);
}






