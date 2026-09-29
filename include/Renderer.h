#pragma once

#include "Field.h"
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <string>
#include <vector>

class Game;
class Team;
class Ball;
class Player;

struct PlayerLabel {
    sf::Vector2f screen;
    std::string text;
    sf::Color color;
};

class Renderer {
public:
    Renderer();

    void drawScene(sf::RenderWindow& window, const Team& home, const Team& away, const Ball& ball,
                   const Player* controlled, const sf::Vector2f& focus, float animTime, float dt);
    void drawLabels(sf::RenderTarget& target);
    void drawHUD(sf::RenderTarget& target, const Game& game);
    void drawBanner(sf::RenderTarget& target, const std::string& message, const sf::Color& color = sf::Color::White);

private:
    void beginFrame(sf::RenderWindow& window, const sf::Vector2f& focus, const sf::Vector2f& velocity, float dt);
    void drawWorld(const Team& home, const Team& away, const Ball& ball, const Player* controlled, float animTime);
    void drawPitch();
    void drawGoals();
    void drawStadium();
    void drawPlayer(const Player& player, bool controlled, float animTime);
    void drawBall(const Ball& ball, float animTime);
    void rememberLabel(const sf::Vector2f& world, float height, const std::string& text, const sf::Color& color);

    sf::Font font_;
    bool fontLoaded_;
    bool cameraReady_;
    float camX_, camY_, camZ_;
    float lookX_, lookY_, lookZ_;
    float ballSpin_;
    std::vector<PlayerLabel> labels_;
};
