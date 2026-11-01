#include "views/event_view.h"
#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"
#include "presentation/audio_manager.h"
#include "raylib.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static Rectangle GetOptionRect(int index, int screenWidth, int screenHeight, int totalOptions) {
    int modalW = 780;
    int modalH = 540;
    int w = 710;
    int h = 54;
    int x = screenWidth / 2 - w / 2;
    int my = screenHeight / 2 - modalH / 2;
    int spacing = 64;
    int startY = my + modalH - (totalOptions * spacing) - 24;
    
    return (Rectangle){ (float)x, (float)(startY + index * spacing), (float)w, (float)h };
}

void EventView_Draw(const EventDef* event, int screenWidth, int screenHeight) {
    if (!event) return;

    int modalW = 780;
    int modalH = 540;
    int mx = screenWidth / 2 - modalW / 2;
    int my = screenHeight / 2 - modalH / 2;

    UI_DrawModalWindow((Rectangle){ (float)mx, (float)my, (float)modalW, (float)modalH }, event->name, NULL, UI_STYLE_PANEL_DARK);

    // Descripción amplia con excelente contraste en Marfil Luminoso
    UI_DrawTextWrapped(event->description, (Vector2){ (float)(mx + 45), (float)(my + 78) }, 19.0f, (float)(modalW - 90), (Color){ 248, 244, 232, 255 });

    // Renderizado de opciones mediante la API modular de botones 9-slice
    for (int i = 0; i < event->optionCount; i++) {
        Rectangle rect = GetOptionRect(i, screenWidth, screenHeight, event->optionCount);
        const EventOption* opt = &event->options[i];

        char reasonBuf[128] = {0};
        if (!opt->enabled && opt->disabledReason[0] != '\0') {
            snprintf(reasonBuf, sizeof(reasonBuf), "[%s]", opt->disabledReason);
        }

        UIButtonSpec spec = {
            .style = UI_STYLE_BTN_DEFAULT,
            .fontSize = 18.0f,
            .disabled = !opt->enabled,
            .subtext = (reasonBuf[0] != '\0') ? reasonBuf : NULL,
            .sfxOnClick = ""
        };
        UI_DrawButtonSpec(rect, opt->text, &spec);
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
                Audio_PlaySfx(SFX_EVENT_CHOICE);
                return i;
            }
        }
    }
    return -1;
}

void ResultView_Draw(const EventResult* res, int screenWidth, int screenHeight) {
    if (!res) return;

    int mw = 720;
    int mh = 420;
    int mx = (screenWidth - mw) / 2;
    int my = (screenHeight - mh) / 2;

    UI_DrawModalWindow((Rectangle){ (float)mx, (float)my, (float)mw, (float)mh }, res->name, NULL, UI_STYLE_PANEL_DARK);

    // Descripción del resultado
    UI_DrawTextWrapped(res->description, (Vector2){ (float)(mx + 35), (float)(my + 78) }, 19.0f, (float)(mw - 70), (Color){ 248, 244, 232, 255 });

    // Botón ACEPTAR
    int btnW = 320;
    int btnH = 50;
    int btnX = mx + (mw - btnW) / 2;
    int btnY = my + mh - 74;
    
    Rectangle btnRec = { (float)btnX, (float)btnY, (float)btnW, (float)btnH };
    UIButtonSpec spec = {
        .style = UI_STYLE_BTN_PRIMARY,
        .fontSize = 20.0f,
        .disabled = false,
        .sfxOnClick = ""
    };
    UI_DrawButtonSpec(btnRec, "ACEPTAR", &spec);
}

int ResultView_HandleInput(const EventResult* res, int screenWidth, int screenHeight) {
    if (!res) return -1;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int mw = 720;
        int mh = 420;
        int mx = (screenWidth - mw) / 2;
        int my = (screenHeight - mh) / 2;

        int btnW = 320;
        int btnH = 50;
        int btnX = mx + (mw - btnW) / 2;
        int btnY = my + mh - 74;
        
        Rectangle btnRec = { (float)btnX, (float)btnY, (float)btnW, (float)btnH };
        if (CheckCollisionPointRec(GetMousePosition(), btnRec)) {
            Audio_PlaySfx(SFX_UI_CONFIRM);
            return 0; // Aceptar clicked
        }
    }
    return -1;
}
