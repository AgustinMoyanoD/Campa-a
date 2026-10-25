#include "raylib.h"
#include "presentation/ui_skin.h"
#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    InitWindow(1400, 900, "UI Skin & Modular Widgets Showcase");
    SetTargetFPS(60);
    UI_Init();

    RenderTexture2D target = LoadRenderTexture(1400, 900);
    BeginTextureMode(target);
    ClearBackground((Color){ 18, 16, 15, 255 });

    /* Título superior */
    UI_DrawText("UI SKIN PACK - API CENTRALIZADA DE WIDGETS Y 9-SLICE (assets/ui/skins.cfg)", (Vector2){ 24, 18 }, 22, (Color){ 238, 214, 146, 255 });

    /* 1. Botones — Estados (Idle, Hover, Pressed, Disabled) */
    UI_DrawText("Botones - estados (base 9-slice, esquinas 9x9, escala 2x)", (Vector2){ 24, 54 }, 15, (Color){ 185, 175, 155, 255 });
    const UIButtonStyleDef* defBtn = UISkin_GetButtonStyle(UI_STYLE_BTN_DEFAULT);
    const char* stateNames[4] = { "IDLE", "HOVER", "PRESSED", "DISABLED" };
    for (int st = 0; st < 4; st++) {
        Rectangle r = { 24.0f + st * 210.0f, 78.0f, 194.0f, 48.0f };
        int cell = (st == 2) ? 1 : ((st == 3) ? 2 : 0);
        UISkin_DrawNineSliceDefault(UI_SKIN_BUTTON, cell, r, defBtn->tint[st]);
        if (st == 1) {
            UISkin_DrawHighlight(r, 0.70f, 0);
        }
        Vector2 tsz = UI_MeasureText(stateNames[st], 18.0f);
        float yOff = (st == 2) ? 2.0f : 0.0f;
        UI_DrawText(stateNames[st], (Vector2){ r.x + (r.width - tsz.x) * 0.5f, r.y + (r.height - tsz.y) * 0.5f + yOff }, 18.0f, defBtn->text[st]);
    }

    /* 2. Botones — Estilos (Default, Primary, Danger, Curse, Flat) */
    UI_DrawText("Botones - estilos (UI_DrawButtonStyled / UI_DrawButtonSpec)", (Vector2){ 24, 142 }, 15, (Color){ 185, 175, 155, 255 });
    struct { const char* label; UIButtonStyle style; } styles[5] = {
        { "DEFAULT",   UI_STYLE_BTN_DEFAULT },
        { "CONFIRMAR", UI_STYLE_BTN_PRIMARY },
        { "SACRIFICAR",UI_STYLE_BTN_DANGER  },
        { "PACTO",     UI_STYLE_BTN_CURSE   },
        { "Secundario",UI_STYLE_BTN_FLAT    }
    };
    for (int i = 0; i < 5; i++) {
        Rectangle r = { 24.0f + i * 195.0f, 166.0f, 180.0f, 46.0f };
        UI_DrawButtonStyled(r, styles[i].label, styles[i].style, false);
    }

    /* 3. Overlays Aditivos (Glow) y Decals de Rotura (Cracks 1..3) */
    UI_DrawText("Overlays aditivos (Glow / Highlight) y decals de rotura (crackStage 1..3)", (Vector2){ 24, 228 }, 15, (Color){ 185, 175, 155, 255 });
    {
        Rectangle rGlow = { 24.0f, 252.0f, 185.0f, 46.0f };
        UIButtonSpec spGlow = { .style = UI_STYLE_BTN_DEFAULT, .selected = true };
        UI_DrawButtonSpec(rGlow, "ELEGIDO", &spGlow);

        const char* crackLabels[3] = { "DANO I", "DANO II", "ROTO" };
        for (int c = 0; c < 3; c++) {
            Rectangle rc = { 225.0f + c * 195.0f, 252.0f, 180.0f, 46.0f };
            UIButtonSpec spCrack = {
                .style = (c == 2) ? UI_STYLE_BTN_DANGER : UI_STYLE_BTN_DEFAULT,
                .crackStage = c + 1
            };
            UI_DrawButtonSpec(rc, crackLabels[c], &spCrack);
        }
    }

    /* 4. Panel Oscuro (panel_dark) con Barras 9-Slice y Ranuras (Slots) */
    UI_DrawText("panel_dark (esquinas 12x12) + barras 9-slice + slots hundidos", (Vector2){ 24, 316 }, 15, (Color){ 185, 175, 155, 255 });
    Rectangle darkPanel = { 24.0f, 340.0f, 430.0f, 310.0f };
    UI_DrawPanelStyled(darkPanel, UI_STYLE_PANEL_DARK);
    UI_DrawText("SOLDADOS (51)", (Vector2){ darkPanel.x + 18, darkPanel.y + 16 }, 20, (Color){ 238, 214, 146, 255 });

    UI_DrawLabeledBar((Rectangle){ darkPanel.x + 18, darkPanel.y + 52, 394, 26 }, 0.85f, "Moral 85%", (Color){ 216, 174, 72, 255 }, UI_ICON_MORAL);
    UI_DrawLabeledBar((Rectangle){ darkPanel.x + 18, darkPanel.y + 88, 394, 26 }, 0.62f, "Salud 62%", (Color){ 82, 184, 92, 255 }, UI_ICON_SALUD);
    UI_DrawLabeledBar((Rectangle){ darkPanel.x + 18, darkPanel.y + 124, 394, 26 }, 0.25f, "Satisfaccion 2/8", (Color){ 206, 68, 62, 255 }, UI_ICON_SATISFACCION);

    UIIconId slotIcons[4] = { UI_ICON_AGUA, UI_ICON_VINO, UI_ICON_COMIDA, UI_ICON_CARNE };
    for (int s = 0; s < 4; s++) {
        Rectangle sr = { darkPanel.x + 18.0f + s * 62.0f, darkPanel.y + 170.0f, 52.0f, 52.0f };
        UI_DrawSlot(sr, WHITE, slotIcons[s]);
    }

    UI_DrawButtonStyled((Rectangle){ darkPanel.x + 18, darkPanel.y + 244, 190, 44 }, "REPARTIR", UI_STYLE_BTN_PRIMARY, false);
    UI_DrawButtonStyled((Rectangle){ darkPanel.x + 222, darkPanel.y + 244, 190, 44 }, "OMITIR", UI_STYLE_BTN_DEFAULT, false);

    /* 5. Panel Pergamino (panel_parchment) */
    UI_DrawText("panel_parchment (marco de madera + pergamino)", (Vector2){ 480, 316 }, 15, (Color){ 185, 175, 155, 255 });
    Rectangle parchPanel = { 480.0f, 340.0f, 430.0f, 310.0f };
    UI_DrawPanelStyled(parchPanel, UI_STYLE_PANEL_PARCHMENT);
    const UIPanelStyleDef* pStyle = UISkin_GetPanelStyle(UI_STYLE_PANEL_PARCHMENT);
    UI_DrawText("ORDEN DEL DIA", (Vector2){ parchPanel.x + 22, parchPanel.y + 20 }, 20, pStyle->textTitle);
    UI_DrawText("Los sargentos exigen vino", (Vector2){ parchPanel.x + 22, parchPanel.y + 52 }, 16, pStyle->textMain);
    UI_DrawText("antes de marchar al desfiladero.", (Vector2){ parchPanel.x + 22, parchPanel.y + 74 }, 16, pStyle->textMain);

    struct { UIIconId icon; const char* text; } reqs[4] = {
        { UI_ICON_AGUA,   "Agua: 51 / 51 u" },
        { UI_ICON_VINO,   "Vino: 8 / 8 u" },
        { UI_ICON_COMIDA, "Comida: 51 / 51 u" },
        { UI_ICON_CARNE,  "Carne: 0 / 8 u" }
    };
    for (int i = 0; i < 4; i++) {
        float ry = parchPanel.y + 114.0f + i * 32.0f;
        UI_DrawIcon(reqs[i].icon, (Vector2){ parchPanel.x + 26, ry }, 24.0f, WHITE);
        UI_DrawText(reqs[i].text, (Vector2){ parchPanel.x + 60, ry + 3.0f }, 16, pStyle->textMain);
    }
    UI_DrawButtonStyled((Rectangle){ parchPanel.x + 22, parchPanel.y + 248, 200, 42 }, "FIRMAR", UI_STYLE_BTN_DEFAULT, false);

    /* 6. Tooltip y Chips Semánticos */
    UI_DrawText("tooltip + chips (UI_DrawStatusChip)", (Vector2){ 936, 316 }, 15, (Color){ 185, 175, 155, 255 });
    Rectangle tipRect = { 936.0f, 340.0f, 430.0f, 125.0f };
    UI_DrawPanelStyled(tipRect, UI_STYLE_PANEL_TOOLTIP);
    UI_DrawIcon(UI_ICON_AVISO, (Vector2){ tipRect.x + 16, tipRect.y + 16 }, 24.0f, WHITE);
    UI_DrawText("RIESGO DE DESERCION", (Vector2){ tipRect.x + 48, tipRect.y + 18 }, 18, (Color){ 245, 205, 110, 255 });
    UI_DrawText("2 dias sin racion completa.", (Vector2){ tipRect.x + 18, tipRect.y + 52 }, 15, (Color){ 210, 200, 180, 255 });
    UI_DrawText("Moral proyectada: -15 manana.", (Vector2){ tipRect.x + 18, tipRect.y + 76 }, 15, (Color){ 235, 130, 120, 255 });

    UI_DrawStatusChip((Rectangle){ 936.0f, 482.0f, 240.0f, 34.0f }, "RACION COMPLETA", UI_STYLE_CHIP_OK,   UI_ICON_OK);
    UI_DrawStatusChip((Rectangle){ 936.0f, 526.0f, 240.0f, 34.0f }, "RACION REDUCIDA", UI_STYLE_CHIP_WARN, UI_ICON_AVISO);
    UI_DrawStatusChip((Rectangle){ 936.0f, 570.0f, 240.0f, 34.0f }, "HAMBRUNA",        UI_STYLE_CHIP_BAD,  UI_ICON_PELIGRO);
    UI_DrawStatusChip((Rectangle){ 936.0f, 614.0f, 240.0f, 34.0f }, "GLOSARIO / INFO", UI_STYLE_CHIP_NEUTRAL, UI_ICON_NONE);

    /* 7. Catálogo de Iconos 16x16 (escalados a 32x32) */
    UI_DrawText("Iconos 16x16 (escalados x2 = 32x32 en slots)", (Vector2){ 24, 674 }, 15, (Color){ 185, 175, 155, 255 });
    const char* iconLabels[11] = {
        "agua", "vino", "comida", "carne", "salud", "moral",
        "satisf.", "ganado", "aviso", "ok", "peligro"
    };
    for (int ic = 1; ic < UI_ICON_COUNT; ic++) {
        int idx = ic - 1;
        Rectangle slotR = { 24.0f + idx * 120.0f, 704.0f, 56.0f, 56.0f };
        UI_DrawSlot(slotR, WHITE, (UIIconId)ic);
        UI_DrawText(iconLabels[idx], (Vector2){ slotR.x, slotR.y + 62.0f }, 14, (Color){ 185, 175, 155, 255 });
    }

    EndTextureMode();

    Image img = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&img);
    ExportImage(img, "ui_skin_showcase.png");
    ExportImage(img, "C:/Users/exaul/.gemini/antigravity/brain/1ec680a5-a412-4783-8263-ef76f34df73d/ui_skin_showcase.png");
    UnloadImage(img);
    UnloadRenderTexture(target);

    UI_Shutdown();
    CloseWindow();
    printf("[OK] ui_skin_showcase.png generado exitosamente.\n");
    return 0;
}
