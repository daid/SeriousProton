#ifndef ENGINE_H
#define ENGINE_H

#include <unordered_map>
#include "stringImproved.h"
#include "P.h"


class Engine;
union SDL_Event;
extern Engine* engine;

class Engine
{
public:
    class EngineTiming
    {
    public:
        float update;
        float collision;
        float render;
        float server_update;
    };
private:
    bool running;
    
    std::unordered_map<string, P<PObject> > objectMap;
    float elapsedTime;
    float gameSpeed;
    
    EngineTiming last_engine_timing;
public:
    Engine();
    ~Engine();
    
    void setGameSpeed(float speed);
    float getGameSpeed();
    float getElapsedTime();
    EngineTiming getEngineTiming();

    void registerObject(string name, P<PObject> obj);
    P<PObject> getObject(string name);
    
    void runMainLoop();
    void shutdown();
private:
    void handleEvent(SDL_Event& event);
};

#endif//ENGINE_H
