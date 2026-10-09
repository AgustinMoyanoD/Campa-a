#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <stdbool.h>

typedef struct {
    float duration;
    void (*on_start)(void *ctx);
    void (*on_update)(void *ctx, float t); // t in [0,1]
    void (*on_end)(void *ctx);
    void *ctx;
} SeqAction;

void Sequence_Push(SeqAction a);
void Sequence_Update(float dt);
bool Sequence_IsBusy(void);
void Sequence_Skip(void);

#endif
