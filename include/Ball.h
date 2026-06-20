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

    void kick(const sf::Vector2f& direction, float power, Player* kicker = nullptr, KickType type = KickType::Pass);
    void applyForce(const sf::Vector2f& force);
    void collideWithPlayer(const Player& player);

    sf::Vector2f getPosition() const { return position_; }
    sf::Vector2f getVelocity() const { return velocity_; }
    void setPosition(const sf::Vector2f& pos) { position_ = pos; }
    void setVelocity(const sf::Vector2f& vel) { velocity_ = vel; }
    Player* getOwner() const { return owner_; }
    void setOwner(Player* owner) { owner_ = owner; }
    void release() { owner_ = nullptr; }

    Player* getLastKicker() const { return lastKicker_; }
    TeamSide getLastTouchTeam() const { return lastTouchTeam_; }
    KickType getLastKickType() const { return lastKickType_; }
    bool isOutOfPlay() const { return outOfPlay_; }
    void setOutOfPlay(bool out) { outOfPlay_ = out; }

private:
    sf::Vector2f position_;
    sf::Vector2f velocity_;
    Player* owner_;
    Player* lastKicker_;
    TeamSide lastTouchTeam_;
    KickType lastKickType_;
    bool outOfPlay_;
    mutable sf::CircleShape shape_;
    mutable sf::CircleShape trail_;
};
