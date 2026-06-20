#include "Field.h"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <cmath>
#include <algorithm>

using namespace Constants;

void Field::draw(sf::RenderTarget& target, const sf::Vector2f& cameraCenter) const {
    float offsetX = WINDOW_WIDTH / 2.0f - cameraCenter.x * PIXELS_PER_METER;
    float offsetY = WINDOW_HEIGHT / 2.0f - cameraCenter.y * PIXELS_PER_METER;

    auto toScreen = [&](float x, float y) -> sf::Vector2f {
        return {x * PIXELS_PER_METER + offsetX, y * PIXELS_PER_METER + offsetY};
    };

    float fw = FIELD_LENGTH * PIXELS_PER_METER;
    float fh = FIELD_WIDTH * PIXELS_PER_METER;
    sf::Vector2f origin = toScreen(0, 0);

    // Surround / runoff
    sf::RectangleShape surround({fw + 48.f, fh + 48.f});
    surround.setPosition(origin.x - 24.f, origin.y - 24.f);
    surround.setFillColor(sf::Color(18, 72, 28));
    target.draw(surround);

    // Grass base
    sf::RectangleShape grass({fw, fh});
    grass.setPosition(origin);
    grass.setFillColor(sf::Color(46, 125, 50));
    target.draw(grass);

    // Mowing stripes
    for (int i = 0; i < 12; ++i) {
        sf::RectangleShape stripe({fw / 12.f, fh});
        stripe.setPosition(origin.x + fw / 12.f * i, origin.y);
        stripe.setFillColor(i % 2 == 0 ? sf::Color(42, 118, 46, 120) : sf::Color(50, 132, 54, 90));
        target.draw(stripe);
    }

    auto drawLine = [&](float x1, float y1, float x2, float y2, float thickness = 2.5f, sf::Color col = lineColor()) {
        sf::Vector2f p1 = toScreen(x1, y1);
        sf::Vector2f p2 = toScreen(x2, y2);
        sf::Vector2f diff = p2 - p1;
        float len = std::sqrt(diff.x * diff.x + diff.y * diff.y);
        sf::RectangleShape line({len, thickness});
        line.setFillColor(col);
        float angle = std::atan2(diff.y, diff.x) * 180.f / 3.14159265f;
        line.setOrigin(0, thickness / 2.f);
        line.setPosition(p1);
        line.setRotation(angle);
        target.draw(line);
    };

    drawLine(0, 0, FIELD_LENGTH, 0, 3.f);
    drawLine(FIELD_LENGTH, 0, FIELD_LENGTH, FIELD_WIDTH, 3.f);
    drawLine(FIELD_LENGTH, FIELD_WIDTH, 0, FIELD_WIDTH, 3.f);
    drawLine(0, FIELD_WIDTH, 0, 0, 3.f);
    drawLine(FIELD_LENGTH / 2.f, 0, FIELD_LENGTH / 2.f, FIELD_WIDTH, 2.5f);

    sf::CircleShape centerCircle(CENTER_CIRCLE_RADIUS * PIXELS_PER_METER);
    centerCircle.setOrigin(CENTER_CIRCLE_RADIUS * PIXELS_PER_METER, CENTER_CIRCLE_RADIUS * PIXELS_PER_METER);
    centerCircle.setPosition(toScreen(FIELD_LENGTH / 2.f, FIELD_WIDTH / 2.f));
    centerCircle.setFillColor(sf::Color::Transparent);
    centerCircle.setOutlineThickness(2.5f);
    centerCircle.setOutlineColor(lineColor());
    target.draw(centerCircle);

    sf::CircleShape spot(4.f);
    spot.setOrigin(4.f, 4.f);
    spot.setPosition(toScreen(FIELD_LENGTH / 2.f, FIELD_WIDTH / 2.f));
    spot.setFillColor(lineColor());
    target.draw(spot);

    auto drawPenaltyArea = [&](float side) {
        float x = side < 0 ? 0.f : FIELD_LENGTH - PENALTY_AREA_LENGTH;
        float y = (FIELD_WIDTH - PENALTY_AREA_WIDTH) / 2.f;
        drawLine(x, y, x + PENALTY_AREA_LENGTH, y);
        drawLine(x + PENALTY_AREA_LENGTH, y, x + PENALTY_AREA_LENGTH, y + PENALTY_AREA_WIDTH);
        drawLine(x + PENALTY_AREA_LENGTH, y + PENALTY_AREA_WIDTH, x, y + PENALTY_AREA_WIDTH);
        drawLine(x, y + PENALTY_AREA_WIDTH, x, y);

        float gx = side < 0 ? 0.f : FIELD_LENGTH - GOAL_AREA_LENGTH;
        float gy = (FIELD_WIDTH - GOAL_AREA_WIDTH) / 2.f;
        drawLine(gx, gy, gx + GOAL_AREA_LENGTH, gy);
        drawLine(gx + GOAL_AREA_LENGTH, gy, gx + GOAL_AREA_LENGTH, gy + GOAL_AREA_WIDTH);
        drawLine(gx + GOAL_AREA_LENGTH, gy + GOAL_AREA_WIDTH, gx, gy + GOAL_AREA_WIDTH);
        drawLine(gx, gy + GOAL_AREA_WIDTH, gx, gy);

        float spotX = side < 0 ? PENALTY_SPOT_DIST : FIELD_LENGTH - PENALTY_SPOT_DIST;
        sf::CircleShape penSpot(4.f);
        penSpot.setOrigin(4.f, 4.f);
        penSpot.setPosition(toScreen(spotX, FIELD_WIDTH / 2.f));
        penSpot.setFillColor(lineColor());
        target.draw(penSpot);

        float goalY = (FIELD_WIDTH - GOAL_WIDTH) / 2.f;
        sf::Vector2f gPos = toScreen(side < 0 ? -GOAL_DEPTH : FIELD_LENGTH, goalY);
        sf::RectangleShape goal({GOAL_DEPTH * PIXELS_PER_METER, GOAL_WIDTH * PIXELS_PER_METER});
        goal.setPosition(gPos);
        goal.setFillColor(sf::Color(240, 240, 240, 50));
        goal.setOutlineThickness(3.f);
        goal.setOutlineColor(sf::Color::White);
        target.draw(goal);

        for (int i = 0; i < 5; ++i) {
            float ny = goalY + GOAL_WIDTH * (i + 0.5f) / 5.f;
            drawLine(side < 0 ? -GOAL_DEPTH : FIELD_LENGTH, ny,
                     side < 0 ? 0.f : FIELD_LENGTH, ny, 1.f, sf::Color(255, 255, 255, 100));
        }
    };

    drawPenaltyArea(-1);
    drawPenaltyArea(1);

    auto drawFlag = [&](float x, float y) {
        sf::Vector2f base = toScreen(x, y);
        sf::RectangleShape pole({2.f, 18.f});
        pole.setFillColor(sf::Color(220, 220, 220));
        pole.setPosition(base.x - 1.f, base.y - 18.f);
        target.draw(pole);
        sf::CircleShape flag(5.f, 3);
        flag.setFillColor(sf::Color(255, 220, 0));
        flag.setPosition(base.x + 1.f, base.y - 18.f);
        target.draw(flag);
    };
    drawFlag(0.5f, 0.5f);
    drawFlag(FIELD_LENGTH - 0.5f, 0.5f);
    drawFlag(0.5f, FIELD_WIDTH - 0.5f);
    drawFlag(FIELD_LENGTH - 0.5f, FIELD_WIDTH - 0.5f);
}

bool Field::isInBounds(const sf::Vector2f& pos) const {
    return pos.x >= 0 && pos.x <= FIELD_LENGTH && pos.y >= 0 && pos.y <= FIELD_WIDTH;
}

bool Field::isGoal(const sf::Vector2f& pos, TeamSide scoringTeam) const {
    float goalTop = (FIELD_WIDTH - GOAL_WIDTH) / 2.f;
    float goalBottom = goalTop + GOAL_WIDTH;
    if (pos.y < goalTop || pos.y > goalBottom) return false;

    if (scoringTeam == TeamSide::Home) {
        return pos.x >= FIELD_LENGTH - 0.5f;
    }
    return pos.x <= 0.5f;
}

sf::Vector2f Field::clampToBounds(const sf::Vector2f& pos) const {
    return {
        std::max(0.f, std::min(FIELD_LENGTH, pos.x)),
        std::max(0.f, std::min(FIELD_WIDTH, pos.y))
    };
}

sf::Vector2f Field::getKickoffPosition(TeamSide kickingTeam) const {
    float y = FIELD_WIDTH / 2.f;
    if (kickingTeam == TeamSide::Home) {
        return {FIELD_LENGTH / 2.f - 0.5f, y};
    }
    return {FIELD_LENGTH / 2.f + 0.5f, y};
}

SetPieceInfo Field::checkOutOfBounds(const sf::Vector2f& pos, TeamSide lastTouch) const {
    SetPieceInfo info;
    float margin = 0.5f;

    float goalTop = (FIELD_WIDTH - GOAL_WIDTH) / 2.f;
    float goalBottom = goalTop + GOAL_WIDTH;
    bool inGoalMouth = pos.y >= goalTop - 0.5f && pos.y <= goalBottom + 0.5f;

    if (pos.x < -margin && !inGoalMouth) {
        if (lastTouch == TeamSide::Away) {
            info.type = OutOfBoundsType::GoalKick;
            info.takingTeam = TeamSide::Home;
            info.position = {FIELD_INSET, FIELD_WIDTH / 2.f};
        } else {
            info.type = OutOfBoundsType::CornerKick;
            info.takingTeam = TeamSide::Away;
            info.position = {FIELD_INSET, pos.y < FIELD_WIDTH / 2.f ? FIELD_INSET : FIELD_WIDTH - FIELD_INSET};
        }
        return info;
    }

    if (pos.x > FIELD_LENGTH + margin && !inGoalMouth) {
        if (lastTouch == TeamSide::Home) {
            info.type = OutOfBoundsType::GoalKick;
            info.takingTeam = TeamSide::Away;
            info.position = {FIELD_LENGTH - FIELD_INSET, FIELD_WIDTH / 2.f};
        } else {
            info.type = OutOfBoundsType::CornerKick;
            info.takingTeam = TeamSide::Home;
            info.position = {FIELD_LENGTH - FIELD_INSET, pos.y < FIELD_WIDTH / 2.f ? FIELD_INSET : FIELD_WIDTH - FIELD_INSET};
        }
        return info;
    }

    return info;
}

sf::Vector2f Field::getSetPiecePosition(const SetPieceInfo& info) const {
    return info.position;
}

sf::Vector2f Field::worldToScreen(const sf::Vector2f& world, const sf::Vector2f& cameraCenter) {
    float offsetX = WINDOW_WIDTH / 2.0f - cameraCenter.x * PIXELS_PER_METER;
    float offsetY = WINDOW_HEIGHT / 2.0f - cameraCenter.y * PIXELS_PER_METER;
    return {world.x * PIXELS_PER_METER + offsetX, world.y * PIXELS_PER_METER + offsetY};
}

sf::Vector2f Field::screenToWorld(const sf::Vector2f& screen, const sf::Vector2f& cameraCenter) {
    float offsetX = WINDOW_WIDTH / 2.0f - cameraCenter.x * PIXELS_PER_METER;
    float offsetY = WINDOW_HEIGHT / 2.0f - cameraCenter.y * PIXELS_PER_METER;
    return {(screen.x - offsetX) / PIXELS_PER_METER, (screen.y - offsetY) / PIXELS_PER_METER};
}
