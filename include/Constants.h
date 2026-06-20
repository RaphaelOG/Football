#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <cmath>

namespace Constants {

// Field dimensions (meters, scaled 1:1 in game units)
constexpr float FIELD_LENGTH = 105.0f;
constexpr float FIELD_WIDTH  = 68.0f;
constexpr float GOAL_WIDTH   = 7.32f;
constexpr float GOAL_DEPTH   = 2.5f;
constexpr float PENALTY_AREA_LENGTH = 16.5f;
constexpr float PENALTY_AREA_WIDTH  = 40.32f;
constexpr float GOAL_AREA_LENGTH    = 5.5f;
constexpr float GOAL_AREA_WIDTH     = 18.32f;
constexpr float CENTER_CIRCLE_RADIUS = 9.15f;
constexpr float PENALTY_SPOT_DIST    = 11.0f;

// Player
constexpr float PLAYER_RADIUS     = 1.1f;
constexpr float GK_RADIUS         = 1.2f;
constexpr float PLAYER_MAX_SPEED  = 7.5f;
constexpr float PLAYER_SPRINT_SPEED = 11.0f;
constexpr float PLAYER_ACCEL      = 28.0f;
constexpr float PLAYER_FRICTION   = 14.0f;
constexpr float GK_MAX_SPEED      = 5.5f;

// Ball
constexpr float BALL_RADIUS       = 0.35f;
constexpr float BALL_FRICTION     = 3.5f;
constexpr float BALL_MAX_SPEED    = 22.0f;
constexpr float KICK_POWER        = 14.0f;
constexpr float SHOOT_POWER       = 20.0f;
constexpr float PASS_POWER        = 12.0f;
constexpr float LONG_PASS_POWER   = 18.0f;
constexpr float POSSESSION_DIST   = 1.8f;
constexpr float TACKLE_RANGE      = 2.2f;
constexpr float TACKLE_COOLDOWN   = 1.2f;

// Match
constexpr float MATCH_DURATION    = 540.0f;  // 9 min real = 90 min game
constexpr float HALF_DURATION     = MATCH_DURATION / 2.0f;
constexpr float GOAL_CELEBRATION  = 3.0f;
constexpr float KICKOFF_DELAY     = 1.5f;
constexpr float SET_PIECE_DELAY   = 1.5f;
constexpr float OOB_COOLDOWN      = 2.5f;
constexpr float FOUL_CHANCE       = 0.08f;
constexpr float FIELD_INSET       = 2.0f;

// Window / camera
constexpr int   WINDOW_WIDTH  = 1280;
constexpr int   WINDOW_HEIGHT = 720;
constexpr float PIXELS_PER_METER = 8.0f;
constexpr float CAMERA_SMOOTH  = 6.0f;

// Teams
constexpr int PLAYERS_PER_TEAM = 11;

enum class Position {
    GK,
    CB, LB, RB,
    CM, LM, RM, CDM,
    ST, LW, RW
};

enum class TeamSide { Home, Away };

enum class MatchState {
    MainMenu,
    Kickoff,
    Playing,
    GoalCelebration,
    HalfTime,
    FullTime,
    Paused,
    ThrowIn,
    GoalKick,
    CornerKick
};

enum class KickType { Pass, Shoot, LongPass };

enum class OutOfBoundsType { None, ThrowIn, GoalKick, CornerKick };

struct SetPieceInfo {
    OutOfBoundsType type = OutOfBoundsType::None;
    TeamSide takingTeam = TeamSide::Home;
    sf::Vector2f position = {0.f, 0.f};
};

enum class PlayerRole {
  Goalkeeper,
  Defender,
  Midfielder,
  Striker
};

inline sf::Color homeColor()  { return sf::Color(30, 100, 220); }
inline sf::Color awayColor()  { return sf::Color(220, 40, 40); }
inline sf::Color fieldColor() { return sf::Color(34, 139, 34); }
inline sf::Color lineColor()  { return sf::Color(255, 255, 255, 180); }

inline float length(const sf::Vector2f& v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

inline sf::Vector2f normalized(const sf::Vector2f& v) {
    float len = length(v);
    if (len < 0.0001f) return {0.f, 0.f};
    return {v.x / len, v.y / len};
}

} // namespace Constants
