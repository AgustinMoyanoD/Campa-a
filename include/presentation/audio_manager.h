#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "controllers/game_master.h"
typedef enum { 
    SFX_DICE_ROLL, SFX_DICE_LAND, SFX_UI_CLICK, SFX_UI_HOVER, SFX_CARD_TRIGGER,
    SFX_HIT, SFX_CASUALTY, SFX_STEP, SFX_EVENT_OPEN, SFX_COUNT 
} SfxId;

typedef enum { MUSIC_MAP, MUSIC_COMBAT, MUSIC_EVENT, MUSIC_COUNT } MusicId;

void Audio_Init(void);
void Audio_Shutdown(void);
void Audio_Update(float dt);
void Audio_PlaySfx(SfxId id);
void Audio_PlayMusic(MusicId id, float fadeSeconds);
void Audio_PlayPhaseMusic(GamePhase phase, float fadeSeconds);
void Audio_PlayPhaseSFX(GamePhase phase);
void Audio_PlayCombatMusic(int state, float fadeSeconds);
void Audio_PlayCombatSFX(int state);
void Audio_SetVolumes(float master, float sfx, float music);

#endif
