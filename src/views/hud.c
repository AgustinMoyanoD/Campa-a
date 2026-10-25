#include "hud.h"
#include "presentation/audio_manager.h"
#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"
#include "models/balance_config.h"
#include "models/ration.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

void Hud_InitState(HudState* hud)
{
    if (!hud) return;
    memset(hud, 0, sizeof(HudState));
    hud->statsExpanded = false;
    hud->blinkTimer = 0.0f;
    hud->dice.valor1 = 1;
    hud->dice.valor2 = 1;
    hud->dice.rolling = false;
    hud->dice.rollTimer = 0.0f;
    hud->dice.rollDuration = 0.5f;
}

HudState Hud_Init(void)
{
    HudState hud;
    Hud_InitState(&hud);
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
            Audio_PlaySfx(SFX_UI_CLICK);
        }
        
        if (CheckCollisionPointRec(mouse, hud->rollBtnRect) && !gm->hasRolledThisTurn && !hud->dice.rolling) {
            Hud_RollDice(hud);
        }

        if (CheckCollisionPointRec(mouse, hud->forageBtnRect) && army && map && !gm->inCombat && gm->phase == PHASE_PLAYER && !gm->hasForagedToday && gm->timeOfDay != TIME_NIGHT && !gm->showCampDecision) {
            if (GameMaster_Forage(gm, army, map)) {
                Audio_PlaySfx(SFX_FORAGE);
                Map_UpdateVision(map, army->position, Army_GetVision(army) + GameMaster_GetVisionModifier(gm, army));
            }
        }

        if (CheckCollisionPointRec(mouse, hud->endTurnBtnRect) && army && map && !gm->inCombat && gm->phase == PHASE_PLAYER && !gm->hasDayReport && !gm->showCampDecision) {
            GameMaster_EndTurn(gm, map, army);
            Map_UpdateVision(map, army->position, Army_GetVision(army) + GameMaster_GetVisionModifier(gm, army));
        }

        if (CheckCollisionPointRec(mouse, hud->officersBtnRect)) {
            hud->showOfficersPanel = !hud->showOfficersPanel;
            Audio_PlaySfx(SFX_UI_CLICK);
        }

        if (CheckCollisionPointRec(mouse, hud->groupsBtnRect)) {
            hud->showGroupsPanel = !hud->showGroupsPanel;
            Audio_PlaySfx(SFX_UI_CLICK);
        }
    }

    if (IsKeyPressed(KEY_O)) {
        hud->showOfficersPanel = !hud->showOfficersPanel;
        Audio_PlaySfx(SFX_UI_CLICK);
    }

    if (IsKeyPressed(KEY_G)) {
        hud->showGroupsPanel = !hud->showGroupsPanel;
        Audio_PlaySfx(SFX_UI_CLICK);
    }

    if (IsKeyPressed(KEY_F) && army && map && !gm->inCombat && gm->phase == PHASE_PLAYER && !gm->hasDayReport && !gm->hasForagedToday && gm->timeOfDay != TIME_NIGHT && !gm->showCampDecision) {
        if (GameMaster_Forage(gm, army, map)) {
            Audio_PlaySfx(SFX_FORAGE);
            Map_UpdateVision(map, army->position, Army_GetVision(army) + GameMaster_GetVisionModifier(gm, army));
        }
    }
}

static void Hud_DrawDivineCardSlot(int x, int y, int w, int h, bool hasCard, const char* cardId,
                                   bool isExhausted, int battlesUntilRecharge, const GameMaster* gm,
                                   const ModifierDef** outHoverCard, bool* outHoverExhausted, int* outHoverBattles)
{
    Rectangle rSlot = { (float)x, (float)y, (float)w, (float)h };
    Vector2 mousePos = GetMousePosition();
    bool hover = CheckCollisionPointRec(mousePos, rSlot);

    Color slotTint = !hasCard ? (Color){ 100, 105, 120, 255 } :
                     (isExhausted ? (Color){ 115, 118, 125, 255 } : (Color){ 195, 165, 95, 255 });
    UI_DrawSlot(rSlot, slotTint, UI_ICON_NONE);
    if (hasCard && isExhausted) {
        DrawRectangle((int)(rSlot.x + 3), (int)(rSlot.y + 3), (int)(rSlot.width - 6), (int)(rSlot.height - 6), (Color){ 32, 34, 40, 150 });
    }
    if (hover && hasCard) {
        UISkin_DrawHighlight(rSlot, 0.45f, 1);
    }

    if (hasCard) {
        char initial = '?';
        const ModifierDef* cardDef = NULL;
        if (gm) {
            for (int m = 0; m < gm->modRegistry.count; m++) {
                if (strcmp(gm->modRegistry.defs[m].id, cardId) == 0) {
                    cardDef = &gm->modRegistry.defs[m];
                    if (cardDef->source[0] != '\0') {
                        initial = cardDef->source[0];
                    } else if (cardDef->name[0] != '\0') {
                        initial = cardDef->name[0];
                    }
                    break;
                }
            }
        }
        Color initCol = isExhausted ? (Color){ 155, 160, 170, 255 } : GOLD;
        Color orbCol  = isExhausted ? (Color){ 95, 98, 108, 190 }   : (Color){ 180, 120, 255, 200 };
        UI_DrawText(TextFormat("%c", initial), (Vector2){ (float)(x+6), (float)(y+3) }, 14, initCol);
        DrawCircle(x + w/2, y + h/2 + (isExhausted ? 0 : 4), w/5, orbCol);

        if (isExhausted) {
            const char* exBadge = (battlesUntilRecharge > 0) ? TextFormat("-%dB", battlesUntilRecharge) : "AG";
            Vector2 bSz = UI_MeasureText(exBadge, 10.5f);
            DrawRectangle(x + 3, y + h - 15, w - 6, 12, (Color){ 24, 22, 26, 230 });
            UI_DrawText(exBadge, (Vector2){ x + (w - bSz.x) * 0.5f, (float)(y + h - 14) }, 10.5f, (Color){ 235, 165, 140, 255 });
        }

        if (hover && cardDef) {
            if (outHoverCard) *outHoverCard = cardDef;
            if (outHoverExhausted) *outHoverExhausted = isExhausted;
            if (outHoverBattles) *outHoverBattles = battlesUntilRecharge;

            if (isExhausted) {
                UI_RequestTooltip("CARTA AGOTADA",
                                  "Esta carta gasto su poder divino en una invocacion. Reconsagrala en un Templo urbano, en un Altar sagrado o reposa en campamento.",
                                  mousePos);
            } else {
                const char* god = (cardDef->source[0] != '\0') ? cardDef->source : "Olimpo";
                UI_RequestTooltip(cardDef->name, TextFormat("Bendicion de %s\n\n%s", god, cardDef->description), mousePos);
            }
        }
    } else {
        UI_DrawText("?", (Vector2){ (float)(x + w/2 - 4), (float)(y + h/2 - 6) }, 12, DARKGRAY);
    }
}

static void Hud_DrawCommanderStats(const Army* army, int x, int y, int w, int h)
{
    UI_DrawPanelStyled((Rectangle){ (float)x, (float)y, (float)w, (float)h }, UI_STYLE_PANEL_DARK);
    
    CommanderRef cmd = army->commander;
    int ty = y + 10;
    int step = 18;
    UI_DrawText(TextFormat("Vision:        %d", Commander_GetVision(cmd)),     (Vector2){ (float)(x+18), (float)ty }, 14, UI_COLOR_TEXT_MAIN);
    ty += step;
    UI_DrawText(TextFormat("Liderazgo:     %d", Commander_GetLeadership(cmd)), (Vector2){ (float)(x+18), (float)ty }, 14, UI_COLOR_TEXT_MAIN);
    ty += step;
    UI_DrawText(TextFormat("Evaluacion:    %d", Commander_GetEvaluation(cmd)), (Vector2){ (float)(x+18), (float)ty }, 14, UI_COLOR_TEXT_MAIN);
    ty += step;
    UI_DrawText(TextFormat("Planificacion: %d", Commander_GetPlanning(cmd)),   (Vector2){ (float)(x+18), (float)ty }, 14, UI_COLOR_TEXT_MAIN);
    ty += step;
    UI_DrawText(TextFormat("Ejecucion:     %d", Commander_GetExecution(cmd)),  (Vector2){ (float)(x+18), (float)ty }, 14, UI_COLOR_TEXT_MAIN);
    
    if (army->commander.type == OFFICER_LIEUTENANT) {
        ty += step;
        int loy = Commander_GetLoyalty(cmd);
        Color loyColor = (loy < 50) ? UI_COLOR_BAD : ((loy < 75) ? UI_COLOR_WARNING : UI_COLOR_GOOD);
        UI_DrawText(TextFormat("Lealtad:       %d", loy), (Vector2){ (float)(x+18), (float)ty }, 14, loyColor);
    }
}

void Hud_DrawCommanderPanel(const Army* army, const GameMaster* gm, HudState* hud, int x, int y, int w)
{
    char rivalConflict[64] = {0};
    bool hasRivalConflict = (gm && army) ? Modifier_CheckRivals(&gm->modRegistry, army, rivalConflict, sizeof(rivalConflict)) : false;
    int hBase = hasRivalConflict ? 132 : 104;
    
    /* Guardamos el rect para el click (solo la parte superior) */
    hud->cmdPanelRect = (Rectangle){ (float)x, (float)y, (float)w, (float)hBase };
    
    UI_DrawPanelStyled(hud->cmdPanelRect, UI_STYLE_PANEL_DARK);
    
    /* Espacio para el retrato (ranura 9-slice) */
    int portraitSize = 72;
    int portraitX = x + 14;
    int portraitY = y + 14;
    UI_DrawSlot((Rectangle){ (float)portraitX, (float)portraitY, (float)portraitSize, (float)portraitSize }, (Color){0, 0, 0, 0}, UI_ICON_NONE);
    UI_DrawText("[Sprite]", (Vector2){ (float)(portraitX + 12), (float)(portraitY + 28) }, 13, UI_COLOR_TEXT_DIM);
    
    /* Nombre y tipo de oficial */
    CommanderRef cmd = army->commander;
    const char* name = Commander_GetName(cmd);
    const char* rank = (cmd.type == OFFICER_GENERAL) ? "General Supremo" : "Teniente";
    
    int slots = (cmd.type == OFFICER_GENERAL) ? MAX_GENERAL_DIVINE_CARDS : MAX_LIEUTENANT_DIVINE_CARDS;
    int cardCount = Commander_GetCardCount(cmd);
    int cardW = 40;
    int cardH = 54;
    int cardX = x + w - (slots * (cardW + 8)) - 14;
    int cardY = y + 14;

    float colX = (float)(portraitX + portraitSize + 14);
    float maxNameW = (float)cardX - colX - 8.0f;
    float nameFont = 18.0f;
    while (UI_MeasureText(name, nameFont).x > maxNameW && nameFont > 12.0f) {
        nameFont -= 0.5f;
    }
    UI_DrawText(name, (Vector2){ colX, (float)(y + 14) }, nameFont, UI_COLOR_TEXT_MAIN);
    UI_DrawText(rank, (Vector2){ colX, (float)(y + 36) }, 13, (cmd.type == OFFICER_GENERAL) ? UI_COLOR_GOLD : UI_COLOR_TEXT_DIM);

    const ModifierDef* hoverCard = NULL;
    bool hoverExhausted = false;
    int hoverBattles = 0;
    
    for (int i = 0; i < slots; i++) {
        bool hasCard = (i < cardCount);
        const char* cardId = "";
        bool isEx = false;
        int battlesLeft = 0;
        if (hasCard) {
            cardId = Commander_GetCard(cmd, i);
            ModifierInstance* inst = Commander_GetCardInstance(cmd, i);
            if (inst) {
                isEx = inst->isExhausted;
                battlesLeft = inst->battlesUntilRecharge;
            }
        }
        Hud_DrawDivineCardSlot(cardX + i * (cardW + 8), cardY, cardW, cardH, hasCard, cardId,
                               isEx, battlesLeft, gm, &hoverCard, &hoverExhausted, &hoverBattles);
    }
    
    /* Maldiciones activas (colocadas en la columna izquierda para NUNCA superponerse a las cartas) */
    int curseCount = Commander_GetCurseCount(cmd) + army->curseCount;
    if (curseCount > 0) {
        float maxCurseW = fminf(124.0f, (float)cardX - colX - 6.0f);
        Rectangle curseBadge = { colX, (float)(y + 52), maxCurseW, 20.0f };
        bool cHover = CheckCollisionPointRec(GetMousePosition(), curseBadge);
        UI_DrawStatusChip(curseBadge, TextFormat("%d Maldicion%s", curseCount, (curseCount > 1 ? "es" : "")), UI_STYLE_CHIP_BAD, UI_ICON_PELIGRO);

        if (cHover) {
            char curseDescBuf[512] = {0};
            int offset = 0;
            int cmdCurses = Commander_GetCurseCount(cmd);
            for (int ci = 0; ci < cmdCurses; ci++) {
                const char* cid = Commander_GetCurse(cmd, ci);
                const ModifierDef* cdef = gm ? Modifier_GetById(gm->modRegistry.defs, gm->modRegistry.count, cid) : NULL;
                const char* cname = cdef ? cdef->name : cid;
                offset += snprintf(curseDescBuf + offset, sizeof(curseDescBuf) - offset, "- %s\n", cname);
            }
            for (int ai = 0; ai < army->curseCount; ai++) {
                const char* cid = army->curses[ai].id;
                const ModifierDef* cdef = gm ? Modifier_GetById(gm->modRegistry.defs, gm->modRegistry.count, cid) : NULL;
                const char* cname = cdef ? cdef->name : cid;
                offset += snprintf(curseDescBuf + offset, sizeof(curseDescBuf) - offset, "- %s (Ejercito)\n", cname);
            }
            UI_RequestTooltip("MALDICIONES ACTIVAS", curseDescBuf, GetMousePosition());
        }
    }
    
    /* Indicador de menu desplegable */
    const char* toggleText = hud->statsExpanded ? "[-] Ocultar Stats" : "[+] Mostrar Stats";
    UI_DrawText(toggleText, (Vector2){ colX, (float)(y + 78) }, 13, UI_COLOR_TEXT_DIM);

    /* Boton de Oficiales (E3) */
    hud->officersBtnRect = (Rectangle){ (float)(x + w - 125), (float)(y + 74), 115.0f, 24.0f };
    UI_DrawButtonStyledSfx(hud->officersBtnRect, "Oficiales (O)", UI_STYLE_BTN_DEFAULT, false, NULL);

    /* 3.2 Indicador de Rivalidad entre Dioses (Discordia Divina) */
    if (hasRivalConflict) {
        const char* pairStr = (strncmp(rivalConflict, "Disputa: ", 9) == 0) ? (rivalConflict + 9) : rivalConflict;
        Rectangle rivalChip = { (float)(x + 12), (float)(y + 102), (float)(w - 24), 24.0f };
        UI_DrawStatusChip(rivalChip, TextFormat("[DISCORDIA DIVINA: %s]", pairStr), UI_STYLE_CHIP_BAD, UI_ICON_PELIGRO);
        /* Icono de espadas cruzadas a la derecha del chip */
        Vector2 swPos = { rivalChip.x + rivalChip.width - 14.0f, rivalChip.y + 12.0f };
        DrawLineEx((Vector2){ swPos.x - 5.0f, swPos.y + 5.0f }, (Vector2){ swPos.x + 5.0f, swPos.y - 5.0f }, 2.0f, (Color){ 255, 190, 140, 255 });
        DrawLineEx((Vector2){ swPos.x - 5.0f, swPos.y - 5.0f }, (Vector2){ swPos.x + 5.0f, swPos.y + 5.0f }, 2.0f, (Color){ 255, 190, 140, 255 });
        if (CheckCollisionPointRec(GetMousePosition(), rivalChip)) {
            UI_RequestTooltip("DISCORDIA DIVINA",
                              "Los dioses rivales disputan en el campamento. Penalizacion de -1 a todas las tiradas de planificacion tactica.",
                              GetMousePosition());
        }
    }
    
    /* Dibujar panel desplegable si esta expandido */
    if (hud->statsExpanded) {
        Hud_DrawCommanderStats(army, x, y + hBase, w, 120);
    }

    /* Vista previa de carta divina completa al hacer hover sobre una ranura */
    if (hoverCard) {
        Rectangle previewRect = { (float)(x + w + 12), (float)y, 240.0f, 360.0f };
        UI_DrawModifierCardEx(hoverCard, previewRect, hoverExhausted, hoverBattles);
    }
}

void Hud_DrawArmyStatsPanel(const Army* army, const GameMaster* gm, const Map* map, HudState* hud, int x, int y, int w, int h)
{
    UI_DrawPanelStyled((Rectangle){ (float)x, (float)y, (float)w, (float)h }, UI_STYLE_PANEL_DARK);

    int total = Army_GetTotalSoldiers(army);
    Vector2 mouse = GetMousePosition();

    /* Columna izquierda: tropas */
    const char* troopHeader = TextFormat("EJERCITO (PA: %d/%d)", gm ? gm->currentActionPoints : 0, gm ? gm->maxActionPoints : 0);
    UI_DrawText(troopHeader, (Vector2){ (float)(x + 14), (float)(y + 8) }, 15, UI_COLOR_GOLD);
    DrawLine(x + 14, y + 26, x + 205, y + 26, Fade(UI_COLOR_PANEL_BORDER, 0.7f));

    /* Estados de cada grupo militar */
    GroupState stSgt = Ration_GetGroupState(army, RG_SARGENTOS);
    GroupState stSol = Ration_GetGroupState(army, RG_SOLDADOS);
    GroupState stIni = Ration_GetGroupState(army, RG_INICIADOS);

    Color colSgt = (stSgt.morale < 35.0f) ? UI_COLOR_BAD : ((stSgt.morale < 65.0f) ? UI_COLOR_WARNING : UI_COLOR_GOOD);
    Color colSol = (stSol.morale < 35.0f) ? UI_COLOR_BAD : ((stSol.morale < 65.0f) ? UI_COLOR_WARNING : UI_COLOR_GOOD);
    Color colIni = (stIni.morale < 35.0f) ? UI_COLOR_BAD : ((stIni.morale < 65.0f) ? UI_COLOR_WARNING : UI_COLOR_GOOD);

    /* Fila 1 tropas: Sargentos y Soldados */
    int tRow1Y = y + 32;
    UI_DrawText(TextFormat("Sgt: %d", army->sargentos), (Vector2){ (float)(x + 14), (float)tRow1Y }, 13, UI_COLOR_TEXT_MAIN);
    DrawCircle(x + 64, tRow1Y + 7, 3.5f, colSgt);
    UI_DrawText(TextFormat("%.0f%%", stSgt.morale), (Vector2){ (float)(x + 71), (float)tRow1Y }, 11, colSgt);
    if (CheckCollisionPointRec(mouse, (Rectangle){ (float)(x + 14), (float)tRow1Y, 95.0f, 16.0f })) {
        UI_RequestTooltip("SARGENTOS", TextFormat("Moral: %.0f%% | Salud: %.0f%%\nSatisfaccion: %.0f/%.0f\nDias sin racion: %d",
                          stSgt.morale, stSgt.health, stSgt.satisfaction, stSgt.satisfactionMax, stSgt.daysUnfed), mouse);
    }

    UI_DrawText(TextFormat("Sol: %d", army->soldados), (Vector2){ (float)(x + 112), (float)tRow1Y }, 13, UI_COLOR_TEXT_MAIN);
    DrawCircle(x + 162, tRow1Y + 7, 3.5f, colSol);
    UI_DrawText(TextFormat("%.0f%%", stSol.morale), (Vector2){ (float)(x + 169), (float)tRow1Y }, 11, colSol);
    if (CheckCollisionPointRec(mouse, (Rectangle){ (float)(x + 112), (float)tRow1Y, 95.0f, 16.0f })) {
        UI_RequestTooltip("SOLDADOS", TextFormat("Moral: %.0f%% | Salud: %.0f%%\nDias sin racion: %d",
                          stSol.morale, stSol.health, stSol.daysUnfed), mouse);
    }

    /* Fila 2 tropas: Iniciados y Total */
    int tRow2Y = y + 54;
    UI_DrawText(TextFormat("Ini: %d", army->iniciados), (Vector2){ (float)(x + 14), (float)tRow2Y }, 13, UI_COLOR_TEXT_MAIN);
    DrawCircle(x + 64, tRow2Y + 7, 3.5f, colIni);
    UI_DrawText(TextFormat("%.0f%%", stIni.morale), (Vector2){ (float)(x + 71), (float)tRow2Y }, 11, colIni);
    if (CheckCollisionPointRec(mouse, (Rectangle){ (float)(x + 14), (float)tRow2Y, 95.0f, 16.0f })) {
        UI_RequestTooltip("INICIADOS", TextFormat("Moral: %.0f%% | Salud: %.0f%%\nDias sin racion: %d",
                          stIni.morale, stIni.health, stIni.daysUnfed), mouse);
    }

    UI_DrawText(TextFormat("Total: %d", total), (Vector2){ (float)(x + 112), (float)tRow2Y }, 13, UI_COLOR_TEXT_TITLE);

    /* Fila 3: Boton Grupos */
    if (hud) {
        hud->groupsBtnRect = (Rectangle){ (float)(x + 14), (float)(y + 76), 105.0f, 24.0f };
        UI_DrawButtonStyledSfx(hud->groupsBtnRect, "Grupos (G)", UI_STYLE_BTN_DEFAULT, false, "ui_click");
    }

    /* Divisor vertical sutil */
    DrawLine(x + 218, y + 8, x + 218, y + 104, Fade(UI_COLOR_PANEL_BORDER, 0.4f));

    /* Columna derecha: recursos */
    int rx = x + 228;
    UI_DrawText("SUMINISTROS", (Vector2){ (float)rx, (float)(y + 8) }, 15, UI_COLOR_GOLD);
    DrawLine(rx, y + 26, x + w - 14, y + 26, Fade(UI_COLOR_PANEL_BORDER, 0.7f));

    RationNeeds rNeeds = Ration_GetNeeds(army, map, army->position);
    float totalDailyDrink = 0.0f;
    float totalDailyFood  = 0.0f;
    for (int g = 0; g < RG_COUNT; g++) {
        totalDailyDrink += rNeeds.drink[g];
        totalDailyFood  += rNeeds.food[g];
    }
    if (totalDailyDrink < 0.1f) totalDailyDrink = 1.0f;
    if (totalDailyFood < 0.1f)  totalDailyFood = 1.0f;

    float aguaDays   = army->agua_disponible / totalDailyDrink;
    float vinoDays   = army->vino_disponible / totalDailyDrink;
    float comidaDays = army->comida_disponible / totalDailyFood;
    float carneDays  = army->carne_disponible / totalDailyFood;

    int rwPill = (w - 228 - 14 - 8) / 2;
    int rhPill = 22;

    /* Fila 1 recursos: Agua y Vino (9-slice chips con iconos) */
    int resY1 = y + 31;
    Rectangle rAgua = { (float)rx, (float)resY1, (float)rwPill, (float)rhPill };
    UI_DrawStatusChip(rAgua, TextFormat("%.1fd", aguaDays), (aguaDays < 2.0f) ? UI_STYLE_CHIP_WARN : UI_STYLE_CHIP_OK, UI_ICON_AGUA);
    if (CheckCollisionPointRec(mouse, rAgua)) {
        UI_RequestTooltip("AGUA", TextFormat("Stock disponible: %.0f unidades\nDuracion estimada: %.1f dias", army->agua_disponible, aguaDays), mouse);
    }

    Rectangle rVino = { (float)(rx + rwPill + 8), (float)resY1, (float)rwPill, (float)rhPill };
    UI_DrawStatusChip(rVino, TextFormat("%.1fd", vinoDays), UI_STYLE_CHIP_OK, UI_ICON_VINO);
    if (CheckCollisionPointRec(mouse, rVino)) {
        UI_RequestTooltip("VINO", TextFormat("Stock disponible: %.0f unidades (Lujo)\nSustituto de Agua. Sube moral y satisfaccion.", army->vino_disponible), mouse);
    }

    /* Fila 2 recursos: Comida y Carne (9-slice chips con iconos) */
    int resY2 = y + 55;
    Rectangle rComida = { (float)rx, (float)resY2, (float)rwPill, (float)rhPill };
    UI_DrawStatusChip(rComida, TextFormat("%.1fd", comidaDays), (comidaDays < 2.0f) ? UI_STYLE_CHIP_WARN : UI_STYLE_CHIP_OK, UI_ICON_COMIDA);
    if (CheckCollisionPointRec(mouse, rComida)) {
        UI_RequestTooltip("COMIDA", TextFormat("Stock disponible: %.0f unidades\nDuracion estimada: %.1f dias", army->comida_disponible, comidaDays), mouse);
    }

    Rectangle rCarne = { (float)(rx + rwPill + 8), (float)resY2, (float)rwPill, (float)rhPill };
    UI_DrawStatusChip(rCarne, TextFormat("%.1fd", carneDays), UI_STYLE_CHIP_OK, UI_ICON_CARNE);
    if (CheckCollisionPointRec(mouse, rCarne)) {
        UI_RequestTooltip("CARNE", TextFormat("Stock disponible: %.0f unidades (Lujo)\nSustituto de Comida obtenido por sacrificio.", army->carne_disponible), mouse);
    }

    /* Fila 3 recursos: Oro y Ganado */
    int resY3 = y + 80;
    UI_DrawText(TextFormat("Oro: %d", army->monedas), (Vector2){ (float)rx, (float)resY3 }, 13, UI_COLOR_GOLD);
    if (CheckCollisionPointRec(mouse, (Rectangle){ (float)rx, (float)resY3, 75, 18 })) UI_RequestGlossaryTooltip("ORO", mouse);

    UI_DrawIcon(UI_ICON_GANADO, (Vector2){ (float)(rx + rwPill + 6), (float)(resY3 - 1) }, 16.0f, WHITE);
    UI_DrawText(TextFormat("%d", army->ganado), (Vector2){ (float)(rx + rwPill + 26), (float)resY3 }, 13, (Color){ 235, 185, 130, 255 });
    if (CheckCollisionPointRec(mouse, (Rectangle){ (float)(rx + rwPill + 6), (float)resY3, 85, 18 })) UI_RequestGlossaryTooltip("GANADO", mouse);

    /* Separador horizontal inferior */
    DrawLine(x + 14, y + 106, x + w - 14, y + 106, Fade(UI_COLOR_PANEL_BORDER, 0.6f));

    /* Barras de moral y salud en paralelo (lado a lado con 9-slice e iconos) */
    int barW = (w - 28 - 12) / 2;
    int barH = 18;
    int barY = y + 112;

    /* Barra 1: Moral */
    Rectangle rMoral = { (float)(x + 14), (float)barY, (float)barW, (float)barH };
    UI_DrawLabeledBar(rMoral, army->moral / 100.0f, TextFormat("Moral %.0f%%", army->moral),
                      (army->moral < 30) ? UI_COLOR_BAD : (army->moral < 60) ? UI_COLOR_WARNING : UI_COLOR_GOOD, UI_ICON_MORAL);
    if (CheckCollisionPointRec(mouse, rMoral)) UI_RequestGlossaryTooltip("MORAL", mouse);

    /* Barra 2: Salud */
    Rectangle rSalud = { (float)(x + 14 + barW + 12), (float)barY, (float)barW, (float)barH };
    UI_DrawLabeledBar(rSalud, army->salud / 100.0f, TextFormat("Salud %.0f%%", army->salud),
                      (army->salud < 40) ? UI_COLOR_BAD : UI_COLOR_GOOD, UI_ICON_SALUD);
    if (CheckCollisionPointRec(mouse, rSalud)) UI_RequestGlossaryTooltip("SALUD", mouse);

    /* Advertencia de hambruna si aplica */
    if (army->daysWithoutFood > 0 || army->daysWithoutWater > 0) {
        UI_DrawText("[!] CONDICION CRITICA DE SUPERVIVENCIA", (Vector2){ (float)(x + 14), (float)(barY + barH + 3) }, 11, UI_COLOR_BAD);
    }
}

static void Hud_DrawSingleDie(int cx, int cy, int size, int value)
{
    UI_DrawSingleDie(cx, cy, size, value);
}

void Hud_DrawDicePanel(HudState* hud, const GameMaster* gm, const Army* army, const Map* map, int x, int y, int w, int h)
{
    UI_DrawPanelStyled((Rectangle){ (float)x, (float)y, (float)w, (float)h }, UI_STYLE_PANEL_DARK);

    const char* paTitle = TextFormat("ACCIONES (PA: %d)", gm ? gm->currentActionPoints : 0);
    Vector2 paSize = UI_MeasureText(paTitle, 15);
    UI_DrawText(paTitle, (Vector2){ x + (w - paSize.x)/2.0f, (float)(y + 7) }, 15, UI_COLOR_GOLD);
    
    Vector2 mouseDice = GetMousePosition();
    if (CheckCollisionPointRec(mouseDice, (Rectangle){ (float)(x + 8), (float)(y + 4), (float)(w - 16), 22 })) {
        UI_RequestGlossaryTooltip("PA", mouseDice);
    }

    /* Dados estilo hueso compactos */
    int dieSize = 26;
    Hud_DrawSingleDie(x + w/4 + 6,   y + 40, dieSize, hud->dice.valor1);
    Hud_DrawSingleDie(x + 3*w/4 - 6, y + 40, dieSize, hud->dice.valor2);

    float btnH = 28.0f;
    float btnW = (float)(w - 16);
    float btnX = (float)(x + 8);

    /* Boton 1: ROLL (SPACE) */
    hud->rollBtnRect = (Rectangle){ btnX, (float)(y + 60), btnW, btnH };
    bool canRoll = (gm && !gm->hasRolledThisTurn && !hud->dice.rolling && !gm->showCampDecision);
    const char* rollTxt = canRoll ? "TIRAR DADOS (ESPACIO)" : (hud->dice.rolling ? "TIRANDO..." : "YA TIRASTE");
    UI_DrawButtonStyledSfx(hud->rollBtnRect, rollTxt, UI_STYLE_BTN_DANGER, !canRoll, NULL);

    /* Boton 2: FORRAJE (F) */
    hud->forageBtnRect = (Rectangle){ btnX, (float)(y + 92), btnW, btnH };
    bool canForage = (gm && !gm->inCombat && gm->phase == PHASE_PLAYER && !gm->hasDayReport && !gm->hasForagedToday && gm->timeOfDay != TIME_NIGHT && !gm->showCampDecision);
    const char* forageTxt;
    if (gm && gm->hasForagedToday) forageTxt = "YA FORRAJEASTE";
    else if (gm && gm->timeOfDay == TIME_NIGHT) forageTxt = "NOCHE: SIN FORRAJE";
    else forageTxt = "FORRAJEAR (F)";
    UI_DrawButtonStyledSfx(hud->forageBtnRect, forageTxt, UI_STYLE_BTN_PRIMARY, !canForage, NULL);

    if (CheckCollisionPointRec(mouseDice, hud->forageBtnRect) && canForage && map) {
        HexTile* curTile = Map_GetTile(map, army->position);
        int bIdx = curTile ? (int)curTile->biome : 0;
        if (bIdx < 0 || bIdx > 5) bIdx = 0;
        const BalanceConfig* bcfg = Balance_GetGlobal();
        float expFood = bcfg ? bcfg->forageFood[bIdx] : 1.0f;
        float expWater = bcfg ? bcfg->forageWater[bIdx] : 0.5f;
        float costDays = (gm->timeOfDay == TIME_MORNING) ? 0.5f : 0.25f;
        UI_RequestTooltip("FORRAJEAR", TextFormat("Estimado: ~+%.1fd comida / +%.1fd agua\nCoste: -%.2f jornada", expFood, expWater, costDays), mouseDice);
    }

    /* Boton 3: PASAR TURNO / ATARDECER (ENTER) */
    hud->endTurnBtnRect = (Rectangle){ btnX, (float)(y + 124), btnW, btnH };
    bool isAfternoon = (gm && gm->timeOfDay == TIME_AFTERNOON);
    const char* endTurnTxt = isAfternoon ? "ATARDECER (ENTER)" : "PASAR TURNO (ENTER)";
    UI_DrawButtonStyledSfx(hud->endTurnBtnRect, endTurnTxt, UI_STYLE_BTN_DEFAULT, false, NULL);
}

void Hud_DrawTopBar(const GameMaster* gm, int screenWidth) {
    if (!gm) return;
    int h = 40;
    DrawRectangle(0, 0, screenWidth, h, UI_COLOR_PANEL);
    DrawLine(0, h, screenWidth, h, UI_COLOR_PANEL_BORDER);

    const char* timeStr = "Manana";
    switch(gm->timeOfDay) {
        case TIME_MORNING: timeStr = "Manana"; break;
        case TIME_MIDDAY: timeStr = "Mediodia"; break;
        case TIME_AFTERNOON: timeStr = "Tarde"; break;
        case TIME_NIGHT: timeStr = "Noche"; break;
    }

    const char* dayInfo = TextFormat("Dia %d - %s (Turno %d)", gm->currentDay, timeStr, gm->currentTurn);
    UI_DrawText(dayInfo, (Vector2){ 24.0f, 10.0f }, 16, UI_COLOR_TEXT_MAIN);

    const char* infamyText = TextFormat("Infamia: %.1f%%", gm->infamy);
    Vector2 infSize = UI_MeasureText(infamyText, 16);
    float infX = screenWidth / 2.0f - infSize.x / 2.0f;
    UI_DrawText(infamyText, (Vector2){ infX, 10.0f }, 16, UI_COLOR_BAD);
    
    Vector2 mouseTop = GetMousePosition();
    if (CheckCollisionPointRec(mouseTop, (Rectangle){ infX - 10.0f, 4.0f, infSize.x + 20.0f, 32.0f })) {
        UI_RequestGlossaryTooltip("INFAMIA", mouseTop);
    }

    const char* shortcutText = (gm->timeOfDay == TIME_AFTERNOON) ? "[ENTER] Atardecer / Noche" : "[ENTER] Pasar Turno";
    Vector2 shortSize = UI_MeasureText(shortcutText, 14);
    UI_DrawText(shortcutText, (Vector2){ screenWidth - shortSize.x - 24.0f, 11.0f }, 14, (gm->timeOfDay == TIME_AFTERNOON) ? UI_COLOR_GOLD : UI_COLOR_TEXT_DIM);
}

void Hud_DrawDayReportModal(const DayReport* report, int screenWidth, int screenHeight)
{
    if (!report) return;

    int mw = 560;
    int mh = 390;
    int mx = (screenWidth - mw) / 2;
    int my = (screenHeight - mh) / 2;

    UI_DrawModalWindow((Rectangle){ (float)mx, (float)my, (float)mw, (float)mh }, TextFormat("RESUMEN DEL DIA %d", report->dayNumber), NULL, UI_STYLE_PANEL_DARK);

    /* Consumo del dia */
    int ty = my + 58;
    UI_DrawText("CONSUMO DE RECURSOS:", (Vector2){ (float)(mx + 28), (float)ty }, 15, (Color){ 200, 170, 80, 255 });
    ty += 22;
    UI_DrawIcon(UI_ICON_COMIDA, (Vector2){ (float)(mx + 38), (float)ty }, 16.0f, WHITE);
    UI_DrawText(TextFormat("Comida consumida: %.0f raciones", report->foodConsumed), (Vector2){ (float)(mx + 60), (float)ty }, 14, LIGHTGRAY);
    ty += 20;
    UI_DrawIcon(UI_ICON_AGUA, (Vector2){ (float)(mx + 38), (float)ty }, 16.0f, WHITE);
    UI_DrawText(TextFormat("Agua consumida:   %.0f unidades", report->waterConsumed), (Vector2){ (float)(mx + 60), (float)ty }, 14, LIGHTGRAY);
    ty += 26;

    /* Estado de Hambruna / Deshidratacion */
    UI_DrawText("ESTADO DE SUPERVIVENCIA:", (Vector2){ (float)(mx + 28), (float)ty }, 15, (Color){ 200, 170, 80, 255 });
    ty += 22;
    if (report->foodShortage) {
        UI_DrawIcon(UI_ICON_PELIGRO, (Vector2){ (float)(mx + 38), (float)ty }, 16.0f, WHITE);
        UI_DrawText(TextFormat("ESCASEZ DE COMIDA: %d dia(s) en hambruna", report->daysWithoutFood), (Vector2){ (float)(mx + 60), (float)ty }, 14, RED);
    } else {
        UI_DrawIcon(UI_ICON_OK, (Vector2){ (float)(mx + 38), (float)ty }, 16.0f, WHITE);
        UI_DrawText("Raciones de comida suficientes", (Vector2){ (float)(mx + 60), (float)ty }, 14, GREEN);
    }
    ty += 20;
    if (report->waterShortage) {
        UI_DrawIcon(UI_ICON_PELIGRO, (Vector2){ (float)(mx + 38), (float)ty }, 16.0f, WHITE);
        UI_DrawText(TextFormat("ESCASEZ DE AGUA: %d dia(s) en sed extrema", report->daysWithoutWater), (Vector2){ (float)(mx + 60), (float)ty }, 14, RED);
    } else {
        UI_DrawIcon(UI_ICON_OK, (Vector2){ (float)(mx + 38), (float)ty }, 16.0f, WHITE);
        UI_DrawText("Reserva de agua suficiente", (Vector2){ (float)(mx + 60), (float)ty }, 14, GREEN);
    }
    ty += 26;

    /* Impacto en la tropa */
    UI_DrawText("IMPACTO EN LA TROPA:", (Vector2){ (float)(mx + 28), (float)ty }, 15, (Color){ 200, 170, 80, 255 });
    ty += 22;
    Color statCol = (report->moralDelta < 0 || report->saludDelta < 0) ? ORANGE : RAYWHITE;
    UI_DrawText(TextFormat("Moral: %+.0f  |  Salud: %+.0f", report->moralDelta, report->saludDelta), (Vector2){ (float)(mx + 42), (float)ty }, 14, statCol);
    ty += 20;

    int totalDeserters = report->desertedInitiates + report->desertedSoldiers + report->desertedSergeants;
    int totalDeaths = report->deadInitiates + report->deadSoldiers + report->deadSergeants;
    if (totalDeserters > 0 || totalDeaths > 0) {
        UI_DrawText(TextFormat("Bajas por hambre: %d muertos | %d desertores", totalDeaths, totalDeserters), (Vector2){ (float)(mx + 42), (float)ty }, 14, RED);
    } else {
        UI_DrawText("Sin bajas ni deserciones registradas.", (Vector2){ (float)(mx + 42), (float)ty }, 14, LIGHTGRAY);
    }

    /* Boton Continuar */
    Rectangle okBtn = { (float)(mx + mw / 2 - 100), (float)(my + mh - 50), 200.0f, 36.0f };
    UI_DrawButtonStyledSfx(okBtn, "[ CONTINUAR (ENTER) ]", UI_STYLE_BTN_PRIMARY, false, "");
}

bool Hud_HandleDayReportInput(GameMaster* gm, int screenWidth, int screenHeight)
{
    if (!gm || !gm->hasDayReport) return false;

    int mw = 560;
    int mh = 390;
    int mx = (screenWidth - mw) / 2;
    int my = (screenHeight - mh) / 2;
    Rectangle okBtn = { (float)(mx + mw / 2 - 100), (float)(my + mh - 50), 200.0f, 36.0f };

    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), okBtn);
    bool key = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);

    if (clicked || key) {
        Audio_PlaySfx(SFX_UI_CONFIRM);
        gm->hasDayReport = false;
        return true;
    }
    return false;
}

void Hud_DrawCampDecisionModal(const GameMaster* gm, const Army* army, const Map* map, int screenWidth, int screenHeight)
{
    if (!gm || !gm->showCampDecision || !army || !map) return;

    /* Evaluacion del terreno actual */
    HexTile* tile = Map_GetTile(map, army->position);
    const BalanceConfig* cfg = Balance_GetGlobal();
    int cover = Balance_GetTerrainCover(cfg, tile ? (int)tile->biome : 0, tile ? (int)tile->structure.type : 0);

    /* Verificación si el hexágono actual cuenta con posada */
    bool hasInn = false;
    int innCost = 10;
    if (tile && tile->structure.type != STRUCT_NONE && !tile->structure.isBesieged) {
        if (tile->structure.type == STRUCT_TOWN_HOUSES ||
            tile->structure.type == STRUCT_TOWN_MARKET ||
            tile->structure.type == STRUCT_TOWN_CASTLE) {
            int fId = tile->structure.factionId;
            bool isHostile = (fId >= 0 && fId < 3 && cfg && gm->factions[fId].opinion <= cfg->factionsThresholdHostile);
            if (!isHostile) {
                hasInn = true;
                innCost = (cfg && cfg->servicesInnRestCost > 0) ? cfg->servicesInnRestCost : 10;
                if (fId >= 0 && fId < 3 && cfg && gm->factions[fId].opinion >= cfg->factionsThresholdAllied) {
                    innCost = 0;
                }
            }
        }
    }

    int mw = hasInn ? 840 : 640;
    int mh = 470;
    int mx = (screenWidth - mw) / 2;
    int my = (screenHeight - mh) / 2;

    UI_DrawModalWindow((Rectangle){ (float)mx, (float)my, (float)mw, (float)mh }, "DECISION DE LA TARDE", "El sol se oculta tras el horizonte. Elige como procedera tu ejercito.", UI_STYLE_PANEL_DARK);
    
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

    Vector2 mouseCamp = GetMousePosition();

    UI_DrawText(TextFormat("Terreno: %s", terrainDesc), (Vector2){ (float)(mx + 28), (float)(my + 76) }, 14, RAYWHITE);
    UI_DrawText(TextFormat("Oficial al mando: %s  |  Evaluacion: %d", army->general.name, evalStat), (Vector2){ (float)(mx + 28), (float)(my + 96) }, 14, RAYWHITE);
    UI_DrawText(TextFormat("Riesgo estimado de emboscada: %s", riskTxt), (Vector2){ (float)(mx + 28), (float)(my + 116) }, 14, riskCol);
    if (CheckCollisionPointRec(mouseCamp, (Rectangle){ (float)(mx + 28), (float)(my + 116), 320, 18 })) {
        UI_RequestGlossaryTooltip("EMBOSCADA", mouseCamp);
    }
    UI_DrawText(TextFormat("Suministros restantes: %.1f dias (Comida: %.0f | Agua: %.0f)", daysSupply, army->comida_disponible, army->agua_disponible), (Vector2){ (float)(mx + 28), (float)(my + 136) }, 14, supplyCol);

    /* 5.2 Banner de Reagrupamiento de Rezagados tras derrota reciente */
    bool hasRecentDefeat = GameMaster_HasFlag(gm, "derrota_reciente");
    if (hasRecentDefeat) {
        Rectangle regroupBanner = { (float)(mx + 24), (float)(my + 158), (float)(mw - 48), 28.0f };
        UISkin_DrawGlow(regroupBanner, (Color){ 120, 235, 110, 255 }, 0.45f, 0);
        UI_DrawStatusChip(regroupBanner,
                          "[REAGRUPAMIENTO]: Los soldados dispersos regresan al campamento (+3 Iniciados, +2 Soldados, Salud reorganizada a 35+, +10 Moral).",
                          UI_STYLE_CHIP_OK, UI_ICON_MORAL);
    }

    /* Tarjetas de Opciones */
    int cardW = hasInn ? ((mw - 64) / 3) : ((mw - 64) / 2);
    int cardH = hasRecentDefeat ? 168 : 190;
    int cardY = hasRecentDefeat ? (my + 194) : (my + 172);

    bool inForest = tile && (tile->biome == BIOME_FOREST || tile->biome == BIOME_JUNGLE);

    /* Tarjeta A: Acampar */
    Rectangle cardA = { (float)(mx + (hasInn ? 16 : 24)), (float)cardY, (float)cardW, (float)cardH };
    UI_DrawPanelStyled(cardA, UI_STYLE_PANEL_DARK);
    UI_DrawText("OPCION A: ACAMPAR", (Vector2){ cardA.x + 14.0f, cardA.y + 12.0f }, 15, GREEN);
    if (inForest) {
        UI_DrawText("+ Recupera +9 Moral y +8 Salud (+3 Bosque)", (Vector2){ cardA.x + 14.0f, cardA.y + 38.0f }, 13, (Color){ 150, 240, 150, 255 });
        UI_DrawText("+ Oculto en bosque: evade cazadores", (Vector2){ cardA.x + 14.0f, cardA.y + 58.0f }, 13, (Color){ 150, 240, 150, 255 });
    } else {
        UI_DrawText("+ Recupera +6 Moral y +5 Salud", (Vector2){ cardA.x + 14.0f, cardA.y + 38.0f }, 13, RAYWHITE);
        UI_DrawText("+ Descanso seguro hasta la manana", (Vector2){ cardA.x + 14.0f, cardA.y + 58.0f }, 13, RAYWHITE);
    }
    UI_DrawText("- Reparto diario de provisiones", (Vector2){ cardA.x + 14.0f, cardA.y + 78.0f }, 13, LIGHTGRAY);
    UI_DrawText("- Riesgo de emboscada segun", (Vector2){ cardA.x + 14.0f, cardA.y + 98.0f }, 13, LIGHTGRAY);
    UI_DrawText("  cobertura y evaluacion", (Vector2){ cardA.x + 14.0f, cardA.y + 116.0f }, 13, LIGHTGRAY);
    if (daysSupply < 2.0f) {
        UI_DrawText("! ATENCION: Reservas muy bajas", (Vector2){ cardA.x + 14.0f, cardA.y + 144.0f }, 13, RED);
    }

    /* Tarjeta B: Marcha Nocturna */
    Rectangle cardB = { (float)(mx + (hasInn ? 32 : 40) + cardW), (float)cardY, (float)cardW, (float)cardH };
    UI_DrawPanelStyled(cardB, UI_STYLE_PANEL_DARK);
    UI_DrawText("OPCION B: MARCHA NOCTURNA", (Vector2){ cardB.x + 14.0f, cardB.y + 12.0f }, 15, (Color){ 230, 110, 100, 255 });
    UI_DrawText("+ Continua el avance de noche", (Vector2){ cardB.x + 14.0f, cardB.y + 38.0f }, 13, RAYWHITE);
    UI_DrawText("- Coste movimiento x1.5 PA", (Vector2){ cardB.x + 14.0f, cardB.y + 58.0f }, 13, LIGHTGRAY);
    UI_DrawText("- Desvelo: -2 Moral y -2 Salud/turno", (Vector2){ cardB.x + 14.0f, cardB.y + 78.0f }, 13, LIGHTGRAY);
    UI_DrawText("- Peligro de encuentros hostiles (35%)", (Vector2){ cardB.x + 14.0f, cardB.y + 98.0f }, 13, LIGHTGRAY);
    if (army->consecutiveNightMarches > 0) {
        UI_DrawText(TextFormat("! Marchas consecutivas: %d (+%.0f%%)", 
            army->consecutiveNightMarches, army->consecutiveNightMarches * 50.0f), 
            (Vector2){ cardB.x + 14.0f, cardB.y + 124.0f }, 13, ORANGE);
    }

    /* Tarjeta C: Posada (Solo si está en asentamiento con meson) */
    Rectangle cardC = { (float)(mx + 48 + cardW * 2), (float)cardY, (float)cardW, (float)cardH };
    if (hasInn) {
        UI_DrawPanelStyled(cardC, UI_STYLE_PANEL_DARK);
        UI_DrawText("OPCION C: POSADA", (Vector2){ cardC.x + 14.0f, cardC.y + 12.0f }, 15, UI_COLOR_GOLD_LIGHT);
        UI_DrawText("+ Duerme en meson protegido", (Vector2){ cardC.x + 14.0f, cardC.y + 38.0f }, 13, (Color){ 230, 215, 160, 255 });
        UI_DrawText("+ Recupera +15 Moral y +10 Salud", (Vector2){ cardC.x + 14.0f, cardC.y + 58.0f }, 13, (Color){ 230, 215, 160, 255 });
        UI_DrawText("+ Tropa alimentada (Racion compl.)", (Vector2){ cardC.x + 14.0f, cardC.y + 78.0f }, 13, (Color){ 230, 215, 160, 255 });
        UI_DrawText("+ Inmune a ataques y emboscadas", (Vector2){ cardC.x + 14.0f, cardC.y + 98.0f }, 13, (Color){ 230, 215, 160, 255 });
        const char* innCostStr = (innCost == 0) ? "Gratis (Aliados)" : TextFormat("Coste: %d Oro", innCost);
        UI_DrawText(innCostStr, (Vector2){ cardC.x + 14.0f, cardC.y + 122.0f }, 13, (innCost == 0) ? GREEN : ((army->monedas >= innCost) ? UI_COLOR_GOLD : RED));
    }

    /* Botones de Accion */
    Rectangle btnA = { (float)(mx + (hasInn ? 16 : 24)), (float)(my + mh - 70), (float)cardW, 36.0f };
    Rectangle btnB = { (float)(mx + (hasInn ? 32 : 40) + cardW), (float)(my + mh - 70), (float)cardW, 36.0f };
    Rectangle btnC = { (float)(mx + 48 + cardW * 2), (float)(my + mh - 70), (float)cardW, 36.0f };

    bool hoverA = CheckCollisionPointRec(mouseCamp, btnA) || CheckCollisionPointRec(mouseCamp, cardA);
    bool hoverB = CheckCollisionPointRec(mouseCamp, btnB) || CheckCollisionPointRec(mouseCamp, cardB);

    if (hoverA) UI_RequestGlossaryTooltip("ACAMPAR", mouseCamp);
    if (hoverB) UI_RequestGlossaryTooltip("MARCHA_NOCTURNA", mouseCamp);

    UI_DrawButtonStyledSfx(btnA, "[ ACAMPAR (C) ]", UI_STYLE_BTN_PRIMARY, false, "");
    UI_DrawButtonStyledSfx(btnB, "[ MARCHA NOCT. (M) ]", UI_STYLE_BTN_DANGER, false, "");

    if (hasInn) {
        bool canAffordInn = (army->monedas >= innCost);
        const char* txtC = !canAffordInn ? TextFormat("[ FALTA ORO (%d) ]", innCost) : (innCost == 0 ? "[ POSADA GRATIS (P) ]" : TextFormat("[ POSADA (%d ORO) (P) ]", innCost));
        UI_DrawButtonStyledSfx(btnC, txtC, UI_STYLE_BTN_DEFAULT, !canAffordInn, "");
    }

    /* Pista de primer uso descartable (Sec B3) */
    static bool s_campHintDismissed = false;
    if (!s_campHintDismissed) {
        Rectangle hintBox = { (float)(mx + 24), (float)(my + mh - 28), (float)(mw - 48), 22.0f };
        bool hintHover = CheckCollisionPointRec(mouseCamp, hintBox);
        UI_DrawStatusChip(hintBox, "[PISTA] Acampar en bosque dificulta que los cazadores te encuentren. (Clic para cerrar)", UI_STYLE_CHIP_OK, UI_ICON_OK);
        if (hintHover && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            s_campHintDismissed = true;
        }
    }
}

bool Hud_HandleCampDecisionInput(GameMaster* gm, Map* map, Army* army, int screenWidth, int screenHeight)
{
    if (!gm || !gm->showCampDecision || !army || !map) return false;

    gm->campDecisionTimer += GetFrameTime();
    /* Debounce: no aceptar input en los primeros 350ms para garantizar que la tecla que abrió el atardecer no se consuma aquí */
    if (gm->campDecisionTimer < 0.35f) return false;

    HexTile* tile = Map_GetTile(map, army->position);
    const BalanceConfig* cfg = Balance_GetGlobal();
    bool hasInn = false;
    int innCost = 10;
    if (tile && tile->structure.type != STRUCT_NONE && !tile->structure.isBesieged) {
        if (tile->structure.type == STRUCT_TOWN_HOUSES ||
            tile->structure.type == STRUCT_TOWN_MARKET ||
            tile->structure.type == STRUCT_TOWN_CASTLE) {
            int fId = tile->structure.factionId;
            bool isHostile = (fId >= 0 && fId < 3 && cfg && gm->factions[fId].opinion <= cfg->factionsThresholdHostile);
            if (!isHostile) {
                hasInn = true;
                innCost = (cfg && cfg->servicesInnRestCost > 0) ? cfg->servicesInnRestCost : 10;
                if (fId >= 0 && fId < 3 && cfg && gm->factions[fId].opinion >= cfg->factionsThresholdAllied) {
                    innCost = 0;
                }
            }
        }
    }

    int mw = hasInn ? 840 : 640;
    int mh = 470;
    int mx = (screenWidth - mw) / 2;
    int my = (screenHeight - mh) / 2;

    int cardW = hasInn ? ((mw - 64) / 3) : ((mw - 64) / 2);
    Rectangle btnA = { (float)(mx + (hasInn ? 16 : 24)), (float)(my + mh - 70), (float)cardW, 36.0f };
    Rectangle btnB = { (float)(mx + (hasInn ? 32 : 40) + cardW), (float)(my + mh - 70), (float)cardW, 36.0f };
    Rectangle btnC = { (float)(mx + 48 + cardW * 2), (float)(my + mh - 70), (float)cardW, 36.0f };

    bool clickA = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), btnA);
    bool clickB = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), btnB);
    bool clickC = hasInn && (army->monedas >= innCost) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), btnC);

    bool keyCamp = IsKeyPressed(KEY_C);
    bool keyMarch = IsKeyPressed(KEY_M);
    bool keyInn = hasInn && (army->monedas >= innCost) && IsKeyPressed(KEY_P);

    if (clickA || keyCamp) {
        gm->campDecisionTimer = 0.0f;
        Audio_PlaySfx(SFX_CAMP_REST);
        GameMaster_ChooseCamp(gm, map, army);
        Map_UpdateVision(map, army->position, Army_GetVision(army) + GameMaster_GetVisionModifier(gm, army));
        return true;
    }
    if (clickB || keyMarch) {
        gm->campDecisionTimer = 0.0f;
        Audio_PlaySfx(SFX_NIGHT_MARCH);
        GameMaster_ChooseNightMarch(gm, map, army);
        Map_UpdateVision(map, army->position, Army_GetVision(army) + GameMaster_GetVisionModifier(gm, army));
        return true;
    }
    if (clickC || keyInn) {
        gm->campDecisionTimer = 0.0f;
        Audio_PlaySfxByName("inn_rest");
        GameMaster_ChooseInn(gm, map, army);
        Map_UpdateVision(map, army->position, Army_GetVision(army) + GameMaster_GetVisionModifier(gm, army));
        return true;
    }

    return false;
}

void Hud_DrawOfficersPanel(const Army* army, const GameMaster* gm, HudState* hud, int screenWidth, int screenHeight)
{
    if (!army || !hud || !hud->showOfficersPanel) return;

    (void)gm;
    int mw = 760;
    int mh = 540;
    int mx = (screenWidth - mw) / 2;
    int my = (screenHeight - mh) / 2;

    UI_DrawModalWindow((Rectangle){ (float)mx, (float)my, (float)mw, (float)mh }, "ESTADO MAYOR Y OFICIALES DE LA LEGION", "Presiona [O] o [ESC] para cerrar", UI_STYLE_PANEL_DARK);

    int curY = my + 62;

    /* 1. Comandante Supremo actual (General o Teniente Líder) */
    bool isGeneralLeader = (army->commander.type == OFFICER_GENERAL);
    const char* leaderTitle = isGeneralLeader ? "COMANDANTE EN JEFE (GENERAL SUPREMO)" : "COMANDANTE EN JEFE (TENIENTE AL MANDO - MOTIN)";
    Color leaderTitleCol = isGeneralLeader ? GOLD : RED;
    UI_DrawText(leaderTitle, (Vector2){ (float)(mx + 28), (float)curY }, 15, leaderTitleCol);
    curY += 22;

    /* Caja de comandante en jefe */
    Rectangle cmdBox = { (float)(mx + 24), (float)curY, (float)(mw - 48), 94.0f };
    UI_DrawPanelStyled(cmdBox, UI_STYLE_PANEL_DARK);

    /* Retrato */
    UI_DrawSlot((Rectangle){ cmdBox.x + 10.0f, cmdBox.y + 10.0f, 74.0f, 74.0f }, (Color){0, 0, 0, 0}, UI_ICON_NONE);
    UI_DrawText("[Sprite]", (Vector2){ cmdBox.x + 20.0f, cmdBox.y + 40.0f }, 12, DARKGRAY);

    /* Datos */
    int tx = (int)cmdBox.x + 96;
    int ty = (int)cmdBox.y + 10;
    const char* cmdName = Commander_GetName(army->commander);
    UI_DrawText(cmdName, (Vector2){ (float)tx, (float)ty }, 18, WHITE);
    
    if (isGeneralLeader) {
        int exCount = 0;
        for (int c = 0; c < army->general.cardCount; c++) {
            if (army->general.cards[c].isExhausted) exCount++;
        }
        UI_DrawText(TextFormat("Edad: %d | Rol: Comandante General", army->general.age), (Vector2){ (float)tx, (float)(ty + 22) }, 13, LIGHTGRAY);
        UI_DrawText(TextFormat("Vis: %d | Lid: %d | Eval: %d | Plan: %d | Ejec: %d",
                            army->general.vision, army->general.leadership, army->general.evaluation,
                            army->general.planning, army->general.execution), (Vector2){ (float)tx, (float)(ty + 42) }, 13, (Color){ 200, 200, 220, 255 });
        if (exCount > 0) {
            UI_DrawText(TextFormat("Cartas Divinas: %d / 3 (%d Agotada%s)", army->general.cardCount, exCount, exCount > 1 ? "s" : ""),
                        (Vector2){ (float)tx, (float)(ty + 62) }, 13, (Color){ 235, 165, 130, 255 });
        } else {
            UI_DrawText(TextFormat("Cartas Divinas Activas: %d / 3", army->general.cardCount), (Vector2){ (float)tx, (float)(ty + 62) }, 13, GOLD);
        }
    } else {
        Lieutenant* ltLeader = (Lieutenant*)army->commander.ptr;
        const char* roleName = ltLeader ? OfficerRole_GetName(ltLeader->role) : "Comandante";
        UI_DrawText(TextFormat("Edad: %d | Rol: %s | Dios: %s", ltLeader ? ltLeader->age : 30, roleName, ltLeader ? ltLeader->patronGod : "Ninguno"), (Vector2){ (float)tx, (float)(ty + 22) }, 13, LIGHTGRAY);
        UI_DrawText(TextFormat("Vis: %d | Lid: %d | Eval: %d | Plan: %d | Ejec: %d",
                            ltLeader ? ltLeader->vision : 0, ltLeader ? ltLeader->leadership : 0, ltLeader ? ltLeader->evaluation : 0,
                            ltLeader ? ltLeader->planning : 0, ltLeader ? ltLeader->execution : 0), (Vector2){ (float)tx, (float)(ty + 42) }, 13, (Color){ 200, 200, 220, 255 });
        int loy = ltLeader ? ltLeader->loyalty : 50;
        UI_DrawText(TextFormat("Lealtad: %d/100 | Carta Patrona: %s", loy, (ltLeader && strlen(ltLeader->patronCard.id) > 0) ? ltLeader->patronCard.id : "Activa"), (Vector2){ (float)tx, (float)(ty + 62) }, 13, RED);
    }

    /* Ranuras de cartas divinas del Comandante y Chip de Discordia Divina en el panel de Oficiales */
    const ModifierDef* hoverOffCard = NULL;
    bool hoverOffEx = false;
    int hoverOffBattles = 0;
    {
        int slots = isGeneralLeader ? MAX_GENERAL_DIVINE_CARDS : MAX_LIEUTENANT_DIVINE_CARDS;
        int cCount = Commander_GetCardCount(army->commander);
        int cW = 42, cH = 56;
        int cX = (int)(cmdBox.x + cmdBox.width) - (slots * (cW + 8)) - 14;
        int cY = (int)cmdBox.y + 10;
        for (int i = 0; i < slots; i++) {
            bool hasC = (i < cCount);
            const char* cId = hasC ? Commander_GetCard(army->commander, i) : "";
            ModifierInstance* inst = hasC ? Commander_GetCardInstance(army->commander, i) : NULL;
            Hud_DrawDivineCardSlot(cX + i * (cW + 8), cY, cW, cH, hasC, cId,
                                   inst ? inst->isExhausted : false,
                                   inst ? inst->battlesUntilRecharge : 0,
                                   gm, &hoverOffCard, &hoverOffEx, &hoverOffBattles);
        }

        char offConflict[64] = {0};
        if (gm && Modifier_CheckRivals(&gm->modRegistry, army, offConflict, sizeof(offConflict))) {
            const char* pairStr = (strncmp(offConflict, "Disputa: ", 9) == 0) ? (offConflict + 9) : offConflict;
            Rectangle rDiscord = { cmdBox.x + cmdBox.width - 310.0f, cmdBox.y + 68.0f, 296.0f, 22.0f };
            UI_DrawStatusChip(rDiscord, TextFormat("[DISCORDIA DIVINA: %s]", pairStr), UI_STYLE_CHIP_BAD, UI_ICON_PELIGRO);
            Vector2 swPos = { rDiscord.x + rDiscord.width - 12.0f, rDiscord.y + 11.0f };
            DrawLineEx((Vector2){ swPos.x - 4.5f, swPos.y + 4.5f }, (Vector2){ swPos.x + 4.5f, swPos.y - 4.5f }, 1.8f, (Color){ 255, 190, 140, 255 });
            DrawLineEx((Vector2){ swPos.x - 4.5f, swPos.y - 4.5f }, (Vector2){ swPos.x + 4.5f, swPos.y + 4.5f }, 1.8f, (Color){ 255, 190, 140, 255 });
            if (CheckCollisionPointRec(GetMousePosition(), rDiscord)) {
                UI_RequestTooltip("DISCORDIA DIVINA",
                                  "Los dioses rivales disputan en el campamento. Penalizacion de -1 a todas las tiradas de planificacion tactica.",
                                  GetMousePosition());
            }
        }
    }

    curY += 106;

    /* 2. Lista de Oficiales Tenientes Subordinados */
    UI_DrawText("TENIENTES SUBORDINADOS Y ASESORES:", (Vector2){ (float)(mx + 28), (float)curY }, 15, (Color){ 180, 150, 80, 255 });
    curY += 22;

    int ltBoxH = 78;
    for (int i = 0; i < army->lieutenantCount && i < 3; i++) {
        const Lieutenant* lt = &army->lieutenants[i];
        bool isCurrentCmd = (!isGeneralLeader && (Lieutenant*)army->commander.ptr == lt);

        Rectangle ltBox = { (float)(mx + 24), (float)curY, (float)(mw - 48), (float)ltBoxH };
        UI_DrawPanelStyled(ltBox, UI_STYLE_PANEL_DARK);

        /* Retrato chico */
        UI_DrawSlot((Rectangle){ ltBox.x + 8.0f, ltBox.y + 8.0f, 62.0f, 62.0f }, (Color){0, 0, 0, 0}, UI_ICON_NONE);
        UI_DrawText("[Ofic]", (Vector2){ ltBox.x + 18.0f, ltBox.y + 32.0f }, 11, GRAY);

        int lx = (int)ltBox.x + 80;
        int ly = (int)ltBox.y + 8;
        const char* rName = OfficerRole_GetName(lt->role);
        UI_DrawText(TextFormat("%s  [%s]", lt->name, rName), (Vector2){ (float)lx, (float)ly }, 15, WHITE);

        /* Dios patrono y stats */
        UI_DrawText(TextFormat("Dios Patrono: %s  |  Carta: %s", lt->patronGod, (strlen(lt->patronCard.id) > 0) ? lt->patronCard.id : "Inactiva"), (Vector2){ (float)lx, (float)(ly + 22) }, 12, (Color){ 170, 150, 220, 255 });
        UI_DrawText(TextFormat("Stats: V:%d L:%d E:%d P:%d Ej:%d", lt->vision, lt->leadership, lt->evaluation, lt->planning, lt->execution), (Vector2){ (float)lx, (float)(ly + 40) }, 12, LIGHTGRAY);

        /* Barra de lealtad 9-slice */
        int barW = 160;
        int barH = 18;
        int barX = (int)ltBox.x + (int)ltBox.width - barW - 18;
        int barY = ly + 22;

        Color loyColor = (lt->loyalty < 35) ? RED : ((lt->loyalty < 65) ? ORANGE : GREEN);
        UI_DrawLabeledBar((Rectangle){ (float)barX, (float)barY, (float)barW, (float)barH }, (float)lt->loyalty / 100.0f, TextFormat("Lealtad: %d%%", lt->loyalty), loyColor, UI_ICON_SATISFACCION);

        /* Si es el comandante rebelde */
        if (isCurrentCmd) {
            UI_DrawText("[AL MANDO]", (Vector2){ (float)(barX - 86), (float)barY }, 12, RED);
        }

        curY += ltBoxH + 8;
    }

    /* Estado de General depuesto (si aplica) */
    if (army->hasDeposedGeneral) {
        UI_DrawStatusChip((Rectangle){ (float)(mx + 24), (float)(my + mh - 68), (float)(mw - 48), 28.0f }, TextFormat("General depuesto: %s (Reevaluable si Moral >= 60)", army->deposedGeneral.name), UI_STYLE_CHIP_BAD, UI_ICON_PELIGRO);
    }

    /* Botón de cierre */
    Rectangle closeBtn = { (float)(mx + mw / 2 - 80), (float)(my + mh - 36), 160.0f, 28.0f };
    UI_DrawButtonStyledSfx(closeBtn, "Cerrar (O / ESC)", UI_STYLE_BTN_DEFAULT, false, "");

    if (hoverOffCard) {
        Rectangle previewRect = { (float)(mx + mw - 260), (float)(my + 80), 240.0f, 360.0f };
        UI_DrawModifierCardEx(hoverOffCard, previewRect, hoverOffEx, hoverOffBattles);
    }
}

bool Hud_HandleOfficersPanelInput(HudState* hud, int screenWidth, int screenHeight)
{
    if (!hud || !hud->showOfficersPanel) return false;

    int mw = 760;
    int mh = 540;
    int mx = (screenWidth - mw) / 2;
    int my = (screenHeight - mh) / 2;
    Rectangle closeBtn = { (float)(mx + mw / 2 - 80), (float)(my + mh - 36), 160.0f, 28.0f };

    bool clickedClose = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), closeBtn);
    bool keyClose = IsKeyPressed(KEY_O) || IsKeyPressed(KEY_ESCAPE);

    if (clickedClose || keyClose) {
        Audio_PlaySfx(SFX_UI_CLICK);
        hud->showOfficersPanel = false;
        return true;
    }

    return false;
}

void Hud_DrawGroupsPanel(const Army* army, const GameMaster* gm, HudState* hud, int screenWidth, int screenHeight)
{
    if (!army || !hud || !hud->showGroupsPanel) return;

    (void)gm;
    int mw = 720;
    int mh = 480;
    int mx = (screenWidth - mw) / 2;
    int my = (screenHeight - mh) / 2;

    UI_DrawModalWindow((Rectangle){ (float)mx, (float)my, (float)mw, (float)mh }, "ESTADO Y MORAL DE LOS GRUPOS DEL EJERCITO", "Presiona [G] o [ESC] para cerrar", UI_STYLE_PANEL_DARK);

    float cardW = (mw - 48.0f - 30.0f) / 4.0f;
    float cardH = mh - 130.0f;
    float cardY = (float)(my + 66);

    const char* gNames[RG_COUNT] = { "Iniciados", "Soldados", "Sargentos", "Ganado" };

    for (int g = 0; g < RG_COUNT; g++) {
        Rectangle card = { (float)(mx + 24) + (float)g * (cardW + 10.0f), cardY, cardW, cardH };
        UI_DrawPanelStyled(card, UI_STYLE_PANEL_DARK);

        int troopCount = (g == RG_INICIADOS) ? army->iniciados :
                        ((g == RG_SOLDADOS)  ? army->soldados :
                        ((g == RG_SARGENTOS) ? army->sargentos : army->ganado));

        UI_DrawText(gNames[g], (Vector2){ card.x + 12.0f, card.y + 10.0f }, 15, UI_COLOR_GOLD);
        UI_DrawText(TextFormat("Tropas: %d", troopCount), (Vector2){ card.x + 12.0f, card.y + 28.0f }, 12, UI_COLOR_TEXT_DIM);
        DrawLine((int)(card.x + 8), (int)(card.y + 46), (int)(card.x + cardW - 8), (int)(card.y + 46), Fade(UI_COLOR_PANEL_BORDER, 0.5f));

        GroupState gs = Ration_GetGroupState(army, (RationGroup)g);
        float py = card.y + 54.0f;

        if (g != RG_GANADO) {
            UI_DrawLabeledBar((Rectangle){ card.x + 10.0f, py, card.width - 20.0f, 18.0f }, gs.morale / 100.0f, TextFormat("Moral: %.0f%%", gs.morale),
                              (gs.morale < 35.0f) ? UI_COLOR_BAD : ((gs.morale < 65.0f) ? UI_COLOR_WARNING : UI_COLOR_GOOD), UI_ICON_MORAL);
            py += 26.0f;
        }

        UI_DrawLabeledBar((Rectangle){ card.x + 10.0f, py, card.width - 20.0f, 18.0f }, gs.health / 100.0f, TextFormat("Salud: %.0f%%", gs.health),
                          (gs.health < 40.0f) ? UI_COLOR_BAD : UI_COLOR_GOOD, UI_ICON_SALUD);
        py += 26.0f;

        if (g == RG_SARGENTOS) {
            UI_DrawLabeledBar((Rectangle){ card.x + 10.0f, py, card.width - 20.0f, 18.0f }, gs.satisfaction / gs.satisfactionMax, TextFormat("Sat: %.0f/%.0f", gs.satisfaction, gs.satisfactionMax),
                              (Color){ 245, 195, 65, 255 }, UI_ICON_SATISFACCION);
            py += 26.0f;
        }

        if (gs.daysUnfed > 0) {
            UI_DrawStatusChip((Rectangle){ card.x + 8.0f, py, card.width - 16.0f, 28.0f }, TextFormat("Hambruna (%dd)", gs.daysUnfed), UI_STYLE_CHIP_BAD, UI_ICON_PELIGRO);
            py += 34.0f;
        } else {
            UI_DrawStatusChip((Rectangle){ card.x + 8.0f, py, card.width - 16.0f, 26.0f }, "Bien alimentados", UI_STYLE_CHIP_OK, UI_ICON_OK);
            py += 32.0f;
        }

        if (g == RG_GANADO) {
            UI_DrawText("Animales de tiro y carga.", (Vector2){ card.x + 10.0f, py }, 10, UI_COLOR_TEXT_DIM);
            UI_DrawText("Pastan gratis en llanuras.", (Vector2){ card.x + 10.0f, py + 14.0f }, 10, UI_COLOR_TEXT_DIM);
        }
    }

    Rectangle closeBtn = { (float)(mx + mw / 2 - 80), (float)(my + mh - 36), 160.0f, 28.0f };
    UI_DrawButtonStyledSfx(closeBtn, "Cerrar (G / ESC)", UI_STYLE_BTN_DEFAULT, false, "");
}

bool Hud_HandleGroupsPanelInput(HudState* hud, int screenWidth, int screenHeight)
{
    if (!hud || !hud->showGroupsPanel) return false;

    int mw = 720;
    int mh = 480;
    int mx = (screenWidth - mw) / 2;
    int my = (screenHeight - mh) / 2;
    Rectangle closeBtn = { (float)(mx + mw / 2 - 80), (float)(my + mh - 36), 160.0f, 28.0f };

    bool clickedClose = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), closeBtn);
    bool keyClose = IsKeyPressed(KEY_G) || IsKeyPressed(KEY_ESCAPE);

    if (clickedClose || keyClose) {
        Audio_PlaySfx(SFX_UI_CLICK);
        hud->showGroupsPanel = false;
        return true;
    }

    return false;
}







