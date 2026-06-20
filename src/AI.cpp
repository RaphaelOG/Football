#include "AI.h"
#include "MatchStats.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace Constants;

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

static int countNearbyOpponents(const sf::Vector2f& pos, const Team& opponent, float range) {
    int count = 0;
    for (const auto& opp : opponent.getPlayers()) {
        if (length(opp.getPosition() - pos) < range) ++count;
    }
    return count;
}

void AI::updateGoalkeeper(Player& gk, const Ball& ball, TeamSide side, bool defendsLeftGoal, float dt) {
    (void)dt;
    (void)side;
    float goalX = defendsLeftGoal ? 0.f : FIELD_LENGTH;
    float goalY = FIELD_WIDTH / 2.f;

    sf::Vector2f target(goalX + (defendsLeftGoal ? 2.5f : -2.5f), goalY);
    sf::Vector2f ballPos = ball.getPosition();
    float ballDistX = std::abs(ballPos.x - goalX);

    if (ballDistX < 30.f) {
        target.y = ballPos.y;
        target.y = std::max(3.f, std::min(FIELD_WIDTH - 3.f, target.y));
    }

    if (defendsLeftGoal) {
        target.x = std::min(target.x, PENALTY_AREA_LENGTH + 1.f);
    } else {
        target.x = std::max(target.x, FIELD_LENGTH - PENALTY_AREA_LENGTH - 1.f);
    }

    gk.setTargetPosition(target);
}

sf::Vector2f AI::getDefensivePosition(const Player& player, const Ball& ball, bool attacksRight) {
    sf::Vector2f home = player.getHomePosition();
    sf::Vector2f ballPos = ball.getPosition();
    float ownGoalX = attacksRight ? 0.f : FIELD_LENGTH;

    float urgency = std::max(0.f, 1.f - std::abs(ballPos.x - ownGoalX) / (FIELD_LENGTH * 0.5f));
    float shiftTowardBall = 0.35f + urgency * 0.35f;

    sf::Vector2f target = home;
    target.x = home.x * (1.f - shiftTowardBall) + (ballPos.x - (attacksRight ? 6.f : -6.f)) * shiftTowardBall;
    target.y = home.y * 0.6f + ballPos.y * 0.4f;

    if (player.getRole() == PlayerRole::Defender) {
        target.x = home.x * 0.35f + (ballPos.x - (attacksRight ? 10.f : -10.f)) * 0.65f;
    }

    target.x = std::max(3.f, std::min(FIELD_LENGTH - 3.f, target.x));
    target.y = std::max(3.f, std::min(FIELD_WIDTH - 3.f, target.y));
    return target;
}

sf::Vector2f AI::getAttackingPosition(const Player& player, const Ball& ball, bool attacksRight) {
    sf::Vector2f home = player.getHomePosition();
    sf::Vector2f ballPos = ball.getPosition();
    float attackDir = attacksRight ? 1.f : -1.f;
    sf::Vector2f target = home;

    switch (player.getRole()) {
        case PlayerRole::Striker:
            target.x = ballPos.x + attackDir * 12.f;
            target.y = home.y + (ballPos.y - FIELD_WIDTH / 2.f) * 0.35f;
            break;
        case PlayerRole::Midfielder:
            target.x = ballPos.x + attackDir * 4.f;
            target.y = home.y + (ballPos.y - home.y) * 0.5f;
            break;
        case PlayerRole::Defender:
            target.x = ballPos.x - attackDir * 14.f;
            break;
        default:
            break;
    }

    target.x = std::max(3.f, std::min(FIELD_LENGTH - 3.f, target.x));
    target.y = std::max(3.f, std::min(FIELD_WIDTH - 3.f, target.y));
    return target;
}

Player* AI::findBestPassTarget(const Player& passer, const Team& team, const Team& opponent,
                               const Player* preferTarget, bool allowBackward) {
    Player* best = nullptr;
    float bestScore = -99999.f;
    sf::Vector2f passDir = passer.getTeam() == TeamSide::Home ? sf::Vector2f(1, 0) : sf::Vector2f(-1, 0);
    const Player* human = team.getControlledPlayer();

    for (const auto& tm : team.getPlayers()) {
        if (&tm == &passer || tm.isGoalkeeper()) continue;

        sf::Vector2f toTm = tm.getPosition() - passer.getPosition();
        float dist = length(toTm);
        if (dist < 2.f || dist > 45.f) continue;

        float forward = toTm.x * passDir.x + toTm.y * passDir.y;
        if (!allowBackward && forward < -2.f) continue;

        float score = forward * 1.5f - dist * 0.2f;

        int nearbyOpps = countNearbyOpponents(tm.getPosition(), opponent, 4.f);
        score -= nearbyOpps * 4.f;

        if (preferTarget && &tm == preferTarget) score += 25.f;
        if (human && &tm == human && &tm != &passer) {
            score += 18.f;
            if (nearbyOpps == 0) score += 10.f;
        }

        if (tm.getRole() == PlayerRole::Striker && forward > 0.f) score += 3.f;

        if (score > bestScore) {
            bestScore = score;
            best = const_cast<Player*>(&tm);
        }
    }
    return best;
}

void AI::updateTeam(Team& team, Team& opponent, Ball& ball, float dt, bool isHumanTeam, MatchStats& stats) {
    (void)dt;
    bool attacksRight = team.attacksRight();
    bool teamHasBall = false;
    Player* ballOwner = ball.getOwner();

    if (ballOwner) {
        teamHasBall = ballOwner->getTeam() == team.getSide();
    }

    sf::Vector2f ballPos = ball.getPosition();
    float ownGoalX = attacksRight ? 0.f : FIELD_LENGTH;

    for (auto& player : team.getPlayers()) {
        bool isControlled = isHumanTeam && team.getControlledPlayer() == &player;
        if (isControlled) continue;

        if (player.isGoalkeeper()) {
            bool defendsLeft = (team.getSide() == TeamSide::Home) == attacksRight;
            updateGoalkeeper(player, ball, team.getSide(), defendsLeft, dt);
            continue;
        }

        if (ballOwner == &player) {
            float dir = attacksRight ? 1.f : -1.f;
            float goalX = attacksRight ? FIELD_LENGTH : 0.f;
            sf::Vector2f toGoal(goalX - player.getPosition().x, FIELD_WIDTH / 2.f - player.getPosition().y);
            float distToGoal = std::abs(player.getPosition().x - goalX);
            Player* nearestOpp = findNearest(opponent.getPlayers(), player.getPosition());
            float pressure = nearestOpp ? length(nearestOpp->getPosition() - player.getPosition()) : 99.f;

            const Player* passToHuman = isHumanTeam ? team.getControlledPlayer() : nullptr;
            bool humanOpen = passToHuman && passToHuman != &player &&
                             countNearbyOpponents(passToHuman->getPosition(), opponent, 5.f) == 0;

            if (distToGoal < 24.f && distToGoal > 6.f && pressure > 3.f) {
                player.kickBall(ball, toGoal, SHOOT_POWER * (0.85f + (24.f - distToGoal) / 35.f), KickType::Shoot);
                stats.recordShot(team.getSide());
            } else if (pressure < 5.f || humanOpen) {
                Player* target = findBestPassTarget(player, team, opponent, passToHuman, humanOpen);
                if (target) {
                    sf::Vector2f passDir = target->getPosition() - player.getPosition();
                    float power = humanOpen && target == passToHuman ? PASS_POWER * 1.1f : PASS_POWER;
                    player.kickBall(ball, passDir, power, KickType::Pass);
                    stats.recordPass(team.getSide());
                } else if (distToGoal < 30.f) {
                    player.kickBall(ball, toGoal, SHOOT_POWER * 0.6f, KickType::Shoot);
                    stats.recordShot(team.getSide());
                } else {
                    player.setTargetPosition(player.getPosition() + sf::Vector2f(dir * 5.f, 0.f));
                }
            } else {
                sf::Vector2f dribble = player.getPosition() + sf::Vector2f(dir * 5.f, 0.f);
                if (nearestOpp) {
                    sf::Vector2f away = player.getPosition() - nearestOpp->getPosition();
                    if (length(away) > 0.1f) dribble += normalized(away) * 2.5f;
                }
                player.setTargetPosition(dribble);
            }
            continue;
        }

        if (!teamHasBall) {
            Player* carrier = ballOwner && ballOwner->getTeam() != team.getSide() ? ballOwner : nullptr;

            if (carrier) {
                float distToCarrier = length(player.getPosition() - carrier->getPosition());
                bool shouldPress = distToCarrier < 22.f &&
                    (player.getRole() != PlayerRole::Striker || distToCarrier < 14.f);

                if (shouldPress && distToCarrier < 16.f) {
                    sf::Vector2f pressPos = carrier->getPosition();
                    float goalSide = attacksRight ? -1.f : 1.f;
                    pressPos.x += goalSide * 1.5f;
                    player.setTargetPosition(pressPos);

                    if (distToCarrier < TACKLE_RANGE + 1.5f) {
                        player.attemptTackle(ball, *carrier);
                        stats.recordTackle(team.getSide());
                    }
                    continue;
                }

                if (player.getRole() == PlayerRole::Defender && distToCarrier < 35.f) {
                    sf::Vector2f markPos = carrier->getPosition();
                    markPos.x = markPos.x * 0.55f + ownGoalX * 0.45f;
                    markPos.y = markPos.y * 0.7f + player.getHomePosition().y * 0.3f;
                    player.setTargetPosition(markPos);
                    continue;
                }
            }

            if (!ballOwner) {
                Player* chaser = findNearest(team.getPlayers(), ballPos);
                if (chaser == &player) {
                    player.setTargetPosition(ballPos);
                    continue;
                }
            }

            player.setTargetPosition(getDefensivePosition(player, ball, attacksRight));
        } else {
            player.setTargetPosition(getAttackingPosition(player, ball, attacksRight));

            if (ballOwner && ballOwner->getTeam() == team.getSide() && ballOwner != &player) {
                if (player.getRole() == PlayerRole::Striker) {
                    float dir = attacksRight ? 1.f : -1.f;
                    sf::Vector2f runPos = player.getPosition();
                    runPos.x += dir * 12.f;
                    runPos.y += (static_cast<float>(rand() % 100) / 100.f - 0.5f) * 10.f;
                    player.setTargetPosition(runPos);
                }
            }
        }
    }
}
