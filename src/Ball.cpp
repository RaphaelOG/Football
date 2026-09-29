#include "Ball.h"
#include "Player.h"
#include "Field.h"

using namespace Constants;

Ball::Ball() : owner_(nullptr) {
    shape_.setRadius(BALL_RADIUS * PIXELS_PER_METER);
    shape_.setOrigin(BALL_RADIUS * PIXELS_PER_METER, BALL_RADIUS * PIXELS_PER_METER);
    shape_.setFillColor(sf::Color::White);
    shape_.setOutlineThickness(1.f);
    shape_.setOutlineColor(sf::Color(30, 30, 30));
    reset({FIELD_LENGTH / 2.f, FIELD_WIDTH / 2.f});
}

void Ball::reset(const sf::Vector2f& position) {
    position_ = position;
    velocity_ = {0.f, 0.f};
    height_ = BALL_RADIUS;
    verticalVelocity_ = 0.f;
    owner_ = nullptr;
}

void Ball::update(float dt, const Field& field) {
    (void)field;
    if (owner_) {
        sf::Vector2f facing = owner_->getFacing();
        if (length(facing) < 0.1f) facing = {1.f, 0.f};
        float offset = owner_->isGoalkeeper() ? 0.65f : 0.9f;
        position_ = owner_->getPosition() + facing * offset;
        velocity_ = owner_->getVelocity();
        height_ = BALL_RADIUS;
        verticalVelocity_ = 0.f;
        return;
    }

    float speed = length(velocity_);
    bool airborne = height_ > BALL_RADIUS + 0.08f;
    if (!airborne && speed > 0.01f) {
        float friction = BALL_FRICTION * dt;
        if (speed <= friction) velocity_ = {0.f, 0.f};
        else velocity_ = velocity_ - normalized(velocity_) * friction;
    } else if (airborne) {
        velocity_ *= (1.f - 0.12f * dt);
    }

    verticalVelocity_ -= 9.8f * dt;
    height_ += verticalVelocity_ * dt;
    if (height_ < BALL_RADIUS) {
        height_ = BALL_RADIUS;
        if (std::abs(verticalVelocity_) > 1.4f) verticalVelocity_ = -verticalVelocity_ * 0.32f;
        else verticalVelocity_ = 0.f;
    }

    position_ += velocity_ * dt;
    resolveBounds();

    speed = length(velocity_);
    if (speed > BALL_MAX_SPEED) velocity_ = normalized(velocity_) * BALL_MAX_SPEED;
}

void Ball::resolveBounds() {
    float goalTop = (FIELD_WIDTH - GOAL_WIDTH) / 2.f;
    float goalBot = goalTop + GOAL_WIDTH;
    bool mouth = position_.y >= goalTop && position_.y <= goalBot;
    bool underBar = height_ <= GOAL_HEIGHT - 0.02f;

    auto keepInMouth = [&]() {
        if (position_.y < goalTop) {
            position_.y = goalTop;
            velocity_.y = std::abs(velocity_.y) * 0.4f;
        }
        if (position_.y > goalBot) {
            position_.y = goalBot;
            velocity_.y = -std::abs(velocity_.y) * 0.4f;
        }
    };

    if (position_.x < 0.f) {
        if (mouth && underBar) {
            position_.x = std::max(position_.x, -GOAL_DEPTH + 0.25f);
            keepInMouth();
            velocity_ *= 0.86f;
        } else {
            position_.x = 0.12f;
            velocity_.x = std::abs(velocity_.x) * 0.42f;
            if (mouth && !underBar) verticalVelocity_ = -std::abs(verticalVelocity_) * 0.45f;
        }
    } else if (position_.x > FIELD_LENGTH) {
        if (mouth && underBar) {
            position_.x = std::min(position_.x, FIELD_LENGTH + GOAL_DEPTH - 0.25f);
            keepInMouth();
            velocity_ *= 0.86f;
        } else {
            position_.x = FIELD_LENGTH - 0.12f;
            velocity_.x = -std::abs(velocity_.x) * 0.42f;
            if (mouth && !underBar) verticalVelocity_ = -std::abs(verticalVelocity_) * 0.45f;
        }
    }

    if (position_.y < BALL_RADIUS) {
        position_.y = BALL_RADIUS;
        velocity_.y = std::abs(velocity_.y) * 0.55f;
    }
    if (position_.y > FIELD_WIDTH - BALL_RADIUS) {
        position_.y = FIELD_WIDTH - BALL_RADIUS;
        velocity_.y = -std::abs(velocity_.y) * 0.55f;
    }
}

void Ball::draw(sf::RenderTarget& target, const sf::Vector2f& cameraCenter) const {
    shape_.setPosition(Field::worldToScreen(position_, cameraCenter));
    target.draw(shape_);
}

void Ball::kick(const sf::Vector2f& direction, float power, float loft) {
    owner_ = nullptr;
    sf::Vector2f dir = normalized(direction);
    if (length(dir) < 0.1f) dir = {1.f, 0.f};
    velocity_ = dir * std::min(power, BALL_MAX_SPEED);
    verticalVelocity_ = loft;
    height_ = BALL_RADIUS + 0.04f;
}

void Ball::applyForce(const sf::Vector2f& force) {
    velocity_ += force;
}

void Ball::collideWithPlayer(const Player& player) {
    if (owner_ == &player || height_ > 1.5f) return;
    sf::Vector2f diff = position_ - player.getPosition();
    float dist = length(diff);
    float minDist = BALL_RADIUS + (player.isGoalkeeper() ? GK_RADIUS : PLAYER_RADIUS);

    if (dist < minDist && dist > 0.001f) {
        sf::Vector2f n = normalized(diff);
        position_ = player.getPosition() + n * minDist;
        float relSpeed = length(velocity_ - player.getVelocity());
        velocity_ = n * relSpeed * 0.5f + player.getVelocity() * 0.3f;
    }
}
