# Geometry Wars - 技术实现规范

本文档记录代码层的技术实现细节。**玩法设计、敌人属性、道具行为、计分规则等**详见同目录 `design.md`，此处不再重复。

## 1. 项目结构

```
GeometryWars/
├── geometry.cpp      # 游戏源码（单文件）
├── design.md         # 玩法设计文档（敌人、道具、计分、操控）
├── spec.md           # 技术文档（本文档，仅记录代码实现细节）
├── assets.md         # 资源列表（音效素材索引）
├── patch.md          # 迭代补丁计划
└── assets/           # 音效资源
```

## 2. 编译配置

```bash
g++ -std=c++11 -O2 -Wall -Wextra -o geometry.exe geometry.cpp -mwindows
```

编译器路径：`D:\Dev\mingw32\bin\g++.exe`（需加入 PATH）。

技术约束：C++11 / GCC 4.9.2 / 仅依赖 GameLib.h（`#include "../GameLib.h"`）/ 不需链接参数。

## 3. 数据结构

### 3.1 核心结构体

```cpp
struct GridPt    { float x, y, vx, vy; };
struct Bullet    { float x, y, vx, vy; bool active; };
struct Enemy     { float x, y, vx, vy; int type; int hp; int maxHp; float r; float speed; bool active; float angle; float timer; };
struct Particle  { float x, y, vx, vy; uint32_t color; float life; float maxLife; float sz; };
struct FloatText { float x, y, vy; char text[16]; uint32_t color; float life; float maxLife; };
struct PowerUp   { float x, y, r; float life; float pulse; bool active; int type; }; // type 0=Nuke, 1=Energy
struct Star      { float x, y; uint32_t color; float drift; float sz; };
struct Popup     { char text[32]; uint32_t color; float life; float maxLife; int scale; };
struct LBEntry   { int score; float time; int kills; int combo; };
```

### 3.2 Black Hole

```cpp
#define MAX_BLACK_HOLES 2
struct BlackHole {
    float x, y;
    float radius;          // 引力半径（初始 100，随 absorbed 增长至 180）
    float pullStrength;    // 引力强度（120）
    float life;            // 剩余时间（初始 8s）
    int absorbed;
    bool active;
    float pulse;           // 动画计时
    bool exploding;
    float explodeTimer;    // 爆炸计时（0.5s）
};
```

### 3.3 跑马灯通知

```cpp
#define MAX_TICKER 4
struct TickerMsg {
    char text[100];
    uint32_t color;
    float life;            // 固定 3.0s
    float timer;
    bool active;
};
static TickerMsg tickerMsgs[MAX_TICKER];
```

API：`tickerAdd(text, color)` / `tickerUpdate(dt)` / `tickerDraw(g)`。详见 design.md §跑马灯。

### 3.4 场景 ID

```cpp
#define SCENE_TITLE     1
#define SCENE_COMBAT    2
#define SCENE_DEATH     3
#define SCENE_GAME_OVER 4
#define SCENE_LEADERBOARD 5
```

场景流程和各场景职责详见 design.md §游戏流程 / §场景状态机。

## 4. 关键常量

仅在 design.md 未列出或代码中硬编码的重要常量：

| 常量 | 值 | 说明 |
|------|-----|------|
| `SAVE_FILE` | `"geometry.sav"` | 存档文件名 |
| `LB_SIZE` | 10 | 排行榜条目数 |

其余常量（MAP_W, PLAYER_SPEED, BULLET_SPEED, SHOOT_RATE 等）见 design.md 各章节和代码顶部 `#define`。

## 5. 弹簧网格系统

**弹簧物理**（代码实现细节）：

```cpp
// 静止位置: fx = c * GRID_SPACING, fy = r * GRID_SPACING
g.vx += (fx - g.x) * 12.0f * dt;    // 弹簧刚度 12
float d = 1.0f - 5.0f * dt; if (d < 0.85f) d = 0.85f;  // 阻尼，最低保留 85%
g.vx *= d;
g.x += g.vx * dt * 60.0f;           // dt*60 缩放：60FPS 下物理正确
```

**冲击** (`gridImpulse`)：力随距离衰减 `f = strength * (1.0f - d / radius)`。

| 冲击源 | 半径 | 强度 |
|--------|------|------|
| 子弹命中 | ~40px | 50 + type*20 |
| 敌人死亡 | ~120px | 50 + type*20 |
| 玩家死亡 | ~500px | 500 |

网格外观参数见 design.md §弹簧网格背景。

## 6. 摄像机系统

```cpp
float tx = px - WIN_W / 2.0f, ty = py - WIN_H / 2.0f;
clamp(tx, 0, MAP_W - WIN_W);
clamp(ty, 0, MAP_H - WIN_H);
camX += (tx - camX) * 0.1f;  // 10% 每帧平滑插值
camY += (ty - camY) * 0.1f;
```

TITLE 场景不更新摄像机；DEATH/GAME_OVER 保持死亡位置。

## 7. 场景切换实现

所有重置逻辑统一通过 `resetGame()` 函数完成。`resetGame()` 重置：分数、连击、位置、敌人池、子弹池、粒子池、浮动文字池、道具池、网格、星空、生命、能量、弹出通知、成就标志、死亡原因、摄像机、震动。

场景计时器用 `game.IsSceneChanged()` 在切换时重置：

```cpp
case SCENE_DEATH: {
    static float deathTimer = 0;
    if (game.IsSceneChanged()) deathTimer = 0;
    deathTimer += fdt;
    ...
}
```

存档逻辑：DEATH→GAME_OVER 转换时插入排行榜、保存最佳纪录。

## 8. gameUpdate 子函数拆分

| 子函数 | 职责 |
|--------|------|
| `updatePlayer(g, dt)` | 玩家移动 + 瞄准 + BH 对玩家引力 |
| `updateShooting(g, dt)` | 射击逻辑（含能量散射） |
| `updateBullets(dt)` | 子弹移动 + 边界消失 |
| `enemiesUpdate(dt)` | 敌人 AI 移动 |
| `blackHolesUpdate(dt)` | BH 引力计算 + 吸入判定 |
| `updateCollisions(g)` | 子弹vs敌人 + 玩家vs敌人 + 玩家vs道具 + 子弹vs BH + 玩家vs BH |
| `updateTimers(dt)` | combo/energy/respawn/shake 计时器 |
| `updateSpawner(g, dt)` | 敌人生成 + 道具生成 + Jack + BH 生成 |

总调度顺序：
```cpp
static void gameUpdate(GameLib &g, float dt) {
    updatePlayer(g, dt);
    updateShooting(g, dt);
    updateBullets(dt);
    enemiesUpdate(dt);
    blackHolesUpdate(dt);
    updateCollisions(g);
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
```

## 9. 碰撞检测半径

碰撞检测的代码实现半径（design.md 描述碰撞概念，此处记录具体数值）：

| 碰撞对 | 公式 |
|--------|------|
| 子弹 vs 敌人 | `dist < enemy.r + 4` |
| 玩家 vs 敌人 | `dist < enemy.r + 8` |
| 玩家 vs 道具 | `dist < powerup.r + 12` |
| 子弹 vs BH 核心 | `dist < 30` |
| 玩家 vs BH 核心 | `dist < coreR + 8`（coreR = `15 + absorbed*2`，上限 35） |

## 10. 渲染顺序

每帧渲染（从后到前）：

```
1. Clear(COLOR_BLACK)
2. 星空 (starsDraw)
3. 弹簧网格 (gridDraw)
4. 地图边框 (drawMapBorder)
5. 子弹 (drawBullets)
6. Black Hole (blackHolesDraw)
7. 敌人 (drawEnemies)
8. 玩家 (drawPlayer)
9. 道具 (powerupsDraw)
10. 粒子 (particlesDraw)
11. 浮动文字 (floatTextsDraw)
12. 弹出通知 (popupDraw)
13. 跑马灯通知 (tickerDraw)
14. HUD (drawHUD)
15. Nuke FX (冲击波 + 闪白叠加，仅 nukeFxActive 时)
```

屏幕震动通过 `shakeX, shakeY` 偏移所有绘制坐标。

## 11. 屏幕震动实现

```cpp
static void shake(int amt, int frames) {
    if (amt > shakeAmt) { shakeAmt = (float)amt; shakeFrames = frames; }
}
// 只保留最大震动幅度（新震动不覆盖更强的旧震动）

float a = shakeAmt * ((float)shakeFrames / 20.0f);
shakeX = (float)((rand() % 200 - 100) / 100.0) * a;
shakeY = (float)((rand() % 200 - 100) / 100.0) * a;
shakeFrames--;
```

震动触发时机和幅度见 design.md §屏幕震动。

## 12. 主循环结构

```cpp
while (!game.IsClosed()) {
    double dt = game.GetDeltaTime();
    if (dt > 0.05) dt = 0.05;      // 防止低帧率时物理崩溃
    float fdt = (float)dt;

    if (game.IsKeyPressed(KEY_ESCAPE)) break;
    if (game.IsKeyPressed(KEY_F9)) invincible = !invincible;

    switch (game.GetScene()) {
        case SCENE_TITLE:       // 含 resetGame()
        case SCENE_COMBAT:      // 调用 gameUpdate
        case SCENE_DEATH:
        case SCENE_GAME_OVER:
        case SCENE_LEADERBOARD: // 含 resetGame()
    }

    if (game.GetScene() != SCENE_TITLE) cameraUpdate();
    game.Update();
}
```

不限帧（不调用 WaitFrame），用于测试 GameLib 渲染性能。

## 13. GameLib.h API 使用清单

| API | 使用场景 |
|-----|---------|
| `Open` | 创建窗口（可缩放） |
| `ShowFps` | 标题栏显示实时 FPS |
| `ShowMouse` | 显示鼠标光标 |
| `GetDeltaTime` | 帧间隔 |
| `GetTime` | 运行总时间（脉冲、闪烁） |
| `IsClosed` | 主循环条件 |
| `IsKeyPressed` | ESC, F5, F6, F7, F9, Enter |
| `IsKeyDown` | WASD 移动 |
| `IsMouseDown` | 左键射击 |
| `GetMouseX / GetMouseY` | 瞄准 |
| `SetScene / GetScene / IsSceneChanged` | 场景管理 |
| `Button` | START / CONTINUE 按钮 |
| `PlayWAV` | 播放音效 |
| `Clear` | 清屏 |
| `DrawLine` | 网格线、敌人轮廓 |
| `DrawRect` | 地图边框 |
| `FillCircle` | 粒子、敌人、子弹、光晕 |
| `DrawCircle` | 绕行者内环、冲击波、信标环 |
| `FillTriangle` | 玩家飞船 |
| `FillRect` | 死亡闪白 |
| `DrawText / DrawTextScale` | 文字 |
| `DrawPrintf / DrawPrintfScale` | 格式化文字 |
| `SaveInt / LoadInt` | 存档 |
| `SaveFloat / LoadFloat` | 存档 |
| `GetFPS` | HUD 右下角 FPS |

## 14. 存档数据

排行榜存档：`geometry.sav` 中 `lb_score0~9`, `lb_time0~9`, `lb_kills0~9`, `lb_combo0~9`（40 个 key）。按分数降序排列。

历史最佳：`bestScore`（LoadInt/SaveInt）、`bestTime`（LoadFloat/SaveFloat）。

## 15. 限制与注意事项

- 最大子弹数 150 / 敌人数 100 / 粒子数 800 / 浮动文字数 30 / 道具数 5
- 绕行者分裂可能瞬间激增敌人数（1→3）
- 弹簧网格 `dt*60.0f` 缩放因子适配 60FPS 物理效果
- 粒子摩擦 `(1.0f - 4.0f*dt)` 约 0.25s 减速一半
- `dt` 上限钳制 0.05s 防低帧率物理崩溃