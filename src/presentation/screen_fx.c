#include "presentation/screen_fx.h"
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
static Color flashColor = BLANK;

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
            mapParticles[i].vel.x *= (1.0f - 2.5f * dt);
            mapParticles[i].vel.y *= (1.0f - 2.5f * dt);
        }
    }
}

void ScreenFX_FadeToBlack(float duration) {
    fadeTimer = duration;
    fadeDuration = duration;
    fadingToBlack = true;
    currentFade = 0.0f;
}

void ScreenFX_FadeFromBlack(float duration) {
    fadeTimer = duration;
    fadeDuration = duration;
    fadingToBlack = false;
    currentFade = 1.0f;
}

void ScreenFX_Shake(float duration, float intensity) {
    shakeTimer = duration;
    shakeDuration = duration;
    shakeIntensity = intensity;
}

void ScreenFX_Flash(float duration, Color color) {
    flashTimer = duration;
    flashDuration = duration;
    flashColor = color;
}

void ScreenFX_ApplyShakeToCamera(Camera2D* cam) {
    cam->target.x += shakeOffset.x;
    cam->target.y += shakeOffset.y;
}

Vector2 ScreenFX_GetShakeOffset(void) {
    return shakeOffset;
}

void ScreenFX_SpawnMarchDust(Vector2 worldPos, Color terrainTint) {
    int spawned = 0;
    for (int i = 0; i < MAX_MAP_PARTICLES && spawned < 5; i++) {
        if (mapParticles[i].life <= 0.0f) {
            float angle = (float)(rand() % 360) * (PI / 180.0f);
            float speed = 14.0f + (float)(rand() % 22);
            mapParticles[i].pos = (Vector2){ worldPos.x + (rand()%14 - 7), worldPos.y + (rand()%8 - 4) };
            mapParticles[i].vel = (Vector2){ cosf(angle) * speed, sinf(angle) * speed * 0.5f - 9.0f };
            mapParticles[i].maxLife = 0.35f + (float)(rand() % 20) / 100.0f;
            mapParticles[i].life = mapParticles[i].maxLife;
            mapParticles[i].size = 5.0f + (float)(rand() % 5);
            Color c = terrainTint;
            if (c.r == 0 && c.g == 0 && c.b == 0) c = (Color){ 190, 165, 120, 255 };
            mapParticles[i].color = c;
            spawned++;
        }
    }
}

void ScreenFX_DrawMapParticles(void) {
    for (int i = 0; i < MAX_MAP_PARTICLES; i++) {
        if (mapParticles[i].life > 0.0f) {
            float progress = 1.0f - (mapParticles[i].life / mapParticles[i].maxLife);
            float currentSize = mapParticles[i].size * (1.0f + progress * 0.7f);
            float alpha = (1.0f - progress);
            Color c = mapParticles[i].color;
            c.a = (unsigned char)(170.0f * alpha);
            DrawCircleV(mapParticles[i].pos, currentSize, c);
        }
    }
}

void ScreenFX_DrawNightAtmosphere(int screenWidth, int screenHeight) {
    int border = 90;
    DrawRectangleGradientV(0, 0, screenWidth, border, Fade((Color){4, 7, 18, 255}, 0.55f), Fade((Color){4, 7, 18, 255}, 0.0f));
    DrawRectangleGradientV(0, screenHeight - border, screenWidth, border, Fade((Color){4, 7, 18, 255}, 0.0f), Fade((Color){4, 7, 18, 255}, 0.55f));
    DrawRectangleGradientH(0, 0, border, screenHeight, Fade((Color){4, 7, 18, 255}, 0.55f), Fade((Color){4, 7, 18, 255}, 0.0f));
    DrawRectangleGradientH(screenWidth - border, 0, border, screenHeight, Fade((Color){4, 7, 18, 255}, 0.0f), Fade((Color){4, 7, 18, 255}, 0.55f));
}

void ScreenFX_DrawOverlays(void) {
    if (flashTimer > 0.0f) {
        float alpha = flashTimer / flashDuration;
        Color c = flashColor;
        c.a = (unsigned char)(255 * alpha);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), c);
    }
    
    if (currentFade > 0.0f) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, currentFade));
    }
}
