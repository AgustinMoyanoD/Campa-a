#ifndef UI_SKIN_H
#define UI_SKIN_H

#include "raylib.h"
#include <stdbool.h>

/* ============================================================
   SISTEMA MODULAR DE SKINS, 9-SLICE, OVERLAYS E ICONOS (ui_skin)
   Carga y administra assets/ui/skins.cfg y sus texturas pixel-art.
   ============================================================ */

typedef enum {
    UI_SKIN_BUTTON = 0,
    UI_SKIN_PANEL_DARK,
    UI_SKIN_PANEL_PARCHMENT,
    UI_SKIN_SLOT,
    UI_SKIN_BAR_FRAME,
    UI_SKIN_TOOLTIP,
    UI_SKIN_CHIP,
    UI_SKIN_OVERLAY_HIGHLIGHT,
    UI_SKIN_OVERLAY_GLOW,
    UI_SKIN_COUNT
} UISkinId;

typedef enum {
    UI_BTN_STATE_IDLE = 0,
    UI_BTN_STATE_HOVER,
    UI_BTN_STATE_PRESSED,
    UI_BTN_STATE_DISABLED,
    UI_BTN_STATE_COUNT
} UIButtonState;

typedef enum {
    UI_STYLE_BTN_DEFAULT = 0,  /* Dorado / Bronce clásico */
    UI_STYLE_BTN_PRIMARY,      /* Verde Confirmar / Acción principal */
    UI_STYLE_BTN_DANGER,       /* Rojo Sacrificar / Peligro / Asedio */
    UI_STYLE_BTN_CURSE,        /* Púrpura Pacto / Maldición */
    UI_STYLE_BTN_FLAT,         /* Sin marco 9-slice (solo texto y subrayado sutil) */
    UI_STYLE_BTN_COUNT
} UIButtonStyle;

typedef enum {
    UI_STYLE_PANEL_DARK = 0,   /* Panel oscuro con tachuelas en las esquinas */
    UI_STYLE_PANEL_PARCHMENT,  /* Panel con marco de madera e interior pergamino claro */
    UI_STYLE_PANEL_SLOT,       /* Ranura hundida (zona de soltar, ítems, cajas internas) */
    UI_STYLE_PANEL_TOOLTIP,    /* Fondo de tooltip */
    UI_STYLE_PANEL_COUNT
} UIPanelStyle;

typedef enum {
    UI_STYLE_CHIP_OK = 0,      /* Verde: Ración completa / Estado óptimo */
    UI_STYLE_CHIP_WARN,        /* Ámbar: Ración reducida / Precaución */
    UI_STYLE_CHIP_BAD,         /* Rojo: Hambruna / Peligro crítico */
    UI_STYLE_CHIP_NEUTRAL,     /* Dorado / Piedra neutra (Glosario / Info) */
    UI_STYLE_CHIP_COUNT
} UIChipStyle;

typedef enum {
    UI_ICON_NONE = 0,
    UI_ICON_AGUA,
    UI_ICON_VINO,
    UI_ICON_COMIDA,
    UI_ICON_CARNE,
    UI_ICON_SALUD,
    UI_ICON_MORAL,
    UI_ICON_SATISFACCION,
    UI_ICON_GANADO,
    UI_ICON_AVISO,
    UI_ICON_OK,
    UI_ICON_PELIGRO,
    UI_ICON_COUNT
} UIIconId;

/* Definición de un estilo de botón cargado desde skins.cfg */
typedef struct {
    UISkinId skin;
    bool hasFrame;
    Color tint[UI_BTN_STATE_COUNT];
    Color text[UI_BTN_STATE_COUNT];
} UIButtonStyleDef;

/* Definición de un estilo de panel cargado desde skins.cfg */
typedef struct {
    UISkinId skin;
    Color tint;
    Color textMain;
    Color textTitle;
} UIPanelStyleDef;

/* Definición de un estilo de chip cargado desde skins.cfg */
typedef struct {
    UISkinId skin;
    Color tint;
    Color text;
} UIChipStyleDef;

/* Ciclo de vida del sistema de skins */
bool UISkin_Init(const char* cfgPath);
void UISkin_Shutdown(void);
bool UISkin_IsLoaded(void);

/* Factor de escala entero para pixel-art (por defecto 2; 1, 2 o 3) */
void UISkin_SetPixelScale(int scale);
int  UISkin_GetPixelScale(void);

/* Consultas de estilos configurados en skins.cfg */
const UIButtonStyleDef* UISkin_GetButtonStyle(UIButtonStyle style);
const UIPanelStyleDef*  UISkin_GetPanelStyle(UIPanelStyle style);
const UIChipStyleDef*   UISkin_GetChipStyle(UIChipStyle style);

/* Dibujado 9-Slice con escala entera y redondeo pixel-perfect */
void UISkin_DrawNineSlice(UISkinId skinId, int cellIndex, Rectangle bounds, int scale, Color tint);
void UISkin_DrawNineSliceDefault(UISkinId skinId, int cellIndex, Rectangle bounds, Color tint);

/* Dibujado de barra (marco 9-slice + relleno horizontal estirado) */
void UISkin_DrawBar(Rectangle bounds, float percentage, Color fillTint, Color frameTint, int scale);

/* Efectos superpuestos (Blend Additive) y Decals de Rotura */
void UISkin_DrawHighlight(Rectangle bounds, float alpha, int scale);
void UISkin_DrawGlow(Rectangle bounds, Color color, float alpha, int scale);
void UISkin_DrawCrack(Rectangle bounds, int stage, int scale);

/* Dibujado y consulta de Iconos 16x16 */
void     UISkin_DrawIcon(UIIconId icon, Vector2 pos, float size, Color tint);
void     UISkin_DrawIconCentered(UIIconId icon, Rectangle slotBounds, int scale, Color tint);
UIIconId UISkin_GetIconByName(const char* name);
Vector2  UISkin_GetIconBaseSize(void);

#endif /* UI_SKIN_H */
