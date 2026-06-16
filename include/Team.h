#pragma once

#include "Player.h"
#include <vector>
#include <memory>

using namespace Constants;

class Team {
public:
    Team(TeamSide side, const std::string& name);

    void setupFormation();
    void resetPositions(bool attackingRight);
    Player* getControlledPlayer() { return controlledPlayer_; }
    const Player* getControlledPlayer() const { return controlledPlayer_; }
    void setControlledPlayer(Player* player) { controlledPlayer_ = player; }
    void switchToNearestPlayer(const sf::Vector2f& ballPos, bool excludeGK = false);

    std::vector<Player>& getPlayers() { return players_; }
    const std::vector<Player>& getPlayers() const { return players_; }
    Player* getGoalkeeper();
    const Player* getGoalkeeper() const;
    TeamSide getSide() const { return side_; }
    const std::string& getName() const { return teamName_; }
    int getScore() const { return score_; }
    void addGoal() { ++score_; }
    void resetScore() { score_ = 0; }
    bool attacksRight() const { return attacksRight_; }
    void setAttacksRight(bool right) { attacksRight_ = right; }

    sf::Vector2f formationToWorld(const sf::Vector2f& normalized) const;

private:
    TeamSide side_;
    std::string teamName_;
    std::vector<Player> players_;
    Player* controlledPlayer_;
    int score_;
    bool attacksRight_;
};
