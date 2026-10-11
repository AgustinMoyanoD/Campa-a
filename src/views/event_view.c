#include "views/event_view.h"
#include "raylib.h"
#include <string.h>
#include <stdio.h>

static Rectangle GetOptionRect(int index, int screenWidth, int screenHeight, int totalOptions) {
    int w = 500;
    int h = 40;
    int x = screenWidth / 2 - w / 2;
    int startY = screenHeight / 2 + 20;
    int spacing = 50;
    
    return (Rectangle){ (float)x, (float)(startY + index * spacing), (float)w, (float)h };
}

void EventView_Draw(const EventDef* event, int screenWidth, int screenHeight) {
    if (!event) return;

    // Fondo oscurecido
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 0, 0, 0, 180 });

    // Pergamino
    int modalW = 600;
    int modalH = 400;
    int mx = screenWidth / 2 - modalW / 2;
    int my = screenHeight / 2 - modalH / 2;

    DrawRectangle(mx, my, modalW, modalH, (Color){ 240, 230, 210, 255 });
    DrawRectangleLines(mx, my, modalW, modalH, (Color){ 100, 80, 40, 255 });
    DrawRectangleLines(mx+2, my+2, modalW-4, modalH-4, (Color){ 100, 80, 40, 255 });

    // Titulo
    int titleW = MeasureText(event->name, 24);
    DrawText(event->name, screenWidth / 2 - titleW / 2, my + 30, 24, BLACK);
    DrawLine(mx + 50, my + 60, mx + modalW - 50, my + 60, GRAY);

    // Descripción (TODO: Word wrap)
    DrawText(event->description, mx + 40, my + 80, 18, DARKGRAY);

    // Opciones
    for (int i = 0; i < event->optionCount; i++) {
        Rectangle rect = GetOptionRect(i, screenWidth, screenHeight, event->optionCount);
        const EventOption* opt = &event->options[i];
        
        bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
        Color btnColor;
        Color textColor;
        if (!opt->enabled) {
            btnColor = (Color){ 180, 180, 180, 200 };
            textColor = (Color){ 100, 100, 100, 255 };
        } else {
            btnColor = hover ? (Color){ 200, 180, 150, 255 } : (Color){ 220, 200, 170, 255 };
            textColor = BLACK;
        }
        
        DrawRectangleRec(rect, btnColor);
        DrawRectangleLinesEx(rect, 1, (Color){ 120, 100, 60, 255 });

        char label[256];
        if (!opt->enabled && opt->disabledReason[0] != '\0') {
            snprintf(label, sizeof(label), "%s [%s]", opt->text, opt->disabledReason);
        } else {
            snprintf(label, sizeof(label), "%s", opt->text);
        }

        int textW = MeasureText(label, 15);
        DrawText(label, rect.x + rect.width / 2 - textW / 2, rect.y + 12, 15, textColor);
    }
}

int EventView_HandleInput(const EventDef* event, int screenWidth, int screenHeight) {
    if (!event) return -1;
    
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePos = GetMousePosition();
        for (int i = 0; i < event->optionCount; i++) {
            if (!event->options[i].enabled) continue;
            Rectangle rect = GetOptionRect(i, screenWidth, screenHeight, event->optionCount);
            if (CheckCollisionPointRec(mousePos, rect)) {
                return i;
            }
        }
    }
    return -1;
}



void ResultView_Draw(const EventResult* res, int screenWidth, int screenHeight) {
    if (!res) return;

    int mw = 600;
    int mh = 400;
    int mx = (screenWidth - mw) / 2;
    int my = (screenHeight - mh) / 2;

    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 0, 0, 0, 180 });
    DrawRectangle(mx, my, mw, mh, (Color){ 40, 35, 30, 255 });
    DrawRectangleLinesEx((Rectangle){ mx, my, mw, mh }, 4, (Color){ 180, 150, 80, 255 });

    DrawText(res->name, mx + 20, my + 20, 24, GOLD);

    DrawTextEx(GetFontDefault(), res->description, (Vector2){ mx + 20, my + 60 }, 18, 2, LIGHTGRAY);

    int btnW = 400;
    int btnH = 40;
    int btnX = mx + (mw - btnW) / 2;
    int btnY = my + mh - 60;
    
    Rectangle btnRec = { btnX, btnY, btnW, btnH };
    bool hover = CheckCollisionPointRec(GetMousePosition(), btnRec);
    
    DrawRectangleRec(btnRec, hover ? (Color){ 80, 70, 60, 255 } : (Color){ 60, 50, 40, 255 });
    DrawRectangleLinesEx(btnRec, 2, (Color){ 120, 100, 70, 255 });
    DrawText("ACEPTAR", btnX + btnW/2 - MeasureText("ACEPTAR", 18)/2, btnY + 11, 18, WHITE);
}

int ResultView_HandleInput(const EventResult* res, int screenWidth, int screenHeight) {
    if (!res) return -1;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int mw = 600;
        int mh = 400;
        int mx = (screenWidth - mw) / 2;
        int my = (screenHeight - mh) / 2;

        int btnW = 400;
        int btnH = 40;
        int btnX = mx + (mw - btnW) / 2;
        int btnY = my + mh - 60;
        
        Rectangle btnRec = { btnX, btnY, btnW, btnH };
        if (CheckCollisionPointRec(GetMousePosition(), btnRec)) {
            return 0; // Aceptar clicked
        }
    }
    return -1;
}

