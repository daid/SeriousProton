#include "event.h"

EventManager eventManager;

EventHandler::EventHandler(string eventName)
{
    eventManager.registerHandler(eventName, this);
}

void EventManager::fire(string eventName, void* param)
{
    foreach(EventHandler, h, eventMap[eventName])
    {
        h->event(eventName, param);
    }
}

void EventManager::registerHandler(string eventName, P<EventHandler> handler)
{
    eventMap[eventName].push_back(handler);
}
