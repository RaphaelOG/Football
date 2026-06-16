#pragma once

#include "Constants.h"
#include <SFML/Graphics/CircleShape.hpp>

using namespace Constants;

class Player;

class Ball {
public:
    Ball();

    void reset(const sf::Vector2f& position);
    void update(float dt, const class Field& field);
    void draw(sf::RenderTarget& target, const sf::Vector2f& cameraCenter) const;

    void kick(const sf::Vector2f& direction, float power);
    void applyForce(const sf::Vector2f& force);
    void collideWithPlayer(const Player& player);

    sf::Vector2f getPosition() const { return position_; }
    sf::Vector2f getVelocity() const { return velocity_; }
    void setPosition(const sf::Vector2f& pos) { position_ = pos; }
    void setVelocity(const sf::Vector2f& vel) { velocity_ = vel; }
    Player* getOwner() const { return owner_; }
    void setOwner(Player* owner) { owner_ = owner; }
    void release() { owner_ = nullptr; }

private:
    sf::Vector2f position_;
    sf::Vector2f velocity_;
    Player* owner_;
    mutable sf::CircleShape shape_;
};
