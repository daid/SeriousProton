#pragma once

#include <glm/vec2.hpp>


class b2Body;
namespace sp {
class CollisionSystem;

// Transform component, to give an entity a position and rotation in the 3D world.
class Transform
{
public:
    glm::vec2 getPosition() const { return position; }
    float getRotation() const { return rotation; }

    void setPosition(glm::vec2 v) { position = v; position_user_set = true; multiplayer_dirty = true; }
    void setRotation(float angle) { rotation = angle; rotation_user_set = true; multiplayer_dirty = true; }
private:
    bool position_user_set = false;
    bool rotation_user_set = false;
    bool multiplayer_dirty = false;

    glm::vec2 position{};
    float rotation = 0.0f;

    friend class sp::CollisionSystem;
};

// The physics component will give the entity a physical presents in the physic simulation
//  this includes collision feedback. A physics component does nothing on it's own, it needs a Position component as well.
//  The position component will be updated by the physics system on each physics step. Updating the position component from another location will force the
//  physics to move the object to that position.
class Physics
{
public:
    enum class Type {
        Sensor,
        Dynamic,
        Static,
    };
    Type getType() const { return type; }
    void setType(Type type) { if (type == this->type) return; this->type = type; physics_dirty = true; multiplayer_dirty = true; }
    void setCircle(Type type, float radius) { if (type == this->type && shape == Shape::Circle && size.x == radius) return; this->type = type; shape = Shape::Circle; size.x = radius; size.y = radius; physics_dirty = true; multiplayer_dirty = true; }
    void setRectangle(Type type, glm::vec2 new_size) { if (type == this->type && shape == Shape::Rectangle && size == new_size) return; this->type = type; shape = Shape::Circle; size = new_size; physics_dirty = true; multiplayer_dirty = true; }
    glm::vec2 getSize() const { return size; }

    glm::vec2 getVelocity() const { return linear_velocity; }
    float getAngularVelocity() const { return angular_velocity; }

    void setVelocity(glm::vec2 velocity) { linear_velocity_user_set = true; linear_velocity = velocity; }
    void setAngularVelocity(float velocity) { angular_velocity_user_set = true; angular_velocity = velocity; }
private:
    bool physics_dirty = true;
    bool multiplayer_dirty = false;

    Type type = Type::Sensor;
    enum class Shape {
        Circle,
        Rectangle,
    } shape = Shape::Circle;
    glm::vec2 size{1.0, 1.0};

    b2Body* body = nullptr;
    glm::vec2 linear_velocity{};
    float angular_velocity = 0.0f;
    bool linear_velocity_user_set = false;
    bool angular_velocity_user_set = false;

    friend class sp::CollisionSystem;
};

}