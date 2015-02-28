#include "engine.h"
#include "random.h"
#include "gameEntity.h"
#include "Updatable.h"
#include "collisionable.h"

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
    InputHandler::initialize();
    gameSpeed = 1.0;
    running = true;
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
            
            sf::sleep(sf::seconds(1.0/60.0 - delta));
        }
    }else{
        sf::Clock frameTimeClock;
#ifdef DEBUG
        sf::Clock debugOutputClock;
#endif
        while (windowManager->window.isOpen())
        {
            InputHandler::mouse_wheel_delta = 0;
            InputHandler::keyboard_text_entry = "";
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
                if (event.type == sf::Event::KeyPressed)
                    InputHandler::keyboard_button_down[event.key.code] = true;
                if (event.type == sf::Event::KeyReleased)
                    InputHandler::keyboard_button_down[event.key.code] = false;
                if (event.type == sf::Event::TextEntered && event.text.unicode > 31 && event.text.unicode < 128)
                    InputHandler::keyboard_text_entry += string(char(event.text.unicode));
                if (event.type == sf::Event::MouseWheelMoved)
                    InputHandler::mouse_wheel_delta += event.mouseWheel.delta;
                if (event.type == sf::Event::MouseButtonPressed)
                    InputHandler::mouse_button_down[event.mouseButton.button] = true;
                if (event.type == sf::Event::MouseButtonReleased)
                    InputHandler::mouse_button_down[event.mouseButton.button] = false;
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
            
            InputHandler::update();
            entityList.update();
            foreach(Updatable, u, updatableList)
                u->update(delta);
            elapsedTime += delta;
            CollisionManager::handleCollisions(delta);
            ScriptObject::clearDestroyedObjects();

            // Clear the window
            windowManager->render();
        }
        soundManager.stopMusic();
    }
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
    if (windowManager)
        windowManager->close();
    running = false;
}
