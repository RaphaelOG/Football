#include "Ball.h"
#include "Player.h"
#include "Field.h"

using namespace Constants;

Ball::Ball()
    : owner_(nullptr)
    , lastKicker_(nullptr)
    , lastTouchTeam_(TeamSide::Home)
    , lastKickType_(KickType::Pass)
    , outOfPlay_(false) {
    shape_.setRadius(BALL_RADIUS * PIXELS_PER_METER);
    shape_.setOrigin(BALL_RADIUS * PIXELS_PER_METER, BALL_RADIUS * PIXELS_PER_METER);
    shape_.setFillColor(sf::Color::White);
    shape_.setOutlineThickness(1.f);
    shape_.setOutlineColor(sf::Color(30, 30, 30));

    trail_.setRadius(BALL_RADIUS * 0.6f * PIXELS_PER_METER);
    trail_.setOrigin(BALL_RADIUS * 0.6f * PIXELS_PER_METER, BALL_RADIUS * 0.6f * PIXELS_PER_METER);
    trail_.setFillColor(sf::Color(255, 255, 255, 80));

    reset({FIELD_LENGTH / 2.f, FIELD_WIDTH / 2.f});
}

void Ball::reset(const sf::Vector2f& position) {
    position_ = position;
    velocity_ = {0.f, 0.f};
    owner_ = nullptr;
    outOfPlay_ = false;
}

void Ball::update(float dt, const Field& field) {
    (void)field;
    if (outOfPlay_) return;

    if (owner_) {
        sf::Vector2f dir = normalized(owner_->getVelocity());
        float offset = owner_->isGoalkeeper() ? 0.8f : 1.2f;
        position_ = owner_->getPosition() + dir * offset;
        if (length(owner_->getVelocity()) < 0.5f) {
            float facing = owner_->getTeam() == TeamSide::Home ? 1.f : -1.f;
            position_ = owner_->getPosition() + sf::Vector2f(facing * offset, 0.f);
        }
        velocity_ = owner_->getVelocity();
        lastKicker_ = owner_;
        lastTouchTeam_ = owner_->getTeam();
        return;
    }

    float speed = length(velocity_);
    if (speed > 0.01f) {
        float friction = BALL_FRICTION * dt;
        if (speed <= friction) {
            velocity_ = {0.f, 0.f};
        } else {
            velocity_ = velocity_ - normalized(velocity_) * friction;
        }
    }

    position_ += velocity_ * dt;

    float goalTop = (FIELD_WIDTH - GOAL_WIDTH) / 2.f;
    float goalBottom = goalTop + GOAL_WIDTH;
    bool inGoalMouth = position_.y >= goalTop && position_.y <= goalBottom;

    // Touchline bounce — keeps ball in play on sidelines
    if (position_.y < BALL_RADIUS) {
        position_.y = BALL_RADIUS;
        velocity_.y = std::abs(velocity_.y) * 0.55f;
    }
    if (position_.y > FIELD_WIDTH - BALL_RADIUS) {
        position_.y = FIELD_WIDTH - BALL_RADIUS;
        velocity_.y = -std::abs(velocity_.y) * 0.55f;
    }

    if (position_.x < BALL_RADIUS) {
        if (!inGoalMouth) {
            // Let ball exit for goal kick / corner detection
        } else {
            position_.x = BALL_RADIUS;
            velocity_.x = std::abs(velocity_.x) * 0.5f;
        }
    }
    if (position_.x > FIELD_LENGTH - BALL_RADIUS) {
        if (!inGoalMouth) {
            // Let ball exit
        } else {
            position_.x = FIELD_LENGTH - BALL_RADIUS;
            velocity_.x = -std::abs(velocity_.x) * 0.5f;
        }
    }

    speed = length(velocity_);
    if (speed > BALL_MAX_SPEED) {
        velocity_ = normalized(velocity_) * BALL_MAX_SPEED;
    }
}

void Ball::draw(sf::RenderTarget& target, const sf::Vector2f& cameraCenter) const {
    float speed = length(velocity_);
    if (!owner_ && speed > 10.f) {
        sf::Vector2f trailPos = position_ - normalized(velocity_) * 0.8f;
        trail_.setPosition(Field::worldToScreen(trailPos, cameraCenter));
        target.draw(trail_);
    }

    shape_.setPosition(Field::worldToScreen(position_, cameraCenter));
    target.draw(shape_);
}

void Ball::kick(const sf::Vector2f& direction, float power, Player* kicker, KickType type) {
    owner_ = nullptr;
    sf::Vector2f dir = normalized(direction);
    velocity_ = dir * power;
    lastKicker_ = kicker;
    lastKickType_ = type;
    if (kicker) lastTouchTeam_ = kicker->getTeam();
    outOfPlay_ = false;
}

void Ball::applyForce(const sf::Vector2f& force) {
    velocity_ += force;
}

void Ball::collideWithPlayer(const Player& player) {
    sf::Vector2f diff = position_ - player.getPosition();
    float dist = length(diff);
    float minDist = BALL_RADIUS + (player.isGoalkeeper() ? GK_RADIUS : PLAYER_RADIUS);

    if (dist < minDist && dist > 0.001f) {
        sf::Vector2f n = normalized(diff);
        position_ = player.getPosition() + n * minDist;
        float relSpeed = length(velocity_ - player.getVelocity());
        velocity_ = n * relSpeed * 0.5f + player.getVelocity() * 0.3f;
        lastTouchTeam_ = player.getTeam();
    }
}
