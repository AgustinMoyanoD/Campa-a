#ifndef HUD_H
#define HUD_H

#include "raylib.h"
#include "models/army.h"
#include "controllers/game_master.h"
#include "map_generator.h"

/* Estado del sistema de dados */
typedef struct {
    int valor1;         /* Valor actual del dado 1 (1-6) */
    int valor2;         /* Valor actual del dado 2 (1-6) */
    bool rolling;       /* TRUE mientras se está animando el tiro */
    float rollTimer;    /* Tiempo acumulado de la animación de tiro */
    float rollDuration; /* Duración total de la animación (0.5s) */
} DiceState;

/* Estado del HUD */
typedef struct {
    bool statsExpanded;     /* TRUE = panel de stats del comandante visible */
    float blinkTimer;       /* Timer acumulado para parpadeo del peón en el minimapa */
    DiceState dice;
    
    Rectangle cmdPanelRect; /* Área clickeable del panel del comandante */
    Rectangle rollBtnRect;  /* Área clickeable del botón ROLL */
} HudState;

/* Inicializa el estado del HUD */
HudState Hud_Init(void);

/* Actualiza timers internos (llamar cada frame con deltaTime) */
void Hud_Update(HudState* hud, float deltaTime);

/* Tira los dados (inicia la animación) */
void Hud_RollDice(HudState* hud);

/* Dibuja el panel del comandante (arriba izquierda) */
void Hud_DrawCommanderPanel(
    const Army*  army,
    HudState*    hud,
    int x, int y, int w
);

/* Dibuja el panel de stats y recursos del ejército (abajo izquierda) */
void Hud_DrawArmyStatsPanel(const Army* army, const GameMaster* gm, int x, int y, int w, int h);

/* Dibuja el panel de dados (abajo centro) */
void Hud_DrawDicePanel(HudState* hud, const GameMaster* gm, int x, int y, int w, int h);

/* Maneja el input del mouse para clics en los paneles */
void Hud_HandleInput(HudState* hud, GameMaster* gm);

#endif /* HUD_H */


void Hud_DrawTopBar(const GameMaster* gm, int screenWidth);


