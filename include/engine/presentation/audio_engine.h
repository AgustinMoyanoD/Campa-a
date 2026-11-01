#ifndef ENGINE_AUDIO_ENGINE_H
#define ENGINE_AUDIO_ENGINE_H

#include <stdbool.h>

/* ============================================================
   ENGINE AUDIO: MOTOR DE AUDIO GENÉRICO Y STREAMING MUSICAL
   Independiente de los enums y estados de Campaña.
   ============================================================ */

#define AUDIO_MAX_CACHED_SOUNDS 48
#define AUDIO_MAX_SFX_CONFIGS   96

/* Inicialización del dispositivo de audio y cargador */
void AudioEngine_Init(void);
void AudioEngine_Shutdown(void);
void AudioEngine_Update(float dt);

/* Control de volúmenes globales (0.0f a 1.0f) */
void AudioEngine_SetVolumes(float master, float sfx, float music);
float AudioEngine_GetMasterVolume(void);
float AudioEngine_GetSfxVolume(void);
float AudioEngine_GetMusicVolume(void);

/* Reproducción de efectos de sonido (SFX) */
void AudioEngine_PlaySfx(const char* name);
void AudioEngine_PlaySfxFile(const char* filepath, float volume, float pitch);

/* Reproducción y fundido de música */
void AudioEngine_PlayMusicFile(const char* filepath, float fadeSeconds, bool loop);
void AudioEngine_StopMusic(float fadeSeconds);
void AudioEngine_PauseMusic(void);
void AudioEngine_ResumeMusic(void);

/* Carga de configuraciones desde manifest o archivo ini */
bool AudioEngine_LoadConfig(const char* configFilePath);

#endif /* ENGINE_AUDIO_ENGINE_H */
