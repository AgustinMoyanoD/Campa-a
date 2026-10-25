#ifndef UI_THEME_H
#define UI_THEME_H

#include "raylib.h"

/* Paleta Base: Mármol Egeo y Carbón */
#define UI_COLOR_BG CLITERAL(Color){ 15, 20, 28, 255 }
#define UI_COLOR_PANEL CLITERAL(Color){ 22, 24, 32, 245 }
#define UI_COLOR_PANEL_BORDER CLITERAL(Color){ 139, 90, 43, 255 }        /* Bronce y sombra cálida (#8B5A2B) */
#define UI_COLOR_PANEL_BORDER_LIGHT CLITERAL(Color){ 212, 166, 58, 255 }  /* Oro principal (#D4A63A) */

/* Tipografía */
#define UI_COLOR_TEXT_TITLE CLITERAL(Color){ 246, 227, 161, 255 }        /* Luz y brillo (#F6E3A1) */
#define UI_COLOR_TEXT_MAIN CLITERAL(Color){ 248, 244, 232, 255 }         /* Superficie clara marfil (#F8F4E8) */
#define UI_COLOR_TEXT_DIM CLITERAL(Color){ 166, 173, 124, 255 }          /* Oliva iluminado / pergamino (#A6AD7C) */

/* Botones */
#define UI_COLOR_BUTTON_IDLE CLITERAL(Color){ 36, 40, 52, 240 }
#define UI_COLOR_BUTTON_HOVER CLITERAL(Color){ 58, 66, 86, 255 }
#define UI_COLOR_BUTTON_PRESS CLITERAL(Color){ 22, 25, 34, 255 }
#define UI_COLOR_BUTTON_DISABLED CLITERAL(Color){ 28, 28, 34, 200 }

/* Paletas temáticas funcionales */
#define UI_COLOR_GOLD CLITERAL(Color){ 212, 166, 58, 255 }               /* Oro Olímpico principal (#D4A63A) */
#define UI_COLOR_GOLD_LIGHT CLITERAL(Color){ 246, 227, 161, 255 }         /* Oro Olímpico brillante (#F6E3A1) */
#define UI_COLOR_GOOD CLITERAL(Color){ 107, 123, 78, 255 }               /* Oliva de Atenea (#6B7B4E) */
#define UI_COLOR_BAD CLITERAL(Color){ 178, 74, 90, 255 }                 /* Vino de Dionisio carmesí (#B24A5A) */
#define UI_COLOR_BAD_DARK CLITERAL(Color){ 110, 30, 46, 255 }             /* Vino de Dionisio rojo oscuro (#6E1E2E) */
#define UI_COLOR_WARNING CLITERAL(Color){ 196, 143, 90, 255 }            /* Oliva tierra cálida (#C48F5A) */

#endif
