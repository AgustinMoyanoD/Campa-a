#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "controllers/game_master.h"
#include <stdbool.h>

/* Contextos musicales del juego (Streaming y fundidos) */
typedef enum {
    MUSIC_CTX_NONE = 0,
    MUSIC_CTX_MENU,     /* menu_theme: Menu de inicio */
    MUSIC_CTX_MAIN,     /* main_theme: Exploracion del mapa */
    MUSIC_CTX_COMBAT,   /* combat_theme: Pantalla de combate */
    MUSIC_CTX_CITY,     /* city_theme: Asentamientos y ciudades */
    MUSIC_CTX_COUNT
} AudioMusicContext;

/* Identificadores de Efectos de Sonido */
typedef enum {
    /* Imprescindibles */
    SFX_COMBAT_START = 0,
    SFX_COMBAT_END,
    SFX_DAMAGE_DEALT,
    SFX_MARKET_BUY,
    SFX_DIVINE_CARD_ACQUIRED,

    /* Interfaz y Dados */
    SFX_UI_CLICK,
    SFX_UI_HOVER,
    SFX_MENU_CLICK,
    SFX_UI_CONFIRM,
    SFX_DICE_ROLL,
    SFX_DICE_LAND,

    /* Movimiento y Campaña */
    SFX_STEP,
    SFX_TURN_END,
    SFX_NIGHT_FALL,
    SFX_EVENT_OPEN,
    SFX_EVENT_CHOICE,
    SFX_FORAGE,
    SFX_CAMP_REST,
    SFX_NIGHT_MARCH,

    /* Combate */
    SFX_DAMAGE_TAKEN,
    SFX_CRITICAL_HIT,

    /* Economia, Altar y Tropas */
    SFX_MARKET_SELL,
    SFX_MARKET_DENIED,
    SFX_ALTAR_OFFER,
    SFX_CURSE_ACQUIRED,
    SFX_CARD_SACRIFICED,
    SFX_PROMOTE_TROOPS,
    SFX_HEAL,
    SFX_INN_REST,

    /* Eventos Mayores y Alertas */
    SFX_SIEGE_START,
    SFX_SIEGE_VICTORY,
    SFX_ENEMY_ALERT,
    SFX_LOW_SUPPLY_WARNING,
    SFX_MUTINY_WARNING,
    SFX_LEVEL_COMPLETE,
    SFX_GAME_OVER,

    /* Efectos Ambientales (Aleatorios en mapa de exploracion) */
    SFX_AMBIENT_CROW,
    SFX_AMBIENT_WIND,
    SFX_AMBIENT_BIRDS,
    SFX_AMBIENT_INSECTS,
    SFX_AMBIENT_ANIMALS,

    SFX_COUNT
} SfxId;

/* Aliases de compatibilidad previa */
#define SFX_CARD_TRIGGER SFX_DIVINE_CARD_ACQUIRED
#define SFX_HIT SFX_DAMAGE_DEALT
#define SFX_CASUALTY SFX_DAMAGE_TAKEN


/* Tipos de musica legados (compatibilidad) */
typedef enum { 
    MUSIC_MAP = MUSIC_CTX_MAIN, 
    MUSIC_COMBAT = MUSIC_CTX_COMBAT, 
    MUSIC_EVENT = MUSIC_CTX_MAIN, 
    MUSIC_COUNT = MUSIC_CTX_COUNT 
} MusicId;

/* Ciclo de vida del subsistema de audio */
void Audio_Init(void);
void Audio_Shutdown(void);
void Audio_Update(float dt);

/* Gestion de musica basada en contexto */
void Audio_SetMusicContext(AudioMusicContext ctx);
AudioMusicContext Audio_GetMusicContext(void);

/* Reproduccion de SFX */
void Audio_PlaySfx(SfxId id);
void Audio_PlaySfxByName(const char* name);
void Audio_PlayCombatEnd(int endReason);
void Audio_PlayStepBiome(int biome);

/* Control de volumen y ambiente */
void Audio_SetVolumes(float master, float sfx, float music);
void Audio_SetAmbientTimeOfDay(TimeOfDay tod);
void Audio_TriggerRandomAmbient(void);

/* Funciones legacy / transicionales mantenidas para compatibilidad */
void Audio_PlayMusic(MusicId id, float fadeSeconds);
void Audio_PlayPhaseMusic(GamePhase phase, float fadeSeconds);
void Audio_PlayPhaseSFX(GamePhase phase);
void Audio_PlayCombatMusic(int state, float fadeSeconds);
void Audio_PlayCombatSFX(int state);

#endif /* AUDIO_MANAGER_H */
