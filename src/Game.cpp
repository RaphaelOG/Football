#include "Game.h"
#include "Renderer.h"
#include "AI.h"
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <algorithm>

using namespace Constants;

Game::Game()
    : window_(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Football 11v11", sf::Style::Close)
    , homeTeam_(TeamSide::Home, "Blue FC")
    , awayTeam_(TeamSide::Away, "Red United")
    , state_(MatchState::Kickoff)
    , kickoffTeam_(TeamSide::Home)
    , stateTimer_(KICKOFF_DELAY)
    , matchTime_(0.f)
    , half_(1)
    , cameraCenter_(FIELD_LENGTH / 2.f, FIELD_WIDTH / 2.f)
    , cameraTarget_(cameraCenter_)
    , moveInput_(0.f, 0.f)
    , sprint_(false)
    , shootPressed_(false)
    , passPressed_(false)
    , longPassPressed_(false)
    , tacklePressed_(false)
    , switchPressed_(false)
{
    window_.setFramerateLimit(60);
    renderer_ = std::make_unique<Renderer>();

    homeTeam_.resetPositions(true);
    awayTeam_.resetPositions(false);
    resetKickoff(TeamSide::Home);
}

Game::~Game() = default;

void Game::run() {
  sf::Clock clock;
    while (window_.isOpen()) {
        float dt = std::min(clock.restart().asSeconds(), 0.05f);
        handleEvents();
        update(dt);
        render();
    }
}

void Game::handleEvents() {
    sf::Event event;
    while (window_.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window_.close();
        }
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) window_.close();
            if (event.key.code == sf::Keyboard::P) {
                state_ = state_ == MatchState::Paused ? MatchState::Playing : MatchState::Paused;
            }
            if (event.key.code == sf::Keyboard::Q) switchPressed_ = true;
            if (event.key.code == sf::Keyboard::Space) shootPressed_ = true;
            if (event.key.code == sf::Keyboard::E) passPressed_ = true;
            if (event.key.code == sf::Keyboard::LShift || event.key.code == sf::Keyboard::RShift) sprint_ = true;
            if (event.key.code == sf::Keyboard::C) tacklePressed_ = true;
        }
        if (event.type == sf::Event::KeyReleased) {
            if (event.key.code == sf::Keyboard::LShift || event.key.code == sf::Keyboard::RShift) sprint_ = false;
        }
    }
}

void Game::processInput(float dt) {
    (void)dt;
    moveInput_ = {0.f, 0.f};

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
        moveInput_.y -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
        moveInput_.y += 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        moveInput_.x -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        moveInput_.x += 1.f;

    if (length(moveInput_) > 0.f) moveInput_ = normalized(moveInput_);

    Player* controlled = homeTeam_.getControlledPlayer();
    if (!controlled) return;

    controlled->setMoveInput(moveInput_, sprint_);

    if (switchPressed_) {
        homeTeam_.switchToNearestPlayer(ball_.getPosition());
        switchPressed_ = false;
    }

    if (tacklePressed_) {
        Player* owner = ball_.getOwner();
        if (owner && owner->getTeam() != TeamSide::Home) {
            controlled->attemptTackle(ball_, *owner);
        }
        tacklePressed_ = false;
    }

    bool hasBall = controlled->hasBall(ball_);

    if (shootPressed_ && hasBall) {
        sf::Vector2f toGoal(FIELD_LENGTH - controlled->getPosition().x, FIELD_WIDTH / 2.f - controlled->getPosition().y);
        controlled->kickBall(ball_, toGoal, SHOOT_POWER);
        shootPressed_ = false;
    } else if (passPressed_ && hasBall) {
        Player* target = AI::findBestPassTarget(*controlled, homeTeam_, awayTeam_);
        if (target) {
            sf::Vector2f dir = target->getPosition() - controlled->getPosition();
            controlled->kickBall(ball_, dir, PASS_POWER);
        } else {
            sf::Vector2f toGoal(FIELD_LENGTH - controlled->getPosition().x, FIELD_WIDTH / 2.f - controlled->getPosition().y);
            controlled->kickBall(ball_, toGoal, PASS_POWER);
        }
        passPressed_ = false;
    } else if (longPassPressed_ && hasBall) {
        Player* target = AI::findBestPassTarget(*controlled, homeTeam_, awayTeam_);
        if (target) {
            sf::Vector2f dir = target->getPosition() - controlled->getPosition();
            controlled->kickBall(ball_, dir, LONG_PASS_POWER);
        }
        longPassPressed_ = false;
    }

    shootPressed_ = false;
}

void Game::update(float dt) {
    if (state_ == MatchState::Paused) return;

    if (state_ == MatchState::Kickoff || state_ == MatchState::GoalCelebration || state_ == MatchState::HalfTime) {
        stateTimer_ -= dt;
        if (stateTimer_ <= 0.f) {
            if (state_ == MatchState::HalfTime) {
                half_ = 2;
                homeTeam_.setAttacksRight(false);
                awayTeam_.setAttacksRight(true);
                homeTeam_.resetPositions(false);
                awayTeam_.resetPositions(true);
                resetKickoff(TeamSide::Away);
            } else if (state_ == MatchState::GoalCelebration) {
                resetKickoff(kickoffTeam_ == TeamSide::Home ? TeamSide::Away : TeamSide::Home);
            } else {
                state_ = MatchState::Playing;
            }
        }
        updateCamera(dt);
        return;
    }

    if (state_ == MatchState::FullTime) {
        updateCamera(dt);
        return;
    }

    // Playing
    matchTime_ += dt;
    if (matchTime_ >= HALF_DURATION && half_ == 1) {
        state_ = MatchState::HalfTime;
        stateTimer_ = 4.f;
        return;
    }
    if (matchTime_ >= MATCH_DURATION) {
        state_ = MatchState::FullTime;
        return;
    }

    processInput(dt);

    AI::updateTeam(homeTeam_, awayTeam_, ball_, dt, true);
    AI::updateTeam(awayTeam_, homeTeam_, ball_, dt, false);

    for (auto& p : homeTeam_.getPlayers()) {
        bool ctrl = homeTeam_.getControlledPlayer() == &p;
        p.update(dt, ctrl);
    }
    for (auto& p : awayTeam_.getPlayers()) {
        p.update(dt, false);
    }

    ball_.update(dt, field_);
    updatePossession();
    checkGoals();

    // Ball-player collisions
    for (auto& p : homeTeam_.getPlayers()) ball_.collideWithPlayer(p);
    for (auto& p : awayTeam_.getPlayers()) ball_.collideWithPlayer(p);

    updateCamera(dt);
}

void Game::updatePossession() {
    if (ball_.getOwner()) {
        Player* owner = ball_.getOwner();
        if (length(ball_.getPosition() - owner->getPosition()) > POSSESSION_DIST + 1.f) {
            ball_.release();
        }
        return;
    }

    // Only allow possession if ball is slow enough
    if (length(ball_.getVelocity()) > 8.f) return;

    Player* nearest = nullptr;
    float nearestDist = POSSESSION_DIST;

    auto checkTeam = [&](Team& team) {
        for (auto& p : team.getPlayers()) {
            float d = length(p.getPosition() - ball_.getPosition());
            if (d < nearestDist) {
                nearestDist = d;
                nearest = &p;
            }
        }
    };

    checkTeam(homeTeam_);
    checkTeam(awayTeam_);

    if (nearest) {
        nearest->takePossession(ball_);
    }
}

void Game::checkGoals() {
    sf::Vector2f pos = ball_.getPosition();
    if (field_.isGoal(pos, TeamSide::Home)) {
        scoreGoal(TeamSide::Home);
    } else if (field_.isGoal(pos, TeamSide::Away)) {
        scoreGoal(TeamSide::Away);
    }
}

void Game::scoreGoal(TeamSide scoringTeam) {
    if (state_ == MatchState::GoalCelebration) return;

    if (scoringTeam == TeamSide::Home) {
        homeTeam_.addGoal();
        kickoffTeam_ = TeamSide::Away;
    } else {
        awayTeam_.addGoal();
        kickoffTeam_ = TeamSide::Home;
    }

    state_ = MatchState::GoalCelebration;
    stateTimer_ = GOAL_CELEBRATION;
    ball_.release();
}

void Game::resetKickoff(TeamSide kickingTeam) {
    state_ = MatchState::Kickoff;
    stateTimer_ = KICKOFF_DELAY;

    sf::Vector2f kickPos = field_.getKickoffPosition(kickingTeam);
    ball_.reset(kickPos);

    // Position kickoff players
    Team& kicking = kickingTeam == TeamSide::Home ? homeTeam_ : awayTeam_;
    Team& defending = kickingTeam == TeamSide::Home ? awayTeam_ : homeTeam_;

    kicking.resetPositions(kickingTeam == TeamSide::Home ? (half_ == 1) : (half_ == 2));
    defending.resetPositions(kickingTeam == TeamSide::Home ? (half_ == 2) : (half_ == 1));

    // Move kickoff taker near ball
    Player* taker = nullptr;
    float bestDist = 999.f;
    for (auto& p : kicking.getPlayers()) {
        if (p.isGoalkeeper()) continue;
        float d = length(p.getHomePosition() - kickPos);
        if (d < bestDist) { bestDist = d; taker = &p; }
    }
    if (taker) {
        taker->reset(kickPos + sf::Vector2f(kickingTeam == TeamSide::Home ? -1.f : 1.f, 0.f));
    }

    cameraCenter_ = {FIELD_LENGTH / 2.f, FIELD_WIDTH / 2.f};
    cameraTarget_ = cameraCenter_;
}

void Game::updateCamera(float dt) {
    cameraTarget_ = ball_.getPosition();
    sf::Vector2f diff = cameraTarget_ - cameraCenter_;
    cameraCenter_ += diff * CAMERA_SMOOTH * dt;

    // Clamp camera
    float margin = 15.f;
    cameraCenter_.x = std::max(margin, std::min(FIELD_LENGTH - margin, cameraCenter_.x));
    cameraCenter_.y = std::max(margin, std::min(FIELD_WIDTH - margin, cameraCenter_.y));
}

void Game::render() {
    window_.clear(sf::Color(20, 60, 20));

    renderer_->drawField(window_, field_, cameraCenter_);

    for (const auto& p : awayTeam_.getPlayers()) {
        p.draw(window_, cameraCenter_, false);
    }
    for (const auto& p : homeTeam_.getPlayers()) {
        bool ctrl = homeTeam_.getControlledPlayer() == &p;
        p.draw(window_, cameraCenter_, ctrl);
    }

    ball_.draw(window_, cameraCenter_);
    renderer_->drawHUD(window_, *this);

    switch (state_) {
        case MatchState::Kickoff:
            renderer_->drawOverlay(window_, "KICK OFF", sf::Color::White);
            break;
        case MatchState::GoalCelebration:
            renderer_->drawOverlay(window_, "GOAL!", sf::Color::Yellow);
            break;
        case MatchState::HalfTime:
            renderer_->drawOverlay(window_, "HALF TIME", sf::Color::White);
            break;
        case MatchState::FullTime: {
            std::string msg = "FULL TIME\n" + std::to_string(homeTeam_.getScore()) +
                              " - " + std::to_string(awayTeam_.getScore());
            renderer_->drawOverlay(window_, msg, sf::Color::White);
            break;
        }
        case MatchState::Paused:
            renderer_->drawOverlay(window_, "PAUSED", sf::Color::White);
            break;
        default:
            break;
    }

    window_.display();
}

std::string Game::formatTime() const {
    int mins = static_cast<int>(matchTime_ / MATCH_DURATION * 90.f);
    return std::to_string(mins) + "'";
}
