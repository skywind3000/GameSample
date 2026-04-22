# SHADOW CAST — Raycasting FPS 提案

基于 GameLib.h 的伪 3D 第一人称射击游戏，通过 raycasting 技术实现 Wolfenstein 3D 风格的 3D 走廊体验。

## 选题理由

### 为什么是 Raycasting FPS

| 维度 | 说明 |
|------|------|
| 展示差异化 | Geometry Wars 展示了 GameLib 的**高层图元 API**；SHADOW CAST 展示**底层 framebuffer 直接操作**，证明 GameLib 既能高层快速开发，也能底层完全掌控 |
| 话题性 | "单头文件 C++ 库做出 3D FPS" 本身就是传播力。发帖标题：*SHADOW CAST — a 3D FPS built with a single-header C++ library* |
| 特性覆盖 | 补全 Geometry Wars 未展示的特性：`GetFramebuffer()` 逐像素操作、`LoadSprite` + `GetSpritePixel` 纹理采样、精灵旋转缩放 |
| 品类差异 | Geometry Wars 是俯视射击，SHADOW CAST 是第一人称 3D，两者完全不同品类，互补性强 |

### 命名

**SHADOW CAST**

- 字面意思："投下暗影"，契合暗黑 FPS 氛围
- 技术双关："Cast" 即 raycast 的 cast，技术圈会心一笑
- 两个单词，朗朗上口，国际传播无障碍

## 核心技术方案

### Raycasting 渲染管线

整个 3D 渲染不使用 GPU，完全通过 `GetFramebuffer()` 返回的 `uint32_t*` 逐像素写入。

```
每帧渲染流程：
1. 天花板/地板 → 逐像素写入 framebuffer
2. 墙壁（逐列光线投射） → 逐像素写入 framebuffer，记录 wallDist[x]
3. 精灵（敌人/道具） → 按距离远→近排序，逐列与 wallDist[x] 比较，遮挡剔除
4. 武器（第一人称） → 最后绘制，覆盖在最上层
5. HUD → 用 GameLib DrawText/DrawRect 等高层 API
```

### 遮挡方案（无 Z-buffer）

使用经典的 **1D 深度缓冲**：

```cpp
float wallDist[SCREEN_WIDTH]; // 每列的墙壁距离

// 第一步：画墙，记录每列深度
for (int x = 0; x < SCREEN_WIDTH; x++) {
    wallDist[x] = castRay(x);
    drawWallColumn(fb, x, wallDist[x]);
}

// 第二步：画精灵，逐列深度比较
sortSpritesByDistance(sprites); // 远→近
for (each sprite) {
    for (int col = startX; col < endX; col++) {
        if (spriteDist < wallDist[col]) {
            drawSpriteColumn(fb, col, ...); // 精灵比墙近，画出来
        }
        // 否则跳过 → 自然被墙遮住
    }
}
```

只需一个 `float wallDist[800]` 的一维数组，不需要完整的 2D Z-buffer。

### GameLib 特性使用分布

| 渲染内容 | 使用的 GameLib 特性 | 说明 |
|---------|-------------------|------|
| 墙壁/地板/天花板 | `GetFramebuffer()` | 逐像素写入，raycasting 核心 |
| 墙壁纹理采样 | `LoadSprite()` + `GetSpritePixel()` | 加载纹理图片，按 UV 坐标采样 |
| 敌人/道具精灵 | `LoadSprite()` + `GetSpritePixel()` | 加载精灵图，逐列渲染带遮挡 |
| 第一人称武器 | `DrawSpriteScaled()` 或 framebuffer | 底部居中绘制武器精灵 |
| HUD 文字 | `DrawText()` / `DrawPrintf()` | 血量、弹药、分数等 |
| HUD 图形 | `DrawRect()` / `FillRect()` | 血条、准星等 |
| 地图数据 | 二维数组（代码内） | 关卡布局，可考虑用 Tilemap 扩展 |
| 场景管理 | `SetScene()` / `GetScene()` | 标题、游戏、死亡、通关 |
| 输入 | `IsKeyDown()` / `GetMouseX()` | WASD 移动 + 鼠标转向 |
| 音效 | `PlayWAV()` | 枪声、怪物、环境音 |
| 存档 | `SaveInt()` / `LoadInt()` | 最高分、关卡进度 |

## 视觉风格

### 混合方案（推荐）

- **墙壁/地板**：加载外部 64x64 像素纹理（经典像素风）
- **敌人**：加载外部精灵图（含动画帧）
- **武器/HUD/粒子效果**：用 GameLib 图元 API 绘制
- 资源放在 `ShadowCast/assets/` 目录

这样既有视觉质量，又同时展示了 GameLib 的图片加载能力和图元绘制能力。

## 资源方案

所有资源均为 **CC0（公共领域）** 许可，无需署名，可自由使用。

### 墙壁纹理

| 资源 | 作者 | 内容 | 规格 | 许可证 | 链接 |
|------|------|------|------|--------|------|
| Old school fps wall textures | knekko | 54 张墙壁纹理，Wolf3D 风格 | 64x64 | CC0 | https://opengameart.org/content/old-school-fps-wall-textures |
| Nekkrobox world textures | — | 世界纹理（墙/地板） | 各种 | CC0 | https://opengameart.org/content/nekkrobox-world-textures |
| Gritty Lowres Scifi Wall and Floor Textures | — | 科幻风墙壁+地板 | 低分辨率 | CC0 | https://opengameart.org/content/gritty-lowres-scifi-wall-and-floor-textures |
| Dark Military Fantasy Textures | — | 暗黑军事奇幻风 | 各种 | CC0 | https://opengameart.org/content/dark-military-fantasy-textures |

**推荐**：knekko 的 Old school fps wall textures，54 张 64x64 纹理，风格统一，专为 Wolf3D 类游戏制作。

### 敌人精灵

| 资源 | 作者 | 内容 | 许可证 | 链接 |
|------|------|------|--------|------|
| FPS Monster Enemies | Ragnar Random | 3 种怪物（demonario, eviloogie, smorficus），含动画帧 | CC0 | https://opengameart.org/content/fps-monster-enemies |
| Human guard for Sprite Based FPS | — | 人类守卫，多方向+动画 | CC0 | https://opengameart.org/content/human-guard-for-sprite-based-fps |
| Monster Mutant Grunt | NMN | 变异兵精灵 | CC0 | https://opengameart.org/content/monster-mutant-grunt-for-fps-game |
| Husk Mutant / Psycho-man / Toad Monster | — | 各一种怪物 | CC0 | https://opengameart.org/content/husk-mutant |

### 武器精灵（第一人称）

| 资源 | 作者 | 内容 | 许可证 | 链接 |
|------|------|------|--------|------|
| FPS Weapon Sprites | Ragnar Random | FPS 武器精灵 | CC0 | https://opengameart.org/content/fps-weapon-sprites |
| Public Domain First Person Pistols | — | 第一人称手枪 | CC0 | https://opengameart.org/content/first-person-pistol |
| FPS Weapons Overlay | — | 武器覆盖层 | CC0 | https://opengameart.org/content/fps-weapons-overlay |

### 装饰/道具

| 资源 | 作者 | 内容 | 许可证 | 链接 |
|------|------|------|--------|------|
| Oldschool FPS decoration sprites | knekko | 火把、桶、柱子等装饰物 | CC0 | https://opengameart.org/content/oldschool-fps-decoration-sprites |
| Items - armor, health, ammo | — | 护甲、血包、弹药 | CC0 | https://opengameart.org/content/items-armor-health-ammo |
| Decorations - torches, trees, corpses | — | 火把、树、尸体 | CC0 | https://opengameart.org/content/decorations-torches-trees-some-corpses |

### 音效

| 资源 | 内容 | 许可证 | 链接 |
|------|------|--------|------|
| Gunshots | 多种枪声 | CC0 | https://opengameart.org/content/gunshots |
| Monster or beast sounds | 怪物音效 | CC0 | https://opengameart.org/content/monster-or-beast-sounds |
| Horror Sound Effects Library | 恐怖氛围音效库 | CC0 | https://opengameart.org/content/horror-sound-effects-library |
| Retro Shooter Sound Effects | 复古射击音效 | CC0 | https://opengameart.org/content/retro-shooter-sound-effects |

### 一站式资源合集

OpenGameArt 上的 **[2.5d FPS (cc0)](https://opengameart.org/content/25d-fps-cc0)** 合集页面汇总了以上大部分 CC0 资源，可作为资源索引起点。

另有两个完整资源包可参考：

| 资源 | 内容 | 许可证 | 备注 |
|------|------|--------|------|
| [Anarch oldschool FPS resources](https://opengameart.org/content/anarch-oldschool-fps-resources) (drummyfish) | 完整一套：纹理+精灵+音效 | CC0 | 32x32 分辨率偏小 |
| [WolfClone Sprite Pack](https://opengameart.org/content/wolfclone-sprite-pack) | Wolf3D 风格精灵包 | CC0 | |

## 备选视觉方案

### 方案 B：纯程序生成（零素材依赖）

和 Geometry Wars 一样不加载任何外部图片，所有纹理运行时用 `CreateSprite` + `SetSpritePixel` 生成。

- 优势：单个 .cpp 文件零依赖，"one file = a full 3D FPS" 话题性极强
- 劣势：视觉精细度有限，程序化纹理开发成本高

### 方案 C：霓虹风格（统一品牌）

走 Tron/霓虹线框风格，与 Geometry Wars 形成统一的 "GameLib 霓虹宇宙"：

- 墙壁用发光网格线框渲染
- 敌人是霓虹几何体
- 如果走此路线，游戏名建议改为 **NEON CRYPT**

## 下一步

1. 确认视觉风格和最终命名
2. 下载并筛选资源素材
3. 编写完整的 `design.md`（玩法设计）
4. 编写 `spec.md`（技术规范）
5. 实现最小原型（显示窗口 + 第一人称视角 + 墙壁渲染）
6. 迭代完善
