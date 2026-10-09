#include "presentation/audio_manager.h"
#include "raylib.h"
#include <stdio.h>

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
    // Update music streams
}

void Audio_PlaySfx(SfxId id) {
    if (!IsAudioDeviceReady()) return;
    // Play sound placeholder
}

void Audio_PlayMusic(MusicId id, float fadeSeconds) {
    if (!IsAudioDeviceReady()) return;
    // Play music placeholder
}

void Audio_SetVolumes(float master, float sfx, float music) {
    masterVol = master;
    sfxVol = sfx;
    musicVol = music;
    SetMasterVolume(masterVol);
}
