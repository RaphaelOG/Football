#pragma once

#include "Constants.h"
#include <SFML/Graphics/RenderTarget.hpp>

using namespace Constants;

class Field {
public:
    void draw(sf::RenderTarget& target, const sf::Vector2f& cameraCenter) const;
    bool isInBounds(const sf::Vector2f& pos) const;
    bool inLeftNet(const sf::Vector2f& pos, float height) const;
    bool inRightNet(const sf::Vector2f& pos, float height) const;
    sf::Vector2f clampToBounds(const sf::Vector2f& pos) const;
    sf::Vector2f getKickoffPosition(TeamSide kickingTeam) const;
    static sf::Vector2f worldToScreen(const sf::Vector2f& world, const sf::Vector2f& cameraCenter);
    static sf::Vector2f screenToWorld(const sf::Vector2f& screen, const sf::Vector2f& cameraCenter);
};
