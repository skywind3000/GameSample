# AGENTS.md

本项目是 GameLib.h 的游戏演示仓库，展示如何使用 GameLib.h 制作完整游戏。项目包含多个使用 GameLib.h 开发的游戏示例，以及 GameLib.h 本身作为参考。

## Structure

```
- include/               # 头文件目录
  - GameLib.h            # GameLib 游戏库（Win32 版，供参考）
  - GameLib.SDL.h        # GameLib 游戏库（SDL 版，供参考）
  - GameSound.h          # GameSound 多声道音频播放库（备份，功能已合并到 GameLib.h）
  - GameTool.h           # GameTool 游戏工具库（增强 GameLib.h，本项目维护）
- docs/                  # GameLib.h API 文档
  - manual.md            # GameLib.h 公开 API 接口说明
  - quickref.md          # GameLib.h 快速 API 参考
  - GameSound.md         # GameSound.h 技术规格文档
- <GameName>/            # 各游戏项目目录（如 GeometryWars/）
  - assets/              # 游戏专属音效资源
  - design.md            # 游戏设计文档（玩法、敌人、计分、操控）
  - spec.md              # 技术文档（技术实现、数据结构、性能）
  - assets.md            # 资源列表（音效、图片等素材索引）
  - <game>.cpp           # 游戏源码
- tools/                 # 工具演示程序
```

## Documentation

| 文档 | 用途 | 何时需要读 |
|------|------|-----------|
| `docs/manual.md` | GameLib.h 公开 API 接口说明 | 开发新游戏或查阅 API 时 |
| `docs/quickref.md` | GameLib.h 快速 API 列表 | 开发新游戏时快速查阅 |

以上文档是 GameLib.h API 的权威来源，开发游戏时应优先参考。

## Games

本仓库包含多个使用 GameLib.h 制作的完整游戏，按目录组织：

| 游戏 | 目录 | 类型 | 特色 |
|------|------|------|------|
| 几何大战 | `GeometryWars/` | 无尽射击 | 霓虹几何体、弹簧网格、粒子爆炸 |

每个游戏目录包含：
- `design.md` — 游戏设计文档（玩法、敌人、计分、操控等）
- `spec.md` — 技术文档（技术实现、数据结构、性能优化等）
- `assets.md` — 资源列表（音效、图片等素材索引）
- `assets/` — 游戏专属音效资源（WAV 格式）
- `<game>.cpp` — 游戏完整源码

## Game Design Principles

开发或修改游戏时，请遵循以下设计原则：

1. **优先使用 GameLib.h 已有功能**
   - 充分利用 GameLib.h 提供的内置机制，如 `SetScene` 管理游戏状态（标题、战斗、结算等）。
   - 避免自行重新实现已有功能（如状态机、碰撞检测、输入处理等）。
   - 开发前先查阅 `docs/manual.md`，确认哪些功能已由 GameLib.h 提供。

2. **文档驱动开发**
   - **玩法设计**：参考同目录下的 `design.md`，明确核心玩法、敌人行为、计分规则、操控方式等。
   - **技术规范**：参考同目录下的 `spec.md`，了解技术实现细节、数据结构、性能优化等。
   - **资源列表**：参考同目录下的 `assets.md`，了解可用的音效、图片等资源及其用途。
   - 开发前先阅读对应文档，不要在没有文档的情况下直接改代码。

3. **保持简洁**
   - 游戏代码应保持简单易懂，避免过度抽象或设计模式滥用。
   - 优先使用 GameLib.h 的简单 API（如整数 ID 管理精灵），不要引入复杂的对象生命周期。

## Build

### 编译命令

```bash
g++ -o <game>.exe <game>.cpp -mwindows
```

- 不需要 `-lwinmm -lgdi32` 等参数，GameLib.h 通过动态加载解决所有依赖。
- 游戏源码通过 `#include "../include/GameLib.h"` 引入 GameLib。

### 示例：编译几何大战

```bash
cd GeometryWars
g++ -o geometry.exe geometry.cpp -mwindows
```

## Code Constraints

### 游戏开发规范

- 只能使用 C++11 语法，不能用 C++14/17/20 特性。
- 必须兼容 GCC 4.9.2（Dev-C++ 5 自带）。
- 游戏文件放在独立目录下，通过 `#include "../include/GameLib.h"` 引入。
- 使用 GameTool.h 时通过 `#include "../include/GameTool.h"` 引入（必须在 GameLib.h 之后）。
- 保持 `GameLib game; game.Open(...); while (...) { ... }` 的上手模型。

### GameLib.h（仅参考，不在此仓库修改）

- GameLib.h 是外部维护的库文件，本仓库仅将其作为参考引入。
- **不在本仓库修改 GameLib.h**，如有 API 需求变更应联系 GameLib 主仓库。

## Guidelines

### 通用 agent 工作方式

- 先看 `docs/manual.md` 或 `docs/quickref.md` 了解 API 再下手。
- 搜索或 review 时忽略目录下的 `.exe` / `.o` 编译产物。
- 每个游戏的设计细节以对应目录下的文档为准：
  - `design.md` — 玩法设计（游戏规则、敌人、计分、操控）
  - `spec.md` — 技术规范（技术实现、数据结构、性能优化）
  - `assets.md` — 资源列表（音效、图片等素材索引）
- 遵循 Game Design Principles 中的设计原则。

### 任务分类与行动路线

收到用户请求后，先判断属于哪类任务，再执行对应流程：

| 任务类型 | 判断标准 | 行动路线 |
|----------|---------|---------|
| 用 GameLib.h 写新游戏 | 用户描述想做一个游戏/功能 | → 见"用 GameLib.h 做游戏"流程 |
| 修改现有游戏 | 用户想改某个游戏的玩法/功能 | → 见"修改现有游戏"流程 |
| 迭代 GameLib.h | 改动 GameLib.h 文件本身 | **不在本仓库进行**，联系主仓库 |
| 迭代 GameTool.h | 改动 GameTool.h 工具库 | 可以在本仓库进行 |

不确定时，先问用户再动手，不要猜。

### 用 GameLib.h 做游戏

1. **先理解需求**：问用户想要什么类型的游戏或功能，确认核心玩法。
2. **读 API 文档**：阅读 `docs/manual.md` 或 `docs/quickref.md` 了解公开 API。
3. **参考现有游戏**：查看 `Games` 表格中与目标功能最接近的游戏，参考其结构和实现方式。
4. **优先用现有素材**：查看各游戏目录下的 `assets/`，优先复用已有音效。
5. **先跑最小原型**：不要一开始就写完整游戏，先让基本功能跑起来（显示窗口、能操作），再逐步加功能。
6. **文件位置**：新建游戏时创建独立目录（如 `NewGame/`），通过 `#include "../include/GameLib.h"` 引入。
7. **写设计文档**：游戏确定后，在目录下创建 `design.md` 记录完整设计。

### 修改现有游戏

1. **读设计文档**：先阅读对应游戏的 `design.md` 了解现有设计。
2. **读技术文档**：如涉及技术实现细节，查阅 `spec.md`。
3. **读资源文档**：如需使用或添加音效/图片，查阅 `assets.md`。
4. **读 API 文档**：如涉及新 API 用法，查阅 `docs/manual.md`。
5. **修改代码**：遵守 Code Constraints 中的游戏开发规范。
6. **更新文档**：如改动涉及设计变化，同步更新 `design.md`、`spec.md` 或 `assets.md`。

### 每次改动最小清单

1. **读**：读相关文档（`docs/manual.md`、游戏 `design.md`/`spec.md`/`assets.md`）
2. **写**：写/改代码（遵守 Code Constraints）
3. **验**：编译游戏验证
4. **更**：更新对应文档（如果玩法、技术或资源变了）

### 禁止事项

- **禁止使用 C++14+ 特性**：`auto` 返回类型推导、`std::make_unique`、结构化绑定、`if constexpr` 等。
- **禁止修改 GameLib.h**：本仓库仅使用 GameLib.h，不维护或修改它（如需迭代请联系主仓库）。GameTool.h 是本项目维护的，可以修改。
- **禁止引入外部依赖**：所有功能通过 GameLib.h 提供的 API 实现。
- **不要用 `git add -A` 或 `git add .`**：可能误提交编译产物或敏感文件，按文件名逐个 `git add`。
- **不要提交编译产物**：忽略所有 `.exe`、`.o` 等文件。
- **不要在没有读过对应文档的情况下直接改代码**：先看 `design.md`、`spec.md`、`assets.md` 和 API 文档。
