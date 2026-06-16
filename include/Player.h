#pragma once

#include "Constants.h"
#include <SFML/Graphics/CircleShape.hpp>
#include <string>

using namespace Constants;

class Ball;

struct FormationSlot {
    Position position;
    sf::Vector2f normalizedPos; // 0-1 relative to own half
    PlayerRole role;
};

class Player {
public:
    Player(int id, TeamSide team, Position pos, const std::string& name);

    void reset(const sf::Vector2f& position);
    void update(float dt, bool isControlled);
    void draw(sf::RenderTarget& target, const sf::Vector2f& cameraCenter, bool isControlled) const;

    void setTargetPosition(const sf::Vector2f& target);
    void setMoveInput(const sf::Vector2f& input, bool sprint);
    void attemptTackle(Ball& ball, Player& opponent);
    void kickBall(Ball& ball, const sf::Vector2f& direction, float power);
    bool hasBall(const Ball& ball) const;
    void takePossession(Ball& ball);

    sf::Vector2f getPosition() const { return position_; }
    sf::Vector2f getVelocity() const { return velocity_; }
    sf::Vector2f getHomePosition() const { return homePosition_; }
    void setHomePosition(const sf::Vector2f& pos) { homePosition_ = pos; }
    sf::Vector2f getTargetPosition() const { return targetPosition_; }

    int getId() const { return id_; }
    TeamSide getTeam() const { return team_; }
    Position getPositionRole() const { return role_; }
    PlayerRole getRole() const;
    const std::string& getName() const { return name_; }
    bool isGoalkeeper() const { return role_ == Position::GK; }
    float getStamina() const { return stamina_; }
    bool canTackle() const { return tackleCooldown_ <= 0.f; }

private:
    int id_;
    TeamSide team_;
    Position role_;
    std::string name_;

    sf::Vector2f position_;
    sf::Vector2f velocity_;
    sf::Vector2f homePosition_;
    sf::Vector2f targetPosition_;
    sf::Vector2f moveInput_;

    float stamina_;
    float tackleCooldown_;
    bool sprinting_;
    bool controlled_;

    mutable sf::CircleShape shape_;
    mutable sf::CircleShape highlight_;
};
