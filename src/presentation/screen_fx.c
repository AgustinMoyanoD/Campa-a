#include "presentation/screen_fx.h"
#include <math.h>
#include <stdlib.h>

static float fadeTimer = 0.0f;
static float fadeDuration = 0.0f;
static bool fadingToBlack = false;
static float currentFade = 0.0f;

static float shakeTimer = 0.0f;
static float shakeIntensity = 0.0f;
static Vector2 shakeOffset = {0};

static float flashTimer = 0.0f;
static float flashDuration = 0.0f;
static Color flashColor = BLANK;

void ScreenFX_Init(void) {
    fadeTimer = 0.0f;
    shakeTimer = 0.0f;
    flashTimer = 0.0f;
    currentFade = 0.0f;
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
        float amt = (shakeTimer / 0.5f); // decays
        if (amt > 1.0f) amt = 1.0f;
        shakeOffset.x = ((float)(rand()%100)/100.0f * 2.0f - 1.0f) * shakeIntensity * amt;
        shakeOffset.y = ((float)(rand()%100)/100.0f * 2.0f - 1.0f) * shakeIntensity * amt;
    } else {
        shakeOffset = (Vector2){0,0};
    }
    
    if (flashTimer > 0.0f) {
        flashTimer -= dt;
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
