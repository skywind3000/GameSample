// geometry.cpp - Geometry Wars (Endless Mode)
//
// Endless geometric shooter with spring grid, particles, and continuous enemy spawning.
// Difficulty increases over time with new enemy types introduced at intervals.
//
// Compile (Win32): g++ -std=c++11 -O2 -Wall -Wextra -o geometry.exe geometry.cpp -mwindows
//
// Controls: WASD = Move, Mouse = Aim, Left Click = Shoot, F9 = Invincibility, ESC = Quit

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "../../GameLib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================
// Constants
// ============================================================
#define MAP_W    1200
#define MAP_H    900
#define WIN_W    800
#define WIN_H    600

#define GRID_SPACING 25
#define GRID_COLS    ((MAP_W + GRID_SPACING - 1) / GRID_SPACING + 1)
#define GRID_ROWS    ((MAP_H + GRID_SPACING - 1) / GRID_SPACING + 1)

#define MAX_BULLETS      150
#define MAX_ENEMIES      100
#define MAX_PARTICLES    800

#define PLAYER_SPEED  250.0f
#define BULLET_SPEED  600.0f
#define SHOOT_RATE    0.12f
#define COMBO_TIMEOUT 2.0f

// ============================================================
// Structs
// ============================================================
struct GridPt  { float x, y, vx, vy; };
struct Bullet  { float x, y, vx, vy; bool active; };
struct Enemy   { float x, y, vx, vy; int type; int hp; int maxHp; float r; float speed; bool active; float angle; };
struct Particle{ float x, y, vx, vy; uint32_t color; float life; float maxLife; float sz; };

// ============================================================
// Global State
// ============================================================
static GridPt grid[GRID_ROWS][GRID_COLS];

static float px = MAP_W / 2.0f, py = MAP_H / 2.0f, pAngle = 0.0f;
static float camX = 0, camY = 0;
static Bullet bullets[MAX_BULLETS];
static Enemy enemies[MAX_ENEMIES];
static Particle particles[MAX_PARTICLES];

static enum {
    ST_TITLE = 0, ST_COMBAT, ST_DEATH, ST_GAME_OVER
} gameState = ST_TITLE;

static int score = 0, combo = 0, kills = 0;
static float comboTimer = 0.0f;
static int highestCombo = 1, totalKills = 0;
static float gameTime = 0.0f;  // Survival time in seconds

static bool playerAlive = true;
static bool invincible = false;
static float shakeAmt = 0.0f, shakeX = 0.0f, shakeY = 0.0f;
static int shakeFrames = 0;
static float shootTimer = 0.0f;
static float spawnTimer = 0.0f;  // Enemy spawn timer

static struct { const char *click, *hit, *coin, *explosion, *gameOver, *noteHigh, *victory; } sounds;

// ============================================================
// Helpers
// ============================================================
static float dist(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1, dy = y2 - y1;
    return (float)sqrt(dx * dx + dy * dy);
}
static void clamp(float &v, float lo, float hi) { if (v < lo) v = lo; if (v > hi) v = hi; }

static const char *pathOf(const char *a, const char *b) {
    FILE *f = fopen(a, "rb"); if (f) { fclose(f); return a; }
    f = fopen(b, "rb"); if (f) { fclose(f); return b; }
    return a;
}

// ============================================================
// Grid
// ============================================================
static void gridInit() {
    int cols = MAP_W / GRID_SPACING + 1, rows = MAP_H / GRID_SPACING + 1;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            GridPt &g = grid[r][c];
            g.x = g.vx = (float)(c * GRID_SPACING);
            g.y = g.vy = (float)(r * GRID_SPACING);
        }
    }
}

static void gridUpdate(float dt) {
    int cols = MAP_W / GRID_SPACING + 1, rows = MAP_H / GRID_SPACING + 1;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            GridPt &g = grid[r][c];
            float fx = (float)(c * GRID_SPACING), fy = (float)(r * GRID_SPACING);
            g.vx += (fx - g.x) * 12.0f * dt;
            g.vy += (fy - g.y) * 12.0f * dt;
            float d = 1.0f - 5.0f * dt;
            if (d < 0.85f) d = 0.85f;
            g.vx *= d; g.vy *= d;
            g.x += g.vx * dt * 60.0f;
            g.y += g.vy * dt * 60.0f;
        }
    }
}

static void gridImpulse(float cx, float cy, float radius, float strength) {
    int cols = MAP_W / GRID_SPACING + 1, rows = MAP_H / GRID_SPACING + 1;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            GridPt &g = grid[r][c];
            float dx = g.x - cx, dy = g.y - cy;
            float d = (float)sqrt(dx * dx + dy * dy);
            if (d < radius && d > 0.1f) {
                float f = strength * (1.0f - d / radius);
                g.vx += (dx / d) * f;
                g.vy += (dy / d) * f;
            }
        }
    }
}

static void gridDraw(GameLib &g) {
    int cols = MAP_W / GRID_SPACING + 1, rows = MAP_H / GRID_SPACING + 1;
    uint32_t c = COLOR_ARGB(70, 50, 50, 130);
    for (int r = 0; r < rows; r++) {
        for (int cc = 0; cc < cols; cc++) {
            float x = grid[r][cc].x - camX + shakeX;
            float y = grid[r][cc].y - camY + shakeY;
            if (cc + 1 < cols) {
                float x2 = grid[r][cc + 1].x - camX + shakeX;
                float y2 = grid[r][cc + 1].y - camY + shakeY;
                g.DrawLine((int)x, (int)y, (int)x2, (int)y2, c);
            }
            if (r + 1 < rows) {
                float x2 = grid[r + 1][cc].x - camX + shakeX;
                float y2 = grid[r + 1][cc].y - camY + shakeY;
                g.DrawLine((int)x, (int)y, (int)x2, (int)y2, c);
            }
        }
    }
}

// ============================================================
// Particles
// ============================================================
static void spawnParticle(float x, float y, float vx, float vy, uint32_t color, float life, float sz) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life <= 0) {
            particles[i].x = x; particles[i].y = y;
            particles[i].vx = vx; particles[i].vy = vy;
            particles[i].color = color;
            particles[i].life = life; particles[i].maxLife = life;
            particles[i].sz = sz;
            return;
        }
    }
}

static void spawnExplosion(float x, float y, uint32_t color, int count) {
    for (int i = 0; i < count; i++) {
        float a = (float)(i * 360.0 / count) * (float)M_PI / 180.0f;
        float spd = (float)(80.0 + rand() % 280);
        spawnParticle(x, y, (float)cos(a) * spd, (float)sin(a) * spd, color, 0.4f + (float)rand() / RAND_MAX * 0.8f, 3.0f + (float)(rand() % 4));
    }
}

static void particlesUpdate(float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life > 0) {
            particles[i].life -= dt;
            particles[i].vx *= (1.0f - 4.0f * dt);
            particles[i].vy *= (1.0f - 4.0f * dt);
            particles[i].x += particles[i].vx * dt;
            particles[i].y += particles[i].vy * dt;
            if (particles[i].life < 0) particles[i].life = 0;
        }
    }
}

static void particlesDraw(GameLib &g) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life > 0) {
            Particle &p = particles[i];
            float alpha = p.life / p.maxLife;
            float s = p.sz * alpha;
            if (s > 0.5f && s < 20) {
                int sx = (int)(p.x - camX + shakeX), sy = (int)(p.y - camY + shakeY);
                // Core particle
                g.FillCircle(sx, sy, (int)s, p.color);
                // Glow effect - larger and brighter
                if (s > 1.5f && alpha > 0.2f) {
                    uint32_t ga = (uint32_t)(alpha * 80);
                    g.FillCircle(sx, sy, (int)(s * 2.5f), COLOR_ARGB(ga, COLOR_GET_R(p.color), COLOR_GET_G(p.color), COLOR_GET_B(p.color)));
                }
                // Extra bright core for fresh particles
                if (alpha > 0.6f && s > 2) {
                    uint32_t coreAlpha = (uint32_t)(alpha * 120);
                    g.FillCircle(sx, sy, (int)(s * 1.3f), COLOR_ARGB(coreAlpha, 255, 255, 255));
                }
            }
        }
    }
}

// ============================================================
// Enemies
// ============================================================
static uint32_t enemyColor(int type) {
    switch (type) {
        case 0: return COLOR_ARGB(255, 255, 150, 40);   // orange - Swarm
        case 1: return COLOR_ARGB(255, 255, 80, 140);    // pink - Chaser
        case 2: return COLOR_ARGB(255, 80, 255, 80);     // green - Bouncer
        case 3: return COLOR_ARGB(255, 160, 60, 220);    // purple - Tank
        default: return COLOR_WHITE;
    }
}

static void spawnEnemy(int type, float x, float y) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            Enemy &e = enemies[i];
            e.active = true;
            e.type = type;
            e.x = x; e.y = y;
            e.angle = 0; e.r = 8; e.speed = 80; e.hp = 1; e.maxHp = 1;
            e.vx = e.vy = 0;
            switch (type) {
                case 0: e.r = 10; e.speed = 60; break;
                case 1: e.r = 12; e.speed = 140; break;
                case 2: e.r = 14; e.speed = 100; e.hp = e.maxHp = 2; break;
                case 3: e.r = 22; e.speed = 45; e.hp = e.maxHp = 5; break;
            }
            if (type == 2) {
                float a = (float)rand() / RAND_MAX * (float)(2.0 * M_PI);
                e.vx = (float)cos(a) * e.speed;
                e.vy = (float)sin(a) * e.speed;
            }
            return;
        }
    }
}

// Spawn enemy from map edge, ensuring distance from player
static void spawnFromEdge(int type) {
    float x, y;
    float margin = 80;
    for (int tries = 0; tries < 20; tries++) {
        int side = rand() % 4;
        switch (side) {
            case 0: x = (float)(rand() % MAP_W); y = -margin; break;
            case 1: x = (float)(rand() % MAP_W); y = MAP_H + margin; break;
            case 2: x = -margin; y = (float)(rand() % MAP_H); break;
            default: x = MAP_W + margin; y = (float)(rand() % MAP_H); break;
        }
        clamp(x, -margin, MAP_W + margin);
        clamp(y, -margin, MAP_H + margin);
        if (dist(x, y, px, py) > 250) {
            spawnEnemy(type, x, y);
            return;
        }
    }
}

static void enemiesUpdate(float dt) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        Enemy &e = enemies[i];
        switch (e.type) {
            case 0: { // Swarm - drift toward player
                float dx = px - e.x, dy = py - e.y;
                float d = dist(e.x, e.y, px, py);
                if (d > 1) { e.vx += (dx / d) * e.speed * 2.0f * dt; e.vy += (dy / d) * e.speed * 2.0f * dt; }
                float f = 1.0f - 3.0f * dt; if (f < 0.7f) f = 0.7f;
                e.vx *= f; e.vy *= f;
                break;
            }
            case 1: { // Chaser - steer toward player
                float dx = px - e.x, dy = py - e.y;
                float d = dist(e.x, e.y, px, py);
                float spd = e.speed;
                if (d > 1) { e.vx += (dx / d) * spd * 4.0f * dt; e.vy += (dy / d) * spd * 4.0f * dt; }
                float cs = spd * 1.2f;
                float cv = (float)sqrt(e.vx * e.vx + e.vy * e.vy);
                if (cv > cs) { e.vx = (e.vx / cv) * cs; e.vy = (e.vy / cv) * cs; }
                e.angle = (float)atan2(e.vy, e.vx);
                break;
            }
            case 2: { // Bouncer - linear with wall bounce
                break;
            }
            case 3: { // Tank - slow drift toward player
                float dx = px - e.x, dy = py - e.y;
                float d = dist(e.x, e.y, px, py);
                if (d > 1) { e.vx += (dx / d) * e.speed * 1.5f * dt; e.vy += (dy / d) * e.speed * 1.5f * dt; }
                float f = 1.0f - 2.0f * dt; if (f < 0.8f) f = 0.8f;
                e.vx *= f; e.vy *= f;
                break;
            }
        }
        e.x += e.vx * dt;
        e.y += e.vy * dt;

        // Map boundary
        if (e.type != 2) {
            clamp(e.x, e.r, MAP_W - e.r);
            clamp(e.y, e.r, MAP_H - e.r);
        } else {
            if (e.x < e.r) { e.x = e.r; e.vx = -e.vx; }
            if (e.x > MAP_W - e.r) { e.x = MAP_W - e.r; e.vx = -e.vx; }
            if (e.y < e.r) { e.y = e.r; e.vy = -e.vy; }
            if (e.y > MAP_H - e.r) { e.y = MAP_H - e.r; e.vy = -e.vy; }
        }
    }
}

// ============================================================
// Screen Shake
// ============================================================
static void shake(int amt, int frames) {
    if (amt > shakeAmt) { shakeAmt = (float)amt; shakeFrames = frames; }
}

static void shakeUpdate() {
    if (shakeFrames > 0) {
        float a = shakeAmt * ((float)shakeFrames / 20.0f);
        shakeX = (float)((rand() % 200 - 100) / 100.0) * a;
        shakeY = (float)((rand() % 200 - 100) / 100.0) * a;
        shakeFrames--;
    } else {
        shakeAmt = 0; shakeX = 0; shakeY = 0;
    }
}

// ============================================================
// Camera
// ============================================================
static void cameraUpdate() {
    float tx = px - WIN_W / 2.0f, ty = py - WIN_H / 2.0f;
    clamp(tx, 0, MAP_W - WIN_W);
    clamp(ty, 0, MAP_H - WIN_H);
    camX += (tx - camX) * 0.1f;
    camY += (ty - camY) * 0.1f;
}

// ============================================================
// Drawing Helpers
// ============================================================
static void drawPlayer(GameLib &g) {
    int sx = (int)(px - camX + shakeX), sy = (int)(py - camY + shakeY);
    float a = pAngle;
    uint32_t glow = COLOR_ARGB(60, 0, 255, 255);
    uint32_t core = COLOR_ARGB(240, 0, 255, 255);
    // Glow
    g.FillCircle(sx, sy, 20, glow);
    // Ship shape (pointed toward mouse)
    float verts[6];
    verts[0] = sx + (float)cos(a) * 18;
    verts[1] = sy + (float)sin(a) * 18;
    float ba = a + (float)M_PI;
    verts[2] = sx + (float)cos(ba) * 10 + (float)cos(ba + 1.3f) * 10;
    verts[3] = sy + (float)sin(ba) * 10 + (float)sin(ba + 1.3f) * 10;
    verts[4] = sx + (float)cos(ba) * 10 + (float)cos(ba - 1.3f) * 10;
    verts[5] = sy + (float)sin(ba) * 10 + (float)sin(ba - 1.3f) * 10;
    g.FillTriangle((int)verts[0], (int)verts[1], (int)verts[2], (int)verts[3], (int)verts[4], (int)verts[5], core);
    // Engine glow
    float ex = sx - (float)cos(a) * 12, ey = sy - (float)sin(a) * 12;
    g.FillCircle((int)ex, (int)ey, 4, COLOR_ARGB(150, 255, 200, 50));
}

static void drawBullets(GameLib &g) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        int sx = (int)(bullets[i].x - camX + shakeX), sy = (int)(bullets[i].y - camY + shakeY);
        g.FillCircle(sx, sy, 3, COLOR_ARGB(255, 200, 255, 200));
        g.FillCircle(sx, sy, 6, COLOR_ARGB(100, 100, 255, 100));
        // Trail
        g.FillCircle(sx - (int)(bullets[i].vx * 0.008f), sy - (int)(bullets[i].vy * 0.008f), 2, COLOR_ARGB(80, 100, 200, 100));
    }
}

static void drawEnemies(GameLib &g) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        Enemy &e = enemies[i];
        int sx = (int)(e.x - camX + shakeX), sy = (int)(e.y - camY + shakeY);
        uint32_t c = enemyColor(e.type);
        uint32_t gl = COLOR_ARGB(60, COLOR_GET_R(c), COLOR_GET_G(c), COLOR_GET_B(c));
        switch (e.type) {
            case 0: // Circle
                g.FillCircle(sx, sy, (int)e.r, c);
                g.FillCircle(sx, sy, (int)(e.r * 1.8f), gl);
                break;
            case 1: // Triangle
                for (int j = 0; j < 3; j++) {
                    float a1 = e.angle + j * (float)(2.0f * M_PI / 3);
                    float a2 = e.angle + (j + 1) * (float)(2.0f * M_PI / 3);
                    g.DrawLine(sx + (int)(cos(a1) * e.r), sy + (int)(sin(a1) * e.r),
                               sx + (int)(cos(a2) * e.r), sy + (int)(sin(a2) * e.r), c);
                }
                g.FillCircle(sx, sy, (int)e.r * 2, gl);
                break;
            case 2: // Diamond (rotated square)
                for (int j = 0; j < 4; j++) {
                    float a1 = (float)M_PI / 4 + e.angle + j * (float)(M_PI / 2);
                    float a2 = (float)M_PI / 4 + e.angle + (j + 1) * (float)(M_PI / 2);
                    g.DrawLine(sx + (int)(cos(a1) * e.r), sy + (int)(sin(a1) * e.r),
                               sx + (int)(cos(a2) * e.r), sy + (int)(sin(a2) * e.r), c);
                }
                g.FillCircle(sx, sy, (int)e.r * 1.5f, gl);
                break;
            case 3: // Tank circle with ring
                g.FillCircle(sx, sy, (int)e.r, c);
                g.FillCircle(sx, sy, (int)(e.r * 1.6f), gl);
                g.DrawCircle(sx, sy, (int)(e.r * 0.6f), COLOR_ARGB(200, 220, 120, 255));
                break;
        }
    }
}

static void drawMapBorder(GameLib &g) {
    uint32_t c = COLOR_ARGB(180, 100, 100, 255);
    int x = (int)(-camX + shakeX), y = (int)(-camY + shakeY);
    int w = MAP_W, h = MAP_H;
    g.DrawRect(x, y, w, 3, c);
    g.DrawRect(x, y + h - 3, w, 3, c);
    g.DrawRect(x, y, 3, h, c);
    g.DrawRect(x + w - 3, y, 3, h, c);
}

static void drawHUD(GameLib &g) {
    g.DrawPrintf(10, 10, COLOR_WHITE, "SCORE: %d", score);
    if (combo > 1) {
        g.DrawPrintfScale(WIN_W / 2 - 30, 10, COLOR_YELLOW, 2, "x%d", combo);
    }
    
    // Survival time
    int minutes = (int)gameTime / 60;
    int seconds = (int)gameTime % 60;
    g.DrawPrintf(WIN_W - 100, 10, COLOR_SKY_BLUE, "TIME %d:%02d", minutes, seconds);
}

static void drawTextCentered(GameLib &g, const char *text, int y, uint32_t color, float scale) {
    int tw = (int)strlen(text) * 8 * (int)scale;
    g.DrawTextScale(WIN_W / 2 - tw / 2, y, text, color, scale);
}

// ============================================================
// Game Logic Update
// ============================================================
static void gameUpdate(GameLib &g, float dt) {
    // Player movement
    if (playerAlive) {
        float dx = 0, dy = 0;
        if (g.IsKeyDown(KEY_W) || g.IsKeyDown(KEY_UP)) dy -= 1;
        if (g.IsKeyDown(KEY_S) || g.IsKeyDown(KEY_DOWN)) dy += 1;
        if (g.IsKeyDown(KEY_A) || g.IsKeyDown(KEY_LEFT)) dx -= 1;
        if (g.IsKeyDown(KEY_D) || g.IsKeyDown(KEY_RIGHT)) dx += 1;
        float len = (float)sqrt(dx * dx + dy * dy);
        if (len > 0) { dx /= len; dy /= len; px += dx * PLAYER_SPEED * dt; py += dy * PLAYER_SPEED * dt; }
        clamp(px, 20, MAP_W - 20);
        clamp(py, 20, MAP_H - 20);

        float mx = (float)(g.GetMouseX()) + camX, my = (float)(g.GetMouseY()) + camY;
        float ddx = mx - px, ddy = my - py;
        if (dist(px, py, mx, my) > 5) pAngle = (float)atan2(ddy, ddx);
    }

    // Shooting
    if (playerAlive && gameState == ST_COMBAT) {
        shootTimer += dt;
        if (g.IsMouseDown(MOUSE_LEFT) && shootTimer >= SHOOT_RATE) {
            shootTimer = 0;
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!bullets[i].active) {
                    bullets[i].active = true;
                    bullets[i].x = px + (float)cos(pAngle) * 15;
                    bullets[i].y = py + (float)sin(pAngle) * 15;
                    bullets[i].vx = (float)cos(pAngle) * BULLET_SPEED;
                    bullets[i].vy = (float)sin(pAngle) * BULLET_SPEED;
                    break;
                }
            }
        }
    }

    // Bullets update
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        bullets[i].x += bullets[i].vx * dt;
        bullets[i].y += bullets[i].vy * dt;
        if (bullets[i].x < -20 || bullets[i].x > MAP_W + 20 || bullets[i].y < -20 || bullets[i].y > MAP_H + 20) {
            bullets[i].active = false;
        }
    }

    // Enemies update
    enemiesUpdate(dt);

    // Bullet-enemy collision
    if (playerAlive) {
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!bullets[i].active) continue;
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (!enemies[j].active) continue;
                if (dist(bullets[i].x, bullets[i].y, enemies[j].x, enemies[j].y) < enemies[j].r + 4) {
                    bullets[i].active = false;
                    enemies[j].hp--;
                    if (enemies[j].hp <= 0) {
                        int pts[4] = { 50, 100, 150, 300 };
                        combo++; comboTimer = COMBO_TIMEOUT;
                        if (combo > highestCombo) highestCombo = combo;
                        score += pts[enemies[j].type] * combo;
                        kills++; totalKills++;
                        spawnExplosion(enemies[j].x, enemies[j].y, enemyColor(enemies[j].type), 25 + enemies[j].type * 10);
                        gridImpulse(enemies[j].x, enemies[j].y, 120, 50 + enemies[j].type * 20);
                        shake(1 + enemies[j].type / 2, 3 + enemies[j].type);

                        // Tank splits
                        if (enemies[j].type == 3) {
                            for (int k = 0; k < 3; k++) {
                                float a = (float)(k * 120) * (float)M_PI / 180.0f;
                                spawnEnemy(0, enemies[j].x + (float)cos(a) * 20, enemies[j].y + (float)sin(a) * 20);
                            }
                        }
                        enemies[j].active = false;
                    } else {
                        shake(1, 2);
                    }
                    break;
                }
            }
        }

        // Player-enemy collision (skip if invincible)
        if (!invincible) {
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (!enemies[i].active) continue;
                if (dist(px, py, enemies[i].x, enemies[i].y) < enemies[i].r + 8) {
                    playerAlive = false;
                    gameState = ST_DEATH;
                    for (int j = 0; j < MAX_ENEMIES; j++) enemies[j].active = false;
                    spawnExplosion(px, py, COLOR_WHITE, 80);
                    spawnExplosion(px, py, COLOR_CYAN, 60);
                    spawnExplosion(px, py, COLOR_ARGB(255, 255, 200, 100), 40);
                    gridImpulse(px, py, 500, 500);
                    shake(10, 25);
                    break;
                }
            }
        }
    }

    // Particles update
    particlesUpdate(dt);

    // Grid update
    gridUpdate(dt);

    // Combo timer
    if (comboTimer > 0) { comboTimer -= dt; if (comboTimer <= 0) combo = 1; }

    // Shake update
    shakeUpdate();

    // Continuous enemy spawning
    if (gameState == ST_COMBAT && playerAlive) {
        gameTime += dt;
        
        // Determine max enemy type based on time
        int maxType = 0;
        if (gameTime > 15.0f) maxType = 1;
        if (gameTime > 30.0f) maxType = 2;
        if (gameTime > 60.0f) maxType = 3;
        
        // Spawn rate increases over time
        float spawnInterval = 0.35f;
        if (gameTime > 120.0f) spawnInterval = 0.18f;
        else if (gameTime > 60.0f) spawnInterval = 0.22f;
        else if (gameTime > 30.0f) spawnInterval = 0.28f;
        
        // Max enemies on screen increases over time
        int maxOnScreen = 15;
        if (gameTime > 120.0f) maxOnScreen = 40;
        else if (gameTime > 60.0f) maxOnScreen = 30;
        else if (gameTime > 30.0f) maxOnScreen = 25;
        else if (gameTime > 15.0f) maxOnScreen = 20;
        
        // Count active enemies
        int activeCount = 0;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) activeCount++;
        }
        
        // Spawn new enemies
        spawnTimer += dt;
        if (spawnTimer >= spawnInterval && activeCount < maxOnScreen) {
            spawnTimer = 0;
            int type = rand() % (maxType + 1);
            spawnFromEdge(type);
        }
    }
}

// ============================================================
// Main
// ============================================================
int main() {
    GameLib game;
    game.Open(WIN_W, WIN_H, "Geometry Wars", true);
    game.ShowMouse(true);

    // Resolve sound paths
    sounds.click = pathOf("assets/sound/click.wav", "../assets/sound/click.wav");
    sounds.hit = pathOf("assets/sound/hit.wav", "../assets/sound/hit.wav");
    sounds.coin = pathOf("assets/sound/coin.wav", "../assets/sound/coin.wav");
    sounds.explosion = pathOf("assets/sound/explosion.wav", "../assets/sound/explosion.wav");
    sounds.gameOver = pathOf("assets/sound/game_over.wav", "../assets/sound/game_over.wav");
    sounds.noteHigh = pathOf("assets/sound/note_do_high.wav", "../assets/sound/note_do_high.wav");
    sounds.victory = pathOf("assets/sound/victory.wav", "../assets/sound/victory.wav");

    // Init
    gridInit();
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].life = 0;

    while (!game.IsClosed()) {
        double dt = game.GetDeltaTime();
        if (dt > 0.05) dt = 0.05;
        float fdt = (float)dt;

        if (game.IsKeyPressed(KEY_ESCAPE)) break;

        // Debug hotkey: F9 = Invincibility
        if (game.IsKeyPressed(KEY_F9)) {
            invincible = !invincible;
        }

        // ---- State machine ----
        switch (gameState) {
            case ST_TITLE: {
                game.Clear(COLOR_BLACK);
                gridUpdate(fdt);
                gridDraw(game);

                // Title text
                float pulse = (float)sin(game.GetTime() * 2) * 0.3f + 0.7f;
                uint32_t tc = COLOR_ARGB((uint32_t)(pulse * 255), 0, 255, 255);
                const char *title = "GEOMETRY WARS";
                int tw = (int)strlen(title) * 8 * 3;
                game.DrawTextScale(WIN_W / 2 - tw / 2, WIN_H / 2 - 40, title, tc, 3);

                // Blink "PRESS ENTER"
                if ((int)(game.GetTime() * 2) % 2 == 0) {
                    drawTextCentered(game, "PRESS ENTER TO START", WIN_H / 2 + 20, COLOR_LIGHT_GRAY, 1);
                }

                game.DrawText(WIN_W / 2 - 90, WIN_H / 2 + 50, "WASD:Move  Mouse:Aim  Click:Shoot", COLOR_DARK_GRAY);

                if (game.IsKeyPressed(KEY_ENTER)) {
                    score = 0; combo = 1; kills = 0;
                    totalKills = 0; highestCombo = 1; comboTimer = 0;
                    gameTime = 0.0f; spawnTimer = 0.0f;
                    px = MAP_W / 2.0f; py = MAP_H / 2.0f;
                    camX = px - WIN_W / 2.0f; camY = py - WIN_H / 2.0f;
                    playerAlive = true;
                    invincible = false;
                    gridInit();
                    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
                    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
                    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].life = 0;
                    gameState = ST_COMBAT;
                    game.PlayWAV(sounds.noteHigh);
                }
                break;
            }

            case ST_COMBAT: {
                gameUpdate(game, fdt);
                game.Clear(COLOR_BLACK);
                gridDraw(game);
                drawMapBorder(game);
                drawBullets(game);
                drawEnemies(game);
                if (playerAlive) drawPlayer(game);
                particlesDraw(game);
                drawHUD(game);
                break;
            }

            case ST_DEATH: {
                static float deathTimer = 0;
                deathTimer += fdt;
                particlesUpdate(fdt);
                gridUpdate(fdt);
                shakeUpdate();

                game.Clear(COLOR_BLACK);
                gridDraw(game);
                drawMapBorder(game);
                particlesDraw(game);

                // White flash
                if (deathTimer < 0.5f) {
                    game.FillRect(0, 0, WIN_W, WIN_H, COLOR_ARGB((int)((0.5f - deathTimer) * 400), 255, 255, 255));
                }

                if (deathTimer < 1.0f && (int)(deathTimer * 2) != (int)((deathTimer - fdt) * 2)) {
                    game.PlayWAV(sounds.explosion);
                }

                if (deathTimer > 1.5f) {
                    gameState = ST_GAME_OVER;
                    deathTimer = 0;
                    game.PlayWAV(sounds.gameOver);
                }
                break;
            }

            case ST_GAME_OVER: {
                static float goTimer = 0;
                goTimer += fdt;
                gridUpdate(fdt);
                particlesUpdate(fdt);

                game.Clear(COLOR_BLACK);
                gridDraw(game);
                particlesDraw(game);

                drawTextCentered(game, "GAME OVER", WIN_H / 2 - 70, COLOR_RED, 3);

                char buf[64];
                sprintf(buf, "FINAL SCORE: %d", score);
                drawTextCentered(game, buf, WIN_H / 2 - 10, COLOR_WHITE, 1);
                
                int minutes = (int)gameTime / 60;
                int seconds = (int)gameTime % 60;
                sprintf(buf, "SURVIVED: %d:%02d", minutes, seconds);
                drawTextCentered(game, buf, WIN_H / 2 + 15, COLOR_WHITE, 1);
                
                sprintf(buf, "TOTAL KILLS: %d", totalKills);
                drawTextCentered(game, buf, WIN_H / 2 + 40, COLOR_WHITE, 1);
                sprintf(buf, "MAX COMBO: x%d", highestCombo);
                drawTextCentered(game, buf, WIN_H / 2 + 65, COLOR_YELLOW, 1);

                if (goTimer > 2.0f) {
                    if ((int)(game.GetTime() * 2) % 2 == 0) {
                        drawTextCentered(game, "PRESS R TO RESTART", WIN_H / 2 + 110, COLOR_LIGHT_GRAY, 1);
                    }
                    if (game.IsKeyPressed(KEY_R)) {
                        gameState = ST_TITLE;
                        goTimer = 0;
                        gridInit();
                        for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
                        for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
                        for (int i = 0; i < MAX_PARTICLES; i++) particles[i].life = 0;
                        playerAlive = true;
                        px = MAP_W / 2.0f; py = MAP_H / 2.0f;
                        camX = px - WIN_W / 2.0f; camY = py - WIN_H / 2.0f;
                        shakeAmt = 0; shakeX = 0; shakeY = 0; shakeFrames = 0;
                    }
                }
                break;
            }
        }

        // Camera always updates (except title)
        if (gameState != ST_TITLE) cameraUpdate();

        game.Update();
    }

    return 0;
}
