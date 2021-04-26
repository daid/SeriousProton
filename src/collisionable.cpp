#include <algorithm>
#include <utility>
#include "collisionable.h"
#include "Renderable.h"
#include "vectorUtils.h"

#define BOX2D_SCALE 20.0f
static inline sf::Vector2f b2v(b2Vec2 v)
{
    return sf::Vector2f(v.x * BOX2D_SCALE, v.y * BOX2D_SCALE);
}
static inline b2Vec2 v2b(sf::Vector2f v)
{
    return b2Vec2(v.x / BOX2D_SCALE, v.y / BOX2D_SCALE);
}

b2World* CollisionManager::world;

void CollisionManager::initialize()
{
    world = new b2World(b2Vec2(0, 0));
}

class QueryCallback : public b2QueryCallback
{
public:
    PVector<Collisionable> list;

	/// Called for each fixture found in the query AABB.
	/// @return false to terminate the query.
	virtual bool ReportFixture(b2Fixture* fixture)
	{
        P<Collisionable> ptr = (Collisionable*)fixture->GetBody()->GetUserData();
        if (ptr)
            list.push_back(ptr);
        return true;
	}
};

PVector<Collisionable> CollisionManager::queryArea(sf::Vector2f lowerBound, sf::Vector2f upperBound)
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

class Collision
{
public:
    P<Collisionable> A;
    P<Collisionable> B;
    float collision_force;

    Collision(P<Collisionable> A, P<Collisionable> B, float collision_force)
    : A(A), B(B), collision_force(collision_force)
    {}
};

void CollisionManager::handleCollisions(float delta)
{
    if (delta <= 0.0)
        return;

    Collisionable* destroy = NULL;
    world->Step(delta, 4, 8);
    std::vector<Collision> collisions;
    for(b2Contact* contact = world->GetContactList(); contact; contact = contact->GetNext())
    {
        if (contact->IsTouching() && contact->IsEnabled())
        {
            Collisionable* A = (Collisionable*)contact->GetFixtureA()->GetBody()->GetUserData();
            Collisionable* B = (Collisionable*)contact->GetFixtureB()->GetBody()->GetUserData();
            if (!A->isDestroyed() && !B->isDestroyed())
            {
                float collision_force = 0.0f;
                for (int n = 0; n < contact->GetManifold()->pointCount; n++)
                {
                    collision_force += contact->GetManifold()->points[n].normalImpulse * BOX2D_SCALE;
                }
                collisions.push_back(Collision(A, B, collision_force));
            }else{
                if (A->isDestroyed())
                {
                    contact->GetFixtureA()->SetSensor(true);
                    destroy = A;
                }
                if (B->isDestroyed())
                {
                    contact->GetFixtureB()->SetSensor(true);
                    destroy = B;
                }
            }
        }
    }

    //Lazy cleanup of already destroyed bodies. We cannot destroy the bodies while we are walking trough the ContactList, as it would invalidate the contact we are iterating on.
    if (destroy && destroy->body)
    {
        world->DestroyBody(destroy->body);
        destroy->body = NULL;
    }

    for(unsigned int n=0; n<collisions.size(); n++)
    {
        Collisionable* A = *collisions[n].A;
        Collisionable* B = *collisions[n].B;
        if (A && B)
        {
            A->collide(B, collisions[n].collision_force);
            B->collide(A, collisions[n].collision_force);
        }
    }
}

Collisionable::Collisionable(float radius)
{
    enable_physics = false;
    static_physics = false;
    body = NULL;
    multiplayer_replication_object_significant_range = -1;

    setCollisionRadius(radius);
}

Collisionable::Collisionable(sf::Vector2f box_size, sf::Vector2f box_origin)
{
    enable_physics = false;
    static_physics = false;
    body = NULL;

    setCollisionBox(box_size, box_origin);
}

Collisionable::Collisionable(const std::vector<sf::Vector2f>& shape)
{
    enable_physics = false;
    static_physics = false;
    body = NULL;

    setCollisionShape(shape);
}

Collisionable::~Collisionable()
{
    destroyBody();
}

void Collisionable::setCollisionRadius(float radius)
{
    if (radius <= 0)
    {
        destroyBody();
    }else{
        b2CircleShape shape;
        shape.m_radius = radius / BOX2D_SCALE;

        createBody(&shape);
    }
}

void Collisionable::setCollisionBox(sf::Vector2f box_size, sf::Vector2f box_origin)
{
    b2PolygonShape shape;
    shape.SetAsBox(box_size.x / 2.f / BOX2D_SCALE, box_size.y / 2.f / BOX2D_SCALE, v2b(box_origin), 0);

    createBody(&shape);
}

void Collisionable::setCollisionShape(const std::vector<sf::Vector2f>& shapeList)
{
    for(unsigned int offset=1; offset<shapeList.size(); offset+=b2_maxPolygonVertices-2)
    {
        size_t len = b2_maxPolygonVertices;
        if (len > shapeList.size() - offset + 1)
            len = shapeList.size() - offset + 1;
        if (len < 3)
            break;

        b2Vec2 points[b2_maxPolygonVertices];
        points[0] = v2b(shapeList[0]);
        for(size_t n=0; n<len-1; n++)
            points[n + 1] = v2b(shapeList[n + size_t{ offset }]);

        b2PolygonShape shape;
        bool valid = shape.Set(points, static_cast<int32>(len));
        if (!valid)
        {
            shape.SetAsBox(1.f/BOX2D_SCALE, 1.f/BOX2D_SCALE, points[0], 0);
            LOG(ERROR) << "Failed to set valid collision shape: " << shapeList.size();
            for(size_t n=0; n<shapeList.size(); n++)
            {
                LOG(ERROR) << shapeList[n];
            }
            destroy();
        }
        if (offset == 1)
        {
            createBody(&shape);
        }else{
            b2FixtureDef shapeDef;
            shapeDef.shape = &shape;
            shapeDef.density = 1.f;
            shapeDef.friction = 0.f;
            shapeDef.isSensor = !enable_physics;
            body->CreateFixture(&shapeDef);
        }
    }
}

void Collisionable::setCollisionChain(const std::vector<sf::Vector2f>& points, bool loop)
{
    b2ChainShape shape;
    std::vector<b2Vec2> b_points;
    b_points.reserve(points.size());
    for(sf::Vector2f point : points)
    {
        b_points.push_back(v2b(point));
    }
    if (loop)
    {
        shape.CreateLoop(b_points.data(), static_cast<int32>(b_points.size()));
    }
    else
    {
        shape.CreateChain(b_points.data(), static_cast<int32>(b_points.size()));
    }

    createBody(&shape);
}

void Collisionable::setCollisionFriction(float amount)
{
    if (!body)
        return;
    for(b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext())
    {
        f->SetFriction(amount);
    }
}

void Collisionable::setCollisionFilter(uint16_t category_bits, uint16_t mask_bits)
{
    if (!body)
        return;
    b2Filter filter;
    filter.categoryBits = category_bits;
    filter.maskBits = mask_bits;
    for(b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext())
    {
        f->SetFilterData(filter);
    }
}

void Collisionable::setCollisionPhysics(bool wants_enable_physics, bool wants_static_physics)
{
    enable_physics = wants_enable_physics;
    static_physics = wants_static_physics;

    if (!body)
        return;

    for(b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext())
        f->SetSensor(!enable_physics);
    body->SetType(static_physics ? b2_kinematicBody : b2_dynamicBody);
}

void Collisionable::createBody(b2Shape* shape)
{
    if (body)
    {
        while(body->GetFixtureList())
            body->DestroyFixture(body->GetFixtureList());
    }else{
        b2BodyDef bodyDef;
        bodyDef.type = static_physics ? b2_kinematicBody : b2_dynamicBody;
        bodyDef.userData = this;
        bodyDef.allowSleep = false;
        body = CollisionManager::world->CreateBody(&bodyDef);
    }

    b2FixtureDef shapeDef;
    shapeDef.shape = shape;
    shapeDef.density = 1.0;
    shapeDef.friction = 0.0;
    shapeDef.isSensor = !enable_physics;
    body->CreateFixture(&shapeDef);
}

void Collisionable::destroyBody()
{
    if (body)
        CollisionManager::world->DestroyBody(body);
    body = nullptr;
}

void Collisionable::collide(Collisionable* /*target*/, float /*force*/)
{
}

void Collisionable::setPosition(sf::Vector2f position)
{
    if (body == NULL) return;
    body->SetTransform(v2b(position), body->GetAngle());
}

sf::Vector2f Collisionable::getPosition()
{
    if (body == NULL) return sf::Vector2f(0, 0);
    return b2v(body->GetPosition());
}

void Collisionable::setRotation(float angle)
{
    if (body == NULL) return;
    body->SetTransform(body->GetPosition(), angle / 180.f * M_PI);
}

float Collisionable::getRotation()
{
    if (body == NULL) return 0;
    return body->GetAngle() / M_PI * 180.f;
}

void Collisionable::setVelocity(sf::Vector2f velocity)
{
    if (body == NULL) return;
    body->SetLinearVelocity(v2b(velocity));
}
sf::Vector2f Collisionable::getVelocity()
{
    if (body == NULL) return sf::Vector2f(0, 0);
    return b2v(body->GetLinearVelocity());
}

void Collisionable::setAngularVelocity(float velocity)
{
    if (body == NULL) return;
    body->SetAngularVelocity(velocity / 180.f * M_PI);
}
float Collisionable::getAngularVelocity()
{
    if (body == NULL) return 0;
    return body->GetAngularVelocity() / M_PI * 180.f;
}

void Collisionable::applyImpulse(sf::Vector2f position, sf::Vector2f impulse)
{
    if (body == NULL) return;
    body->ApplyLinearImpulse(v2b(impulse), v2b(position), true);
}

sf::Vector2f Collisionable::toLocalSpace(sf::Vector2f v)
{
    if (body == NULL) return sf::Vector2f(0, 0);
    return b2v(body->GetLocalPoint(v2b(v)));
}
sf::Vector2f Collisionable::toWorldSpace(sf::Vector2f v)
{
    if (body == NULL) return sf::Vector2f(0, 0);
    return b2v(body->GetWorldPoint(v2b(v)));
}

std::vector<sf::Vector2f> Collisionable::getCollisionShape()
{
    std::vector<sf::Vector2f> ret;
    if (body == NULL) return ret;
    b2Fixture* f = body->GetFixtureList();
    b2Shape* s = f->GetShape();
    switch(s->GetType())
    {
    case b2Shape::e_circle:
        {
            b2CircleShape* cs = static_cast<b2CircleShape*>(s);
            float radius = cs->m_radius * BOX2D_SCALE;
            for(int n=0; n<32; n++)
                ret.push_back(sf::Vector2f(sin(float(n)/32.f*M_PI*2) * radius, cos(float(n)/32.f*M_PI*2) * radius));
        }
        break;
    case b2Shape::e_polygon:
        {
            b2PolygonShape* cs = static_cast<b2PolygonShape*>(s);
            for(int n=0; n<cs->GetVertexCount(); n++)
                ret.push_back(b2v(cs->GetVertex(n)));
        }
        break;
    default:
        break;
    }
    return ret;
}

#ifdef DEBUG
CollisionDebugDraw::CollisionDebugDraw(RenderLayer* layer)
: Renderable(layer)
{
    SetFlags(e_shapeBit | e_jointBit | e_centerOfMassBit);
    CollisionManager::world->SetDebugDraw(this);
}

void CollisionDebugDraw::render(sf::RenderTarget& window)
{
    render_target = &window;
    CollisionManager::world->DrawDebugData();
}

/// Draw a closed polygon provided in CCW order.
void CollisionDebugDraw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
    sf::VertexArray a(sf::LinesStrip, vertexCount+1);
    for(int32 n=0; n<vertexCount; n++)
    {
        a[n].position = b2v(vertices[n]);
        a[n].color = sf::Color(color.r * 255, color.g * 255, color.b * 255, color.a * 255);
    }
    a[vertexCount].position = b2v(vertices[0]);
    a[vertexCount].color = sf::Color(color.r * 255, color.g * 255, color.b * 255, color.a * 255);
    render_target->draw(a);
}

/// Draw a solid closed polygon provided in CCW order.
void CollisionDebugDraw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
    sf::VertexArray a(sf::LinesStrip, vertexCount+1);
    for(int32 n=0; n<vertexCount; n++)
    {
        a[n].position = b2v(vertices[n]);
        a[n].color = sf::Color(color.r * 255, color.g * 255, color.b * 255, color.a * 255);
    }
    a[vertexCount].position = b2v(vertices[0]);
    a[vertexCount].color = sf::Color(color.r * 255, color.g * 255, color.b * 255, color.a * 255);
    render_target->draw(a);
}

/// Draw a circle.
void CollisionDebugDraw::DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color)
{
    sf::CircleShape shape(radius * BOX2D_SCALE, 16);
    shape.setOrigin(radius * BOX2D_SCALE, radius * BOX2D_SCALE);
    shape.setPosition(b2v(center));
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(sf::Color(color.r * 255, color.g * 255, color.b * 255, color.a * 255));
    shape.setOutlineThickness(0.3);
    render_target->draw(shape);
}

/// Draw a solid circle.
void CollisionDebugDraw::DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color)
{
    sf::CircleShape shape(radius * BOX2D_SCALE, 16);
    shape.setOrigin(radius * BOX2D_SCALE, radius * BOX2D_SCALE);
    shape.setPosition(b2v(center));
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(sf::Color(color.r * 255, color.g * 255, color.b * 255, color.a * 255));
    shape.setOutlineThickness(0.3);
    render_target->draw(shape);
}

/// Draw a line segment.
void CollisionDebugDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
{
    sf::VertexArray a(sf::Lines, 2);
    a[0].position = b2v(p1);
    a[0].color = sf::Color(color.r * 255, color.g * 255, color.b * 255, color.a * 255);
    a[1].position = b2v(p2);
    a[1].color = sf::Color(color.r * 255, color.g * 255, color.b * 255, color.a * 255);
    render_target->draw(a);
}

/// Draw a transform. Choose your own length scale.
/// @param xf a transform.
void CollisionDebugDraw::DrawTransform(const b2Transform& xf)
{
    sf::VertexArray a(sf::Lines, 4);
    a[0].position = b2v(xf.p);
    a[1].position = b2v(xf.p) + sf::Vector2f(xf.q.GetXAxis().x * 10, xf.q.GetXAxis().y * 10);
    a[0].position = b2v(xf.p);
    a[1].position = b2v(xf.p) + sf::Vector2f(xf.q.GetYAxis().x * 10, xf.q.GetYAxis().y * 10);
    render_target->draw(a);
}
#endif//DEBUG
