#include "Player.h"
#include "Ball.h"
#include "Field.h"

using namespace Constants;

Player::Player(int id, TeamSide team, Position pos, const std::string& name)
    : id_(id), team_(team), role_(pos), name_(name),
      stamina_(100.f), tackleCooldown_(0.f), sprinting_(false), controlled_(false) {
    float radius = isGoalkeeper() ? GK_RADIUS : PLAYER_RADIUS;
    shape_.setRadius(radius * PIXELS_PER_METER);
    shape_.setOrigin(radius * PIXELS_PER_METER, radius * PIXELS_PER_METER);

    highlight_.setRadius((radius + 0.4f) * PIXELS_PER_METER);
    highlight_.setOrigin((radius + 0.4f) * PIXELS_PER_METER, (radius + 0.4f) * PIXELS_PER_METER);
    highlight_.setFillColor(sf::Color::Transparent);
    highlight_.setOutlineThickness(2.f);
    highlight_.setOutlineColor(sf::Color::Yellow);
}

void Player::reset(const sf::Vector2f& position) {
    position_ = position;
    velocity_ = {0.f, 0.f};
    homePosition_ = position;
    targetPosition_ = position;
    moveInput_ = {0.f, 0.f};
    stamina_ = 100.f;
    tackleCooldown_ = 0.f;
    sprinting_ = false;
}

PlayerRole Player::getRole() const {
    switch (role_) {
        case Position::GK: return PlayerRole::Goalkeeper;
        case Position::CB: case Position::LB: case Position::RB: return PlayerRole::Defender;
        case Position::CM: case Position::LM: case Position::RM: case Position::CDM: return PlayerRole::Midfielder;
        default: return PlayerRole::Striker;
    }
}

void Player::update(float dt, bool isControlled) {
    controlled_ = isControlled;
    tackleCooldown_ = std::max(0.f, tackleCooldown_ - dt);

    float maxSpeed = isGoalkeeper() ? GK_MAX_SPEED : PLAYER_MAX_SPEED;
    if (sprinting_ && stamina_ > 5.f && !isGoalkeeper()) {
        maxSpeed = PLAYER_SPRINT_SPEED;
        stamina_ -= 25.f * dt;
    } else {
        stamina_ = std::min(100.f, stamina_ + 12.f * dt);
    }

    sf::Vector2f desiredVel = {0.f, 0.f};

    if (isControlled) {
        desiredVel = moveInput_ * maxSpeed;
    } else {
        sf::Vector2f toTarget = targetPosition_ - position_;
        float dist = length(toTarget);
        if (dist > 0.5f) {
            desiredVel = normalized(toTarget) * maxSpeed;
        }
    }

    sf::Vector2f accel = desiredVel - velocity_;
    float accelMag = length(accel);
    if (accelMag > PLAYER_ACCEL * dt) {
        accel = normalized(accel) * PLAYER_ACCEL * dt;
    }
    velocity_ += accel;

    // Friction
    float speed = length(velocity_);
    if (speed > 0.01f) {
        float friction = PLAYER_FRICTION * dt;
        if (speed <= friction) {
            velocity_ = {0.f, 0.f};
        } else {
            velocity_ = velocity_ - normalized(velocity_) * friction;
        }
    }

    position_ += velocity_ * dt;

    // Keep on field
    position_.x = std::max(PLAYER_RADIUS, std::min(FIELD_LENGTH - PLAYER_RADIUS, position_.x));
    position_.y = std::max(PLAYER_RADIUS, std::min(FIELD_WIDTH - PLAYER_RADIUS, position_.y));

    // GK stays in penalty box
    if (isGoalkeeper()) {
        float boxDepth = PENALTY_AREA_LENGTH + 2.f;
        if (team_ == TeamSide::Home) {
            position_.x = std::min(position_.x, boxDepth);
        } else {
            position_.x = std::max(position_.x, FIELD_LENGTH - boxDepth);
        }
    }
}

void Player::draw(sf::RenderTarget& target, const sf::Vector2f& cameraCenter, bool isControlled) const {
    sf::Color color = team_ == TeamSide::Home ? homeColor() : awayColor();
    if (isGoalkeeper()) color = sf::Color(
        static_cast<sf::Uint8>(color.r * 0.7f),
        static_cast<sf::Uint8>(color.g * 0.7f),
        static_cast<sf::Uint8>(color.b * 0.7f)
    );

    shape_.setFillColor(color);
    shape_.setPosition(Field::worldToScreen(position_, cameraCenter));
    target.draw(shape_);

    if (isControlled) {
        highlight_.setPosition(Field::worldToScreen(position_, cameraCenter));
        target.draw(highlight_);
    }
}

void Player::setTargetPosition(const sf::Vector2f& target) {
    targetPosition_ = target;
}

void Player::setMoveInput(const sf::Vector2f& input, bool sprint) {
    moveInput_ = input;
    sprinting_ = sprint;
}

void Player::attemptTackle(Ball& ball, Player& opponent) {
    if (tackleCooldown_ > 0.f) return;
    sf::Vector2f diff = opponent.getPosition() - position_;
    if (length(diff) > TACKLE_RANGE) return;

    tackleCooldown_ = TACKLE_COOLDOWN;
    sf::Vector2f dir = normalized(diff);
    velocity_ = dir * 8.f;

    if (ball.getOwner() == &opponent) {
        ball.release();
        ball.kick(-dir, 6.f);
    }
}

void Player::kickBall(Ball& ball, const sf::Vector2f& direction, float power) {
    if (!hasBall(ball)) return;
    ball.kick(direction, power);
}

bool Player::hasBall(const Ball& ball) const {
    return ball.getOwner() == this;
}

void Player::takePossession(Ball& ball) {
    ball.setOwner(this);
    ball.setVelocity(velocity_);
}
