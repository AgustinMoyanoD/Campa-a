#include "engine/presentation/screen_fx.h"
#include <math.h>
#include <stdlib.h>

static float fadeTimer = 0.0f;
static float fadeDuration = 0.0f;
static bool fadingToBlack = false;
static float currentFade = 0.0f;

static float shakeTimer = 0.0f;
static float shakeDuration = 0.0f;
static float shakeIntensity = 0.0f;
static Vector2 shakeOffset = {0};

static float flashTimer = 0.0f;
static float flashDuration = 0.0f;
static Color flashColor = { 0, 0, 0, 0 };

#define MAX_MAP_PARTICLES 48
typedef struct {
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    float size;
    Color color;
} MapParticle;

static MapParticle mapParticles[MAX_MAP_PARTICLES] = {0};

void ScreenFX_Init(void) {
    fadeTimer = 0.0f;
    shakeTimer = 0.0f;
    shakeDuration = 0.0f;
    flashTimer = 0.0f;
    currentFade = 0.0f;
    shakeOffset = (Vector2){0,0};
    for (int i = 0; i < MAX_MAP_PARTICLES; i++) {
        mapParticles[i].life = 0.0f;
    }
}

void ScreenFX_Update(float dt) {
    if (fadeTimer > 0.0f) {
        fadeTimer -= dt;
        float t = 1.0f - (fadeTimer / fadeDuration);
        if (t > 1.0f) t = 1.0f;
        currentFade = fadingToBlack ? t : (1.0f - t);
    }
    
    if (shakeTimer > 0.0f) {
        shakeTimer -= dt;
        float amt = (shakeDuration > 0.0f) ? (shakeTimer / shakeDuration) : 0.0f;
        if (amt > 1.0f) amt = 1.0f;
        if (amt < 0.0f) amt = 0.0f;
        shakeOffset.x = ((float)(rand()%100)/100.0f * 2.0f - 1.0f) * shakeIntensity * amt;
        shakeOffset.y = ((float)(rand()%100)/100.0f * 2.0f - 1.0f) * shakeIntensity * amt;
    } else {
        shakeOffset = (Vector2){0,0};
    }
    
    if (flashTimer > 0.0f) {
        flashTimer -= dt;
    }
    
    for (int i = 0; i < MAX_MAP_PARTICLES; i++) {
        if (mapParticles[i].life > 0.0f) {
            mapParticles[i].life -= dt;
            mapParticles[i].pos.x += mapParticles[i].vel.x * dt;
            mapParticles[i].pos.y += mapParticles[i].vel.y * dt;
        }
    }
}

void ScreenFX_FadeToBlack(float duration) {
    fadeDuration = duration > 0.01f ? duration : 0.01f;
    fadeTimer = fadeDuration;
    fadingToBlack = true;
}

void ScreenFX_FadeFromBlack(float duration) {
    fadeDuration = duration > 0.01f ? duration : 0.01f;
    fadeTimer = fadeDuration;
    fadingToBlack = false;
    currentFade = 1.0f;
}

void ScreenFX_Shake(float duration, float intensity) {
    shakeDuration = duration;
    shakeTimer = duration;
    shakeIntensity = intensity;
}

void ScreenFX_Flash(float duration, Color color) {
    flashDuration = duration;
    flashTimer = duration;
    flashColor = color;
}

void ScreenFX_ApplyShakeToCamera(Camera2D* cam) {
    if (!cam) return;
    cam->target.x += shakeOffset.x;
    cam->target.y += shakeOffset.y;
}

Vector2 ScreenFX_GetShakeOffset(void) {
    return shakeOffset;
}

void ScreenFX_DrawOverlays(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    
    if (flashTimer > 0.0f && flashDuration > 0.0f) {
        float alpha = flashTimer / flashDuration;
        Color c = flashColor;
        c.a = (unsigned char)((float)c.a * alpha);
        DrawRectangle(0, 0, sw, sh, c);
    }
    
    if (currentFade > 0.001f) {
        DrawRectangle(0, 0, sw, sh, Fade(BLACK, currentFade));
    }
}

void ScreenFX_SpawnMarchDust(Vector2 worldPos, Color terrainTint) {
    for (int p = 0; p < 4; p++) {
        for (int i = 0; i < MAX_MAP_PARTICLES; i++) {
            if (mapParticles[i].life <= 0.0f) {
                mapParticles[i].pos = worldPos;
                mapParticles[i].pos.x += ((float)(rand()%40) - 20.0f);
                mapParticles[i].pos.y += ((float)(rand()%20) - 10.0f);
                mapParticles[i].vel = (Vector2){
                    ((float)(rand()%60) - 30.0f),
                    -((float)(rand()%40) + 10.0f)
                };
                mapParticles[i].maxLife = 0.4f + ((float)(rand()%30)/100.0f);
                mapParticles[i].life = mapParticles[i].maxLife;
                mapParticles[i].size = 3.0f + ((float)(rand()%4));
                mapParticles[i].color = terrainTint;
                break;
            }
        }
    }
}

void ScreenFX_DrawMapParticles(void) {
    for (int i = 0; i < MAX_MAP_PARTICLES; i++) {
        if (mapParticles[i].life > 0.0f) {
            float alpha = mapParticles[i].life / mapParticles[i].maxLife;
            Color c = mapParticles[i].color;
            c.a = (unsigned char)((float)c.a * alpha * 0.7f);
            DrawCircleV(mapParticles[i].pos, mapParticles[i].size * alpha, c);
        }
    }
}

void ScreenFX_DrawNightAtmosphere(int screenWidth, int screenHeight) {
    Color nightBlue = (Color){ 10, 15, 35, 110 };
    DrawRectangle(0, 0, screenWidth, screenHeight, nightBlue);
}
