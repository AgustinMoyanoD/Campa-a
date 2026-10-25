#include "views/city_view.h"
#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"
#include "presentation/audio_manager.h"
#include "raylib.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

/* ============================================================
   CACHE DE FONDOS AMBIENTALES DE ESTRUCTURAS (assets/structuresBG/)
   ============================================================ */

typedef struct {
    char name[32];
    Texture2D tex;
    bool loaded;
} StructureBgEntry;

static StructureBgEntry s_bgCache[10] = {0};
static int s_bgCount = 0;

static Texture2D GetStructureBg(const char* name) {
    if (!name || name[0] == '\0') return (Texture2D){0};

    for (int i = 0; i < s_bgCount; i++) {
        if (strcmp(s_bgCache[i].name, name) == 0) {
            return s_bgCache[i].tex;
        }
    }

    if (s_bgCount < 10) {
        char path[256];
        snprintf(path, sizeof(path), "assets/structuresBG/%sBG.png", name);
        if (!FileExists(path)) {
            snprintf(path, sizeof(path), "%sassets/structuresBG/%sBG.png", GetApplicationDirectory(), name);
        }
        if (FileExists(path)) {
            Texture2D tex = LoadTexture(path);
            SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
            s_bgCache[s_bgCount].tex = tex;
            strncpy(s_bgCache[s_bgCount].name, name, sizeof(s_bgCache[s_bgCount].name) - 1);
            s_bgCache[s_bgCount].loaded = true;
            return s_bgCache[s_bgCount++].tex;
        }
    }
    return (Texture2D){0};
}

void CityView_UnloadAssets(void) {
    for (int i = 0; i < s_bgCount; i++) {
        if (s_bgCache[i].loaded) {
            UnloadTexture(s_bgCache[i].tex);
            s_bgCache[i].loaded = false;
        }
    }
    s_bgCount = 0;
}

/* ============================================================
   WIDGETS INTERNOS DE APOYO VISUAL (CENTRALIZADOS EN UI_WIDGETS)
   ============================================================ */

/* Dibuja un botón de pestaña 9-slice con estado activo/inactivo destacado */
static bool DrawTabButton(Rectangle bounds, const char* label, bool isActive) {
    UIButtonSpec spec = {
        .style = UI_STYLE_BTN_DEFAULT,
        .fontSize = 15.0f,
        .disabled = false,
        .selected = isActive,
        .sfxOnClick = "ui_click"
    };
    return UI_DrawButtonSpec(bounds, label, &spec);
}

/* Píldora informativa de recurso con 9-slice UI_SKIN_CHIP e icono */
static void DrawResourcePill(Rectangle rect, const char* label, const char* val, Color pillBorder, Color textValCol) {
    (void)pillBorder;
    (void)textValCol;
    UIIconId icon = UI_ICON_NONE;
    if (strcmp(label, "Comida") == 0) icon = UI_ICON_COMIDA;
    else if (strcmp(label, "Agua") == 0) icon = UI_ICON_AGUA;
    else if (strcmp(label, "Vino") == 0) icon = UI_ICON_VINO;
    else if (strcmp(label, "Carne") == 0) icon = UI_ICON_CARNE;
    else if (strcmp(label, "Ganado") == 0) icon = UI_ICON_GANADO;
    else if (strcmp(label, "Salud") == 0) icon = UI_ICON_SALUD;
    else if (strcmp(label, "Moral") == 0) icon = UI_ICON_MORAL;

    char buf[64];
    if (icon != UI_ICON_NONE) {
        snprintf(buf, sizeof(buf), "%s", val);
    } else {
        snprintf(buf, sizeof(buf), "%s: %s", label, val);
    }
    UI_DrawStatusChip(rect, buf, UI_STYLE_CHIP_OK, icon);
}

/* Barra de progreso 9-slice con porcentaje e icono */
static void DrawStatBar(Rectangle rect, float current, float maxVal, const char* title, Color barCol) {
    float pct = (maxVal > 0.0f) ? (current / maxVal) : 0.0f;
    UIIconId icon = (strstr(title, "Salud") != NULL) ? UI_ICON_SALUD : UI_ICON_MORAL;
    char buf[64];
    snprintf(buf, sizeof(buf), "%s: %.1f / %.0f", title, current, maxVal);
    UI_DrawLabeledBar(rect, pct, buf, barCol, icon);
}

/* ============================================================
   FUNCIÓN PRINCIPAL: CityView_UpdateDraw
   ============================================================ */

bool CityView_UpdateDraw(CityContext* ctx, Army* army, GameMaster* gm, Map* map, int SW, int SH) {
    if (!ctx || !army || !gm) return true;

    // Telón de fondo oscurecido
    DrawRectangle(0, 0, SW, SH, Fade(BLACK, 0.82f));

    // Dimensiones responsivas del panel central
    int pw = (SW < 1040) ? (SW - 40) : 1000;
    int ph = (SH < 700) ? (SH - 40) : 660;
    int px = (SW - pw) / 2;
    int py = (SH - ph) / 2;

    // Fondo 9-slice oscuro con marco biselado
    UI_DrawPanelStyled((Rectangle){ (float)px, (float)py, (float)pw, (float)ph }, UI_STYLE_PANEL_DARK);

    // Determinar nombre del fondo ambiental según pestaña activa o estructura
    const char* bgName = "castle";
    switch (ctx->currentTab) {
        case CITY_TAB_MARKET:   bgName = "market"; break;
        case CITY_TAB_ARMORY:   bgName = "armory"; break;
        case CITY_TAB_HOSPITAL: bgName = "houses"; break;
        case CITY_TAB_TEMPLE:   bgName = "altar"; break;
        case CITY_TAB_INN:      bgName = "houses"; break;
        case CITY_TAB_MAIN:
        default:
            switch (ctx->structType) {
                case STRUCT_ALTAR:       bgName = "altar"; break;
                case STRUCT_BRIDGE:      bgName = "bridge"; break;
                case STRUCT_WATCHTOWER:  bgName = "watchtower"; break;
                case STRUCT_TOWN_CASTLE: bgName = "castle"; break;
                case STRUCT_TOWN_MARKET: bgName = "market"; break;
                case STRUCT_TOWN_ARMORY: bgName = "armory"; break;
                case STRUCT_TOWN_HOUSES: bgName = "houses"; break;
                default:                 bgName = "castle"; break;
            }
            break;
    }

    // Dibujar imagen de fondo ambiental si existe en assets/structuresBG/
    Texture2D bgTex = GetStructureBg(bgName);
    if (bgTex.id > 0) {
        Rectangle srcRec = { 0.0f, 0.0f, (float)bgTex.width, (float)bgTex.height };
        Rectangle destRec = { (float)(px + 10), (float)(py + 10), (float)(pw - 20), (float)(ph - 20) };
        DrawTexturePro(bgTex, srcRec, destRec, (Vector2){0, 0}, 0.0f, Fade(WHITE, 0.22f));
    }

    // Cabecera: Título de la estructura
    UI_DrawText(ctx->name, (Vector2){ (float)(px + 30), (float)(py + 16) }, 28.0f, UI_COLOR_TEXT_TITLE);

    // Cabecera: Información de Facción y Opinión
    FactionOpinionLevel opLvl = Faction_GetLevel(ctx->factionOpinion, Balance_GetGlobal());
    const char* opName = Faction_GetLevelName(opLvl);
    const char* alertStr = (ctx->factionAlertLevel == 0) ? "Calma" : ((ctx->factionAlertLevel == 1) ? "Alerta Guardia" : "Bajo Asedio");
    Color facCol = (opLvl == FACTION_HOSTILE) ? RED : ((opLvl == FACTION_ALLIED) ? GREEN : ((opLvl == FACTION_FRIENDLY) ? SKYBLUE : ((opLvl == FACTION_SUSPICIOUS) ? ORANGE : RAYWHITE)));

    char facBuf[128];
    snprintf(facBuf, sizeof(facBuf), "%s | %s (%+d) | %s", ctx->factionName, opName, ctx->factionOpinion, alertStr);
    
    // Chip de facción en la esquina superior derecha
    int fChipW = 320;
    DrawRectangle(px + pw - fChipW - 30, py + 18, fChipW, 26, (Color){ 24, 28, 40, 230 });
    DrawRectangleLinesEx((Rectangle){ (float)(px + pw - fChipW - 30), (float)(py + 18), (float)fChipW, 26.0f }, 1.0f, (Color){ 100, 85, 60, 200 });
    Vector2 fsz = UI_MeasureText(facBuf, 14.0f);
    UI_DrawText(facBuf, (Vector2){ (float)(px + pw - 30 - fChipW + (fChipW - fsz.x)/2.0f), (float)(py + 23) }, 14.0f, facCol);

    // Barra de Recursos limpia y sin sobrecarga de texto
    int totalSoldiers = Army_GetTotalSoldiers(army);
    float dailyF = Army_CalculateFoodUsage(totalSoldiers, army->ganado);
    if (dailyF < 0.1f) dailyF = 1.0f;
    float foodDays = army->comida_disponible / dailyF;

    float dailyW = Army_CalculateWaterUsage(totalSoldiers, army->ganado);
    if (dailyW < 0.1f) dailyW = 1.0f;
    float waterDays = army->agua_disponible / dailyW;

    int pillY = py + 52;
    int pillH = 26;
    int curPillX = px + 30;
    int pillGap = 6;
    int pW_small = 74;
    int pW_supply = 112;

    char numBuf[32];
    snprintf(numBuf, sizeof(numBuf), "%d", army->monedas);
    DrawResourcePill((Rectangle){ (float)curPillX, (float)pillY, (float)pW_small, (float)pillH }, "Oro", numBuf, (Color){ 180, 140, 50, 200 }, UI_COLOR_GOLD_LIGHT);
    Vector2 mouseCity = GetMousePosition();
    if (CheckCollisionPointRec(mouseCity, (Rectangle){ (float)curPillX, (float)pillY, (float)pW_small, (float)pillH })) UI_RequestGlossaryTooltip("ORO", mouseCity);
    curPillX += pW_small + pillGap;

    snprintf(numBuf, sizeof(numBuf), "%.0fd (%.0f)", foodDays, army->comida_disponible);
    DrawResourcePill((Rectangle){ (float)curPillX, (float)pillY, (float)pW_supply, (float)pillH }, "Comida", numBuf, (Color){ 100, 130, 60, 200 }, (Color){ 160, 210, 110, 255 });
    if (CheckCollisionPointRec(mouseCity, (Rectangle){ (float)curPillX, (float)pillY, (float)pW_supply, (float)pillH })) UI_RequestGlossaryTooltip("COMIDA", mouseCity);
    curPillX += pW_supply + pillGap;

    snprintf(numBuf, sizeof(numBuf), "%.0fd (%.0f)", waterDays, army->agua_disponible);
    DrawResourcePill((Rectangle){ (float)curPillX, (float)pillY, (float)pW_supply, (float)pillH }, "Agua", numBuf, (Color){ 60, 120, 180, 200 }, (Color){ 120, 200, 255, 255 });
    if (CheckCollisionPointRec(mouseCity, (Rectangle){ (float)curPillX, (float)pillY, (float)pW_supply, (float)pillH })) UI_RequestGlossaryTooltip("AGUA", mouseCity);
    curPillX += pW_supply + pillGap;

    snprintf(numBuf, sizeof(numBuf), "%.0f", army->vino_disponible);
    DrawResourcePill((Rectangle){ (float)curPillX, (float)pillY, (float)pW_small, (float)pillH }, "Vino", numBuf, (Color){ 130, 60, 140, 200 }, (Color){ 240, 180, 255, 255 });
    curPillX += pW_small + pillGap;

    snprintf(numBuf, sizeof(numBuf), "%.0f", army->carne_disponible);
    DrawResourcePill((Rectangle){ (float)curPillX, (float)pillY, (float)pW_small, (float)pillH }, "Carne", numBuf, (Color){ 160, 80, 60, 200 }, (Color){ 255, 190, 160, 255 });
    curPillX += pW_small + pillGap;

    snprintf(numBuf, sizeof(numBuf), "%d", army->ganado);
    DrawResourcePill((Rectangle){ (float)curPillX, (float)pillY, (float)pW_small, (float)pillH }, "Ganado", numBuf, (Color){ 150, 110, 70, 200 }, (Color){ 240, 190, 130, 255 });
    if (CheckCollisionPointRec(mouseCity, (Rectangle){ (float)curPillX, (float)pillY, (float)pW_small, (float)pillH })) UI_RequestGlossaryTooltip("GANADO", mouseCity);
    curPillX += pW_small + pillGap;

    snprintf(numBuf, sizeof(numBuf), "%.0f%%", army->salud);
    DrawResourcePill((Rectangle){ (float)curPillX, (float)pillY, (float)pW_small, (float)pillH }, "Salud", numBuf, (Color){ 170, 70, 80, 200 }, (army->salud < 50.0f) ? RED : (Color){ 240, 140, 150, 255 });
    if (CheckCollisionPointRec(mouseCity, (Rectangle){ (float)curPillX, (float)pillY, (float)pW_small, (float)pillH })) UI_RequestGlossaryTooltip("SALUD", mouseCity);
    curPillX += pW_small + pillGap;

    snprintf(numBuf, sizeof(numBuf), "%.0f%%", army->moral);
    DrawResourcePill((Rectangle){ (float)curPillX, (float)pillY, (float)pW_small, (float)pillH }, "Moral", numBuf, (Color){ 170, 140, 50, 200 }, (Color){ 250, 230, 140, 255 });
    if (CheckCollisionPointRec(mouseCity, (Rectangle){ (float)curPillX, (float)pillY, (float)pW_small, (float)pillH })) UI_RequestGlossaryTooltip("MORAL", mouseCity);
    curPillX += pW_small + pillGap;

    snprintf(numBuf, sizeof(numBuf), "%d (%dI/%dS/%dG)", totalSoldiers, army->iniciados, army->soldados, army->sargentos);
    DrawResourcePill((Rectangle){ (float)curPillX, (float)pillY, (float)(pw - 60 - (curPillX - px - 30)), (float)pillH }, "Tropas", numBuf, (Color){ 90, 95, 110, 200 }, (Color){ 245, 240, 230, 255 });

    // Barra de navegación de pestañas
    int tabY = py + 88;
    int tabW = 140;
    int tabH = 38;
    int curTabX = px + 30;

    if (DrawTabButton((Rectangle){ (float)curTabX, (float)tabY, (float)tabW, (float)tabH }, "1. Principal", ctx->currentTab == CITY_TAB_MAIN) || IsKeyPressed(KEY_ONE)) {
        ctx->currentTab = CITY_TAB_MAIN;
    }
    curTabX += tabW + 8;

    if (City_HasMarket(ctx)) {
        if (DrawTabButton((Rectangle){ (float)curTabX, (float)tabY, (float)tabW, (float)tabH }, "2. Mercado", ctx->currentTab == CITY_TAB_MARKET) || IsKeyPressed(KEY_TWO)) {
            ctx->currentTab = CITY_TAB_MARKET;
        }
        curTabX += tabW + 8;
    }

    if (City_HasArmory(ctx)) {
        if (DrawTabButton((Rectangle){ (float)curTabX, (float)tabY, (float)tabW, (float)tabH }, "3. Armeria", ctx->currentTab == CITY_TAB_ARMORY) || IsKeyPressed(KEY_THREE)) {
            ctx->currentTab = CITY_TAB_ARMORY;
        }
        curTabX += tabW + 8;
    }

    if (City_HasHospital(ctx)) {
        if (DrawTabButton((Rectangle){ (float)curTabX, (float)tabY, (float)tabW, (float)tabH }, "4. Hospital", ctx->currentTab == CITY_TAB_HOSPITAL) || IsKeyPressed(KEY_FOUR)) {
            ctx->currentTab = CITY_TAB_HOSPITAL;
        }
        curTabX += tabW + 8;
    }

    if (City_HasTemple(ctx)) {
        if (DrawTabButton((Rectangle){ (float)curTabX, (float)tabY, (float)tabW, (float)tabH }, "5. Templo", ctx->currentTab == CITY_TAB_TEMPLE) || IsKeyPressed(KEY_FIVE)) {
            ctx->currentTab = CITY_TAB_TEMPLE;
        }
        curTabX += tabW + 8;
    }

    if (City_HasInn(ctx)) {
        if (DrawTabButton((Rectangle){ (float)curTabX, (float)tabY, (float)tabW, (float)tabH }, "6. Posada", ctx->currentTab == CITY_TAB_INN) || IsKeyPressed(KEY_SIX)) {
            ctx->currentTab = CITY_TAB_INN;
        }
        curTabX += tabW + 8;
    }

    // Botón Salir / Cerrar
    if (UI_DrawButton((Rectangle){ (float)(px + pw - 125), (float)tabY, 95.0f, (float)tabH }, "Salir [ESC]", false) || IsKeyPressed(KEY_ESCAPE)) {
        GameMaster_CloseCity(gm, NULL, army);
        return true;
    }

    // Línea separadora ornamental
    DrawLine(px + 24, tabY + tabH + 6, px + pw - 24, tabY + tabH + 6, (Color){ 212, 166, 58, 200 });
    DrawLine(px + 24, tabY + tabH + 7, px + pw - 24, tabY + tabH + 7, (Color){ 120, 85, 45, 140 });

    // Área de Contenido
    int contentY = tabY + tabH + 18;
    int contentX = px + 35;
    int contentW = pw - 70;

    switch (ctx->currentTab) {
        case CITY_TAB_MAIN: {
            if (ctx->isBesieged) {
                DrawRectangle(contentX, contentY, contentW, 40, (Color){ 65, 45, 15, 235 });
                DrawRectangleLinesEx((Rectangle){ (float)contentX, (float)contentY, (float)contentW, 40.0f }, 1.5f, UI_COLOR_GOLD_LIGHT);
                UI_DrawText("[CIUDAD OCUPADA]: Guarnicion derrotada, asentamiento bajo tu control.", (Vector2){ (float)(contentX + 16), (float)(contentY + 11) }, 15.0f, UI_COLOR_TEXT_TITLE);
                contentY += 50;
            } else if (ctx->factionAlertLevel >= 1 || opLvl == FACTION_HOSTILE) {
                DrawRectangle(contentX, contentY, contentW, 40, (Color){ 65, 20, 25, 235 });
                DrawRectangleLinesEx((Rectangle){ (float)contentX, (float)contentY, (float)contentW, 40.0f }, 1.5f, RED);
                UI_DrawText("[PUERTAS CERRADAS]: La guarnicion rechaza el comercio debido a la hostilidad.", (Vector2){ (float)(contentX + 16), (float)(contentY + 11) }, 15.0f, (Color){ 255, 140, 140, 255 });
                contentY += 50;
            } else {
                UI_DrawText("Plaza Central", (Vector2){ (float)contentX, (float)contentY }, 22.0f, UI_COLOR_TEXT_TITLE);
                contentY += 36;
            }

            // Distritos disponibles en tarjetas interactivas compactas
            int cardW = (contentW - 20) / 2;
            int cardH = 54;
            int cX = contentX;
            int cY = contentY;

            if (City_HasMarket(ctx)) {
                Rectangle mRec = { (float)cX, (float)cY, (float)cardW, (float)cardH };
                UI_DrawPanelStyled(mRec, UI_STYLE_PANEL_DARK);
                UI_DrawText("Mercado de Provisiones", (Vector2){ mRec.x + 18, mRec.y + 16 }, 17.0f, UI_COLOR_GOLD_LIGHT);
                if (UI_DrawButton((Rectangle){ mRec.x + mRec.width - 110, mRec.y + 10, 95, 34 }, "Entrar", false)) {
                    ctx->currentTab = CITY_TAB_MARKET;
                }
                cX += cardW + 20;
            }

            if (City_HasArmory(ctx)) {
                Rectangle aRec = { (float)cX, (float)cY, (float)cardW, (float)cardH };
                UI_DrawPanelStyled(aRec, UI_STYLE_PANEL_DARK);
                UI_DrawText("Armeria y Cuartel", (Vector2){ aRec.x + 18, aRec.y + 16 }, 17.0f, UI_COLOR_GOLD_LIGHT);
                if (UI_DrawButton((Rectangle){ aRec.x + aRec.width - 110, aRec.y + 10, 95, 34 }, "Entrar", false)) {
                    ctx->currentTab = CITY_TAB_ARMORY;
                }
                cX = contentX;
                cY += cardH + 12;
            }

            if (City_HasHospital(ctx)) {
                Rectangle hRec = { (float)cX, (float)cY, (float)cardW, (float)cardH };
                UI_DrawPanelStyled(hRec, UI_STYLE_PANEL_DARK);
                UI_DrawText("Hospital de Campana", (Vector2){ hRec.x + 18, hRec.y + 16 }, 17.0f, UI_COLOR_GOLD_LIGHT);
                if (UI_DrawButton((Rectangle){ hRec.x + hRec.width - 110, hRec.y + 10, 95, 34 }, "Entrar", false)) {
                    ctx->currentTab = CITY_TAB_HOSPITAL;
                }
                cX += cardW + 20;
            }

            if (City_HasTemple(ctx)) {
                Rectangle tRec = { (float)cX, (float)cY, (float)cardW, (float)cardH };
                UI_DrawPanelStyled(tRec, UI_STYLE_PANEL_DARK);
                UI_DrawText("Templo y Altar", (Vector2){ tRec.x + 18, tRec.y + 16 }, 17.0f, UI_COLOR_GOLD_LIGHT);
                if (UI_DrawButton((Rectangle){ tRec.x + tRec.width - 110, tRec.y + 10, 95, 34 }, "Entrar", false)) {
                    ctx->currentTab = CITY_TAB_TEMPLE;
                }
                cX = contentX;
                cY += cardH + 12;
            }

            if (City_HasInn(ctx)) {
                Rectangle iRec = { (float)cX, (float)cY, (float)cardW, (float)cardH };
                UI_DrawPanelStyled(iRec, UI_STYLE_PANEL_DARK);
                UI_DrawText("Posada del Meson", (Vector2){ iRec.x + 18, iRec.y + 16 }, 17.0f, UI_COLOR_GOLD_LIGHT);
                if (UI_DrawButton((Rectangle){ iRec.x + iRec.width - 110, iRec.y + 10, 95, 34 }, "Entrar", false)) {
                    ctx->currentTab = CITY_TAB_INN;
                }
                cY += cardH + 12;
            }

            // Opcion de Asedio
            if (City_CanSiege(ctx)) {
                Rectangle sRec = { (float)contentX, (float)(py + ph - 80), (float)contentW, 52.0f };
                UI_DrawPanelStyled(sRec, UI_STYLE_PANEL_DARK);
                UI_DrawText("ASEDIAR ASENTAMIENTO", (Vector2){ sRec.x + 16, sRec.y + 16 }, 16.0f, UI_COLOR_GOLD_LIGHT);
                if (UI_DrawButtonStyledSfx((Rectangle){ sRec.x + sRec.width - 190, sRec.y + 9, 175, 34 }, "Proponer Asedio [A]", UI_STYLE_BTN_DANGER, false, NULL) || IsKeyPressed(KEY_A)) {
                    Audio_PlaySfxByName("siege_start");
                    ctx->requestSiege = true;
                    GameMaster_CloseCity(gm, NULL, army);
                    return true;
                }
            }
            break;
        }

        case CITY_TAB_MARKET: {
            UI_DrawText("Mercado de Provisiones", (Vector2){ (float)contentX, (float)contentY }, 22.0f, UI_COLOR_TEXT_TITLE);

            int rowY = contentY + 36;
            int rowH = 56;
            int rowSpacing = 62;

            int totalSoldiers = Army_GetTotalSoldiers(army);
            float dailyF = Army_CalculateFoodUsage(totalSoldiers, army->ganado);
            if (dailyF < 0.1f) dailyF = 1.0f;
            float foodDays = army->comida_disponible / dailyF;

            float dailyW = Army_CalculateWaterUsage(totalSoldiers, army->ganado);
            if (dailyW < 0.1f) dailyW = 1.0f;
            float waterDays = army->agua_disponible / dailyW;

            float btnRight = (float)contentX + (float)contentW - 12;
            float btnSellW = 92;
            float btnBuy2W = 106;
            float btnBuy1W = 102;

            float btnSellX = btnRight - btnSellW;
            float btnBuy2X = btnSellX - 8 - btnBuy2W;
            float btnBuy1X = btnBuy2X - 8 - btnBuy1W;

            // Fila 1: Comida
            int buyFood = City_GetBuyPriceFood(ctx);
            int sellFood = City_GetSellPriceFood(ctx);
            Rectangle fRec = { (float)contentX, (float)rowY, (float)contentW, (float)rowH };
            UI_DrawPanelStyled(fRec, UI_STYLE_PANEL_DARK);

            UI_DrawIcon(UI_ICON_COMIDA, (Vector2){ fRec.x + 12, fRec.y + 11 }, 16.0f, WHITE);
            UI_DrawText("Comida", (Vector2){ fRec.x + 32, fRec.y + 11 }, 17.0f, UI_COLOR_GOLD_LIGHT);
            UI_DrawText(TextFormat("Stock: %d", ctx->stockFood), (Vector2){ fRec.x + 14, fRec.y + 33 }, 13.0f, (Color){ 175, 180, 190, 255 });

            // Cuadrícula de raciones (28 cuadrados)
            UI_DrawSupplySquares((Rectangle){ fRec.x + 105, fRec.y + 9, 165.0f, 38.0f }, foodDays, army->comida_disponible, 28, (Color){ 130, 205, 90, 255 }, "Raciones", army->daysWithoutFood > 0);

            // Precios Compra / Venta
            UI_DrawStatusChip((Rectangle){ fRec.x + 285, fRec.y + 12, 105, 32 }, TextFormat("Compra: %d O", buyFood), UI_STYLE_CHIP_WARN, UI_ICON_NONE);
            UI_DrawStatusChip((Rectangle){ fRec.x + 398, fRec.y + 12, 98, 32 }, TextFormat("Venta: %d O", sellFood), UI_STYLE_CHIP_OK, UI_ICON_NONE);

            int fBuyQty1 = 10;
            int fBuyQty2 = (ctx->stockFood > 25) ? 50 : 20;
            int fSellQty = 10;

            if (UI_DrawButtonSfx((Rectangle){ btnBuy1X, fRec.y + 12, btnBuy1W, 32 }, TextFormat("Comprar %d", fBuyQty1), ctx->stockFood < fBuyQty1 || army->monedas < fBuyQty1 * buyFood, NULL)) {
                if (City_BuyFood(ctx, army, fBuyQty1)) Audio_PlaySfxByName("market_buy");
                else Audio_PlaySfxByName("market_denied");
            }
            if (UI_DrawButtonSfx((Rectangle){ btnBuy2X, fRec.y + 12, btnBuy2W, 32 }, TextFormat("Comprar %d", fBuyQty2), ctx->stockFood < fBuyQty2 || army->monedas < fBuyQty2 * buyFood, NULL)) {
                if (City_BuyFood(ctx, army, fBuyQty2)) Audio_PlaySfxByName("market_buy");
                else Audio_PlaySfxByName("market_denied");
            }
            if (UI_DrawButtonSfx((Rectangle){ btnSellX, fRec.y + 12, btnSellW, 32 }, TextFormat("Vender %d", fSellQty), (int)army->comida_disponible < fSellQty, NULL)) {
                if (City_SellFood(ctx, army, fSellQty)) Audio_PlaySfxByName("market_sell");
                else Audio_PlaySfxByName("market_denied");
            }
            rowY += rowSpacing;

            // Fila 2: Agua
            int buyWater = City_GetBuyPriceWater(ctx);
            int sellWater = City_GetSellPriceWater(ctx);
            Rectangle wRec = { (float)contentX, (float)rowY, (float)contentW, (float)rowH };
            UI_DrawPanelStyled(wRec, UI_STYLE_PANEL_DARK);

            UI_DrawIcon(UI_ICON_AGUA, (Vector2){ wRec.x + 12, wRec.y + 11 }, 16.0f, WHITE);
            UI_DrawText("Agua", (Vector2){ wRec.x + 32, wRec.y + 11 }, 17.0f, UI_COLOR_GOLD_LIGHT);
            UI_DrawText(TextFormat("Stock: %d", ctx->stockWater), (Vector2){ wRec.x + 14, wRec.y + 33 }, 13.0f, (Color){ 175, 180, 190, 255 });

            // Cuadrícula de agua (28 cuadrados)
            UI_DrawSupplySquares((Rectangle){ wRec.x + 105, wRec.y + 9, 165.0f, 38.0f }, waterDays, army->agua_disponible, 28, (Color){ 90, 180, 255, 255 }, "Odres", army->daysWithoutWater > 0);

            // Precios Compra / Venta
            UI_DrawStatusChip((Rectangle){ wRec.x + 285, wRec.y + 12, 105, 32 }, TextFormat("Compra: %d O", buyWater), UI_STYLE_CHIP_WARN, UI_ICON_NONE);
            UI_DrawStatusChip((Rectangle){ wRec.x + 398, wRec.y + 12, 98, 32 }, TextFormat("Venta: %d O", sellWater), UI_STYLE_CHIP_OK, UI_ICON_NONE);

            int wBuyQty1 = 10;
            int wBuyQty2 = (ctx->stockWater > 25) ? 50 : 20;
            int wSellQty = 10;

            if (UI_DrawButtonSfx((Rectangle){ btnBuy1X, wRec.y + 12, btnBuy1W, 32 }, TextFormat("Comprar %d", wBuyQty1), ctx->stockWater < wBuyQty1 || army->monedas < wBuyQty1 * buyWater, NULL)) {
                if (City_BuyWater(ctx, army, wBuyQty1)) Audio_PlaySfxByName("market_buy");
                else Audio_PlaySfxByName("market_denied");
            }
            if (UI_DrawButtonSfx((Rectangle){ btnBuy2X, wRec.y + 12, btnBuy2W, 32 }, TextFormat("Comprar %d", wBuyQty2), ctx->stockWater < wBuyQty2 || army->monedas < wBuyQty2 * buyWater, NULL)) {
                if (City_BuyWater(ctx, army, wBuyQty2)) Audio_PlaySfxByName("market_buy");
                else Audio_PlaySfxByName("market_denied");
            }
            if (UI_DrawButtonSfx((Rectangle){ btnSellX, wRec.y + 12, btnSellW, 32 }, TextFormat("Vender %d", wSellQty), (int)army->agua_disponible < wSellQty, NULL)) {
                if (City_SellWater(ctx, army, wSellQty)) Audio_PlaySfxByName("market_sell");
                else Audio_PlaySfxByName("market_denied");
            }
            rowY += rowSpacing;

            // Fila 3: Vino
            int buyWine = City_GetBuyPriceWine(ctx);
            int sellWine = City_GetSellPriceWine(ctx);
            Rectangle vnRec = { (float)contentX, (float)rowY, (float)contentW, (float)rowH };
            UI_DrawPanelStyled(vnRec, UI_STYLE_PANEL_DARK);

            UI_DrawIcon(UI_ICON_VINO, (Vector2){ vnRec.x + 12, vnRec.y + 11 }, 16.0f, WHITE);
            UI_DrawText("Vino", (Vector2){ vnRec.x + 32, vnRec.y + 11 }, 17.0f, (Color){ 230, 160, 255, 255 });
            UI_DrawText(TextFormat("Stock: %d  |  Barriles: %.0f", ctx->stockWine, army->vino_disponible), (Vector2){ vnRec.x + 14, vnRec.y + 33 }, 13.0f, (Color){ 175, 180, 190, 255 });

            UI_DrawStatusChip((Rectangle){ vnRec.x + 285, vnRec.y + 12, 105, 32 }, TextFormat("Compra: %d O", buyWine), UI_STYLE_CHIP_WARN, UI_ICON_NONE);
            UI_DrawStatusChip((Rectangle){ vnRec.x + 398, vnRec.y + 12, 98, 32 }, TextFormat("Venta: %d O", sellWine), UI_STYLE_CHIP_OK, UI_ICON_NONE);

            int vnBuyQty1 = 2;
            int vnBuyQty2 = (ctx->stockWine > 10) ? 10 : 5;
            int vnSellQty = 2;

            if (UI_DrawButtonSfx((Rectangle){ btnBuy1X, vnRec.y + 12, btnBuy1W, 32 }, TextFormat("Comprar %d", vnBuyQty1), ctx->stockWine < vnBuyQty1 || army->monedas < vnBuyQty1 * buyWine, NULL)) {
                if (City_BuyWine(ctx, army, vnBuyQty1)) Audio_PlaySfxByName("market_buy");
                else Audio_PlaySfxByName("market_denied");
            }
            if (UI_DrawButtonSfx((Rectangle){ btnBuy2X, vnRec.y + 12, btnBuy2W, 32 }, TextFormat("Comprar %d", vnBuyQty2), ctx->stockWine < vnBuyQty2 || army->monedas < vnBuyQty2 * buyWine, NULL)) {
                if (City_BuyWine(ctx, army, vnBuyQty2)) Audio_PlaySfxByName("market_buy");
                else Audio_PlaySfxByName("market_denied");
            }
            if (UI_DrawButtonSfx((Rectangle){ btnSellX, vnRec.y + 12, btnSellW, 32 }, TextFormat("Vender %d", vnSellQty), (int)army->vino_disponible < vnSellQty, NULL)) {
                if (City_SellWine(ctx, army, vnSellQty)) Audio_PlaySfxByName("market_sell");
                else Audio_PlaySfxByName("market_denied");
            }
            rowY += rowSpacing;

            // Fila 4: Carne
            int buyMeat = City_GetBuyPriceMeat(ctx);
            int sellMeat = City_GetSellPriceMeat(ctx);
            Rectangle mtRec = { (float)contentX, (float)rowY, (float)contentW, (float)rowH };
            UI_DrawPanelStyled(mtRec, UI_STYLE_PANEL_DARK);

            UI_DrawIcon(UI_ICON_CARNE, (Vector2){ mtRec.x + 12, mtRec.y + 11 }, 16.0f, WHITE);
            UI_DrawText("Carne", (Vector2){ mtRec.x + 32, mtRec.y + 11 }, 17.0f, (Color){ 255, 180, 140, 255 });
            UI_DrawText(TextFormat("Stock: %d  |  Raciones: %.0f", ctx->stockMeat, army->carne_disponible), (Vector2){ mtRec.x + 14, mtRec.y + 33 }, 13.0f, (Color){ 175, 180, 190, 255 });

            UI_DrawStatusChip((Rectangle){ mtRec.x + 285, mtRec.y + 12, 105, 32 }, TextFormat("Compra: %d O", buyMeat), UI_STYLE_CHIP_WARN, UI_ICON_NONE);
            UI_DrawStatusChip((Rectangle){ mtRec.x + 398, mtRec.y + 12, 98, 32 }, TextFormat("Venta: %d O", sellMeat), UI_STYLE_CHIP_OK, UI_ICON_NONE);

            int mtBuyQty1 = 2;
            int mtBuyQty2 = (ctx->stockMeat > 10) ? 10 : 5;
            int mtSellQty = 2;

            if (UI_DrawButtonSfx((Rectangle){ btnBuy1X, mtRec.y + 12, btnBuy1W, 32 }, TextFormat("Comprar %d", mtBuyQty1), ctx->stockMeat < mtBuyQty1 || army->monedas < mtBuyQty1 * buyMeat, NULL)) {
                if (City_BuyMeat(ctx, army, mtBuyQty1)) Audio_PlaySfxByName("market_buy");
                else Audio_PlaySfxByName("market_denied");
            }
            if (UI_DrawButtonSfx((Rectangle){ btnBuy2X, mtRec.y + 12, btnBuy2W, 32 }, TextFormat("Comprar %d", mtBuyQty2), ctx->stockMeat < mtBuyQty2 || army->monedas < mtBuyQty2 * buyMeat, NULL)) {
                if (City_BuyMeat(ctx, army, mtBuyQty2)) Audio_PlaySfxByName("market_buy");
                else Audio_PlaySfxByName("market_denied");
            }
            if (UI_DrawButtonSfx((Rectangle){ btnSellX, mtRec.y + 12, btnSellW, 32 }, TextFormat("Vender %d", mtSellQty), (int)army->carne_disponible < mtSellQty, NULL)) {
                if (City_SellMeat(ctx, army, mtSellQty)) Audio_PlaySfxByName("market_sell");
                else Audio_PlaySfxByName("market_denied");
            }
            rowY += rowSpacing;

            // Fila 5: Ganado
            int buyCattle = City_GetBuyPriceCattle(ctx);
            int sellCattle = City_GetSellPriceCattle(ctx);
            Rectangle cRec = { (float)contentX, (float)rowY, (float)contentW, (float)rowH };
            UI_DrawPanelStyled(cRec, UI_STYLE_PANEL_DARK);

            UI_DrawIcon(UI_ICON_GANADO, (Vector2){ cRec.x + 12, cRec.y + 11 }, 16.0f, WHITE);
            UI_DrawText("Ganado Vivo", (Vector2){ cRec.x + 32, cRec.y + 11 }, 17.0f, UI_COLOR_GOLD_LIGHT);
            UI_DrawText(TextFormat("En rebano: %d  |  Stock: %d", army->ganado, ctx->stockCattle), (Vector2){ cRec.x + 14, cRec.y + 33 }, 13.0f, (Color){ 175, 180, 190, 255 });

            // Precios Compra / Venta
            UI_DrawStatusChip((Rectangle){ cRec.x + 285, cRec.y + 12, 105, 32 }, TextFormat("Compra: %d O", buyCattle), UI_STYLE_CHIP_WARN, UI_ICON_NONE);
            UI_DrawStatusChip((Rectangle){ cRec.x + 398, cRec.y + 12, 98, 32 }, TextFormat("Venta: %d O", sellCattle), UI_STYLE_CHIP_OK, UI_ICON_NONE);

            if (UI_DrawButtonSfx((Rectangle){ btnBuy1X, cRec.y + 12, btnBuy1W, 32 }, "Comprar 1", ctx->stockCattle < 1 || army->monedas < buyCattle, NULL)) {
                if (City_BuyCattle(ctx, army, 1)) Audio_PlaySfxByName("market_buy");
                else Audio_PlaySfxByName("market_denied");
            }
            if (UI_DrawButtonSfx((Rectangle){ btnBuy2X, cRec.y + 12, btnBuy2W, 32 }, "Comprar 3", ctx->stockCattle < 3 || army->monedas < 3 * buyCattle, NULL)) {
                if (City_BuyCattle(ctx, army, 3)) Audio_PlaySfxByName("market_buy");
                else Audio_PlaySfxByName("market_denied");
            }
            if (UI_DrawButtonSfx((Rectangle){ btnSellX, cRec.y + 12, btnSellW, 32 }, "Vender 1", army->ganado < 1, NULL)) {
                if (City_SellCattle(ctx, army, 1)) Audio_PlaySfxByName("market_sell");
                else Audio_PlaySfxByName("market_denied");
            }
            rowY += rowSpacing;

            // Fila 6: Iniciados
            int buyInit = City_GetBuyPriceInitiate(ctx);
            Rectangle iRec = { (float)contentX, (float)rowY, (float)contentW, (float)rowH };
            UI_DrawPanelStyled(iRec, UI_STYLE_PANEL_DARK);

            UI_DrawText("Reclutar Iniciados", (Vector2){ iRec.x + 14, iRec.y + 11 }, 17.0f, UI_COLOR_GOLD_LIGHT);
            UI_DrawText(TextFormat("En filas: %d  |  Disponibles: %d", army->iniciados, ctx->stockInitiates), (Vector2){ iRec.x + 14, iRec.y + 33 }, 13.0f, (Color){ 175, 180, 190, 255 });

            UI_DrawStatusChip((Rectangle){ iRec.x + 285, iRec.y + 12, 211, 32 }, TextFormat("Coste: %d Oro c/u", buyInit), UI_STYLE_CHIP_WARN, UI_ICON_NONE);

            if (UI_DrawButtonSfx((Rectangle){ btnBuy2X, iRec.y + 12, btnBuy2W, 32 }, "Reclutar 1", ctx->stockInitiates < 1 || army->monedas < buyInit, NULL)) {
                if (City_RecruitInitiates(ctx, army, 1)) Audio_PlaySfxByName("market_buy");
                else Audio_PlaySfxByName("market_denied");
            }
            if (UI_DrawButtonSfx((Rectangle){ btnSellX, iRec.y + 12, btnSellW, 32 }, "Reclutar 5", ctx->stockInitiates < 5 || army->monedas < 5 * buyInit, NULL)) {
                if (City_RecruitInitiates(ctx, army, 5)) Audio_PlaySfxByName("market_buy");
                else Audio_PlaySfxByName("market_denied");
            }
            break;
        }


        case CITY_TAB_ARMORY: {
            UI_DrawText("Armeria y Cuartel Hoplita", (Vector2){ (float)contentX, (float)contentY }, 22.0f, UI_COLOR_TEXT_TITLE);

            int armY = contentY + 36;
            int maxPromInit = City_GetMaxPromoteInitiates(ctx, army);
            int maxPromSold = City_GetMaxPromoteSoldiers(ctx, army);

            // Tarjeta 1: Iniciados -> Soldados
            Rectangle t1Rec = { (float)contentX, (float)armY, (float)contentW, 76.0f };
            UI_DrawPanelStyled(t1Rec, UI_STYLE_PANEL_DARK);

            UI_DrawText("Instruccion: Soldados de Linea", (Vector2){ t1Rec.x + 20, t1Rec.y + 14 }, 18.0f, UI_COLOR_GOLD_LIGHT);
            UI_DrawText(TextFormat("Iniciados: %d  |  Coste: 15 Oro  |  Cupo: %d/10", army->iniciados, ctx->promotedInitiates), (Vector2){ t1Rec.x + 20, t1Rec.y + 44 }, 14.0f, UI_COLOR_TEXT_DIM);

            if (UI_DrawButtonSfx((Rectangle){ t1Rec.x + t1Rec.width - 270, t1Rec.y + 18, 120, 38 }, "Ascender 1", maxPromInit < 1, NULL)) {
                if (City_PromoteInitiates(ctx, army, 1)) Audio_PlaySfxByName("promote_troops");
            }
            if (UI_DrawButtonSfx((Rectangle){ t1Rec.x + t1Rec.width - 140, t1Rec.y + 18, 120, 38 }, "Ascender 5", maxPromInit < 5, NULL)) {
                if (City_PromoteInitiates(ctx, army, 5)) Audio_PlaySfxByName("promote_troops");
            }
            armY += 92;

            // Tarjeta 2: Soldados -> Sargentos
            Rectangle t2Rec = { (float)contentX, (float)armY, (float)contentW, 76.0f };
            UI_DrawPanelStyled(t2Rec, UI_STYLE_PANEL_DARK);

            UI_DrawText("Promocion: Sargentos Veteranos", (Vector2){ t2Rec.x + 20, t2Rec.y + 14 }, 18.0f, UI_COLOR_GOLD_LIGHT);
            UI_DrawText(TextFormat("Soldados: %d | Sargentos: %d | Cupo: %d/2", army->soldados, army->sargentos, ctx->promotedSoldiers), (Vector2){ t2Rec.x + 20, t2Rec.y + 44 }, 14.0f, UI_COLOR_TEXT_DIM);

            if (UI_DrawButtonSfx((Rectangle){ t2Rec.x + t2Rec.width - 220, t2Rec.y + 18, 200, 38 }, "Ascender 1 (60 Oro)", maxPromSold < 1, NULL)) {
                if (City_PromoteSoldiers(ctx, army, 1)) Audio_PlaySfxByName("promote_troops");
            }
            break;
        }

        case CITY_TAB_HOSPITAL: {
            UI_DrawText("Hospital de Campana", (Vector2){ (float)contentX, (float)contentY }, 22.0f, UI_COLOR_TEXT_TITLE);

            int hospY = contentY + 36;
            Rectangle hCard = { (float)contentX, (float)hospY, (float)contentW, 140.0f };
            UI_DrawPanelStyled(hCard, UI_STYLE_PANEL_DARK);

            // Barra de Salud visual
            DrawStatBar((Rectangle){ hCard.x + 24, hCard.y + 20, hCard.width - 48, 28 }, army->salud, 100.0f, "Salud General", (army->salud < 50.0f) ? (Color){ 190, 60, 70, 255 } : (Color){ 90, 150, 75, 255 });

            int missingSalud = (int)ceilf(100.0f - army->salud);
            int heal10Cost = City_GetHospitalCost(ctx, army, (missingSalud < 10) ? missingSalud : 10);
            int healAllCost = City_GetHospitalCost(ctx, army, missingSalud);

            if (UI_DrawButtonStyledSfx((Rectangle){ hCard.x + 24, hCard.y + 70, 260, 44 }, TextFormat("Curar +10 Salud (%d Oro)", heal10Cost), UI_STYLE_BTN_PRIMARY, missingSalud <= 0 || army->monedas < heal10Cost, NULL)) {
                if (City_HealArmy(ctx, army, (missingSalud < 10) ? missingSalud : 10)) Audio_PlaySfxByName("heal");
            }
            if (UI_DrawButtonStyledSfx((Rectangle){ hCard.x + 300, hCard.y + 70, 310, 44 }, TextFormat("Curar Todo (+%d Salud, %d Oro)", missingSalud, healAllCost), UI_STYLE_BTN_PRIMARY, missingSalud <= 0 || army->monedas < healAllCost, NULL)) {
                if (City_HealArmy(ctx, army, missingSalud)) Audio_PlaySfxByName("heal");
            }
            break;
        }

        case CITY_TAB_TEMPLE: {
            UI_DrawText("Templo y Altar Sagrado", (Vector2){ (float)contentX, (float)contentY }, 22.0f, UI_COLOR_TEXT_TITLE);

            int tempY = contentY + 34;
            /* 1. Panel Superior: Purificación de Maldiciones */
            Rectangle tCard = { (float)contentX, (float)tempY, (float)contentW, 134.0f };
            UI_DrawPanelStyled(tCard, UI_STYLE_PANEL_DARK);
            UI_DrawText("PURIFICACION DE MALDICIONES", (Vector2){ tCard.x + 24, tCard.y + 10 }, 15.0f, UI_COLOR_GOLD);

            int totalCurses = army->curseCount + Commander_GetCurseCount(army->commander);
            int curseCost = City_GetTempleCurseRemoveCost(ctx, army);
            if (totalCurses > 0) {
                UI_DrawStatusChip((Rectangle){ tCard.x + 24, tCard.y + 34, 420, 28 }, TextFormat("Maldiciones activas: %d", totalCurses), UI_STYLE_CHIP_BAD, UI_ICON_PELIGRO);
            } else {
                UI_DrawStatusChip((Rectangle){ tCard.x + 24, tCard.y + 34, 420, 28 }, "El ejercito no padece maldiciones.", UI_STYLE_CHIP_OK, UI_ICON_OK);
            }

            if (UI_DrawButtonStyledSfx((Rectangle){ tCard.x + 24, tCard.y + 74, 340, 42 }, TextFormat("Purificar 1 Maldicion (%d Oro)", curseCost), UI_STYLE_BTN_CURSE, totalCurses <= 0 || army->monedas < curseCost, NULL)) {
                if (City_RemoveCurse(ctx, army)) Audio_PlaySfxByName("heal");
            }

            /* 2. Panel Inferior: Reconsagración de Cartas Divinas */
            Rectangle dCard = { (float)contentX, (float)(tempY + 146), (float)contentW, 146.0f };
            UI_DrawPanelStyled(dCard, UI_STYLE_PANEL_DARK);
            UI_DrawText("RECONSAGRACION DE CARTAS DIVINAS", (Vector2){ dCard.x + 24, dCard.y + 10 }, 15.0f, UI_COLOR_GOLD);

            int exhaustedCount = 0;
            int firstExhaustedSlot = -1;
            if (army && army->commander.ptr) {
                for (int c = 0; c < 8; c++) {
                    ModifierInstance* inst = Commander_GetCardInstance(army->commander, c);
                    if (!inst) break;
                    if (inst->isExhausted) {
                        exhaustedCount++;
                        if (firstExhaustedSlot == -1) firstExhaustedSlot = c;
                    }
                }
            }

            if (exhaustedCount > 0) {
                UI_DrawStatusChip((Rectangle){ dCard.x + 24, dCard.y + 34, 420, 28 }, TextFormat("Cartas divinas agotadas: %d", exhaustedCount), UI_STYLE_CHIP_WARN, UI_ICON_AVISO);
                if (UI_DrawButtonStyledSfx((Rectangle){ dCard.x + 24, dCard.y + 76, 280, 42 }, "Reconsagrar (15 Oro)", UI_STYLE_BTN_DEFAULT, army->monedas < 15, NULL)) {
                    army->monedas -= 15;
                    Commander_SetCardExhausted(army->commander, firstExhaustedSlot, false);
                    Audio_PlaySfxByName("heal");
                }
                if (UI_DrawButtonStyledSfx((Rectangle){ dCard.x + 316, dCard.y + 76, 280, 42 }, "Ofrendar Vino (4 Vino)", UI_STYLE_BTN_DEFAULT, army->vino_disponible < 4.0f, NULL)) {
                    army->vino_disponible -= 4.0f;
                    Commander_SetCardExhausted(army->commander, firstExhaustedSlot, false);
                    Audio_PlaySfxByName("heal");
                }
            } else {
                UI_DrawStatusChip((Rectangle){ dCard.x + 24, dCard.y + 34, 480, 28 }, "Todas las cartas divinas estan consagradas y listas para la batalla.", UI_STYLE_CHIP_OK, UI_ICON_OK);
                UI_DrawText("Las cartas divinas se agotan al invocar su poder activo en combate.", (Vector2){ dCard.x + 24, dCard.y + 76 }, 14, (Color){ 170, 170, 180, 255 });
            }
            break;
        }

        case CITY_TAB_INN: {
            UI_DrawText("Posada del Meson", (Vector2){ (float)contentX, (float)contentY }, 22.0f, UI_COLOR_TEXT_TITLE);

            int innY = contentY + 36;
            Rectangle iCard = { (float)contentX, (float)innY, (float)contentW, 160.0f };
            UI_DrawPanelStyled(iCard, UI_STYLE_PANEL_DARK);

            // Barra de Moral visual
            DrawStatBar((Rectangle){ iCard.x + 24, iCard.y + 18, iCard.width - 48, 26 }, army->moral, 100.0f, "Moral del Ejercito", (Color){ 212, 166, 58, 255 });

            UI_DrawText("+ Pasa la noche seguro: descanso hasta la manana siguiente sin riesgo de ataques ni emboscadas.", (Vector2){ iCard.x + 24, iCard.y + 54 }, 13, (Color){ 190, 220, 190, 255 });
            UI_DrawText("+ Comida y bebida incluidas para toda la tropa (racion completa sin gastar provisiones propias).", (Vector2){ iCard.x + 24, iCard.y + 74 }, 13, (Color){ 190, 220, 190, 255 });
            UI_DrawText("+ Recupera +15 Moral y +10 Salud.", (Vector2){ iCard.x + 24, iCard.y + 94 }, 13, UI_COLOR_GOLD_LIGHT);

            int innCost = City_GetInnCost(ctx);
            const char* costDesc = (innCost == 0) ? "Gratis (Aliados)" : TextFormat("%d Monedas de Oro", innCost);

            if (UI_DrawButtonSfx((Rectangle){ iCard.x + 24, iCard.y + 118, 360, 32 }, TextFormat("Pasar la Noche (%s)", costDesc), army->monedas < innCost, NULL)) {
                GameMaster_ChooseInn(gm, map, army);
                Audio_PlaySfxByName("inn_rest");
                return true;
            }
            break;
        }
    }

    return false;
}
