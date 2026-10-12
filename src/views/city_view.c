#include "views/city_view.h"
#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"
#include <stdio.h>
#include <math.h>

bool CityView_UpdateDraw(CityContext* ctx, Army* army, GameMaster* gm, int SW, int SH) {
    if (!ctx || !army || !gm) return true;

    // Fondo oscurecido
    DrawRectangle(0, 0, SW, SH, Fade(BLACK, 0.75f));

    // Dimensiones del panel central
    int pw = 980;
    int ph = 640;
    int px = (SW - pw) / 2;
    int py = (SH - ph) / 2;

    // Marco del panel
    DrawRectangle(px, py, pw, ph, (Color){ 22, 22, 28, 245 });
    DrawRectangleLinesEx((Rectangle){ (float)px, (float)py, (float)pw, (float)ph }, 2, (Color){ 160, 120, 50, 255 });

    // Cabecera: Título de la estructura
    UI_DrawText(ctx->name, (Vector2){ (float)(px + 30), (float)(py + 15) }, 24, UI_COLOR_GOLD);

    // Cabecera: Información de Facción y Opinión (9.4)
    char facBuf[256];
    FactionOpinionLevel opLvl = Faction_GetLevel(ctx->factionOpinion, Balance_GetGlobal());
    const char* opName = Faction_GetLevelName(opLvl);
    const char* alertStr = (ctx->factionAlertLevel == 0) ? "Calma" : ((ctx->factionAlertLevel == 1) ? "Alerta Guardia" : "Bajo Asedio");
    snprintf(facBuf, sizeof(facBuf), "Faccion: %s | Relacion: %s (%+d) | Estado: %s",
             ctx->factionName, opName, ctx->factionOpinion, alertStr);
    Color facCol = (opLvl == FACTION_HOSTILE) ? RED : ((opLvl == FACTION_ALLIED) ? GREEN : ((opLvl == FACTION_FRIENDLY) ? SKYBLUE : ((opLvl == FACTION_SUSPICIOUS) ? ORANGE : RAYWHITE)));
    UI_DrawText(facBuf, (Vector2){ (float)(px + 30), (float)(py + 45) }, 15, facCol);
    
    // Recursos del ejército
    char statusBuf[256];
    snprintf(statusBuf, sizeof(statusBuf), "Oro: %d | Comida: %.0f | Agua: %.0f | Ganado: %d | Salud: %.0f/100 | Moral: %.0f/100 | Tropas: %d (Ini:%d Sol:%d Sgt:%d)",
             army->monedas, army->comida_disponible, army->agua_disponible, army->ganado,
             army->salud, army->moral,
             Army_GetTotalSoldiers(army), army->iniciados, army->soldados, army->sargentos);
    UI_DrawText(statusBuf, (Vector2){ (float)(px + 30), (float)(py + 68) }, 14, LIGHTGRAY);

    // Barra de pestañas de navegación
    int tabY = py + 95;
    int tabW = 140;
    int tabH = 34;
    int curTabX = px + 30;

    if (UI_DrawButton((Rectangle){ (float)curTabX, (float)tabY, (float)tabW, (float)tabH }, "1. Principal", false) || IsKeyPressed(KEY_ONE)) {
        ctx->currentTab = CITY_TAB_MAIN;
    }
    curTabX += tabW + 10;

    if (City_HasMarket(ctx)) {
        if (UI_DrawButton((Rectangle){ (float)curTabX, (float)tabY, (float)tabW, (float)tabH }, "2. Mercado", false) || IsKeyPressed(KEY_TWO)) {
            ctx->currentTab = CITY_TAB_MARKET;
        }
        curTabX += tabW + 10;
    }

    if (City_HasArmory(ctx)) {
        if (UI_DrawButton((Rectangle){ (float)curTabX, (float)tabY, (float)tabW, (float)tabH }, "3. Armeria", false) || IsKeyPressed(KEY_THREE)) {
            ctx->currentTab = CITY_TAB_ARMORY;
        }
        curTabX += tabW + 10;
    }

    if (City_HasHospital(ctx)) {
        if (UI_DrawButton((Rectangle){ (float)curTabX, (float)tabY, (float)tabW, (float)tabH }, "4. Hospital", false) || IsKeyPressed(KEY_FOUR)) {
            ctx->currentTab = CITY_TAB_HOSPITAL;
        }
        curTabX += tabW + 10;
    }

    if (City_HasTemple(ctx)) {
        if (UI_DrawButton((Rectangle){ (float)curTabX, (float)tabY, (float)tabW, (float)tabH }, "5. Templo", false) || IsKeyPressed(KEY_FIVE)) {
            ctx->currentTab = CITY_TAB_TEMPLE;
        }
        curTabX += tabW + 10;
    }

    if (City_HasInn(ctx)) {
        if (UI_DrawButton((Rectangle){ (float)curTabX, (float)tabY, (float)tabW, (float)tabH }, "6. Posada", false) || IsKeyPressed(KEY_SIX)) {
            ctx->currentTab = CITY_TAB_INN;
        }
        curTabX += tabW + 10;
    }

    // Botón de salir / cerrar
    if (UI_DrawButton((Rectangle){ (float)(px + pw - 120), (float)tabY, 90, (float)tabH }, "Salir [ESC]", false) || IsKeyPressed(KEY_ESCAPE)) {
        GameMaster_CloseCity(gm, NULL, army);
        return true;
    }

    // Línea separadora
    DrawLine(px + 20, tabY + 42, px + pw - 20, tabY + 42, (Color){ 100, 80, 40, 200 });

    // Contenido según la pestaña activa
    int contentY = tabY + 55;
    int contentX = px + 40;

    switch (ctx->currentTab) {
        case CITY_TAB_MAIN: {
            if (ctx->isBesieged) {
                DrawRectangle(contentX, contentY, pw - 80, 40, (Color){ 60, 40, 10, 200 });
                DrawRectangleLines(contentX, contentY, pw - 80, 40, GOLD);
                DrawText("[CIUDAD CONQUISTADA]: La guarnicion fue derrotada y el asentamiento esta bajo tu ocupacion.", contentX + 15, contentY + 12, 16, GOLD);
                contentY += 55;
            } else if (ctx->factionAlertLevel >= 1 || opLvl == FACTION_HOSTILE) {
                DrawRectangle(contentX, contentY, pw - 80, 40, (Color){ 60, 15, 15, 200 });
                DrawRectangleLines(contentX, contentY, pw - 80, 40, RED);
                DrawText("[!] PUERTAS CERRADAS: La guarnicion rechaza el comercio y los servicios civiles debido a la hostilidad o alerta.", contentX + 15, contentY + 12, 15, RED);
                contentY += 55;
            } else {
                UI_DrawText("Bienvenido al asentamiento. Selecciona una opcion o pestana arriba.", (Vector2){ (float)contentX, (float)contentY }, 18, RAYWHITE);
                contentY += 35;
            }
            
            int btnY = contentY + 15;
            if (City_HasMarket(ctx)) {
                if (UI_DrawButton((Rectangle){ (float)contentX, (float)btnY, 320, 38 }, "Ir al Mercado y Provisiones", false)) {
                    ctx->currentTab = CITY_TAB_MARKET;
                }
                btnY += 46;
            }
            if (City_HasArmory(ctx)) {
                if (UI_DrawButton((Rectangle){ (float)contentX, (float)btnY, 320, 38 }, "Ir a la Armeria y Cuartel", false)) {
                    ctx->currentTab = CITY_TAB_ARMORY;
                }
                btnY += 46;
            }
            if (City_HasHospital(ctx)) {
                if (UI_DrawButton((Rectangle){ (float)contentX, (float)btnY, 320, 38 }, "Consultar Hospital de Campana", false)) {
                    ctx->currentTab = CITY_TAB_HOSPITAL;
                }
                btnY += 46;
            }
            if (City_HasTemple(ctx)) {
                if (UI_DrawButton((Rectangle){ (float)contentX, (float)btnY, 320, 38 }, "Visitar Templo y Purificar", false)) {
                    ctx->currentTab = CITY_TAB_TEMPLE;
                }
                btnY += 46;
            }
            if (City_HasInn(ctx)) {
                if (UI_DrawButton((Rectangle){ (float)contentX, (float)btnY, 320, 38 }, "Descansar en la Posada", false)) {
                    ctx->currentTab = CITY_TAB_INN;
                }
                btnY += 46;
            }

            // Opción de Asedio (9.5)
            if (City_CanSiege(ctx)) {
                btnY += 10;
                DrawRectangle(contentX, btnY, 450, 70, (Color){ 40, 20, 20, 220 });
                DrawRectangleLines(contentX, btnY, 450, 70, RED);
                DrawText("OPCION TACTICA: ASEDIAR ASENTAMIENTO (C3, 9.5)", contentX + 15, btnY + 8, 14, GOLD);
                DrawText("Enfrentar a la guarnicion fortificada para saquear grandes botines.", contentX + 15, btnY + 28, 12, LIGHTGRAY);
                
                if (UI_DrawButton((Rectangle){ (float)(contentX + 15), (float)(btnY + 42), 220, 24 }, "Proponer Asedio [A]", false) || IsKeyPressed(KEY_A)) {
                    ctx->requestSiege = true;
                    GameMaster_CloseCity(gm, NULL, army);
                    return true;
                }
            }
            break;
        }

        case CITY_TAB_MARKET: {
            UI_DrawText("Mercado de Abastos (Compra y Venta)", (Vector2){ (float)contentX, (float)contentY }, 22, UI_COLOR_GOLD);
            UI_DrawText("Los precios de venta son un 40% del valor base. Precios sujetos a la reputacion con la faccion.", (Vector2){ (float)contentX, (float)(contentY + 25) }, 14, LIGHTGRAY);

            int rowY = contentY + 60;

            // Fila 1: Comida
            int buyFood = City_GetBuyPriceFood(ctx);
            int sellFood = City_GetSellPriceFood(ctx);
            DrawText(TextFormat("Comida: Stock: %d | Compra: %d oro | Venta: %d oro | En ejercito: %.0f", ctx->stockFood, buyFood, sellFood, army->comida_disponible), contentX, rowY + 5, 16, RAYWHITE);
            if (UI_DrawButton((Rectangle){ (float)(contentX + 550), (float)rowY, 95, 30 }, "Comprar 5", ctx->stockFood < 5 || army->monedas < 5 * buyFood)) {
                City_BuyFood(ctx, army, 5);
            }
            if (UI_DrawButton((Rectangle){ (float)(contentX + 655), (float)rowY, 105, 30 }, "Comprar 20", ctx->stockFood < 20 || army->monedas < 20 * buyFood)) {
                City_BuyFood(ctx, army, 20);
            }
            if (UI_DrawButton((Rectangle){ (float)(contentX + 770), (float)rowY, 95, 30 }, "Vender 5", (int)army->comida_disponible < 5)) {
                City_SellFood(ctx, army, 5);
            }
            rowY += 45;

            // Fila 2: Agua
            int buyWater = City_GetBuyPriceWater(ctx);
            int sellWater = City_GetSellPriceWater(ctx);
            DrawText(TextFormat("Agua: Stock: %d | Compra: %d oro | Venta: %d oro | En ejercito: %.0f", ctx->stockWater, buyWater, sellWater, army->agua_disponible), contentX, rowY + 5, 16, RAYWHITE);
            if (UI_DrawButton((Rectangle){ (float)(contentX + 550), (float)rowY, 95, 30 }, "Comprar 5", ctx->stockWater < 5 || army->monedas < 5 * buyWater)) {
                City_BuyWater(ctx, army, 5);
            }
            if (UI_DrawButton((Rectangle){ (float)(contentX + 655), (float)rowY, 105, 30 }, "Comprar 20", ctx->stockWater < 20 || army->monedas < 20 * buyWater)) {
                City_BuyWater(ctx, army, 20);
            }
            if (UI_DrawButton((Rectangle){ (float)(contentX + 770), (float)rowY, 95, 30 }, "Vender 5", (int)army->agua_disponible < 5)) {
                City_SellWater(ctx, army, 5);
            }
            rowY += 45;

            // Fila 3: Ganado
            int buyCattle = City_GetBuyPriceCattle(ctx);
            int sellCattle = City_GetSellPriceCattle(ctx);
            DrawText(TextFormat("Ganado: Stock: %d | Compra: %d oro | Venta: %d oro | En ejercito: %d", ctx->stockCattle, buyCattle, sellCattle, army->ganado), contentX, rowY + 5, 16, RAYWHITE);
            if (UI_DrawButton((Rectangle){ (float)(contentX + 550), (float)rowY, 95, 30 }, "Comprar 1", ctx->stockCattle < 1 || army->monedas < buyCattle)) {
                City_BuyCattle(ctx, army, 1);
            }
            if (UI_DrawButton((Rectangle){ (float)(contentX + 655), (float)rowY, 105, 30 }, "Comprar 3", ctx->stockCattle < 3 || army->monedas < 3 * buyCattle)) {
                City_BuyCattle(ctx, army, 3);
            }
            if (UI_DrawButton((Rectangle){ (float)(contentX + 770), (float)rowY, 95, 30 }, "Vender 1", army->ganado < 1)) {
                City_SellCattle(ctx, army, 1);
            }
            rowY += 45;

            // Fila 4: Reclutar Iniciados
            int buyInit = City_GetBuyPriceInitiate(ctx);
            DrawText(TextFormat("Iniciados: Disponibles: %d | Coste alistamiento: %d oro | En ejercito: %d", ctx->stockInitiates, buyInit, army->iniciados), contentX, rowY + 5, 16, RAYWHITE);
            if (UI_DrawButton((Rectangle){ (float)(contentX + 550), (float)rowY, 95, 30 }, "Reclutar 1", ctx->stockInitiates < 1 || army->monedas < buyInit)) {
                City_RecruitInitiates(ctx, army, 1);
            }
            if (UI_DrawButton((Rectangle){ (float)(contentX + 655), (float)rowY, 105, 30 }, "Reclutar 5", ctx->stockInitiates < 5 || army->monedas < 5 * buyInit)) {
                City_RecruitInitiates(ctx, army, 5);
            }
            break;
        }

        case CITY_TAB_ARMORY: {
            UI_DrawText("Armeria y Campo de Instruccion", (Vector2){ (float)contentX, (float)contentY }, 22, UI_COLOR_GOLD);
            UI_DrawText("Adiestra a tus reclutas e instruye veteranos. El entrenamiento consume la jornada militar.", (Vector2){ (float)contentX, (float)(contentY + 25) }, 14, LIGHTGRAY);

            int armY = contentY + 65;

            // Ascenso 1: Iniciados -> Soldados
            int maxPromInit = City_GetMaxPromoteInitiates(ctx, army);
            DrawText(TextFormat("Ascenso a Soldados de Linea: Iniciados aptos: %d | Coste: 15 Oro | Limite visita: %d/10", army->iniciados, ctx->promotedInitiates), contentX, armY + 5, 16, RAYWHITE);
            if (UI_DrawButton((Rectangle){ (float)(contentX + 655), (float)armY, 105, 30 }, "Ascender 1", maxPromInit < 1)) {
                City_PromoteInitiates(ctx, army, 1);
            }
            if (UI_DrawButton((Rectangle){ (float)(contentX + 770), (float)armY, 105, 30 }, "Ascender 5", maxPromInit < 5)) {
                City_PromoteInitiates(ctx, army, 5);
            }
            armY += 55;

            // Ascenso 2: Soldados -> Sargentos
            int maxPromSold = City_GetMaxPromoteSoldiers(ctx, army);
            DrawText(TextFormat("Ascenso a Sargentos Veteranos: Soldados: %d | Sargentos: %d | Ratio max 1:15 | Limite: %d/2", army->soldados, army->sargentos, ctx->promotedSoldiers), contentX, armY + 5, 15, RAYWHITE);
            if (UI_DrawButton((Rectangle){ (float)(contentX + 655), (float)armY, 105, 30 }, "Ascender 1 (60 O)", maxPromSold < 1)) {
                City_PromoteSoldiers(ctx, army, 1);
            }
            break;
        }

        case CITY_TAB_HOSPITAL: {
            UI_DrawText("Hospital de Campana y Sanadores", (Vector2){ (float)contentX, (float)contentY }, 22, UI_COLOR_GOLD);
            UI_DrawText("Los cirujanos atienden las heridas de los soldados fuera del campo de batalla por una tasa fija.", (Vector2){ (float)contentX, (float)(contentY + 25) }, 14, LIGHTGRAY);

            int hospY = contentY + 70;
            DrawText(TextFormat("Salud Actual del Ejercito: %.1f / 100.0", army->salud), contentX, hospY, 18, (army->salud < 50.0f) ? RED : RAYWHITE);
            DrawText("Coste del tratamiento: 2 monedas de oro por punto de salud curado.", contentX, hospY + 25, 14, GRAY);

            hospY += 60;
            int missingSalud = (int)ceilf(100.0f - army->salud);
            int heal10Cost = City_GetHospitalCost(ctx, army, (missingSalud < 10) ? missingSalud : 10);
            int healAllCost = City_GetHospitalCost(ctx, army, missingSalud);

            if (UI_DrawButton((Rectangle){ (float)contentX, (float)hospY, 220, 45 }, TextFormat("Curar +10 Salud (%d Oro)", heal10Cost), missingSalud <= 0 || army->monedas < heal10Cost)) {
                City_HealArmy(ctx, army, (missingSalud < 10) ? missingSalud : 10);
            }
            if (UI_DrawButton((Rectangle){ (float)(contentX + 240), (float)hospY, 260, 45 }, TextFormat("Curar Todo (+%d Salud, %d Oro)", missingSalud, healAllCost), missingSalud <= 0 || army->monedas < healAllCost)) {
                City_HealArmy(ctx, army, missingSalud);
            }
            break;
        }

        case CITY_TAB_TEMPLE: {
            UI_DrawText("Templo y Santuario Divino", (Vector2){ (float)contentX, (float)contentY }, 22, UI_COLOR_GOLD);
            UI_DrawText("Los sacerdotes pueden purificar maldiciones que afligen al ejercito. El coste escala con cada quita.", (Vector2){ (float)contentX, (float)(contentY + 25) }, 14, LIGHTGRAY);

            int tempY = contentY + 70;
            int curseCost = City_GetTempleCurseRemoveCost(ctx, army);
            DrawText(TextFormat("Maldiciones activas en el ejercito: %d", army->curseCount), contentX, tempY, 18, (army->curseCount > 0) ? RED : GREEN);
            DrawText(TextFormat("Coste de purificacion: %d monedas de oro (Maldiciones ya quitadas: %d)", curseCost, army->cursesRemovedCount), contentX, tempY + 25, 14, GRAY);

            tempY += 60;
            if (UI_DrawButton((Rectangle){ (float)contentX, (float)tempY, 280, 45 }, TextFormat("Purificar 1 Maldicion (%d Oro)", curseCost), army->curseCount <= 0 || army->monedas < curseCost)) {
                City_RemoveCurse(ctx, army);
            }
            break;
        }

        case CITY_TAB_INN: {
            UI_DrawText("Posada del Asentamiento", (Vector2){ (float)contentX, (float)contentY }, 22, UI_COLOR_GOLD);
            UI_DrawText("Un lugar calido para que las tropas descansen y recuperen el animo. Consume la jornada.", (Vector2){ (float)contentX, (float)(contentY + 25) }, 14, LIGHTGRAY);

            int innY = contentY + 70;
            int innCost = City_GetInnCost(ctx);
            DrawText(TextFormat("Moral Actual: %.1f / 100.0", army->moral), contentX, innY, 18, RAYWHITE);
            const char* costDesc = (innCost == 0) ? "GRATIS para aliados de la faccion!" : TextFormat("%d monedas de oro", innCost);
            DrawText(TextFormat("Coste de estancia: %s (Recupera +15 de Moral y finaliza el dia)", costDesc), contentX, innY + 25, 14, GRAY);

            innY += 60;
            if (UI_DrawButton((Rectangle){ (float)contentX, (float)innY, 320, 45 }, TextFormat("Descansar en la Posada (%d Oro)", innCost), army->monedas < innCost || army->moral >= 100.0f)) {
                City_RestAtInn(ctx, army);
            }
            break;
        }
    }

    return false;
}
