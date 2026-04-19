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
#include "../GameLib.h"
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
#define MAX_FLOAT_TEXTS  30       // floating score texts
#define MAX_POWERUPS     5        // max powerups on screen

#define PLAYER_SPEED  250.0f
#define BULLET_SPEED  810.0f
#define SHOOT_RATE    0.12f
#define COMBO_TIMEOUT 2.0f

#define MAX_STARS_FAR   50
#define MAX_STARS_NEAR  30
#define ENERGY_DURATION      5.0f
#define ENERGY_SHOOT_RATE    0.08f
#define POPUP_LIFE      2.0f
#define POPUP_GROW_TIME 0.2f
#define POPUP_FADE_TIME 0.5f
#define MAX_LIVES           3
#define RESPAWN_INVINCIBLE  2.0f
#define SPAWN_CLEAR_RADIUS  150.0f
#define LB_SIZE            10

// ============================================================
// Structs
// ============================================================
struct GridPt  { float x, y, vx, vy; };
struct Bullet  { float x, y, vx, vy; bool active; };
struct Enemy   { float x, y, vx, vy; int type; int hp; int maxHp; float r; float speed; bool active; float angle; };
struct Particle{ float x, y, vx, vy; uint32_t color; float life; float maxLife; float sz; };
struct FloatText { float x, y, vy; char text[16]; uint32_t color; float life; float maxLife; };
struct PowerUp { float x, y, r; float life; float pulse; bool active; int type; }; // type 0 = nuke, 1 = energy
struct Star { float x, y; uint32_t color; float drift; float sz; };
struct Popup { char text[32]; uint32_t color; float life; float maxLife; int scale; };
struct LBEntry { int score; float time; int kills; int combo; };

// ============================================================
// Global State
// ============================================================

// Scene IDs (managed by GameLib.h SetScene)
#define SCENE_TITLE     1
#define SCENE_COMBAT    2
#define SCENE_DEATH     3
#define SCENE_GAME_OVER 4
#define SCENE_LEADERBOARD 5

// -- Player --
static float px = MAP_W / 2.0f, py = MAP_H / 2.0f, pAngle = 0.0f;
static bool playerAlive = true;
static bool invincible = false;
static int   lives = MAX_LIVES;
static float respawnTimer = 0.0f;
static bool  respawnInvincible = false;
static int killedByType = -1;

// -- Combat --
static int score = 0, combo = 0, kills = 0;
static int highestCombo = 1, totalKills = 0;
static float comboTimer = 0.0f;
static float gameTime = 0.0f;  // Survival time in seconds
static float shootTimer = 0.0f;
static float spawnTimer = 0.0f;  // Enemy spawn timer
static float powerupSpawnTimer = 0.0f;  // Powerup spawn timer

// -- Effects --
static float shakeAmt = 0.0f, shakeX = 0.0f, shakeY = 0.0f;
static int shakeFrames = 0;
static float energyTimer = 0.0f;
static bool  energyActive = false;

// -- Nuke FX --
static float nukeFlashAlpha = 0.0f;   // white flash overlay (1.0 → 0.0)
static float nukeWaveRadius = 0.0f;   // shockwave circle radius (0 → >screen)
static float nukeWaveAlpha = 0.0f;    // shockwave circle alpha (1.0 → 0.0)
static bool  nukeFxActive = false;
static Popup popup;
static bool combo5Shown = false, combo10Shown = false;
static bool kills50Shown = false, kills100Shown = false, kills200Shown = false;

// -- Camera --
static float camX = 0, camY = 0;

// -- Leaderboard --
static int lbHighlight = -1;

// -- Records --
static int bestScore = 0;
static float bestTime = 0.0f;
static const char *SAVE_FILE = "geometry.sav";

// -- Pools --
static GridPt grid[GRID_ROWS][GRID_COLS];
static Bullet bullets[MAX_BULLETS];
static Enemy enemies[MAX_ENEMIES];
static Particle particles[MAX_PARTICLES];
static FloatText floatTexts[MAX_FLOAT_TEXTS];
static PowerUp powerups[MAX_POWERUPS];
static Star starsFar[MAX_STARS_FAR];
static Star starsNear[MAX_STARS_NEAR];
static LBEntry leaderboard[LB_SIZE];

// -- Sound --
static struct {
    const char *shoot[4];    // shoot-01.wav ~ shoot-04.wav
    const char *explosion[8]; // explosion-01.wav ~ explosion-08.wav
    const char *spawn[8];    // spawn-01.wav ~ spawn-08.wav
    const char *death;       // player death (use explosion variant)
    const char *gameOver;    // game over screen
    const char *noteHigh;    // start game prompt
} sounds;

// ============================================================
// Helpers
// ============================================================
static float dist(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1, dy = y2 - y1;
    return (float)sqrt(dx * dx + dy * dy);
}
static void clamp(float &v, float lo, float hi) { if (v < lo) v = lo; if (v > hi) v = hi; }

static const char *enemyTypeName(int type) {
    switch (type) {
        case 0: return "SWARM";
        case 1: return "CHASER";
        case 2: return "BOUNCER";
        case 3: return "TANK";
        default: return "UNKNOWN";
    }
}

static uint32_t enemyColor(int type) {
    switch (type) {
        case 0: return COLOR_ARGB(255, 255, 150, 40);   // orange - Swarm
        case 1: return COLOR_ARGB(255, 255, 80, 140);    // pink - Chaser
        case 2: return COLOR_ARGB(255, 80, 255, 80);     // green - Bouncer
        case 3: return COLOR_ARGB(255, 160, 60, 220);    // purple - Tank
        default: return COLOR_WHITE;
    }
}

static const char *pickRandom(const char *variants[], int count) {
    return variants[rand() % count];
}

// Forward declarations (needed because resetGame and triggerNuke call later-defined functions)
static void gridInit();
static void starsInit();
static void shake(int amt, int frames);
static void showPopup(const char *text, uint32_t color, int scale);
static void spawnExplosion(float x, float y, uint32_t color, int count);
static void spawnFloatText(float x, float y, const char *text, uint32_t color);
static void spawnPowerUp(float x, float y, int type);

// ============================================================
// Init & Reset
// ============================================================
static void resetGame() {
    score = 0; combo = 1; kills = 0;
    totalKills = 0; highestCombo = 1; comboTimer = 0;
    gameTime = 0.0f; spawnTimer = 0.0f; powerupSpawnTimer = 0.0f;
    px = MAP_W / 2.0f; py = MAP_H / 2.0f;
    camX = px - WIN_W / 2.0f; camY = py - WIN_H / 2.0f;
    playerAlive = true;
    invincible = false;
    lives = MAX_LIVES;
    respawnInvincible = false;
    respawnTimer = 0.0f;
    energyActive = false;
    energyTimer = 0.0f;
    nukeFxActive = false;
    nukeFlashAlpha = 0.0f;
    nukeWaveRadius = 0.0f;
    nukeWaveAlpha = 0.0f;
    killedByType = -1;
    combo5Shown = false; combo10Shown = false;
    kills50Shown = false; kills100Shown = false; kills200Shown = false;
    popup.life = 0;
    lbHighlight = -1;
    gridInit();
    starsInit();
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].life = 0;
    for (int i = 0; i < MAX_FLOAT_TEXTS; i++) floatTexts[i].life = 0;
    for (int i = 0; i < MAX_POWERUPS; i++) powerups[i].active = false;
}

// ============================================================
// Starfield
// ============================================================
static void starsInit() {
    for (int i = 0; i < MAX_STARS_FAR; i++) {
        Star &s = starsFar[i];
        s.x = (float)(rand() % MAP_W);
        s.y = (float)(rand() % MAP_H);
        int colorType = rand() % 3;
        int a = 30 + rand() % 30;
        if (colorType == 0) s.color = COLOR_ARGB(a, 200, 200, 255);
        else if (colorType == 1) s.color = COLOR_ARGB(a, 180, 220, 255);
        else s.color = COLOR_ARGB(a, 220, 220, 220);
        s.drift = (float)(5 + rand() % 5);
        s.sz = 1.0f + (float)(rand() % 2);
    }
    for (int i = 0; i < MAX_STARS_NEAR; i++) {
        Star &s = starsNear[i];
        s.x = (float)(rand() % MAP_W);
        s.y = (float)(rand() % MAP_H);
        int colorType = rand() % 3;
        int a = 60 + rand() % 40;
        if (colorType == 0) s.color = COLOR_ARGB(a, 200, 200, 255);
        else if (colorType == 1) s.color = COLOR_ARGB(a, 180, 220, 255);
        else s.color = COLOR_ARGB(a, 220, 220, 220);
        s.drift = (float)(15 + rand() % 10);
        s.sz = 2.0f + (float)(rand() % 2);
    }
}

static void starsUpdate(float dt) {
    for (int i = 0; i < MAX_STARS_FAR; i++) {
        starsFar[i].y += starsFar[i].drift * dt;
        if (starsFar[i].y > MAP_H) { starsFar[i].y = 0; starsFar[i].x = (float)(rand() % MAP_W); }
    }
    for (int i = 0; i < MAX_STARS_NEAR; i++) {
        starsNear[i].y += starsNear[i].drift * dt;
        if (starsNear[i].y > MAP_H) { starsNear[i].y = 0; starsNear[i].x = (float)(rand() % MAP_W); }
    }
}

static void starsDraw(GameLib &g) {
    for (int i = 0; i < MAX_STARS_FAR; i++) {
        int sx = (int)(starsFar[i].x - camX + shakeX);
        int sy = (int)(starsFar[i].y - camY + shakeY);
        g.FillCircle(sx, sy, (int)starsFar[i].sz, starsFar[i].color);
    }
    for (int i = 0; i < MAX_STARS_NEAR; i++) {
        int sx = (int)(starsNear[i].x - camX + shakeX);
        int sy = (int)(starsNear[i].y - camY + shakeY);
        g.FillCircle(sx, sy, (int)starsNear[i].sz, starsNear[i].color);
    }
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
                g.FillCircle(sx, sy, (int)s, p.color);
                if (s > 1.5f && alpha > 0.2f) {
                    uint32_t ga = (uint32_t)(alpha * 80);
                    g.FillCircle(sx, sy, (int)(s * 2.5f), COLOR_ARGB(ga, COLOR_GET_R(p.color), COLOR_GET_G(p.color), COLOR_GET_B(p.color)));
                }
                if (alpha > 0.6f && s > 2) {
                    uint32_t coreAlpha = (uint32_t)(alpha * 120);
                    g.FillCircle(sx, sy, (int)(s * 1.3f), COLOR_ARGB(coreAlpha, 255, 255, 255));
                }
            }
        }
    }
}

// ============================================================
// Floating Score Texts
// ============================================================
static void spawnFloatText(float x, float y, const char *text, uint32_t color) {
    for (int i = 0; i < MAX_FLOAT_TEXTS; i++) {
        if (floatTexts[i].life <= 0) {
            FloatText &ft = floatTexts[i];
            ft.x = x; ft.y = y;
            ft.vy = -60.0f;
            strncpy(ft.text, text, sizeof(ft.text) - 1);
            ft.text[sizeof(ft.text) - 1] = '\0';
            ft.color = color;
            ft.life = ft.maxLife = 1.0f;
            return;
        }
    }
}

static void floatTextsUpdate(float dt) {
    for (int i = 0; i < MAX_FLOAT_TEXTS; i++) {
        if (floatTexts[i].life > 0) {
            floatTexts[i].life -= dt;
            floatTexts[i].y += floatTexts[i].vy * dt;
            if (floatTexts[i].life < 0) floatTexts[i].life = 0;
        }
    }
}

static void floatTextsDraw(GameLib &g) {
    for (int i = 0; i < MAX_FLOAT_TEXTS; i++) {
        if (floatTexts[i].life > 0) {
            FloatText &ft = floatTexts[i];
            float alpha = ft.life / ft.maxLife;
            int sx = (int)(ft.x - camX + shakeX);
            int sy = (int)(ft.y - camY + shakeY);
            uint32_t c = COLOR_ARGB((uint32_t)(alpha * 255), COLOR_GET_R(ft.color), COLOR_GET_G(ft.color), COLOR_GET_B(ft.color));
            g.DrawTextScale(sx, sy, ft.text, c, 1.2f);
        }
    }
}

// ============================================================
// PowerUps (includes triggerNuke)
// ============================================================
static void spawnPowerUp(float x, float y, int type) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!powerups[i].active) {
            PowerUp &p = powerups[i];
            p.x = x; p.y = y; p.r = 15; p.life = 8.0f; p.pulse = 0; p.active = true; p.type = type;
            return;
        }
    }
}

static void powerupsUpdate(float dt) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!powerups[i].active) continue;
        PowerUp &p = powerups[i];
        p.life -= dt;
        p.pulse += dt * 5.0f;
        if (p.life <= 0) { p.active = false; }
    }
}

static void powerupsDraw(GameLib &g) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!powerups[i].active) continue;
        PowerUp &p = powerups[i];
        int sx = (int)(p.x - camX + shakeX);
        int sy = (int)(p.y - camY + shakeY);
        float pulseScale = (float)sin(p.pulse) * 0.2f + 1.0f;
        int pr = (int)(p.r * pulseScale);

        uint32_t core, glow;
        const char *label;
        if (p.type == 0) {
            core = COLOR_ARGB(255, 0, 255, 255);
            glow = COLOR_ARGB(80, 0, 200, 255);
            label = "NUKE";
        } else {
            core = COLOR_ARGB(255, 255, 200, 50);
            glow = COLOR_ARGB(80, 255, 150, 0);
            label = "ENERGY";
        }
        // Beacon rings: 2 outward-expanding pulse circles
        float beaconPhase1 = (float)fmod(p.pulse * 0.8f, 2.0f);
        float beaconPhase2 = (float)fmod(p.pulse * 0.8f + 1.0f, 2.0f);
        if (beaconPhase1 < 1.5f) {
            float expand1 = beaconPhase1 / 1.5f;
            int ringR1 = (int)(p.r + expand1 * 30);
            int ringA1 = (int)(120 * (1.0f - expand1));
            g.DrawCircle(sx, sy, ringR1, COLOR_ARGB(ringA1, COLOR_GET_R(core), COLOR_GET_G(core), COLOR_GET_B(core)));
        }
        if (beaconPhase2 < 1.5f) {
            float expand2 = beaconPhase2 / 1.5f;
            int ringR2 = (int)(p.r + expand2 * 30);
            int ringA2 = (int)(120 * (1.0f - expand2));
            g.DrawCircle(sx, sy, ringR2, COLOR_ARGB(ringA2, COLOR_GET_R(core), COLOR_GET_G(core), COLOR_GET_B(core)));
        }
        g.FillCircle(sx, sy, pr * 2, glow);
        int pts[8] = {
            sx, sy - pr,
            sx + pr, sy,
            sx, sy + pr,
            sx - pr, sy
        };
        g.FillTriangle(pts[0], pts[1], pts[2], pts[3], pts[4], pts[5], core);
        g.FillTriangle(pts[2], pts[3], pts[4], pts[5], pts[6], pts[7], core);
        // Label text above the diamond
        g.DrawText(sx - (int)(strlen(label) * 3), sy - pr - 10, label, COLOR_ARGB(180, COLOR_GET_R(core), COLOR_GET_G(core), COLOR_GET_B(core)));

        if (p.life < 3.0f && (int)(p.pulse * 2) % 2 == 0) {
            g.FillCircle(sx, sy, pr * 2, COLOR_ARGB(40, 255, 255, 255));
        }
    }
}

static void triggerNuke(GameLib &g) {
    showPopup("NUKE ACTIVATED!", COLOR_CYAN, 3);
    g.PlayWAV(pickRandom(sounds.explosion, 8));
    shake(8, 20);
    gridImpulse(px, py, 600, 400);

    // Nuke visual FX: white flash + shockwave circle
    nukeFlashAlpha = 1.0f;
    nukeWaveRadius = 0.0f;
    nukeWaveAlpha = 1.0f;
    nukeFxActive = true;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        Enemy &e = enemies[i];
        int pts[4] = { 50, 100, 150, 300 };
        int earned = pts[e.type];
        score += earned;
        kills++; totalKills++;
        spawnExplosion(e.x, e.y, enemyColor(e.type), 20 + e.type * 8);
        char buf[16];
        sprintf(buf, "+%d", earned);
        spawnFloatText(e.x, e.y - 10, buf, COLOR_YELLOW);
        e.active = false;
    }
    spawnExplosion(px, py, COLOR_ARGB(255, 255, 255, 200), 50);
}

// ============================================================
// Enemies
// ============================================================
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
                e.angle = (float)rand() / RAND_MAX * (float)(2.0 * M_PI);
            }
            return;
        }
    }
}

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
            case 2: { // Bouncer - linear with wall bounce, constant counter-clockwise rotation
                e.angle -= (float)(4.0 * M_PI) * dt;
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
// Achievement Popup
// ============================================================
static void showPopup(const char *text, uint32_t color, int scale) {
    strncpy(popup.text, text, sizeof(popup.text) - 1);
    popup.text[sizeof(popup.text) - 1] = '\0';
    popup.color = color;
    popup.life = POPUP_LIFE;
    popup.maxLife = POPUP_LIFE;
    popup.scale = scale;
}

static void popupUpdate(float dt) {
    if (popup.life > 0) {
        popup.life -= dt;
        if (popup.life < 0) popup.life = 0;
    }
}

static void popupDraw(GameLib &g) {
    if (popup.life <= 0) return;
    float elapsed = popup.maxLife - popup.life;
    int currentScale;
    if (elapsed < POPUP_GROW_TIME) {
        currentScale = 1 + (int)((elapsed / POPUP_GROW_TIME) * (popup.scale - 1));
        if (currentScale < 1) currentScale = 1;
    } else {
        currentScale = popup.scale;
    }
    uint32_t drawColor = popup.color;
    if (popup.life < POPUP_FADE_TIME) {
        float alpha = popup.life / POPUP_FADE_TIME;
        drawColor = COLOR_ARGB((uint32_t)(alpha * 255), COLOR_GET_R(popup.color), COLOR_GET_G(popup.color), COLOR_GET_B(popup.color));
    }
    int tw = (int)strlen(popup.text) * 8 * currentScale;
    g.DrawTextScale(WIN_W / 2 - tw / 2, WIN_H / 2 - 20, popup.text, drawColor, currentScale);
}

// ============================================================
// Leaderboard
// ============================================================
static void lbLoad() {
    for (int i = 0; i < LB_SIZE; i++) {
        char key[32];
        sprintf(key, "lb_score%d", i);
        leaderboard[i].score = GameLib::LoadInt(SAVE_FILE, key, 0);
        sprintf(key, "lb_time%d", i);
        leaderboard[i].time = GameLib::LoadFloat(SAVE_FILE, key, 0.0f);
        sprintf(key, "lb_kills%d", i);
        leaderboard[i].kills = GameLib::LoadInt(SAVE_FILE, key, 0);
        sprintf(key, "lb_combo%d", i);
        leaderboard[i].combo = GameLib::LoadInt(SAVE_FILE, key, 0);
    }
}

static void lbSave() {
    for (int i = 0; i < LB_SIZE; i++) {
        char key[32];
        sprintf(key, "lb_score%d", i);
        GameLib::SaveInt(SAVE_FILE, key, leaderboard[i].score);
        sprintf(key, "lb_time%d", i);
        GameLib::SaveFloat(SAVE_FILE, key, leaderboard[i].time);
        sprintf(key, "lb_kills%d", i);
        GameLib::SaveInt(SAVE_FILE, key, leaderboard[i].kills);
        sprintf(key, "lb_combo%d", i);
        GameLib::SaveInt(SAVE_FILE, key, leaderboard[i].combo);
    }
}

static int lbInsert(int newScore, float newTime, int newKills, int newCombo) {
    int pos = -1;
    for (int i = 0; i < LB_SIZE; i++) {
        if (newScore > leaderboard[i].score) { pos = i; break; }
    }
    if (pos < 0) return -1;
    for (int i = LB_SIZE - 1; i > pos; i--) {
        leaderboard[i] = leaderboard[i - 1];
    }
    leaderboard[pos].score = newScore;
    leaderboard[pos].time = newTime;
    leaderboard[pos].kills = newKills;
    leaderboard[pos].combo = newCombo;
    lbSave();
    return pos;
}

static void lbDraw(GameLib &g) {
    const char *title = "LEADERBOARD";
    int tw = (int)strlen(title) * 8 * 3;
    g.DrawTextScale(WIN_W / 2 - tw / 2, 30, title, COLOR_CYAN, 3);
    int y = 80;
    g.DrawText(60, y, "RANK", COLOR_LIGHT_GRAY);
    g.DrawText(120, y, "SCORE", COLOR_LIGHT_GRAY);
    g.DrawText(260, y, "KILLS", COLOR_LIGHT_GRAY);
    g.DrawText(380, y, "TIME", COLOR_LIGHT_GRAY);
    y += 25;
    for (int i = 0; i < LB_SIZE; i++) {
        if (leaderboard[i].score == 0) continue;
        uint32_t rowColor = (i == lbHighlight) ? COLOR_GOLD : COLOR_WHITE;
        g.DrawPrintf(60, y, rowColor, "#%d", i + 1);
        g.DrawPrintf(120, y, rowColor, "%d", leaderboard[i].score);
        g.DrawPrintf(260, y, rowColor, "%d", leaderboard[i].kills);
        int m = (int)leaderboard[i].time / 60;
        int s = (int)leaderboard[i].time % 60;
        g.DrawPrintf(380, y, rowColor, "%d:%02d", m, s);
        y += 22;
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
// Rendering
// ============================================================
static void drawTextCentered(GameLib &g, const char *text, int y, uint32_t color, float scale) {
    int tw = (int)strlen(text) * 8 * (int)scale;
    g.DrawTextScale(WIN_W / 2 - tw / 2, y, text, color, scale);
}

static void drawPlayer(GameLib &g) {
    if (respawnInvincible && (int)(g.GetTime() * 8) % 2 == 0) return;
    int sx = (int)(px - camX + shakeX), sy = (int)(py - camY + shakeY);
    float a = pAngle;
    uint32_t glow = COLOR_ARGB(60, 0, 255, 255);
    uint32_t core = COLOR_ARGB(240, 0, 255, 255);
    g.FillCircle(sx, sy, 20, glow);
    if (energyActive) {
        g.FillCircle(sx, sy, 25, COLOR_ARGB(80, 255, 200, 50));
    }
    float verts[6];
    verts[0] = sx + (float)cos(a) * 18;
    verts[1] = sy + (float)sin(a) * 18;
    float ba = a + (float)M_PI;
    verts[2] = sx + (float)cos(ba) * 10 + (float)cos(ba + 1.3f) * 10;
    verts[3] = sy + (float)sin(ba) * 10 + (float)sin(ba + 1.3f) * 10;
    verts[4] = sx + (float)cos(ba) * 10 + (float)cos(ba - 1.3f) * 10;
    verts[5] = sy + (float)sin(ba) * 10 + (float)sin(ba - 1.3f) * 10;
    g.FillTriangle((int)verts[0], (int)verts[1], (int)verts[2], (int)verts[3], (int)verts[4], (int)verts[5], core);
    float ex = sx - (float)cos(a) * 12, ey = sy - (float)sin(a) * 12;
    g.FillCircle((int)ex, (int)ey, 4, COLOR_ARGB(150, 255, 200, 50));
}

static void drawBullets(GameLib &g) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        int sx = (int)(bullets[i].x - camX + shakeX), sy = (int)(bullets[i].y - camY + shakeY);
        g.FillCircle(sx, sy, 3, COLOR_ARGB(255, 200, 255, 200));
        g.FillCircle(sx, sy, 6, COLOR_ARGB(100, 100, 255, 100));
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

    g.DrawPrintfScale(WIN_W / 2 - 80, 8, COLOR_GOLD, 1, "BEST: %d", bestScore);

    if (combo > 1) {
        g.DrawPrintfScale(WIN_W / 2 - 30, 30, COLOR_YELLOW, 2, "x%d", combo);
    }

    int minutes = (int)gameTime / 60;
    int seconds = (int)gameTime % 60;
    g.DrawPrintf(WIN_W - 100, 10, COLOR_SKY_BLUE, "TIME %d:%02d", minutes, seconds);

    g.DrawPrintf(10, WIN_H - 20, COLOR_WHITE, "LIVES: %d", lives);

    g.DrawPrintf(WIN_W - 60, WIN_H - 20, COLOR_WHITE, "%.0f FPS", g.GetFPS());

    if (energyActive) {
        float ratio = energyTimer / ENERGY_DURATION;
        int barW = 80, barH = 6;
        int barX = WIN_W / 2 - barW / 2, barY = 50;
        g.FillRect(barX, barY, (int)(barW * ratio), barH, COLOR_ARGB(200, 255, 200, 50));
        g.DrawRect(barX, barY, barW, barH, COLOR_ARGB(150, 255, 150, 0));
    }
}

// ============================================================
// Game Logic Update (split into sub-functions)
// ============================================================
static void updatePlayer(GameLib &g, float dt) {
    if (!playerAlive) return;
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

static void updateShooting(GameLib &g, float dt) {
    if (!playerAlive || g.GetScene() != SCENE_COMBAT) return;
    shootTimer += dt;
    float rate = energyActive ? ENERGY_SHOOT_RATE : SHOOT_RATE;
    if (g.IsMouseDown(MOUSE_LEFT) && shootTimer >= rate) {
        shootTimer = 0;
        int bulletCount = energyActive ? 5 : 1;
        float spreadAngles[] = { -15.0f, -7.0f, 0.0f, 7.0f, 15.0f };
        for (int b = 0; b < bulletCount; b++) {
            float angle = pAngle;
            if (energyActive) angle += spreadAngles[b] * (float)M_PI / 180.0f;
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!bullets[i].active) {
                    bullets[i].active = true;
                    bullets[i].x = px + (float)cos(angle) * 15;
                    bullets[i].y = py + (float)sin(angle) * 15;
                    bullets[i].vx = (float)cos(angle) * BULLET_SPEED;
                    bullets[i].vy = (float)sin(angle) * BULLET_SPEED;
                    break;
                }
            }
        }
        g.PlayWAV(pickRandom(sounds.shoot, 4));
    }
}

static void updateBullets(float dt) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        bullets[i].x += bullets[i].vx * dt;
        bullets[i].y += bullets[i].vy * dt;
        if (bullets[i].x < -20 || bullets[i].x > MAP_W + 20 || bullets[i].y < -20 || bullets[i].y > MAP_H + 20) {
            bullets[i].active = false;
        }
    }
}

static void updateCollisions(GameLib &g) {
    if (!playerAlive) return;

    // Bullet-enemy collision
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!enemies[j].active) continue;
            if (dist(bullets[i].x, bullets[i].y, enemies[j].x, enemies[j].y) < enemies[j].r + 4) {
                float bulletAngle = (float)atan2(bullets[i].vy, bullets[i].vx);
                int sparkCount = 5 + rand() % 4;
                for (int s = 0; s < sparkCount; s++) {
                    float spreadAngle = bulletAngle + (float)M_PI + (float)((rand() % 100 - 50) * M_PI / 180.0f);
                    float spd = (float)(150.0 + rand() % 250);
                    uint32_t sparkColor = COLOR_ARGB(255, 255, 255, 200);
                    spawnParticle(bullets[i].x, bullets[i].y,
                                  (float)cos(spreadAngle) * spd,
                                  (float)sin(spreadAngle) * spd,
                                  sparkColor,
                                  0.3f + (float)rand() / RAND_MAX * 0.4f,
                                  3.0f + (float)(rand() % 4));
                }

                bullets[i].active = false;
                enemies[j].hp--;
                if (enemies[j].hp <= 0) {
                    int pts[4] = { 50, 100, 150, 300 };
                    combo++; comboTimer = COMBO_TIMEOUT;
                    if (combo > highestCombo) highestCombo = combo;
                    int earned = pts[enemies[j].type] * combo;
                    score += earned;
                    kills++; totalKills++;
                    spawnExplosion(enemies[j].x, enemies[j].y, enemyColor(enemies[j].type), 25 + enemies[j].type * 10);
                    gridImpulse(enemies[j].x, enemies[j].y, 120, 50 + enemies[j].type * 20);
                    shake(1 + enemies[j].type / 2, 3 + enemies[j].type);
                    g.PlayWAV(pickRandom(sounds.explosion, 8));

                    char buf[16];
                    sprintf(buf, "+%d", earned);
                    spawnFloatText(enemies[j].x, enemies[j].y - 10, buf, COLOR_YELLOW);

                    int baseDrop[] = { 7, 10, 12, 17 };
                    int activeEnemies = 0;
                    for (int k = 0; k < MAX_ENEMIES; k++) if (enemies[k].active) activeEnemies++;
                    float dropScale = 1.0f;
                    if (activeEnemies > 15) {
                        dropScale -= (float)(activeEnemies - 15) / 50.0f;
                        if (dropScale < 0.3f) dropScale = 0.3f;
                    }
                    int dropPct = (int)(baseDrop[enemies[j].type] * dropScale);
                    if (dropPct > 0) {
                        bool nearEnergy = false;
                        for (int k = 0; k < MAX_POWERUPS; k++) {
                            if (powerups[k].active && powerups[k].type == 1 &&
                                dist(enemies[j].x, enemies[j].y, powerups[k].x, powerups[k].y) < 100.0f) {
                                nearEnergy = true; break;
                            }
                        }
                        if (nearEnergy) dropPct /= 2;
                        if (rand() % 100 < dropPct) {
                            spawnPowerUp(enemies[j].x, enemies[j].y, 1);
                        }
                    }

                    if (combo >= 5 && !combo5Shown) { combo5Shown = true; showPopup("x5 COMBO!", COLOR_YELLOW, 3); shake(4, 10); }
                    if (combo >= 10 && !combo10Shown) { combo10Shown = true; showPopup("x10 COMBO!", COLOR_ARGB(255, 255, 200, 0), 3); shake(6, 15); }
                    if (totalKills == 50 && !kills50Shown) { kills50Shown = true; showPopup("50 KILLS!", COLOR_GREEN, 3); shake(3, 8); }
                    if (totalKills == 100 && !kills100Shown) { kills100Shown = true; showPopup("100 KILLS!", COLOR_ARGB(255, 80, 255, 80), 3); shake(5, 12); }
                    if (totalKills == 200 && !kills200Shown) { kills200Shown = true; showPopup("200 KILLS!", COLOR_ARGB(255, 160, 60, 220), 3); shake(7, 15); }

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

    // Player-enemy collision (skip if invincible or respawn invincible)
    if (!invincible && !respawnInvincible) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!enemies[i].active) continue;
            if (dist(px, py, enemies[i].x, enemies[i].y) < enemies[i].r + 8) {
                killedByType = enemies[i].type;
                lives--;
                if (lives <= 0) {
                    playerAlive = false;
                    g.SetScene(SCENE_DEATH);
                    for (int j = 0; j < MAX_ENEMIES; j++) enemies[j].active = false;
                    spawnExplosion(px, py, COLOR_WHITE, 80);
                    spawnExplosion(px, py, COLOR_CYAN, 60);
                    spawnExplosion(px, py, COLOR_ARGB(255, 255, 200, 100), 40);
                    gridImpulse(px, py, 500, 500);
                    shake(10, 25);
                } else {
                    spawnExplosion(px, py, COLOR_WHITE, 30);
                    spawnExplosion(px, py, COLOR_CYAN, 20);
                    gridImpulse(px, py, 200, 200);
                    shake(5, 10);
                    g.PlayWAV(pickRandom(sounds.explosion, 8));
                    px = MAP_W / 2.0f; py = MAP_H / 2.0f;
                    camX = px - WIN_W / 2.0f; camY = py - WIN_H / 2.0f;
                    for (int j = 0; j < MAX_ENEMIES; j++) {
                        if (enemies[j].active && dist(px, py, enemies[j].x, enemies[j].y) < SPAWN_CLEAR_RADIUS) {
                            spawnExplosion(enemies[j].x, enemies[j].y, enemyColor(enemies[j].type), 8);
                            enemies[j].active = false;
                        }
                    }
                    respawnInvincible = true;
                    respawnTimer = RESPAWN_INVINCIBLE;
                    energyActive = false;
                    energyTimer = 0;
                    for (int j = 0; j < MAX_BULLETS; j++) bullets[j].active = false;
                }
                break;
            }
        }
    }

    // Player-powerup collision
    if (playerAlive) {
        for (int i = 0; i < MAX_POWERUPS; i++) {
            if (!powerups[i].active) continue;
            if (dist(px, py, powerups[i].x, powerups[i].y) < powerups[i].r + 12) {
                if (powerups[i].type == 0) {
                    triggerNuke(g);
                } else if (powerups[i].type == 1) {
                    energyActive = true;
                    energyTimer = ENERGY_DURATION;
                    showPopup("SPREAD SHOT!", COLOR_ARGB(255, 255, 200, 50), 3);
                    shake(3, 8);
                }
                powerups[i].active = false;
            }
        }
    }
}

static void updateTimers(float dt) {
    if (comboTimer > 0) { comboTimer -= dt; if (comboTimer <= 0) combo = 1; }

    if (energyActive) {
        energyTimer -= dt;
        if (energyTimer <= 0) { energyActive = false; energyTimer = 0; }
    }

    if (respawnInvincible) {
        respawnTimer -= dt;
        if (respawnTimer <= 0) { respawnInvincible = false; respawnTimer = 0; }
    }

    shakeUpdate();

    // Nuke FX timers
    if (nukeFxActive) {
        // Flash: alpha decays slowly (full white → transparent in ~0.5s)
        nukeFlashAlpha -= dt * 2.0f;
        if (nukeFlashAlpha < 0) nukeFlashAlpha = 0;

        // Wave: radius expands rapidly, alpha fades as it grows
        nukeWaveRadius += dt * 1200.0f;
        nukeWaveAlpha -= dt * 2.5f;
        if (nukeWaveAlpha < 0) nukeWaveAlpha = 0;

        if (nukeFlashAlpha <= 0 && nukeWaveAlpha <= 0) {
            nukeFxActive = false;
            nukeWaveRadius = 0;
        }
    }
}

static void updateSpawner(GameLib &g, float dt) {
    if (g.GetScene() != SCENE_COMBAT || !playerAlive) return;
    gameTime += dt;

    int maxType = 0;
    if (gameTime > 15.0f) maxType = 1;
    if (gameTime > 30.0f) maxType = 2;
    if (gameTime > 60.0f) maxType = 3;

    float spawnInterval = 0.35f;
    if (gameTime > 120.0f) spawnInterval = 0.18f;
    else if (gameTime > 60.0f) spawnInterval = 0.22f;
    else if (gameTime > 30.0f) spawnInterval = 0.28f;

    int maxOnScreen = 15;
    if (gameTime > 120.0f) maxOnScreen = 40;
    else if (gameTime > 60.0f) maxOnScreen = 30;
    else if (gameTime > 30.0f) maxOnScreen = 25;
    else if (gameTime > 15.0f) maxOnScreen = 20;

    int activeCount = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) activeCount++;
    }

    spawnTimer += dt;
    if (spawnTimer >= spawnInterval && activeCount < maxOnScreen) {
        spawnTimer = 0;
        int type = rand() % (maxType + 1);
        spawnFromEdge(type);
        g.PlayWAV(pickRandom(sounds.spawn, 8));
    }

    powerupSpawnTimer += dt;
    if (powerupSpawnTimer >= 14.0f + (float)(rand() % 80) / 10.0f) {
        powerupSpawnTimer = 0;
        float pux = (float)(rand() % MAP_W);
        float puy = (float)(rand() % MAP_H);
        if (dist(pux, puy, px, py) > 200) {
            spawnPowerUp(pux, puy, 0);
        }
    }
}

static void gameUpdate(GameLib &g, float dt) {
    updatePlayer(g, dt);
    updateShooting(g, dt);
    updateBullets(dt);
    enemiesUpdate(dt);
    updateCollisions(g);
    particlesUpdate(dt);
    floatTextsUpdate(dt);
    popupUpdate(dt);
    powerupsUpdate(dt);
    gridUpdate(dt);
    starsUpdate(dt);
    updateTimers(dt);
    updateSpawner(g, dt);
}

// ============================================================
// Main
// ============================================================
int main() {
    GameLib game;
    game.Open(WIN_W, WIN_H, "Geometry Wars", true, true);
    game.ShowFps(true);
    game.ShowMouse(true);

    // Load all-time records from save file (defaults to 0 if no save)
    bestScore = GameLib::LoadInt(SAVE_FILE, "bestScore", 0);
    bestTime = GameLib::LoadFloat(SAVE_FILE, "bestTime", 0.0f);
    lbLoad();

    // Resolve sound paths - use actual files in assets/ directory
    sounds.shoot[0] = "assets/shoot-01.wav";
    sounds.shoot[1] = "assets/shoot-02.wav";
    sounds.shoot[2] = "assets/shoot-03.wav";
    sounds.shoot[3] = "assets/shoot-04.wav";

    sounds.explosion[0] = "assets/explosion-01.wav";
    sounds.explosion[1] = "assets/explosion-02.wav";
    sounds.explosion[2] = "assets/explosion-03.wav";
    sounds.explosion[3] = "assets/explosion-04.wav";
    sounds.explosion[4] = "assets/explosion-05.wav";
    sounds.explosion[5] = "assets/explosion-06.wav";
    sounds.explosion[6] = "assets/explosion-07.wav";
    sounds.explosion[7] = "assets/explosion-08.wav";

    sounds.spawn[0] = "assets/spawn-01.wav";
    sounds.spawn[1] = "assets/spawn-02.wav";
    sounds.spawn[2] = "assets/spawn-03.wav";
    sounds.spawn[3] = "assets/spawn-04.wav";
    sounds.spawn[4] = "assets/spawn-05.wav";
    sounds.spawn[5] = "assets/spawn-06.wav";
    sounds.spawn[6] = "assets/spawn-07.wav";
    sounds.spawn[7] = "assets/spawn-08.wav";

    // Use explosion variants for death and game over
    sounds.death = sounds.explosion[0];
    sounds.gameOver = sounds.explosion[1];
    sounds.noteHigh = sounds.spawn[0];

    // Init
    game.SetScene(SCENE_TITLE);
    resetGame();

    while (!game.IsClosed()) {
        double dt = game.GetDeltaTime();
        if (dt > 0.05) dt = 0.05;
        float fdt = (float)dt;

        if (game.IsKeyPressed(KEY_ESCAPE)) break;

        // Debug hotkey: F9 = Invincibility
        if (game.IsKeyPressed(KEY_F9)) {
            invincible = !invincible;
        }

        // Debug hotkey: F5 = Trigger Nuke
        if (game.IsKeyPressed(KEY_F5)) {
            triggerNuke(game);
        }

        // ---- State machine using SetScene ----
        switch (game.GetScene()) {
            case SCENE_TITLE: {
                game.Clear(COLOR_BLACK);
                starsDraw(game);
                gridUpdate(fdt);
                gridDraw(game);

                // Title text - centered on upper golden ratio line (600 * 0.382 ≈ 229)
                float pulse = (float)sin(game.GetTime() * 2) * 0.3f + 0.7f;
                uint32_t tc = COLOR_ARGB((uint32_t)(pulse * 255), 0, 255, 255);
                const char *title = "GEOMETRY WARS";
                int tw = (int)strlen(title) * 8 * 3;
                game.DrawTextScale(WIN_W / 2 - tw / 2, 210, title, tc, 3);

                // Blink "PRESS ENTER"
                if ((int)(game.GetTime() * 2) % 2 == 0) {
                    drawTextCentered(game, "PRESS ENTER TO START", 255, COLOR_LIGHT_GRAY, 1);
                }

                // START button (mouse clickable)
                int btnW = 140, btnH = 30;
                bool startBtn = game.Button(WIN_W / 2 - btnW / 2, 290, btnW, btnH, "START", COLOR_CYAN);

                // Controls instructions
                int ctrlY = 340;
                game.DrawText(WIN_W / 2 - 120, ctrlY, "WASD : Move", COLOR_DARK_GRAY);
                game.DrawText(WIN_W / 2 - 120, ctrlY + 15, "Mouse : Aim", COLOR_DARK_GRAY);
                game.DrawText(WIN_W / 2 - 120, ctrlY + 30, "Left Click : Shoot", COLOR_DARK_GRAY);
                game.DrawText(WIN_W / 2 - 120, ctrlY + 45, "Enter : Start Game", COLOR_DARK_GRAY);
                game.DrawText(WIN_W / 2 - 120, ctrlY + 60, "L : Leaderboard", COLOR_DARK_GRAY);

                // Best records at bottom (single line)
                int bestMin = (int)bestTime / 60;
                int bestSec = (int)bestTime % 60;
                game.DrawPrintfScale(WIN_W / 2 - 130, WIN_H - 50, COLOR_DARK_GRAY, 1, "BEST: %d  |  TIME %d:%02d", bestScore, bestMin, bestSec);

                // Powered by GameLib
                drawTextCentered(game, "Powered by GameLib", WIN_H - 25, COLOR_ARGB(100, 100, 100, 100), 1);

                if (game.IsKeyPressed(KEY_ENTER) || game.IsKeyPressed(KEY_SPACE) || startBtn) {
                    resetGame();
                    game.SetScene(SCENE_COMBAT);
                    game.PlayWAV(sounds.noteHigh);
                }

                if (game.IsKeyPressed(KEY_L)) {
                    lbHighlight = -1;
                    game.SetScene(SCENE_LEADERBOARD);
                }
                break;
            }

            case SCENE_COMBAT: {
                gameUpdate(game, fdt);
                game.Clear(COLOR_BLACK);
                starsDraw(game);
                gridDraw(game);
                drawMapBorder(game);
                drawBullets(game);
                drawEnemies(game);
                if (playerAlive) drawPlayer(game);
                powerupsDraw(game);
                particlesDraw(game);
                floatTextsDraw(game);
                popupDraw(game);
                drawHUD(game);

                // Nuke FX: shockwave circle + white flash overlay
                if (nukeFxActive) {
                    // Shockwave circle centered on player (world-space)
                    if (nukeWaveAlpha > 0) {
                        int waveSx = (int)(px - camX + shakeX);
                        int waveSy = (int)(py - camY + shakeY);
                        uint32_t wc = COLOR_ARGB((uint32_t)(nukeWaveAlpha * 200), 0, 255, 255);
                        game.DrawCircle(waveSx, waveSy, (int)nukeWaveRadius, wc);
                        wc = COLOR_ARGB((uint32_t)(nukeWaveAlpha * 100), 200, 255, 255);
                        game.DrawCircle(waveSx, waveSy, (int)(nukeWaveRadius * 0.85f), wc);
                    }
                    // White flash overlay (screen-space)
                    if (nukeFlashAlpha > 0) {
                        uint32_t fc = COLOR_ARGB((uint32_t)(nukeFlashAlpha * 220), 255, 255, 255);
                        game.FillRect(0, 0, WIN_W, WIN_H, fc);
                    }
                }
                break;
            }

            case SCENE_DEATH: {
                static float deathTimer = 0;
                if (game.IsSceneChanged()) deathTimer = 0;
                deathTimer += fdt;
                particlesUpdate(fdt);
                gridUpdate(fdt);
                starsUpdate(fdt);
                shakeUpdate();

                game.Clear(COLOR_BLACK);
                starsDraw(game);
                gridDraw(game);
                drawMapBorder(game);
                particlesDraw(game);

                // White flash
                if (deathTimer < 0.5f) {
                    game.FillRect(0, 0, WIN_W, WIN_H, COLOR_ARGB((int)((0.5f - deathTimer) * 400), 255, 255, 255));
                }

                if (deathTimer < 1.0f && (int)(deathTimer * 2) != (int)((deathTimer - fdt) * 2)) {
                    game.PlayWAV(sounds.death);
                }

                if (deathTimer > 1.5f) {
                    // Insert score into leaderboard & save best records
                    lbHighlight = lbInsert(score, gameTime, totalKills, highestCombo);
                    if (score > bestScore) {
                        bestScore = score;
                        GameLib::SaveInt(SAVE_FILE, "bestScore", bestScore);
                    }
                    if (gameTime > bestTime) {
                        bestTime = gameTime;
                        GameLib::SaveFloat(SAVE_FILE, "bestTime", bestTime);
                    }
                    game.SetScene(SCENE_GAME_OVER);
                    deathTimer = 0;
                    game.PlayWAV(sounds.gameOver);
                }
                break;
            }

            case SCENE_GAME_OVER: {
                static float goTimer = 0;
                if (game.IsSceneChanged()) goTimer = 0;
                goTimer += fdt;
                gridUpdate(fdt);
                starsUpdate(fdt);
                particlesUpdate(fdt);

                game.Clear(COLOR_BLACK);
                starsDraw(game);
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

                // Death cause
                if (killedByType >= 0 && killedByType <= 3) {
                    uint32_t kc = enemyColor(killedByType);
                    sprintf(buf, "KILLED BY: %s", enemyTypeName(killedByType));
                    drawTextCentered(game, buf, WIN_H / 2 + 90, kc, 1);
                    int iconX = WIN_W / 2 - (int)(strlen(buf) * 8) / 2 - 20;
                    int iconY = WIN_H / 2 + 93;
                    switch (killedByType) {
                        case 0: game.FillCircle(iconX, iconY, 6, kc); break;
                        case 1:
                            for (int j = 0; j < 3; j++) {
                                float a1 = j * (float)(2.0 * M_PI / 3);
                                float a2 = (j + 1) * (float)(2.0 * M_PI / 3);
                                game.DrawLine(iconX + (int)(cos(a1) * 8), iconY + (int)(sin(a1) * 8),
                                              iconX + (int)(cos(a2) * 8), iconY + (int)(sin(a2) * 8), kc);
                            }
                            break;
                        case 2:
                            for (int j = 0; j < 4; j++) {
                                float a1 = (float)M_PI / 4 + j * (float)(M_PI / 2);
                                float a2 = (float)M_PI / 4 + (j + 1) * (float)(M_PI / 2);
                                game.DrawLine(iconX + (int)(cos(a1) * 8), iconY + (int)(sin(a1) * 8),
                                              iconX + (int)(cos(a2) * 8), iconY + (int)(sin(a2) * 8), kc);
                            }
                            break;
                        case 3: game.FillCircle(iconX, iconY, 10, kc); game.DrawCircle(iconX, iconY, 5, COLOR_ARGB(200, 220, 120, 255)); break;
                    }
                }

                popupDraw(game);

                if (goTimer > 2.0f) {
                    // CONTINUE button (mouse clickable)
                    int btnW2 = 160, btnH2 = 30;
                    bool contBtn = game.Button(WIN_W / 2 - btnW2 / 2, WIN_H / 2 + 115, btnW2, btnH2, "CONTINUE", COLOR_RED);
                    if (game.IsKeyPressed(KEY_SPACE) || game.IsKeyPressed(KEY_ENTER) || contBtn) {
                        game.SetScene(SCENE_LEADERBOARD);
                        goTimer = 0;
                    }
                }
                break;
            }

            case SCENE_LEADERBOARD: {
                static float lbTimer = 0;
                if (game.IsSceneChanged()) {
                    lbTimer = 0;
                    camX = (MAP_W - WIN_W) / 2.0f;
                    camY = (MAP_H - WIN_H) / 2.0f;
                }
                lbTimer += fdt;
                gridUpdate(fdt);
                starsUpdate(fdt);

                game.Clear(COLOR_BLACK);
                starsDraw(game);
                gridDraw(game);

                lbDraw(game);

                if (lbTimer > 1.0f) {
                    // CONTINUE button (mouse clickable)
                    int btnW3 = 160, btnH3 = 30;
                    bool contBtn = game.Button(WIN_W / 2 - btnW3 / 2, WIN_H - 45, btnW3, btnH3, "CONTINUE", COLOR_CYAN);
                    if (game.IsKeyPressed(KEY_SPACE) || game.IsKeyPressed(KEY_ENTER) || contBtn) {
                        game.SetScene(SCENE_TITLE);
                        lbTimer = 0;
                        resetGame();
                    }
                }
                break;
            }
        }

        // Camera always updates (except title)
        if (game.GetScene() != SCENE_TITLE) cameraUpdate();

        game.Update();
    }

    return 0;
}