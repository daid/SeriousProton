#ifndef GAME_ENTITY_H
#define GAME_ENTITY_H

#include "Updatable.h"
#include "Renderable.h"
/**
    GameEntities are the base for each object in the game.
    They are automaticly added to the entityList, which means they get update() and render() calls.

    The GameEntities are based of the Pobject class, which helps in tracking references and destruction.

    Create a new GameEntity with "new GameEntity()" and destroy it with "entity->destroy()", never use
    "delete entity", as the PObject takes care of this.
 */

class GameEntity;
extern PVector<GameEntity> entityList;
class GameEntity: public Updatable, public Renderable
{
public:
    sf::Sprite sprite;

    GameEntity()
    {
        entityList.push_back(this);
    }
    GameEntity(RenderLayer* renderLayer)
    : Renderable(renderLayer)
    {
        entityList.push_back(this);
    }

    virtual ~GameEntity();

    virtual void update(float delta);

    virtual void render(sf::RenderTarget& window);

    virtual bool takeDamage(sf::Vector2f position, int damageType, int damage_amount);
};

#endif//GAME_ENTITY_H
