#include "presentation/ui_skin.h"
#include "presentation/ui_theme.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

typedef struct {
    char texPath[128];
    Texture2D tex;
    int cellW;
    int cellH;
    int cellCount;
    int sliceL;
    int sliceT;
    int sliceR;
    int sliceB;
    int outset;
    bool additive;
} UISkinEntry;

static UISkinEntry s_skins[UI_SKIN_COUNT];
static char s_barFillPath[128] = "ui/ui_bar_fill.png";
static Texture2D s_barFillTex = {0};

static char s_crackPaths[3][128] = {
    "ui/ui_crack_1.png",
    "ui/ui_crack_2.png",
    "ui/ui_crack_3.png"
};
static Texture2D s_crackTex[3] = {0};

static char s_iconsDir[128] = "ui/icons";
static int  s_iconBaseW = 16;
static int  s_iconBaseH = 16;
static Texture2D s_iconTex[UI_ICON_COUNT] = {0};

static const char* s_iconNames[UI_ICON_COUNT] = {
    "none",
    "agua",
    "vino",
    "comida",
    "carne",
    "salud",
    "moral",
    "satisfaccion",
    "ganado",
    "aviso",
    "ok",
    "peligro"
};

static UIButtonStyleDef s_btnStyles[UI_STYLE_BTN_COUNT];
static UIPanelStyleDef  s_panelStyles[UI_STYLE_PANEL_COUNT];
static UIChipStyleDef   s_chipStyles[UI_STYLE_CHIP_COUNT];

static int  s_pixelScale = 2;
static bool s_loaded = false;

/* ============================================================
   HELPERS DE CADENAS Y PARSEO DE SKINS.CFG
   ============================================================ */

static void TrimStr(char* s) {
    if (!s) return;
    /* Cortar comentarios ';' */
    char* semi = strchr(s, ';');
    if (semi) *semi = '\0';

    int len = (int)strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
    char* start = s;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    if (start != s) {
        memmove(s, start, strlen(start) + 1);
    }
}

static Color ParseRGB(const char* str, Color fallback) {
    if (!str) return fallback;
    int r = fallback.r, g = fallback.g, b = fallback.b;
    if (sscanf(str, "%d %d %d", &r, &g, &b) == 3) {
        if (r < 0) r = 0; else if (r > 255) r = 255;
        if (g < 0) g = 0; else if (g > 255) g = 255;
        if (b < 0) b = 0; else if (b > 255) b = 255;
        return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
    }
    return fallback;
}

static void ParseStateColors(const char* val, Color outColors[UI_BTN_STATE_COUNT]) {
    if (!val) return;
    char buf[256];
    strncpy(buf, val, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* saveptr = NULL;
    char* tok = strtok(buf, "|");
    int idx = 0;
    while (tok && idx < UI_BTN_STATE_COUNT) {
        TrimStr(tok);
        outColors[idx] = ParseRGB(tok, outColors[idx]);
        idx++;
        tok = strtok(NULL, "|");
    }
    (void)saveptr;
}

static UISkinId ParseSkinName(const char* name, UISkinId fallback) {
    if (!name) return fallback;
    if (strcmp(name, "SKIN_BUTTON") == 0)          return UI_SKIN_BUTTON;
    if (strcmp(name, "SKIN_PANEL_DARK") == 0)      return UI_SKIN_PANEL_DARK;
    if (strcmp(name, "SKIN_PANEL_PARCHMENT") == 0) return UI_SKIN_PANEL_PARCHMENT;
    if (strcmp(name, "SKIN_SLOT") == 0)            return UI_SKIN_SLOT;
    if (strcmp(name, "SKIN_BAR_FRAME") == 0)       return UI_SKIN_BAR_FRAME;
    if (strcmp(name, "SKIN_TOOLTIP") == 0)         return UI_SKIN_TOOLTIP;
    if (strcmp(name, "SKIN_CHIP") == 0)            return UI_SKIN_CHIP;
    return fallback;
}

static Texture2D LoadPixelTextureRel(const char* relAssetPath) {
    Texture2D tex = {0};
    if (!relAssetPath || relAssetPath[0] == '\0') return tex;

    char path[256];
    snprintf(path, sizeof(path), "assets/%s", relAssetPath);
    if (!FileExists(path)) {
        snprintf(path, sizeof(path), "%sassets/%s", GetApplicationDirectory(), relAssetPath);
    }
    if (FileExists(path)) {
        tex = LoadTexture(path);
        if (tex.id > 0) {
            SetTextureFilter(tex, TEXTURE_FILTER_POINT);
        }
    }
    return tex;
}

/* ============================================================
   VALORES POR DEFECTO DE SKINS Y ESTILOS
   ============================================================ */

static void InitDefaults(void) {
    memset(s_skins, 0, sizeof(s_skins));

    /* [SKIN_BUTTON] */
    s_skins[UI_SKIN_BUTTON] = (UISkinEntry){
        .texPath = "ui/ui_button.png",
        .cellW = 32, .cellH = 32, .cellCount = 4,
        .sliceL = 4, .sliceT = 4, .sliceR = 4, .sliceB = 4
    };

    /* [SKIN_PANEL_DARK] */
    s_skins[UI_SKIN_PANEL_DARK] = (UISkinEntry){
        .texPath = "ui/ui_panel_dark.png",
        .cellW = 48, .cellH = 48, .cellCount = 1,
        .sliceL = 8, .sliceT = 8, .sliceR = 8, .sliceB = 8
    };

    /* [SKIN_PANEL_PARCHMENT] */
    s_skins[UI_SKIN_PANEL_PARCHMENT] = (UISkinEntry){
        .texPath = "ui/ui_panel_parchment.png",
        .cellW = 48, .cellH = 48, .cellCount = 1,
        .sliceL = 8, .sliceT = 8, .sliceR = 8, .sliceB = 8
    };

    /* [SKIN_SLOT] */
    s_skins[UI_SKIN_SLOT] = (UISkinEntry){
        .texPath = "ui/ui_slot_inset.png",
        .cellW = 24, .cellH = 24, .cellCount = 1,
        .sliceL = 3, .sliceT = 3, .sliceR = 3, .sliceB = 3
    };

    /* [SKIN_BAR_FRAME] */
    s_skins[UI_SKIN_BAR_FRAME] = (UISkinEntry){
        .texPath = "ui/ui_bar_frame.png",
        .cellW = 24, .cellH = 12, .cellCount = 1,
        .sliceL = 3, .sliceT = 3, .sliceR = 3, .sliceB = 3
    };

    /* [SKIN_TOOLTIP] */
    s_skins[UI_SKIN_TOOLTIP] = (UISkinEntry){
        .texPath = "ui/ui_tooltip.png",
        .cellW = 32, .cellH = 32, .cellCount = 1,
        .sliceL = 4, .sliceT = 4, .sliceR = 4, .sliceB = 4
    };

    /* [SKIN_CHIP] */
    s_skins[UI_SKIN_CHIP] = (UISkinEntry){
        .texPath = "ui/ui_chip.png",
        .cellW = 24, .cellH = 16, .cellCount = 1,
        .sliceL = 3, .sliceT = 3, .sliceR = 3, .sliceB = 3
    };

    /* [OVERLAY_HIGHLIGHT] */
    s_skins[UI_SKIN_OVERLAY_HIGHLIGHT] = (UISkinEntry){
        .texPath = "ui/ui_highlight.png",
        .cellW = 32, .cellH = 32, .cellCount = 1,
        .sliceL = 4, .sliceT = 4, .sliceR = 4, .sliceB = 4,
        .additive = true
    };

    /* [OVERLAY_GLOW] */
    s_skins[UI_SKIN_OVERLAY_GLOW] = (UISkinEntry){
        .texPath = "ui/ui_glow.png",
        .cellW = 32, .cellH = 32, .cellCount = 1,
        .sliceL = 8, .sliceT = 8, .sliceR = 8, .sliceB = 8,
        .outset = 8,
        .additive = true
    };

    /* Estilos de Botón */
    s_btnStyles[UI_STYLE_BTN_DEFAULT] = (UIButtonStyleDef){
        .skin = UI_SKIN_BUTTON, .hasFrame = true,
        .tint = { {225,190,120,255}, {245,215,150,255}, {190,155,95,255}, {130,130,130,255} },
        .text = { {245,235,210,255}, {255,248,225,255}, {215,205,175,255}, {140,140,140,255} }
    };
    s_btnStyles[UI_STYLE_BTN_PRIMARY] = (UIButtonStyleDef){
        .skin = UI_SKIN_BUTTON, .hasFrame = true,
        .tint = { {120,205,140,255}, {150,225,165,255}, {95,165,112,255}, {120,130,122,255} },
        .text = { {240,250,240,255}, {255,255,255,255}, {215,230,215,255}, {140,140,140,255} }
    };
    s_btnStyles[UI_STYLE_BTN_DANGER] = (UIButtonStyleDef){
        .skin = UI_SKIN_BUTTON, .hasFrame = true,
        .tint = { {225,105,95,255}, {245,135,120,255}, {180,80,72,255}, {130,120,120,255} },
        .text = { {250,235,230,255}, {255,245,240,255}, {225,205,200,255}, {140,140,140,255} }
    };
    s_btnStyles[UI_STYLE_BTN_CURSE] = (UIButtonStyleDef){
        .skin = UI_SKIN_BUTTON, .hasFrame = true,
        .tint = { {185,115,235,255}, {205,140,250,255}, {145,90,190,255}, {125,120,135,255} },
        .text = { {245,235,255,255}, {255,245,255,255}, {220,205,235,255}, {140,140,150,255} }
    };
    s_btnStyles[UI_STYLE_BTN_FLAT] = (UIButtonStyleDef){
        .skin = UI_SKIN_BUTTON, .hasFrame = false,
        .tint = { WHITE, WHITE, WHITE, WHITE },
        .text = { {225,215,185,255}, {255,235,150,255}, {190,180,150,255}, {120,120,120,255} }
    };

    /* Estilos de Panel */
    s_panelStyles[UI_STYLE_PANEL_DARK] = (UIPanelStyleDef){
        .skin = UI_SKIN_PANEL_DARK,
        .tint = { 215, 185, 130, 255 },
        .textMain = { 235, 225, 200, 255 },
        .textTitle = { 246, 227, 161, 255 }
    };
    s_panelStyles[UI_STYLE_PANEL_PARCHMENT] = (UIPanelStyleDef){
        .skin = UI_SKIN_PANEL_PARCHMENT,
        .tint = { 255, 236, 206, 255 },
        .textMain = { 60, 40, 20, 255 },
        .textTitle = { 45, 28, 12, 255 }
    };
    s_panelStyles[UI_STYLE_PANEL_SLOT] = (UIPanelStyleDef){
        .skin = UI_SKIN_SLOT,
        .tint = { 190, 175, 150, 255 },
        .textMain = { 225, 218, 200, 255 },
        .textTitle = { 230, 195, 120, 255 }
    };
    s_panelStyles[UI_STYLE_PANEL_TOOLTIP] = (UIPanelStyleDef){
        .skin = UI_SKIN_TOOLTIP,
        .tint = { 230, 195, 120, 255 },
        .textMain = { 220, 220, 230, 255 },
        .textTitle = { 255, 220, 120, 255 }
    };

    /* Estilos de Chip */
    s_chipStyles[UI_STYLE_CHIP_OK] = (UIChipStyleDef){
        .skin = UI_SKIN_CHIP,
        .tint = { 110, 200, 120, 255 },
        .text = { 240, 248, 238, 255 }
    };
    s_chipStyles[UI_STYLE_CHIP_WARN] = (UIChipStyleDef){
        .skin = UI_SKIN_CHIP,
        .tint = { 230, 180, 70, 255 },
        .text = { 248, 242, 225, 255 }
    };
    s_chipStyles[UI_STYLE_CHIP_BAD] = (UIChipStyleDef){
        .skin = UI_SKIN_CHIP,
        .tint = { 220, 85, 80, 255 },
        .text = { 252, 238, 235, 255 }
    };
    s_chipStyles[UI_STYLE_CHIP_NEUTRAL] = (UIChipStyleDef){
        .skin = UI_SKIN_CHIP,
        .tint = { 185, 165, 125, 255 },
        .text = { 242, 232, 205, 255 }
    };
}

/* ============================================================
   LECTURA DE ASSETS/UI/SKINS.CFG
   ============================================================ */

static void ParseSkinsFile(const char* cfgPath) {
    char actualPath[256];
    snprintf(actualPath, sizeof(actualPath), "%s", cfgPath ? cfgPath : "assets/ui/skins.cfg");
    if (!FileExists(actualPath)) {
        snprintf(actualPath, sizeof(actualPath), "%s%s", GetApplicationDirectory(), cfgPath ? cfgPath : "assets/ui/skins.cfg");
    }

    FILE* fp = fopen(actualPath, "r");
    if (!fp) return;

    char section[64] = {0};
    char line[512];

    while (fgets(line, sizeof(line), fp)) {
        TrimStr(line);
        if (line[0] == '\0') continue;

        if (line[0] == '[') {
            char* end = strchr(line, ']');
            if (end) {
                *end = '\0';
                strncpy(section, line + 1, sizeof(section) - 1);
                section[sizeof(section) - 1] = '\0';
                TrimStr(section);
            }
            continue;
        }

        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char* key = line;
        char* val = eq + 1;
        TrimStr(key);
        TrimStr(val);

        /* Secciones de Skin */
        int skinIdx = -1;
        if (strcmp(section, "SKIN_BUTTON") == 0)             skinIdx = UI_SKIN_BUTTON;
        else if (strcmp(section, "SKIN_PANEL_DARK") == 0)    skinIdx = UI_SKIN_PANEL_DARK;
        else if (strcmp(section, "SKIN_PANEL_PARCHMENT") == 0) skinIdx = UI_SKIN_PANEL_PARCHMENT;
        else if (strcmp(section, "SKIN_SLOT") == 0)          skinIdx = UI_SKIN_SLOT;
        else if (strcmp(section, "SKIN_BAR_FRAME") == 0)     skinIdx = UI_SKIN_BAR_FRAME;
        else if (strcmp(section, "SKIN_TOOLTIP") == 0)       skinIdx = UI_SKIN_TOOLTIP;
        else if (strcmp(section, "SKIN_CHIP") == 0)          skinIdx = UI_SKIN_CHIP;
        else if (strcmp(section, "OVERLAY_HIGHLIGHT") == 0)  skinIdx = UI_SKIN_OVERLAY_HIGHLIGHT;
        else if (strcmp(section, "OVERLAY_GLOW") == 0)       skinIdx = UI_SKIN_OVERLAY_GLOW;

        if (skinIdx >= 0) {
            UISkinEntry* e = &s_skins[skinIdx];
            if (strcmp(key, "Texture") == 0) {
                strncpy(e->texPath, val, sizeof(e->texPath) - 1);
            } else if (strcmp(key, "CellSize") == 0) {
                sscanf(val, "%d %d", &e->cellW, &e->cellH);
            } else if (strcmp(key, "Slice") == 0) {
                sscanf(val, "%d %d %d %d", &e->sliceL, &e->sliceT, &e->sliceR, &e->sliceB);
            } else if (strcmp(key, "Fill") == 0) {
                strncpy(s_barFillPath, val, sizeof(s_barFillPath) - 1);
            } else if (strcmp(key, "Outset") == 0) {
                e->outset = atoi(val);
            } else if (strcmp(key, "Blend") == 0) {
                e->additive = (strcmp(val, "additive") == 0);
            }
            continue;
        }

        if (strcmp(section, "DECAL_CRACK") == 0) {
            if (strcmp(key, "Stages") == 0) {
                sscanf(val, "%127s %127s %127s", s_crackPaths[0], s_crackPaths[1], s_crackPaths[2]);
            }
            continue;
        }

        if (strcmp(section, "ICONS") == 0) {
            if (strcmp(key, "Dir") == 0) {
                strncpy(s_iconsDir, val, sizeof(s_iconsDir) - 1);
            } else if (strcmp(key, "Size") == 0) {
                sscanf(val, "%d %d", &s_iconBaseW, &s_iconBaseH);
            }
            continue;
        }

        /* Estilos de Botón */
        int btnStyleIdx = -1;
        if (strcmp(section, "STYLE_BUTTON_DEFAULT") == 0)      btnStyleIdx = UI_STYLE_BTN_DEFAULT;
        else if (strcmp(section, "STYLE_BUTTON_PRIMARY") == 0) btnStyleIdx = UI_STYLE_BTN_PRIMARY;
        else if (strcmp(section, "STYLE_BUTTON_DANGER") == 0)  btnStyleIdx = UI_STYLE_BTN_DANGER;
        else if (strcmp(section, "STYLE_BUTTON_CURSE") == 0)   btnStyleIdx = UI_STYLE_BTN_CURSE;
        else if (strcmp(section, "STYLE_BUTTON_FLAT") == 0)    btnStyleIdx = UI_STYLE_BTN_FLAT;

        if (btnStyleIdx >= 0) {
            UIButtonStyleDef* bs = &s_btnStyles[btnStyleIdx];
            if (strcmp(key, "Skin") == 0) {
                bs->skin = ParseSkinName(val, bs->skin);
            } else if (strcmp(key, "Frame") == 0) {
                bs->hasFrame = (strcmp(val, "true") == 0 || strcmp(val, "1") == 0);
            } else if (strcmp(key, "Tint") == 0) {
                ParseStateColors(val, bs->tint);
            } else if (strcmp(key, "Text") == 0) {
                ParseStateColors(val, bs->text);
            }
            continue;
        }

        /* Estilos de Panel */
        int panelStyleIdx = -1;
        if (strcmp(section, "STYLE_PANEL_DEFAULT") == 0)        panelStyleIdx = UI_STYLE_PANEL_DARK;
        else if (strcmp(section, "STYLE_PANEL_PARCHMENT") == 0) panelStyleIdx = UI_STYLE_PANEL_PARCHMENT;
        else if (strcmp(section, "STYLE_TOOLTIP_DEFAULT") == 0) panelStyleIdx = UI_STYLE_PANEL_TOOLTIP;

        if (panelStyleIdx >= 0) {
            UIPanelStyleDef* ps = &s_panelStyles[panelStyleIdx];
            if (strcmp(key, "Skin") == 0) {
                ps->skin = ParseSkinName(val, ps->skin);
            } else if (strcmp(key, "Tint") == 0) {
                ps->tint = ParseRGB(val, ps->tint);
            }
            continue;
        }

        /* Estilos de Chip */
        int chipStyleIdx = -1;
        if (strcmp(section, "STYLE_CHIP_OK") == 0)        chipStyleIdx = UI_STYLE_CHIP_OK;
        else if (strcmp(section, "STYLE_CHIP_WARN") == 0) chipStyleIdx = UI_STYLE_CHIP_WARN;
        else if (strcmp(section, "STYLE_CHIP_BAD") == 0)  chipStyleIdx = UI_STYLE_CHIP_BAD;

        if (chipStyleIdx >= 0) {
            UIChipStyleDef* cs = &s_chipStyles[chipStyleIdx];
            if (strcmp(key, "Skin") == 0) {
                cs->skin = ParseSkinName(val, cs->skin);
            } else if (strcmp(key, "Tint") == 0) {
                cs->tint = ParseRGB(val, cs->tint);
            }
            continue;
        }
    }

    fclose(fp);
}

/* ============================================================
   CICLO DE VIDA: INIT / SHUTDOWN
   ============================================================ */

bool UISkin_Init(const char* cfgPath) {
    InitDefaults();
    ParseSkinsFile(cfgPath ? cfgPath : "assets/ui/skins.cfg");

    /* Solo cargar texturas de GPU si la ventana Raylib está activa */
    if (!IsWindowReady()) {
        s_loaded = false;
        return true;
    }

    int loadedCount = 0;
    for (int i = 0; i < UI_SKIN_COUNT; i++) {
        s_skins[i].tex = LoadPixelTextureRel(s_skins[i].texPath);
        if (s_skins[i].tex.id > 0) {
            if (s_skins[i].cellCount <= 1) {
                s_skins[i].cellW = s_skins[i].tex.width;
                s_skins[i].cellH = s_skins[i].tex.height;
            }
            loadedCount++;
        }
    }

    s_barFillTex = LoadPixelTextureRel(s_barFillPath);
    for (int i = 0; i < 3; i++) {
        s_crackTex[i] = LoadPixelTextureRel(s_crackPaths[i]);
    }

    for (int ic = 1; ic < UI_ICON_COUNT; ic++) {
        char iconRel[192];
        snprintf(iconRel, sizeof(iconRel), "%s/icon_%s.png", s_iconsDir, s_iconNames[ic]);
        s_iconTex[ic] = LoadPixelTextureRel(iconRel);
    }

    s_loaded = (loadedCount > 0);
    if (s_loaded) {
        TraceLog(LOG_INFO, "[UISkin] Sistema de skins 9-slice e iconos cargado exitosamente (%d skins).", loadedCount);
    }
    return s_loaded;
}

void UISkin_Shutdown(void) {
    if (IsWindowReady()) {
        for (int i = 0; i < UI_SKIN_COUNT; i++) {
            if (s_skins[i].tex.id > 0) {
                UnloadTexture(s_skins[i].tex);
                s_skins[i].tex = (Texture2D){0};
            }
        }
        if (s_barFillTex.id > 0) {
            UnloadTexture(s_barFillTex);
            s_barFillTex = (Texture2D){0};
        }
        for (int i = 0; i < 3; i++) {
            if (s_crackTex[i].id > 0) {
                UnloadTexture(s_crackTex[i]);
                s_crackTex[i] = (Texture2D){0};
            }
        }
        for (int ic = 1; ic < UI_ICON_COUNT; ic++) {
            if (s_iconTex[ic].id > 0) {
                UnloadTexture(s_iconTex[ic]);
                s_iconTex[ic] = (Texture2D){0};
            }
        }
    }
    s_loaded = false;
}

bool UISkin_IsLoaded(void) {
    return s_loaded;
}

void UISkin_SetPixelScale(int scale) {
    if (scale < 1) scale = 1;
    if (scale > 4) scale = 4;
    s_pixelScale = scale;
}

int UISkin_GetPixelScale(void) {
    return s_pixelScale;
}

/* ============================================================
   CONSULTA DE ESTILOS
   ============================================================ */

const UIButtonStyleDef* UISkin_GetButtonStyle(UIButtonStyle style) {
    if (style < 0 || style >= UI_STYLE_BTN_COUNT) return &s_btnStyles[UI_STYLE_BTN_DEFAULT];
    return &s_btnStyles[style];
}

const UIPanelStyleDef* UISkin_GetPanelStyle(UIPanelStyle style) {
    if (style < 0 || style >= UI_STYLE_PANEL_COUNT) return &s_panelStyles[UI_STYLE_PANEL_DARK];
    return &s_panelStyles[style];
}

const UIChipStyleDef* UISkin_GetChipStyle(UIChipStyle style) {
    if (style < 0 || style >= UI_STYLE_CHIP_COUNT) return &s_chipStyles[UI_STYLE_CHIP_NEUTRAL];
    return &s_chipStyles[style];
}

/* ============================================================
   DIBUJADO 9-SLICE PIXEL-PERFECT CON ESCALA ENTERA
   ============================================================ */

void UISkin_DrawNineSlice(UISkinId skinId, int cellIndex, Rectangle bounds, int scale, Color tint) {
    if (bounds.width <= 0.0f || bounds.height <= 0.0f) return;
    if (skinId < 0 || skinId >= UI_SKIN_COUNT) return;
    if (scale <= 0) scale = s_pixelScale;

    float dx = roundf(bounds.x);
    float dy = roundf(bounds.y);
    float dw = roundf(bounds.width);
    float dh = roundf(bounds.height);

    const UISkinEntry* e = &s_skins[skinId];
    if (e->tex.id == 0) {
        /* Fallback limpio si se ejecuta sin texturas */
        Color bg = { (unsigned char)(tint.r / 4), (unsigned char)(tint.g / 4), (unsigned char)(tint.b / 4), tint.a };
        DrawRectangle((int)dx, (int)dy, (int)dw, (int)dh, bg);
        DrawRectangleLinesEx((Rectangle){ dx, dy, dw, dh }, 2.0f, tint);
        return;
    }

    if (cellIndex < 0) cellIndex = 0;
    if (e->cellCount > 1 && cellIndex >= e->cellCount) cellIndex = e->cellCount - 1;

    float cx = (float)(cellIndex * e->cellW);
    float cy = 0.0f;
    float cw = (float)e->cellW;
    float ch = (float)e->cellH;

    float l = (float)e->sliceL;
    float t = (float)e->sliceT;
    float r = (float)e->sliceR;
    float b = (float)e->sliceB;

    float sl = l * (float)scale;
    float st = t * (float)scale;
    float sr = r * (float)scale;
    float sb = b * (float)scale;

    /* Evitar solapamiento de esquinas en elementos muy pequeños */
    if (sl + sr > dw) {
        float ratio = dw / (sl + sr);
        sl = floorf(sl * ratio);
        sr = dw - sl;
    }
    if (st + sb > dh) {
        float ratio = dh / (st + sb);
        st = floorf(st * ratio);
        sb = dh - st;
    }

    float midSrcW = cw - l - r;
    float midSrcH = ch - t - b;
    float midDstW = dw - sl - sr;
    float midDstH = dh - st - sb;

    Vector2 orig = { 0.0f, 0.0f };

    /* 1. Centro */
    if (midDstW > 0.0f && midDstH > 0.0f && midSrcW > 0.0f && midSrcH > 0.0f) {
        DrawTexturePro(e->tex, (Rectangle){ cx + l, cy + t, midSrcW, midSrcH },
                               (Rectangle){ dx + sl, dy + st, midDstW, midDstH }, orig, 0.0f, tint);
    }

    /* 2. Bordes superior e inferior */
    if (midDstW > 0.0f && midSrcW > 0.0f) {
        if (st > 0.0f && t > 0.0f) {
            DrawTexturePro(e->tex, (Rectangle){ cx + l, cy, midSrcW, t },
                                   (Rectangle){ dx + sl, dy, midDstW, st }, orig, 0.0f, tint);
        }
        if (sb > 0.0f && b > 0.0f) {
            DrawTexturePro(e->tex, (Rectangle){ cx + l, cy + ch - b, midSrcW, b },
                                   (Rectangle){ dx + sl, dy + dh - sb, midDstW, sb }, orig, 0.0f, tint);
        }
    }

    /* 3. Bordes izquierdo y derecho */
    if (midDstH > 0.0f && midSrcH > 0.0f) {
        if (sl > 0.0f && l > 0.0f) {
            DrawTexturePro(e->tex, (Rectangle){ cx, cy + t, l, midSrcH },
                                   (Rectangle){ dx, dy + st, sl, midDstH }, orig, 0.0f, tint);
        }
        if (sr > 0.0f && r > 0.0f) {
            DrawTexturePro(e->tex, (Rectangle){ cx + cw - r, cy + t, r, midSrcH },
                                   (Rectangle){ dx + dw - sr, dy + st, sr, midDstH }, orig, 0.0f, tint);
        }
    }

    /* 4. Cuatro esquinas */
    if (sl > 0.0f && st > 0.0f) {
        DrawTexturePro(e->tex, (Rectangle){ cx, cy, l, t },
                               (Rectangle){ dx, dy, sl, st }, orig, 0.0f, tint);
    }
    if (sr > 0.0f && st > 0.0f) {
        DrawTexturePro(e->tex, (Rectangle){ cx + cw - r, cy, r, t },
                               (Rectangle){ dx + dw - sr, dy, sr, st }, orig, 0.0f, tint);
    }
    if (sl > 0.0f && sb > 0.0f) {
        DrawTexturePro(e->tex, (Rectangle){ cx, cy + ch - b, l, b },
                               (Rectangle){ dx, dy + dh - sb, sl, sb }, orig, 0.0f, tint);
    }
    if (sr > 0.0f && sb > 0.0f) {
        DrawTexturePro(e->tex, (Rectangle){ cx + cw - r, cy + ch - b, r, b },
                               (Rectangle){ dx + dw - sr, dy + dh - sb, sr, sb }, orig, 0.0f, tint);
    }
}

void UISkin_DrawNineSliceDefault(UISkinId skinId, int cellIndex, Rectangle bounds, Color tint) {
    UISkin_DrawNineSlice(skinId, cellIndex, bounds, s_pixelScale, tint);
}

/* ============================================================
   BARRAS DE PROGRESO (MARCO 9-SLICE + RELLENO ESTIRADO)
   ============================================================ */

void UISkin_DrawBar(Rectangle bounds, float percentage, Color fillTint, Color frameTint, int scale) {
    if (bounds.width <= 0.0f || bounds.height <= 0.0f) return;
    if (scale <= 0) scale = s_pixelScale;

    if (percentage < 0.0f) percentage = 0.0f;
    if (percentage > 1.0f) percentage = 1.0f;

    float dx = roundf(bounds.x);
    float dy = roundf(bounds.y);
    float dw = roundf(bounds.width);
    float dh = roundf(bounds.height);

    /* En barras muy delgadas (< 14px), usar escala 1 para el marco */
    int barScale = (dh < 16.0f) ? 1 : scale;
    UISkin_DrawNineSlice(UI_SKIN_BAR_FRAME, 0, (Rectangle){ dx, dy, dw, dh }, barScale, frameTint);

    float pad = (float)(3 * barScale);
    if (pad * 2.0f >= dh - 2.0f) pad = 2.0f;

    float maxFillW = dw - pad * 2.0f;
    float fillH    = dh - pad * 2.0f;
    float fillW    = roundf(maxFillW * percentage);

    if (percentage > 0.001f && fillW < 1.0f) fillW = 1.0f;

    if (fillW > 0.0f && fillH > 0.0f) {
        Rectangle dstFill = { dx + pad, dy + pad, fillW, fillH };
        if (s_barFillTex.id > 0) {
            DrawTexturePro(s_barFillTex,
                           (Rectangle){ 0.0f, 0.0f, (float)s_barFillTex.width, (float)s_barFillTex.height },
                           dstFill,
                           (Vector2){ 0.0f, 0.0f },
                           0.0f,
                           fillTint);
        } else {
            DrawRectangleRec(dstFill, fillTint);
        }
    }
}

/* ============================================================
   EFECTOS ADITIVOS (HIGHLIGHT / GLOW) Y DECALS DE GRIETAS
   ============================================================ */

void UISkin_DrawHighlight(Rectangle bounds, float alpha, int scale) {
    if (alpha <= 0.01f) return;
    if (alpha > 1.0f) alpha = 1.0f;
    if (scale <= 0) scale = s_pixelScale;

    Color tint = { 255, 255, 255, (unsigned char)(255.0f * alpha) };
    BeginBlendMode(BLEND_ADDITIVE);
    UISkin_DrawNineSlice(UI_SKIN_OVERLAY_HIGHLIGHT, 0, bounds, scale, tint);
    EndBlendMode();
}

void UISkin_DrawGlow(Rectangle bounds, Color color, float alpha, int scale) {
    if (alpha <= 0.01f) return;
    if (alpha > 1.0f) alpha = 1.0f;
    if (scale <= 0) scale = s_pixelScale;

    float outset = (float)(s_skins[UI_SKIN_OVERLAY_GLOW].outset * scale);
    Rectangle glowBounds = {
        bounds.x - outset,
        bounds.y - outset,
        bounds.width + outset * 2.0f,
        bounds.height + outset * 2.0f
    };

    Color tint = { color.r, color.g, color.b, (unsigned char)(255.0f * alpha) };
    BeginBlendMode(BLEND_ADDITIVE);
    UISkin_DrawNineSlice(UI_SKIN_OVERLAY_GLOW, 0, glowBounds, scale, tint);
    EndBlendMode();
}

void UISkin_DrawCrack(Rectangle bounds, int stage, int scale) {
    if (stage < 1 || stage > 3) return;
    if (scale <= 0) scale = s_pixelScale;

    Texture2D tex = s_crackTex[stage - 1];
    if (tex.id == 0) return;

    float cw = (float)(tex.width * scale);
    float ch = (float)(tex.height * scale);
    float dx = roundf(bounds.x + bounds.width - cw);
    float dy = roundf(bounds.y - (float)(4 * scale));

    DrawTexturePro(tex,
                   (Rectangle){ 0.0f, 0.0f, (float)tex.width, (float)tex.height },
                   (Rectangle){ dx, dy, cw, ch },
                   (Vector2){ 0.0f, 0.0f },
                   0.0f,
                   WHITE);
}

/* ============================================================
   ICONOS PIXEL-ART 16x16
   ============================================================ */

void UISkin_DrawIcon(UIIconId icon, Vector2 pos, float size, Color tint) {
    if (icon <= UI_ICON_NONE || icon >= UI_ICON_COUNT) return;
    Texture2D tex = s_iconTex[icon];
    if (tex.id == 0) return;

    if (size <= 0.0f) size = (float)(s_iconBaseW * s_pixelScale);
    float dx = roundf(pos.x);
    float dy = roundf(pos.y);
    float ds = roundf(size);

    DrawTexturePro(tex,
                   (Rectangle){ 0.0f, 0.0f, (float)tex.width, (float)tex.height },
                   (Rectangle){ dx, dy, ds, ds },
                   (Vector2){ 0.0f, 0.0f },
                   0.0f,
                   tint);
}

void UISkin_DrawIconCentered(UIIconId icon, Rectangle slotBounds, int scale, Color tint) {
    if (icon <= UI_ICON_NONE || icon >= UI_ICON_COUNT) return;
    if (scale <= 0) scale = s_pixelScale;

    float size = (float)(s_iconBaseW * scale);
    if (size > slotBounds.width - 4.0f) size = (float)s_iconBaseW;
    if (size > slotBounds.height - 4.0f) size = (float)s_iconBaseH;

    Vector2 pos = {
        roundf(slotBounds.x + (slotBounds.width - size) / 2.0f),
        roundf(slotBounds.y + (slotBounds.height - size) / 2.0f)
    };
    UISkin_DrawIcon(icon, pos, size, tint);
}

UIIconId UISkin_GetIconByName(const char* name) {
    if (!name || name[0] == '\0') return UI_ICON_NONE;
    for (int i = 1; i < UI_ICON_COUNT; i++) {
        if (strcmp(s_iconNames[i], name) == 0) {
            return (UIIconId)i;
        }
    }
    return UI_ICON_NONE;
}

Vector2 UISkin_GetIconBaseSize(void) {
    return (Vector2){ (float)s_iconBaseW, (float)s_iconBaseH };
}
