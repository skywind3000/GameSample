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
struct Enemy   { float x, y, vx, vy; int type; int hp; int maxHp; float r; float speed; bool active; float angle; float timer; };
struct Particle{ float x, y, vx, vy; uint32_t color; float life; float maxLife; float sz; };
struct FloatText { float x, y, vy; char text[16]; uint32_t color; float life; float maxLife; };
struct PowerUp { float x, y, r; float life; float pulse; bool active; int type; }; // type 0 = nuke, 1 = energy
struct Star { float x, y; uint32_t color; float drift; float sz; };
struct Popup { char text[32]; uint32_t color; float life; float maxLife; int scale; };
struct LBEntry { int score; float time; int kills; int combo; };

#define MAX_BLACK_HOLES 2
struct BlackHole {
    float x, y;
    float radius;
    float pullStrength;
    float life;
    int absorbed;
    bool active;
    float pulse;
    bool exploding;
    float explodeTimer;
};

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

// -- Player trail --
#define TRAIL_LEN 12
static float trailX[TRAIL_LEN], trailY[TRAIL_LEN], trailA[TRAIL_LEN];
static int trailCount = 0;
static float trailTimer = 0.0f;
static float thrustParticleTimer = 0.0f;

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

// -- Events --
#define MAX_TICKER 4
struct TickerMsg {
    char text[100];
    uint32_t color;
    float life;       // total display time
    float timer;      // elapsed time
    bool active;
};
static void tickerAdd(const char *text, uint32_t color);
static TickerMsg tickerMsgs[MAX_TICKER];
static const char *jackPhrases[] = {
    "THE SWARM RISES FROM EVERY CORNER!",
    "SHADOWS CONVERGE - ENEMIES SURGING FROM ALL SIDES!",
    "THE HIVE HAS AWAKENED - INVASION INBOUND!",
    "FOUR WALLS BREACHED - DENSITY CRITICAL!",
    "NOWHERE TO HIDE - THEY COME FROM EVERYWHERE!"
};
static const char *bhPhrases[] = {
    "GRAVITATIONAL ANOMALY DETECTED - CAUTION ADVISED!",
    "A SINGULARITY FORMS - ALL TRAJECTORIES BENDING!",
    "THE VOID AWAKENS - SPACE ITSELF IS WARPING!",
    "GRAVITY WELL ACTIVE - NOTHING ESCAPES ITS PULL!",
    "DIMENSIONAL RIFT - REALITY COLLAPSING INWARD!"
};
static float jackTimer = 0.0f;
static bool jackActive = false;
static float jackSpawnTimer = 0.0f;
static int jackCount = 0;
static int jackTotal = 0;
static float bhSpawnTimer = 0.0f;
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
static BlackHole blackHoles[MAX_BLACK_HOLES];
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
        case 3: return "ORBITER";
        case 4: return "WEAVER";
        default: return "UNKNOWN";
    }
}

static uint32_t enemyColor(int type) {
    switch (type) {
        case 0: return COLOR_ARGB(255, 255, 150, 40);   // orange - Swarm
        case 1: return COLOR_ARGB(255, 255, 80, 140);    // pink - Chaser
        case 2: return COLOR_ARGB(255, 80, 255, 80);     // green - Bouncer
        case 3: return COLOR_ARGB(255, 160, 60, 220);    // purple - Orbiter
        case 4: return COLOR_ARGB(255, 255, 220, 50);    // yellow - Weaver
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
static void spawnEnemy(int type, float x, float y);

// ============================================================
// Init & Reset
// ============================================================
static void resetGame() {
    score = 0; combo = 1; kills = 0;
    totalKills = 0; highestCombo = 1; comboTimer = 0;
    gameTime = 0.0f; spawnTimer = 0.0f; powerupSpawnTimer = 0.0f;
    px = MAP_W / 2.0f; py = MAP_H / 2.0f;
    camX = px - WIN_W / 2.0f; camY = py - WIN_H / 2.0f;
    trailCount = 0; trailTimer = 0;
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
    for (int i = 0; i < MAX_TICKER; i++) tickerMsgs[i].active = false;
    lbHighlight = -1;
    gridInit();
    starsInit();
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].life = 0;
    for (int i = 0; i < MAX_FLOAT_TEXTS; i++) floatTexts[i].life = 0;
    for (int i = 0; i < MAX_POWERUPS; i++) powerups[i].active = false;
    for (int i = 0; i < MAX_BLACK_HOLES; i++) blackHoles[i].active = false;
    jackTimer = 0; jackActive = false; jackCount = 0; jackTotal = 0;
    bhSpawnTimer = 0;
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
            float elapsed = ft.maxLife - ft.life;
            float holdTime = 0.5f;
            float fadeTime = ft.maxLife - holdTime;
            float alpha;
            if (elapsed < holdTime) {
                alpha = 1.0f;
            } else {
                alpha = 1.0f - (elapsed - holdTime) / fadeTime;
                if (alpha < 0) alpha = 0;
            }
            int sx = (int)(ft.x - camX + shakeX);
            int sy = (int)(ft.y - camY + shakeY);
            uint32_t c = COLOR_ARGB((uint32_t)(alpha * 255), COLOR_GET_R(ft.color), COLOR_GET_G(ft.color), COLOR_GET_B(ft.color));
            g.DrawTextScale(sx + 1, sy + 1, ft.text, COLOR_ARGB((uint32_t)(alpha * 100), 255, 255, 255), 1.2f);
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
            core = COLOR_ARGB(255, 255, 180, 60);
            glow = COLOR_ARGB(80, 255, 120, 30);
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

// ============================================================
// Black Hole System
// ============================================================
static void blackHolesUpdate(float dt) {
    for (int i = 0; i < MAX_BLACK_HOLES; i++) {
        if (!blackHoles[i].active) continue;
        BlackHole &bh = blackHoles[i];

        if (bh.exploding) {
            bh.explodeTimer += dt;
            if (bh.explodeTimer >= 0.5f) {
                bh.active = false;
                continue;
            }
            continue;
        }

        bh.pulse += dt * 3.0f;
        bh.life -= dt;
        bh.radius = 100.0f + bh.absorbed * 8.0f;
        if (bh.radius > 180.0f) bh.radius = 180.0f;

        // Pull enemies toward BH
        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!enemies[j].active) continue;
            Enemy &e = enemies[j];
            float dx = bh.x - e.x, dy = bh.y - e.y;
            float d = dist(e.x, e.y, bh.x, bh.y);
            if (d < bh.radius && d > 1) {
                e.vx += (dx / d) * bh.pullStrength * dt;
                e.vy += (dy / d) * bh.pullStrength * dt;
            }
            // Absorb: enemy close to BH core
            if (d < 20.0f) {
                bh.absorbed++;
                int pts[5] = { 50, 100, 150, 300, 200 };
                int earned = pts[e.type] * combo;
                score += earned;
                combo++; comboTimer = COMBO_TIMEOUT;
                if (combo > highestCombo) highestCombo = combo;
                spawnExplosion(e.x, e.y, enemyColor(e.type), 8);
                char buf[16];
                sprintf(buf, "+%d", earned);
                spawnFloatText(e.x, e.y - 10, buf, COLOR_ARGB(255, 255, 255, 255));
                e.active = false;
            }
        }

        // Explode if absorbed enough
        if (bh.absorbed >= 10) {
            bh.exploding = true;
            bh.explodeTimer = 0;
            shake(5, 12);
            spawnExplosion(bh.x, bh.y, COLOR_ARGB(255, 160, 60, 220), 30);
            tickerAdd(bhPhrases[rand() % 5], COLOR_ARGB(255, 200, 60, 80));
            gridImpulse(bh.x, bh.y, 300, 200);
            // Release 8 small enemies from BH
            for (int k = 0; k < 8; k++) {
                float a = (float)(k * 45) * (float)M_PI / 180.0f;
                spawnEnemy(0, bh.x + (float)cos(a) * 20, bh.y + (float)sin(a) * 20);
            }
        }

        // Quiet disappear if life runs out
        if (bh.life <= 0 && !bh.exploding) {
            spawnExplosion(bh.x, bh.y, COLOR_ARGB(255, 100, 50, 150), 10);
            bh.active = false;
        }
    }
}

static void blackHolesDraw(GameLib &g) {
    for (int i = 0; i < MAX_BLACK_HOLES; i++) {
        if (!blackHoles[i].active) continue;
        BlackHole &bh = blackHoles[i];
        int sx = (int)(bh.x - camX + shakeX), sy = (int)(bh.y - camY + shakeY);

        if (bh.exploding) {
            float expandProgress = bh.explodeTimer / 0.5f;
            int waveR = (int)(expandProgress * 400);
            int waveAlpha = (int)(200 * (1.0f - expandProgress));
            g.DrawCircle(sx, sy, waveR, COLOR_ARGB(waveAlpha, 160, 60, 220));
            g.DrawCircle(sx, sy, waveR / 2, COLOR_ARGB(waveAlpha / 2, 220, 150, 255));
            continue;
        }

        // Gravity range ring (pulsing)
        float pulseScale = (float)sin(bh.pulse) * 0.1f + 1.0f;
        int ringR = (int)(bh.radius * pulseScale);
        g.DrawCircle(sx, sy, ringR, COLOR_ARGB(40, 160, 60, 220));

        // Core
        int coreR = 15 + bh.absorbed * 2;
        if (coreR > 35) coreR = 35;
        g.FillCircle(sx, sy, coreR, COLOR_ARGB(200, 120, 40, 160));
        g.FillCircle(sx, sy, coreR + 5, COLOR_ARGB(80, 160, 60, 220));

        // Center bright dot
        int dotAlpha = 100 + bh.absorbed * 15;
        if (dotAlpha > 255) dotAlpha = 255;
        g.FillCircle(sx, sy, 3, COLOR_ARGB(dotAlpha, 220, 150, 255));
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
        int pts[5] = { 50, 100, 150, 300, 200 };
        int earned = pts[e.type];
        score += earned;
        kills++; totalKills++;
        spawnExplosion(e.x, e.y, enemyColor(e.type), 20 + e.type * 8);
        char buf[16];
        sprintf(buf, "+%d", earned);
        spawnFloatText(e.x, e.y - 10, buf, COLOR_ARGB(255, 255, 255, 255));
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
            e.vx = e.vy = 0; e.timer = 0;
            switch (type) {
                case 0: e.r = 10; e.speed = 60; break;
                case 1: e.r = 12; e.speed = 140; break;
                case 2: e.r = 14; e.speed = 100; e.hp = e.maxHp = 2; break;
                case 3: e.r = 22; e.speed = 50; e.hp = e.maxHp = 5; e.timer = (float)(rand() % 600) / 100.0f; break;
                case 4: e.r = 12; e.speed = 120; e.hp = 2; e.maxHp = 2; e.timer = (float)(rand() % 600) / 100.0f; break;
            }
            if (type == 2) {
                float a = (float)rand() / RAND_MAX * (float)(2.0 * M_PI);
                e.vx = (float)cos(a) * e.speed;
                e.vy = (float)sin(a) * e.speed;
                e.angle = (float)rand() / RAND_MAX * (float)(2.0 * M_PI);
            }
            if (type == 4) {
                float a = (float)rand() / RAND_MAX * (float)(2.0 * M_PI);
                e.vx = (float)cos(a) * e.speed;
                e.vy = (float)sin(a) * e.speed;
                e.angle = a;
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
            case 3: { // Orbiter - orbit around player at a distance
                float dx = px - e.x, dy = py - e.y;
                float d = dist(e.x, e.y, px, py);
                if (d > 1) {
                    float targetDist = 180.0f;
                    float radialForce = (d - targetDist) / d;
                    e.vx += (dx / d) * radialForce * e.speed * 2.0f * dt;
                    e.vy += (dy / d) * radialForce * e.speed * 2.0f * dt;
                    e.timer += dt;
                    float tangAngle = (float)atan2(dy, dx) + (float)M_PI / 2.0f;
                    e.vx += (float)cos(tangAngle) * e.speed * 3.0f * dt;
                    e.vy += (float)sin(tangAngle) * e.speed * 3.0f * dt;
                }
                float f = 1.0f - 3.0f * dt; if (f < 0.8f) f = 0.8f;
                e.vx *= f; e.vy *= f;
                e.angle += 2.0f * dt;
                break;
            }
            case 4: { // Weaver - sinusoidal weaving motion
                e.timer += dt;
                float baseAngle = e.angle;
                float weaveOffset = (float)sin(e.timer * 3.0f) * 0.8f;
                float moveAngle = baseAngle + weaveOffset;
                e.vx = (float)cos(moveAngle) * e.speed;
                e.vy = (float)sin(moveAngle) * e.speed;
                break;
            }
        }
        e.x += e.vx * dt;
        e.y += e.vy * dt;

        if (e.type != 2 && e.type != 4) {
            clamp(e.x, e.r, MAP_W - e.r);
            clamp(e.y, e.r, MAP_H - e.r);
        } else if (e.type == 2) {
            if (e.x < e.r) { e.x = e.r; e.vx = -e.vx; }
            if (e.x > MAP_W - e.r) { e.x = MAP_W - e.r; e.vx = -e.vx; }
            if (e.y < e.r) { e.y = e.r; e.vy = -e.vy; }
            if (e.y > MAP_H - e.r) { e.y = MAP_H - e.r; e.vy = -e.vy; }
        } else { // Weaver - bounce off walls like Bouncer
            if (e.x < e.r) { e.x = e.r; e.vx = -e.vx; e.angle = (float)atan2(e.vy, e.vx); }
            if (e.x > MAP_W - e.r) { e.x = MAP_W - e.r; e.vx = -e.vx; e.angle = (float)atan2(e.vy, e.vx); }
            if (e.y < e.r) { e.y = e.r; e.vy = -e.vy; e.angle = (float)atan2(e.vy, e.vx); }
            if (e.y > MAP_H - e.r) { e.y = MAP_H - e.r; e.vy = -e.vy; e.angle = (float)atan2(e.vy, e.vx); }
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
    int px = WIN_W / 2 - tw / 2;
    int py = WIN_H / 2 - 20;
    g.DrawTextScale(px + 1, py + 1, popup.text, COLOR_ARGB(100, 255, 255, 255), currentScale);
    g.DrawTextScale(px, py, popup.text, drawColor, currentScale);
}

// ============================================================
// Ticker (scrolling event notification queue)
// ============================================================
static void tickerAdd(const char *text, uint32_t color) {
    // Find an inactive slot, or use oldest active if full
    int slot = -1;
    for (int i = 0; i < MAX_TICKER; i++) {
        if (!tickerMsgs[i].active) { slot = i; break; }
    }
    if (slot == -1) {
        // All full: find the one closest to finishing
        float bestLife = 999;
        for (int i = 0; i < MAX_TICKER; i++) {
            float remaining = tickerMsgs[i].life - tickerMsgs[i].timer;
            if (remaining < bestLife) { bestLife = remaining; slot = i; }
        }
    }
    strncpy(tickerMsgs[slot].text, text, 99);
    tickerMsgs[slot].text[99] = '\0';
    tickerMsgs[slot].color = color;
    tickerMsgs[slot].life = 3.0f;
    tickerMsgs[slot].timer = 0;
    tickerMsgs[slot].active = true;
}

static void tickerUpdate(float dt) {
    for (int i = 0; i < MAX_TICKER; i++) {
        if (!tickerMsgs[i].active) continue;
        tickerMsgs[i].timer += dt;
        if (tickerMsgs[i].timer >= tickerMsgs[i].life) {
            tickerMsgs[i].active = false;
        }
    }
}

static void tickerDraw(GameLib &g) {
    float slideInDur = 0.4f;
    float slideOutDur = 0.5f;
    int yOffset = 35;
    for (int i = 0; i < MAX_TICKER; i++) {
        if (!tickerMsgs[i].active) continue;
        TickerMsg &msg = tickerMsgs[i];
        float t = msg.timer;
        float total = msg.life;
        int tw = (int)strlen(msg.text) * 8;
        int y = yOffset + i * 15;

        int x;
        if (t < slideInDur) {
            float progress = t / slideInDur;
            x = WIN_W + 10 - (int)(progress * (WIN_W + 10 + tw + 20));
        } else if (t < total - slideOutDur) {
            x = 20;
        } else {
            float progress = (t - (total - slideOutDur)) / slideOutDur;
            x = 20 - (int)(progress * (tw + 20));
        }

        uint32_t drawColor;
        if (t >= total - slideOutDur) {
            float alpha = 1.0f - (t - (total - slideOutDur)) / slideOutDur;
            drawColor = COLOR_ARGB((uint32_t)(alpha * 200), COLOR_GET_R(msg.color), COLOR_GET_G(msg.color), COLOR_GET_B(msg.color));
        } else {
            drawColor = COLOR_ARGB(200, COLOR_GET_R(msg.color), COLOR_GET_G(msg.color), COLOR_GET_B(msg.color));
        }
        g.DrawText(x + 1, y + 1, msg.text, COLOR_ARGB(100, 255, 255, 255));
        g.DrawText(x, y, msg.text, drawColor);
    }
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
    uint32_t outline = COLOR_ARGB(160, 100, 255, 255);

    // Draw trail (semi-transparent copies fading out)
    for (int i = trailCount - 1; i >= 1; i--) {
        float alphaFactor = 1.0f - (float)i / (float)TRAIL_LEN;
        int trailSx = (int)(trailX[i] - camX + shakeX);
        int trailSy = (int)(trailY[i] - camY + shakeY);
        float ta = trailA[i];
        int trailAlpha = (int)(alphaFactor * 100);
        float scale = 0.7f + alphaFactor * 0.3f;
        uint32_t trailColor = COLOR_ARGB(trailAlpha, 0, 200, 255);
        float tVerts[6];
        tVerts[0] = trailSx + (float)cos(ta) * 18 * scale;
        tVerts[1] = trailSy + (float)sin(ta) * 18 * scale;
        float tba = ta + (float)M_PI;
        tVerts[2] = trailSx + ((float)cos(tba) * 10 + (float)cos(tba + 1.3f) * 10) * scale;
        tVerts[3] = trailSy + ((float)sin(tba) * 10 + (float)sin(tba + 1.3f) * 10) * scale;
        tVerts[4] = trailSx + ((float)cos(tba) * 10 + (float)cos(tba - 1.3f) * 10) * scale;
        tVerts[5] = trailSy + ((float)sin(tba) * 10 + (float)sin(tba - 1.3f) * 10) * scale;
        g.FillTriangle((int)tVerts[0], (int)tVerts[1], (int)tVerts[2], (int)tVerts[3], (int)tVerts[4], (int)tVerts[5], trailColor);
    }

    // Glow circle
    g.FillCircle(sx, sy, 20, glow);
    if (energyActive) {
        g.FillCircle(sx, sy, 25, COLOR_ARGB(80, 255, 180, 60));
    }

    // Outer outline (slightly larger triangle)
    float oScale = 1.3f;
    float oVerts[6];
    oVerts[0] = sx + (float)cos(a) * 18 * oScale;
    oVerts[1] = sy + (float)sin(a) * 18 * oScale;
    float oba = a + (float)M_PI;
    oVerts[2] = sx + (float)cos(oba) * 10 * oScale + (float)cos(oba + 1.3f) * 10 * oScale;
    oVerts[3] = sy + (float)sin(oba) * 10 * oScale + (float)sin(oba + 1.3f) * 10 * oScale;
    oVerts[4] = sx + (float)cos(oba) * 10 * oScale + (float)cos(oba - 1.3f) * 10 * oScale;
    oVerts[5] = sy + (float)sin(oba) * 10 * oScale + (float)sin(oba - 1.3f) * 10 * oScale;
    g.DrawTriangle((int)oVerts[0], (int)oVerts[1], (int)oVerts[2], (int)oVerts[3], (int)oVerts[4], (int)oVerts[5], outline);

    // Core triangle
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
    g.FillCircle((int)ex, (int)ey, 4, COLOR_ARGB(150, 255, 180, 60));
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
            case 0: // Swarm - circle
                g.FillCircle(sx, sy, (int)e.r, c);
                g.FillCircle(sx, sy, (int)(e.r * 1.8f), gl);
                break;
            case 1: // Chaser - triangle
                for (int j = 0; j < 3; j++) {
                    float a1 = e.angle + j * (float)(2.0f * M_PI / 3);
                    float a2 = e.angle + (j + 1) * (float)(2.0f * M_PI / 3);
                    g.DrawLine(sx + (int)(cos(a1) * e.r), sy + (int)(sin(a1) * e.r),
                               sx + (int)(cos(a2) * e.r), sy + (int)(sin(a2) * e.r), c);
                }
                g.FillCircle(sx, sy, (int)e.r * 2, gl);
                break;
            case 2: // Bouncer - diamond (rotated square)
                for (int j = 0; j < 4; j++) {
                    float a1 = (float)M_PI / 4 + e.angle + j * (float)(M_PI / 2);
                    float a2 = (float)M_PI / 4 + e.angle + (j + 1) * (float)(M_PI / 2);
                    g.DrawLine(sx + (int)(cos(a1) * e.r), sy + (int)(sin(a1) * e.r),
                               sx + (int)(cos(a2) * e.r), sy + (int)(sin(a2) * e.r), c);
                }
                g.FillCircle(sx, sy, (int)e.r * 1.5f, gl);
                break;
            case 3: // Orbiter - circle with ring
                g.FillCircle(sx, sy, (int)e.r, c);
                g.FillCircle(sx, sy, (int)(e.r * 1.6f), gl);
                g.DrawCircle(sx, sy, (int)(e.r * 0.6f), COLOR_ARGB(200, 220, 120, 255));
                break;
            case 4: { // Weaver - pentagon (5-sided)
                for (int j = 0; j < 5; j++) {
                    float a1 = e.angle + j * (float)(2.0f * M_PI / 5);
                    float a2 = e.angle + (j + 1) * (float)(2.0f * M_PI / 5);
                    g.DrawLine(sx + (int)(cos(a1) * e.r), sy + (int)(sin(a1) * e.r),
                               sx + (int)(cos(a2) * e.r), sy + (int)(sin(a2) * e.r), c);
                }
                g.FillCircle(sx, sy, (int)(e.r * 1.6f), gl);
                break;
            }
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
        g.FillRect(barX, barY, (int)(barW * ratio), barH, COLOR_ARGB(200, 255, 180, 60));
        g.DrawRect(barX, barY, barW, barH, COLOR_ARGB(150, 255, 120, 30));
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
    bool isMoving = (len > 0);
    if (isMoving) { dx /= len; dy /= len; px += dx * PLAYER_SPEED * dt; py += dy * PLAYER_SPEED * dt; }
    clamp(px, 20, MAP_W - 20);
    clamp(py, 20, MAP_H - 20);

    // Black Hole pull on player (weaker than enemies)
    for (int i = 0; i < MAX_BLACK_HOLES; i++) {
        if (!blackHoles[i].active || blackHoles[i].exploding) continue;
        float bdx = blackHoles[i].x - px, bdy = blackHoles[i].y - py;
        float bd = dist(px, py, blackHoles[i].x, blackHoles[i].y);
        if (bd < blackHoles[i].radius && bd > 1) {
            px += (bdx / bd) * blackHoles[i].pullStrength * 0.3f * dt;
            py += (bdy / bd) * blackHoles[i].pullStrength * 0.3f * dt;
            clamp(px, 20, MAP_W - 20);
            clamp(py, 20, MAP_H - 20);
        }
    }

    float mx = (float)(g.GetMouseX()) + camX, my = (float)(g.GetMouseY()) + camY;
    float ddx = mx - px, ddy = my - py;
    if (dist(px, py, mx, my) > 5) pAngle = (float)atan2(ddy, ddx);

    // Trail update: record position every ~0.03s
    trailTimer += dt;
    if (trailTimer >= 0.03f) {
        trailTimer = 0;
        if (trailCount < TRAIL_LEN) trailCount++;
        for (int i = trailCount - 1; i > 0; i--) {
            trailX[i] = trailX[i - 1]; trailY[i] = trailY[i - 1]; trailA[i] = trailA[i - 1];
        }
        trailX[0] = px; trailY[0] = py; trailA[0] = pAngle;
    }

    // Thrust particles: emit ~1 every 2 frames from rear when moving
    thrustParticleTimer += dt;
    if (isMoving && !respawnInvincible && thrustParticleTimer >= 0.06f) {
        thrustParticleTimer = 0;
        float rearX = px - (float)cos(pAngle) * 14;
        float rearY = py - (float)sin(pAngle) * 14;
        float spread = (float)((rand() % 60 - 30) * M_PI / 180.0f);
        float spd = (float)(350 + rand() % 250);
        float thrustAngle = pAngle + (float)M_PI + spread;
        int r = 150 + rand() % 105;
        int gb = (rand() % 2 == 0) ? 255 : 220;
        int b2 = 200 + rand() % 55;
        uint32_t thrustColor = energyActive
            ? COLOR_ARGB(255, 255, 240, 200 + rand() % 55)
            : COLOR_ARGB(255, r, gb, b2);
        spawnParticle(rearX, rearY,
                      (float)cos(thrustAngle) * spd, (float)sin(thrustAngle) * spd,
                      thrustColor, 0.35f + (float)rand() / RAND_MAX * 0.2f, 2.0f + (float)(rand() % 2));
    }
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
                    int pts[5] = { 50, 100, 150, 300, 200 };
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

                    int baseDrop[] = { 7, 10, 12, 17, 10 };
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
                    showPopup("SPREAD SHOT!", COLOR_ARGB(255, 255, 180, 60), 3);
                    shake(3, 8);
                }
                powerups[i].active = false;
            }
        }
    }

    // Bullet-Black Hole collision
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        for (int j = 0; j < MAX_BLACK_HOLES; j++) {
            if (!blackHoles[j].active || blackHoles[j].exploding) continue;
            if (dist(bullets[i].x, bullets[i].y, blackHoles[j].x, blackHoles[j].y) < 30) {
                // Hit spark toward BH
                for (int s = 0; s < 3; s++) {
                    float a = (float)atan2(blackHoles[j].y - bullets[i].y, blackHoles[j].x - bullets[i].x) + (float)((rand() % 60 - 30) * M_PI / 180.0f);
                    float spd = (float)(100 + rand() % 80);
                    spawnParticle(bullets[i].x, bullets[i].y,
                                  (float)cos(a) * spd, (float)sin(a) * spd,
                                  COLOR_ARGB(255, 220, 150, 255), 0.3f, 3.0f);
                }
                blackHoles[j].absorbed++;
                bullets[i].active = false;
                break;
            }
        }
    }

    // Player-Black Hole collision (touch the core = death)
    if (!invincible && !respawnInvincible && playerAlive) {
        for (int i = 0; i < MAX_BLACK_HOLES; i++) {
            if (!blackHoles[i].active || blackHoles[i].exploding) continue;
            int coreR = 15 + blackHoles[i].absorbed * 2;
            if (coreR > 35) coreR = 35;
            if (dist(px, py, blackHoles[i].x, blackHoles[i].y) < coreR + 8) {
                killedByType = -1; // Killed by Black Hole, not an enemy type
                lives--;
                if (lives <= 0) {
                    playerAlive = false;
                    g.SetScene(SCENE_DEATH);
                    for (int j = 0; j < MAX_ENEMIES; j++) enemies[j].active = false;
                    for (int j = 0; j < MAX_BLACK_HOLES; j++) blackHoles[j].active = false;
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

    // -- Jack Invasion event --
    jackTimer += dt;
    float jackCooldown = 45.0f - gameTime * 0.01f;
    if (jackCooldown < 30.0f) jackCooldown = 30.0f;
    if (!jackActive && jackTimer >= jackCooldown) {
        jackTimer = 0;
        jackActive = true;
        jackSpawnTimer = 0;
        jackCount = 0;
        jackTotal = 20 + (int)(gameTime / 30);
        if (jackTotal > 40) jackTotal = 40;
        showPopup("JACK INVASION!", COLOR_ARGB(255, 255, 220, 50), 3);
        shake(5, 10);
    }
    if (jackActive) {
        jackSpawnTimer += dt;
        if (jackSpawnTimer >= 0.05f && jackCount < jackTotal) {
            jackSpawnTimer = 0;
            int side = rand() % 4;
            float jx, jy;
            float margin = 80;
            switch (side) {
                case 0: jx = (float)(rand() % MAP_W); jy = -margin; break;
                case 1: jx = (float)(rand() % MAP_W); jy = MAP_H + margin; break;
                case 2: jx = -margin; jy = (float)(rand() % MAP_H); break;
                default: jx = MAP_W + margin; jy = (float)(rand() % MAP_H); break;
            }
            spawnEnemy(0, jx, jy);
            g.PlayWAV(pickRandom(sounds.spawn, 8));
            jackCount++;
        }
        if (jackCount >= jackTotal) {
            jackActive = false;
        }
        if (jackActive) return; // Pause normal spawning during Jack
    }

    // -- Normal spawning (continuous difficulty) --
    float spawnInterval = 0.35f - gameTime * 0.001f;
    if (spawnInterval < 0.10f) spawnInterval = 0.10f;

    int maxOnScreen = 15 + (int)(gameTime / 10);
    if (maxOnScreen > 60) maxOnScreen = 60;

    int activeCount = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) activeCount++;
    }

    spawnTimer += dt;
    if (spawnTimer >= spawnInterval && activeCount < maxOnScreen) {
        spawnTimer = 0;
        int type;
        if (gameTime <= 15.0f) {
            type = 0;
        } else if (gameTime <= 30.0f) {
            type = (rand() % 3 == 0) ? 2 : 0;
        } else if (gameTime <= 60.0f) {
            int roll = rand() % 10;
            if (roll < 5) type = 0;
            else if (roll < 8) type = 1;
            else type = 2;
        } else if (gameTime <= 90.0f) {
            int roll = rand() % 10;
            if (roll < 4) type = 0;
            else if (roll < 6) type = 1;
            else if (roll < 8) type = 2;
            else type = 4;
        } else {
            int roll = rand() % 10;
            if (roll < 3) type = 0;
            else if (roll < 5) type = 1;
            else if (roll < 7) type = 2;
            else if (roll < 8) type = 3;
            else type = 4;
        }
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

    // -- Black Hole spawning --
    bhSpawnTimer += dt;
    float bhCooldown = 60.0f + (float)(rand() % 30);
    int activeBH = 0;
    for (int i = 0; i < MAX_BLACK_HOLES; i++) if (blackHoles[i].active) activeBH++;
    if (bhSpawnTimer >= bhCooldown && activeBH < MAX_BLACK_HOLES && gameTime > 60.0f) {
        bhSpawnTimer = 0;
        for (int tries = 0; tries < 20; tries++) {
            float bhx = (float)(80 + rand() % (MAP_W - 160));
            float bhy = (float)(80 + rand() % (MAP_H - 160));
            if (dist(bhx, bhy, px, py) > 250) {
                for (int i = 0; i < MAX_BLACK_HOLES; i++) {
                    if (!blackHoles[i].active) {
                        blackHoles[i].active = true;
                        blackHoles[i].x = bhx;
                        blackHoles[i].y = bhy;
                        blackHoles[i].radius = 100.0f;
                        blackHoles[i].pullStrength = 120.0f;
                        blackHoles[i].life = 8.0f;
                        blackHoles[i].absorbed = 0;
                        blackHoles[i].pulse = 0;
                        blackHoles[i].exploding = false;
                        blackHoles[i].explodeTimer = 0;
                        break;
                    }
                }
                break;
            }
        }
    }
}

static void gameUpdate(GameLib &g, float dt) {
    updatePlayer(g, dt);
    updateShooting(g, dt);
    updateBullets(dt);
    enemiesUpdate(dt);
    updateCollisions(g);
    blackHolesUpdate(dt);
    particlesUpdate(dt);
    floatTextsUpdate(dt);
    popupUpdate(dt);
    tickerUpdate(dt);
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

        // Debug hotkey: F6 = Trigger Jack Invasion
        if (game.IsKeyPressed(KEY_F6)) {
            jackTimer = 0;
            jackActive = true;
            jackSpawnTimer = 0;
            jackCount = 0;
            jackTotal = 25;
            tickerAdd(jackPhrases[rand() % 5], COLOR_ARGB(255, 255, 80, 60));
            shake(5, 10);
        }

        // Debug hotkey: F7 = Spawn Black Hole
        if (game.IsKeyPressed(KEY_F7)) {
            for (int i = 0; i < MAX_BLACK_HOLES; i++) {
                if (!blackHoles[i].active) {
                    blackHoles[i].active = true;
                    blackHoles[i].x = px + 200;
                    blackHoles[i].y = py;
                    blackHoles[i].radius = 100.0f;
                    blackHoles[i].pullStrength = 120.0f;
                    blackHoles[i].life = 8.0f;
                    blackHoles[i].absorbed = 0;
                    blackHoles[i].pulse = 0;
                    blackHoles[i].exploding = false;
                    blackHoles[i].explodeTimer = 0;
                    break;
                }
            }
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
                blackHolesDraw(game);
                drawEnemies(game);
                if (playerAlive) drawPlayer(game);
                powerupsDraw(game);
                particlesDraw(game);
                floatTextsDraw(game);
                popupDraw(game);
                tickerDraw(game);
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
                if (killedByType >= 0 && killedByType <= 4) {
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
                        case 4:
                            for (int j = 0; j < 5; j++) {
                                float a1 = j * (float)(2.0 * M_PI / 5);
                                float a2 = (j + 1) * (float)(2.0 * M_PI / 5);
                                game.DrawLine(iconX + (int)(cos(a1) * 8), iconY + (int)(sin(a1) * 8),
                                              iconX + (int)(cos(a2) * 8), iconY + (int)(sin(a2) * 8), kc);
                            }
                            break;
                    }
                }

                popupDraw(game);
                tickerDraw(game);

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