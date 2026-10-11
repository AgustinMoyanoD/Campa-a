#include "models/rng.h"

void Rng_Init(Rng* rng, uint64_t seed) {
    if (!rng) return;
    rng->state = seed;
    rng->inc = 1442695040888963407ULL | 1ULL;
    Rng_Next(rng);
}

uint32_t Rng_Next(Rng* rng) {
    if (!rng) return 0;
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = (uint32_t)(oldstate >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

int Rng_Range(Rng* rng, int min, int max) {
    if (min >= max) return min;
    uint32_t span = (uint32_t)(max - min + 1);
    return min + (int)(Rng_Next(rng) % span);
}

float Rng_Float(Rng* rng) {
    return (float)Rng_Next(rng) / 4294967296.0f;
}

int Rng_RollDice(Rng* rng, int count, int sides) {
    if (count <= 0 || sides <= 0) return 0;
    int total = 0;
    for (int i = 0; i < count; i++) {
        total += Rng_Range(rng, 1, sides);
    }
    return total;
}
