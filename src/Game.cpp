#include "Game.h"
#include "Renderer.h"
#include "AI.h"
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <algorithm>
#include <cstdlib>

using namespace Constants;

Game::Game()
    : window_(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Football 11v11", sf::Style::Close)
    , homeTeam_(TeamSide::Home, "Blue FC")
    , awayTeam_(TeamSide::Away, "Red United")
    , state_(MatchState::MainMenu)
    , kickoffTeam_(TeamSide::Home)
    , lastTouchTeam_(TeamSide::Home)
    , stateTimer_(0.f)
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
    , startPressed_(false)
    , restartPressed_(false)
    , oobCooldown_(0.f)
{
    window_.setFramerateLimit(60);
    renderer_ = std::make_unique<Renderer>();
}

Game::~Game() = default;

void Game::startNewMatch() {
    stats_.reset();
    homeTeam_.resetScore();
    awayTeam_.resetScore();
    matchTime_ = 0.f;
    half_ = 1;
    lastTouchTeam_ = TeamSide::Home;
    homeTeam_.resetPositions(true);
    awayTeam_.resetPositions(false);
    resetKickoff(TeamSide::Home);
}

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
            if (event.key.code == sf::Keyboard::Escape) {
                if (state_ == MatchState::MainMenu) window_.close();
                else if (state_ == MatchState::FullTime) window_.close();
                else state_ = MatchState::MainMenu;
            }
            if (event.key.code == sf::Keyboard::Return || event.key.code == sf::Keyboard::Space) {
                if (state_ == MatchState::MainMenu) startPressed_ = true;
            }
            if (event.key.code == sf::Keyboard::R && state_ == MatchState::FullTime) {
                restartPressed_ = true;
            }
            if (event.key.code == sf::Keyboard::P && state_ != MatchState::MainMenu) {
                state_ = state_ == MatchState::Paused ? MatchState::Playing : MatchState::Paused;
            }
            if (event.key.code == sf::Keyboard::Q) switchPressed_ = true;
            if (event.key.code == sf::Keyboard::Tab) {
                if (state_ == MatchState::Playing)
                    homeTeam_.switchToNearestPlayer(ball_.getPosition());
            }
            if (event.key.code == sf::Keyboard::Space && state_ == MatchState::Playing) shootPressed_ = true;
            if (event.key.code == sf::Keyboard::E) passPressed_ = true;
            if (event.key.code == sf::Keyboard::R && state_ == MatchState::Playing) longPassPressed_ = true;
            if (event.key.code == sf::Keyboard::LShift || event.key.code == sf::Keyboard::RShift) sprint_ = true;
            if (event.key.code == sf::Keyboard::C) tacklePressed_ = true;
        }
        if (event.type == sf::Event::KeyReleased) {
            if (event.key.code == sf::Keyboard::LShift || event.key.code == sf::Keyboard::RShift) sprint_ = false;
        }
    }
}

void Game::recordKickStats(Player& kicker, KickType type) {
  TeamSide team = kicker.getTeam();
    if (type == KickType::Shoot) {
        stats_.recordShot(team);
    } else {
        stats_.recordPass(team);
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
        homeTeam_.switchToNextOutfieldPlayer();
        switchPressed_ = false;
    }

    if (tacklePressed_) {
        Player* owner = ball_.getOwner();
        if (owner && owner->getTeam() != TeamSide::Home) {
            controlled->attemptTackle(ball_, *owner);
            stats_.recordTackle(TeamSide::Home);
            if (static_cast<float>(rand()) / RAND_MAX < FOUL_CHANCE && !owner->isGoalkeeper()) {
                stats_.recordFoul(TeamSide::Home, controlled->getName());
            }
        }
        tacklePressed_ = false;
    }

    bool hasBall = controlled->hasBall(ball_);

    if (shootPressed_ && hasBall) {
        sf::Vector2f toGoal(FIELD_LENGTH - controlled->getPosition().x, FIELD_WIDTH / 2.f - controlled->getPosition().y);
        controlled->kickBall(ball_, toGoal, SHOOT_POWER, KickType::Shoot);
        recordKickStats(*controlled, KickType::Shoot);
        shootPressed_ = false;
    } else if (passPressed_ && hasBall) {
        Player* target = AI::findBestPassTarget(*controlled, homeTeam_, awayTeam_);
        if (target) {
            sf::Vector2f dir = target->getPosition() - controlled->getPosition();
            controlled->kickBall(ball_, dir, PASS_POWER, KickType::Pass);
        } else {
            sf::Vector2f toGoal(FIELD_LENGTH - controlled->getPosition().x, FIELD_WIDTH / 2.f - controlled->getPosition().y);
            controlled->kickBall(ball_, toGoal, PASS_POWER, KickType::Pass);
        }
        recordKickStats(*controlled, KickType::Pass);
        passPressed_ = false;
    } else if (longPassPressed_ && hasBall) {
        Player* target = AI::findBestPassTarget(*controlled, homeTeam_, awayTeam_, nullptr, true);
        if (target) {
            sf::Vector2f dir = target->getPosition() - controlled->getPosition();
            controlled->kickBall(ball_, dir, LONG_PASS_POWER, KickType::LongPass);
            recordKickStats(*controlled, KickType::LongPass);
            stats_.addEvent("Long ball from " + controlled->getName());
        }
        longPassPressed_ = false;
    }

    shootPressed_ = false;
}

void Game::performKickoffPass() {
    Team& kicking = kickoffTeam_ == TeamSide::Home ? homeTeam_ : awayTeam_;
    Player* taker = nullptr;
    float bestDist = 999.f;
    sf::Vector2f kickPos = ball_.getPosition();

    for (auto& p : kicking.getPlayers()) {
        if (p.isGoalkeeper()) continue;
        float d = length(p.getPosition() - kickPos);
        if (d < bestDist) { bestDist = d; taker = &p; }
    }
    if (!taker) return;

    Player* target = nullptr;
    for (auto& p : kicking.getPlayers()) {
        if (&p == taker || p.isGoalkeeper()) continue;
        target = &p;
        break;
    }
    if (target) {
        sf::Vector2f dir = target->getPosition() - taker->getPosition();
        taker->kickBall(ball_, dir, PASS_POWER * 0.7f, KickType::Pass);
        recordKickStats(*taker, KickType::Pass);
    }
}

void Game::performSetPiecePass() {
    Team& taking = pendingSetPiece_.takingTeam == TeamSide::Home ? homeTeam_ : awayTeam_;
    Team& other = pendingSetPiece_.takingTeam == TeamSide::Home ? awayTeam_ : homeTeam_;
    sf::Vector2f pos = pendingSetPiece_.position;

    Player* taker = nullptr;
    if (pendingSetPiece_.type == OutOfBoundsType::GoalKick) {
        taker = taking.getGoalkeeper();
    } else {
        float bestDist = 999.f;
        for (auto& p : taking.getPlayers()) {
            if (p.isGoalkeeper()) continue;
            float d = length(p.getHomePosition() - pos);
            if (d < bestDist) { bestDist = d; taker = &p; }
        }
    }
    if (!taker) return;

    sf::Vector2f inward = {FIELD_LENGTH / 2.f - pos.x, FIELD_WIDTH / 2.f - pos.y};
    inward = normalized(inward);
    taker->reset(pos + inward * 1.2f);

    sf::Vector2f center(FIELD_LENGTH / 2.f, FIELD_WIDTH / 2.f);
    float power = PASS_POWER;
    if (pendingSetPiece_.type == OutOfBoundsType::GoalKick) power = LONG_PASS_POWER * 0.85f;
    if (pendingSetPiece_.type == OutOfBoundsType::CornerKick) power = PASS_POWER * 1.15f;

    Player* target = AI::findBestPassTarget(*taker, taking, other, nullptr, true);
    sf::Vector2f dir = target ? target->getPosition() - taker->getPosition() : center - taker->getPosition();
    if (length(dir) < 0.1f) dir = inward;

    taker->kickBall(ball_, dir, power, KickType::Pass);
    recordKickStats(*taker, KickType::Pass);
    oobCooldown_ = OOB_COOLDOWN;
}

void Game::setupSetPiece(const SetPieceInfo& info) {
    pendingSetPiece_ = info;
    ball_.reset(info.position);
    ball_.setOutOfPlay(true);

    switch (info.type) {
        case OutOfBoundsType::ThrowIn:
            state_ = MatchState::ThrowIn;
            stats_.addEvent("Throw-in");
            break;
        case OutOfBoundsType::GoalKick:
            state_ = MatchState::GoalKick;
            stats_.addEvent("Goal kick");
            break;
        case OutOfBoundsType::CornerKick:
            state_ = MatchState::CornerKick;
            stats_.addEvent("Corner kick");
            break;
        default:
            return;
    }
    stateTimer_ = SET_PIECE_DELAY;
}

void Game::checkOutOfBounds() {
    if (ball_.getOwner() || state_ != MatchState::Playing || oobCooldown_ > 0.f) return;

    SetPieceInfo info = field_.checkOutOfBounds(ball_.getPosition(), lastTouchTeam_);
    if (info.type == OutOfBoundsType::None) return;

    ball_.setVelocity({0.f, 0.f});
    ball_.release();
    setupSetPiece(info);
}

void Game::update(float dt) {
    if (state_ == MatchState::MainMenu) {
        if (startPressed_) {
            startPressed_ = false;
            startNewMatch();
        }
        return;
    }

    if (state_ == MatchState::Paused) return;

    if (state_ == MatchState::Kickoff || state_ == MatchState::GoalCelebration ||
        state_ == MatchState::HalfTime || state_ == MatchState::ThrowIn ||
        state_ == MatchState::GoalKick || state_ == MatchState::CornerKick) {
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
            } else if (state_ == MatchState::Kickoff) {
                performKickoffPass();
                state_ = MatchState::Playing;
            } else if (state_ == MatchState::ThrowIn || state_ == MatchState::GoalKick ||
                       state_ == MatchState::CornerKick) {
                performSetPiecePass();
                ball_.setOutOfPlay(false);
                state_ = MatchState::Playing;
                oobCooldown_ = OOB_COOLDOWN;
            }
        }
        updateCamera(dt);
        return;
    }

    if (state_ == MatchState::FullTime) {
        if (restartPressed_) {
            restartPressed_ = false;
            startNewMatch();
        }
        updateCamera(dt);
        return;
    }

    matchTime_ += dt;
    if (oobCooldown_ > 0.f) oobCooldown_ -= dt;

    if (ball_.getOwner()) {
        stats_.updatePossession(ball_.getOwner()->getTeam(), dt);
        lastTouchTeam_ = ball_.getOwner()->getTeam();
    } else if (ball_.getLastKicker()) {
        lastTouchTeam_ = ball_.getLastTouchTeam();
    }

    if (matchTime_ >= HALF_DURATION && half_ == 1) {
        state_ = MatchState::HalfTime;
        stateTimer_ = 4.f;
        stats_.addEvent("Half time");
        return;
    }
    if (matchTime_ >= MATCH_DURATION) {
        state_ = MatchState::FullTime;
        stats_.addEvent("Full time!");
        return;
    }

    processInput(dt);

    AI::updateTeam(homeTeam_, awayTeam_, ball_, dt, true, stats_);
    AI::updateTeam(awayTeam_, homeTeam_, ball_, dt, false, stats_);

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
    if (state_ == MatchState::Playing) checkOutOfBounds();

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
        if (nearest->getTeam() == TeamSide::Home && !nearest->isGoalkeeper()) {
            homeTeam_.setControlledPlayer(nearest);
        }
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

    std::string scorer = "Unknown";
    if (ball_.getLastKicker() && ball_.getLastKickType() == KickType::Shoot) {
        scorer = ball_.getLastKicker()->getName();
    } else if (ball_.getLastKicker()) {
        scorer = ball_.getLastKicker()->getName();
    }

    if (scoringTeam == TeamSide::Home) {
        homeTeam_.addGoal();
        kickoffTeam_ = TeamSide::Away;
    } else {
        awayTeam_.addGoal();
        kickoffTeam_ = TeamSide::Home;
    }

    stats_.recordGoal(scoringTeam, scorer);
    state_ = MatchState::GoalCelebration;
    stateTimer_ = GOAL_CELEBRATION;
    ball_.release();
    ball_.setOutOfPlay(true);
}

void Game::resetKickoff(TeamSide kickingTeam) {
    state_ = MatchState::Kickoff;
    stateTimer_ = KICKOFF_DELAY;
    kickoffTeam_ = kickingTeam;

    sf::Vector2f kickPos = field_.getKickoffPosition(kickingTeam);
    ball_.reset(kickPos);
    ball_.setOutOfPlay(true);

    Team& kicking = kickingTeam == TeamSide::Home ? homeTeam_ : awayTeam_;
    Team& defending = kickingTeam == TeamSide::Home ? awayTeam_ : homeTeam_;

    kicking.resetPositions(kickingTeam == TeamSide::Home ? (half_ == 1) : (half_ == 2));
    defending.resetPositions(kickingTeam == TeamSide::Home ? (half_ == 2) : (half_ == 1));

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

    float margin = 15.f;
    cameraCenter_.x = std::max(margin, std::min(FIELD_LENGTH - margin, cameraCenter_.x));
    cameraCenter_.y = std::max(margin, std::min(FIELD_WIDTH - margin, cameraCenter_.y));
}

std::string Game::getStateOverlayMessage() const {
    switch (state_) {
        case MatchState::Kickoff: return "KICK OFF";
        case MatchState::GoalCelebration: {
            std::string msg = "GOAL!\n" + stats_.getLastScorer();
            return msg;
        }
        case MatchState::HalfTime: return "HALF TIME";
        case MatchState::FullTime: {
            return "FULL TIME\n" + std::to_string(homeTeam_.getScore()) +
                   " - " + std::to_string(awayTeam_.getScore()) +
                   "\n\nPress R to restart | Esc for menu";
        }
        case MatchState::Paused: return "PAUSED";
        case MatchState::ThrowIn: return "THROW IN";
        case MatchState::GoalKick: return "GOAL KICK";
        case MatchState::CornerKick: return "CORNER";
        default: return "";
    }
}

void Game::render() {
    window_.clear(sf::Color(20, 60, 20));

    if (state_ == MatchState::MainMenu) {
        renderer_->drawMainMenu(window_);
        window_.display();
        return;
    }

    renderer_->drawField(window_, field_, cameraCenter_);

    for (const auto& p : awayTeam_.getPlayers()) {
        p.draw(window_, cameraCenter_, false);
    }
    for (const auto& p : homeTeam_.getPlayers()) {
        bool ctrl = homeTeam_.getControlledPlayer() == &p;
        p.draw(window_, cameraCenter_, ctrl);
    }

    renderer_->drawPlayerLabels(window_, homeTeam_, awayTeam_, cameraCenter_);
    ball_.draw(window_, cameraCenter_);
    renderer_->drawHUD(window_, *this);

    std::string overlay = getStateOverlayMessage();
    if (!overlay.empty()) {
        sf::Color color = state_ == MatchState::GoalCelebration ? sf::Color::Yellow : sf::Color::White;
        renderer_->drawOverlay(window_, overlay, color);
    }

    if (state_ == MatchState::FullTime) {
        renderer_->drawMatchSummary(window_, *this);
    }

    window_.display();
}

std::string Game::formatTime() const {
    int mins = static_cast<int>(matchTime_ / MATCH_DURATION * 90.f);
    return std::to_string(mins) + "'";
}
