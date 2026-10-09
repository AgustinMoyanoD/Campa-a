#include "presentation/sequence.h"
#include <stddef.h>

#define MAX_QUEUE 64

static SeqAction queue[MAX_QUEUE];
static int qHead = 0;
static int qTail = 0;
static bool currentActive = false;
static float currentElapsed = 0.0f;

void Sequence_Push(SeqAction a) {
    int nextTail = (qTail + 1) % MAX_QUEUE;
    if (nextTail != qHead) {
        queue[qTail] = a;
        qTail = nextTail;
    }
}

void Sequence_Update(float dt) {
    if (!currentActive) {
        if (qHead == qTail) return; // Empty
        
        currentActive = true;
        currentElapsed = 0.0f;
        if (queue[qHead].on_start) queue[qHead].on_start(queue[qHead].ctx);
    }
    
    currentElapsed += dt;
    float t = queue[qHead].duration > 0.001f ? (currentElapsed / queue[qHead].duration) : 1.0f;
    if (t > 1.0f) t = 1.0f;
    
    if (queue[qHead].on_update) queue[qHead].on_update(queue[qHead].ctx, t);
    
    if (t >= 1.0f) {
        if (queue[qHead].on_end) queue[qHead].on_end(queue[qHead].ctx);
        currentActive = false;
        qHead = (qHead + 1) % MAX_QUEUE;
    }
}

bool Sequence_IsBusy(void) {
    return currentActive || (qHead != qTail);
}

void Sequence_Skip(void) {
    while (Sequence_IsBusy()) {
        if (currentActive && queue[qHead].on_end) {
            queue[qHead].on_end(queue[qHead].ctx);
        } else if (!currentActive && queue[qHead].on_start) {
            // For skipped actions that haven't started, we might just call end if they need state resolution
            if (queue[qHead].on_end) queue[qHead].on_end(queue[qHead].ctx);
        }
        currentActive = false;
        qHead = (qHead + 1) % MAX_QUEUE;
    }
}
