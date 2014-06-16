#ifndef RANDOM_H
#define RANDOM_H

#include <stdlib.h>
#include <time.h>
#include <stdint.h>

static inline void initRandom()
{
    srand(time(NULL));
}

static inline float random(float fmin, float fmax)
{
    return (float(rand()) / float(RAND_MAX)) * (fmax - fmin) + fmin;
}

static inline int irandom(int imin, int imax)
{
    return (int64_t(rand()) * int64_t(imax - imin + 1) / (int64_t(RAND_MAX) + 1)) + imin;
}

#endif//RANDOM_H
