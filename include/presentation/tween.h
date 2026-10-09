#ifndef TWEEN_H
#define TWEEN_H

#include <stdbool.h>

typedef float (*EaseFn)(float t); // t en [0,1]

typedef struct {
    float *target;
    float from, to, duration, elapsed;
    EaseFn ease;
    void (*on_done)(void *user);
    void *user;
    bool active;
} Tween;

#define MAX_TWEENS 128

void Tween_Start(float *target, float to, float duration, EaseFn ease);
void Tween_StartEx(float *target, float to, float duration, EaseFn ease, void (*on_done)(void *), void *user);
void Tween_Update(float dt);
void Tween_CompleteAll(void);

float Ease_Linear(float t);
float Ease_OutCubic(float t);
float Ease_InOutQuad(float t);
float Ease_OutBack(float t);

#endif
