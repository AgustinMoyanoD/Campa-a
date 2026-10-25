#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include "raylib.h"
#include "presentation/ui_skin.h"
#include "presentation/ui_cards.h"
#include <stdbool.h>

/* ============================================================
   API CENTRALIZADA DE WIDGETS, CONTENEDORES Y PANTALLAS (ui_widgets)
   ============================================================ */

/* Ciclo de vida general de la interfaz (Fuentes, Glosario, Skins 9-slice y Cartas) */
void UI_Init(void);
void UI_Shutdown(void);

/* Tipografía y Medición */
Font    UI_GetFont(void);
Vector2 UI_MeasureText(const char* text, float fontSize);
void    UI_DrawText(const char* text, Vector2 pos, float fontSize, Color color);
void    UI_DrawTextWrapped(const char* text, Vector2 pos, float fontSize, float maxWidth, Color color);
void    UI_DrawTextWrappedMaxH(const char* text, Vector2 pos, float fontSize, float maxWidth, float maxHeight, Color color);

/* Paneles, Contenedores, Ranuras (Slots) y Pantallas Modales */
void UI_DrawPanel(Rectangle bounds);
void UI_DrawPanelStyled(Rectangle bounds, UIPanelStyle style);
void UI_DrawPanelTinted(Rectangle bounds, UISkinId skin, Color tint);
void UI_DrawSlot(Rectangle bounds, Color tint, UIIconId icon);
void UI_DrawModalWindow(Rectangle modalBounds, const char* title, const char* subtitle, UIPanelStyle style);

/* Iconos 16x16 */
void UI_DrawIcon(UIIconId icon, Vector2 pos, float size, Color tint);

/* Especificación modular para botones avanzados */
typedef struct {
    UIButtonStyle style;      /* Estilo semántico: DEFAULT, PRIMARY, DANGER, CURSE, FLAT */
    float fontSize;           /* 0.0f = automático según altura del botón */
    bool disabled;            /* Estado deshabilitado */
    bool selected;            /* Activa resplandor exterior (glow) + brillo (Elegido/Tab activa) */
    int crackStage;           /* 0 = intacto, 1..3 = decal de rotura en esquina */
    UIIconId icon;            /* Icono opcional a la izquierda del texto */
    const char* subtext;      /* Subtexto opcional en 2da línea (ej. razón de bloqueo) */
    const char* sfxOnClick;   /* Nombre de SFX al clickear (NULL = "ui_click", "" = silencio) */
    Color customTint;         /* Si customTint.a > 0, sobreescribe el tint del estilo */
    Color customText;         /* Si customText.a > 0, sobreescribe el color de texto */
} UIButtonSpec;

/* Botones (API Basada en Estilos y Especificación + Retrocompatibilidad) */
bool UI_DrawButtonSpec(Rectangle bounds, const char* text, const UIButtonSpec* spec);
bool UI_DrawButtonStyled(Rectangle bounds, const char* text, UIButtonStyle style, bool disabled);
bool UI_DrawButtonStyledSfx(Rectangle bounds, const char* text, UIButtonStyle style, bool disabled, const char* sfxOnClick);

bool UI_DrawButton(Rectangle bounds, const char* text, bool disabled);
bool UI_DrawButtonSfx(Rectangle bounds, const char* text, bool disabled, const char* sfxOnClick);
bool UI_DrawButtonEx(Rectangle bounds, const char* text, float fontSize, bool disabled, Color bg, Color fg, Color border);
bool UI_DrawButtonExSfx(Rectangle bounds, const char* text, float fontSize, bool disabled, Color bg, Color fg, Color border, const char* sfxOnClick);
bool UI_ButtonClicked(Rectangle bounds);

/* Barras de Progreso 9-Slice */
void UI_DrawBar(Rectangle bounds, float percentage, Color fg, Color bg);
void UI_DrawLabeledBar(Rectangle bounds, float percentage, const char* label, Color fillTint, UIIconId icon);

/* Chips Semánticos y Etiquetas de Glosario */
Rectangle UI_DrawStatusChip(Rectangle bounds, const char* label, UIChipStyle style, UIIconId icon);
Rectangle UI_DrawChip(const char* termId, Rectangle bounds);
Rectangle UI_DrawChipEx(const char* label, const char* termId, Rectangle bounds, Color bg, Color fg);

/* Tooltips y Glosario (Sec B1) */
void UI_RequestTooltip(const char* title, const char* text, Vector2 anchorPos);
void UI_RequestGlossaryTooltip(const char* termId, Vector2 anchorPos);
void UI_DrawPendingTooltips(void);

/* Cuadrículas de raciones de comida y agua (cuadrados por día de consumo) */
void UI_DrawSupplySquares(Rectangle bounds, float days, float totalUnits, int maxSquares, Color filledCol, const char* label, bool isShortage);

/* Representación visual de un dado de seis caras (marfil clásico) */
void UI_DrawSingleDie(int cx, int cy, int size, int value);

/* ============================================================
   SISTEMA DE ARRASTRAR Y SOLTAR (UI_DragDrop)
   ============================================================ */

typedef struct {
    bool active;             /* TRUE mientras se está arrastrando */
    int sourceKind;          /* Identificador del origen (ej. recurso RS_AGUA..RS_CARNE) */
    int sourceGroup;         /* -1 si viene de la pila general; 0..3 si viene de un grupo */
    float amount;            /* Cantidad transportada en el paquete */
    Vector2 startPos;        /* Posición donde se inició el drag */
    Vector2 currentPos;      /* Posición actual del cursor */
    bool isShift;            /* Paquete grande */
    bool isCtrl;             /* Unidad individual */
    char label[32];          /* Etiqueta descriptiva (ej. "Agua: 5u") */
    Color color;             /* Color representativo del recurso */
} UIDragDrop;

void UIDragDrop_Init(UIDragDrop* dd);
bool UIDragDrop_Start(UIDragDrop* dd, int sourceKind, int sourceGroup, float amount, Vector2 mousePos, const char* label, Color color);
void UIDragDrop_Update(UIDragDrop* dd, Vector2 mousePos);
void UIDragDrop_Cancel(UIDragDrop* dd);
bool UIDragDrop_Finish(UIDragDrop* dd, int* outSourceKind, int* outSourceGroup, float* outAmount);
void UIDragDrop_DrawFloating(const UIDragDrop* dd);

#endif /* UI_WIDGETS_H */
