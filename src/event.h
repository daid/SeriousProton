#ifndef EVENT_H
#define EVENT_H

#include <unordered_map>
#include <vector>

#include <SFML/System.hpp>
#include "P.h"
#include "stringImproved.h"

class EventHandler : public virtual PObject
{
public:
    EventHandler(string eventName);
    virtual void event(string eventName, void* param) = 0;
};

class EventManager
{
    std::unordered_map<string, PVector<EventHandler> > eventMap;
public:
    void fire(string eventName, void* param = NULL);
    void registerHandler(string eventName, P<EventHandler> handler);
};

extern EventManager eventManager;

#endif//EVENT_H
