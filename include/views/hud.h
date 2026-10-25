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
    bool rolling;       /* TRUE mientras se esta animando el tiro */
    float rollTimer;    /* Tiempo acumulado de la animacion de tiro */
    float rollDuration; /* Duracion total de la animacion (0.5s) */
} DiceState;

/* Estado del HUD */
typedef struct {
    bool statsExpanded;     /* TRUE = panel de stats del comandante visible */
    float blinkTimer;       /* Timer acumulado para parpadeo del peon en el minimapa */
    DiceState dice;
    
    Rectangle cmdPanelRect; /* Area clickeable del panel del comandante */
    Rectangle rollBtnRect;  /* Area clickeable del boton ROLL */
    Rectangle forageBtnRect;/* Area clickeable del boton FORAGE */
    Rectangle endTurnBtnRect;/* Area clickeable del boton PASAR TURNO */
    Rectangle officersBtnRect;/* Area clickeable del boton OFICIALES */
    bool showOfficersPanel; /* TRUE = modal de lista de oficiales visible (E3) */
    Rectangle groupsBtnRect;  /* Area clickeable del boton GRUPOS (Moral y Estado) */
    bool showGroupsPanel;   /* TRUE = modal de grupos del ejercito visible */
} HudState;

/* Inicializa el estado del HUD */
void Hud_InitState(HudState* hud);
HudState Hud_Init(void);

/* Actualiza timers internos (llamar cada frame con deltaTime) */
void Hud_Update(HudState* hud, float deltaTime);

/* Tira los dados (inicia la animacion) */
void Hud_RollDice(HudState* hud);

/* Dibuja el panel del comandante (arriba izquierda) */
void Hud_DrawCommanderPanel(
    const Army*  army,
    const GameMaster* gm,
    HudState*    hud,
    int x, int y, int w
);

/* Dibuja el panel de stats y recursos del ejercito (abajo izquierda) */
void Hud_DrawArmyStatsPanel(const Army* army, const GameMaster* gm, const Map* map, HudState* hud, int x, int y, int w, int h);

/* Dibuja el panel de dados (abajo centro) */
void Hud_DrawDicePanel(HudState* hud, const GameMaster* gm, const Army* army, const Map* map, int x, int y, int w, int h);

/* Maneja el input del mouse para clics en los paneles */
void Hud_HandleInput(HudState* hud, GameMaster* gm, Army* army, Map* map);

/* Barra superior de informacion de campaña */
void Hud_DrawTopBar(const GameMaster* gm, int screenWidth);

/* Modal de resumen de cierre de dia (I7) */
void Hud_DrawDayReportModal(const DayReport* report, int screenWidth, int screenHeight);
bool Hud_HandleDayReportInput(GameMaster* gm, int screenWidth, int screenHeight);

/* Modal de decision de campamento / marcha nocturna (Fase 2) */
void Hud_DrawCampDecisionModal(const GameMaster* gm, const Army* army, const Map* map, int screenWidth, int screenHeight);
bool Hud_HandleCampDecisionInput(GameMaster* gm, Map* map, Army* army, int screenWidth, int screenHeight);

/* Modal del panel de oficiales y tenientes (E3) */
void Hud_DrawOfficersPanel(const Army* army, const GameMaster* gm, HudState* hud, int screenWidth, int screenHeight);
bool Hud_HandleOfficersPanelInput(HudState* hud, int screenWidth, int screenHeight);

/* Modal del panel de grupos del ejercito (moral, salud, raciones y satisfaccion) */
void Hud_DrawGroupsPanel(const Army* army, const GameMaster* gm, HudState* hud, int screenWidth, int screenHeight);
bool Hud_HandleGroupsPanelInput(HudState* hud, int screenWidth, int screenHeight);

#endif /* HUD_H */
