#include "Renderer.h"
#include "Game.h"
#include "Team.h"
#include "Ball.h"
#include "Field.h"
#include <sstream>
#include <iomanip>

using namespace Constants;

Renderer::Renderer() : fontLoaded_(false) {
    const char* fontPaths[] = {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/calibri.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
    };
    for (const char* path : fontPaths) {
        if (font_.loadFromFile(path)) {
            fontLoaded_ = true;
            break;
        }
    }
}

void Renderer::drawText(sf::RenderTarget& target, const std::string& text, float x, float y,
                        unsigned size, const sf::Color& color, bool bold) {
    if (!fontLoaded_) return;
    sf::Text t(text, font_, size);
    t.setFillColor(color);
    if (bold) t.setStyle(sf::Text::Bold);
    t.setPosition(x, y);
    target.draw(t);
}

void Renderer::drawBar(sf::RenderTarget& target, float x, float y, float w, float h,
                       float fill, const sf::Color& fillColor, const sf::Color& bgColor) {
    sf::RectangleShape bg({w, h});
    bg.setFillColor(bgColor);
    bg.setPosition(x, y);
    target.draw(bg);

    fill = std::max(0.f, std::min(1.f, fill));
    sf::RectangleShape bar({w * fill, h});
    bar.setFillColor(fillColor);
    bar.setPosition(x, y);
    target.draw(bar);
}

void Renderer::drawField(sf::RenderTarget& target, const Field& field, const sf::Vector2f& camera) {
    field.draw(target, camera);
}

void Renderer::drawPlayerLabels(sf::RenderTarget& target, const Team& home, const Team& away,
                                const sf::Vector2f& camera) {
    if (!fontLoaded_) return;

    auto drawTeam = [&](const Team& team, bool isHome) {
        for (const auto& p : team.getPlayers()) {
            sf::Vector2f screen = Field::worldToScreen(p.getPosition(), camera);
            float px = (p.isGoalkeeper() ? GK_RADIUS : PLAYER_RADIUS) * PIXELS_PER_METER;

            std::string num = std::to_string(p.getJerseyNumber());
            sf::Text label(num, font_, 10);
            label.setFillColor(sf::Color(255, 255, 255, 230));
            label.setStyle(sf::Text::Bold);
            sf::FloatRect b = label.getLocalBounds();
            label.setPosition(screen.x - b.width / 2.f - b.left, screen.y - px * 0.15f - b.height / 2.f);
            target.draw(label);

            if (isHome && team.getControlledPlayer() == &p) {
                sf::Text tag("YOU", font_, 9);
                tag.setFillColor(sf::Color(255, 230, 50));
                tag.setStyle(sf::Text::Bold);
                sf::FloatRect tb = tag.getLocalBounds();
                tag.setPosition(screen.x - tb.width / 2.f - tb.left, screen.y - px - 6.f);
                target.draw(tag);
            }
        }
    };
    drawTeam(home, true);
    drawTeam(away, false);
}

void Renderer::drawControlsPanel(sf::RenderTarget& target) {
    struct Control { const char* key; const char* action; };
    static const Control controls[] = {
        {"WASD", "Move"}, {"Shift", "Sprint"}, {"Space", "Shoot"},
        {"E", "Pass"}, {"R", "Long pass"}, {"Q", "Next player"},
        {"Tab", "Nearest"}, {"C", "Tackle"}, {"P", "Pause"},
    };

    float panelW = WINDOW_WIDTH - 20.f;
    float panelH = 58.f;
    sf::RectangleShape panel({panelW, panelH});
    panel.setFillColor(sf::Color(0, 0, 0, 185));
    panel.setOutlineThickness(1.f);
    panel.setOutlineColor(sf::Color(255, 255, 255, 40));
    panel.setPosition(10.f, WINDOW_HEIGHT - panelH - 8.f);
    target.draw(panel);

    drawText(target, "CONTROLS", 22.f, WINDOW_HEIGHT - panelH + 2.f, 12, sf::Color(255, 220, 80), true);

    float x = 22.f;
    float y = WINDOW_HEIGHT - panelH + 22.f;
    for (const auto& c : controls) {
        sf::RectangleShape keyBox({36.f, 22.f});
        keyBox.setFillColor(sf::Color(50, 50, 50, 220));
        keyBox.setOutlineThickness(1.f);
        keyBox.setOutlineColor(sf::Color(120, 120, 120));
        keyBox.setPosition(x, y);
        target.draw(keyBox);

        std::string keyStr(c.key);
        drawText(target, keyStr, x + 18.f - keyStr.length() * 3.5f, y + 3.f, 11, sf::Color::White, true);
        drawText(target, c.action, x + 42.f, y + 4.f, 12, sf::Color(210, 210, 210));

        x += 128.f;
        if (x > WINDOW_WIDTH - 120.f) {
            x = 22.f;
            y += 26.f;
        }
    }
}

void Renderer::drawHUD(sf::RenderTarget& target, const Game& game) {
    const Team& home = game.getHomeTeam();
    const Team& away = game.getAwayTeam();
    const MatchStats& stats = game.getStats();

    sf::RectangleShape scoreBg({460.f, 78.f});
    scoreBg.setFillColor(sf::Color(0, 0, 0, 190));
    scoreBg.setOutlineThickness(1.f);
    scoreBg.setOutlineColor(sf::Color(255, 255, 255, 35));
    scoreBg.setPosition(WINDOW_WIDTH / 2.f - 230.f, 8.f);
    target.draw(scoreBg);

    sf::RectangleShape homeBlock({6.f, 50.f});
    homeBlock.setFillColor(homeColor());
    homeBlock.setPosition(WINDOW_WIDTH / 2.f - 220.f, 18.f);
    target.draw(homeBlock);

    sf::RectangleShape awayBlock({6.f, 50.f});
    awayBlock.setFillColor(awayColor());
    awayBlock.setPosition(WINDOW_WIDTH / 2.f + 214.f, 18.f);
    target.draw(awayBlock);

    if (fontLoaded_) {
        std::ostringstream score;
        score << home.getScore() << "  -  " << away.getScore();
        sf::Text scoreText(score.str(), font_, 36);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setStyle(sf::Text::Bold);
        sf::FloatRect bounds = scoreText.getLocalBounds();
        scoreText.setPosition(WINDOW_WIDTH / 2.f - bounds.width / 2.f - bounds.left, 16.f);
        target.draw(scoreText);

        drawText(target, home.getName(), WINDOW_WIDTH / 2.f - 210.f, 58.f, 13, homeColor());
        drawText(target, away.getName(), WINDOW_WIDTH / 2.f + 130.f, 58.f, 13, awayColor());

        int gameMinutes = static_cast<int>(game.getMatchTime() / MATCH_DURATION * 90.f);
        std::ostringstream timeStr;
        timeStr << std::setw(2) << std::setfill('0') << gameMinutes << "'   Half " << game.getHalf();
        drawText(target, timeStr.str(), WINDOW_WIDTH / 2.f - 52.f, 58.f, 14, sf::Color(220, 220, 220));
    }

    float homePoss = stats.getPossessionPercent(TeamSide::Home);
    drawBar(target, WINDOW_WIDTH / 2.f - 90.f, 72.f, 180.f, 6.f,
            homePoss / 100.f, homeColor(), awayColor());

    if (home.getControlledPlayer()) {
        const Player* ctrl = home.getControlledPlayer();
        sf::RectangleShape playerPanel({200.f, 58.f});
        playerPanel.setFillColor(sf::Color(0, 0, 0, 170));
        playerPanel.setOutlineThickness(1.f);
        playerPanel.setOutlineColor(sf::Color(255, 230, 50, 80));
        playerPanel.setPosition(10.f, 8.f);
        target.draw(playerPanel);

        drawText(target, ctrl->getName(), 18.f, 14.f, 17, sf::Color::Yellow, true);
        drawText(target, "#" + std::to_string(ctrl->getJerseyNumber()), 18.f, 36.f, 13, sf::Color(200, 200, 200));

        float stamina = ctrl->getStamina() / 100.f;
        sf::Color staminaColor = stamina > 0.5f ? sf::Color(50, 220, 80) :
                               stamina > 0.25f ? sf::Color(220, 180, 40) : sf::Color(220, 60, 60);
        drawBar(target, 90.f, 40.f, 100.f, 10.f, stamina, staminaColor, sf::Color(40, 40, 40, 180));
        drawText(target, "STA", 90.f, 26.f, 10, sf::Color(160, 160, 160));

        if (ctrl->hasBall(game.getBall())) {
            sf::RectangleShape ballTag({44.f, 18.f});
            ballTag.setFillColor(sf::Color(255, 255, 255, 220));
            ballTag.setPosition(148.f, 14.f);
            target.draw(ballTag);
            drawText(target, "BALL", 154.f, 16.f, 11, sf::Color(20, 20, 20), true);
        }
    }

    sf::RectangleShape statsPanel({200.f, 72.f});
    statsPanel.setFillColor(sf::Color(0, 0, 0, 160));
    statsPanel.setPosition(WINDOW_WIDTH - 210.f, 8.f);
    target.draw(statsPanel);

    std::ostringstream quickStats;
    quickStats << "Shots  " << stats.getShots(TeamSide::Home) << " - " << stats.getShots(TeamSide::Away) << "\n"
               << "Pass   " << stats.getPasses(TeamSide::Home) << " - " << stats.getPasses(TeamSide::Away) << "\n"
               << "Poss   " << static_cast<int>(homePoss) << "% - " << static_cast<int>(100 - homePoss) << "%";
    drawText(target, quickStats.str(), WINDOW_WIDTH - 198.f, 14.f, 12, sf::Color(210, 210, 210));

    float feedY = 92.f;
    for (const auto& event : stats.getEventFeed()) {
        drawText(target, event, 10.f, feedY, 12, sf::Color(255, 255, 200, 190));
        feedY += 16.f;
        if (feedY > 170.f) break;
    }

    drawControlsPanel(target);
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
                     WINDOW_HEIGHT / 2.f - bounds.height / 2.f - bounds.top - 40.f);
    target.draw(text);
}

void Renderer::drawMainMenu(sf::RenderTarget& target) {
    sf::RectangleShape bg({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    bg.setFillColor(sf::Color(15, 50, 20));
    target.draw(bg);

    // Decorative pitch stripe
    for (int i = 0; i < 8; ++i) {
        sf::RectangleShape stripe({static_cast<float>(WINDOW_WIDTH), 40.f});
        stripe.setPosition(0.f, i * 100.f);
        stripe.setFillColor(i % 2 == 0 ? sf::Color(25, 90, 30, 80) : sf::Color(20, 75, 25, 80));
        target.draw(stripe);
    }

    drawText(target, "FOOTBALL 11v11", WINDOW_WIDTH / 2.f - 180.f, 120.f, 52, sf::Color::White, true);
    drawText(target, "Blue FC  vs  Red United", WINDOW_WIDTH / 2.f - 130.f, 190.f, 24, sf::Color(200, 220, 255));

    sf::RectangleShape panel({500.f, 280.f});
    panel.setFillColor(sf::Color(0, 0, 0, 140));
    panel.setPosition(WINDOW_WIDTH / 2.f - 250.f, 250.f);
    target.draw(panel);

    drawText(target, "Features", WINDOW_WIDTH / 2.f - 50.f, 265.f, 22, sf::Color::Yellow, true);
    drawText(target, "- Full 11v11 match with AI opponents", WINDOW_WIDTH / 2.f - 220.f, 300.f, 16, sf::Color(220, 220, 220));
    drawText(target, "- Passing, shooting, tackling, and sprinting", WINDOW_WIDTH / 2.f - 220.f, 325.f, 16, sf::Color(220, 220, 220));
    drawText(target, "- Corners and goal kicks with safe restarts", WINDOW_WIDTH / 2.f - 220.f, 350.f, 16, sf::Color(220, 220, 220));
    drawText(target, "- Q cycles player | Tab jumps to ball", WINDOW_WIDTH / 2.f - 220.f, 375.f, 16, sf::Color(220, 220, 220));

    drawText(target, "Press ENTER or SPACE to kick off", WINDOW_WIDTH / 2.f - 175.f, 440.f, 22, sf::Color::White, true);
    drawText(target, "Esc to quit", WINDOW_WIDTH / 2.f - 50.f, 475.f, 16, sf::Color(150, 150, 150));
}

void Renderer::drawMatchSummary(sf::RenderTarget& target, const Game& game) {
    const MatchStats& stats = game.getStats();
    const Team& home = game.getHomeTeam();
    const Team& away = game.getAwayTeam();

    sf::RectangleShape panel({360.f, 220.f});
    panel.setFillColor(sf::Color(0, 0, 0, 200));
    panel.setPosition(WINDOW_WIDTH / 2.f - 180.f, WINDOW_HEIGHT / 2.f + 20.f);
    target.draw(panel);

    drawText(target, "MATCH STATS", WINDOW_WIDTH / 2.f - 80.f, WINDOW_HEIGHT / 2.f + 30.f, 20, sf::Color::Yellow, true);

    auto row = [&](const std::string& label, int homeVal, int awayVal, float y) {
        std::ostringstream line;
        line << homeVal << "  " << label << "  " << awayVal;
        drawText(target, line.str(), WINDOW_WIDTH / 2.f - 80.f, y, 16, sf::Color::White);
    };

    float y = WINDOW_HEIGHT / 2.f + 65.f;
    row("Shots", stats.getShots(TeamSide::Home), stats.getShots(TeamSide::Away), y); y += 28;
    row("Passes", stats.getPasses(TeamSide::Home), stats.getPasses(TeamSide::Away), y); y += 28;
    row("Tackles", stats.getTackles(TeamSide::Home), stats.getTackles(TeamSide::Away), y); y += 28;

    std::ostringstream poss;
    poss << static_cast<int>(stats.getPossessionPercent(TeamSide::Home)) << "% Possession "
         << static_cast<int>(stats.getPossessionPercent(TeamSide::Away)) << "%";
    drawText(target, poss.str(), WINDOW_WIDTH / 2.f - 100.f, y, 15, sf::Color(180, 220, 255));

    std::string result;
    if (home.getScore() > away.getScore()) result = home.getName() + " wins!";
    else if (away.getScore() > home.getScore()) result = away.getName() + " wins!";
    else result = "Draw!";
    drawText(target, result, WINDOW_WIDTH / 2.f - 60.f, WINDOW_HEIGHT / 2.f + 210.f, 18, sf::Color(255, 220, 80), true);
}
