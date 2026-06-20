#include "Team.h"

using namespace Constants;

static const FormationSlot FORMATION_442[] = {
    {Position::GK,  {0.05f, 0.50f}, PlayerRole::Goalkeeper},
    {Position::LB,  {0.22f, 0.15f}, PlayerRole::Defender},
    {Position::CB,  {0.18f, 0.38f}, PlayerRole::Defender},
    {Position::CB,  {0.18f, 0.62f}, PlayerRole::Defender},
    {Position::RB,  {0.22f, 0.85f}, PlayerRole::Defender},
    {Position::LM,  {0.40f, 0.15f}, PlayerRole::Midfielder},
    {Position::CM,  {0.38f, 0.38f}, PlayerRole::Midfielder},
    {Position::CM,  {0.38f, 0.62f}, PlayerRole::Midfielder},
    {Position::RM,  {0.40f, 0.85f}, PlayerRole::Midfielder},
    {Position::ST,  {0.58f, 0.38f}, PlayerRole::Striker},
    {Position::ST,  {0.58f, 0.62f}, PlayerRole::Striker},
};

static const char* HOME_NAMES[] = {
    "Martinez", "Walker", "Stones", "Dias", "Robertson",
    "Saka", "Rice", "Odegaard", "Foden", "Haaland", "Salah"
};

static const char* AWAY_NAMES[] = {
    "Alisson", "Carvajal", "Ramos", "Van Dijk", "Cancelo",
    "Modric", "Kroos", "De Bruyne", "Vinicius", "Benzema", "Mbappe"
};

Team::Team(TeamSide side, const std::string& name)
    : side_(side), teamName_(name), controlledPlayer_(nullptr), score_(0), attacksRight_(true) {
    setupFormation();
}

void Team::setupFormation() {
    players_.clear();
    const char** names = side_ == TeamSide::Home ? HOME_NAMES : AWAY_NAMES;

    for (int i = 0; i < PLAYERS_PER_TEAM; ++i) {
        players_.emplace_back(i, side_, FORMATION_442[i].position, names[i]);
    }

    for (auto& p : players_) {
        if (p.getPositionRole() == Position::CM) {
            controlledPlayer_ = &p;
            break;
        }
    }
    if (!controlledPlayer_) controlledPlayer_ = &players_[7];
}

void Team::resetPositions(bool attackingRight) {
    attacksRight_ = attackingRight;
    for (int i = 0; i < PLAYERS_PER_TEAM; ++i) {
        sf::Vector2f worldPos = formationToWorld(FORMATION_442[i].normalizedPos);
        players_[i].reset(worldPos);
    }
}

sf::Vector2f Team::formationToWorld(const sf::Vector2f& normalized) const {
    float x, y;
    if (attacksRight_) {
        x = normalized.x * FIELD_LENGTH;
        y = normalized.y * FIELD_WIDTH;
    } else {
        x = FIELD_LENGTH - normalized.x * FIELD_LENGTH;
        y = FIELD_WIDTH - normalized.y * FIELD_WIDTH;
    }
    return {x, y};
}

void Team::switchToNearestPlayer(const sf::Vector2f& ballPos, bool excludeGK) {
    Player* best = nullptr;
    float bestDist = 99999.f;

    for (auto& p : players_) {
        if (excludeGK && p.isGoalkeeper()) continue;
        if (&p == controlledPlayer_) continue;
        float d = length(p.getPosition() - ballPos);
        if (d < bestDist) {
            bestDist = d;
            best = &p;
        }
    }
    if (best) controlledPlayer_ = best;
}

void Team::switchToNextOutfieldPlayer() {
    if (players_.empty()) return;

    int startIdx = 0;
    for (int i = 0; i < PLAYERS_PER_TEAM; ++i) {
        if (&players_[i] == controlledPlayer_) {
            startIdx = i;
            break;
        }
    }

    for (int step = 1; step < PLAYERS_PER_TEAM; ++step) {
        int idx = (startIdx + step) % PLAYERS_PER_TEAM;
        if (!players_[idx].isGoalkeeper()) {
            controlledPlayer_ = &players_[idx];
            return;
        }
    }
}

void Team::switchToBallHolder() {
    for (auto& p : players_) {
        if (!p.isGoalkeeper()) {
            controlledPlayer_ = &p;
            return;
        }
    }
}

Player* Team::getGoalkeeper() {
    for (auto& p : players_) {
        if (p.isGoalkeeper()) return &p;
    }
    return &players_[0];
}

const Player* Team::getGoalkeeper() const {
    for (const auto& p : players_) {
        if (p.isGoalkeeper()) return &p;
    }
    return &players_[0];
}
