#include "Renderer.h"
#include "Game.h"
#include <sstream>
#include <iomanip>

using namespace Constants;

Renderer::Renderer() : fontLoaded_(false) {
    // Try system fonts on macOS
    const char* fontPaths[] = {
        "/System/Library/Fonts/SFNS.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
    };
    for (const char* path : fontPaths) {
        if (font_.loadFromFile(path)) {
            fontLoaded_ = true;
            break;
        }
    }
}

void Renderer::drawField(sf::RenderTarget& target, const Field& field, const sf::Vector2f& camera) {
    field.draw(target, camera);
}

void Renderer::drawHUD(sf::RenderTarget& target, const Game& game) {
    if (!fontLoaded_) return;

    const Team& home = game.getHomeTeam();
    const Team& away = game.getAwayTeam();

    std::ostringstream score;
    score << home.getName() << "  " << home.getScore() << " - " << away.getScore() << "  " << away.getName();

    sf::Text scoreText(score.str(), font_, 28);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setStyle(sf::Text::Bold);
    sf::FloatRect bounds = scoreText.getLocalBounds();
    scoreText.setPosition(WINDOW_WIDTH / 2.f - bounds.width / 2.f - bounds.left, 12.f);

    sf::RectangleShape scoreBg({bounds.width + 40.f, 50.f});
    scoreBg.setFillColor(sf::Color(0, 0, 0, 160));
    scoreBg.setPosition(WINDOW_WIDTH / 2.f - (bounds.width + 40.f) / 2.f, 5.f);
    target.draw(scoreBg);
    target.draw(scoreText);

    // Match time
    std::ostringstream timeStr;
    int gameMinutes = static_cast<int>(game.getMatchTime() / MATCH_DURATION * 90.f);
    timeStr << std::setw(2) << std::setfill('0') << gameMinutes << "'  H" << game.getHalf();

    sf::Text timeText(timeStr.str(), font_, 22);
    timeText.setFillColor(sf::Color(220, 220, 220));
    timeText.setPosition(WINDOW_WIDTH / 2.f - 30.f, 58.f);
    target.draw(timeText);

    // Controls hint
    sf::Text hint("WASD: Move | Shift: Sprint | Space: Shoot | E: Pass | Q: Switch | C: Tackle | P: Pause",
                  font_, 14);
    hint.setFillColor(sf::Color(200, 200, 200, 180));
    hint.setPosition(10.f, WINDOW_HEIGHT - 28.f);
    target.draw(hint);

    // Controlled player name
    if (home.getControlledPlayer()) {
        sf::Text playerText("You: " + home.getControlledPlayer()->getName(), font_, 16);
        playerText.setFillColor(sf::Color::Yellow);
        playerText.setPosition(10.f, 10.f);
        target.draw(playerText);
    }
}

void Renderer::drawOverlay(sf::RenderTarget& target, const std::string& message, const sf::Color& color) {
    sf::RectangleShape overlay({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    overlay.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(overlay);

    if (!fontLoaded_) return;

    sf::Text text(message, font_, 48);
    text.setFillColor(color);
    text.setStyle(sf::Text::Bold);
    sf::FloatRect bounds = text.getLocalBounds();
    text.setPosition(WINDOW_WIDTH / 2.f - bounds.width / 2.f - bounds.left,
                     WINDOW_HEIGHT / 2.f - bounds.height / 2.f - bounds.top);
    target.draw(text);
}
