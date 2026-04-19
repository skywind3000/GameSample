# Geometry Wars - 技术实现规范

本项目是 GameLib.h 的霓虹风格几何射击游戏，无尽模式。本文档记录完整的技术实现细节，便于在另一台机器上继续开发。

## 1. 项目结构

```
GeometryWars/
├── geometry.cpp      # 游戏源码（单文件，~2050 行）
├── design.md         # 游戏设计文档（玩法、敌人、计分、操控）
├── spec.md           # 技术文档（本文档）
├── assets.md         # 资源列表（音效素材索引）
├── patch.md          # 迭代补丁计划（6 个新功能的详细规划）
└── assets/           # 音效资源
    ├── explosion-01.wav ~ explosion-08.wav  # 爆炸音效（8个变体）
    ├── shoot-01.wav ~ shoot-04.wav          # 射击音效（4个变体）
    └── spawn-01.wav ~ spawn-08.wav          # 生成音效（8个变体）
```

## 2. 编译配置

### 编译命令

```bash
g++ -std=c++11 -O2 -Wall -Wextra -o geometry.exe geometry.cpp -mwindows
```

### 编译器路径

```
D:\Dev\mingw32\bin\g++.exe
```

需要将此目录加入环境变量 `PATH`。

### 技术约束

- **C++ 标准**：C++11（不能用 C++14/17/20）
- **编译器**：GCC 4.9.2（Dev-C++ 5 自带）
- **依赖**：仅 `GameLib.h`（通过 `#include "../GameLib.h"` 引入）
- **不需要**链接参数（如 `-lwinmm -lgdi32`），GameLib.h 通过动态加载解决所有依赖

## 3. 核心常量

### 尺寸

| 常量 | 值 | 说明 |
|------|-----|------|
| `MAP_W` | 1200 | 地图宽度（逻辑坐标） |
| `MAP_H` | 900 | 地图高度（逻辑坐标） |
| `WIN_W` | 800 | 窗口宽度（framebuffer） |
| `WIN_H` | 600 | 窗口高度（framebuffer） |
| `resizable` | true | 窗口可缩放 |
| `showFps` | true | 标题栏显示实时 FPS |

### 网格

| 常量 | 值 | 说明 |
|------|-----|------|
| `GRID_SPACING` | 25 | 网格间距（像素） |
| `GRID_COLS` | ~49 | 自动计算：`(MAP_W + GRID_SPACING - 1) / GRID_SPACING + 1` |
| `GRID_ROWS` | ~37 | 自动计算：`(MAP_H + GRID_SPACING - 1) / GRID_SPACING + 1` |

### 实体上限

| 常量 | 值 | 说明 |
|------|-----|------|
| `MAX_BULLETS` | 150 | 同时存在的最大子弹数 |
| `MAX_ENEMIES` | 100 | 同时存在的最大敌人数 |
| `MAX_PARTICLES` | 800 | 同时存在的最大粒子数 |
| `MAX_FLOAT_TEXTS` | 30 | 同时存在的最大浮动文字数 |
| `MAX_POWERUPS` | 5 | 同时存在的最大道具数 |
| `MAX_STARS_FAR` | 50 | 远层星空粒子数 |
| `MAX_STARS_NEAR` | 30 | 近层星空粒子数 |
| `LB_SIZE` | 10 | 排行榜条目数 |

### 游戏参数

| 常量 | 值 | 说明 |
|------|-----|------|
| `PLAYER_SPEED` | 250.0f | 玩家移动速度（px/s） |
| `BULLET_SPEED` | 810.0f | 子弹速度（px/s，已提升 35%） |
| `SHOOT_RATE` | 0.12f | 射击间隔（秒），约每秒 8.3 发 |
| `ENERGY_SHOOT_RATE` | 0.08f | 能量模式射击间隔（秒），约每秒 12.5 发 |
| `COMBO_TIMEOUT` | 2.0f | 连击超时（秒） |
| `ENERGY_DURATION` | 5.0f | 能量道具持续时间（秒） |
| `POPUP_LIFE` | 2.0f | 弹出通知总显示时间（秒） |
| `POPUP_GROW_TIME` | 0.2f | 弹出通知渐增时间（秒） |
| `POPUP_FADE_TIME` | 0.5f | 弹出通知渐隐时间（秒） |
| `MAX_LIVES` | 3 | 玩家初始生命数 |
| `RESPAWN_INVINCIBLE` | 2.0f | 重生无敌时间（秒） |
| `SPAWN_CLEAR_RADIUS` | 150.0f | 重生时清除敌人半径（px） |

### 存档

| 常量 | 值 | 说明 |
|------|-----|------|
| `SAVE_FILE` | `"geometry.sav"` | 存档文件名 |

**存档数据**：

| 变量 | 类型 | 说明 | 默认值 |
|------|------|------|--------|
| `bestScore` | `int` | 历史最高分 | 0 |
| `bestTime` | `float` | 历史最长存活时间（秒） | 0.0 |

**API 使用**：

| 操作 | API | 时机 |
|------|-----|------|
| 加载 | `GameLib::LoadInt(SAVE_FILE, "bestScore", 0)` | `main()` 初始化时 |
| 加载 | `GameLib::LoadFloat(SAVE_FILE, "bestTime", 0.0f)` | `main()` 初始化时 |
| 保存 | `GameLib::SaveInt(SAVE_FILE, "bestScore", bestScore)` | GAME_OVER 按 R 时（如破纪录） |
| 保存 | `GameLib::SaveFloat(SAVE_FILE, "bestTime", bestTime)` | GAME_OVER 按 R 时（如破纪录） |

### 场景 ID

| 常量 | 值 | 说明 |
|------|-----|------|
| `SCENE_TITLE` | 1 | 标题画面 |
| `SCENE_COMBAT` | 2 | 战斗（正常游戏） |
| `SCENE_DEATH` | 3 | 玩家死亡动画 |
| `SCENE_GAME_OVER` | 4 | 结算画面 |
| `SCENE_LEADERBOARD` | 5 | 排行榜画面 |

## 4. 数据结构

### 4.1 弹簧网格点

```cpp
struct GridPt {
    float x, y;   // 当前位置（偏离静止位置时产生扭曲）
    float vx, vy; // 速度（用于弹簧阻尼振荡）
};
```

**弹簧物理**：
- 静止位置：`fx = c * GRID_SPACING, fy = r * GRID_SPACING`
- 弹簧力：`vx += (fx - x) * 12.0f * dt`
- 阻尼：`vx *= (1.0f - 5.0f * dt)`，最小 0.85
- 位置更新：`x += vx * dt * 60.0f`
- 恢复时间：约 0.3~0.5 秒

### 4.2 子弹

```cpp
struct Bullet {
    float x, y;   // 位置
    float vx, vy; // 速度（沿瞄准方向）
    bool active;  // 是否活跃
};
```

### 4.3 敌人

```cpp
struct Enemy {
    float x, y;     // 位置
    float vx, vy;   // 速度
    int type;       // 类型：0=蜂群圆, 1=追踪三角, 2=弹跳方块, 3=绕行者, 4=蛇行者
    int hp;         // 当前血量
    int maxHp;      // 最大血量
    float r;        // 碰撞半径
    float speed;    // 基础速度
    bool active;    // 是否活跃
    float angle;    // 旋转角度（三角形/五边形朝向用）
    float timer;    // 计时器（漫游者换方向、蛇行者正弦相位）
};
```

**敌人属性表**：

| 类型 | 名称 | 半径 | 速度 | HP | 分值 | 颜色 |
|------|------|------|------|-----|------|------|
| 0 | 蜂群圆 (Swarm) | 10 | 60 | 1 | 50 | 橙色霓虹 (255,150,40) |
| 1 | 追踪三角 (Chaser) | 12 | 140 | 1 | 100 | 粉红霓虹 (255,80,140) |
| 2 | 弹跳方块 (Bouncer) | 14 | 100 | 2 | 150 | 绿色霓虹 (80,255,80) |
| 3 | 绕行者 (Orbiter) | 22 | 50 | 5 | 300 | 紫色霓虹 (160,60,220) |
| 4 | 蛇行者 (Weaver) | 12 | 120 | 2 | 200 | 黄色霓虹 (255,220,50) |

### 4.4 粒子

```cpp
struct Particle {
    float x, y;       // 位置
    float vx, vy;     // 速度
    uint32_t color;   // 颜色（ARGB）
    float life;       // 当前生命值（秒）
    float maxLife;    // 最大生命值（秒）
    float sz;         // 尺寸（像素）
};
```

### 4.5 浮动文字

```cpp
struct FloatText {
    float x, y;       // 位置
    float vy;         // 向上飘动速度（固定 -60px/s）
    char text[16];    // 显示文字（如 "+150"）
    uint32_t color;   // 颜色
    float life;       // 当前生命值（秒）
    float maxLife;    // 最大生命值（秒，固定 1.0s）
};
```

**行为**：
- 敌人死亡时在命中位置生成，显示 `+得分`（含连击倍率）
- 以 60px/s 速度向上飘动
- 1 秒生命周期，alpha 渐隐
- 最多同时存在 30 个

### 4.6 道具

```cpp
struct PowerUp {
    float x, y;       // 位置
    float r;          // 碰撞半径（15px）
    float life;       // 剩余时间（秒，最大 8s）
    float pulse;      // 脉冲动画计时
    bool active;      // 是否活跃
    int type;         // 类型：0=清屏(Nuke), 1=能量(Energy)
};
```

**清屏道具 (Nuke)**（type 0）：
- **生成**：每 14~22 秒随机生成，距离玩家 > 200px
- **外观**：霓虹青色菱形，脉冲呼吸动画，两圈向外扩散的信标环（`DrawCircle`，从半径15→45px扩展，alpha 120→0衰减，周期1.5s，两圈间隔1s错开），上方显示 "NUKE" 标签文字（`DrawText`，alpha 180），剩余 3 秒时闪烁
- **触发**：玩家碰撞检测（距离 < 道具半径 + 12px）
- **效果** (`triggerNuke`)：
  1. 弹出 "NUKE ACTIVATED!" 通知
  2. 播放爆炸音效
  3. 屏幕闪白（`FillRect` 全屏白色叠加，alpha 220 → 0，衰减速率 2.0/s，约 0.5s 复原）
  4. 冲击波圆（以玩家为中心，双环 `DrawCircle`，半径以 1200px/s 扩大，alpha 以 2.5/s fade out）
  5. 屏幕强震（8px，20帧）
  6. 网格强力冲击（半径 600，强度 400）
  7. 每个敌人独立爆炸 + 得分飘字
  8. 玩家位置白色大闪光（50粒子）

**能量道具 (Energy)**（type 1）：
- **掉落**：击杀敌人时概率掉落——蜂群圆 7%、追踪三角 10%、弹跳方块 12%、绕行者 17%、蛇行者 10%。活跃敌人数超过 15 时概率递减（每多5个敌人约减10%，最低30%）；100px 内已有能量道具时概率减半
- **外观**：橙金色菱形（核心 `(255,180,60)`，光晕 `(255,120,30)`），偏暖金而非纯黄，与蜂群圆橙色(255,150,40)和蛇行者黄色(255,220,50)拉开距离。脉冲呼吸动画，两圈向外扩散的信标环（`DrawCircle`，从半径15→45px扩展，alpha 120→0衰减，周期1.5s，两圈间隔1s错开），上方显示 "ENERGY" 标签文字（`DrawText`，alpha 180），剩余 3 秒时闪烁
- **触发**：玩家碰撞检测（距离 < 道具半径 + 12px）
- **效果**（持续 5 秒）：
  1. 弹出 "SPREAD SHOT!" 通知 + 震动（3px, 8帧）
  2. 射击变为 5 发扇形散射（角度偏移 -15°, -7°, 0°, +7°, +15°）
  3. 射速提升至 0.08s 间隔
  4. 玩家金色光晕覆盖
  5. HUD 显示金色能量条
- **重生时重置**：失去一命时 `energyActive = false`

### 4.7 星空粒子

```cpp
struct Star { float x, y; uint32_t color; float drift; float sz; };
```

**两层星空**：
- 远层（50颗）：sz=1~2px, alpha=30~60, drift=5~10px/s 向下, 颜色白/淡蓝/淡青
- 近层（30颗）：sz=2~3px, alpha=60~100, drift=15~25px/s 向下, 颜色白/淡蓝/淡青
- 漂出地图底部后从顶部重新出现

### 4.8 弹出通知

```cpp
struct Popup { char text[32]; uint32_t color; float life; float maxLife; int scale; };
```

**动画**：
- 前 0.2s：scale 从 1 渐增到目标值（整数阶梯）
- 中间段：保持目标 scale
- 最后 0.5s：alpha 渐隐
- 屏幕中央显示，每局每种成就只触发一次

### 4.9 跑马灯事件通知（Ticker）

```cpp
#define MAX_TICKER 4
struct TickerMsg {
    char text[100];     // 事件描述文字（最长 100 字符，超出截断）
    uint32_t color;     // 文字颜色（ARGB）
    float life;         // 总显示时间（秒，固定 3.0s）
    float timer;        // 已显示时间（秒）
    bool active;        // 是否活跃
};
static TickerMsg tickerMsgs[MAX_TICKER];
```

**短语池**：

```cpp
static const char *jackPhrases[] = {  // 5 条，随机选择
    "THE SWARM RISES FROM EVERY CORNER!",
    "SHADOWS CONVERGE - ENEMIES SURGING FROM ALL SIDES!",
    "THE HIVE HAS AWAKENED - INVASION INBOUND!",
    "FOUR WALLS BREACHED - DENSITY CRITICAL!",
    "NOWHERE TO HIDE - THEY COME FROM EVERYWHERE!"
};
static const char *bhPhrases[] = {     // 5 条，随机选择
    "GRAVITATIONAL ANOMALY DETECTED - CAUTION ADVISED!",
    "A SINGULARITY FORMS - ALL TRAJECTORIES BENDING!",
    "THE VOID AWAKENS - SPACE ITSELF IS WARPING!",
    "GRAVITY WELL ACTIVE - NOTHING ESCAPES ITS PULL!",
    "DIMENSIONAL RIFT - REALITY COLLAPSING INWARD!"
};
```

**API**：

| 函数 | 说明 |
|------|------|
| `tickerAdd(text, color)` | 添加一条消息到队列。用 `strncpy` 截断保护（99 字符 + `\0`）。找空闲 slot；全满时替换最接近结束的 |
| `tickerUpdate(dt)` | 更新所有活跃 slot 的 timer，超时后 `active = false` |
| `tickerDraw(g)` | 绘制所有活跃 slot |

**动画**：
- 滑入阶段（0.4s）：从屏幕右侧外滑入到 x=20 位置
- 保持阶段（2.1s）：停留在 x=20
- 滑出阶段（0.5s）：从 x=20 向左滑出屏幕外
- 滑出时 alpha 从 200 渐隐到 0
- 多条消息垂直排列：y = 35 + i * 15

**调用方**：
- Jack 涌入触发时：`tickerAdd(jackPhrases[rand()%5], COLOR_ARGB(255, 255, 80, 60))`（霓虹红色）
- Black Hole 爆炸时：`tickerAdd(bhPhrases[rand()%5], COLOR_ARGB(255, 200, 60, 80))`（深红色）
- F6 调试触发 Jack：同上
- `resetGame()`：`for (i=0..MAX_TICKER) tickerMsgs[i].active = false`

### 4.10 排行榜条目

```cpp
struct LBEntry { int score; float time; int kills; int combo; };
```

**存储**：`geometry.sav` 中 lb_score0~9, lb_time0~9, lb_kills0~9, lb_combo0~9（40 个 key）
**排序**：按分数降序

## 5. 场景状态机

使用 GameLib.h 的 `SetScene()` / `GetScene()` / `IsSceneChanged()` 机制管理游戏大状态。

### 5.1 场景转换图

```
SCENE_TITLE (1)
    │ Enter 键              │ L 键
    ▼                        ▼
SCENE_COMBAT (2) ─── 玩家最终死亡 ──→ SCENE_DEATH (3)
    │                                    │ 1.5秒后
    │                                    ▼
    │                              SCENE_GAME_OVER (4)
    │                                    │ Space 键
    └────────────────────── SCENE_LEADERBOARD (5) ←──┘
                                         │ Space 键
                                         ▼
                                    SCENE_TITLE (1)
```

### 5.2 各场景职责

#### SCENE_TITLE - 标题画面
- **更新**：弹簧网格（自动波动）、星空漂移
- **绘制**：星空 → 网格 → 标题文字 → 操作说明 → 历史纪录 → "Powered by GameLib"
- **输入**：Enter → 重置游戏数据，切换到 SCENE_COMBAT；L → 切换到 SCENE_LEADERBOARD
- **摄像机**：不更新

#### SCENE_COMBAT - 战斗
- **更新**：调用 `gameUpdate()` 处理所有游戏逻辑（含道具生成、浮动文字、道具碰撞、能量计时、弹出通知、重生无敌）
- **绘制**：星空 → 网格 → 地图边框 → 子弹 → 敌人 → 玩家 → 道具 → 粒子 → 浮动文字 → 弹出通知 → HUD
- **输入**：WASD 移动、鼠标瞄准、左键射击、道具拾取
- **摄像机**：跟随玩家

#### SCENE_DEATH - 玩家死亡动画
- **更新**：粒子、网格、星空、屏幕震动（不调用 `gameUpdate()`）
- **绘制**：星空 → 黑色背景 → 网格 → 地图边框 → 粒子 → 白色闪光（前 0.5 秒）
- **自动转换**：1.5 秒后 → SCENE_GAME_OVER（同时插入排行榜、保存最佳纪录）
- **音效**：播放 explosion.wav（第 1 秒内）
- **摄像机**：跟随死亡位置

#### SCENE_GAME_OVER - 结算画面
- **更新**：网格、星空、粒子
- **绘制**：星空 → 黑色背景 → 网格 → 粒子 → GAME OVER 文字 → 统计数据 → 死亡原因 → 弹出通知 → "PRESS SPACE FOR LEADERBOARD"（闪烁）
- **输入**：Space 键 → 切换到 SCENE_LEADERBOARD
- **显示数据**：最终分数、存活时间、总击杀数、最高连击倍率、死亡原因（敌人名+颜色+图标）
- **存档逻辑**：排行榜插入和最佳纪录保存已在 DEATH→GAME_OVER 转换时完成
- **摄像机**：保持在死亡位置

#### SCENE_LEADERBOARD - 排行榜
- **更新**：网格、星空
- **绘制**：星空 → 网格 → 排行榜标题 → RANK/SCORE/KILLS/TIME 列头 → 10 行数据 → "PRESS SPACE TO RETURN"（闪烁）
- **输入**：Space 键 → 切换到 SCENE_TITLE
- **摄像机**：居中（`camX = (MAP_W - WIN_W) / 2, camY = (MAP_H - WIN_H) / 2`）
- **高亮**：本次成绩行用金色显示（`lbHighlight` 索引）

### 5.3 场景切换时的初始化

所有重置逻辑统一通过 `resetGame()` 函数完成，消除重复代码。

- **TITLE → COMBAT**：调用 `resetGame()` + `game.SetScene(SCENE_COMBAT)`
- **COMBAT → DEATH**：玩家最终死亡时触发（`playerAlive = false`，`lives <= 0`）
- **DEATH → GAME_OVER**：自动（1.5 秒后），插入排行榜、保存最佳纪录、播放 game_over.wav
- **GAME_OVER → LEADERBOARD**：按 Space 键
- **LEADERBOARD → TITLE**：按 Space 键，调用 `resetGame()`（含清除震动状态）

**`resetGame()` 函数**：重置分数、连击、位置、敌人池、子弹池、粒子池、浮动文字池、道具池、网格、星空、生命、能量、弹出通知、成就标志、死亡原因、摄像机、震动。

### 5.4 场景计时器

使用 `game.IsSceneChanged()` 在场景切换时重置静态计时器：

```cpp
case SCENE_DEATH: {
    static float deathTimer = 0;
    if (game.IsSceneChanged()) deathTimer = 0;  // 场景切换时重置
    deathTimer += fdt;
    ...
}

case SCENE_GAME_OVER: {
    static float goTimer = 0;
    if (game.IsSceneChanged()) goTimer = 0;  // 场景切换时重置
    goTimer += fdt;
    ...
}
```

## 6. 核心系统实现

### 6.1 弹簧网格系统

**初始化** (`gridInit`)：
- 将每个网格点的 `x, y` 设置为静止位置（`c * GRID_SPACING, r * GRID_SPACING`）
- `vx, vy` 初始化为 0

**更新** (`gridUpdate`)：
```cpp
// 弹簧力拉回静止位置
g.vx += (fx - g.x) * 12.0f * dt;
g.vy += (fy - g.y) * 12.0f * dt;
// 阻尼衰减
float d = 1.0f - 5.0f * dt; if (d < 0.85f) d = 0.85f;
g.vx *= d; g.vy *= d;
// 位置更新
g.x += g.vx * dt * 60.0f;
g.y += g.vy * dt * 60.0f;
```

**冲击** (`gridImpulse`)：
- 对半径 `radius` 内的所有网格点施加冲击力
- 力的大小随距离衰减：`f = strength * (1.0f - d / radius)`
- 沿冲击方向添加速度：`vx += (dx / d) * f`

**绘制** (`gridDraw`)：
- 用 `DrawLine` 连接相邻网格点
- 颜色：`COLOR_ARGB(70, 50, 50, 130)`（低亮度蓝紫色，带透明）
- 应用摄像机偏移和屏幕震动

**冲击源**：
| 事件 | 半径 | 强度 |
|------|------|------|
| 子弹命中 | ~40px | 50 + type * 20 |
| 敌人死亡 | ~120px | 50 + type * 20 |
| 玩家死亡 | ~500px | 500 |

### 6.2 粒子系统

**生成** (`spawnParticle`)：
- 查找第一个 `life <= 0` 的空闲槽位
- 设置位置、速度、颜色、生命值、尺寸

**爆炸** (`spawnExplosion`)：
- 在 360 度均匀分布 `count` 个粒子
- 速度范围：80~360 px/s（随机）
- 生命值：0.4~1.2 秒（随机）
- 尺寸：3~6 像素（随机）

**绘制效果**（三层渲染）：
1. **核心粒子**：`FillCircle` 绘制主体
2. **光晕**：`FillCircle` 绘制 2.5 倍大小的半透明光晕（`alpha * 80`）
3. **高亮核心**：新粒子（`alpha > 0.6f`）额外绘制 1.3 倍白色核心（`alpha * 120`）

**物理**：
- 摩擦力减速：`vx *= (1.0f - 4.0f * dt)`
- 尺寸随生命衰减：`s = sz * alpha`（`alpha = life / maxLife`）

### 6.3 敌人 AI

#### 蜂群圆 (Type 0)
- **行为**：缓慢飘向玩家（加速度朝向玩家）
- **物理**：`vx += (dx / d) * speed * 2.0f * dt`，阻尼 `1.0f - 3.0f * dt`（最小 0.7）
- **边界**：钳制在地图内

#### 追踪三角 (Type 1)
- **行为**：快速追踪玩家，转向灵活
- **物理**：`vx += (dx / d) * speed * 4.0f * dt`，速度上限 `speed * 1.2f`
- **角度**：`angle = atan2(vy, vx)`（用于绘制朝向）
- **边界**：钳制在地图内

#### 弹跳方块 (Type 2)
- **行为**：匀速直线运动，不追踪玩家
- **初始化**：随机方向 `vx = cos(a) * speed, vy = sin(a) * speed`，独立随机初始旋转角度 `angle`
- **旋转**：每帧 `angle -= 4π * dt`（逆时针 2 圈/秒）
- **边界**：碰到地图边界反弹（`vx = -vx` 或 `vy = -vy`）

#### 绕行者 (Type 3)
- **行为**：绕玩家做弧线运动，保持约 180px 跑离环绕
- **物理**：径向力（保持距离）`vx += (dx / d) * radialForce * speed * 2.0f * dt`；切向力（环绕）`vx += cos(tangAngle) * speed * 3.0f * dt`，阻尼 `1.0f - 3.0f * dt`（最小 0.8）
- **角度**：持续旋转 `angle += 2.0f * dt`（用于内环装饰动画）
- **特殊**：死亡时分裂为 3 个漫游者（间隔 120 度，距离 20px）
- **边界**：钳制在地图内

#### 蛇行者 (Type 4)
- **行为**：蛇形曲线移动，沿初始方向做正弦波动穿过地图
- **初始化**：随机初始方向 `angle`，随机初始正弦相位 `timer`，初始速度沿 `angle` 方向
- **物理**：`vx = cos(baseAngle + sin(timer * 3.0f) * 0.8f) * speed`，正弦偏移 ±0.8 弧度（约 ±46°）
- **边界**：碰到地图边界反弹并更新 `angle = atan2(vy, vx)`（新反弹方向）

### 6.4 敌人生成系统

**生成位置** (`spawnFromEdge`)：
- 从地图四条边随机选择一边（`rand() % 4`）
- 在边上随机选择位置（带 `margin = 80` 偏移）
- 确保与玩家距离 > 250px（最多尝试 20 次）

**生成节奏**（连续递增，无上限封顶）：

- **spawnInterval**：`max(0.10f, 0.35f - gameTime * 0.001f)`，线性递减，最低 0.10s
- **maxOnScreen**：`min(60, 15 + (int)(gameTime / 10))`，每 10 秒 +1，上限 60

| 游戏时间 | spawnInterval | maxOnScreen | 可用类型及比例 |
|----------|--------------|-------------|---------------|
| 0~15s | 0.35s | 15 | 蜂群圆 100% |
| 15~30s | 0.30s | 18 | 蜂群圆 75% + 弹跳方块 25% |
| 30~60s | 0.25s | 21 | 蜂群圆 50% + 追踪三角 30% + 弹跳方块 20% |
| 60~90s | 0.22s | 24 | + 蛇行者 20% |
| 90~120s | 0.20s | 27 | + 绕行者 10% |
| 180s | 0.17s | 33 | 全类型混合 |
| 300s+ | 0.10s | 45+ | |

**类型分布逻辑**：使用 `rand() % 10` 区间分配，各阶段比例不同

### 6.5 突发事件系统

#### Jack 涌入事件

**触发条件**：`jackTimer >= max(30, 45 - gameTime * 0.01)`

**数据结构**：
```cpp
static float jackTimer, jackSpawnTimer;
static bool jackActive;
static int jackCount, jackTotal;
```

**机制**：
- 触发时 `showPopup("JACK INVASION!")`，`shake(5, 10)`
- 暂停常规敌人生成（`updateSpawner` 中 `if (jackActive) return`）
- 从四角涌入蜂群圆（type 0），0.05s 间隔，数量 `min(40, 20 + gameTime/30)`
- 涌入的敌人全部计分（含连击）
- 完成后恢复常规生成

#### Black Hole 引力场

**数据结构**：
```cpp
#define MAX_BLACK_HOLES 2
struct BlackHole {
    float x, y;           // 位置
    float radius;          // 引力半径（初始 100，随 absorbed 增长至 180）
    float pullStrength;    // 引力强度（120）
    float life;            // 剩余时间（初始 8s）
    int absorbed;          // 吸入敌人计数
    bool active;
    float pulse;           // 动画计时
    bool exploding;        // 爆炸阶段
    float explodeTimer;    // 爆炸计时（0.5s）
};
```

**生成**：`bhSpawnTimer >= 60~90s`，`gameTime > 60s`，`dist(bh, player) > 250`，最多 2 个

**引力计算**：
- 敌人：`vx += (dx/d) * pullStrength * dt`（在 `blackHolesUpdate` 中）
- 玩家：`px += (dx/d) * pullStrength * 0.3f * dt`（在 `updatePlayer` 中）

**吸入**：`dist(enemy, bh) < 20` → enemy 消灭，absorbed++，计分（含连击）

**爆炸**：`absorbed >= 10` → `exploding = true`，紫色粒子 30 个 + 震动 5px 12帧 + 释放 8 个蜂群圆 + 网格冲击 + "BLACK HOLE BURST!"

**自然消失**：`life <= 0 && !exploding` → 10 粒子消散效果

**碰撞**：
- 子弹 vs BH：`dist(bullet, bh) < 30` → absorbed++，3 个紫白火花，子弹消失
- 玩家 vs BH 核心：`dist(player, bh) < coreR + 8` → 死亡（同碰敌人）

**绘制** (`blackHolesDraw`)：
- 引力环：`DrawCircle` 半透明紫色脉冲，半径 = `radius * pulseScale`
- 核心：`FillCircle` 暗紫色 `(120,40,160)` 半径 `15 + absorbed * 2`（上限 35）+ 外光晕 `(80, 160,60,220)`
- 中心亮点：`FillCircle(3)` 白紫色 `alpha = 100 + absorbed * 15`
- 爆炸：`DrawCircle` 双环冲击波扩展

### 6.6 碰撞检测

#### 子弹 vs 敌人
- **检测**：圆形碰撞，`dist(bullet, enemy) < enemy.r + 4`
- **效果**：
  - **命中火花**：从命中点生成 5~8 个白色高亮火花，沿子弹反方向反弹（±50° 散射，150~400 px/s，0.3~0.7s 寿命，3~6px 尺寸）
  - 子弹消失
  - 敌人 HP -1
  - HP <= 0：触发击杀爆炸、网格冲击、屏幕震动、计分、浮动得分文字
  - 绕行者（type 3）死亡时分裂为 3 个漫游者

#### 玩家 vs 敌人
- **检测**：圆形碰撞，`dist(player, enemy) < enemy.r + 8`
- **效果**（无敌模式跳过）：
  - `playerAlive = false`
  - 切换到 `SCENE_DEATH`
  - 清除所有敌人
  - 触发巨型死亡爆炸（3 组粒子 + 强力网格冲击 + 强震动）

#### 玩家 vs 道具
- **检测**：圆形碰撞，`dist(player, powerup) < powerup.r + 12`
- **效果**：
  - 触发道具效果（清屏道具 = `triggerNuke()`）
  - 道具消失

#### 子弹 vs Black Hole
- **检测**：圆形碰撞，`dist(bullet, bh) < 30`
- **效果**：
  - `absorbed++`，3 个紫白火花向 BH 方向
  - 子弹消失
  - absorbed >= 10 时触发爆炸

#### 玩家 vs Black Hole 核心
- **检测**：圆形碰撞，`dist(player, bh) < coreR + 8`（coreR = `15 + absorbed * 2`，上限 35）
- **效果**（无敌模式跳过）：同碰敌人——失去 1 命或最终死亡
- `killedByType = -1`（死于 BH，不是敌人类型）

### 6.6 屏幕震动

**触发** (`shake`)：
```cpp
static void shake(int amt, int frames) {
    if (amt > shakeAmt) { shakeAmt = (float)amt; shakeFrames = frames; }
}
```
- 只保留最大震动幅度（新震动不会覆盖更强的旧震动）

**更新**：
```cpp
float a = shakeAmt * ((float)shakeFrames / 20.0f);
shakeX = (float)((rand() % 200 - 100) / 100.0) * a;
shakeY = (float)((rand() % 200 - 100) / 100.0) * a;
shakeFrames--;
```

**震动级别**：
| 事件 | 幅度 | 帧数 |
|------|------|------|
| 普通敌人击杀 | 1~2px | 3~4 帧 |
| 绕行者击杀 | 2px | 5 帧 |
| 玩家死亡 | 10px | 25 帧 |

### 6.7 摄像机系统

**跟随逻辑**：
```cpp
float tx = px - WIN_W / 2.0f, ty = py - WIN_H / 2.0f;
clamp(tx, 0, MAP_W - WIN_W);
clamp(ty, 0, MAP_H - WIN_H);
camX += (tx - camX) * 0.1f;  // 平滑插值（10% 每帧）
camY += (ty - camY) * 0.1f;
```

- 玩家始终大致在屏幕中央
- 摄像机在地图范围内漫游，不会超出边界
- **TITLE 场景**：不更新摄像机
- **DEATH/GAME_OVER 场景**：摄像机保持在死亡位置

### 6.8 计分与连击

**计分**：
- 击杀得分 = `基础分 * 连击倍率`
- 基础分：蜂群圆 50、追踪三角 100、弹跳方块 150、绕行者 300、蛇行者 200

**连击规则**：
- 每次击杀：`combo++`，`comboTimer = 2.0f`
- 2 秒无击杀：`combo = 1`（重置）
- 最高倍率：x10（代码未硬性限制，但实际很难超过）
- 记录 `highestCombo`（历史最高）

**HUD 显示**：
- 左上角：`SCORE: xxx`（白色）
- 顶部中央：`BEST: xxx`（金色，历史最高分）
- 顶部中央偏右：`xCombo`（黄色，2 倍放大，仅 combo > 1 时显示）
- 右上角：`TIME M:SS`（天蓝色）

### 6.9 玩家绘制

**组成**（5 层）：
1. **拖尾**：最近 12 帧位置的半透明三角形副本（`FillTriangle`），alpha 从 100（最新）到 0（最旧）线性衰减，尺寸从 0.7 倍到 1.0 倍缩放，颜色 `(0, 200, 255)`
   - 拖尾历史：每 0.03s 记录一次位置和角度，最多 12 条记录
   - 停止移动时拖尾追上来与主角重合并消失
2. **光晕**：`FillCircle` 半径 20，`COLOR_ARGB(60, 0, 255, 255)`；能量模式叠加 `COLOR_ARGB(80, 255, 200, 50)`
3. **外层轮廓**：`DrawTriangle`，比核心三角形大 1.3 倍，`COLOR_ARGB(160, 100, 255, 255)`（半透明霓虹描边）
4. **核心形状**：`FillTriangle`，顶点朝鼠标方向，`COLOR_ARGB(240, 0, 255, 255)`
   - 前顶点：`(cos(a) * 18, sin(a) * 18)`
   - 后两顶点：从后方偏移 10px，左右各展开 10px（角度 ±1.3 弧度）
5. **引擎光晕**：`FillCircle` 半径 4，后方 12px 处，`COLOR_ARGB(150, 255, 200, 50)`

**推进粒子**：移动时约每 2 帧从尾部喷射 1 个火花粒子（`spawnParticle`，0.06s 间隔）
- 方向：尾部方向 ±30° 散射
- 速度：350~600 px/s（远距离喷射）
- 颜色：围绕火花色（青色/浅蓝/白色），每个粒子 R/G/B 各有 ±50 随机偏移；能量模式下偏暖白 `(255, 240, 200~255)`
- 寿命：0.35~0.55s，尺寸 2~3px
- 停止移动时停止喷射

### 6.10 子弹绘制

**组成**（3 层）：
1. **核心**：`FillCircle` 半径 3，`COLOR_ARGB(255, 200, 255, 200)`
2. **光晕**：`FillCircle` 半径 6，`COLOR_ARGB(100, 100, 255, 100)`
3. **拖尾**：`FillCircle` 半径 2，位置偏移 `-vx * 0.008f`，`COLOR_ARGB(80, 100, 200, 100)`

### 6.11 音效系统

**音效路径**：所有音效文件位于 `assets/` 目录下。

**音效映射**（使用 `assets/` 下的变体文件，随机选择）：

| 事件 | 音效文件 | 触发时机 |
|------|---------|---------|
| 射击 | `shoot-01.wav` ~ `shoot-04.wav` | 每次发射子弹时（4 个变体随机） |
| 敌人击杀 | `explosion-01.wav` ~ `explosion-08.wav` | 敌人 HP 归零时（8 个变体随机） |
| 敌人生成 | `spawn-01.wav` ~ `spawn-08.wav` | 每次从边缘生成敌人时（8 个变体随机） |
| 玩家死亡 | `explosion-01.wav` | SCENE_DEATH 前 1 秒内 |
| 游戏结束 | `explosion-02.wav` | 进入 SCENE_GAME_OVER 时 |
| 开始游戏 | `spawn-01.wav` | 进入 SCENE_COMBAT 时 |

## 7. 渲染顺序

每帧的渲染顺序（从后到前）：

```
1. Clear(COLOR_BLACK)
2. 星空（starsDraw）
3. 弹簧网格（gridDraw）
4. 地图边框（drawMapBorder）
5. 子弹（drawBullets）
6. Black Hole（blackHolesDraw）
7. 敌人（drawEnemies）
8. 玩家（drawPlayer，仅存活时，含拖尾、外层轮廓、重生闪烁和能量金光）
9. 道具（powerupsDraw，含 Nuke 和 Energy）
10. 粒子（particlesDraw）
11. 浮动文字（floatTextsDraw）
12. 弹出通知（popupDraw）
13. 跑马灯通知（tickerDraw）
14. HUD（drawHUD，含生命、能量条、右下角 FPS）
15. Nuke FX（冲击波圆 + 闪白叠加，仅在 nukeFxActive 时绘制）
```

**特殊效果**：
- 白色闪光（DEATH 场景前 0.5 秒）：在网格和粒子之上绘制半透明白色矩形
- 屏幕震动：通过 `shakeX, shakeY` 偏移所有绘制坐标

## 8. 主循环结构

```cpp
while (!game.IsClosed()) {
    double dt = game.GetDeltaTime();
    if (dt > 0.05) dt = 0.05;
    float fdt = (float)dt;

    if (game.IsKeyPressed(KEY_ESCAPE)) break;
    if (game.IsKeyPressed(KEY_F9)) invincible = !invincible;

    // 场景状态机
    switch (game.GetScene()) {
        case SCENE_TITLE:       // 标题画面（含 resetGame() 调用）
        case SCENE_COMBAT:      // 战斗（调用 gameUpdate）
        case SCENE_DEATH:       // 死亡动画
        case SCENE_GAME_OVER:   // 结算
        case SCENE_LEADERBOARD: // 排行榜（含 resetGame() 调用）
    }

    if (game.GetScene() != SCENE_TITLE) cameraUpdate();
    game.Update();
}
```

### gameUpdate 子函数拆分

`gameUpdate()` 从原先的 280 行单体函数拆分为 6 个子函数：

| 子函数 | 职责 |
|--------|------|
| `updatePlayer(g, dt)` | 玩家移动 + 瞄准 + Black Hole 对玩家引力 |
| `updateShooting(g, dt)` | 射击逻辑（含能量散射） |
| `updateBullets(dt)` | 子弹移动 + 边界消失 |
| `enemiesUpdate(dt)` | 敌人 AI 移动 |
| `updateCollisions(g)` | 子弹vs敌人 + 玩家vs敌人 + 玩家vs道具 + 子弹vs BH + 玩家vs BH 碰撞 |
| `updateTimers(dt)` | combo/energy/respawn/shake 计时器 |
| `updateSpawner(g, dt)` | 敌人生成 + 道具生成 + Jack 涌入 + Black Hole 生成 |

总调度：
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

## 9. 已知特性与注意事项

### 9.1 调试功能
- **F9**：切换无敌模式（开启后不会被敌人/弹幕伤害）
- **F5**：无条件触发 Nuke（测试闪白+冲击波视觉效果）
- **F6**：触发 Jack 涌入事件（测试蜂群涌入效果）
- **F7**：触发 Black Hole 引力场（在玩家右侧 200px 生成一个 BH）

### 9.2 限制
- 最大子弹数 150，超过后无法射击（会等待空闲槽位）
- 最大敌人数 100，超过后无法生成新敌人
- 最大粒子数 800，超过后无法生成新粒子
- 最大浮动文字数 30，超过后不显示新文字
- 最大道具数 5，超过后不生成新道具
- 绕行者分裂可能导致瞬间敌人数激增（1 个绕行者 → 3 个蜂群圆）

### 9.3 物理注意事项
- 弹簧网格的 `dt * 60.0f` 缩放因子是为了在 60 FPS 下获得正确的物理效果
- 粒子的摩擦力公式 `(1.0f - 4.0f * dt)` 确保在 60 FPS 下约 0.25 秒减速到一半
- 屏幕震动使用随机偏移，每帧重新计算
- **不限帧设计**：不调用 `WaitFrame`，游戏全速运行，用于测试 GameLib 渲染性能；`dt` 上限钳制为 0.05s 防止帧率过低时物理崩溃

### 9.4 GameLib.h API 使用清单

| API | 使用场景 |
|-----|---------|
| `Open` | 创建窗口（可缩放） |
| `ShowFps` | 标题栏显示实时 FPS |
| `ShowMouse` | 显示鼠标光标 |
| `GetDeltaTime` | 帧间隔 |
| `GetTime` | 运行总时间（标题脉冲、闪烁文字） |
| `IsClosed` | 主循环条件 |
| `IsKeyPressed` | ESC、F5、F6、F7、F9、Enter、R |
| `IsKeyDown` | WASD 移动 |
| `IsMouseDown` | 左键射击 |
| `GetMouseX / GetMouseY` | 瞄准 |
| `SetScene / GetScene / IsSceneChanged` | 场景管理 |
| `PlayWAV` | 播放音效 |
| `Clear` | 清屏 |
| `DrawLine` | 网格线、敌人轮廓 |
| `DrawRect` | 地图边框 |
| `FillCircle` | 粒子、敌人、子弹、玩家光晕 |
| `DrawCircle` | 绕行者内环装饰、冲击波圆、道具信标环 |
| `FillTriangle` | 玩家飞船形状 |
| `FillRect` | 死亡白色闪光 |
| `DrawText / DrawTextScale` | 文字 |
| `DrawPrintf / DrawPrintfScale` | 格式化文字（分数、连击、时间） |
| `SaveInt / LoadInt` | 历史最高分存档 |
| `SaveFloat / LoadFloat` | 历史最长存活时间存档 |

## 10. 未来可扩展方向

- [x] 射击音效（shoot-01~04 随机播放）
- [x] 敌人死亡浮动得分文字（向上飘动 fade out）
- [x] 清屏道具（Nuke，全屏敌人爆炸）
- [x] 子弹命中火花特效（白色反弹粒子）
- [x] 历史最高分/最长时间存档系统
- [x] 多命系统（3 命，重生无敌 2 秒，清除附近敌人）
- [x] 能量道具（击杀掉落，5 发扇形散射，5 秒持续）
- [x] 敌人多样化（新增蛇行者 Weaver + 绕行者 Orbiter，5 种敌人类型）
- [x] 玩家外观增强（拖尾残影 + 推进粒子 + 双层轮廓）
- [x] 连续递增难度（无上限封顶，公式化 spawnInterval 和 maxOnScreen）
- [x] 突发事件系统（Jack 涌入 + Black Hole 引力场）
- [x] 跑马灯事件通知（Ticker 队列，4 条缓存，滑入/滑出动画）
- [ ] Boss 敌人（超大血量，特殊攻击模式）
- [x] 屏幕文字通知（Achievement Popup：连击/击杀/道具里程碑）
- [x] 死亡原因提示（GAME_OVER 显示 "KILLED BY: [敌人名]" + 图标）
- [x] 背景星空（两层漂移星点，远层 50 + 近层 30）
- [x] 高分排行榜（Top 10，本地存档，GAME_OVER→Space→排行榜→Space→标题）
- [ ] 多武器类型（散射、激光、导弹）
- [ ] 移动端适配（虚拟摇杆 + 自动射击）

## 11. 文件修改历史

| 日期 | 修改内容 |
|------|---------|
| 2026-04-18 | 初始版本：基础游戏实现 |
| 2026-04-18 | 提升子弹速度 35%（600 → 810） |
| 2026-04-18 | Title 屏幕添加详细操作说明 |
| 2026-04-18 | 使用 GameLib.h SetScene 机制管理游戏状态 |
| 2026-04-19 | 添加音效系统（射击/击杀/生成随机变体播放） |
| 2026-04-19 | 添加子弹命中火花特效（白色反弹粒子） |
| 2026-04-19 | 添加敌人死亡浮动得分文字（向上飘动 fade out） |
| 2026-04-19 | 添加清屏道具 Nuke（全屏敌人爆炸） |
| 2026-04-19 | 弹跳方块添加逆时针旋转（2 圈/秒，独立初始角度） |
| 2026-04-19 | 标题文字移至上部黄金分割线（Y≈229） |
| 2026-04-19 | 添加历史最高分/最长时间存档系统（GameLib Save/Load API） |
| 2026-04-19 | HUD 顶部中央显示历史最高分（金色） |
| 2026-04-19 | 标题画面底部灰色单行显示历史纪录 |
| 2026-04-19 | 添加背景星空（两层漂移星点：远层 50 + 近层 30） |
| 2026-04-19 | 添加能量道具 Energy（击杀掉落，5 发扇形散射，5 秒持续） |
| 2026-04-19 | 添加屏幕文字通知 Achievement Popup（连击/击杀/道具里程碑） |
| 2026-04-19 | 添加死亡原因提示（GAME_OVER 显示 "KILLED BY: [敌人名]" + 图标） |
| 2026-04-19 | 添加多命系统（3 呞，重生无敌 2 秒，清除附近敌人） |
| 2026-04-19 | 添加高分排行榜（Top 10，SCENE_LEADERBOARD，GAME_OVER→Space→排行榜→Space→标题） |
| 2026-04-19 | 代码整理：全局变量分组注释、gameUpdate拆分为6个子函数、提取resetGame公共重置、triggerNuke归入PowerUps区、Draw函数集中到Rendering区、前向声明 |
| 2026-04-19 | 道具概率调整：能量道具基础概率修正(7/10/12/17%)、敌人多时概率递减、100px内已有能量道具概率减半；Nuke生成间隔改为14~22秒 |
| 2026-04-19 | 窗口改为可缩放（Open resizable=true）、标题栏显示 FPS（ShowFps）、标题画面底部添加 "Powered by GameLib" |
| 2026-04-19 | Nuke 道具视觉增强：拾取瞬间屏幕闪白（FillRect alpha 220→0，0.5s 复原）+ 冲击波圆（双环 DrawCircle 从玩家中心扩展，1200px/s，alpha fade out） |
| 2026-04-19 | 调试快捷键 F5 无条件触发 Nuke；HUD 右下角显示实时 FPS（GetFPS） |
| 2026-04-19 | 存档文件名改为 geometry.sav；FPS 颜色调亮为白色；文档补充不限帧设计说明（不调用 WaitFrame，用于测试 GameLib 性能） |
| 2026-04-20 | 道具视觉增强：信标环（DrawCircle 从15→45px 扩展，alpha 120→0，两圈错开）+ 标签文字（NUKE/ENERGY，alpha 180）+ 剩余 3s 闪烁提示 |
| 2026-04-20 | 敌人多样化：新增蛇行者（Weaver，五边形，蛇形正弦移动）+ 绕行者（Orbiter，大圆，弧线环绕）；重命名原坦克大圆→绕行者；蜂群圆改回追逐玩家 |
| 2026-04-20 | 玩家外观增强：拖尾残影（12帧历史，alpha 100→0，0.7→1.0 缩放）+ 推进粒子（青色/白色，350~600px/s，±30°）+ 双层轮廓（1.3x 外描边，半透明青白） |
| 2026-04-20 | 能量道具颜色改为橙金色（255,180,60），远离蜂群圆橙色和蛇行者黄色；推进粒子颜色改为青/白色，远离黄色敌人 |
| 2026-04-20 | 连续递增难度：spawnInterval 公式 max(0.10, 0.35-gameTime*0.001)，maxOnScreen 公式 min(60, 15+gameTime/10)，不再 120s 封顶 |
| 2026-04-20 | 突发事件系统：Jack 涌入（递减间隔涌入蜂群圆，暂停常规生成）+ Black Hole 引力场（吸引敌人+子弹+轻微拉扯玩家，吸入10个后爆炸释放8蜂群圆） |
| 2026-04-20 | 调试快捷键 F6 触发 Jack 涌入 + F7 触发 Black Hole |
| 2026-04-20 | 跑马灯事件通知：TickerMsg 队列（MAX_TICKER=4），滑入→保持→滑出动画，strncpy 截断保护（100字符），Jack/BH 爆炸时随机短语 + 霓虹红/深红颜色 |
