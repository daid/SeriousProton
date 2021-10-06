#include "engine.h"
#include "random.h"
#include "gameEntity.h"
#include "Updatable.h"
#include "collisionable.h"
#include "audio/source.h"

#include <SDL.h>

#ifdef DEBUG
#include <typeinfo>
int DEBUG_PobjCount;
PObject* DEBUG_PobjListStart;
#endif

#ifdef WIN32
#include <windows.h>

namespace
{
    HINSTANCE exchndl = nullptr;
}
#endif

Engine* engine;

Engine::Engine()
{
    engine = this;

#ifdef WIN32
    // Setup crash reporter (Dr. MinGW) if available.
    exchndl = LoadLibrary(TEXT("exchndl.dll"));

    if (exchndl)
    {
        auto pfnExcHndlInit = GetProcAddress(exchndl, "ExcHndlInit");

        if (pfnExcHndlInit)
        {
            pfnExcHndlInit();
            LOG(INFO) << "Crash Reporter ON";
        }
        else
        {
            LOG(WARNING) << "Failed to initialize Crash Reporter";
            FreeLibrary(exchndl);
            exchndl = nullptr;
        }
    } 
#endif // WIN32

    SDL_Init(SDL_INIT_EVERYTHING);
    atexit(SDL_Quit);

    initRandom();
    windowManager = nullptr;
    CollisionManager::initialize();
    InputHandler::initialize();
    gameSpeed = 1.0;
    running = true;
    elapsedTime = 0.0;
    soundManager = new SoundManager();
}

Engine::~Engine()
{
    if (windowManager)
        windowManager->close();
    delete soundManager;
    soundManager = nullptr;

#ifdef WIN32
    if (exchndl)
    {
        FreeLibrary(exchndl);
        exchndl = nullptr;
    }
#endif // WIN32
}

void Engine::registerObject(string name, P<PObject> obj)
{
    objectMap[name] = obj;
}

P<PObject> Engine::getObject(string name)
{
    if (!objectMap[name])
        return NULL;
    return objectMap[name];
}

void Engine::runMainLoop()
{
    windowManager = dynamic_cast<WindowManager*>(*getObject("windowManager"));
    if (!windowManager)
    {
        sp::SystemStopwatch frame_timer;
        while(running)
        {
            float delta = frame_timer.restart();
            if (delta > 0.5f)
                delta = 0.5f;
            if (delta < 0.001f)
                delta = 0.001f;
            delta *= gameSpeed;

            foreach(Updatable, u, updatableList)
                u->update(delta);
            elapsedTime += delta;
            CollisionManager::handleCollisions(delta);
            ScriptObject::clearDestroyedObjects();
            soundManager->updateTick();
            
            std::this_thread::sleep_for(std::chrono::duration<float>(1.f/60.f - delta));
        }
    }else{
        sp::audio::Source::startAudioSystem();
        sp::SystemStopwatch frame_timer;
#ifdef DEBUG
        sp::SystemTimer debug_output_timer;
        debug_output_timer.repeat(5);
#endif
        while(running)
        {
            InputHandler::preEventsUpdate();
            // Handle events
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                handleEvent(event);
            }
            InputHandler::postEventsUpdate();

#ifdef DEBUG
            if (debug_output_timer.isExpired())
                printf("Object count: %4d %4zd\n", DEBUG_PobjCount, updatableList.size());
#endif

            float delta = frame_timer.restart();
            if (delta > 0.5f)
                delta = 0.5f;
            if (delta < 0.001f)
                delta = 0.001f;
            delta *= gameSpeed;
#ifdef DEBUG
#warning TODO SDL2
/*
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Tab))
                delta /= 5.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Tilde))
                delta *= 5.f;
*/
#endif
            EngineTiming engine_timing;
            
            sp::SystemStopwatch engine_timing_stopwatch;
            foreach(Updatable, u, updatableList) {
                u->update(delta);
            }
            elapsedTime += delta;
            engine_timing.update = engine_timing_stopwatch.restart();
            CollisionManager::handleCollisions(delta);
            engine_timing.collision = engine_timing_stopwatch.restart();
            ScriptObject::clearDestroyedObjects();
            soundManager->updateTick();

            // Clear the window
            windowManager->render();
            engine_timing.render = engine_timing_stopwatch.restart();
            engine_timing.server_update = 0.0f;
            if (game_server)
                engine_timing.server_update = game_server->getUpdateTime();
            
            last_engine_timing = engine_timing;
        }
        soundManager->stopMusic();
    }
}

void Engine::handleEvent(SDL_Event& event)
{
    // Window closed: exit
    if (event.type == SDL_QUIT)
        running = false;
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
        windowManager->windowHasFocus = true;
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
        windowManager->windowHasFocus = false;
#ifdef DEBUG
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
        running = false;
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_l)
    {
        int n = 0;
        printf("------------------------\n");
        std::unordered_map<string,int> totals;
        for(PObject* obj = DEBUG_PobjListStart; obj; obj = obj->DEBUG_PobjListNext)
        {
            printf("%c%4d: %4d: %s\n", obj->isDestroyed() ? '>' : ' ', n++, obj->getRefCount(), typeid(*obj).name());
            if (!obj->isDestroyed())
            {
                totals[typeid(*obj).name()]=totals[typeid(*obj).name()]+1;
            }
        }
        printf("--non-destroyed totals--\n");
        int grand_total=0;
        for (auto entry : totals)
        {
            printf("%4d %s\n", entry.second, entry.first.c_str());
            grand_total+=entry.second;
        }
        printf("%4d %s\n",grand_total,"All PObjects");
        printf("------------------------\n");
    }
#endif
    InputHandler::handleEvent(event);
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
        windowManager->setupView();
}

void Engine::setGameSpeed(float speed)
{
    gameSpeed = speed;
}

float Engine::getGameSpeed()
{
    return gameSpeed;
}

float Engine::getElapsedTime()
{
    return elapsedTime;
}

Engine::EngineTiming Engine::getEngineTiming()
{
    return last_engine_timing;
}

void Engine::shutdown()
{
    running = false;
}
