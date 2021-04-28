#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <random>

#include "random.h"

static std::mt19937_64 random_engine;

void initRandom()
{
    random_engine.seed(time(NULL));
}

float random(float fmin, float fmax)
{
    return std::uniform_real_distribution<float>(fmin, fmax)(random_engine);
}

int irandom(int imin, int imax)
{
    return std::uniform_int_distribution<>(imin, imax)(random_engine);
}
