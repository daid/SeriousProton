#ifndef ENGINE_H
#define ENGINE_H

#include <unordered_map>
#include "stringImproved.h"
#include "P.h"

#include "input.h"
#include "windowManager.h"
#include "postProcessManager.h"
#include "shaderManager.h"
#include "scriptInterface.h"
#include "resources.h"
#include "soundManager.h"
#include "textureManager.h"
#include "gameEntity.h"
#include "collisionable.h"
#include "random.h"
#include "vectorUtils.h"
#include "multiplayer.h"
#include "multiplayer_server.h"
#include "multiplayer_server_scanner.h"
#include "multiplayer_client.h"
#include "event.h"
#include "logging.h"
#include "tween.h"
#include "clipboard.h"

class Engine;
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
    WindowManager* windowManager;
    
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
    void handleEvent(sf::Event& event);
};

#endif//ENGINE_H
