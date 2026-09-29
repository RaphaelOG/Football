#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include "Renderer.h"
#include "Game.h"
#include "Team.h"
#include "Ball.h"
#include "Player.h"
#include <SFML/OpenGL.hpp>
#include <cmath>
#include <sstream>
#include <iomanip>

using namespace Constants;

namespace {

constexpr float PI = 3.14159265f;

struct V3 { float x, y, z; };

V3 sub(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V3 cross(V3 a, V3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
V3 norm(V3 a) {
    float len = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    if (len < 1e-5f) return {0.f, 1.f, 0.f};
    return {a.x / len, a.y / len, a.z / len};
}

void setPerspective(float fovDeg, float aspect, float zn, float zf) {
    float f = 1.f / std::tan(fovDeg * PI / 360.f);
    float m[16] = {
        f / aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, (zf + zn) / (zn - zf), -1,
        0, 0, (2.f * zf * zn) / (zn - zf), 0
    };
    glLoadMatrixf(m);
}

void setLookAt(V3 eye, V3 center, V3 up) {
    V3 f = norm(sub(center, eye));
    V3 s = norm(cross(f, up));
    V3 u = cross(s, f);
    float m[16] = {
        s.x, u.x, -f.x, 0,
        s.y, u.y, -f.y, 0,
        s.z, u.z, -f.z, 0,
        0, 0, 0, 1
    };
    glMultMatrixf(m);
    glTranslatef(-eye.x, -eye.y, -eye.z);
}

void box(float cx, float cy, float cz, float hx, float hy, float hz) {
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(cx - hx, cy + hy, cz - hz); glVertex3f(cx + hx, cy + hy, cz - hz);
    glVertex3f(cx + hx, cy + hy, cz + hz); glVertex3f(cx - hx, cy + hy, cz + hz);
    glNormal3f(0, -1, 0);
    glVertex3f(cx - hx, cy - hy, cz + hz); glVertex3f(cx + hx, cy - hy, cz + hz);
    glVertex3f(cx + hx, cy - hy, cz - hz); glVertex3f(cx - hx, cy - hy, cz - hz);
    glNormal3f(0, 0, 1);
    glVertex3f(cx - hx, cy - hy, cz + hz); glVertex3f(cx - hx, cy + hy, cz + hz);
    glVertex3f(cx + hx, cy + hy, cz + hz); glVertex3f(cx + hx, cy - hy, cz + hz);
    glNormal3f(0, 0, -1);
    glVertex3f(cx + hx, cy - hy, cz - hz); glVertex3f(cx + hx, cy + hy, cz - hz);
    glVertex3f(cx - hx, cy + hy, cz - hz); glVertex3f(cx - hx, cy - hy, cz - hz);
    glNormal3f(1, 0, 0);
    glVertex3f(cx + hx, cy - hy, cz - hz); glVertex3f(cx + hx, cy - hy, cz + hz);
    glVertex3f(cx + hx, cy + hy, cz + hz); glVertex3f(cx + hx, cy + hy, cz - hz);
    glNormal3f(-1, 0, 0);
    glVertex3f(cx - hx, cy - hy, cz + hz); glVertex3f(cx - hx, cy - hy, cz - hz);
    glVertex3f(cx - hx, cy + hy, cz - hz); glVertex3f(cx - hx, cy + hy, cz + hz);
    glEnd();
}

void sphere(float cx, float cy, float cz, float radius, int stacks, int slices, bool soccer) {
    for (int i = 0; i < stacks; ++i) {
        float v0 = PI * i / stacks;
        float v1 = PI * (i + 1) / stacks;
        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= slices; ++j) {
            float u = 2.f * PI * j / slices;
            for (int k = 0; k < 2; ++k) {
                float v = k == 0 ? v0 : v1;
                float nx = std::sin(v) * std::cos(u);
                float ny = std::cos(v);
                float nz = std::sin(v) * std::sin(u);
                if (soccer) {
                    bool panel = ((i / 2) + (j / 2)) % 3 == 0;
                    if (panel) glColor3f(0.08f, 0.08f, 0.08f);
                    else glColor3f(0.95f, 0.95f, 0.93f);
                }
                glNormal3f(nx, ny, nz);
                glVertex3f(cx + nx * radius, cy + ny * radius, cz + nz * radius);
            }
        }
        glEnd();
    }
}

void cylinderY(float cx, float cy, float cz, float radius, float halfHeight, int slices) {
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= slices; ++i) {
        float a = 2.f * PI * i / slices;
        float nx = std::cos(a), nz = std::sin(a);
        glNormal3f(nx, 0, nz);
        glVertex3f(cx + nx * radius, cy + halfHeight, cz + nz * radius);
        glVertex3f(cx + nx * radius, cy - halfHeight, cz + nz * radius);
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glVertex3f(cx, cy + halfHeight, cz);
    for (int i = 0; i <= slices; ++i) {
        float a = 2.f * PI * i / slices;
        glVertex3f(cx + std::cos(a) * radius, cy + halfHeight, cz + std::sin(a) * radius);
    }
    glEnd();
}

void ring(float x, float y, float z, float radius, float width, float r, float g, float b) {
    glDisable(GL_LIGHTING);
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= 40; ++i) {
        float a = 2.f * PI * i / 40.f;
        float c = std::cos(a), s = std::sin(a);
        glVertex3f(x + c * (radius - width), z, y + s * (radius - width));
        glVertex3f(x + c * (radius + width), z, y + s * (radius + width));
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void arc(float cx, float cy, float z, float radius, float a0, float a1, float width) {
    const int segs = 36;
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= segs; ++i) {
        float a = a0 + (a1 - a0) * i / segs;
        float c = std::cos(a), s = std::sin(a);
        glVertex3f(cx + c * (radius - width), z, cy + s * (radius - width));
        glVertex3f(cx + c * (radius + width), z, cy + s * (radius + width));
    }
    glEnd();
}

void lineQuad(float x1, float y1, float x2, float y2, float width, float z) {
    float dx = x2 - x1, dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.001f) return;
    float nx = -dy / len * width * 0.5f;
    float ny = dx / len * width * 0.5f;
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(x1 + nx, z, y1 + ny);
    glVertex3f(x2 + nx, z, y2 + ny);
    glVertex3f(x2 - nx, z, y2 - ny);
    glVertex3f(x1 - nx, z, y1 - ny);
    glEnd();
}

bool projectToScreen(float x, float y, float z, float& sx, float& sy) {
    GLfloat model[16], proj[16];
    GLint viewport[4];
    glGetFloatv(GL_MODELVIEW_MATRIX, model);
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    glGetIntegerv(GL_VIEWPORT, viewport);

    float eye[4] = {
        model[0] * x + model[4] * y + model[8] * z + model[12],
        model[1] * x + model[5] * y + model[9] * z + model[13],
        model[2] * x + model[6] * y + model[10] * z + model[14],
        model[3] * x + model[7] * y + model[11] * z + model[15]
    };
    float clip[4] = {
        proj[0] * eye[0] + proj[4] * eye[1] + proj[8] * eye[2] + proj[12] * eye[3],
        proj[1] * eye[0] + proj[5] * eye[1] + proj[9] * eye[2] + proj[13] * eye[3],
        proj[2] * eye[0] + proj[6] * eye[1] + proj[10] * eye[2] + proj[14] * eye[3],
        proj[3] * eye[0] + proj[7] * eye[1] + proj[11] * eye[2] + proj[15] * eye[3]
    };
    if (clip[3] <= 0.05f) return false;
    float ndcX = clip[0] / clip[3];
    float ndcY = clip[1] / clip[3];
    sx = viewport[0] + (ndcX + 1.f) * viewport[2] * 0.5f;
    sy = viewport[1] + (ndcY + 1.f) * viewport[3] * 0.5f;
    sy = viewport[3] - sy;
    return ndcX > -1.2f && ndcX < 1.2f && ndcY > -1.2f && ndcY < 1.2f;
}

} // namespace

Renderer::Renderer()
    : fontLoaded_(false), cameraReady_(false),
      camX_(FIELD_LENGTH * 0.5f), camY_(28.f), camZ_(FIELD_WIDTH * 0.5f - 40.f),
      lookX_(FIELD_LENGTH * 0.5f), lookY_(0.f), lookZ_(FIELD_WIDTH * 0.5f),
      ballSpin_(0.f) {
    const char* fontPaths[] = {
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/SFNS.ttf",
    };
    for (const char* path : fontPaths) {
        if (font_.loadFromFile(path)) {
            fontLoaded_ = true;
            break;
        }
    }
}

void Renderer::drawScene(sf::RenderWindow& window, const Team& home, const Team& away, const Ball& ball,
                         const Player* controlled, const sf::Vector2f& focus, float animTime, float dt) {
    labels_.clear();
    window.setActive(true);
    beginFrame(window, focus, ball.getVelocity(), dt);
    drawWorld(home, away, ball, controlled, animTime);
}

void Renderer::beginFrame(sf::RenderWindow& window, const sf::Vector2f& focus, const sf::Vector2f& velocity, float dt) {
    sf::Vector2u size = window.getSize();
    glViewport(0, 0, static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y));
    glClearColor(0.53f, 0.74f, 0.95f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);

    GLfloat ambient[] = {0.48f, 0.50f, 0.52f, 1.f};
    GLfloat diffuse[] = {0.95f, 0.93f, 0.88f, 1.f};
    GLfloat pos[] = {40.f, 55.f, -15.f, 1.f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    GLfloat spec[] = {0.25f, 0.25f, 0.25f, 1.f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 18.f);

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    GLfloat fog[] = {0.53f, 0.74f, 0.95f, 1.f};
    glFogfv(GL_FOG_COLOR, fog);
    glFogf(GL_FOG_START, 70.f);
    glFogf(GL_FOG_END, 190.f);

    sf::Vector2f look = focus + velocity * 0.25f;
    look.x = std::max(14.f, std::min(FIELD_LENGTH - 14.f, look.x));
    look.y = std::max(18.f, std::min(FIELD_WIDTH - 18.f, look.y));
    float targetEyeX = look.x;
    float targetEyeY = 32.f;
    float targetEyeZ = look.y - 46.f;
    float targetLookX = look.x;
    float targetLookY = 0.4f;
    float targetLookZ = look.y + 8.f;

    if (!cameraReady_) {
        camX_ = targetEyeX; camY_ = targetEyeY; camZ_ = targetEyeZ;
        lookX_ = targetLookX; lookY_ = targetLookY; lookZ_ = targetLookZ;
        cameraReady_ = true;
    } else {
        float k = 1.f - std::exp(-3.2f * std::max(dt, 0.001f));
        camX_ += (targetEyeX - camX_) * k;
        camY_ += (targetEyeY - camY_) * k;
        camZ_ += (targetEyeZ - camZ_) * k;
        lookX_ += (targetLookX - lookX_) * k;
        lookY_ += (targetLookY - lookY_) * k;
        lookZ_ += (targetLookZ - lookZ_) * k;
    }

    glMatrixMode(GL_PROJECTION);
    setPerspective(58.f, size.x / static_cast<float>(std::max(1u, size.y)), 0.4f, 280.f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    setLookAt({camX_, camY_, camZ_}, {lookX_, lookY_, lookZ_}, {0.f, 1.f, 0.f});
}

void Renderer::drawWorld(const Team& home, const Team& away, const Ball& ball,
                         const Player* controlled, float animTime) {
    drawStadium();
    drawPitch();
    drawGoals();
    for (const auto& p : away.getPlayers()) drawPlayer(p, controlled == &p, animTime);
    for (const auto& p : home.getPlayers()) drawPlayer(p, controlled == &p, animTime);
    drawBall(ball, animTime);
}

void Renderer::drawStadium() {
    glColor3f(0.18f, 0.32f, 0.16f);
    box(FIELD_LENGTH * 0.5f, -0.55f, FIELD_WIDTH * 0.5f, 90.f, 0.25f, 70.f);

    auto stand = [](float cx, float cy, float cz, float hx, float hy, float hz, float r, float g, float b) {
        glColor3f(r, g, b);
        box(cx, cy, cz, hx, hy, hz);
    };
    stand(FIELD_LENGTH * 0.5f, 4.5f, -12.f, 62.f, 4.5f, 6.f, 0.18f, 0.22f, 0.32f);
    stand(FIELD_LENGTH * 0.5f, 8.2f, -16.f, 62.f, 2.2f, 5.f, 0.55f, 0.16f, 0.18f);
    stand(FIELD_LENGTH * 0.5f, 4.5f, FIELD_WIDTH + 12.f, 62.f, 4.5f, 6.f, 0.18f, 0.22f, 0.32f);
    stand(FIELD_LENGTH * 0.5f, 8.2f, FIELD_WIDTH + 16.f, 62.f, 2.2f, 5.f, 0.15f, 0.28f, 0.62f);
    stand(-14.f, 5.f, FIELD_WIDTH * 0.5f, 6.f, 5.f, 28.f, 0.22f, 0.22f, 0.26f);
    stand(FIELD_LENGTH + 14.f, 5.f, FIELD_WIDTH * 0.5f, 6.f, 5.f, 28.f, 0.22f, 0.22f, 0.26f);

    glDisable(GL_LIGHTING);
    glColor3f(0.95f, 0.95f, 0.85f);
    box(8.f, 16.f, -8.f, 1.2f, 0.4f, 1.2f);
    box(FIELD_LENGTH - 8.f, 16.f, -8.f, 1.2f, 0.4f, 1.2f);
    box(8.f, 16.f, FIELD_WIDTH + 8.f, 1.2f, 0.4f, 1.2f);
    box(FIELD_LENGTH - 8.f, 16.f, FIELD_WIDTH + 8.f, 1.2f, 0.4f, 1.2f);
    glColor3f(0.75f, 0.75f, 0.78f);
    box(8.f, 8.f, -8.f, 0.15f, 8.f, 0.15f);
    box(FIELD_LENGTH - 8.f, 8.f, -8.f, 0.15f, 8.f, 0.15f);
    box(8.f, 8.f, FIELD_WIDTH + 8.f, 0.15f, 8.f, 0.15f);
    box(FIELD_LENGTH - 8.f, 8.f, FIELD_WIDTH + 8.f, 0.15f, 8.f, 0.15f);
    glEnable(GL_LIGHTING);

    glColor3f(0.92f, 0.92f, 0.9f);
    box(FIELD_LENGTH * 0.5f, 0.45f, -1.15f, FIELD_LENGTH * 0.5f + 1.f, 0.45f, 0.35f);
    box(FIELD_LENGTH * 0.5f, 0.45f, FIELD_WIDTH + 1.15f, FIELD_LENGTH * 0.5f + 1.f, 0.45f, 0.35f);
}

void Renderer::drawPitch() {
    glColor3f(0.05f, 0.28f, 0.08f);
    box(FIELD_LENGTH * 0.5f, -0.18f, FIELD_WIDTH * 0.5f, FIELD_LENGTH * 0.5f + 1.6f, 0.18f, FIELD_WIDTH * 0.5f + 1.6f);

    const int stripes = 14;
    float stripeW = FIELD_LENGTH / stripes;
    for (int i = 0; i < stripes; ++i) {
        if (i % 2 == 0) glColor3f(0.16f, 0.52f, 0.20f);
        else glColor3f(0.12f, 0.45f, 0.16f);
        float x = stripeW * i + stripeW * 0.5f;
        box(x, 0.02f, FIELD_WIDTH * 0.5f, stripeW * 0.5f, 0.02f, FIELD_WIDTH * 0.5f);
    }

    glDisable(GL_LIGHTING);
    glColor3f(0.96f, 0.96f, 0.96f);
    float z = 0.07f;
    float w = 0.12f;
    lineQuad(0, 0, FIELD_LENGTH, 0, w, z);
    lineQuad(FIELD_LENGTH, 0, FIELD_LENGTH, FIELD_WIDTH, w, z);
    lineQuad(FIELD_LENGTH, FIELD_WIDTH, 0, FIELD_WIDTH, w, z);
    lineQuad(0, FIELD_WIDTH, 0, 0, w, z);
    lineQuad(FIELD_LENGTH * 0.5f, 0, FIELD_LENGTH * 0.5f, FIELD_WIDTH, w, z);
    arc(FIELD_LENGTH * 0.5f, FIELD_WIDTH * 0.5f, z, CENTER_CIRCLE_RADIUS, 0, 2.f * PI, w * 0.5f);

    auto boxLines = [&](float x, float depth, float boxW) {
        float y0 = (FIELD_WIDTH - boxW) * 0.5f;
        float y1 = y0 + boxW;
        float x1 = x + depth;
        lineQuad(x, y0, x1, y0, w, z);
        lineQuad(x1, y0, x1, y1, w, z);
        lineQuad(x1, y1, x, y1, w, z);
    };
    boxLines(0, PENALTY_AREA_LENGTH, PENALTY_AREA_WIDTH);
    boxLines(0, GOAL_AREA_LENGTH, GOAL_AREA_WIDTH);
    boxLines(FIELD_LENGTH, -PENALTY_AREA_LENGTH, PENALTY_AREA_WIDTH);
    boxLines(FIELD_LENGTH, -GOAL_AREA_LENGTH, GOAL_AREA_WIDTH);

    arc(PENALTY_SPOT_DIST, FIELD_WIDTH * 0.5f, z, CENTER_CIRCLE_RADIUS, -0.9f, 0.9f, w * 0.5f);
    arc(FIELD_LENGTH - PENALTY_SPOT_DIST, FIELD_WIDTH * 0.5f, z, CENTER_CIRCLE_RADIUS, PI - 0.9f, PI + 0.9f, w * 0.5f);
    arc(0, 0, z, 1.f, 0, PI * 0.5f, w * 0.45f);
    arc(0, FIELD_WIDTH, z, 1.f, -PI * 0.5f, 0, w * 0.45f);
    arc(FIELD_LENGTH, 0, z, 1.f, PI * 0.5f, PI, w * 0.45f);
    arc(FIELD_LENGTH, FIELD_WIDTH, z, 1.f, PI, PI * 1.5f, w * 0.45f);

    glColor3f(1.f, 1.f, 1.f);
    sphere(FIELD_LENGTH * 0.5f, z, FIELD_WIDTH * 0.5f, 0.18f, 6, 8, false);
    sphere(PENALTY_SPOT_DIST, z, FIELD_WIDTH * 0.5f, 0.16f, 6, 8, false);
    sphere(FIELD_LENGTH - PENALTY_SPOT_DIST, z, FIELD_WIDTH * 0.5f, 0.16f, 6, 8, false);

    glColor3f(1.f, 0.25f, 0.2f);
    cylinderY(0.15f, 0.7f, 0.15f, 0.05f, 0.7f, 6);
    cylinderY(0.15f, 0.7f, FIELD_WIDTH - 0.15f, 0.05f, 0.7f, 6);
    cylinderY(FIELD_LENGTH - 0.15f, 0.7f, 0.15f, 0.05f, 0.7f, 6);
    cylinderY(FIELD_LENGTH - 0.15f, 0.7f, FIELD_WIDTH - 0.15f, 0.05f, 0.7f, 6);
    glEnable(GL_LIGHTING);
}

void Renderer::drawGoals() {
    float top = (FIELD_WIDTH - GOAL_WIDTH) * 0.5f;
    float bot = top + GOAL_WIDTH;
    auto goal = [&](float x, float dir) {
        glColor3f(0.95f, 0.95f, 0.97f);
        cylinderY(x, GOAL_HEIGHT * 0.5f, top, 0.07f, GOAL_HEIGHT * 0.5f, 10);
        cylinderY(x, GOAL_HEIGHT * 0.5f, bot, 0.07f, GOAL_HEIGHT * 0.5f, 10);
        glPushMatrix();
        glTranslatef(x, GOAL_HEIGHT, (top + bot) * 0.5f);
        glRotatef(90.f, 1, 0, 0);
        cylinderY(0, 0, 0, 0.07f, GOAL_WIDTH * 0.5f, 10);
        glPopMatrix();

        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.92f, 0.94f, 0.96f, 0.55f);
        glLineWidth(1.5f);
        glBegin(GL_LINES);
        const int nx = 8, ny = 10;
        float back = x + dir * GOAL_DEPTH;
        for (int i = 0; i <= ny; ++i) {
            float y = top + GOAL_WIDTH * i / ny;
            glVertex3f(x, 0.05f, y); glVertex3f(back, 0.35f, y);
            glVertex3f(x, GOAL_HEIGHT, y); glVertex3f(back, GOAL_HEIGHT * 0.72f, y);
            glVertex3f(x, 0.05f, y); glVertex3f(x, GOAL_HEIGHT, y);
        }
        for (int i = 0; i <= nx; ++i) {
            float h = GOAL_HEIGHT * i / nx;
            glVertex3f(x, h, top); glVertex3f(x, h, bot);
            float bh = 0.35f + (GOAL_HEIGHT * 0.72f - 0.35f) * i / nx;
            glVertex3f(back, bh, top); glVertex3f(back, bh, bot);
        }
        glEnd();
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    };
    goal(0.f, -1.f);
    goal(FIELD_LENGTH, 1.f);
}

void Renderer::drawPlayer(const Player& player, bool controlled, float animTime) {
    sf::Vector2f pos = player.getPosition();
    sf::Vector2f facing = player.getFacing();
    if (length(facing) < 0.1f) facing = {1.f, 0.f};
    float speed = length(player.getVelocity());
    float phase = animTime * (3.5f + speed * 0.7f) + player.getId();
    float swing = speed > 0.45f ? std::sin(phase) * std::min(speed / 7.f, 1.f) : 0.f;
    float yaw = -std::atan2(facing.y, facing.x) * 180.f / PI;

    if (controlled) ring(pos.x, pos.y, 0.08f, 0.72f, 0.07f, 1.f, 0.86f, 0.1f);

    glDisable(GL_LIGHTING);
    glColor4f(0.f, 0.f, 0.f, 0.28f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(pos.x, 0.06f, pos.y);
    for (int i = 0; i <= 16; ++i) {
        float a = 2.f * PI * i / 16.f;
        glVertex3f(pos.x + std::cos(a) * 0.42f, 0.06f, pos.y + std::sin(a) * 0.32f);
    }
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    bool home = player.getTeam() == TeamSide::Home;
    float shirt[3] = {0.12f, 0.38f, 0.86f};
    float shorts[3] = {0.95f, 0.95f, 0.97f};
    float socks[3] = {0.12f, 0.38f, 0.86f};
    if (!home) {
        shirt[0] = 0.78f; shirt[1] = 0.10f; shirt[2] = 0.13f;
        shorts[0] = 0.10f; shorts[1] = 0.10f; shorts[2] = 0.12f;
        socks[0] = 0.78f; socks[1] = 0.10f; socks[2] = 0.13f;
    }
    if (player.isGoalkeeper()) {
        if (home) { shirt[0] = 0.95f; shirt[1] = 0.82f; shirt[2] = 0.12f; }
        else { shirt[0] = 0.10f; shirt[1] = 0.72f; shirt[2] = 0.38f; }
        shorts[0] = shorts[1] = shorts[2] = 0.08f;
        socks[0] = shirt[0]; socks[1] = shirt[1]; socks[2] = shirt[2];
    }

    static const float skins[][3] = {
        {0.93f, 0.76f, 0.63f}, {0.80f, 0.58f, 0.42f}, {0.55f, 0.36f, 0.24f},
        {0.38f, 0.25f, 0.18f}, {0.96f, 0.84f, 0.74f}
    };
    const float* skin = skins[player.getId() % 5];

    glPushMatrix();
    glTranslatef(pos.x, 0.f, pos.y);
    glRotatef(yaw, 0, 1, 0);

    glColor3f(0.08f, 0.08f, 0.08f);
    box(0.08f + swing * 0.18f, 0.08f, 0.11f, 0.12f, 0.06f, 0.07f);
    box(0.08f - swing * 0.18f, 0.08f, -0.11f, 0.12f, 0.06f, 0.07f);
    glColor3fv(socks);
    box(swing * 0.16f, 0.28f, 0.11f, 0.08f, 0.16f, 0.07f);
    box(-swing * 0.16f, 0.28f, -0.11f, 0.08f, 0.16f, 0.07f);
    glColor3fv(skin);
    box(swing * 0.1f, 0.58f, 0.11f, 0.08f, 0.16f, 0.075f);
    box(-swing * 0.1f, 0.58f, -0.11f, 0.08f, 0.16f, 0.075f);
    glColor3fv(shorts);
    box(0.f, 0.86f, 0.f, 0.13f, 0.16f, 0.20f);
    glColor3fv(shirt);
    box(0.f, 1.22f, 0.f, 0.16f, 0.24f, 0.24f);
    glColor3f(0.95f, 0.95f, 0.97f);
    box(0.02f, 1.40f, 0.f, 0.16f, 0.035f, 0.24f);
    if (player.getSquadNumber() == 7) {
        glColor3f(0.95f, 0.82f, 0.15f);
        box(0.f, 1.18f, 0.28f, 0.05f, 0.04f, 0.05f);
    }

    glColor3fv(skin);
    box(swing * 0.22f, 1.12f, 0.34f, 0.07f, 0.20f, 0.06f);
    box(-swing * 0.22f, 1.12f, -0.34f, 0.07f, 0.20f, 0.06f);
    sphere(0.04f, 1.64f, 0.f, 0.16f, 8, 10, false);
    glColor3f(0.1f, 0.08f, 0.07f);
    sphere(-0.02f, 1.72f, 0.f, 0.15f, 8, 10, false);
    glColor3f(0.05f, 0.05f, 0.05f);
    sphere(0.14f, 1.66f, 0.06f, 0.025f, 4, 6, false);
    sphere(0.14f, 1.66f, -0.06f, 0.025f, 4, 6, false);
    glPopMatrix();

    sf::Color label = home ? sf::Color(190, 220, 255) : sf::Color(255, 190, 190);
    if (controlled) label = sf::Color::Yellow;
    std::string text = std::to_string(player.getSquadNumber());
    if (controlled) text = player.getName();
    rememberLabel(pos, 2.15f, text, label);
}

void Renderer::drawBall(const Ball& ball, float animTime) {
    (void)animTime;
    sf::Vector2f pos = ball.getPosition();
    float h = ball.getHeight();
    sf::Vector2f vel = ball.getVelocity();
    float speed = length(vel);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.f, 0.f, 0.f, 0.3f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(pos.x, 0.065f, pos.y);
    float shadow = 0.22f + h * 0.04f;
    for (int i = 0; i <= 14; ++i) {
        float a = 2.f * PI * i / 14.f;
        glVertex3f(pos.x + std::cos(a) * shadow, 0.065f, pos.y + std::sin(a) * shadow);
    }
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    if (speed > 0.2f) ballSpin_ += speed * 8.f;
    glPushMatrix();
    glTranslatef(pos.x, h, pos.y);
    if (speed > 0.2f) glRotatef(ballSpin_, -vel.y, 0.f, vel.x);
    glColor3f(0.95f, 0.95f, 0.93f);
    sphere(0, 0, 0, 0.22f, 8, 12, true);
    glPopMatrix();
}

void Renderer::rememberLabel(const sf::Vector2f& world, float height, const std::string& text, const sf::Color& color) {
    float sx, sy;
    if (!projectToScreen(world.x, height, world.y, sx, sy)) return;
    labels_.push_back({sf::Vector2f(sx, sy), text, color});
}

void Renderer::drawLabels(sf::RenderTarget& target) {
    if (!fontLoaded_) return;
    for (const auto& label : labels_) {
        sf::Text text(label.text, font_, label.text.size() > 2 ? 13 : 12);
        text.setFillColor(label.color);
        text.setStyle(sf::Text::Bold);
        sf::FloatRect bounds = text.getLocalBounds();
        text.setPosition(label.screen.x - bounds.width * 0.5f - bounds.left, label.screen.y);
        target.draw(text);
    }
}

void Renderer::drawHUD(sf::RenderTarget& target, const Game& game) {
    if (!fontLoaded_) return;
    const Team& home = game.getHomeTeam();
    const Team& away = game.getAwayTeam();

    std::ostringstream score;
    score << home.getName() << "   " << home.getScore() << "  -  " << away.getScore() << "   " << away.getName();
    sf::Text scoreText(score.str(), font_, 26);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setStyle(sf::Text::Bold);
    sf::FloatRect bounds = scoreText.getLocalBounds();
    float boxW = bounds.width + 48.f;
    sf::RectangleShape scoreBg({boxW, 72.f});
    scoreBg.setFillColor(sf::Color(8, 16, 28, 170));
    scoreBg.setPosition(WINDOW_WIDTH / 2.f - boxW / 2.f, 8.f);
    target.draw(scoreBg);
    scoreText.setPosition(WINDOW_WIDTH / 2.f - bounds.width / 2.f - bounds.left, 14.f);
    target.draw(scoreText);

    float shown = std::min(game.getMatchTime(), MATCH_DURATION);
    int gameSeconds = static_cast<int>(shown / MATCH_DURATION * 90.f * 60.f);
    int mins = gameSeconds / 60;
    int secs = gameSeconds % 60;
    std::ostringstream timeStr;
    timeStr << std::setw(2) << std::setfill('0') << mins << ":" << std::setw(2) << std::setfill('0') << secs
            << "    H" << game.getHalf();
    sf::Text timeText(timeStr.str(), font_, 18);
    timeText.setFillColor(sf::Color(230, 230, 230));
    sf::FloatRect tb = timeText.getLocalBounds();
    timeText.setPosition(WINDOW_WIDTH / 2.f - tb.width / 2.f - tb.left, 46.f);
    target.draw(timeText);

    if (const Player* controlled = home.getControlledPlayer()) {
        sf::Text playerText(controlled->getName() + "  #" + std::to_string(controlled->getSquadNumber()), font_, 16);
        playerText.setFillColor(sf::Color::Yellow);
        playerText.setPosition(16.f, 12.f);
        target.draw(playerText);

        float stamina = controlled->getStamina() / 100.f;
        sf::RectangleShape back({120.f, 8.f});
        back.setFillColor(sf::Color(0, 0, 0, 140));
        back.setPosition(16.f, 36.f);
        target.draw(back);
        sf::RectangleShape fill({120.f * stamina, 8.f});
        fill.setFillColor(stamina > 0.35f ? sf::Color(80, 200, 90) : sf::Color(210, 70, 50));
        fill.setPosition(16.f, 36.f);
        target.draw(fill);
    }

    sf::Text hint("WASD move   Shift sprint   Space shoot/tackle   E pass   F lofted pass   Q switch   C tackle   P pause",
                  font_, 14);
    hint.setFillColor(sf::Color(240, 240, 240, 210));
    hint.setPosition(12.f, WINDOW_HEIGHT - 26.f);
    target.draw(hint);
}

void Renderer::drawBanner(sf::RenderTarget& target, const std::string& message, const sf::Color& color) {
    if (!fontLoaded_) return;
    sf::Text text(message, font_, 42);
    text.setFillColor(color);
    text.setStyle(sf::Text::Bold);
    sf::FloatRect bounds = text.getLocalBounds();
    float w = bounds.width + 64.f;
    float h = bounds.height + 36.f;
    sf::RectangleShape panel({w, h});
    panel.setFillColor(sf::Color(6, 12, 22, 190));
    panel.setOutlineThickness(2.f);
    panel.setOutlineColor(sf::Color(255, 255, 255, 180));
    panel.setPosition(WINDOW_WIDTH / 2.f - w / 2.f, WINDOW_HEIGHT / 2.f - h / 2.f - 20.f);
    target.draw(panel);
    text.setPosition(WINDOW_WIDTH / 2.f - bounds.width / 2.f - bounds.left,
                     WINDOW_HEIGHT / 2.f - bounds.height / 2.f - bounds.top - 20.f);
    target.draw(text);
}
