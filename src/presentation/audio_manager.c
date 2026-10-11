#include "presentation/audio_manager.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>

#define MAX_CACHED_SOUNDS 16
typedef struct {
    char filepath[128];
    Sound sound;
} CachedSound;

static CachedSound sfxCache[MAX_CACHED_SOUNDS] = {0};

static Music currentMusic = {0};
static char currentMusicPath[128] = {0};

static void PlayMusicFile(const char* filepath) {
    if (strcmp(currentMusicPath, filepath) == 0) return; // Ya esta sonando

    if (!FileExists(filepath)) {
        // Si no existe el archivo que se solicita, detener la musica actual y dejar en silencio
        if ((currentMusic.stream.buffer != NULL)) {
            StopMusicStream(currentMusic);
            UnloadMusicStream(currentMusic);
            memset(&currentMusic, 0, sizeof(Music));
            memset(currentMusicPath, 0, sizeof(currentMusicPath));
        }
        return;
    }
    
    if ((currentMusic.stream.buffer != NULL)) {
        StopMusicStream(currentMusic);
        UnloadMusicStream(currentMusic);
    }
    
    currentMusic = LoadMusicStream(filepath);
    if ((currentMusic.stream.buffer != NULL)) {
        PlayMusicStream(currentMusic);
        strcpy(currentMusicPath, filepath);
    }
}

static void PlaySFXFile(const char* filepath) {
    if (!FileExists(filepath)) return;
    
    // Buscar en cache
    for (int i = 0; i < MAX_CACHED_SOUNDS; i++) {
        if (sfxCache[i].filepath[0] != '\0' && strcmp(sfxCache[i].filepath, filepath) == 0) {
            PlaySound(sfxCache[i].sound);
            return;
        }
    }
    
    // Encontrar slot libre
    int slot = -1;
    for (int i = 0; i < MAX_CACHED_SOUNDS; i++) {
        if (sfxCache[i].filepath[0] == '\0') {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        UnloadSound(sfxCache[0].sound);
        slot = 0;
    }
    
    sfxCache[slot].sound = LoadSound(filepath);
    if ((sfxCache[slot].sound.frameCount > 0)) {
        strcpy(sfxCache[slot].filepath, filepath);
        PlaySound(sfxCache[slot].sound);
    }
}

static float masterVol = 1.0f;
static float sfxVol = 1.0f;
static float musicVol = 1.0f;

void Audio_Init(void) {
    InitAudioDevice();
    // Load sounds here (Placeholders for now)
}

void Audio_Shutdown(void) {
    // Unload sounds here
    CloseAudioDevice();
}

void Audio_Update(float dt) {
    if ((currentMusic.stream.buffer != NULL)) {
        UpdateMusicStream(currentMusic);
    }
}

void Audio_PlaySfx(SfxId id) {
    if (!IsAudioDeviceReady()) return;
    // Play sound placeholder
}

void Audio_PlayMusic(MusicId id, float fadeSeconds) {
    if (!IsAudioDeviceReady()) return;
    // Play music placeholder
}

void Audio_PlayPhaseMusic(GamePhase phase, float fadeSeconds) {
    if (!IsAudioDeviceReady()) return;
    
    const char* phaseNames[] = {
        "menu", "player", "enemy", "event", "draft", "game_over", "victory"
    };
    
    if (phase >= 0 && phase <= PHASE_VICTORY) {
        char filename[128];
        sprintf(filename, "assets/audio/music/phase_%s.mp3", phaseNames[phase]);
        TraceLog(LOG_INFO, "[AUDIO] Solicitando musica: %s (Fade %.1fs)", filename, fadeSeconds);
        PlayMusicFile(filename);    }
}

void Audio_PlayPhaseSFX(GamePhase phase) {
    if (!IsAudioDeviceReady()) return;
    
    const char* phaseNames[] = {
        "menu", "player", "enemy", "event", "draft", "game_over", "victory"
    };
    
    if (phase >= 0 && phase <= PHASE_VICTORY && phase != PHASE_ENEMY) {
        char filename[128];
        sprintf(filename, "assets/audio/sfx/enter_%s.wav", phaseNames[phase]);
        TraceLog(LOG_INFO, "[AUDIO] Solicitando SFX: %s", filename);
        PlaySFXFile(filename);    }
}

void Audio_PlayCombatMusic(int state, float fadeSeconds) {
    if (!IsAudioDeviceReady()) return;
    const char* stateNames[] = { "init", "eval", "plan", "exec", "close", "end", "recount", "done" };
    if (state >= 0 && state <= 7) {
        char filename[128];
        sprintf(filename, "assets/audio/music/combat_%s.mp3", stateNames[state]);
        TraceLog(LOG_INFO, "[AUDIO] Solicitando musica de combate: %s (Fade %.1f)", filename, fadeSeconds);
        PlayMusicFile(filename);    }
}

void Audio_PlayCombatSFX(int state) {
    if (!IsAudioDeviceReady()) return;
    const char* stateNames[] = { "init", "eval", "plan", "exec", "close", "end", "recount", "done" };
    if (state >= 0 && state <= 7) {
        char filename[128];
        sprintf(filename, "assets/audio/sfx/combat_enter_%s.wav", stateNames[state]);
        TraceLog(LOG_INFO, "[AUDIO] Solicitando SFX de combate: %s", filename);
        PlaySFXFile(filename);    }
}

void Audio_SetVolumes(float master, float sfx, float music) {
    masterVol = master;
    sfxVol = sfx;
    musicVol = music;
    SetMasterVolume(masterVol);
}
