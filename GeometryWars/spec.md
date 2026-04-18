# Geometry Wars - 技术实现规范

基于 GameLib.h 的霓虹风格几何射击游戏，无尽模式。本文档记录完整的技术实现细节，便于在另一台机器上继续开发。

## 1. 项目结构

```
GeometryWars/
├── geometry.cpp      # 游戏源码（单文件，~800 行）
├── design.md         # 游戏设计文档（玩法、敌人、计分、操控）
├── spec.md           # 技术文档（本文档）
└── assets/sound/     # 音效资源
    ├── click.wav         # 射击音效
    ├── hit.wav           # 击中音效
    ├── coin.wav          # 击杀音效
    ├── explosion.wav     # 玩家死亡音效
    ├── game_over.wav     # 游戏结束音效
    ├── note_do_high.wav  # 开始游戏提示音
    └── victory.wav       # 胜利音效
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

### 游戏参数

| 常量 | 值 | 说明 |
|------|-----|------|
| `PLAYER_SPEED` | 250.0f | 玩家移动速度（px/s） |
| `BULLET_SPEED` | 810.0f | 子弹速度（px/s，已提升 35%） |
| `SHOOT_RATE` | 0.12f | 射击间隔（秒），约每秒 8.3 发 |
| `COMBO_TIMEOUT` | 2.0f | 连击超时（秒） |

### 场景 ID

| 常量 | 值 | 说明 |
|------|-----|------|
| `SCENE_TITLE` | 1 | 标题画面 |
| `SCENE_COMBAT` | 2 | 战斗（正常游戏） |
| `SCENE_DEATH` | 3 | 玩家死亡动画 |
| `SCENE_GAME_OVER` | 4 | 结算画面 |

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
    int type;       // 类型：0=蜂群圆, 1=追踪三角, 2=弹跳方块, 3=坦克大圆
    int hp;         // 当前血量
    int maxHp;      // 最大血量
    float r;        // 碰撞半径
    float speed;    // 基础速度
    bool active;    // 是否活跃
    float angle;    // 旋转角度（三角形朝向用）
};
```

**敌人属性表**：

| 类型 | 名称 | 半径 | 速度 | HP | 分值 | 颜色 |
|------|------|------|------|-----|------|------|
| 0 | 蜂群圆 (Swarm) | 10 | 60 | 1 | 50 | 橙色霓虹 (255,150,40) |
| 1 | 追踪三角 (Chaser) | 12 | 140 | 1 | 100 | 粉红霓虹 (255,80,140) |
| 2 | 弹跳方块 (Bouncer) | 14 | 100 | 2 | 150 | 绿色霓虹 (80,255,80) |
| 3 | 坦克大圆 (Tank) | 22 | 45 | 5 | 300 | 紫色霓虹 (160,60,220) |

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

## 5. 场景状态机

使用 GameLib.h 的 `SetScene()` / `GetScene()` / `IsSceneChanged()` 机制管理游戏大状态。

### 5.1 场景转换图

```
SCENE_TITLE (1)
    │ Enter 键
    ▼
SCENE_COMBAT (2) ───── 玩家被碰撞 ────→ SCENE_DEATH (3)
    │                                         │ 1.5秒后
    │                                         ▼
    │                                   SCENE_GAME_OVER (4)
    │                                         │ R 键
    └─────────────────────────────────────────┘
```

### 5.2 各场景职责

#### SCENE_TITLE - 标题画面
- **更新**：弹簧网格（自动波动）
- **绘制**：网格、标题文字、"PRESS ENTER TO START"（闪烁）、操作说明
- **输入**：Enter → 重置游戏数据，切换到 SCENE_COMBAT
- **摄像机**：不更新

#### SCENE_COMBAT - 战斗
- **更新**：调用 `gameUpdate()` 处理所有游戏逻辑
- **绘制**：网格 → 地图边框 → 子弹 → 敌人 → 玩家 → 粒子 → HUD
- **输入**：WASD 移动、鼠标瞄准、左键射击
- **摄像机**：跟随玩家

#### SCENE_DEATH - 玩家死亡动画
- **更新**：粒子、网格、屏幕震动（不调用 `gameUpdate()`）
- **绘制**：黑色背景 → 网格 → 地图边框 → 粒子 → 白色闪光（前 0.5 秒）
- **自动转换**：1.5 秒后 → SCENE_GAME_OVER
- **音效**：播放 explosion.wav（第 1 秒内）
- **摄像机**：跟随死亡位置

#### SCENE_GAME_OVER - 结算画面
- **更新**：网格、粒子
- **绘制**：黑色背景 → 网格 → 粒子 → GAME OVER 文字 → 统计数据 → "PRESS R TO RESTART"（闪烁）
- **输入**：R 键 → 重置游戏数据，切换到 SCENE_TITLE
- **显示数据**：最终分数、存活时间、总击杀数、最高连击倍率
- **摄像机**：不更新（保持在死亡位置）

### 5.3 场景切换时的初始化

- **TITLE → COMBAT**：重置所有游戏数据（分数、连击、位置、敌人、子弹、粒子、网格）
- **COMBAT → DEATH**：玩家死亡时触发（`playerAlive = false`）
- **DEATH → GAME_OVER**：自动（1.5 秒后），播放 game_over.wav
- **GAME_OVER → TITLE**：按 R 键，重置所有游戏数据 + 清除震动状态

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
- **初始化**：随机方向 `vx = cos(a) * speed, vy = sin(a) * speed`
- **边界**：碰到地图边界反弹（`vx = -vx` 或 `vy = -vy`）

#### 坦克大圆 (Type 3)
- **行为**：缓慢移向玩家
- **物理**：`vx += (dx / d) * speed * 1.5f * dt`，阻尼 `1.0f - 2.0f * dt`（最小 0.8）
- **特殊**：死亡时分裂为 3 个蜂群圆（间隔 120 度，距离 20px）
- **边界**：钳制在地图内

### 6.4 敌人生成系统

**生成位置** (`spawnFromEdge`)：
- 从地图四条边随机选择一边（`rand() % 4`）
- 在边上随机选择位置（带 `margin = 80` 偏移）
- 确保与玩家距离 > 250px（最多尝试 20 次）

**生成节奏**（随时间递增）：

| 游戏时间 | 生成间隔 | 同时存在上限 | 可用类型 |
|----------|---------|-------------|---------|
| 0~15 秒 | 0.35s | 15 | 蜂群圆 (0) |
| 15~30 秒 | 0.28s | 20 | 蜂群圆 + 追踪三角 (0~1) |
| 30~60 秒 | 0.22s | 25 | 蜂群圆 + 追踪三角 + 弹跳方块 (0~2) |
| 60~120 秒 | 0.22s | 30 | 全部类型 (0~3) |
| 120 秒+ | 0.18s | 40 | 全部类型 (0~3) |

**实现逻辑**：
```cpp
// 确定最大敌人类型
int maxType = 0;
if (gameTime > 15.0f) maxType = 1;
if (gameTime > 30.0f) maxType = 2;
if (gameTime > 60.0f) maxType = 3;

// 生成间隔
float spawnInterval = 0.35f;
if (gameTime > 120.0f) spawnInterval = 0.18f;
else if (gameTime > 60.0f) spawnInterval = 0.22f;
else if (gameTime > 30.0f) spawnInterval = 0.28f;

// 生成
spawnTimer += dt;
if (spawnTimer >= spawnInterval && activeCount < maxOnScreen) {
    spawnTimer = 0;
    int type = rand() % (maxType + 1);
    spawnFromEdge(type);
}
```

### 6.5 碰撞检测

#### 子弹 vs 敌人
- **检测**：圆形碰撞，`dist(bullet, enemy) < enemy.r + 4`
- **效果**：
  - 子弹消失
  - 敌人 HP -1
  - HP <= 0：触发击杀爆炸、网格冲击、屏幕震动、计分
  - 坦克（type 3）死亡时分裂为 3 个蜂群圆

#### 玩家 vs 敌人
- **检测**：圆形碰撞，`dist(player, enemy) < enemy.r + 8`
- **效果**（无敌模式跳过）：
  - `playerAlive = false`
  - 切换到 `SCENE_DEATH`
  - 清除所有敌人
  - 触发巨型死亡爆炸（3 组粒子 + 强力网格冲击 + 强震动）

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
| 坦克击杀 | 2px | 5 帧 |
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
- 基础分：蜂群圆 50、追踪三角 100、弹跳方块 150、坦克 300

**连击规则**：
- 每次击杀：`combo++`，`comboTimer = 2.0f`
- 2 秒无击杀：`combo = 1`（重置）
- 最高倍率：x10（代码未硬性限制，但实际很难超过）
- 记录 `highestCombo`（历史最高）

**HUD 显示**：
- 左上角：`SCORE: xxx`（白色）
- 顶部中央：`xCombo`（黄色，2 倍放大，仅 combo > 1 时显示）
- 右上角：`TIME M:SS`（天蓝色）

### 6.9 玩家绘制

**组成**（3 层）：
1. **光晕**：`FillCircle` 半径 20，`COLOR_ARGB(60, 0, 255, 255)`
2. **飞船形状**：三角形（`FillTriangle`），顶点朝鼠标方向
   - 前顶点：`(cos(a) * 18, sin(a) * 18)`
   - 后两顶点：从后方偏移 10px，左右各展开 10px（角度 ±1.3 弧度）
3. **引擎光晕**：`FillCircle` 半径 4，后方 12px 处，`COLOR_ARGB(150, 255, 200, 50)`

### 6.10 子弹绘制

**组成**（3 层）：
1. **核心**：`FillCircle` 半径 3，`COLOR_ARGB(255, 200, 255, 200)`
2. **光晕**：`FillCircle` 半径 6，`COLOR_ARGB(100, 100, 255, 100)`
3. **拖尾**：`FillCircle` 半径 2，位置偏移 `-vx * 0.008f`，`COLOR_ARGB(80, 100, 200, 100)`

### 6.11 音效系统

**音效路径解析** (`pathOf`)：
- 尝试两个路径（相对当前目录和相对上级目录）
- 返回第一个存在的文件路径

**音效映射**：

| 事件 | 音效文件 | 触发时机 |
|------|---------|---------|
| 射击 | `click.wav` | （代码中已定义但未在射击时调用，可选择添加） |
| 玩家死亡 | `explosion.wav` | SCENE_DEATH 前 1 秒内 |
| 游戏结束 | `game_over.wav` | 进入 SCENE_GAME_OVER 时 |
| 开始游戏 | `note_do_high.wav` | 进入 SCENE_COMBAT 时 |

## 7. 渲染顺序

每帧的渲染顺序（从后到前）：

```
1. Clear(COLOR_BLACK)
2. 弹簧网格（gridDraw）
3. 地图边框（drawMapBorder）
4. 子弹（drawBullets）
5. 敌人（drawEnemies）
6. 玩家（drawPlayer，仅存活时）
7. 粒子（particlesDraw）
8. HUD（drawHUD）
```

**特殊效果**：
- 白色闪光（DEATH 场景前 0.5 秒）：在网格和粒子之上绘制半透明白色矩形
- 屏幕震动：通过 `shakeX, shakeY` 偏移所有绘制坐标

## 8. 主循环结构

```cpp
while (!game.IsClosed()) {
    double dt = game.GetDeltaTime();
    if (dt > 0.05) dt = 0.05;  // 限制最大 dt 防止跳帧
    float fdt = (float)dt;

    // 输入处理
    if (game.IsKeyPressed(KEY_ESCAPE)) break;
    if (game.IsKeyPressed(KEY_F9)) invincible = !invincible;  // 调试：无敌

    // 场景状态机
    switch (game.GetScene()) {
        case SCENE_TITLE:       // 标题画面
        case SCENE_COMBAT:      // 战斗
        case SCENE_DEATH:       // 死亡动画
        case SCENE_GAME_OVER:   // 结算
    }

    // 摄像机更新（TITLE 场景除外）
    if (game.GetScene() != SCENE_TITLE) cameraUpdate();

    game.Update();  // 提交帧
}
```

## 9. 已知特性与注意事项

### 9.1 调试功能
- **F9**：切换无敌模式（开启后不会被敌人/弹幕伤害）

### 9.2 限制
- 最大子弹数 150，超过后无法射击（会等待空闲槽位）
- 最大敌人数 100，超过后无法生成新敌人
- 最大粒子数 800，超过后无法生成新粒子
- 坦克分裂可能导致瞬间敌人数激增（1 个坦克 → 3 个蜂群圆）

### 9.3 物理注意事项
- 弹簧网格的 `dt * 60.0f` 缩放因子是为了在 60 FPS 下获得正确的物理效果
- 粒子的摩擦力公式 `(1.0f - 4.0f * dt)` 确保在 60 FPS 下约 0.25 秒减速到一半
- 屏幕震动使用随机偏移，每帧重新计算

### 9.4 GameLib.h API 使用清单

| API | 使用场景 |
|-----|---------|
| `Open` | 创建窗口 |
| `ShowMouse` | 显示鼠标光标 |
| `GetDeltaTime` | 帧间隔 |
| `GetTime` | 运行总时间（标题脉冲、闪烁文字） |
| `IsClosed` | 主循环条件 |
| `IsKeyPressed` | ESC、F9、Enter、R |
| `IsKeyDown` | WASD 移动 |
| `IsMouseDown` | 左键射击 |
| `GetMouseX / GetMouseY` | 瞄准 |
| `SetScene / GetScene / IsSceneChanged` | 场景管理 |
| `PlayWAV` | 播放音效 |
| `Clear` | 清屏 |
| `DrawLine` | 网格线、敌人轮廓 |
| `DrawRect` | 地图边框 |
| `FillCircle` | 粒子、敌人、子弹、玩家光晕 |
| `DrawCircle` | 坦克内环装饰 |
| `FillTriangle` | 玩家飞船形状 |
| `FillRect` | 死亡白色闪光 |
| `DrawText / DrawTextScale` | 文字 |
| `DrawPrintf / DrawPrintfScale` | 格式化文字（分数、连击、时间） |

## 10. 未来可扩展方向

- [ ] 添加射击音效（`sounds.click` 在射击时调用）
- [ ] 添加敌人被击中文字提示
- [ ] 多命系统（3 命）
- [ ] 能量道具（击杀敌人掉落，拾取后强化武器）
- [ ] Boss 敌人（超大血量，特殊攻击模式）
- [ ] 背景星空（静态粒子，增加画面层次）
- [ ] 高分排行榜（本地存档）
- [ ] 多武器类型（散射、激光、导弹）
- [ ] 移动端适配（虚拟摇杆 + 自动射击）

## 11. 文件修改历史

| 日期 | 修改内容 |
|------|---------|
| 2026-04-18 | 初始版本：基础游戏实现 |
| 2026-04-18 | 提升子弹速度 35%（600 → 810） |
| 2026-04-18 | Title 屏幕添加详细操作说明 |
| 2026-04-18 | 使用 GameLib.h SetScene 机制管理游戏状态 |
