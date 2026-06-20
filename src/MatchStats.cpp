#include "MatchStats.h"
#include <sstream>

void MatchStats::reset() {
    homeShots_ = awayShots_ = 0;
    homePasses_ = awayPasses_ = 0;
    homeTackles_ = awayTackles_ = 0;
    homeFouls_ = awayFouls_ = 0;
    homePossession_ = awayPossession_ = 0.f;
    lastScorer_.clear();
    eventFeed_.clear();
}

void MatchStats::addEvent(const std::string& message) {
    eventFeed_.push_front(message);
    while (static_cast<int>(eventFeed_.size()) > MAX_EVENTS) {
        eventFeed_.pop_back();
    }
}

void MatchStats::recordShot(TeamSide team) {
    if (team == TeamSide::Home) ++homeShots_;
    else ++awayShots_;
}

void MatchStats::recordPass(TeamSide team) {
    if (team == TeamSide::Home) ++homePasses_;
    else ++awayPasses_;
}

void MatchStats::recordTackle(TeamSide team) {
    if (team == TeamSide::Home) ++homeTackles_;
    else ++awayTackles_;
}

void MatchStats::recordFoul(TeamSide team, const std::string& playerName) {
    if (team == TeamSide::Home) ++homeFouls_;
    else ++awayFouls_;
    addEvent(playerName + " fouls!");
}

void MatchStats::recordGoal(TeamSide team, const std::string& scorer) {
    lastScorer_ = scorer;
    lastGoalTeam_ = team;
    addEvent("GOAL! " + scorer);
}

void MatchStats::updatePossession(TeamSide team, float dt) {
    if (team == TeamSide::Home) homePossession_ += dt;
    else awayPossession_ += dt;
}

float MatchStats::getPossessionPercent(TeamSide team) const {
    float total = homePossession_ + awayPossession_;
    if (total < 0.01f) return 50.f;
    float val = team == TeamSide::Home ? homePossession_ : awayPossession_;
    return val / total * 100.f;
}

int MatchStats::getShots(TeamSide team) const {
    return team == TeamSide::Home ? homeShots_ : awayShots_;
}

int MatchStats::getPasses(TeamSide team) const {
    return team == TeamSide::Home ? homePasses_ : awayPasses_;
}

int MatchStats::getTackles(TeamSide team) const {
    return team == TeamSide::Home ? homeTackles_ : awayTackles_;
}

int MatchStats::getFouls(TeamSide team) const {
    return team == TeamSide::Home ? homeFouls_ : awayFouls_;
}
