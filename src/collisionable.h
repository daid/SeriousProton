#ifndef COLLISIONABLE_H
#define COLLISIONABLE_H

#include "P.h"
#include "Box2D/Box2D.h"

class Collisionable;
class CollisionManager
{
public:
    static void initialize();
    static void handleCollisions(float delta);
    static PVector<Collisionable> queryArea(glm::vec2 lowerBound, glm::vec2 upperBound);
private:
    static b2World* world;

    friend class Collisionable;
    friend class CollisionDebugDraw;
};

class Collisionable : public virtual PObject
{
private:
    b2Body* body;
    bool enable_physics;
    bool static_physics;

    void createBody(b2Shape* shape);
    void destroyBody();
public:
    Collisionable(float radius);
    Collisionable(glm::vec2 box_size, glm::vec2 box_origin = glm::vec2(0, 0));
    Collisionable(const std::vector<glm::vec2>& shape);
    virtual ~Collisionable();
    virtual void collide(Collisionable* target, float force);

    void setCollisionRadius(float radius);
    void setCollisionBox(glm::vec2 box_size, glm::vec2 box_origin = glm::vec2(0, 0));
    void setCollisionShape(const std::vector<glm::vec2>& shape);
    void setCollisionChain(const std::vector<glm::vec2>& points, bool loop);
    void setCollisionPhysics(bool enable_physics, bool static_physics);
    void setCollisionFriction(float amount);
    void setCollisionFilter(uint16_t category_bits, uint16_t mask_bits);    //Collision happens if (A->category_bits & B->mask_bits) && (B->category_bits & A->mask_bits)

    void setPosition(glm::vec2 v);
    glm::vec2 getPosition() const;
    void setRotation(float angle);
    float getRotation() const;
    void setVelocity(glm::vec2 velocity);
    glm::vec2 getVelocity() const;
    void setAngularVelocity(float velocity);
    float getAngularVelocity() const;
    void applyImpulse(glm::vec2 position, glm::vec2 impulse);

    glm::vec2 toLocalSpace(glm::vec2 v) const;
    glm::vec2 toWorldSpace(glm::vec2 v) const;

    std::vector<glm::vec2> getCollisionShape() const; //For debugging

    float multiplayer_replication_object_significant_range;

    friend class CollisionManager;
};

#ifdef DEBUG
#include "Renderable.h"

class CollisionDebugDraw : public Renderable, public b2Draw
{
    sf::RenderTarget* render_target;
public:
    CollisionDebugDraw(RenderLayer* layer);

    virtual void render(sf::RenderTarget& window);

	virtual void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
	virtual void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
	virtual void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color);
	virtual void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color);
	virtual void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);
	virtual void DrawTransform(const b2Transform& xf);
};
#endif//DEBUG

#endif // COLLISIONABLE_H
