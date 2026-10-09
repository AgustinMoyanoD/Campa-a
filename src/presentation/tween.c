#include "presentation/tween.h"
#include <stddef.h>

static Tween tweens[MAX_TWEENS] = {0};

void Tween_StartEx(float *target, float to, float duration, EaseFn ease, void (*on_done)(void *), void *user) {
    if (!target) return;
    
    // Replace existing target
    for (int i = 0; i < MAX_TWEENS; i++) {
        if (tweens[i].active && tweens[i].target == target) {
            tweens[i].active = false;
        }
    }
    
    // Find free slot
    for (int i = 0; i < MAX_TWEENS; i++) {
        if (!tweens[i].active) {
            tweens[i].target = target;
            tweens[i].from = *target;
            tweens[i].to = to;
            tweens[i].duration = duration;
            tweens[i].elapsed = 0.0f;
            tweens[i].ease = ease ? ease : Ease_Linear;
            tweens[i].on_done = on_done;
            tweens[i].user = user;
            tweens[i].active = true;
            return;
        }
    }
}

void Tween_Start(float *target, float to, float duration, EaseFn ease) {
    Tween_StartEx(target, to, duration, ease, NULL, NULL);
}

void Tween_Update(float dt) {
    for (int i = 0; i < MAX_TWEENS; i++) {
        if (!tweens[i].active) continue;
        
        tweens[i].elapsed += dt;
        float t = tweens[i].duration > 0.001f ? (tweens[i].elapsed / tweens[i].duration) : 1.0f;
        
        if (t >= 1.0f) {
            *tweens[i].target = tweens[i].to;
            tweens[i].active = false;
            if (tweens[i].on_done) tweens[i].on_done(tweens[i].user);
        } else {
            float eased = tweens[i].ease(t);
            *tweens[i].target = tweens[i].from + (tweens[i].to - tweens[i].from) * eased;
        }
    }
}

void Tween_CompleteAll(void) {
    for (int i = 0; i < MAX_TWEENS; i++) {
        if (tweens[i].active) {
            *tweens[i].target = tweens[i].to;
            tweens[i].active = false;
            if (tweens[i].on_done) tweens[i].on_done(tweens[i].user);
        }
    }
}

float Ease_Linear(float t) { return t; }
float Ease_OutCubic(float t) { t--; return t*t*t + 1.0f; }
float Ease_InOutQuad(float t) { return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; }
float Ease_OutBack(float t) { const float c1 = 1.70158f; const float c3 = c1 + 1.0f; t--; return 1.0f + c3 * t * t * t + c1 * t * t; }
