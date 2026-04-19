# GameSample

使用 [GameLib](https://github.com/skywind3000/GameLib) 制作的完整游戏示例集合。

## GameLib

GameLib 是一个单头文件 C++ 游戏库，专为初学者设计——无需 SDL/DirectX/OpenGL，无需配置，只需 `#include` 即可开始。

- **项目主页**：[https://github.com/skywind3000/GameLib](https://github.com/skywind3000/GameLib)
- **API 文档**：[docs/manual.md](docs/manual.md) | [docs/quickref.md](docs/quickref.md)

## 游戏列表

| 游戏 | 目录 | 类型 | 特色 |
|------|------|------|------|
| 几何大战 | `GeometryWars/` | 无尽射击 | 霓虹几何体、弹簧网格、粒子爆炸 |

每个游戏目录包含：
- `design.md` — 玩法设计文档
- `spec.md` — 技术规范文档
- `assets.md` — 资源列表
- `assets/` — 音效资源
- `<game>.cpp` — 游戏源码

## 编译

```bash
g++ -o <game>.exe <game>.cpp -mwindows
```

示例：

```bash
cd GeometryWars
g++ -o geometry.exe geometry.cpp -mwindows
```

无需额外链接参数，GameLib 通过动态加载解决所有依赖。

## 代码规范

- C++11 语法，兼容 GCC 4.9.2
- 通过 `#include "../GameLib.h"` 引入 GameLib
- 保持 `GameLib game; game.Open(...); while (...) { ... }` 的上手模型