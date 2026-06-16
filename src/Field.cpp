#include "Field.h"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <cmath>

using namespace Constants;

void Field::draw(sf::RenderTarget& target, const sf::Vector2f& cameraCenter) const {
    float offsetX = WINDOW_WIDTH / 2.0f - cameraCenter.x * PIXELS_PER_METER;
    float offsetY = WINDOW_HEIGHT / 2.0f - cameraCenter.y * PIXELS_PER_METER;

    auto toScreen = [&](float x, float y) -> sf::Vector2f {
        return {x * PIXELS_PER_METER + offsetX, y * PIXELS_PER_METER + offsetY};
    };

    // Grass background
    sf::RectangleShape grass({FIELD_LENGTH * PIXELS_PER_METER, FIELD_WIDTH * PIXELS_PER_METER});
    grass.setPosition(toScreen(0, 0));
    grass.setFillColor(fieldColor());
    target.draw(grass);

    // Stripes
    for (int i = 0; i < 14; ++i) {
        sf::RectangleShape stripe({FIELD_LENGTH * PIXELS_PER_METER / 14.0f, FIELD_WIDTH * PIXELS_PER_METER});
        stripe.setPosition(toScreen(FIELD_LENGTH / 14.0f * i, 0));
        stripe.setFillColor(i % 2 == 0 ? sf::Color(30, 130, 30) : sf::Color(25, 115, 25));
        target.draw(stripe);
    }

    auto drawLine = [&](float x1, float y1, float x2, float y2, float thickness = 2.f) {
        sf::Vector2f p1 = toScreen(x1, y1);
        sf::Vector2f p2 = toScreen(x2, y2);
        sf::Vector2f diff = p2 - p1;
        float len = std::sqrt(diff.x * diff.x + diff.y * diff.y);
        sf::RectangleShape line({len, thickness});
        line.setFillColor(lineColor());
        float angle = std::atan2(diff.y, diff.x) * 180.f / 3.14159265f;
        line.setOrigin(0, thickness / 2.f);
        line.setPosition(p1);
        line.setRotation(angle);
        target.draw(line);
    };

    // Boundary
    drawLine(0, 0, FIELD_LENGTH, 0);
    drawLine(FIELD_LENGTH, 0, FIELD_LENGTH, FIELD_WIDTH);
    drawLine(FIELD_LENGTH, FIELD_WIDTH, 0, FIELD_WIDTH);
    drawLine(0, FIELD_WIDTH, 0, 0);

    // Halfway line
    drawLine(FIELD_LENGTH / 2.f, 0, FIELD_LENGTH / 2.f, FIELD_WIDTH);

    // Center circle
    sf::CircleShape centerCircle(CENTER_CIRCLE_RADIUS * PIXELS_PER_METER);
    centerCircle.setOrigin(CENTER_CIRCLE_RADIUS * PIXELS_PER_METER, CENTER_CIRCLE_RADIUS * PIXELS_PER_METER);
    centerCircle.setPosition(toScreen(FIELD_LENGTH / 2.f, FIELD_WIDTH / 2.f));
    centerCircle.setFillColor(sf::Color::Transparent);
    centerCircle.setOutlineThickness(2.f);
    centerCircle.setOutlineColor(lineColor());
    target.draw(centerCircle);

    // Center spot
    sf::CircleShape spot(3.f);
    spot.setOrigin(3.f, 3.f);
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
        sf::CircleShape penSpot(3.f);
        penSpot.setOrigin(3.f, 3.f);
        penSpot.setPosition(toScreen(spotX, FIELD_WIDTH / 2.f));
        penSpot.setFillColor(lineColor());
        target.draw(penSpot);

        // Goal
        float goalY = (FIELD_WIDTH - GOAL_WIDTH) / 2.f;
        sf::RectangleShape goal({GOAL_DEPTH * PIXELS_PER_METER, GOAL_WIDTH * PIXELS_PER_METER});
        if (side < 0) {
            goal.setPosition(toScreen(-GOAL_DEPTH, goalY));
        } else {
            goal.setPosition(toScreen(FIELD_LENGTH, goalY));
        }
        goal.setFillColor(sf::Color(200, 200, 200, 80));
        goal.setOutlineThickness(2.f);
        goal.setOutlineColor(sf::Color::White);
        target.draw(goal);
    };

    drawPenaltyArea(-1);
    drawPenaltyArea(1);
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
