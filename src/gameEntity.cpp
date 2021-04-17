#include "gameEntity.h"

PVector<GameEntity> entityList;
GameEntity::~GameEntity(){}
void GameEntity::update(float /*delta*/){}
void GameEntity::render(sf::RenderTarget& /*window*/) {}
bool GameEntity::takeDamage(sf::Vector2f /*position*/, int /*damageType*/, int /*damage_amount*/) { return false; }
