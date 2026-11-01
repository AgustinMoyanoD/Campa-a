#ifndef ENGINE_RNG_H
#define ENGINE_RNG_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
   ENGINE RNG: GENERADOR PSEUDOALEATORIO DETERMINISTA (PCG32)
   Independiente de raylib, puro, portable y reproducible por semilla.
   ============================================================ */

typedef struct Rng {
    uint64_t state;
    uint64_t inc;
} Rng;

void     Rng_Init(Rng* rng, uint64_t seed);
uint32_t Rng_Next(Rng* rng);
int      Rng_Range(Rng* rng, int min, int max);       /* [min, max] inclusive */
float    Rng_Float(Rng* rng);                         /* [0.0f, 1.0f) */
int      Rng_RollDice(Rng* rng, int count, int sides);

#endif /* ENGINE_RNG_H */
