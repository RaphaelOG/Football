#pragma once

#include "Team.h"
#include "Ball.h"
#include "Field.h"
#include "Constants.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <memory>
#include <string>

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

private:
    void resetKickoff(TeamSide kickingTeam);
    void checkGoals();
    void updatePossession();
    void updateCamera(float dt);
    void processInput(float dt);
    void scoreGoal(TeamSide scoringTeam);
    std::string formatTime() const;

    sf::RenderWindow window_;
    std::unique_ptr<Renderer> renderer_;

    Field field_;
    Ball ball_;
    Team homeTeam_;
    Team awayTeam_;

    MatchState state_;
    MatchState stateBeforePause_;
    TeamSide kickoffTeam_;
    std::string lastScorer_;
    float stateTimer_;
    float matchTime_;
    float animTime_;
    int half_;
    sf::Vector2f cameraCenter_;
    sf::Vector2f cameraTarget_;

    // Input state
    sf::Vector2f moveInput_;
    bool sprint_;
    bool shootPressed_;
    bool passPressed_;
    bool longPassPressed_;
    bool tacklePressed_;
    bool switchPressed_;
};
