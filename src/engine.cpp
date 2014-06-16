#include "engine.h"
#include "random.h"
#include "gameEntity.h"
#include "Updatable.h"
#include "Collisionable.h"

#ifdef DEBUG
#include <typeinfo>
int DEBUG_PobjCount;
PObject* DEBUG_PobjListStart;
#endif

Engine* engine;

Engine::Engine()
{
    engine = this;
    initRandom();
    windowManager = NULL;
    CollisionManager::initialize();
    gameSpeed = 1.0;
}
Engine::~Engine()
{
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
    sf::Clock frameTimeClock;
#ifdef DEBUG
    sf::Clock debugOutputClock;
#endif
    while (windowManager->window.isOpen())
    {
        // Handle events
        sf::Event event;
        while (windowManager->window.pollEvent(event))
        {
            // Window closed: exit
            if ((event.type == sf::Event::Closed))
            {
                windowManager->window.close();
                break;
            }
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
        }

#ifdef DEBUG
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape) && windowManager->hasFocus())
            windowManager->window.close();

        if (debugOutputClock.getElapsedTime().asSeconds() > 1.0)
        {
            printf("Object count: %4d %4d %4d\n", DEBUG_PobjCount, updatableList.size(), entityList.size());
            debugOutputClock.restart();
        }
#endif

        float delta = frameTimeClock.getElapsedTime().asSeconds();
        frameTimeClock.restart();
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
        
        entityList.update();
        foreach(Updatable, u, updatableList)
            u->update(delta);
        elapsedTime += delta;
        CollisionManager::handleCollisions(delta);

        // Clear the window
        windowManager->render();
    }
    soundManager.stopMusic();
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

void Engine::shutdown()
{
    windowManager->close();
}
