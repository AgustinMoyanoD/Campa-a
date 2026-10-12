#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"

static Font uiFont = {0};
static bool fontLoaded = false;

void UI_Init(void) {
    // Try to load a font if it exists, otherwise fallback to default
    if (FileExists("assets/font.ttf")) {
        uiFont = LoadFontEx("assets/font.ttf", 32, 0, 250);
        SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
        fontLoaded = true;
    } else {
        uiFont = GetFontDefault();
    }
}

void UI_Shutdown(void) {
    if (fontLoaded) {
        UnloadFont(uiFont);
        fontLoaded = false;
    }
}

void UI_DrawText(const char* text, Vector2 pos, float fontSize, Color color) {
    DrawTextEx(uiFont, text, pos, fontSize, 1.0f, color);
}

void UI_DrawPanel(Rectangle bounds) {
    DrawRectangleRec(bounds, UI_COLOR_PANEL);
    DrawRectangleLinesEx(bounds, 2.0f, UI_COLOR_PANEL_BORDER);
}

bool UI_DrawButton(Rectangle bounds, const char* text, bool disabled) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);
    bool clicked = false;
    
    Color bg = UI_COLOR_BUTTON_IDLE;
    Color fg = UI_COLOR_TEXT_MAIN;
    
    if (disabled) {
        bg = UI_COLOR_BUTTON_DISABLED;
        fg = UI_COLOR_TEXT_DIM;
    } else {
        if (hovered) {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                bg = UI_COLOR_BUTTON_PRESS;
            } else {
                bg = UI_COLOR_BUTTON_HOVER;
            }
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                clicked = true;
            }
        }
    }
    
    DrawRectangleRec(bounds, bg);
    DrawRectangleLinesEx(bounds, 2.0f, UI_COLOR_PANEL_BORDER);
    
    float fontSize = 20.0f;
    Vector2 tSize = MeasureTextEx(uiFont, text, fontSize, 1.0f);
    UI_DrawText(text, (Vector2){ bounds.x + (bounds.width - tSize.x)/2.0f, bounds.y + (bounds.height - tSize.y)/2.0f }, fontSize, fg);
    
    return clicked;
}

void UI_DrawBar(Rectangle bounds, float percentage, Color fg, Color bg) {
    DrawRectangleRec(bounds, bg);
    if (percentage < 0.0f) percentage = 0.0f;
    if (percentage > 1.0f) percentage = 1.0f;
    
    Rectangle fgBounds = { bounds.x, bounds.y, bounds.width * percentage, bounds.height };
    DrawRectangleRec(fgBounds, fg);
    DrawRectangleLinesEx(bounds, 1.0f, UI_COLOR_PANEL_BORDER);
}

bool UI_ButtonClicked(Rectangle bounds) {
    Vector2 mouse = GetMousePosition();
    return CheckCollisionPointRec(mouse, bounds) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
}
