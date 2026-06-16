#pragma once

#include "Field.h"
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <string>

class Game;

class Renderer {
public:
    Renderer();

    void drawField(sf::RenderTarget& target, const Field& field, const sf::Vector2f& camera);
    void drawHUD(sf::RenderTarget& target, const Game& game);
    void drawOverlay(sf::RenderTarget& target, const std::string& message, const sf::Color& color = sf::Color::White);

private:
    sf::Font font_;
    bool fontLoaded_;
};
