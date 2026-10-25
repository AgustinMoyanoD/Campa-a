#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"
#include "presentation/glossary.h"
#include "presentation/audio_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

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

/* ============================================================
   CICLO DE VIDA: INIT / SHUTDOWN
   ============================================================ */

void UI_Init(void) {
    if (IsWindowReady()) {
        const char* fontPath = "assets/font.ttf";
        if (!FileExists(fontPath)) {
            fontPath = TextFormat("%sassets/font.ttf", GetApplicationDirectory());
        }

        if (FileExists(fontPath)) {
            int codepoints[300];
            int count = 0;
            for (int c = 32; c <= 126; c++) codepoints[count++] = c;
            for (int c = 160; c <= 255; c++) codepoints[count++] = c;
            uiFont = LoadFontEx(fontPath, 38, codepoints, count);
            SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
            fontLoaded = true;
            TraceLog(LOG_INFO, "[UI] Fuente medieval cargada exitosamente desde: %s (%d glyphs)", fontPath, uiFont.glyphCount);
        } else {
            uiFont = GetFontDefault();
            TraceLog(LOG_WARNING, "[UI] No se encontro assets/font.ttf; usando fuente default.");
        }
    }

    /* Inicializar y validar Glosario (Sec B1) */
    const char* cfgPath = "assets/glossary.cfg";
    if (!FileExists(cfgPath) && IsWindowReady()) {
        cfgPath = TextFormat("%sassets/glossary.cfg", GetApplicationDirectory());
    }
    if (Glossary_Init(cfgPath)) {
        if (!Glossary_ValidateAll()) {
            TraceLog(LOG_ERROR, "[UI GLOSSARY] Faltan terminos obligatorios en assets/glossary.cfg");
        }
    }

    /* Inicializar motor de Skins 9-slice, estilos e iconos */
    UISkin_Init("assets/ui/skins.cfg");

    /* Inicializar marcos de cartas divinas y maldiciones */
    UICards_Init();
}

void UI_Shutdown(void) {
    if (fontLoaded && IsWindowReady()) {
        UnloadFont(uiFont);
        fontLoaded = false;
    }
    UICards_Shutdown();
    UISkin_Shutdown();
    Glossary_Shutdown();
}

/* ============================================================
   TIPOGRAFÍA Y MEDICIÓN DE TEXTO
   ============================================================ */

Font UI_GetFont(void) {
    return uiFont;
}

Vector2 UI_MeasureText(const char* text, float fontSize) {
    if (!text) return (Vector2){0, 0};
    return MeasureTextEx(uiFont, text, fontSize, 1.0f);
}

void UI_DrawText(const char* text, Vector2 pos, float fontSize, Color color) {
    if (!text) return;
    DrawTextEx(uiFont, text, (Vector2){ roundf(pos.x), roundf(pos.y) }, fontSize, 1.0f, color);
}

void UI_DrawTextWrappedMaxH(const char* text, Vector2 pos, float fontSize, float maxWidth, float maxHeight, Color color) {
    if (!text || maxWidth <= 0.0f) return;

    char buffer[2048];
    strncpy(buffer, text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char* word = strtok(buffer, " ");
    char currentLine[512] = {0};
    float curY = pos.y;
    float lineHeight = fontSize * 1.22f;

    while (word != NULL) {
        char testLine[512];
        if (strlen(currentLine) == 0) {
            snprintf(testLine, sizeof(testLine), "%s", word);
        } else {
            size_t curLen = strlen(currentLine);
            snprintf(testLine, sizeof(testLine), "%s", currentLine);
            if (curLen + 1 < sizeof(testLine)) {
                snprintf(testLine + curLen, sizeof(testLine) - curLen, " %s", word);
            }
        }

        Vector2 sz = MeasureTextEx(uiFont, testLine, fontSize, 1.0f);
        if (sz.x > maxWidth && strlen(currentLine) > 0) {
            if (maxHeight > 0.0f && (curY + lineHeight > pos.y + maxHeight)) {
                break;
            }
            UI_DrawText(currentLine, (Vector2){ pos.x, curY }, fontSize, color);
            curY += lineHeight;
            snprintf(currentLine, sizeof(currentLine), "%s", word);
        } else {
            snprintf(currentLine, sizeof(currentLine), "%s", testLine);
        }
        word = strtok(NULL, " ");
    }

    if (strlen(currentLine) > 0) {
        if (maxHeight <= 0.0f || (curY + fontSize <= pos.y + maxHeight)) {
            UI_DrawText(currentLine, (Vector2){ pos.x, curY }, fontSize, color);
        }
    }
}

void UI_DrawTextWrapped(const char* text, Vector2 pos, float fontSize, float maxWidth, Color color) {
    UI_DrawTextWrappedMaxH(text, pos, fontSize, maxWidth, 0.0f, color);
}

/* ============================================================
   PANELES, CONTENEDORES, RANURAS (SLOTS) Y PANTALLAS MODALES
   ============================================================ */

void UI_DrawPanelStyled(Rectangle bounds, UIPanelStyle style) {
    const UIPanelStyleDef* ps = UISkin_GetPanelStyle(style);
    UISkin_DrawNineSliceDefault(ps->skin, 0, bounds, ps->tint);
}

void UI_DrawPanelTinted(Rectangle bounds, UISkinId skin, Color tint) {
    UISkin_DrawNineSliceDefault(skin, 0, bounds, tint);
}

void UI_DrawPanel(Rectangle bounds) {
    UI_DrawPanelStyled(bounds, UI_STYLE_PANEL_DARK);
}

void UI_DrawSlot(Rectangle bounds, Color tint, UIIconId icon) {
    if (tint.a == 0) {
        tint = UISkin_GetPanelStyle(UI_STYLE_PANEL_SLOT)->tint;
    }
    int sc = (bounds.height < 28.0f || bounds.width < 28.0f) ? 1 : UISkin_GetPixelScale();
    UISkin_DrawNineSlice(UI_SKIN_SLOT, 0, bounds, sc, tint);
    if (icon != UI_ICON_NONE) {
        UISkin_DrawIconCentered(icon, bounds, sc, WHITE);
    }
}

void UI_DrawModalWindow(Rectangle modalBounds, const char* title, const char* subtitle, UIPanelStyle style) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, (Color){ 10, 12, 16, 215 });

    UI_DrawPanelStyled(modalBounds, style);

    const UIPanelStyleDef* ps = UISkin_GetPanelStyle(style);
    Color titleCol = (style == UI_STYLE_PANEL_PARCHMENT) ? ps->textTitle : UI_COLOR_TEXT_TITLE;
    Color subCol   = (style == UI_STYLE_PANEL_PARCHMENT) ? ps->textMain  : UI_COLOR_TEXT_DIM;

    float curY = modalBounds.y + 18.0f;
    if (title && title[0] != '\0') {
        float titleFont = 24.0f;
        Vector2 tSz = UI_MeasureText(title, titleFont);
        UI_DrawText(title, (Vector2){ modalBounds.x + (modalBounds.width - tSz.x) / 2.0f, curY }, titleFont, titleCol);
        curY += titleFont + 4.0f;
    }

    if (subtitle && subtitle[0] != '\0') {
        float subFont = 13.0f;
        Vector2 sSz = UI_MeasureText(subtitle, subFont);
        UI_DrawText(subtitle, (Vector2){ modalBounds.x + (modalBounds.width - sSz.x) / 2.0f, curY }, subFont, subCol);
        curY += subFont + 6.0f;
    }

    if ((title && title[0] != '\0') || (subtitle && subtitle[0] != '\0')) {
        DrawLine((int)(modalBounds.x + 28.0f), (int)curY, (int)(modalBounds.x + modalBounds.width - 28.0f), (int)curY, Fade(ps->tint, 0.55f));
    }
}

void UI_DrawIcon(UIIconId icon, Vector2 pos, float size, Color tint) {
    UISkin_DrawIcon(icon, pos, size, tint);
}

/* ============================================================
   BOTONES MODULARES (ESTADOS, ESTILOS, GLOW, HIGHLIGHT, GRIETAS)
   ============================================================ */

static UIButtonStyle InferStyleFromColor(Color bg, Color border) {
    /* Heurística semántica para convertir llamadas heredadas UI_DrawButtonEx al estilo 9-slice apropiado */
    if (bg.g > bg.r + 25 && bg.g > bg.b + 15) {
        return UI_STYLE_BTN_PRIMARY; /* Botón verde (Confirmar) */
    }
    if (bg.r > bg.g + 40 && bg.r > bg.b + 30) {
        return UI_STYLE_BTN_DANGER;  /* Botón rojo/terracota (Sacrificar / Peligro) */
    }
    if (bg.b > bg.g + 35 && bg.r > bg.g + 20) {
        return UI_STYLE_BTN_CURSE;   /* Botón púrpura (Maldición) */
    }
    (void)border;
    return UI_STYLE_BTN_DEFAULT;
}

bool UI_DrawButtonSpec(Rectangle bounds, const char* text, const UIButtonSpec* spec) {
    UIButtonSpec defaultSpec = {
        .style = UI_STYLE_BTN_DEFAULT,
        .fontSize = 0.0f,
        .disabled = false,
        .selected = false,
        .crackStage = 0,
        .icon = UI_ICON_NONE,
        .subtext = NULL,
        .sfxOnClick = "ui_click",
        .customTint = {0, 0, 0, 0},
        .customText = {0, 0, 0, 0}
    };
    const UIButtonSpec* s = spec ? spec : &defaultSpec;

    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);
    bool clicked = false;

    static Rectangle s_lastHoveredRect = { -9999, -9999, 0, 0 };
    if (hovered && !s->disabled) {
        if (fabsf(bounds.x - s_lastHoveredRect.x) > 0.5f ||
            fabsf(bounds.y - s_lastHoveredRect.y) > 0.5f ||
            fabsf(bounds.width - s_lastHoveredRect.width) > 0.5f ||
            fabsf(bounds.height - s_lastHoveredRect.height) > 0.5f) {
            Audio_PlaySfx(SFX_UI_HOVER);
            s_lastHoveredRect = bounds;
        }
    } else if (s_lastHoveredRect.width > 0 && !CheckCollisionPointRec(mouse, s_lastHoveredRect)) {
        s_lastHoveredRect = (Rectangle){ -9999, -9999, 0, 0 };
    }

    UIButtonState btnState = UI_BTN_STATE_IDLE;
    if (s->disabled) {
        btnState = UI_BTN_STATE_DISABLED;
    } else if (hovered) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            btnState = UI_BTN_STATE_PRESSED;
        } else {
            btnState = UI_BTN_STATE_HOVER;
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            clicked = true;
            if (s->sfxOnClick == NULL) {
                Audio_PlaySfxByName("ui_click");
            } else if (s->sfxOnClick[0] != '\0') {
                Audio_PlaySfxByName(s->sfxOnClick);
            }
        }
    }

    const UIButtonStyleDef* styleDef = UISkin_GetButtonStyle(s->style);
    Color tintCol = (s->customTint.a > 0) ? s->customTint : styleDef->tint[btnState];
    Color textCol = (s->customText.a > 0) ? s->customText : styleDef->text[btnState];

    if (s->customTint.a > 0) {
        if (btnState == UI_BTN_STATE_DISABLED) {
            tintCol = (Color){ 130, 130, 130, 255 };
        } else if (btnState == UI_BTN_STATE_HOVER) {
            tintCol = (Color){
                (unsigned char)fminf(255.0f, (float)tintCol.r + 25.0f),
                (unsigned char)fminf(255.0f, (float)tintCol.g + 25.0f),
                (unsigned char)fminf(255.0f, (float)tintCol.b + 25.0f),
                tintCol.a
            };
        } else if (btnState == UI_BTN_STATE_PRESSED) {
            tintCol = (Color){
                (unsigned char)fmaxf(0.0f, (float)tintCol.r - 30.0f),
                (unsigned char)fmaxf(0.0f, (float)tintCol.g - 30.0f),
                (unsigned char)fmaxf(0.0f, (float)tintCol.b - 30.0f),
                tintCol.a
            };
        }
    }

    int btnScale = (bounds.height < 30.0f || bounds.width < 30.0f) ? 1 : UISkin_GetPixelScale();

    /* Resplandor exterior (Glow) cuando está seleccionado / elegido */
    if (s->selected && !s->disabled) {
        UISkin_DrawGlow(bounds, (Color){ 255, 220, 140, 255 }, 0.95f, btnScale);
    }

    /* Dibujado del marco 9-slice o modo plano (sin marco) */
    if (styleDef->hasFrame) {
        UISkin_DrawNineSlice(styleDef->skin, (int)btnState, bounds, btnScale, tintCol);

        /* Brillo interior aditivo en hover o seleccionado */
        if (s->selected && !s->disabled) {
            UISkin_DrawHighlight(bounds, 0.85f, btnScale);
        } else if (btnState == UI_BTN_STATE_HOVER) {
            UISkin_DrawHighlight(bounds, 0.35f, btnScale);
        }

        /* Decal de grietas si tiene etapa de rotura (1..3) */
        if (s->crackStage >= 1 && s->crackStage <= 3) {
            UISkin_DrawCrack(bounds, s->crackStage, btnScale);
        }
    } else {
        /* Botón plano (sin marco): subrayado sutil */
        float lineY = roundf(bounds.y + bounds.height - 4.0f);
        DrawLine((int)(bounds.x + 4.0f), (int)lineY, (int)(bounds.x + bounds.width - 4.0f), (int)lineY,
                 (btnState == UI_BTN_STATE_HOVER) ? UI_COLOR_GOLD : (Color){ 120, 105, 75, 255 });
    }

    /* Desplazamiento táctil +1px al presionar */
    float pressOffsetY = (btnState == UI_BTN_STATE_PRESSED) ? (float)btnScale : 0.0f;

    float fontSize = s->fontSize;
    if (fontSize <= 0.0f) {
        fontSize = (bounds.height < 34.0f) ? (bounds.height * 0.50f) : (bounds.height >= 46.0f ? 19.0f : 16.0f);
    }

    /* Dibujado de icono + texto (con soporte para subtexto en 2da línea) */
    float iconSpace = 0.0f;
    float iconSize = (bounds.height < 34.0f) ? 16.0f : 24.0f;
    if (s->icon != UI_ICON_NONE) {
        iconSpace = iconSize + 6.0f;
    }

    if (text && text[0] != '\0') {
        if (s->subtext && s->subtext[0] != '\0') {
            float f1 = fontSize;
            Vector2 sz1 = UI_MeasureText(text, f1);
            if (sz1.x > bounds.width - 20.0f) {
                f1 = fmaxf(13.0f, f1 - 2.0f);
                sz1 = UI_MeasureText(text, f1);
            }
            float f2 = fmaxf(12.0f, f1 - 2.0f);
            Vector2 sz2 = UI_MeasureText(s->subtext, f2);

            float totalH = sz1.y + sz2.y + 2.0f;
            float y1 = bounds.y + (bounds.height - totalH) / 2.0f + pressOffsetY;
            float y2 = y1 + sz1.y + 2.0f;

            float x1 = bounds.x + (bounds.width - (sz1.x + iconSpace)) / 2.0f;
            if (s->icon != UI_ICON_NONE) {
                UISkin_DrawIcon(s->icon, (Vector2){ x1, bounds.y + (bounds.height - iconSize) / 2.0f + pressOffsetY }, iconSize, WHITE);
            }
            UI_DrawText(text, (Vector2){ x1 + iconSpace, y1 }, f1, textCol);

            Color subCol = s->disabled ? (Color){ 220, 120, 120, 255 } : UI_COLOR_TEXT_DIM;
            UI_DrawText(s->subtext, (Vector2){ bounds.x + (bounds.width - sz2.x) / 2.0f, y2 }, f2, subCol);
        } else {
            Vector2 tSize = UI_MeasureText(text, fontSize);
            /* Si el texto excede el ancho del botón y el botón es alto (>= 46px), dividir en 2 líneas */
            if (tSize.x > bounds.width - iconSpace - 20.0f && bounds.height >= 46.0f) {
                int len = (int)strlen(text);
                int splitIdx = -1;
                const char* paren = strchr(text, '(');
                if (paren && (paren - text) > 8 && (paren - text) < len - 4) {
                    splitIdx = (int)(paren - text);
                } else {
                    int mid = len / 2;
                    int bestDist = 999;
                    for (int i = 0; i < len; i++) {
                        if (text[i] == ' ') {
                            int dist = abs(i - mid);
                            if (dist < bestDist) {
                                bestDist = dist;
                                splitIdx = i;
                            }
                        }
                    }
                }

                if (splitIdx > 0 && splitIdx < len) {
                    char l1[256], l2[256];
                    int l1Len = splitIdx;
                    while (l1Len > 0 && text[l1Len - 1] == ' ') l1Len--;
                    if (l1Len >= (int)sizeof(l1)) l1Len = (int)sizeof(l1) - 1;
                    memcpy(l1, text, l1Len);
                    l1[l1Len] = '\0';

                    const char* p2 = text + splitIdx;
                    while (*p2 == ' ') p2++;
                    snprintf(l2, sizeof(l2), "%s", p2);

                    float fSplit = fmaxf(14.0f, fontSize - 2.0f);
                    Vector2 sz1 = UI_MeasureText(l1, fSplit);
                    Vector2 sz2 = UI_MeasureText(l2, fSplit);
                    float totalH = sz1.y + sz2.y + 2.0f;
                    float y1 = bounds.y + (bounds.height - totalH) / 2.0f + pressOffsetY;
                    UI_DrawText(l1, (Vector2){ bounds.x + (bounds.width - sz1.x) / 2.0f, y1 }, fSplit, textCol);
                    UI_DrawText(l2, (Vector2){ bounds.x + (bounds.width - sz2.x) / 2.0f, y1 + sz1.y + 2.0f }, fSplit, textCol);
                    return clicked;
                }
            }

            float fitFont = fontSize;
            while (tSize.x > bounds.width - iconSpace - 10.0f && fitFont > 11.0f) {
                fitFont -= 1.0f;
                tSize = UI_MeasureText(text, fitFont);
            }

            float tx = bounds.x + (bounds.width - (tSize.x + iconSpace)) / 2.0f;
            float ty = bounds.y + (bounds.height - tSize.y) / 2.0f + pressOffsetY;
            if (s->icon != UI_ICON_NONE) {
                UISkin_DrawIcon(s->icon, (Vector2){ tx, bounds.y + (bounds.height - iconSize) / 2.0f + pressOffsetY }, iconSize, WHITE);
            }
            UI_DrawText(text, (Vector2){ tx + iconSpace, ty }, fitFont, textCol);
        }
    }

    return clicked;
}

bool UI_DrawButtonStyledSfx(Rectangle bounds, const char* text, UIButtonStyle style, bool disabled, const char* sfxOnClick) {
    UIButtonSpec spec = {
        .style = style,
        .fontSize = 0.0f,
        .disabled = disabled,
        .sfxOnClick = sfxOnClick
    };
    return UI_DrawButtonSpec(bounds, text, &spec);
}

bool UI_DrawButtonStyled(Rectangle bounds, const char* text, UIButtonStyle style, bool disabled) {
    return UI_DrawButtonStyledSfx(bounds, text, style, disabled, "ui_click");
}

bool UI_DrawButtonExSfx(Rectangle bounds, const char* text, float fontSize, bool disabled, Color bg, Color fg, Color border, const char* sfxOnClick) {
    UIButtonStyle inferred = InferStyleFromColor(bg, border);
    UIButtonSpec spec = {
        .style = inferred,
        .fontSize = fontSize,
        .disabled = disabled,
        .sfxOnClick = sfxOnClick
    };
    (void)fg;
    return UI_DrawButtonSpec(bounds, text, &spec);
}

bool UI_DrawButtonEx(Rectangle bounds, const char* text, float fontSize, bool disabled, Color bg, Color fg, Color border) {
    return UI_DrawButtonExSfx(bounds, text, fontSize, disabled, bg, fg, border, "ui_click");
}

bool UI_DrawButtonSfx(Rectangle bounds, const char* text, bool disabled, const char* sfxOnClick) {
    return UI_DrawButtonStyledSfx(bounds, text, UI_STYLE_BTN_DEFAULT, disabled, sfxOnClick);
}

bool UI_DrawButton(Rectangle bounds, const char* text, bool disabled) {
    return UI_DrawButtonStyledSfx(bounds, text, UI_STYLE_BTN_DEFAULT, disabled, "ui_click");
}

bool UI_ButtonClicked(Rectangle bounds) {
    Vector2 mouse = GetMousePosition();
    return CheckCollisionPointRec(mouse, bounds) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
}

/* ============================================================
   BARRAS DE PROGRESO (9-SLICE + TEXTO E ICONO)
   ============================================================ */

void UI_DrawBar(Rectangle bounds, float percentage, Color fg, Color bg) {
    (void)bg;
    UISkin_DrawBar(bounds, percentage, fg, (Color){ 200, 190, 170, 255 }, 0);
}

void UI_DrawLabeledBar(Rectangle bounds, float percentage, const char* label, Color fillTint, UIIconId icon) {
    UISkin_DrawBar(bounds, percentage, fillTint, (Color){ 200, 190, 170, 255 }, 0);

    float textX = bounds.x + 8.0f;
    if (icon != UI_ICON_NONE) {
        float iconSz = fminf(16.0f, bounds.height - 2.0f);
        UISkin_DrawIcon(icon, (Vector2){ bounds.x + 5.0f, bounds.y + (bounds.height - iconSz) / 2.0f }, iconSz, WHITE);
        textX = bounds.x + iconSz + 9.0f;
    }

    if (label && label[0] != '\0') {
        float fontSz = (bounds.height >= 20.0f) ? 13.0f : 12.0f;
        Vector2 sz = UI_MeasureText(label, fontSz);
        float textY = bounds.y + (bounds.height - sz.y) / 2.0f;
        /* Sombra sutil de 1px para garantizar legibilidad sobre cualquier color de barra */
        UI_DrawText(label, (Vector2){ textX + 1.0f, textY + 1.0f }, fontSz, (Color){ 15, 15, 20, 220 });
        UI_DrawText(label, (Vector2){ textX, textY }, fontSz, (Color){ 248, 245, 235, 255 });
    }
}

/* ============================================================
   CHIPS DE ESTADO Y GLOSARIO (9-SLICE UI_SKIN_CHIP)
   ============================================================ */

Rectangle UI_DrawStatusChip(Rectangle bounds, const char* label, UIChipStyle style, UIIconId icon) {
    if (!label) label = "";
    const UIChipStyleDef* cs = UISkin_GetChipStyle(style);

    float fontSize = (bounds.height >= 26.0f) ? 13.0f : 12.0f;
    Vector2 textSz = UI_MeasureText(label, fontSize);
    float iconW = (icon != UI_ICON_NONE) ? 20.0f : 0.0f;

    if (bounds.width <= 0.0f) {
        bounds.width = textSz.x + iconW + 18.0f;
    } else {
        while ((textSz.x + iconW > bounds.width - 14.0f) && fontSize > 8.0f) {
            fontSize -= 0.5f;
            textSz = UI_MeasureText(label, fontSize);
        }
    }
    if (bounds.height <= 0.0f) bounds.height = 24.0f;

    int chipScale = (bounds.height < 24.0f) ? 1 : UISkin_GetPixelScale();
    UISkin_DrawNineSlice(cs->skin, 0, bounds, chipScale, cs->tint);

    float totalContentW = textSz.x + iconW;
    float startX = bounds.x + (bounds.width - totalContentW) / 2.0f;

    if (icon != UI_ICON_NONE) {
        UISkin_DrawIcon(icon, (Vector2){ startX, bounds.y + (bounds.height - 16.0f) / 2.0f }, 16.0f, WHITE);
        startX += iconW;
    }

    float ty = bounds.y + (bounds.height - textSz.y) / 2.0f;
    UI_DrawText(label, (Vector2){ startX + 1.0f, ty + 1.0f }, fontSize, (Color){ 15, 15, 20, 200 });
    UI_DrawText(label, (Vector2){ startX, ty }, fontSize, cs->text);

    return bounds;
}

Rectangle UI_DrawChipEx(const char* label, const char* termId, Rectangle bounds, Color bg, Color fg) {
    if (!label) label = termId ? termId : "";

    float fontSize = 12.0f;
    Vector2 textSz = UI_MeasureText(label, fontSize);
    float textW = textSz.x;

    if (bounds.width <= 0.0f) bounds.width = textW + 16.0f;
    if (bounds.height <= 0.0f) bounds.height = 22.0f;

    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);

    Color chipTint = (bg.r < 60 && bg.g < 60 && bg.b < 60) ? fg : bg;
    int chipScale = (bounds.height < 24.0f) ? 1 : UISkin_GetPixelScale();
    UISkin_DrawNineSlice(UI_SKIN_CHIP, 0, bounds, chipScale, chipTint);

    if (hovered) {
        UISkin_DrawHighlight(bounds, 0.35f, chipScale);
    }

    float tx = bounds.x + (bounds.width - textW) / 2.0f;
    float ty = bounds.y + (bounds.height - textSz.y) / 2.0f;
    UI_DrawText(label, (Vector2){ tx, ty }, fontSize, (Color){ 245, 240, 230, 255 });

    if (hovered && termId) {
        UI_RequestGlossaryTooltip(termId, mouse);
    }

    return bounds;
}

Rectangle UI_DrawChip(const char* termId, Rectangle bounds) {
    if (!termId) return bounds;
    const GlossaryEntry* entry = Glossary_Get(termId);
    const char* label = entry ? entry->title : termId;
    return UI_DrawChipEx(label, termId, bounds, (Color){ 185, 165, 125, 255 }, (Color){ 242, 232, 205, 255 });
}

/* ============================================================
   TOOLTIPS Y GLOSARIO (9-SLICE UI_SKIN_TOOLTIP)
   ============================================================ */

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
            size_t curLen = strlen(line);
            snprintf(testLine, sizeof(testLine), "%s", line);
            if (curLen + 1 < sizeof(testLine)) {
                snprintf(testLine + curLen, sizeof(testLine) - curLen, " %s", word);
            }
        }

        if (MeasureTextEx(uiFont, testLine, fontSize, 1.0f).x > maxWidth && line[0] != '\0') {
            lines += 1.0f;
            snprintf(line, sizeof(line), "%s", word);
        } else {
            strncpy(line, testLine, sizeof(line) - 1);
            line[sizeof(line) - 1] = '\0';
        }
        word = strtok(NULL, " ");
    }
    return lines * (fontSize * 1.22f);
}

void UI_DrawPendingTooltips(void) {
    if (s_hoveredKeyThisFrame[0] == '\0') {
        s_tooltip.hoverTimer = 0.0f;
        s_tooltip.currentHoverKey[0] = '\0';
        s_tooltip.active = false;
        return;
    }

    if (s_tooltip.active && s_tooltip.hoverTimer >= 0.35f) {
        float boxW = 290.0f;
        float bodyH = MeasureWrappedTextHeight(s_tooltip.text, boxW - 24.0f, 12.0f);
        float boxH = 38.0f + bodyH + 12.0f;

        float x = s_tooltip.anchor.x + 12.0f;
        float y = s_tooltip.anchor.y + 12.0f;

        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        if (x + boxW > (float)(sw - 10)) x = s_tooltip.anchor.x - boxW - 8.0f;
        if (x < 10.0f) x = 10.0f;
        if (y + boxH > (float)(sh - 10)) y = s_tooltip.anchor.y - boxH - 8.0f;
        if (y < 10.0f) y = 10.0f;

        const UIPanelStyleDef* ts = UISkin_GetPanelStyle(UI_STYLE_PANEL_TOOLTIP);
        UI_DrawPanelStyled((Rectangle){ x, y, boxW, boxH }, UI_STYLE_PANEL_TOOLTIP);

        UI_DrawText(s_tooltip.title, (Vector2){ x + 12.0f, y + 9.0f }, 13.0f, ts->textTitle);
        UI_DrawTextWrapped(s_tooltip.text, (Vector2){ x + 12.0f, y + 30.0f }, 12.0f, boxW - 24.0f, ts->textMain);
    }

    s_hoveredKeyThisFrame[0] = '\0';
}

/* ============================================================
   CUADRÍCULAS DE SUMINISTROS Y DADO DE SEIS CARAS
   ============================================================ */

void UI_DrawSupplySquares(Rectangle bounds, float days, float totalUnits, int maxSquares, Color filledCol, const char* label, bool isShortage) {
    if (maxSquares <= 0) maxSquares = 28;
    if (days < 0.0f) days = 0.0f;

    Color activeCol = filledCol;
    if (isShortage || days <= 0.0f) {
        activeCol = UI_COLOR_BAD;
    } else if (days < 2.0f) {
        activeCol = UI_COLOR_BAD;
    } else if (days < 4.0f) {
        activeCol = UI_COLOR_WARNING;
    }

    char titleBuf[128];
    if (days <= 0.0f || isShortage) {
        if (label && label[0]) snprintf(titleBuf, sizeof(titleBuf), "%s: 0d [ESCASEZ]", label);
        else snprintf(titleBuf, sizeof(titleBuf), "0d [ESCASEZ]");
    } else if (days > (float)maxSquares) {
        if (label && label[0]) snprintf(titleBuf, sizeof(titleBuf), "%s: %.0fd (+%.0fd)", label, days, days - (float)maxSquares);
        else snprintf(titleBuf, sizeof(titleBuf), "%.0fd (+%.0fd) (%.0f)", days, days - (float)maxSquares, totalUnits);
    } else {
        if (label && label[0]) snprintf(titleBuf, sizeof(titleBuf), "%s: %.0fd (%.0f)", label, days, totalUnits);
        else snprintf(titleBuf, sizeof(titleBuf), "%.0fd raciones (%.0f)", days, totalUnits);
    }

    Color textCol = (days < 2.0f || isShortage) ? UI_COLOR_BAD : ((days < 4.0f) ? UI_COLOR_WARNING : UI_COLOR_TEXT_MAIN);
    UI_DrawText(titleBuf, (Vector2){ bounds.x, bounds.y }, 13.0f, textCol);

    int sqSize = 8;
    int sqGap = 2;
    int weekGap = 5;
    float startY = bounds.y + 16.0f;

    int colsPerRow = maxSquares / 2;
    if (colsPerRow < 7) colsPerRow = 7;

    for (int i = 0; i < maxSquares; i++) {
        int row = i / colsPerRow;
        int col = i % colsPerRow;
        int week = col / 7;
        int colInWeek = col % 7;

        float sqX = bounds.x + (float)(colInWeek * (sqSize + sqGap)) + (float)(week * (7 * (sqSize + sqGap) + weekGap));
        float sqY = startY + (float)(row * (sqSize + 3));

        float targetDay = (float)i + 1.0f;

        if (days >= targetDay) {
            DrawRectangle((int)sqX, (int)sqY, sqSize, sqSize, activeCol);
            DrawRectangleLines((int)sqX, (int)sqY, sqSize, sqSize, Fade(BLACK, 0.45f));
        } else if (days > (float)i && days < targetDay) {
            float frac = days - (float)i;
            int fillW = (int)roundf((float)sqSize * frac);
            if (fillW < 1) fillW = 1;

            DrawRectangle((int)sqX, (int)sqY, sqSize, sqSize, (Color){ 24, 28, 38, 220 });
            DrawRectangle((int)sqX, (int)sqY, fillW, sqSize, activeCol);
            DrawRectangleLines((int)sqX, (int)sqY, sqSize, sqSize, Fade(BLACK, 0.45f));
        } else {
            Color emptyBg = (Color){ 22, 26, 36, 210 };
            Color emptyBorder = (isShortage || days <= 0.0f) ? (Color){ 160, 45, 55, 180 } : (Color){ 65, 70, 85, 160 };
            DrawRectangle((int)sqX, (int)sqY, sqSize, sqSize, emptyBg);
            DrawRectangleLines((int)sqX, (int)sqY, sqSize, sqSize, emptyBorder);
        }
    }
}

void UI_DrawSingleDie(int cx, int cy, int size, int value) {
    if (value < 1) value = 1;
    if (value > 6) value = 6;
    int hs = size / 2;

    DrawRectangle(cx - hs + 2, cy - hs + 3, size, size, (Color){ 10, 12, 18, 140 });
    DrawRectangle(cx - hs, cy - hs, size, size, (Color){ 246, 243, 235, 255 });
    DrawRectangleLinesEx((Rectangle){ (float)(cx - hs), (float)(cy - hs), (float)size, (float)size }, 2.0f, (Color){ 62, 50, 38, 255 });

    DrawLine(cx - hs + 2, cy - hs + 2, cx + hs - 2, cy - hs + 2, (Color){ 255, 255, 255, 160 });
    DrawLine(cx - hs + 2, cy - hs + 2, cx - hs + 2, cy + hs - 2, (Color){ 255, 255, 255, 160 });
    DrawLine(cx - hs + 2, cy + hs - 2, cx + hs - 2, cy + hs - 2, (Color){ 190, 180, 165, 180 });
    DrawLine(cx + hs - 2, cy - hs + 2, cx + hs - 2, cy + hs - 2, (Color){ 190, 180, 165, 180 });

    int dotR = size / 9;
    if (dotR < 2) dotR = 2;
    int padding = (int)((float)size / 3.4f);
    Color dotCol = (Color){ 36, 28, 24, 255 };

    if (value == 1 || value == 3 || value == 5) {
        DrawCircle(cx, cy, (float)dotR, dotCol);
    }
    if (value > 1) {
        DrawCircle(cx - padding, cy - padding, (float)dotR, dotCol);
        DrawCircle(cx + padding, cy + padding, (float)dotR, dotCol);
    }
    if (value > 3) {
        DrawCircle(cx + padding, cy - padding, (float)dotR, dotCol);
        DrawCircle(cx - padding, cy + padding, (float)dotR, dotCol);
    }
    if (value == 6) {
        DrawCircle(cx - padding, cy, (float)dotR, dotCol);
        DrawCircle(cx + padding, cy, (float)dotR, dotCol);
    }
}

/* ============================================================
   SISTEMA DE ARRASTRAR Y SOLTAR (UI_DragDrop)
   ============================================================ */

void UIDragDrop_Init(UIDragDrop* dd) {
    if (!dd) return;
    memset(dd, 0, sizeof(UIDragDrop));
    dd->sourceGroup = -1;
}

bool UIDragDrop_Start(UIDragDrop* dd, int sourceKind, int sourceGroup, float amount, Vector2 mousePos, const char* label, Color color) {
    if (!dd || amount <= 0.001f) return false;

    dd->active = true;
    dd->sourceKind = sourceKind;
    dd->sourceGroup = sourceGroup;
    dd->amount = amount;
    dd->startPos = mousePos;
    dd->currentPos = mousePos;
    dd->isShift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    dd->isCtrl  = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    dd->color = color;

    if (label && label[0] != '\0') {
        strncpy(dd->label, label, sizeof(dd->label) - 1);
        dd->label[sizeof(dd->label) - 1] = '\0';
    } else {
        snprintf(dd->label, sizeof(dd->label), "%.0f u.", amount);
    }

    return true;
}

void UIDragDrop_Update(UIDragDrop* dd, Vector2 mousePos) {
    if (!dd || !dd->active) return;

    if (IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        UIDragDrop_Cancel(dd);
        return;
    }

    dd->currentPos = mousePos;
}

void UIDragDrop_Cancel(UIDragDrop* dd) {
    if (!dd) return;
    dd->active = false;
    dd->amount = 0.0f;
    dd->sourceGroup = -1;
}

bool UIDragDrop_Finish(UIDragDrop* dd, int* outSourceKind, int* outSourceGroup, float* outAmount) {
    if (!dd || !dd->active) return false;

    if (outSourceKind)  *outSourceKind  = dd->sourceKind;
    if (outSourceGroup) *outSourceGroup = dd->sourceGroup;
    if (outAmount)      *outAmount      = dd->amount;

    dd->active = false;
    return true;
}

void UIDragDrop_DrawFloating(const UIDragDrop* dd) {
    if (!dd || !dd->active) return;

    float fx = dd->currentPos.x + 14.0f;
    float fy = dd->currentPos.y + 14.0f;
    float fw = 140.0f;
    float fh = 42.0f;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    if (fx + fw > (float)sw - 10.0f) fx = dd->currentPos.x - fw - 10.0f;
    if (fy + fh > (float)sh - 10.0f) fy = dd->currentPos.y - fh - 10.0f;

    /* Fondo 9-slice usando UI_SKIN_TOOLTIP con el tinte del recurso */
    UISkin_DrawNineSliceDefault(UI_SKIN_TOOLTIP, 0, (Rectangle){ fx, fy, fw, fh }, dd->color);

    /* Icono del recurso si corresponde a RS_AGUA..RS_CARNE (0..3) */
    UIIconId resIcon = UI_ICON_NONE;
    if (dd->sourceKind == 0) resIcon = UI_ICON_AGUA;
    else if (dd->sourceKind == 1) resIcon = UI_ICON_VINO;
    else if (dd->sourceKind == 2) resIcon = UI_ICON_COMIDA;
    else if (dd->sourceKind == 3) resIcon = UI_ICON_CARNE;

    if (resIcon != UI_ICON_NONE) {
        UISkin_DrawIcon(resIcon, (Vector2){ fx + 8.0f, fy + (fh - 16.0f) / 2.0f }, 16.0f, WHITE);
    } else {
        DrawCircle((int)(fx + 14.0f), (int)(fy + fh / 2.0f), 5.5f, dd->color);
    }

    char textBuf[48];
    snprintf(textBuf, sizeof(textBuf), "%s (%.0fu)", dd->label, dd->amount);
    UI_DrawText(textBuf, (Vector2){ fx + 28.0f, fy + 8.0f }, 12, UI_COLOR_TEXT_MAIN);

    const char* modeTxt = dd->isShift ? "[Shift: Lote]" : (dd->isCtrl ? "[Ctrl: 1u]" : "[Paquete]");
    Color modeCol = dd->isShift ? UI_COLOR_GOLD : (dd->isCtrl ? (Color){ 140, 200, 255, 255 } : UI_COLOR_TEXT_DIM);
    UI_DrawText(modeTxt, (Vector2){ fx + 28.0f, fy + 22.0f }, 10, modeCol);
}
