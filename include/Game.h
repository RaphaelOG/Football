#pragma once

#include "Team.h"
#include "Ball.h"
#include "Field.h"
#include "MatchStats.h"
#include "Constants.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <memory>

using namespace Constants;

class Renderer;

class Game {
public:
    Game();
    ~Game();

    void run();
    void handleEvents();
    void update(float dt);
    void render();

    MatchState getState() const { return state_; }
    float getMatchTime() const { return matchTime_; }
    int getHalf() const { return half_; }
    const Team& getHomeTeam() const { return homeTeam_; }
    const Team& getAwayTeam() const { return awayTeam_; }
    const MatchStats& getStats() const { return stats_; }
    const Field& getField() const { return field_; }
    const Ball& getBall() const { return ball_; }
    const sf::Vector2f& getCameraCenter() const { return cameraCenter_; }

private:
    void startNewMatch();
    void resetKickoff(TeamSide kickingTeam);
    void setupSetPiece(const SetPieceInfo& info);
    void performKickoffPass();
    void performSetPiecePass();
    void checkGoals();
    void checkOutOfBounds();
    void updatePossession();
    void updateCamera(float dt);
    void processInput(float dt);
    void scoreGoal(TeamSide scoringTeam);
    void recordKickStats(Player& kicker, KickType type);
    std::string formatTime() const;
    std::string getStateOverlayMessage() const;

    sf::RenderWindow window_;
    std::unique_ptr<Renderer> renderer_;

    Field field_;
    Ball ball_;
    Team homeTeam_;
    Team awayTeam_;
    MatchStats stats_;

    MatchState state_;
    TeamSide kickoffTeam_;
    TeamSide lastTouchTeam_;
    SetPieceInfo pendingSetPiece_;
    float stateTimer_;
    float oobCooldown_;
    float matchTime_;
    int half_;
    sf::Vector2f cameraCenter_;
    sf::Vector2f cameraTarget_;

    sf::Vector2f moveInput_;
    bool sprint_;
    bool shootPressed_;
    bool passPressed_;
    bool longPassPressed_;
    bool tacklePressed_;
    bool switchPressed_;
    bool startPressed_;
    bool restartPressed_;
};
