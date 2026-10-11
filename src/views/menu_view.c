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
        UI_DrawText("TITULO DEL JUEGO", (Vector2){cx - 150, cy - 150}, 40, GOLD);
        
        if (UI_DrawButton((Rectangle){cx - 125, cy - 50, 250, 50}, "Nueva Campana", false)) {
            menu->startGame = true;
        }
        if (UI_DrawButton((Rectangle){cx - 125, cy + 20, 250, 50}, "Opciones", false)) {
            menu->state = MENU_STATE_OPTIONS;
        }
        if (UI_DrawButton((Rectangle){cx - 125, cy + 90, 250, 50}, "Salir", false)) {
            menu->quitGame = true;
        }
    } else if (menu->state == MENU_STATE_OPTIONS) {
        UI_DrawText("Opciones", (Vector2){cx - 70, cy - 150}, 40, GOLD);
        
        if (UI_DrawButton((Rectangle){cx - 125, cy + 90, 250, 50}, "Volver", false)) {
            menu->state = MENU_STATE_MAIN;
        }
    }
}
