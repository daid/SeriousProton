#include "systems/collision.h"
#include "components/collision.h"
#include "ecs/query.h"

#include <glm/trigonometric.hpp>


#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#endif//__GNUC__
#include "Box2D/Box2D.h"
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif//__GNUC__


#define BOX2D_SCALE 20.0f
static inline glm::vec2 b2v(b2Vec2 v)
{
    return glm::vec2(v.x * BOX2D_SCALE, v.y * BOX2D_SCALE);
}
static inline b2Vec2 v2b(glm::vec2 v)
{
    return b2Vec2(v.x / BOX2D_SCALE, v.y / BOX2D_SCALE);
}


static b2World* world;


namespace sp {

std::vector<CollisionHandler*> CollisionSystem::handlers;

namespace {
struct Collision
{
    sp::ecs::Entity A;
    sp::ecs::Entity B;
    float collision_force;
};
}

void CollisionSystem::update(float delta)
{
    if (!world)
        world = new b2World(b2Vec2(0, 0));
    if (delta <= 0.0f)
        return;
    world->Step(delta, 4, 8);
    
    // Go over each entity with physics, and create/update bodies if needed.
    for(auto [entity, transform, physics] : sp::ecs::Query<Transform, Physics>()) {
        if (physics.physics_dirty)
        {
            physics.physics_dirty = false;
            sp::ecs::Entity* ptr;
            if (physics.body) {
                ptr = (sp::ecs::Entity*)physics.body->GetUserData();
                world->DestroyBody(physics.body);
            } else {
                ptr = new sp::ecs::Entity();
                *ptr = entity;
            }

            b2BodyDef bodyDef;
            bodyDef.type = physics.type == Physics::Type::Dynamic ? b2_dynamicBody : b2_kinematicBody;
            bodyDef.userData = ptr;
            bodyDef.allowSleep = false;
            bodyDef.position = v2b(transform.position);
            bodyDef.angle = glm::radians(transform.rotation);
            physics.body = world->CreateBody(&bodyDef);

            b2FixtureDef shapeDef;
            shapeDef.density = 1.f;
            shapeDef.friction = 0.f;
            shapeDef.isSensor = physics.type == Physics::Type::Sensor;

            if (physics.shape == Physics::Shape::Circle) {
                b2CircleShape shape;
                shape.m_radius = physics.size.x / BOX2D_SCALE;
                shapeDef.shape = &shape;
                physics.body->CreateFixture(&shapeDef);
            } else {
                b2PolygonShape shape;
                shape.SetAsBox(physics.size.x / 2.f / BOX2D_SCALE, physics.size.y / 2.f / BOX2D_SCALE, {0, 0}, 0);
                shapeDef.shape = &shape;
                physics.body->CreateFixture(&shapeDef);
            }
        }
        if (transform.position_user_set && physics.body) {
            physics.body->SetTransform(v2b(transform.position), physics.body->GetAngle());
            transform.position_user_set = false;
        }
        if (transform.rotation_user_set && physics.body) {
            physics.body->SetTransform(physics.body->GetPosition(), glm::radians(transform.rotation));
            transform.rotation_user_set = false;
        }
        if (physics.linear_velocity_user_set && physics.body) {
            physics.body->SetLinearVelocity(v2b(physics.linear_velocity));
            physics.linear_velocity_user_set = false;
        }
        if (physics.angular_velocity_user_set && physics.body) {
            physics.body->SetAngularVelocity(glm::radians(physics.angular_velocity));
            physics.angular_velocity_user_set = false;
        }
    }
    
    // Go over each body in the physics world, and update the entity, or delete the body if the entity is gone.
    std::vector<b2Body*> remove_list;
    for(b2Body* body = world->GetBodyList(); body; body = body->GetNext()) {
        sp::ecs::Entity* entity_ptr = (sp::ecs::Entity*)body->GetUserData();
        Transform* transform;
        Physics* physics;
        if (!*entity_ptr || !(physics = entity_ptr->getComponent<Physics>()) || !(transform = entity_ptr->getComponent<Transform>())) {
            delete entity_ptr;
            remove_list.push_back(body);
        } else {
            transform->position = b2v(body->GetPosition());
            transform->rotation = glm::degrees(body->GetAngle());
            physics->linear_velocity = b2v(body->GetLinearVelocity());
            physics->angular_velocity = glm::degrees(body->GetAngularVelocity());
            transform->multiplayer_dirty = true; //TODO make this condition and like, not sending every tick.
        }
    }
    for(auto body : remove_list) {
        world->DestroyBody(body);
    }

    // Find all the collisions and process them.
    std::vector<Collision> collisions;
    for(b2Contact* contact = world->GetContactList(); contact; contact = contact->GetNext())
    {
        if (contact->IsTouching() && contact->IsEnabled())
        {
            float force = 0.0f;
            for (int n = 0; n < contact->GetManifold()->pointCount; n++)
            {
                force += contact->GetManifold()->points[n].normalImpulse * BOX2D_SCALE;
            }
            auto a = (sp::ecs::Entity*)contact->GetFixtureA()->GetBody()->GetUserData();
            auto b = (sp::ecs::Entity*)contact->GetFixtureB()->GetBody()->GetUserData();
            
            for(auto handler : handlers)
            {
                if (!*a || !*b)
                    break;
                handler->collision(*a, *b, force);
                if (!*a || !*b)
                    break;
                handler->collision(*b, *a, force);
            }
        }
    }
}

class QueryCallback : public b2QueryCallback
{
public:
    std::vector<sp::ecs::Entity> list;

	/// Called for each fixture found in the query AABB.
	/// @return false to terminate the query.
	virtual bool ReportFixture(b2Fixture* fixture) override
	{
        auto ptr = (sp::ecs::Entity*)fixture->GetBody()->GetUserData();
        if (*ptr)
            list.push_back(*ptr);
        return true;
	}
};

std::vector<sp::ecs::Entity> CollisionSystem::queryArea(glm::vec2 lowerBound, glm::vec2 upperBound)
{
    QueryCallback callback;
    b2AABB aabb;
    aabb.lowerBound = v2b(lowerBound);
    aabb.upperBound = v2b(upperBound);
    if (aabb.lowerBound.x > aabb.upperBound.x)
        std::swap(aabb.upperBound.x, aabb.lowerBound.x);
    if (aabb.lowerBound.y > aabb.upperBound.y)
        std::swap(aabb.upperBound.y, aabb.lowerBound.y);
    world->QueryAABB(&callback, aabb);
    return callback.list;
}

}