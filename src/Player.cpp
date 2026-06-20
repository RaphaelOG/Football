#include "Player.h"
#include "Ball.h"
#include "Field.h"
#include <cmath>

using namespace Constants;

Player::Player(int id, TeamSide team, Position pos, const std::string& name)
    : id_(id), team_(team), role_(pos), name_(name),
      stamina_(100.f), tackleCooldown_(0.f), sprinting_(false), controlled_(false) {
    float radius = isGoalkeeper() ? GK_RADIUS : PLAYER_RADIUS;
    float px = radius * PIXELS_PER_METER;

    body_.setRadius(px);
    body_.setOrigin(px, px);
    body_.setOutlineThickness(2.f);
    body_.setOutlineColor(sf::Color(0, 0, 0, 100));

    shadow_.setRadius(px * 0.9f);
    shadow_.setOrigin(px * 0.9f, px * 0.9f);
    shadow_.setFillColor(sf::Color(0, 0, 0, 60));
    shadow_.setScale(1.f, 0.55f);

    highlight_.setRadius((radius + 0.55f) * PIXELS_PER_METER);
    highlight_.setOrigin(highlight_.getRadius(), highlight_.getRadius());
    highlight_.setFillColor(sf::Color::Transparent);
    highlight_.setOutlineThickness(3.f);
    highlight_.setOutlineColor(sf::Color(255, 230, 50, 220));

    direction_.setPointCount(3);
    direction_.setFillColor(sf::Color(255, 255, 255, 200));
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

float Player::getFacingAngle() const {
    sf::Vector2f dir = velocity_;
    if (length(dir) < 0.3f) {
        dir = team_ == TeamSide::Home ? sf::Vector2f(1.f, 0.f) : sf::Vector2f(-1.f, 0.f);
    }
    return std::atan2(dir.y, dir.x) * 180.f / 3.14159265f;
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

    position_.x = std::max(PLAYER_RADIUS, std::min(FIELD_LENGTH - PLAYER_RADIUS, position_.x));
    position_.y = std::max(PLAYER_RADIUS, std::min(FIELD_WIDTH - PLAYER_RADIUS, position_.y));

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
    sf::Vector2f screen = Field::worldToScreen(position_, cameraCenter);
    float px = body_.getRadius();

    shadow_.setPosition(screen.x + 2.f, screen.y + 4.f);
    target.draw(shadow_);

    sf::Color color = team_ == TeamSide::Home ? homeColor() : awayColor();
    sf::Color shorts = sf::Color(
        static_cast<sf::Uint8>(color.r * 0.55f),
        static_cast<sf::Uint8>(color.g * 0.55f),
        static_cast<sf::Uint8>(color.b * 0.55f));

    if (isGoalkeeper()) {
        color = sf::Color(40, 40, 40);
        shorts = sf::Color(30, 30, 30);
    }

    body_.setFillColor(color);
    body_.setPosition(screen);
    target.draw(body_);

    sf::CircleShape shortsMark(px * 0.55f);
    shortsMark.setOrigin(px * 0.55f, px * 0.55f);
    shortsMark.setFillColor(shorts);
    shortsMark.setPosition(screen.x, screen.y + px * 0.15f);
    target.draw(shortsMark);

    float angle = getFacingAngle();
    direction_.setPosition(screen);
    direction_.setRotation(angle);
    float tip = px * 0.85f;
    direction_.setPoint(0, {tip, 0.f});
    direction_.setPoint(1, {-px * 0.35f, px * 0.4f});
    direction_.setPoint(2, {-px * 0.35f, -px * 0.4f});
    target.draw(direction_);

    if (isControlled) {
        highlight_.setPosition(screen);
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
        ball.kick(-dir, 6.f, this, KickType::Pass);
    }
}

void Player::kickBall(Ball& ball, const sf::Vector2f& direction, float power, KickType type) {
    if (!hasBall(ball)) return;
    ball.kick(direction, power, this, type);
}

bool Player::hasBall(const Ball& ball) const {
    return ball.getOwner() == this;
}

void Player::takePossession(Ball& ball) {
    ball.setOwner(this);
    ball.setVelocity(velocity_);
    ball.setOutOfPlay(false);
}
