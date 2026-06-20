#pragma once

#include "Team.h"
#include "Ball.h"
#include "Field.h"
#include "MatchStats.h"

using namespace Constants;

class AI {
public:
    static void updateTeam(Team& team, Team& opponent, Ball& ball, float dt, bool isHumanTeam, MatchStats& stats);
    static void updateGoalkeeper(Player& gk, const Ball& ball, TeamSide side, bool defendsLeftGoal, float dt);
    static Player* findBestPassTarget(const Player& passer, const Team& team, const Team& opponent,
                                      const Player* preferTarget = nullptr, bool allowBackward = false);
    static sf::Vector2f getDefensivePosition(const Player& player, const Ball& ball, bool attacksRight);
    static sf::Vector2f getAttackingPosition(const Player& player, const Ball& ball, bool attacksRight);
};
