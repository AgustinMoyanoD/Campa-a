#include "views/menu_view.h"
#include "presentation/ui_widgets.h"
#include <raylib.h>

void MenuView_Init(void) {
    //
}

bool MenuView_Update(MainMenu* menu, int screenWidth, int screenHeight) {
    if (!menu) return false;
    return false;
}

void MenuView_Draw(MainMenu* menu, int screenWidth, int screenHeight) {
    if (!menu) return;
    
    int cx = screenWidth / 2;
    int cy = screenHeight / 2;
    
    ClearBackground(BLACK);
    
    if (menu->state == MENU_STATE_MAIN) {
        UI_DrawPanelStyled((Rectangle){ (float)(cx - 190), (float)(cy - 180), 380.0f, 350.0f }, UI_STYLE_PANEL_DARK);
        Vector2 titleSz = UI_MeasureText("TITULO DEL JUEGO", 36);
        UI_DrawText("TITULO DEL JUEGO", (Vector2){(float)(cx - titleSz.x / 2.0f), (float)(cy - 140)}, 36, GOLD);
        
        if (UI_DrawButtonStyledSfx((Rectangle){(float)(cx - 125), (float)(cy - 50), 250.0f, 50.0f}, "Nueva Campana", UI_STYLE_BTN_PRIMARY, false, "menu_click")) {
            menu->startGame = true;
        }
        if (UI_DrawButtonStyledSfx((Rectangle){(float)(cx - 125), (float)(cy + 20), 250.0f, 50.0f}, "Opciones", UI_STYLE_BTN_DEFAULT, false, "menu_click")) {
            menu->state = MENU_STATE_OPTIONS;
        }
        if (UI_DrawButtonStyledSfx((Rectangle){(float)(cx - 125), (float)(cy + 90), 250.0f, 50.0f}, "Salir", UI_STYLE_BTN_DANGER, false, "menu_click")) {
            menu->quitGame = true;
        }
    } else if (menu->state == MENU_STATE_OPTIONS) {
        UI_DrawPanelStyled((Rectangle){ (float)(cx - 190), (float)(cy - 180), 380.0f, 350.0f }, UI_STYLE_PANEL_DARK);
        Vector2 titleSz = UI_MeasureText("Opciones", 36);
        UI_DrawText("Opciones", (Vector2){(float)(cx - titleSz.x / 2.0f), (float)(cy - 140)}, 36, GOLD);
        
        if (UI_DrawButtonStyledSfx((Rectangle){(float)(cx - 125), (float)(cy + 90), 250.0f, 50.0f}, "Volver", UI_STYLE_BTN_DEFAULT, false, "menu_click")) {
            menu->state = MENU_STATE_MAIN;
        }
    }
}
