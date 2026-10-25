#include "presentation/audio_manager.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_SFX_ENTRIES 96
#define MAX_CACHED_SOUNDS 32
#define MAX_PATH_LEN 160

/* Configuración de una entrada SFX */
typedef struct {
    char name[48];
    char file[48];
    float volume;
    int minIntervalMs;
    float pitchVariance;
    double lastPlayTimeMs;
    bool missingWarned;
} SfxConfig;

/* Slot de sonido en caché */
typedef struct {
    char filepath[MAX_PATH_LEN];
    Sound sound;
    bool loaded;
    double lastUsed;
} CachedSound;

/* Estado global de Audio */
static float s_masterVol = 1.0f;
static float s_musicVol = 0.7f;
static float s_sfxVol = 0.9f;
static float s_fadeSeconds = 0.8f;
static bool s_logAudio = true;

/* Gestor de contexto musical */
static AudioMusicContext s_currentMusicCtx = MUSIC_CTX_NONE;
static AudioMusicContext s_targetMusicCtx = MUSIC_CTX_NONE;

/* Stream musical activo y stream anterior (durante fade) */
static Music s_musicStream = {0};
static char s_musicPath[MAX_PATH_LEN] = {0};
static float s_musicCurrentVol = 0.0f;
static float s_musicTargetVol = 0.0f;

/* Stream especial para retención de main_theme (pausa y reanudación) */
static Music s_mainThemeStream = {0};
static bool s_mainThemeLoaded = false;
static bool s_mainThemePaused = false;
static float s_mainThemeCurrentVol = 0.0f;
static float s_mainThemeTargetVol = 0.0f;

/* Fading secundario para transición suave */
static Music s_fadeStream = {0};
static float s_fadeStreamVol = 0.0f;
static bool s_hasFadeStream = false;

/* Tabla de configuración SFX */
static SfxConfig s_sfxConfigs[MAX_SFX_ENTRIES];
static int s_sfxConfigCount = 0;

/* Caché de sonidos en memoria */
static CachedSound s_sfxCache[MAX_CACHED_SOUNDS] = {0};

/* Configuración y estado de SFX Ambientales */
static bool s_ambientEnabled = true;
static float s_ambientMinInterval = 10.0f;
static float s_ambientMaxInterval = 25.0f;
static float s_ambientVolume = 0.6f;
static float s_ambientTimer = 0.0f;
static TimeOfDay s_ambientTimeOfDay = TIME_MORNING;
static unsigned int s_ambientRngState = 987654321;

static float AmbientRandomFloat(float minVal, float maxVal) {
    s_ambientRngState ^= s_ambientRngState << 13;
    s_ambientRngState ^= s_ambientRngState >> 17;
    s_ambientRngState ^= s_ambientRngState << 5;
    float norm = (float)(s_ambientRngState & 0x7FFFFFFF) / (float)0x7FFFFFFF;
    return minVal + norm * (maxVal - minVal);
}

/* Nombres estándar de contexto musical */
static const char* MusicContextNames[MUSIC_CTX_COUNT] = {
    "none",
    "menu_theme",
    "main_theme",
    "combat_theme",
    "city_theme"
};

/* Configuración de pistas musicales */
typedef struct {
    char name[48];
    char file[64];
    float volume;
} MusicConfig;

static MusicConfig s_musicConfigs[MUSIC_CTX_COUNT];

/* Nombres canónicos de SFX según enum SfxId */
static const char* SfxIdNames[SFX_COUNT] = {
    "combat_start",
    "combat_end",
    "damage_dealt",
    "market_buy",
    "divine_card_acquired",
    "ui_click",
    "ui_hover",
    "menu_click",
    "ui_confirm",
    "dice_roll",
    "dice_land",
    "step_move",
    "turn_end",
    "night_fall",
    "event_open",
    "event_choice",
    "forage",
    "camp_rest",
    "night_march",
    "damage_taken",
    "critical_hit",
    "market_sell",
    "market_denied",
    "altar_offer",
    "curse_acquired",
    "card_sacrificed",
    "promote_troops",
    "heal",
    "inn_rest",
    "siege_start",
    "siege_victory",
    "enemy_alert",
    "low_supply_warning",
    "mutiny_warning",
    "level_complete",
    "game_over",
    "ambient_crow",
    "ambient_wind",
    "ambient_birds",
    "ambient_insects",
    "ambient_animals"
};

/* ==============================================================================
   RESOLUCIÓN DE ARCHIVOS (Jerarquía .mp3 -> .ogg -> .wav)
   ============================================================================== */
static bool CheckAudioCandidate(const char* folder, const char* fileName, char* outPath, int outSize, long* outModTime) {
    const char* exts[] = { ".mp3", ".ogg", ".wav", ".MP3", ".OGG", ".WAV" };
    char testPath[MAX_PATH_LEN];
    for (int i = 0; i < 6; i++) {
        snprintf(testPath, sizeof(testPath), "%s/%s%s", folder, fileName, exts[i]);
        if (FileExists(testPath)) {
            strncpy(outPath, testPath, outSize - 1);
            outPath[outSize - 1] = '\0';
            if (outModTime) *outModTime = GetFileModTime(testPath);
            return true;
        }
    }
    // Si fileName ya incluye extensión
    snprintf(testPath, sizeof(testPath), "%s/%s", folder, fileName);
    if (FileExists(testPath)) {
        strncpy(outPath, testPath, outSize - 1);
        outPath[outSize - 1] = '\0';
        if (outModTime) *outModTime = GetFileModTime(testPath);
        return true;
    }
    return false;
}

static bool ResolveAudioFile(const char* baseFolder, const char* altFolder, const char* fileName, char* outPath, int outSize) {
    char path1[MAX_PATH_LEN] = {0};
    char path2[MAX_PATH_LEN] = {0};
    long time1 = 0;
    long time2 = 0;

    bool found1 = CheckAudioCandidate(baseFolder, fileName, path1, sizeof(path1), &time1);
    bool found2 = (altFolder != NULL) ? CheckAudioCandidate(altFolder, fileName, path2, sizeof(path2), &time2) : false;

    if (found1 && found2) {
        // Si existe en ambas carpetas, priorizar el archivo modificado más recientemente
        if (time2 > time1) {
            strncpy(outPath, path2, outSize - 1);
        } else {
            strncpy(outPath, path1, outSize - 1);
        }
        outPath[outSize - 1] = '\0';
        return true;
    }

    if (found1) {
        strncpy(outPath, path1, outSize - 1);
        outPath[outSize - 1] = '\0';
        return true;
    }

    if (found2) {
        strncpy(outPath, path2, outSize - 1);
        outPath[outSize - 1] = '\0';
        return true;
    }

    outPath[0] = '\0';
    return false;
}

static bool ResolveSfxFile(const char* sfxName, char* outPath, int outSize) {
    if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", sfxName, outPath, outSize)) {
        return true;
    }
    /* Fallbacks inteligentes para variantes comunes */
    if (strcmp(sfxName, "combat_start") == 0) {
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "combat_enter_init", outPath, outSize)) return true;
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "combat_enter", outPath, outSize)) return true;
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "combat_init", outPath, outSize)) return true;
    } else if (strcmp(sfxName, "ambient_crow") == 0) {
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "crow", outPath, outSize)) return true;
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "cuervo", outPath, outSize)) return true;
    } else if (strcmp(sfxName, "ambient_wind") == 0) {
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "wind", outPath, outSize)) return true;
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "viento", outPath, outSize)) return true;
    } else if (strcmp(sfxName, "ambient_birds") == 0) {
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "birds", outPath, outSize)) return true;
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "pajaros", outPath, outSize)) return true;
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "bird", outPath, outSize)) return true;
    } else if (strcmp(sfxName, "ambient_insects") == 0) {
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "insects", outPath, outSize)) return true;
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "insectos", outPath, outSize)) return true;
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "crickets", outPath, outSize)) return true;
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "grillos", outPath, outSize)) return true;
    } else if (strcmp(sfxName, "ambient_animals") == 0) {
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "animals", outPath, outSize)) return true;
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "animales", outPath, outSize)) return true;
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "wolf", outPath, outSize)) return true;
        if (ResolveAudioFile("assets/audio/sfx", "assets/sfx", "lobo", outPath, outSize)) return true;
    }
    return false;
}

static bool ResolveMusicFile(const char* musicName, char* outPath, int outSize) {
    if (ResolveAudioFile("assets/audio/music", "assets/music", musicName, outPath, outSize)) {
        return true;
    }
    /* Fallbacks inteligentes para pistas comunes */
    if (strcmp(musicName, "combat_theme") == 0) {
        if (ResolveAudioFile("assets/audio/music", "assets/music", "combat_init", outPath, outSize)) return true;
        if (ResolveAudioFile("assets/audio/music", "assets/music", "combat", outPath, outSize)) return true;
    } else if (strcmp(musicName, "main_theme") == 0) {
        if (ResolveAudioFile("assets/audio/music", "assets/music", "map_theme", outPath, outSize)) return true;
        if (ResolveAudioFile("assets/audio/music", "assets/music", "exploration_theme", outPath, outSize)) return true;
    } else if (strcmp(musicName, "city_theme") == 0) {
        if (ResolveAudioFile("assets/audio/music", "assets/music", "town_theme", outPath, outSize)) return true;
        if (ResolveAudioFile("assets/audio/music", "assets/music", "settlement_theme", outPath, outSize)) return true;
    }
    return false;
}

/* ==============================================================================
   CONFIGURACIÓN INI (assets/audio.cfg)
   ============================================================================== */
static SfxConfig* GetOrCreateSfxConfig(const char* name) {
    for (int i = 0; i < s_sfxConfigCount; i++) {
        if (strcmp(s_sfxConfigs[i].name, name) == 0) {
            return &s_sfxConfigs[i];
        }
    }
    if (s_sfxConfigCount < MAX_SFX_ENTRIES) {
        SfxConfig* cfg = &s_sfxConfigs[s_sfxConfigCount++];
        memset(cfg, 0, sizeof(SfxConfig));
        strncpy(cfg->name, name, sizeof(cfg->name) - 1);
        strncpy(cfg->file, name, sizeof(cfg->file) - 1);
        cfg->volume = 1.0f;
        cfg->minIntervalMs = 0;
        cfg->pitchVariance = 0.0f;
        cfg->lastPlayTimeMs = -10000.0;
        cfg->missingWarned = false;
        return cfg;
    }
    return NULL;
}

static void TrimString(char* str) {
    char* p = str;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != str) memmove(str, p, strlen(p) + 1);
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
}

static void LoadAudioConfig(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        TraceLog(LOG_INFO, "[AUDIO] No se encontro %s, usando valores por defecto.", filename);
        return;
    }

    char line[256];
    char currentSection[64] = {0};

    while (fgets(line, sizeof(line), f)) {
        TrimString(line);
        if (line[0] == '\0' || line[0] == ';' || line[0] == '#') continue;

        if (line[0] == '[' && line[strlen(line) - 1] == ']') {
            line[strlen(line) - 1] = '\0';
            strncpy(currentSection, line + 1, sizeof(currentSection) - 1);
            TrimString(currentSection);
            continue;
        }

        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char key[64];
        char val[64];
        strncpy(key, line, sizeof(key) - 1);
        strncpy(val, eq + 1, sizeof(val) - 1);
        TrimString(key);
        TrimString(val);

        if (strcmp(currentSection, "VOLUME") == 0) {
            if (strcmp(key, "Master") == 0) s_masterVol = (float)atof(val);
            else if (strcmp(key, "Music") == 0) s_musicVol = (float)atof(val);
            else if (strcmp(key, "Sfx") == 0) s_sfxVol = (float)atof(val);
        } else if (strcmp(currentSection, "MUSIC") == 0) {
            if (strcmp(key, "FadeSeconds") == 0) s_fadeSeconds = (float)atof(val);
        } else if (strcmp(currentSection, "DEBUG") == 0) {
            if (strcmp(key, "LogAudio") == 0) s_logAudio = (atoi(val) != 0);
        } else if (strcmp(currentSection, "AMBIENT") == 0) {
            if (strcmp(key, "Enabled") == 0) s_ambientEnabled = (atoi(val) != 0);
            else if (strcmp(key, "MinIntervalSeconds") == 0) s_ambientMinInterval = (float)atof(val);
            else if (strcmp(key, "MaxIntervalSeconds") == 0) s_ambientMaxInterval = (float)atof(val);
            else if (strcmp(key, "Volume") == 0) s_ambientVolume = (float)atof(val);
        } else if (strncmp(currentSection, "MUSIC_", 6) == 0) {
            const char* trackName = currentSection + 6;
            for (int i = 1; i < MUSIC_CTX_COUNT; i++) {
                if (strcmp(MusicContextNames[i], trackName) == 0) {
                    if (strcmp(key, "File") == 0) {
                        strncpy(s_musicConfigs[i].file, val, sizeof(s_musicConfigs[i].file) - 1);
                    } else if (strcmp(key, "Volume") == 0) {
                        s_musicConfigs[i].volume = (float)atof(val);
                    }
                    break;
                }
            }
        } else if (strncmp(currentSection, "SFX_", 4) == 0) {
            const char* sfxName = currentSection + 4;
            SfxConfig* cfg = GetOrCreateSfxConfig(sfxName);
            if (cfg) {
                if (strcmp(key, "File") == 0) strncpy(cfg->file, val, sizeof(cfg->file) - 1);
                else if (strcmp(key, "Volume") == 0) cfg->volume = (float)atof(val);
                else if (strcmp(key, "MinIntervalMs") == 0) cfg->minIntervalMs = atoi(val);
                else if (strcmp(key, "PitchVariance") == 0) cfg->pitchVariance = (float)atof(val);
            }
        }
    }
    fclose(f);
}

/* ==============================================================================
   INICIALIZACIÓN Y SHUTDOWN
   ============================================================================== */
void Audio_Init(void) {
    InitAudioDevice();

    /* Inicializar configuración de música por defecto */
    for (int i = 0; i < MUSIC_CTX_COUNT; i++) {
        strncpy(s_musicConfigs[i].name, MusicContextNames[i], sizeof(s_musicConfigs[i].name) - 1);
        strncpy(s_musicConfigs[i].file, MusicContextNames[i], sizeof(s_musicConfigs[i].file) - 1);
        s_musicConfigs[i].volume = 1.0f;
    }

    /* Inicializar lista básica con nombres canónicos */
    for (int i = 0; i < SFX_COUNT; i++) {
        GetOrCreateSfxConfig(SfxIdNames[i]);
    }

    LoadAudioConfig("assets/audio.cfg");

    SetMasterVolume(s_masterVol);

    /* Inicializar temporizador de ambiente aleatorio */
    s_ambientRngState = (unsigned int)(GetTime() * 1000.0) ^ 0xA5A55A5A;
    if (s_ambientRngState == 0) s_ambientRngState = 987654321;
    s_ambientTimer = AmbientRandomFloat(s_ambientMinInterval, s_ambientMaxInterval);

    /* Comprobación de archivos faltantes al inicio */
    TraceLog(LOG_INFO, "[AUDIO] --- Auditoria de Archivos de Audio al Iniciar ---");
    char pathBuf[MAX_PATH_LEN];

    /* Auditoria de musica esperada */
    for (int i = 1; i < MUSIC_CTX_COUNT; i++) {
        if (!ResolveMusicFile(s_musicConfigs[i].file, pathBuf, sizeof(pathBuf))) {
            TraceLog(LOG_WARNING, "[AUDIO] Musica esperada faltante: '%s' (archivo '%s' no encontrado en assets/music/)", MusicContextNames[i], s_musicConfigs[i].file);
        } else {
            TraceLog(LOG_INFO, "[AUDIO] Musica disponible: '%s' (archivo '%s') -> %s", MusicContextNames[i], s_musicConfigs[i].file, pathBuf);
        }
    }

    /* Auditoria de sfx esperados */
    int missingCount = 0;
    for (int i = 0; i < s_sfxConfigCount; i++) {
        if (!ResolveSfxFile(s_sfxConfigs[i].file, pathBuf, sizeof(pathBuf))) {
            TraceLog(LOG_WARNING, "[AUDIO] SFX esperado faltante: '%s' (archivo '%s' buscado en assets/sfx/)", s_sfxConfigs[i].name, s_sfxConfigs[i].file);
            missingCount++;
        } else {
            TraceLog(LOG_INFO, "[AUDIO] SFX disponible: '%s' -> %s", s_sfxConfigs[i].name, pathBuf);
        }
    }
    TraceLog(LOG_INFO, "[AUDIO] Auditoria finalizada. Total SFX configurados: %d, Faltantes: %d", s_sfxConfigCount, missingCount);
}

void Audio_Shutdown(void) {
    /* Liberar caché de SFX */
    for (int i = 0; i < MAX_CACHED_SOUNDS; i++) {
        if (s_sfxCache[i].loaded) {
            UnloadSound(s_sfxCache[i].sound);
            s_sfxCache[i].loaded = false;
            s_sfxCache[i].filepath[0] = '\0';
        }
    }

    /* Liberar música en reproducción */
    if (s_musicStream.stream.buffer != NULL) {
        StopMusicStream(s_musicStream);
        UnloadMusicStream(s_musicStream);
        memset(&s_musicStream, 0, sizeof(Music));
    }

    /* Liberar main theme retenido */
    if (s_mainThemeStream.stream.buffer != NULL) {
        StopMusicStream(s_mainThemeStream);
        UnloadMusicStream(s_mainThemeStream);
        memset(&s_mainThemeStream, 0, sizeof(Music));
    }

    /* Liberar stream de fade secundario */
    if (s_fadeStream.stream.buffer != NULL) {
        StopMusicStream(s_fadeStream);
        UnloadMusicStream(s_fadeStream);
        memset(&s_fadeStream, 0, sizeof(Music));
    }

    CloseAudioDevice();
}

/* ==============================================================================
   SISTEMA DE MÚSICA BASADO EN CONTEXTO (STREAMING, FADE Y PAUSE/RESUME)
   Garantía estricta: Solo suena 1 pista musical a la vez.
   ============================================================================== */
void Audio_SetMusicContext(AudioMusicContext targetCtx) {
    if (!IsAudioDeviceReady()) return;
    if (targetCtx == s_currentMusicCtx) return;

    AudioMusicContext prevCtx = s_currentMusicCtx;
    s_currentMusicCtx = targetCtx;
    s_targetMusicCtx = targetCtx;

    if (s_logAudio) {
        TraceLog(LOG_INFO, "[AUDIO_DEBUG] Cambio contexto musical: %s -> %s (Fade: %.2fs)",
                 MusicContextNames[prevCtx], MusicContextNames[targetCtx], s_fadeSeconds);
    }

    /* Caso A: Cambio a MAIN_THEME (Exploración en mapa) */
    if (targetCtx == MUSIC_CTX_MAIN) {
        // Detener y descargar inmediatamente cualquier música de combate/ciudad anterior
        if (s_musicStream.stream.buffer != NULL) {
            StopMusicStream(s_musicStream);
            UnloadMusicStream(s_musicStream);
            memset(&s_musicStream, 0, sizeof(Music));
            s_musicPath[0] = '\0';
        }
        if (s_fadeStream.stream.buffer != NULL) {
            StopMusicStream(s_fadeStream);
            UnloadMusicStream(s_fadeStream);
            memset(&s_fadeStream, 0, sizeof(Music));
            s_hasFadeStream = false;
        }

        // Si ya teníamos main_theme cargado y pausado, reanudarlo donde quedó
        if (s_mainThemeLoaded && s_mainThemeStream.stream.buffer != NULL) {
            if (s_mainThemePaused) {
                ResumeMusicStream(s_mainThemeStream);
                s_mainThemePaused = false;
            }
            s_mainThemeCurrentVol = 1.0f;
            s_mainThemeTargetVol = 1.0f;
            float mainTrackVol = s_musicConfigs[MUSIC_CTX_MAIN].volume;
            SetMusicVolume(s_mainThemeStream, mainTrackVol * s_musicVol * s_masterVol);
            return;
        }

        // Si no estaba cargado aún, cargarlo exclusivamente en s_mainThemeStream
        const char* trackName = s_musicConfigs[MUSIC_CTX_MAIN].file;
        char resolvedPath[MAX_PATH_LEN];
        if (ResolveMusicFile(trackName, resolvedPath, sizeof(resolvedPath))) {
            s_mainThemeStream = LoadMusicStream(resolvedPath);
            if (s_mainThemeStream.stream.buffer != NULL) {
                s_mainThemeStream.looping = true;
                s_mainThemeLoaded = true;
                s_mainThemePaused = false;
                s_mainThemeCurrentVol = 1.0f;
                s_mainThemeTargetVol = 1.0f;
                float mainTrackVol = s_musicConfigs[MUSIC_CTX_MAIN].volume;
                SetMusicVolume(s_mainThemeStream, mainTrackVol * s_musicVol * s_masterVol);
                PlayMusicStream(s_mainThemeStream);
            }
        }
        return;
    }

    /* Caso B: Salir de MAIN_THEME hacia Combate, Ciudad o Menú */
    if (prevCtx == MUSIC_CTX_MAIN) {
        // Pausar main_theme para retener su posición exacta y no reiniciarlo
        if (s_mainThemeLoaded && s_mainThemeStream.stream.buffer != NULL) {
            PauseMusicStream(s_mainThemeStream);
            s_mainThemePaused = true;
        }
    } else {
        // Si no venía de main_theme, detener y descargar s_musicStream
        if (s_musicStream.stream.buffer != NULL) {
            StopMusicStream(s_musicStream);
            UnloadMusicStream(s_musicStream);
            memset(&s_musicStream, 0, sizeof(Music));
            s_musicPath[0] = '\0';
        }
    }

    if (s_fadeStream.stream.buffer != NULL) {
        StopMusicStream(s_fadeStream);
        UnloadMusicStream(s_fadeStream);
        memset(&s_fadeStream, 0, sizeof(Music));
        s_hasFadeStream = false;
    }

    if (targetCtx == MUSIC_CTX_NONE) return;

    /* Cargar la nueva pista de música según el contexto (s_musicStream) */
    const char* trackName = (targetCtx >= 0 && targetCtx < MUSIC_CTX_COUNT) ? s_musicConfigs[targetCtx].file : MusicContextNames[targetCtx];
    char resolvedPath[MAX_PATH_LEN];
    if (ResolveMusicFile(trackName, resolvedPath, sizeof(resolvedPath))) {
        s_musicStream = LoadMusicStream(resolvedPath);
        if (s_musicStream.stream.buffer != NULL) {
            s_musicStream.looping = true;
            s_musicCurrentVol = 1.0f;
            s_musicTargetVol = 1.0f;
            float trackVol = (targetCtx >= 0 && targetCtx < MUSIC_CTX_COUNT) ? s_musicConfigs[targetCtx].volume : 1.0f;
            SetMusicVolume(s_musicStream, trackVol * s_musicVol * s_masterVol);
            PlayMusicStream(s_musicStream);
            strncpy(s_musicPath, resolvedPath, sizeof(s_musicPath) - 1);
            if (s_logAudio) {
                TraceLog(LOG_INFO, "[AUDIO_DEBUG] Pista musical iniciada: %s (vol=%.2f, trackVol=%.2f)",
                         resolvedPath, s_musicCurrentVol, trackVol);
            }
        }
    } else {
        static bool warnedMusicMissing[MUSIC_CTX_COUNT] = {false};
        if (!warnedMusicMissing[targetCtx]) {
            TraceLog(LOG_INFO, "[AUDIO] Pista musical '%s' (archivo '%s') no encontrada (silenciada).",
                     MusicContextNames[targetCtx], trackName);
            warnedMusicMissing[targetCtx] = true;
        }
    }
}

AudioMusicContext Audio_GetMusicContext(void) {
    return s_currentMusicCtx;
}

void Audio_Update(float dt) {
    if (!IsAudioDeviceReady()) return;

    /* 1. Si estamos en el mapa, SOLO reproducir/actualizar s_mainThemeStream (1 sola cancion) */
    if (s_currentMusicCtx == MUSIC_CTX_MAIN) {
        if (s_mainThemeLoaded && !s_mainThemePaused && s_mainThemeStream.stream.buffer != NULL) {
            UpdateMusicStream(s_mainThemeStream);
            if (!IsMusicStreamPlaying(s_mainThemeStream)) {
                PlayMusicStream(s_mainThemeStream);
            }
            float mainTrackVol = s_musicConfigs[MUSIC_CTX_MAIN].volume;
            SetMusicVolume(s_mainThemeStream, mainTrackVol * s_musicVol * s_masterVol);
        }
    } else {
        /* 2. Si estamos en otro contexto (Menú, Ciudad, Combate), SOLO actualizar s_musicStream */
        if (s_musicStream.stream.buffer != NULL) {
            UpdateMusicStream(s_musicStream);
            if (!IsMusicStreamPlaying(s_musicStream)) {
                PlayMusicStream(s_musicStream);
            }
            float trackVol = (s_currentMusicCtx >= 0 && s_currentMusicCtx < MUSIC_CTX_COUNT) ? s_musicConfigs[s_currentMusicCtx].volume : 1.0f;
            SetMusicVolume(s_musicStream, trackVol * s_musicVol * s_masterVol);
        }
    }

    /* 3. Actualizar temporizador de SFX de ambiente (solo en exploracion de mapa) */
    if (s_ambientEnabled && s_currentMusicCtx == MUSIC_CTX_MAIN) {
        s_ambientTimer -= dt;
        if (s_ambientTimer <= 0.0f) {
            Audio_TriggerRandomAmbient();
            float minI = (s_ambientMinInterval > 1.0f) ? s_ambientMinInterval : 1.0f;
            float maxI = (s_ambientMaxInterval >= minI) ? s_ambientMaxInterval : minI + 5.0f;
            s_ambientTimer = AmbientRandomFloat(minI, maxI);
        }
    }
}

/* ==============================================================================
   REPRODUCCIÓN DE SFX CON SATURACIÓN, INTERVALO Y VARIACIÓN DE TONO
   ============================================================================== */
static Sound GetOrLoadSfxSound(const char* filepath) {
    double now = GetTime();
    int oldestIdx = 0;
    double oldestTime = 1e12;

    for (int i = 0; i < MAX_CACHED_SOUNDS; i++) {
        if (s_sfxCache[i].loaded && strcmp(s_sfxCache[i].filepath, filepath) == 0) {
            s_sfxCache[i].lastUsed = now;
            return s_sfxCache[i].sound;
        }
        if (!s_sfxCache[i].loaded) {
            Sound snd = LoadSound(filepath);
            if (snd.frameCount > 0) {
                s_sfxCache[i].sound = snd;
                s_sfxCache[i].loaded = true;
                s_sfxCache[i].lastUsed = now;
                strncpy(s_sfxCache[i].filepath, filepath, sizeof(s_sfxCache[i].filepath) - 1);
                return snd;
            }
            return (Sound){0};
        }
        if (s_sfxCache[i].lastUsed < oldestTime) {
            oldestTime = s_sfxCache[i].lastUsed;
            oldestIdx = i;
        }
    }

    /* Expulsar slot más antiguo de la caché */
    UnloadSound(s_sfxCache[oldestIdx].sound);
    s_sfxCache[oldestIdx].loaded = false;
    Sound snd = LoadSound(filepath);
    if (snd.frameCount > 0) {
        s_sfxCache[oldestIdx].sound = snd;
        s_sfxCache[oldestIdx].loaded = true;
        s_sfxCache[oldestIdx].lastUsed = now;
        strncpy(s_sfxCache[oldestIdx].filepath, filepath, sizeof(s_sfxCache[oldestIdx].filepath) - 1);
        return snd;
    }
    return (Sound){0};
}

void Audio_PlaySfxByName(const char* name) {
    if (!IsAudioDeviceReady() || !name) return;

    SfxConfig* cfg = GetOrCreateSfxConfig(name);
    if (!cfg) return;

    /* Control de saturación por intervalo mínimo */
    double nowMs = GetTime() * 1000.0;
    if (cfg->minIntervalMs > 0 && (nowMs - cfg->lastPlayTimeMs) < (double)cfg->minIntervalMs) {
        return;
    }

    char resolvedPath[MAX_PATH_LEN];
    if (!ResolveSfxFile(cfg->file, resolvedPath, sizeof(resolvedPath))) {
        if (!cfg->missingWarned) {
            TraceLog(LOG_INFO, "[AUDIO] Archivo faltante para SFX '%s' (silenciado).", cfg->file);
            cfg->missingWarned = true;
        }
        return;
    }

    Sound snd = GetOrLoadSfxSound(resolvedPath);
    if (snd.frameCount <= 0) return;

    /* Tono natural (sin variación de pitch) */
    SetSoundPitch(snd, 1.0f);

    /* Ajuste de volumen efectivo */
    float ambientFactor = (strncmp(name, "ambient_", 8) == 0) ? s_ambientVolume : 1.0f;
    float effectiveVol = s_masterVol * s_sfxVol * cfg->volume * ambientFactor;
    SetSoundVolume(snd, effectiveVol);

    PlaySound(snd);
    cfg->lastPlayTimeMs = nowMs;

    if (s_logAudio) {
        TraceLog(LOG_INFO, "[AUDIO_DEBUG] Play SFX: '%s' (Archivo: %s, Vol: %.2f)",
                 name, resolvedPath, effectiveVol);
    }
}

void Audio_PlaySfx(SfxId id) {
    if (id < 0 || id >= SFX_COUNT) return;
    Audio_PlaySfxByName(SfxIdNames[id]);
}

void Audio_PlayCombatEnd(int endReason) {
    const char* variant = "combat_end";
    if (endReason == COMBAT_END_VICTORY) {
        variant = "combat_end_victory";
    } else if (endReason == COMBAT_END_DEFEAT) {
        variant = "combat_end_defeat";
    } else if (endReason == COMBAT_END_FLEE) {
        variant = "combat_end_flee";
    }

    char pathBuf[MAX_PATH_LEN];
    if (ResolveSfxFile(variant, pathBuf, sizeof(pathBuf))) {
        Audio_PlaySfxByName(variant);
    } else {
        Audio_PlaySfxByName("combat_end");
    }
}

void Audio_PlayStepBiome(int biome) {
    const char* biomeNames[] = { "plains", "forest", "desert", "jungle", "mountain", "water" };
    if (biome >= 0 && biome < 6) {
        char variantName[48];
        snprintf(variantName, sizeof(variantName), "step_%s", biomeNames[biome]);
        char pathBuf[MAX_PATH_LEN];
        if (ResolveSfxFile(variantName, pathBuf, sizeof(pathBuf))) {
            Audio_PlaySfxByName(variantName);
            return;
        }
    }
    Audio_PlaySfxByName("step_move");
}

void Audio_SetAmbientTimeOfDay(TimeOfDay tod) {
    s_ambientTimeOfDay = tod;
}

void Audio_TriggerRandomAmbient(void) {
    static const SfxId dayPool[] = {
        SFX_AMBIENT_BIRDS,
        SFX_AMBIENT_CROW,
        SFX_AMBIENT_WIND,
        SFX_AMBIENT_ANIMALS
    };
    static const SfxId nightPool[] = {
        SFX_AMBIENT_INSECTS,
        SFX_AMBIENT_WIND,
        SFX_AMBIENT_ANIMALS,
        SFX_AMBIENT_CROW
    };

    const SfxId* pool = (s_ambientTimeOfDay == TIME_NIGHT) ? nightPool : dayPool;
    int poolCount = (s_ambientTimeOfDay == TIME_NIGHT) ? (int)(sizeof(nightPool) / sizeof(nightPool[0]))
                                                       : (int)(sizeof(dayPool) / sizeof(dayPool[0]));

    SfxId available[8];
    int availableCount = 0;
    char pathBuf[MAX_PATH_LEN];

    for (int i = 0; i < poolCount; i++) {
        SfxId id = pool[i];
        const char* name = SfxIdNames[id];
        SfxConfig* cfg = GetOrCreateSfxConfig(name);
        const char* fileToResolve = (cfg && cfg->file[0] != '\0') ? cfg->file : name;
        if (ResolveSfxFile(fileToResolve, pathBuf, sizeof(pathBuf))) {
            available[availableCount++] = id;
        }
    }

    if (availableCount > 0) {
        int choice = (int)AmbientRandomFloat(0.0f, (float)availableCount);
        if (choice < 0) choice = 0;
        if (choice >= availableCount) choice = availableCount - 1;
        Audio_PlaySfx(available[choice]);
    }
}

void Audio_SetVolumes(float master, float sfx, float music) {
    s_masterVol = master;
    s_sfxVol = sfx;
    s_musicVol = music;
    SetMasterVolume(s_masterVol);
    if (s_musicStream.stream.buffer != NULL) {
        float trackVol = (s_currentMusicCtx >= 0 && s_currentMusicCtx < MUSIC_CTX_COUNT) ? s_musicConfigs[s_currentMusicCtx].volume : 1.0f;
        SetMusicVolume(s_musicStream, s_musicCurrentVol * trackVol * s_musicVol * s_masterVol);
    }
    if (s_mainThemeStream.stream.buffer != NULL) {
        float mainTrackVol = s_musicConfigs[MUSIC_CTX_MAIN].volume;
        SetMusicVolume(s_mainThemeStream, s_mainThemeCurrentVol * mainTrackVol * s_musicVol * s_masterVol);
    }
}

/* ==============================================================================
   FUNCIONES TRANSICIONALES / DE COMPATIBILIDAD
   ============================================================================== */
void Audio_PlayMusic(MusicId id, float fadeSeconds) {
    if (fadeSeconds > 0.0f) s_fadeSeconds = fadeSeconds;
    Audio_SetMusicContext((AudioMusicContext)id);
}

void Audio_PlayPhaseMusic(GamePhase phase, float fadeSeconds) {
    if (fadeSeconds > 0.0f) s_fadeSeconds = fadeSeconds;
    switch (phase) {
        case PHASE_MENU:
            Audio_SetMusicContext(MUSIC_CTX_MENU);
            break;
        case PHASE_PLAYER:
        case PHASE_ENEMY:
        case PHASE_DRAFT:
            Audio_SetMusicContext(MUSIC_CTX_MAIN);
            break;
        case PHASE_CITY:
            Audio_SetMusicContext(MUSIC_CTX_CITY);
            break;
        case PHASE_GAME_OVER:
            Audio_PlaySfxByName("game_over");
            break;
        case PHASE_VICTORY:
            Audio_PlaySfxByName("level_complete");
            break;
        default:
            break;
    }
}

void Audio_PlayPhaseSFX(GamePhase phase) {
    switch (phase) {
        case PHASE_GAME_OVER:
            Audio_PlaySfxByName("game_over");
            break;
        case PHASE_VICTORY:
            Audio_PlaySfxByName("level_complete");
            break;
        default:
            break;
    }
}

void Audio_PlayCombatMusic(int state, float fadeSeconds) {
    (void)state;
    if (fadeSeconds > 0.0f) s_fadeSeconds = fadeSeconds;
    Audio_SetMusicContext(MUSIC_CTX_COMBAT);
}

void Audio_PlayCombatSFX(int state) {
    if (state == 0) {
        Audio_PlaySfxByName("combat_start");
    }
}
