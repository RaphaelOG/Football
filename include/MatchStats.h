#pragma once

#include "Constants.h"
#include <deque>
#include <string>

using namespace Constants;

class MatchStats {
public:
    void reset();

    void addEvent(const std::string& message);
    void recordShot(TeamSide team);
    void recordPass(TeamSide team);
    void recordTackle(TeamSide team);
    void recordFoul(TeamSide team, const std::string& playerName);
    void recordGoal(TeamSide team, const std::string& scorer);
    void updatePossession(TeamSide team, float dt);

    float getPossessionPercent(TeamSide team) const;

    int getShots(TeamSide team) const;
    int getPasses(TeamSide team) const;
    int getTackles(TeamSide team) const;
    int getFouls(TeamSide team) const;

    const std::string& getLastScorer() const { return lastScorer_; }
    TeamSide getLastGoalTeam() const { return lastGoalTeam_; }
    const std::deque<std::string>& getEventFeed() const { return eventFeed_; }

private:
    int homeShots_ = 0, awayShots_ = 0;
    int homePasses_ = 0, awayPasses_ = 0;
    int homeTackles_ = 0, awayTackles_ = 0;
    int homeFouls_ = 0, awayFouls_ = 0;
    float homePossession_ = 0.f, awayPossession_ = 0.f;

    std::string lastScorer_;
    TeamSide lastGoalTeam_ = TeamSide::Home;
    std::deque<std::string> eventFeed_;

    static constexpr int MAX_EVENTS = 6;
};
