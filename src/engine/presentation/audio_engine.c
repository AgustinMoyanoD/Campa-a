#include "engine/presentation/audio_engine.h"
#include "engine/core/config_parser.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    char name[48];
    char filepath[128];
    float volume;
    float pitchVariance;
    double minIntervalSec;
    double lastPlayedTime;
} EngineSfxDef;

typedef struct {
    char filepath[128];
    Sound sound;
    bool loaded;
} CachedSoundEntry;

static float s_masterVol = 1.0f;
static float s_sfxVol = 0.9f;
static float s_musicVol = 0.7f;

static EngineSfxDef s_sfxDefs[AUDIO_MAX_SFX_CONFIGS];
static int s_sfxCount = 0;

static CachedSoundEntry s_soundCache[AUDIO_MAX_CACHED_SOUNDS];
static int s_cacheCount = 0;

static Music s_currentMusic = {0};
static bool s_musicLoaded = false;
static float s_musicCurrentFadeVol = 0.0f;
static float s_musicTargetFadeVol = 0.0f;
static float s_musicFadeSpeed = 1.0f;

void AudioEngine_Init(void) {
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }
    s_sfxCount = 0;
    s_cacheCount = 0;
    s_musicLoaded = false;
    s_musicCurrentFadeVol = 0.0f;
    s_musicTargetFadeVol = 0.0f;

    AudioEngine_LoadConfig("assets/audio.cfg");
}

void AudioEngine_Shutdown(void) {
    if (s_musicLoaded) {
        StopMusicStream(s_currentMusic);
        UnloadMusicStream(s_currentMusic);
        s_musicLoaded = false;
    }
    for (int i = 0; i < s_cacheCount; i++) {
        if (s_soundCache[i].loaded) {
            UnloadSound(s_soundCache[i].sound);
            s_soundCache[i].loaded = false;
        }
    }
    s_cacheCount = 0;
    s_sfxCount = 0;
    if (IsAudioDeviceReady()) {
        CloseAudioDevice();
    }
}

void AudioEngine_Update(float dt) {
    if (s_musicLoaded) {
        UpdateMusicStream(s_currentMusic);

        if (s_musicCurrentFadeVol < s_musicTargetFadeVol) {
            s_musicCurrentFadeVol += s_musicFadeSpeed * dt;
            if (s_musicCurrentFadeVol > s_musicTargetFadeVol) s_musicCurrentFadeVol = s_musicTargetFadeVol;
            SetMusicVolume(s_currentMusic, s_musicCurrentFadeVol * s_musicVol * s_masterVol);
        } else if (s_musicCurrentFadeVol > s_musicTargetFadeVol) {
            s_musicCurrentFadeVol -= s_musicFadeSpeed * dt;
            if (s_musicCurrentFadeVol < s_musicTargetFadeVol) s_musicCurrentFadeVol = s_musicTargetFadeVol;
            SetMusicVolume(s_currentMusic, s_musicCurrentFadeVol * s_musicVol * s_masterVol);
            if (s_musicCurrentFadeVol <= 0.001f && s_musicTargetFadeVol <= 0.001f) {
                StopMusicStream(s_currentMusic);
            }
        }
    }
}

void AudioEngine_SetVolumes(float master, float sfx, float music) {
    s_masterVol = master < 0.0f ? 0.0f : (master > 1.0f ? 1.0f : master);
    s_sfxVol = sfx < 0.0f ? 0.0f : (sfx > 1.0f ? 1.0f : sfx);
    s_musicVol = music < 0.0f ? 0.0f : (music > 1.0f ? 1.0f : music);

    if (s_musicLoaded) {
        SetMusicVolume(s_currentMusic, s_musicCurrentFadeVol * s_musicVol * s_masterVol);
    }
}

float AudioEngine_GetMasterVolume(void) { return s_masterVol; }
float AudioEngine_GetSfxVolume(void)    { return s_sfxVol; }
float AudioEngine_GetMusicVolume(void)  { return s_musicVol; }

static Sound* GetOrLoadSound(const char* filepath) {
    if (!filepath || !filepath[0]) return NULL;

    for (int i = 0; i < s_cacheCount; i++) {
        if (s_soundCache[i].loaded && strcmp(s_soundCache[i].filepath, filepath) == 0) {
            return &s_soundCache[i].sound;
        }
    }

    if (s_cacheCount < AUDIO_MAX_CACHED_SOUNDS) {
        if (FileExists(filepath)) {
            Sound snd = LoadSound(filepath);
            int idx = s_cacheCount++;
            strncpy(s_soundCache[idx].filepath, filepath, sizeof(s_soundCache[idx].filepath) - 1);
            s_soundCache[idx].sound = snd;
            s_soundCache[idx].loaded = true;
            return &s_soundCache[idx].sound;
        }
    }
    return NULL;
}

void AudioEngine_PlaySfx(const char* name) {
    if (!name || !name[0]) return;
    double now = GetTime();

    for (int i = 0; i < s_sfxCount; i++) {
        if (strcmp(s_sfxDefs[i].name, name) == 0) {
            if (s_sfxDefs[i].minIntervalSec > 0.0 && (now - s_sfxDefs[i].lastPlayedTime) < s_sfxDefs[i].minIntervalSec) {
                return; // Demasiado pronto
            }
            s_sfxDefs[i].lastPlayedTime = now;
            float pitch = 1.0f;
            if (s_sfxDefs[i].pitchVariance > 0.0f) {
                pitch += ((float)(rand() % 200) / 100.0f - 1.0f) * s_sfxDefs[i].pitchVariance;
            }
            AudioEngine_PlaySfxFile(s_sfxDefs[i].filepath, s_sfxDefs[i].volume, pitch);
            return;
        }
    }

    // Fallback: intentar reproducir directamente como archivo en assets/audio/sfx/
    char directPath[128];
    snprintf(directPath, sizeof(directPath), "assets/audio/sfx/%s.wav", name);
    AudioEngine_PlaySfxFile(directPath, 1.0f, 1.0f);
}

void AudioEngine_PlaySfxFile(const char* filepath, float volume, float pitch) {
    if (!filepath) return;
    Sound* snd = GetOrLoadSound(filepath);
    if (snd) {
        SetSoundVolume(*snd, volume * s_sfxVol * s_masterVol);
        SetSoundPitch(*snd, pitch > 0.1f ? pitch : 1.0f);
        PlaySound(*snd);
    }
}

void AudioEngine_PlayMusicFile(const char* filepath, float fadeSeconds, bool loop) {
    if (!filepath || !FileExists(filepath)) return;

    if (s_musicLoaded) {
        StopMusicStream(s_currentMusic);
        UnloadMusicStream(s_currentMusic);
        s_musicLoaded = false;
    }

    s_currentMusic = LoadMusicStream(filepath);
    s_currentMusic.looping = loop;
    PlayMusicStream(s_currentMusic);
    s_musicLoaded = true;

    if (fadeSeconds > 0.05f) {
        s_musicCurrentFadeVol = 0.0f;
        s_musicTargetFadeVol = 1.0f;
        s_musicFadeSpeed = 1.0f / fadeSeconds;
        SetMusicVolume(s_currentMusic, 0.0f);
    } else {
        s_musicCurrentFadeVol = 1.0f;
        s_musicTargetFadeVol = 1.0f;
        SetMusicVolume(s_currentMusic, s_musicVol * s_masterVol);
    }
}

void AudioEngine_StopMusic(float fadeSeconds) {
    if (!s_musicLoaded) return;
    if (fadeSeconds > 0.05f) {
        s_musicTargetFadeVol = 0.0f;
        s_musicFadeSpeed = 1.0f / fadeSeconds;
    } else {
        StopMusicStream(s_currentMusic);
        s_musicCurrentFadeVol = 0.0f;
    }
}

void AudioEngine_PauseMusic(void) {
    if (s_musicLoaded) PauseMusicStream(s_currentMusic);
}

void AudioEngine_ResumeMusic(void) {
    if (s_musicLoaded) ResumeMusicStream(s_currentMusic);
}

bool AudioEngine_LoadConfig(const char* configFilePath) {
    ConfigFile cfg;
    if (!Config_LoadFile(&cfg, configFilePath)) return false;

    s_masterVol = Config_GetFloat(&cfg, "VOLUME", "Master", 1.0f);
    s_musicVol  = Config_GetFloat(&cfg, "VOLUME", "Music", 0.7f);
    s_sfxVol    = Config_GetFloat(&cfg, "VOLUME", "Sfx", 0.9f);

    s_sfxCount = 0;
    for (int i = 0; i < cfg.count; i++) {
        if (strncmp(cfg.entries[i].section, "SFX_", 4) == 0 && strcmp(cfg.entries[i].key, "File") == 0) {
            const char* sfxName = cfg.entries[i].section + 4;
            const char* fileName = cfg.entries[i].value;

            if (s_sfxCount < AUDIO_MAX_SFX_CONFIGS) {
                EngineSfxDef* def = &s_sfxDefs[s_sfxCount++];
                strncpy(def->name, sfxName, sizeof(def->name) - 1);
                snprintf(def->filepath, sizeof(def->filepath), "assets/audio/sfx/%s.wav", fileName);
                def->volume = Config_GetFloat(&cfg, cfg.entries[i].section, "Volume", 1.0f);
                def->pitchVariance = Config_GetFloat(&cfg, cfg.entries[i].section, "PitchVariance", 0.0f);
                int minMs = Config_GetInt(&cfg, cfg.entries[i].section, "MinIntervalMs", 0);
                def->minIntervalSec = (double)minMs / 1000.0;
                def->lastPlayedTime = 0.0;
            }
        }
    }
    return true;
}
