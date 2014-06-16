#ifndef ENGINE_H
#define ENGINE_H

#include <map>
#include "stringImproved.h"
#include "P.h"

#include "input.h"
#include "windowManager.h"
#include "postProcessManager.h"
#include "scriptInterface.h"
#include "resources.h"
#include "soundManager.h"
#include "textureManager.h"
#include "gameEntity.h"
#include "Collisionable.h"
#include "random.h"
#include "vectorUtils.h"
#include "multiplayer.h"
#include "event.h"

class Engine;
extern Engine* engine;

class Engine
{
    WindowManager* windowManager;
    float q;
    
    std::map<string, P<PObject> > objectMap;
    float elapsedTime;
    float gameSpeed;
public:
    Engine();
    ~Engine();
    
    void setGameSpeed(float speed);
    float getGameSpeed();
    float getElapsedTime();

    void registerObject(string name, P<PObject> obj);
    P<PObject> getObject(string name);
    
    void runMainLoop();
    void shutdown();
};

#endif//ENGINE_H
