#ifndef RNG_H
#define RNG_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
   RNG DETERMINISTA CON SEMILLA (I5)
   Generador de números pseudoaleatorios puro (PCG32/XorShift64)
   Completamente independiente de raylib y sin variables globales.
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

#endif /* RNG_H */
