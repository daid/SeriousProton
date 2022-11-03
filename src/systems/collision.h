#pragma once

#include "ecs/entity.h"
#include <glm/vec2.hpp>


namespace sp {

class CollisionHandler
{
public:
    virtual void collision(sp::ecs::Entity a, sp::ecs::Entity b, float force) = 0;
};

class CollisionSystem
{
public:
    static void update(float delta);
    static void addHandler(CollisionHandler* handler) { handlers.push_back(handler); }

    static std::vector<sp::ecs::Entity> queryArea(glm::vec2 lowerBound, glm::vec2 upperBound);
private:
    static std::vector<CollisionHandler*> handlers;
};

}