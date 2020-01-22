#include "engine.h"
#include "random.h"
#include "gameEntity.h"
#include "Updatable.h"
#include "collisionable.h"

#ifdef __linux__
#include <fenv.h>
#endif
#ifdef __WIN32__
#include <float.h>
#endif

#ifdef DEBUG
#include <typeinfo>
int DEBUG_PobjCount;
PObject* DEBUG_PobjListStart;
#endif

#ifdef ENABLE_CRASH_LOGGER
#ifdef __WIN32__
//Exception handler for mingw, from https://github.com/jrfonseca/drmingw
#include <exchndl.h>
#endif//__WIN32__
#endif//ENABLE_CRASH_LOGGER

Engine* engine;

Engine::Engine()
{
    engine = this;
#ifdef ENABLE_CRASH_LOGGER
#ifdef __WIN32__
    ExcHndlInit();
#endif//__WIN32__
#endif//ENABLE_CRASH_LOGGER
#ifdef __linux__
    feenableexcept(FE_DIVBYZERO | FE_INVALID);
#endif
#ifdef __WIN32__
    unsigned int current_word = 0;
    _controlfp_s(&current_word, _EM_INVALID | _EM_ZERODIVIDE, _MCW_EM);
#endif
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
        sf::Clock frameTimeClock;
        while(running)
        {
            float delta = frameTimeClock.getElapsedTime().asSeconds();
            frameTimeClock.restart();
            if (delta > 0.5)
                delta = 0.5;
            if (delta < 0.001)
                delta = 0.001;
            delta *= gameSpeed;

            entityList.update();
            foreach(Updatable, u, updatableList)
                u->update(delta);
            elapsedTime += delta;
            CollisionManager::handleCollisions(delta);
            ScriptObject::clearDestroyedObjects();
            soundManager->updateTick();
            
            sf::sleep(sf::seconds(1.0/60.0 - delta));
            //if (elapsedTime > 2.0)
            //    break;
        }
    }else{
        sf::Clock frameTimeClock;
#ifdef DEBUG
        sf::Clock debugOutputClock;
#endif
        while(running && windowManager->window.isOpen())
        {
            InputHandler::preEventsUpdate();
            // Handle events
            sf::Event event;
            while (windowManager->window.pollEvent(event))
            {
                handleEvent(event);
            }
            InputHandler::postEventsUpdate();

#ifdef DEBUG
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape) && windowManager->hasFocus())
                running = false;

            if (debugOutputClock.getElapsedTime().asSeconds() > 1.0)
            {
                printf("Object count: %4d %4d %4d\n", DEBUG_PobjCount, updatableList.size(), entityList.size());
                debugOutputClock.restart();
            }
#endif

            float delta = frameTimeClock.restart().asSeconds();
            if (delta > 0.5)
                delta = 0.5;
            if (delta < 0.001)
                delta = 0.001;
            delta *= gameSpeed;
#ifdef DEBUG
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Tab))
                delta /= 5.0;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Tilde))
                delta *= 5.0;
#endif
            EngineTiming engine_timing;
            
            sf::Clock engine_timing_clock;
            entityList.update();
            foreach(Updatable, u, updatableList)
                u->update(delta);
            elapsedTime += delta;
            engine_timing.update = engine_timing_clock.restart().asSeconds();
            CollisionManager::handleCollisions(delta);
            engine_timing.collision = engine_timing_clock.restart().asSeconds();
            ScriptObject::clearDestroyedObjects();
            soundManager->updateTick();

            // Clear the window
            windowManager->render();
            engine_timing.render = engine_timing_clock.restart().asSeconds();
            engine_timing.server_update = 0.0f;
            if (game_server)
                engine_timing.server_update = game_server->getUpdateTime();
            
            last_engine_timing = engine_timing;
        }
        soundManager->stopMusic();
    }
}

void Engine::handleEvent(sf::Event& event)
{
    // Window closed: exit
    if ((event.type == sf::Event::Closed))
        running = false;
    if (event.type == sf::Event::GainedFocus)
        windowManager->windowHasFocus = true;
    if (event.type == sf::Event::LostFocus)
        windowManager->windowHasFocus = false;
#ifdef DEBUG
    if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::L))
    {
        int n = 0;
        printf("---------------------\n");
        for(PObject* obj = DEBUG_PobjListStart; obj; obj = obj->DEBUG_PobjListNext)
            printf("%c%4d: %4d: %s\n", obj->isDestroyed() ? '>' : ' ', n++, obj->getRefCount(), typeid(*obj).name());
        printf("---------------------\n");
    }
#endif
    InputHandler::handleEvent(event);
    if (event.type == sf::Event::Resized)
        windowManager->setupView();
#ifdef __ANDROID__
    //Focus lost and focus gained events are used when the application window is created and destroyed.
    if (event.type == sf::Event::LostFocus)
        running = false;
    
    //The MouseEntered and MouseLeft events are received when the activity needs to pause or resume.
    if (event.type == sf::Event::MouseLeft)
    {
        //Pause is when a small popup is on top of the window. So keep running.
        while(windowManager->window.isOpen() && windowManager->window.waitEvent(event))
        {
            if (event.type != sf::Event::MouseLeft)
                handleEvent(event);
            if (event.type == sf::Event::MouseEntered)
                break;
        }
    }
#endif//__ANDROID__
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
