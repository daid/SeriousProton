#include "timer.h"
#include "engine.h"

namespace sp {

EngineTime::time_point EngineTime::now()
{
    return EngineTime::time_point(engine ? engine->getElapsedTime() : 0.0f);
}

} //!namespace