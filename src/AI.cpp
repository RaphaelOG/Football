#include "AI.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace Constants;

static float distSq(const sf::Vector2f& a, const sf::Vector2f& b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return dx * dx + dy * dy;
}

static Player* findNearest(const std::vector<Player>& players, const sf::Vector2f& pos, Player* exclude = nullptr) {
    Player* best = nullptr;
    float bestD = 99999.f;
    for (auto& p : players) {
        if (&p == exclude) continue;
        float d = length(p.getPosition() - pos);
        if (d < bestD) { bestD = d; best = const_cast<Player*>(&p); }
    }
    return best;
}

void AI::updateGoalkeeper(Player& gk, const Ball& ball, TeamSide side, bool defendsLeftGoal, float dt) {
    (void)dt;
    (void)side;
    float goalX = defendsLeftGoal ? 0.f : FIELD_LENGTH;
    float goalY = FIELD_WIDTH / 2.f;

    sf::Vector2f target(goalX + (defendsLeftGoal ? 2.f : -2.f), goalY);

    sf::Vector2f ballPos = ball.getPosition();
    float ballDistX = std::abs(ballPos.x - goalX);

    if (ballDistX < 25.f) {
        target.y = ballPos.y;
        target.y = std::max(2.f, std::min(FIELD_WIDTH - 2.f, target.y));
    }

    if (defendsLeftGoal) {
        target.x = std::min(target.x, PENALTY_AREA_LENGTH);
    } else {
        target.x = std::max(target.x, FIELD_LENGTH - PENALTY_AREA_LENGTH);
    }

    gk.setTargetPosition(target);

    // Attempt to grab ball if close
    if (length(ballPos - gk.getPosition()) < POSSESSION_DIST + 0.5f && !ball.getOwner()) {
        // GK will get possession via game logic
    }
}

sf::Vector2f AI::getDefensivePosition(const Player& player, const Ball& ball, bool attacksRight) {
    sf::Vector2f home = player.getHomePosition();
    sf::Vector2f ballPos = ball.getPosition();

    float defendLineX = attacksRight ?
        std::min(ballPos.x - 8.f, FIELD_LENGTH * 0.45f) :
        std::max(ballPos.x + 8.f, FIELD_LENGTH * 0.55f);

    sf::Vector2f target = home;
    target.x = defendLineX * 0.4f + home.x * 0.6f;

    if (player.getRole() == PlayerRole::Defender) {
        target.x = defendLineX * 0.6f + home.x * 0.4f;
    }

    return target;
}

sf::Vector2f AI::getAttackingPosition(const Player& player, const Ball& ball, bool attacksRight) {
    sf::Vector2f home = player.getHomePosition();
    sf::Vector2f ballPos = ball.getPosition();

    float attackDir = attacksRight ? 1.f : -1.f;
    sf::Vector2f target = home;

    switch (player.getRole()) {
        case PlayerRole::Striker:
            target.x = ballPos.x + attackDir * 8.f;
            target.y = home.y + (ballPos.y - FIELD_WIDTH / 2.f) * 0.3f;
            break;
        case PlayerRole::Midfielder:
            target.x = ballPos.x - attackDir * 5.f;
            target.y = home.y + (ballPos.y - home.y) * 0.4f;
            break;
        case PlayerRole::Defender:
            target.x = ballPos.x - attackDir * 18.f;
            break;
        default:
            break;
    }

    target.x = std::max(2.f, std::min(FIELD_LENGTH - 2.f, target.x));
    target.y = std::max(2.f, std::min(FIELD_WIDTH - 2.f, target.y));
    return target;
}

Player* AI::findBestPassTarget(const Player& passer, const Team& team, const Team& opponent) {
    Player* best = nullptr;
    float bestScore = -99999.f;
    sf::Vector2f passDir = passer.getTeam() == TeamSide::Home ? sf::Vector2f(1, 0) : sf::Vector2f(-1, 0);

    for (const auto& tm : team.getPlayers()) {
        if (&tm == &passer || tm.isGoalkeeper()) continue;

        sf::Vector2f toTm = tm.getPosition() - passer.getPosition();
        float dist = length(toTm);
        if (dist < 3.f || dist > 40.f) continue;

        float forward = toTm.x * passDir.x + toTm.y * passDir.y;
        float score = forward * 2.f - dist * 0.3f;

        // Penalize if opponent nearby
        for (const auto& opp : opponent.getPlayers()) {
            if (length(opp.getPosition() - tm.getPosition()) < 4.f) {
                score -= 5.f;
            }
        }

        if (score > bestScore) {
            bestScore = score;
            best = const_cast<Player*>(&tm);
        }
    }
    return best;
}

void AI::updateTeam(Team& team, Team& opponent, Ball& ball, float dt, bool isHumanTeam) {
    (void)dt;
    bool attacksRight = team.attacksRight();
    bool teamHasBall = false;
  Player* ballOwner = ball.getOwner();

    if (ballOwner) {
        teamHasBall = ballOwner->getTeam() == team.getSide();
    }

    for (auto& player : team.getPlayers()) {
        bool isControlled = isHumanTeam && team.getControlledPlayer() == &player;

        if (isControlled) continue;

        if (player.isGoalkeeper()) {
            bool defendsLeft = (team.getSide() == TeamSide::Home) == attacksRight;
            updateGoalkeeper(player, ball, team.getSide(), defendsLeft, dt);
            continue;
        }

        sf::Vector2f target;

        if (ballOwner == &player) {
            // Player has ball - dribble toward goal
            float dir = attacksRight ? 1.f : -1.f;
            float goalX = attacksRight ? FIELD_LENGTH : 0.f;
            sf::Vector2f toGoal(goalX - player.getPosition().x, FIELD_WIDTH / 2.f - player.getPosition().y);

            // AI decision: shoot, pass, or dribble
            float distToGoal = std::abs(player.getPosition().x - goalX);
            Player* nearestOpp = findNearest(opponent.getPlayers(), player.getPosition());

            if (distToGoal < 22.f && distToGoal > 5.f) {
                // Shoot
                player.kickBall(ball, toGoal, SHOOT_POWER * (0.8f + (22.f - distToGoal) / 30.f));
            } else if (nearestOpp && length(nearestOpp->getPosition() - player.getPosition()) < 4.f) {
                // Pass under pressure
                Player* target_ = findBestPassTarget(player, team, opponent);
                if (target_) {
                    sf::Vector2f passDir = target_->getPosition() - player.getPosition();
                    player.kickBall(ball, passDir, PASS_POWER);
                } else {
                    player.kickBall(ball, toGoal, SHOOT_POWER * 0.5f);
                }
            } else {
                // Dribble forward
                target = player.getPosition() + sf::Vector2f(dir * 6.f, 0.f);
                if (nearestOpp) {
                    sf::Vector2f away = player.getPosition() - nearestOpp->getPosition();
                    if (length(away) > 0.1f) {
                        target += normalized(away) * 3.f;
                    }
                }
                player.setTargetPosition(target);
            }
            continue;
        }

        // Chase loose ball if nearest
        if (!ballOwner) {
            Player* nearest = findNearest(team.getPlayers(), ball.getPosition());
            if (nearest == &player) {
                player.setTargetPosition(ball.getPosition());
                continue;
            }
        }

        // Mark opponent or support
        if (!teamHasBall) {
            // Defensive
            Player* dangerous = findNearest(opponent.getPlayers(), sf::Vector2f(
                attacksRight ? FIELD_LENGTH * 0.3f : FIELD_LENGTH * 0.7f,
                FIELD_WIDTH / 2.f
            ));

            if (player.getRole() == PlayerRole::Defender && dangerous && dangerous != opponent.getGoalkeeper()) {
                sf::Vector2f markPos = dangerous->getPosition();
                float goalX = attacksRight ? 0.f : FIELD_LENGTH;
                markPos.x = markPos.x * 0.6f + goalX * 0.4f;
                player.setTargetPosition(markPos);
            } else {
                player.setTargetPosition(getDefensivePosition(player, ball, attacksRight));
            }

            // Tackle attempt
            if (ballOwner && ballOwner->getTeam() != team.getSide()) {
                if (length(player.getPosition() - ballOwner->getPosition()) < TACKLE_RANGE + 1.f) {
                    player.attemptTackle(ball, *ballOwner);
                }
            }
        } else {
            // Attacking support
            player.setTargetPosition(getAttackingPosition(player, ball, attacksRight));

            // Make run if striker and teammate has ball
            if (ballOwner && ballOwner->getTeam() == team.getSide() && ballOwner != &player) {
                if (player.getRole() == PlayerRole::Striker) {
                    float dir = attacksRight ? 1.f : -1.f;
                    sf::Vector2f runPos = player.getPosition();
                    runPos.x += dir * 10.f;
                    runPos.y += (static_cast<float>(rand() % 100) / 100.f - 0.5f) * 8.f;
                    player.setTargetPosition(runPos);
                }
            }
        }
    }
}
