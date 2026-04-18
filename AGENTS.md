# AGENTS.md

本项目是 GameLib.h 的游戏演示仓库，展示如何使用 GameLib.h 制作完整游戏。项目包含多个使用 GameLib.h 开发的游戏示例，以及 GameLib.h 本身作为参考。

## Structure

```
- docs/                  # GameLib.h API 文档
  - manual.md            # GameLib.h 公开 API 接口说明
  - quickref.md          # GameLib.h 快速 API 参考
- <GameName>/            # 各游戏项目目录（如 GeometryWars/）
  - assets/              # 游戏专属音效资源
  - design.md            # 游戏设计文档
  - <game>.cpp           # 游戏源码
- GameLib.h              # GameLib 游戏库（Win32 版，供参考）
- GameLib.SDL.h          # GameLib 游戏库（SDL 版，供参考）
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
- `assets/` — 游戏专属音效资源（WAV 格式）
- `<game>.cpp` — 游戏完整源码

## Build

### 编译命令

```bash
g++ -o <game>.exe <game>.cpp -mwindows
```

- 不需要 `-lwinmm -lgdi32` 等参数，GameLib.h 通过动态加载解决所有依赖。
- 游戏源码通过 `#include "../GameLib.h"` 引入 GameLib。

### 示例：编译几何大战

```bash
cd GeometryWars
g++ -o geometry.exe geometry.cpp -mwindows
```

## Code Constraints

### 游戏开发规范

- 只能使用 C++11 语法，不能用 C++14/17/20 特性。
- 必须兼容 GCC 4.9.2（Dev-C++ 5 自带）。
- 游戏文件放在独立目录下，通过 `#include "../GameLib.h"` 引入。
- 保持 `GameLib game; game.Open(...); while (...) { ... }` 的上手模型。

### GameLib.h（仅参考，不在此仓库修改）

- GameLib.h 是外部维护的库文件，本仓库仅将其作为参考引入。
- **不在本仓库修改 GameLib.h**，如有 API 需求变更应联系 GameLib 主仓库。

## Guidelines

### 通用 agent 工作方式

- 先看 `docs/manual.md` 或 `docs/quickref.md` 了解 API 再下手。
- 搜索或 review 时忽略目录下的 `.exe` / `.o` 编译产物。
- 每个游戏的设计细节以对应 `design.md` 为准。

### 任务分类与行动路线

收到用户请求后，先判断属于哪类任务，再执行对应流程：

| 任务类型 | 判断标准 | 行动路线 |
|----------|---------|---------|
| 用 GameLib.h 写新游戏 | 用户描述想做一个游戏/功能 | → 见"用 GameLib.h 做游戏"流程 |
| 修改现有游戏 | 用户想改某个游戏的玩法/功能 | → 见"修改现有游戏"流程 |
| 迭代 GameLib.h | 改动 GameLib.h 文件本身 | **不在本仓库进行**，联系主仓库 |

不确定时，先问用户再动手，不要猜。

### 用 GameLib.h 做游戏

1. **先理解需求**：问用户想要什么类型的游戏或功能，确认核心玩法。
2. **读 API 文档**：阅读 `docs/manual.md` 或 `docs/quickref.md` 了解公开 API。
3. **参考现有游戏**：查看 `Games` 表格中与目标功能最接近的游戏，参考其结构和实现方式。
4. **优先用现有素材**：查看各游戏目录下的 `assets/`，优先复用已有音效。
5. **先跑最小原型**：不要一开始就写完整游戏，先让基本功能跑起来（显示窗口、能操作），再逐步加功能。
6. **文件位置**：新建游戏时创建独立目录（如 `NewGame/`），通过 `#include "../GameLib.h"` 引入。
7. **写设计文档**：游戏确定后，在目录下创建 `design.md` 记录完整设计。

### 修改现有游戏

1. **读设计文档**：先阅读对应游戏的 `design.md` 了解现有设计。
2. **读 API 文档**：如涉及新 API 用法，查阅 `docs/manual.md`。
3. **修改代码**：遵守 Code Constraints 中的游戏开发规范。
4. **更新文档**：如改动涉及设计变化，同步更新 `design.md`。

### 每次改动最小清单

1. **读**：读相关文档（`docs/manual.md` 或游戏 `design.md`）
2. **写**：写/改代码（遵守 Code Constraints）
3. **验**：编译游戏验证
4. **更**：更新 `design.md`（如果游戏玩法/设计变了）

### 禁止事项

- **禁止使用 C++14+ 特性**：`auto` 返回类型推导、`std::make_unique`、结构化绑定、`if constexpr` 等。
- **禁止修改 GameLib.h**：本仓库仅使用 GameLib.h，不维护或修改它。
- **禁止引入外部依赖**：所有功能通过 GameLib.h 提供的 API 实现。
- **不要用 `git add -A` 或 `git add .`**：可能误提交编译产物或敏感文件，按文件名逐个 `git add`。
- **不要提交编译产物**：忽略所有 `.exe`、`.o` 等文件。
- **不要在没有读过对应文档的情况下直接改代码**：先看 `design.md` 和 API 文档。
