// GameTool.h - Utility library for GameLib.h games
//
// Header-only, C++11, only depends on GameLib.h.
// Usage: #include "GameTool.h" (after including GameLib.h)
//
// Modules: Math, Color, Easing, Object Pool, CooldownTimer,
//          CountdownTimer, Screen Shake, Camera2D, Particles, FloatText, Input

#ifndef GAMETOOL_H_INCLUDED
#define GAMETOOL_H_INCLUDED

#ifndef GAMELIB_H
#error "GameTool.h requires GameLib.h - include GameLib.h first"
#endif

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================
// Math
// ============================================================

inline float gtClamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline int gtClampInt(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float gtLerp(float a, float b, float t) {
    return a + (b - a) * t;
}

inline float gtDist(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1, dy = y2 - y1;
    return (float)sqrt(dx * dx + dy * dy);
}

inline float gtDistSq(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1, dy = y2 - y1;
    return dx * dx + dy * dy;
}

inline float gtAngle(float x1, float y1, float x2, float y2) {
    return (float)atan2(y2 - y1, x2 - x1);
}

inline float gtDegToRad(float deg) { return deg * (float)M_PI / 180.0f; }
inline float gtRadToDeg(float rad) { return rad * 180.0f / (float)M_PI; }

inline float gtRandomFloat(float lo, float hi) {
    return lo + (hi - lo) * (float)rand() / (float)RAND_MAX;
}

inline float gtRandomAngle() {
    return (float)rand() / (float)RAND_MAX * (float)(2.0 * M_PI);
}

inline float gtApproach(float current, float target, float step) {
    if (current < target) {
        current += step;
        if (current > target) current = target;
    } else {
        current -= step;
        if (current < target) current = target;
    }
    return current;
}

inline float gtNormalize(float &vx, float &vy) {
    float len = (float)sqrt(vx * vx + vy * vy);
    if (len > 0.0001f) { vx /= len; vy /= len; }
    return len;
}

// ============================================================
// Color
// ============================================================

inline uint32_t gtColorAlpha(uint32_t color, int alpha) {
    return COLOR_ARGB(alpha & 0xFF, COLOR_GET_R(color),
                      COLOR_GET_G(color), COLOR_GET_B(color));
}

inline uint32_t gtColorAlphaF(uint32_t color, float factor) {
    int a = gtClampInt((int)(factor * 255), 0, 255);
    return COLOR_ARGB(a, COLOR_GET_R(color), COLOR_GET_G(color), COLOR_GET_B(color));
}

inline uint32_t gtColorLerp(uint32_t c1, uint32_t c2, float t) {
    int r = (int)(COLOR_GET_R(c1) + (COLOR_GET_R(c2) - COLOR_GET_R(c1)) * t);
    int g = (int)(COLOR_GET_G(c1) + (COLOR_GET_G(c2) - COLOR_GET_G(c1)) * t);
    int b = (int)(COLOR_GET_B(c1) + (COLOR_GET_B(c2) - COLOR_GET_B(c1)) * t);
    int a = (int)(COLOR_GET_A(c1) + (COLOR_GET_A(c2) - COLOR_GET_A(c1)) * t);
    return COLOR_ARGB(gtClampInt(a, 0, 255), gtClampInt(r, 0, 255),
                      gtClampInt(g, 0, 255), gtClampInt(b, 0, 255));
}

inline uint32_t gtColorBrighten(uint32_t color, float factor) {
    int r = gtClampInt((int)(COLOR_GET_R(color) * factor), 0, 255);
    int g = gtClampInt((int)(COLOR_GET_G(color) * factor), 0, 255);
    int b = gtClampInt((int)(COLOR_GET_B(color) * factor), 0, 255);
    return COLOR_ARGB(COLOR_GET_A(color), r, g, b);
}

inline uint32_t gtColorDarken(uint32_t color, float factor) {
    return gtColorBrighten(color, factor);
}

// ============================================================
// Easing
// ============================================================

inline float gtEaseInQuad(float t)     { return t * t; }
inline float gtEaseOutQuad(float t)    { return t * (2.0f - t); }
inline float gtEaseInOutQuad(float t)  { return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; }
inline float gtEaseInCubic(float t)    { return t * t * t; }
inline float gtEaseOutCubic(float t)   { return (t - 1.0f) * (t - 1.0f) * (t - 1.0f) + 1.0f; }
inline float gtEaseInOutCubic(float t) { return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f; }
inline float gtEaseOutBounce(float t) {
    if (t < 1.0f / 2.75f) return 7.5625f * t * t;
    if (t < 2.0f / 2.75f) { t -= 1.5f / 2.75f; return 7.5625f * t * t + 0.75f; }
    if (t < 2.5f / 2.75f) { t -= 2.25f / 2.75f; return 7.5625f * t * t + 0.9375f; }
    t -= 2.625f / 2.75f; return 7.5625f * t * t + 0.984375f;
}

// ============================================================
// Object Pool helpers
// ============================================================

// Find a free slot in an active-based pool (where .active == false).
// Returns pointer to the slot, or NULL if pool is full.
// Usage: Bullet *b = gtPoolAlloc(bullets, MAX_BULLETS);
template <typename T>
inline T *gtPoolAlloc(T *pool, int maxN) {
    for (int i = 0; i < maxN; i++) {
        if (!pool[i].active) return &pool[i];
    }
    return NULL;
}

// Find a free slot in a life-based pool (where .life <= 0).
// Returns pointer to the slot, or NULL if pool is full.
template <typename T>
inline T *gtPoolAllocLife(T *pool, int maxN) {
    for (int i = 0; i < maxN; i++) {
        if (pool[i].life <= 0) return &pool[i];
    }
    return NULL;
}

// Iterate over active items in a pool (.active bool field).
// item becomes a reference to the current pool element.
// After processing one item, the inner loop sets _gti=maxN to terminate
// the outer loop, then the outer for increments _gti past maxN.
// Usage: GT_FOR_EACH(bullets, MAX_BULLETS, b) { b.x += b.vx * dt; }
#define GT_FOR_EACH(pool, maxN, item) \
    for (int _gti = 0; _gti < (maxN); _gti++) \
        if ((pool)[_gti].active) \
            for (auto &item = (pool)[_gti]; _gti < (maxN); _gti = (maxN))

// Iterate over alive items (where .life > 0).
#define GT_FOR_EACH_LIFE(pool, maxN, item) \
    for (int _gti = 0; _gti < (maxN); _gti++) \
        if ((pool)[_gti].life > 0) \
            for (auto &item = (pool)[_gti]; _gti < (maxN); _gti = (maxN))

// ============================================================
// CooldownTimer - fire-at-interval (shoot cooldown, spawn rate)
// ============================================================

struct gtCooldown {
    float interval;
    float remaining;

    void set(float sec) { interval = sec; remaining = 0; }

    bool fire(float dt) {
        remaining -= dt;
        if (remaining <= 0) {
            remaining = interval;
            return true;
        }
        return false;
    }

    bool ready() const { return remaining <= 0; }
    void reset() { remaining = 0; }
};

// ============================================================
// CountdownTimer - countdown to zero (combo timeout, respawn)
// ============================================================

struct gtCountdown {
    float duration;
    float remaining;

    void set(float sec) { duration = sec; remaining = sec; }

    bool tick(float dt) {
        remaining -= dt;
        if (remaining <= 0) { remaining = 0; return true; }
        return false;
    }

    bool expired() const { return remaining <= 0; }

    float progress() const {
        return duration > 0 ? 1.0f - remaining / duration : 1.0f;
    }

    float time() const { return remaining; }
    void restart() { remaining = duration; }
};

// ============================================================
// Screen Shake
// ============================================================

struct gtShake {
    float amplitude;
    int frames;
    float offsetX, offsetY;

    void trigger(float amp, int frameCount) {
        if (amp > amplitude) {
            amplitude = amp;
            frames = frameCount;
        }
    }

    void update() {
        if (frames > 0) {
            float a = amplitude * ((float)frames / 20.0f);
            offsetX = ((rand() % 200 - 100) / 100.0f) * a;
            offsetY = ((rand() % 200 - 100) / 100.0f) * a;
            frames--;
        } else {
            amplitude = 0; offsetX = 0; offsetY = 0;
        }
    }

    bool active() const { return frames > 0; }
    float ox() const { return offsetX; }
    float oy() const { return offsetY; }
};

// ============================================================
// Camera2D - smooth-follow with boundary clamping
// ============================================================

struct gtCamera2D {
    float x, y;
    float smooth;
    int screenW, screenH;
    int mapW, mapH;

    void init(int sw, int sh, int mw, int mh, float smoothFactor = 0.1f) {
        screenW = sw; screenH = sh;
        mapW = mw; mapH = mh;
        smooth = smoothFactor;
        x = (float)(mw - sw) / 2.0f;
        y = (float)(mh - sh) / 2.0f;
    }

    void follow(float targetX, float targetY) {
        float tx = gtClamp(targetX - screenW / 2.0f, 0, (float)(mapW - screenW));
        float ty = gtClamp(targetY - screenH / 2.0f, 0, (float)(mapH - screenH));
        x += (tx - x) * smooth;
        y += (ty - y) * smooth;
    }

    void snap(float targetX, float targetY) {
        x = gtClamp(targetX - screenW / 2.0f, 0, (float)(mapW - screenW));
        y = gtClamp(targetY - screenH / 2.0f, 0, (float)(mapH - screenH));
    }

    void reset() {
        x = (float)(mapW - screenW) / 2.0f;
        y = (float)(mapH - screenH) / 2.0f;
    }

    int screenX(float worldX) const { return (int)(worldX - x); }
    int screenY(float worldY) const { return (int)(worldY - y); }
};

// ============================================================
// Particle System
// ============================================================

struct gtParticle {
    float x, y, vx, vy;
    uint32_t color;
    float life, maxLife;
    float size;
};

struct gtParticles {
    gtParticle *pool;
    int maxCount;
    float friction;

    void bind(gtParticle *arr, int count, float frictionVal = 4.0f) {
        pool = arr; maxCount = count; friction = frictionVal;
        clear();
    }

    void clear() {
        for (int i = 0; i < maxCount; i++) pool[i].life = 0;
    }

    bool spawn(float x, float y, float vx, float vy,
               uint32_t color, float life, float size) {
        for (int i = 0; i < maxCount; i++) {
            if (pool[i].life <= 0) {
                gtParticle &p = pool[i];
                p.x = x; p.y = y; p.vx = vx; p.vy = vy;
                p.color = color; p.life = life; p.maxLife = life; p.size = size;
                return true;
            }
        }
        return false;
    }

    void explode(float cx, float cy, uint32_t color, int count,
                 float speedMin, float speedMax,
                 float lifeMin, float lifeMax,
                 float sizeMin, float sizeMax) {
        for (int i = 0; i < count; i++) {
            float a = gtRandomAngle();
            float spd = gtRandomFloat(speedMin, speedMax);
            spawn(cx, cy, (float)cos(a) * spd, (float)sin(a) * spd,
                  color, gtRandomFloat(lifeMin, lifeMax),
                  gtRandomFloat(sizeMin, sizeMax));
        }
    }

    void explode(float cx, float cy, uint32_t color, int count,
                 float speed = 200.0f, float life = 0.8f, float size = 3.5f) {
        explode(cx, cy, color, count, speed * 0.5f, speed * 1.5f,
                life * 0.4f, life, size * 0.7f, size * 1.5f);
    }

    void update(float dt) {
        float decay = gtClamp(1.0f - friction * dt, 0.0f, 1.0f);
        for (int i = 0; i < maxCount; i++) {
            if (pool[i].life > 0) {
                gtParticle &p = pool[i];
                p.life -= dt;
                p.vx *= decay; p.vy *= decay;
                p.x += p.vx * dt; p.y += p.vy * dt;
                if (p.life < 0) p.life = 0;
            }
        }
    }

    void draw(GameLib &g, float camX = 0, float camY = 0,
              float shakeX = 0, float shakeY = 0, bool glow = true) {
        for (int i = 0; i < maxCount; i++) {
            gtParticle &p = pool[i];
            if (p.life <= 0) continue;
            float alpha = p.life / p.maxLife;
            float s = p.size * alpha;
            if (s < 0.5f || s > 20.0f) continue;
            int sx = (int)(p.x - camX + shakeX);
            int sy = (int)(p.y - camY + shakeY);
            g.FillCircle(sx, sy, (int)s, p.color);
            if (glow && s > 1.5f && alpha > 0.2f) {
                uint32_t ga = (uint32_t)(alpha * 80);
                g.FillCircle(sx, sy, (int)(s * 2.5f),
                             COLOR_ARGB(ga, COLOR_GET_R(p.color),
                                        COLOR_GET_G(p.color), COLOR_GET_B(p.color)));
            }
        }
    }
};

// ============================================================
// FloatText - floating score/damage text
// ============================================================

struct gtFloatText {
    float x, y, vy;
    char text[16];
    uint32_t color;
    float life, maxLife;
};

struct gtFloatTexts {
    gtFloatText *pool;
    int maxCount;
    float riseSpeed;
    float holdTime;
    int textW, textH;

    void bind(gtFloatText *arr, int count,
              float rise = -60.0f, float hold = 0.5f,
              int charW = 10, int charH = 10) {
        pool = arr; maxCount = count;
        riseSpeed = rise; holdTime = hold;
        textW = charW; textH = charH;
        clear();
    }

    void clear() {
        for (int i = 0; i < maxCount; i++) pool[i].life = 0;
    }

    bool spawn(float x, float y, const char *str,
               uint32_t color, float life = 1.0f) {
        for (int i = 0; i < maxCount; i++) {
            if (pool[i].life <= 0) {
                gtFloatText &ft = pool[i];
                ft.x = x; ft.y = y; ft.vy = riseSpeed;
                strncpy(ft.text, str, sizeof(ft.text) - 1);
                ft.text[sizeof(ft.text) - 1] = '\0';
                ft.color = color; ft.life = life; ft.maxLife = life;
                return true;
            }
        }
        return false;
    }

    void update(float dt) {
        for (int i = 0; i < maxCount; i++) {
            if (pool[i].life > 0) {
                pool[i].life -= dt;
                pool[i].y += pool[i].vy * dt;
                if (pool[i].life < 0) pool[i].life = 0;
            }
        }
    }

    void draw(GameLib &g, float camX = 0, float camY = 0,
              float shakeX = 0, float shakeY = 0) {
        for (int i = 0; i < maxCount; i++) {
            gtFloatText &ft = pool[i];
            if (ft.life <= 0) continue;
            float elapsed = ft.maxLife - ft.life;
            float fadeTime = ft.maxLife - holdTime;
            float alpha;
            if (elapsed < holdTime) {
                alpha = 1.0f;
            } else {
                alpha = 1.0f - (elapsed - holdTime) / (fadeTime > 0 ? fadeTime : 0.01f);
                if (alpha < 0) alpha = 0;
            }
            int sx = (int)(ft.x - camX + shakeX);
            int sy = (int)(ft.y - camY + shakeY);
            uint32_t c = COLOR_ARGB((uint32_t)(alpha * 255),
                                     COLOR_GET_R(ft.color),
                                     COLOR_GET_G(ft.color),
                                     COLOR_GET_B(ft.color));
            g.DrawTextScale(sx + 1, sy + 1, ft.text,
                            COLOR_ARGB((uint32_t)(alpha * 100), 255, 255, 255),
                            textW, textH);
            g.DrawTextScale(sx, sy, ft.text, c, textW, textH);
        }
    }
};

// ============================================================
// Input Helpers
// ============================================================

inline bool gtMouseInRect(GameLib &g, int x, int y, int w, int h) {
    return GameLib::PointInRect(g.GetMouseX(), g.GetMouseY(), x, y, w, h);
}

inline bool gtAnyKeyPressed(GameLib &g) {
    for (int k = KEY_A; k <= KEY_Z; k++) if (g.IsKeyPressed(k)) return true;
    for (int k = KEY_0; k <= KEY_9; k++) if (g.IsKeyPressed(k)) return true;
    if (g.IsKeyPressed(KEY_SPACE))  return true;
    if (g.IsKeyPressed(KEY_ENTER))  return true;
    if (g.IsKeyPressed(KEY_LEFT))   return true;
    if (g.IsKeyPressed(KEY_RIGHT))  return true;
    if (g.IsKeyPressed(KEY_UP))     return true;
    if (g.IsKeyPressed(KEY_DOWN))   return true;
    if (g.IsMousePressed(MOUSE_LEFT)) return true;
    return false;
}

#endif // GAMETOOL_H_INCLUDED