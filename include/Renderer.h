#pragma once

#include "Field.h"
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <string>

class Game;
class Team;

class Renderer {
public:
    Renderer();

    void drawField(sf::RenderTarget& target, const Field& field, const sf::Vector2f& camera);
    void drawHUD(sf::RenderTarget& target, const Game& game);
    void drawOverlay(sf::RenderTarget& target, const std::string& message, const sf::Color& color = sf::Color::White);
    void drawMainMenu(sf::RenderTarget& target);
    void drawMatchSummary(sf::RenderTarget& target, const Game& game);
    void drawPlayerLabels(sf::RenderTarget& target, const Team& home, const Team& away, const sf::Vector2f& camera);

    bool hasFont() const { return fontLoaded_; }

private:
    void drawControlsPanel(sf::RenderTarget& target);
    void drawBar(sf::RenderTarget& target, float x, float y, float w, float h,
                 float fill, const sf::Color& fillColor, const sf::Color& bgColor);
    void drawText(sf::RenderTarget& target, const std::string& text, float x, float y,
                  unsigned size, const sf::Color& color, bool bold = false);

    sf::Font font_;
    bool fontLoaded_;
};
