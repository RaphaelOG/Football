#include "Game.h"
#include "Renderer.h"
#include "AI.h"
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <algorithm>

using namespace Constants;

Game::Game()
    : window_(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Football 11v11", sf::Style::Close,
              sf::ContextSettings(24, 8, 4, 2, 1))
    , homeTeam_(TeamSide::Home, "Blue FC")
    , awayTeam_(TeamSide::Away, "Red United")
    , state_(MatchState::Kickoff)
    , stateBeforePause_(MatchState::Playing)
    , kickoffTeam_(TeamSide::Home)
    , lastScorer_("")
    , stateTimer_(KICKOFF_DELAY)
    , matchTime_(0.f)
    , animTime_(0.f)
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
                if (state_ == MatchState::Paused) state_ = stateBeforePause_;
                else {
                    stateBeforePause_ = state_;
                    state_ = MatchState::Paused;
                }
            }
            if (event.key.code == sf::Keyboard::Q) switchPressed_ = true;
            if (event.key.code == sf::Keyboard::Space) shootPressed_ = true;
            if (event.key.code == sf::Keyboard::E) passPressed_ = true;
            if (event.key.code == sf::Keyboard::F) longPassPressed_ = true;
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
    sprint_ = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)
           || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);

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
    float goalX = homeTeam_.attacksRight() ? FIELD_LENGTH : 0.f;
    sf::Vector2f toGoal(goalX - controlled->getPosition().x, FIELD_WIDTH / 2.f - controlled->getPosition().y);
    if (length(moveInput_) > 0.2f) toGoal += sf::Vector2f(moveInput_.x * 6.f, moveInput_.y * 10.f);

    if (shootPressed_) {
        if (hasBall) controlled->kickBall(ball_, toGoal, SHOOT_POWER, 6.4f);
        else {
            Player* nearestOpp = nullptr;
            float best = TACKLE_RANGE;
            for (auto& opp : awayTeam_.getPlayers()) {
                float d = length(opp.getPosition() - controlled->getPosition());
                if (d < best) { best = d; nearestOpp = &opp; }
            }
            if (nearestOpp) controlled->attemptTackle(ball_, *nearestOpp);
        }
    } else if (passPressed_ && hasBall) {
        if (length(moveInput_) > 0.2f) {
            controlled->kickBall(ball_, moveInput_, PASS_POWER, 2.2f);
        } else {
            Player* target = AI::findBestPassTarget(*controlled, homeTeam_, awayTeam_);
            if (target) controlled->kickBall(ball_, target->getPosition() - controlled->getPosition(), PASS_POWER, 2.2f);
            else controlled->kickBall(ball_, toGoal, PASS_POWER, 2.2f);
        }
    } else if (longPassPressed_ && hasBall) {
        Player* target = AI::findBestPassTarget(*controlled, homeTeam_, awayTeam_);
        sf::Vector2f dir = target ? target->getPosition() - controlled->getPosition() : toGoal;
        if (length(moveInput_) > 0.2f) dir = moveInput_;
        controlled->kickBall(ball_, dir, LONG_PASS_POWER, 7.5f);
    }

    shootPressed_ = false;
    passPressed_ = false;
    longPassPressed_ = false;
}

static void separatePlayers(std::vector<Player>& players) {
    const float minDist = 1.45f;
    for (size_t i = 0; i < players.size(); ++i) {
        for (size_t j = i + 1; j < players.size(); ++j) {
            sf::Vector2f delta = players[j].getPosition() - players[i].getPosition();
            float dist = length(delta);
            if (dist >= minDist) continue;
            sf::Vector2f push = dist < 0.001f ? sf::Vector2f(minDist, 0.f)
                                              : normalized(delta) * (minDist - dist);
            players[i].shift(push * -0.5f);
            players[j].shift(push * 0.5f);
        }
    }
}

void Game::update(float dt) {
    if (state_ == MatchState::Paused) return;
    animTime_ += dt;

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
                resetKickoff(kickoffTeam_);
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
        p.update(dt, ctrl, homeTeam_.attacksRight());
    }
    for (auto& p : awayTeam_.getPlayers()) {
        p.update(dt, false, awayTeam_.attacksRight());
    }
    separatePlayers(homeTeam_.getPlayers());
    separatePlayers(awayTeam_.getPlayers());

    ball_.update(dt, field_);
    updatePossession();
    checkGoals();

    if (state_ == MatchState::Playing) {
        for (auto& p : homeTeam_.getPlayers()) ball_.collideWithPlayer(p);
        for (auto& p : awayTeam_.getPlayers()) ball_.collideWithPlayer(p);
    }

    updateCamera(dt);
}

void Game::updatePossession() {
    if (field_.inLeftNet(ball_.getPosition(), ball_.getHeight())
        || field_.inRightNet(ball_.getPosition(), ball_.getHeight())) {
        return;
    }

    if (ball_.getOwner()) {
        Player* owner = ball_.getOwner();
        if (length(ball_.getPosition() - owner->getPosition()) > POSSESSION_DIST + 1.f) {
            ball_.release();
        }
        return;
    }

    float height = ball_.getHeight();
    if (height > 1.35f && height > GOAL_HEIGHT) return;

    Player* nearest = nullptr;
    float nearestDist = 1e9f;
    bool ballFast = length(ball_.getVelocity()) > 8.f;

    auto checkTeam = [&](Team& team) {
        for (auto& p : team.getPlayers()) {
            float d = length(p.getPosition() - ball_.getPosition());
            float reach = p.isGoalkeeper() ? 2.35f : POSSESSION_DIST;
            if (height > 1.15f && !p.isGoalkeeper()) continue;
            if (ballFast && !p.isGoalkeeper()) continue;
            if (p.isGoalkeeper() && height > GOAL_HEIGHT) continue;
            if (d < reach && d < nearestDist) {
                nearestDist = d;
                nearest = &p;
            }
        }
    };

    checkTeam(homeTeam_);
    checkTeam(awayTeam_);

    if (nearest) nearest->takePossession(ball_);
}

void Game::checkGoals() {
    bool homeAttacksRight = homeTeam_.attacksRight();
    sf::Vector2f pos = ball_.getPosition();
    float height = ball_.getHeight();
    if (field_.inRightNet(pos, height)) scoreGoal(homeAttacksRight ? TeamSide::Home : TeamSide::Away);
    else if (field_.inLeftNet(pos, height)) scoreGoal(homeAttacksRight ? TeamSide::Away : TeamSide::Home);
}

void Game::scoreGoal(TeamSide scoringTeam) {
    if (state_ == MatchState::GoalCelebration) return;

    if (scoringTeam == TeamSide::Home) {
        homeTeam_.addGoal();
        kickoffTeam_ = TeamSide::Away;
        lastScorer_ = homeTeam_.getName();
    } else {
        awayTeam_.addGoal();
        kickoffTeam_ = TeamSide::Home;
        lastScorer_ = awayTeam_.getName();
    }

    state_ = MatchState::GoalCelebration;
    stateTimer_ = GOAL_CELEBRATION;
    ball_.release();
}

void Game::resetKickoff(TeamSide kickingTeam) {
    state_ = MatchState::Kickoff;
    stateTimer_ = KICKOFF_DELAY;

    sf::Vector2f kickPos = field_.getKickoffPosition(kickingTeam);

    Team& kicking = kickingTeam == TeamSide::Home ? homeTeam_ : awayTeam_;
    Team& defending = kickingTeam == TeamSide::Home ? awayTeam_ : homeTeam_;

    bool kickAttacksRight = (kickingTeam == TeamSide::Home) ? (half_ == 1) : (half_ == 2);
    kicking.resetPositions(kickAttacksRight);
    defending.resetPositions(!kickAttacksRight);

    Player* taker = nullptr;
    for (auto& p : kicking.getPlayers()) {
        if (p.getPositionRole() == Position::CM) { taker = &p; break; }
    }
    if (!taker) taker = &kicking.getPlayers()[6];

    sf::Vector2f behind = kickAttacksRight ? sf::Vector2f(-1.35f, 0.f) : sf::Vector2f(1.35f, 0.f);
    taker->reset(kickPos + behind);
    ball_.reset(kickPos);
    taker->takePossession(ball_);
    if (kickingTeam == TeamSide::Home) homeTeam_.setControlledPlayer(taker);

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
    renderer_->drawScene(window_, homeTeam_, awayTeam_, ball_,
                         homeTeam_.getControlledPlayer(), cameraCenter_, animTime_, 1.f / 60.f);

    window_.pushGLStates();
    renderer_->drawLabels(window_);
    renderer_->drawHUD(window_, *this);

    const Team& kicking = kickoffTeam_ == TeamSide::Home ? homeTeam_ : awayTeam_;
    switch (state_) {
        case MatchState::Kickoff:
            renderer_->drawBanner(window_, "KICK OFF\n" + kicking.getName(), sf::Color::White);
            break;
        case MatchState::GoalCelebration:
            renderer_->drawBanner(window_, "GOAL!\n" + lastScorer_, sf::Color::Yellow);
            break;
        case MatchState::HalfTime:
            renderer_->drawBanner(window_, "HALF TIME\nSwitching ends", sf::Color::White);
            break;
        case MatchState::FullTime:
            renderer_->drawBanner(window_,
                "FULL TIME\n" + homeTeam_.getName() + "  " + std::to_string(homeTeam_.getScore())
                + " - " + std::to_string(awayTeam_.getScore()) + "  " + awayTeam_.getName(),
                sf::Color::White);
            break;
        case MatchState::Paused:
            renderer_->drawBanner(window_, "PAUSED", sf::Color::White);
            break;
        default:
            break;
    }
    window_.popGLStates();
    window_.display();
}

std::string Game::formatTime() const {
    int mins = static_cast<int>(matchTime_ / MATCH_DURATION * 90.f);
    return std::to_string(mins) + "'";
}
