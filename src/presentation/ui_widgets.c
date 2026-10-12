#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"
#include "presentation/glossary.h"
#include <stdio.h>
#include <string.h>

static Font uiFont = {0};
static bool fontLoaded = false;

typedef struct {
    bool active;
    char title[GLOSSARY_TITLE_LEN];
    char text[GLOSSARY_TEXT_LEN];
    Vector2 anchor;
    float hoverTimer;
    char currentHoverKey[GLOSSARY_ID_LEN];
} TooltipManager;

static TooltipManager s_tooltip = {0};
static char s_hoveredKeyThisFrame[GLOSSARY_ID_LEN] = {0};

void UI_Init(void) {
    if (FileExists("assets/font.ttf")) {
        uiFont = LoadFontEx("assets/font.ttf", 32, 0, 250);
        SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
        fontLoaded = true;
    } else {
        uiFont = GetFontDefault();
    }

    /* Inicializar y validar Glosario (Sec B1) */
    if (Glossary_Init("assets/glossary.cfg")) {
        if (!Glossary_ValidateAll()) {
            TraceLog(LOG_ERROR, "[UI GLOSSARY] Faltan terminos obligatorios en assets/glossary.cfg");
        }
    }
}

void UI_Shutdown(void) {
    if (fontLoaded) {
        UnloadFont(uiFont);
        fontLoaded = false;
    }
    Glossary_Shutdown();
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

void UI_RequestTooltip(const char* title, const char* text, Vector2 anchorPos) {
    if (!title || !text) return;

    if (strcmp(s_tooltip.currentHoverKey, title) == 0) {
        s_tooltip.hoverTimer += GetFrameTime();
    } else {
        s_tooltip.hoverTimer = 0.0f;
        strncpy(s_tooltip.currentHoverKey, title, sizeof(s_tooltip.currentHoverKey) - 1);
        s_tooltip.currentHoverKey[sizeof(s_tooltip.currentHoverKey) - 1] = '\0';
    }

    strncpy(s_tooltip.title, title, sizeof(s_tooltip.title) - 1);
    s_tooltip.title[sizeof(s_tooltip.title) - 1] = '\0';

    strncpy(s_tooltip.text, text, sizeof(s_tooltip.text) - 1);
    s_tooltip.text[sizeof(s_tooltip.text) - 1] = '\0';

    s_tooltip.anchor = anchorPos;
    s_tooltip.active = true;

    strncpy(s_hoveredKeyThisFrame, title, sizeof(s_hoveredKeyThisFrame) - 1);
    s_hoveredKeyThisFrame[sizeof(s_hoveredKeyThisFrame) - 1] = '\0';
}

void UI_RequestGlossaryTooltip(const char* termId, Vector2 anchorPos) {
    if (!termId) return;
    const GlossaryEntry* entry = Glossary_Get(termId);
    if (entry) {
        UI_RequestTooltip(entry->title, entry->text, anchorPos);
    } else {
        UI_RequestTooltip(termId, "Termino pendiente de definir en glosario.", anchorPos);
    }
}

static void DrawWrappedText(const char* text, float x, float y, float maxWidth, float fontSize, Color color) {
    char buffer[GLOSSARY_TEXT_LEN];
    strncpy(buffer, text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char* word = strtok(buffer, " ");
    char line[128] = "";
    float curY = y;

    while (word) {
        char testLine[128];
        if (line[0] == '\0') {
            snprintf(testLine, sizeof(testLine), "%s", word);
        } else {
            snprintf(testLine, sizeof(testLine), "%s %s", line, word);
        }

        if (MeasureText(testLine, (int)fontSize) > maxWidth && line[0] != '\0') {
            DrawText(line, (int)x, (int)curY, (int)fontSize, color);
            curY += fontSize + 3.0f;
            snprintf(line, sizeof(line), "%s", word);
        } else {
            strncpy(line, testLine, sizeof(line) - 1);
            line[sizeof(line) - 1] = '\0';
        }
        word = strtok(NULL, " ");
    }
    if (line[0] != '\0') {
        DrawText(line, (int)x, (int)curY, (int)fontSize, color);
    }
}

static float MeasureWrappedTextHeight(const char* text, float maxWidth, float fontSize) {
    char buffer[GLOSSARY_TEXT_LEN];
    strncpy(buffer, text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char* word = strtok(buffer, " ");
    char line[128] = "";
    float lines = 1.0f;

    while (word) {
        char testLine[128];
        if (line[0] == '\0') {
            snprintf(testLine, sizeof(testLine), "%s", word);
        } else {
            snprintf(testLine, sizeof(testLine), "%s %s", line, word);
        }

        if (MeasureText(testLine, (int)fontSize) > maxWidth && line[0] != '\0') {
            lines += 1.0f;
            snprintf(line, sizeof(line), "%s", word);
        } else {
            strncpy(line, testLine, sizeof(line) - 1);
            line[sizeof(line) - 1] = '\0';
        }
        word = strtok(NULL, " ");
    }
    return lines * (fontSize + 3.0f);
}

void UI_DrawPendingTooltips(void) {
    if (s_hoveredKeyThisFrame[0] == '\0') {
        s_tooltip.hoverTimer = 0.0f;
        s_tooltip.currentHoverKey[0] = '\0';
        s_tooltip.active = false;
        return;
    }

    /* Retardo de activacion ~0.35s (Sec B1) */
    if (s_tooltip.active && s_tooltip.hoverTimer >= 0.35f) {
        float boxW = 280.0f;
        float bodyH = MeasureWrappedTextHeight(s_tooltip.text, boxW - 20.0f, 11.0f);
        float boxH = 34.0f + bodyH + 8.0f;

        float x = s_tooltip.anchor.x + 12.0f;
        float y = s_tooltip.anchor.y + 12.0f;

        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        /* Limites de pantalla */
        if (x + boxW > (float)(sw - 10)) x = s_tooltip.anchor.x - boxW - 8.0f;
        if (x < 10.0f) x = 10.0f;
        if (y + boxH > (float)(sh - 10)) y = s_tooltip.anchor.y - boxH - 8.0f;
        if (y < 10.0f) y = 10.0f;

        DrawRectangleRec((Rectangle){ x, y, boxW, boxH }, (Color){ 18, 18, 24, 248 });
        DrawRectangleLinesEx((Rectangle){ x, y, boxW, boxH }, 1.0f, (Color){ 200, 160, 60, 255 });

        DrawText(s_tooltip.title, (int)(x + 10.0f), (int)(y + 7.0f), 12, (Color){ 255, 215, 100, 255 });
        DrawLine((int)(x + 10.0f), (int)(y + 24.0f), (int)(x + boxW - 10.0f), (int)(y + 24.0f), (Color){ 140, 100, 50, 180 });

        DrawWrappedText(s_tooltip.text, x + 10.0f, y + 29.0f, boxW - 20.0f, 11.0f, (Color){ 220, 220, 230, 255 });
    }

    s_hoveredKeyThisFrame[0] = '\0';
}

Rectangle UI_DrawChipEx(const char* label, const char* termId, Rectangle bounds, Color bg, Color fg) {
    if (!label) label = termId ? termId : "";

    float fontSize = 11.0f;
    float textW = (float)MeasureText(label, (int)fontSize);

    if (bounds.width <= 0.0f) bounds.width = textW + 14.0f;
    if (bounds.height <= 0.0f) bounds.height = 19.0f;

    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);

    Color drawBg = hovered ? (Color){ (unsigned char)(bg.r + 25), (unsigned char)(bg.g + 25), (unsigned char)(bg.b + 25), bg.a } : bg;
    DrawRectangleRec(bounds, drawBg);
    DrawRectangleLinesEx(bounds, 1.0f, Fade(fg, hovered ? 1.0f : 0.6f));

    float tx = bounds.x + (bounds.width - textW) / 2.0f;
    float ty = bounds.y + (bounds.height - fontSize) / 2.0f;
    DrawText(label, (int)tx, (int)ty, (int)fontSize, fg);

    if (hovered && termId) {
        UI_RequestGlossaryTooltip(termId, mouse);
    }

    return bounds;
}

Rectangle UI_DrawChip(const char* termId, Rectangle bounds) {
    if (!termId) return bounds;
    const GlossaryEntry* entry = Glossary_Get(termId);
    const char* label = entry ? entry->title : termId;
    return UI_DrawChipEx(label, termId, bounds, (Color){ 35, 35, 45, 230 }, (Color){ 220, 200, 140, 255 });
}

