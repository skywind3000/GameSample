# WASM 构建指南

将 GameLib.SDL.h 游戏编译为 WebAssembly，在浏览器中运行。

## 1. 环境准备

### 1.1 安装 Emscripten

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh        # Linux/macOS
emsdk_env.bat                 # Windows
```

每次新终端会话都需要激活环境（或写入 shell profile 自动加载）。

### 1.2 验证安装

```bash
emcc --version
```

确认输出包含 `emscripten` 版本号即可。

## 2. 编译命令

### 2.1 完整命令模板

```bash
emcc -std=c++11 -O2 \
     -s USE_SDL=2 -s USE_SDL_IMAGE=2 --use-port=sdl2_image:formats=png \
     -s USE_SDL_TTF=2 -s USE_SDL_MIXER=2 \
     --use-port=sdl2_mixer:formats=mp3 --use-port=sdl2_mixer:formats=ogg \
     -s ASYNCIFY=1 \
     -s ALLOW_MEMORY_GROWTH=1 \
     -DUSE_SDL \
     <GameName>/<game>.cpp \
     -o html/<game>.js \
     --preload-file <GameName>/assets@assets
```

### 2.2 几何大战实际命令

```bash
emcc -std=c++11 -O2 \
     -s USE_SDL=2 -s USE_SDL_IMAGE=2 --use-port=sdl2_image:formats=png \
     -s USE_SDL_TTF=2 -s USE_SDL_MIXER=2 \
     --use-port=sdl2_mixer:formats=mp3 --use-port=sdl2_mixer:formats=ogg \
     -s ASYNCIFY=1 \
     -s ALLOW_MEMORY_GROWTH=1 \
     -DUSE_SDL \
     GeometryWars/geometry.cpp \
     -o html/geometry.js \
     --preload-file GeometryWars/assets@assets
```

### 2.3 编译产物

输出 `-o html/geometry.js` 后会生成三个文件：

| 文件 | 说明 |
|------|------|
| `html/geometry.js` | JavaScript 载入模块 + 运行时 |
| `html/geometry.wasm` | WebAssembly 二进制 |
| `html/geometry.data` | 预加载资源包（assets 目录打包） |

自定义的 `html/geometry.html` 是手动编写的宿主页面（见第 5 节），不会被编译覆盖。

## 3. 参数详解

| 参数 | 说明 |
|------|------|
| `-std=c++11` | C++11 标准，与项目约束一致 |
| `-O2` | 优化级别，兼顾体积与速度 |
| `-s USE_SDL=2` | 启用 SDL2 核心库 |
| `-s USE_SDL_IMAGE=2` | 启用 SDL2_image 扩展库 |
| `--use-port=sdl2_image:formats=png` | 指定 PNG 解码器。默认 USE_SDL_IMAGE 不含任何解码器，必须显式声明。可追加 jpg 等格式 |
| `-s USE_SDL_TTF=2` | 启用 SDL2_ttf 扩展库 |
| `-s USE_SDL_MIXER=2` | 启用 SDL2_mixer 扩展库 |
| `--use-port=sdl2_mixer:formats=mp3` | 指定 MP3 解码器，必须分别写 |
| `--use-port=sdl2_mixer:formats=ogg` | 指定 OGG 解码器，必须分别写 |
| `-s ASYNCIFY=1` | **必须启用**。GameLib.SDL.h 用 `emscripten_sleep()` 替代 `SDL_Delay()`，没有 Asyncify 浏览器主线程会阻塞卡死 |
| `-s ALLOW_MEMORY_GROWTH=1` | 允许 WASM 内存动态增长，避免内存不足崩溃 |
| `-DUSE_SDL` | 宏开关，让 `#include "../include/GameLib.h"` 在 SDL 编译下走 `GameLib.SDL.h` 路径 |
| `--preload-file <path>@<vfs_path>` | 将资源打包到 MEMFS 虚拟文件系统。`@assets` 表示在 VFS 中挂载为 `assets/` 目录 |

### 3.1 注意事项

- **`--use-port=sdl2_mixer:formats` 不能逗号合写多格式**：`--use-port=sdl2_mixer:formats=mp3,ogg` 会失败（逗号被替换为空格导致解析错误），必须分开写两个参数。
- **WAV 不需要额外参数**：SDL_mixer 内置支持 WAV 格式。
- **SDL2_mixer port 不支持 FLAC/MIDI**。
- **输出必须是 `-o game.js` 而非 `-o game.html`**：Emscripten 每次编译会重新生成 `.html` 覆盖手动修改。输出 `.js` 只生成运行文件，自定义 HTML 页面不受影响。

## 4. 代码适配

WASM 编译需要对游戏源码做少量条件编译适配。

### 4.1 帧率控制

Win32 桌面版不限帧（全速运行测试性能），WASM 版必须交出控制权给浏览器：

```cpp
game.Update();
#if defined(__EMSCRIPTEN__)
    game.WaitFrame(60);
#endif
```

`WaitFrame(60)` 内部调用 `emscripten_sleep()`（需要 `-s ASYNCIFY=1`），每帧交出约 16ms 让浏览器处理事件和渲染。

### 4.2 ESC 键行为

Win32 桌面版 ESC 退出游戏（`break` 跳出循环 → 窗口关闭），WASM 版没有窗口关闭机制，`break` 后黑屏。改为回到标题画面：

```cpp
#if defined(__EMSCRIPTEN__)
    if (game.IsKeyPressed(KEY_ESCAPE)) {
        if (game.GetScene() != SCENE_TITLE) {
            game.SetScene(SCENE_TITLE);
            resetGame();
        }
    }
#else
    if (game.IsKeyPressed(KEY_ESCAPE)) break;
#endif
```

### 4.3 音频路径

WASM 下 `SDL_OpenAudioDevice` 只允许一个音频设备，GameLib.SDL.h 在 `__EMSCRIPTEN__` 或 `GAMELIB_SDL_MIXER_CHANNEL` 宏下自动走 `Mix_Chunk` + `Mix_PlayChannel` 通道路径，音效和音乐共用 mixer，不需要手动修改游戏代码。

### 4.4 资源路径

`--preload-file GeometryWars/assets@assets` 将 `assets/` 目录打包到 MEMFS。游戏代码中的路径（如 `"assets/music/music1.mp3"`）直接生效，不需要修改。

## 5. 自定义 HTML 页面

### 5.1 为什么需要自定义

Emscripten 默认生成的 HTML 页面（`-o game.html`）是简陋的调试界面，包含多余的控制项和输出框。每次编译都会覆盖，无法持久美化。

改为 `-o game.js` 输出后，手动编写宿主 HTML 页面，只加载 `game.js` 并提供干净的 canvas 环境。

### 5.2 页面模板

以下是几何大战的自定义页面 `html/geometry.html`，可作为模板参考：

```html
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Geometry Wars</title>
<meta name="description" content="Neon-style endless geometric shooter. WASD to move, mouse to aim and shoot, survive the swarm. Built with GameLib.h, runs entirely in browser via WebAssembly.">
<meta property="og:title" content="Geometry Wars">
<meta property="og:description" content="Neon-style endless geometric shooter. WASD to move, mouse to aim and shoot, survive the swarm. Built with GameLib.h, runs entirely in browser via WebAssembly.">
<meta property="og:image" content="screenshot.png">
<meta property="og:type" content="website">
<meta name="twitter:card" content="summary_large_image">
<meta name="twitter:title" content="Geometry Wars">
<meta name="twitter:description" content="Neon-style endless geometric shooter. WASD to move, mouse to aim and shoot, survive the swarm. Built with GameLib.h, runs entirely in browser via WebAssembly.">
<meta name="twitter:image" content="screenshot.png">
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
html, body { width: 100%; height: 100%; overflow: hidden; background: #0a0a1a; }
body {
  display: flex;
  align-items: center;
  justify-content: center;
}
#loading {
  position: fixed; top: 0; left: 0; width: 100%; height: 100%;
  display: flex; align-items: center; justify-content: center;
  background: #0a0a1a; z-index: 100; transition: opacity 0.6s;
}
#loading.hidden { opacity: 0; pointer-events: none; }
#loading-inner { text-align: center; color: #00ffff; font-family: 'Segoe UI', sans-serif; }
#loading-inner h1 { font-size: 2em; margin-bottom: 0.5em; letter-spacing: 0.3em; }
#loading-inner p { font-size: 0.9em; color: #4488aa; }
#loading-bar { width: 200px; height: 3px; background: #112233; margin: 1em auto 0; border-radius: 2px; }
#loading-fill { width: 0%; height: 100%; background: #00ffff; border-radius: 2px; transition: width 0.3s; }
#canvas {
  display: block;
  width: 800px;
  height: 600px;
  border: 1px solid #1a2a4a;
  image-rendering: pixelated;
  image-rendering: crisp-edges;
}
</style>
</head>
<body>

<div id="loading">
  <div id="loading-inner">
    <h1>GEOMETRY WARS</h1>
    <p>Loading...</p>
    <div id="loading-bar"><div id="loading-fill"></div></div>
  </div>
</div>

<canvas id="canvas" oncontextmenu="event.preventDefault()" tabindex="-1"></canvas>

<script>
var Module = {
  canvas: document.getElementById('canvas'),
  setStatus: function(text) {
    var p = document.querySelector('#loading-inner p');
    if (p) p.textContent = text || 'Loading...';
  },
  monitorRunDependencies: function(left) {
    var fill = document.getElementById('loading-fill');
    if (fill) {
      fill.style.width = (left > 0 ? ((1 - left / 2) * 100) : 100) + '%';
    }
  }
};

(function poll() {
  var c = document.getElementById('canvas');
  if (c && c.width >= 100) {
    document.getElementById('loading').classList.add('hidden');
    return;
  }
  requestAnimationFrame(poll);
})();
</script>
<script async src="geometry.js"></script>

</body>
</html>
```

### 5.3 关键要点

- **canvas 必须设置 CSS `width` 和 `height`**（如 `800px` / `600px`）：flex 布局会导致无固定尺寸的 canvas 坍缩为极小像素，游戏渲染到 2x2 画布上几乎不可见。
- **`Module.canvas` 必须指向页面中的 canvas 元素**：Emscripten 通过 `Module.canvas` 找到渲染目标。
- **Loading 遮罩用轮询检测隐藏**：`onRuntimeInitialized` / `postRun` 在 WASM 输出模式下不一定可靠。轮询 canvas 宽度（`c.width >= 100`）是更稳定的检测方式——Emscripten SDL 初始化时会将 canvas 设置为游戏窗口尺寸。
- **`<script async>` 加载 JS 模块**：与 Emscripten 推荐的加载方式一致。
- **`oncontextmenu="event.preventDefault()"`**：防止右键弹出浏览器菜单干扰游戏。

## 6. 运行与测试

### 6.1 启动本地服务器

WASM 必须通过 HTTP 服务器访问，`file://` 协议无法加载 `.data` 和 `.wasm` 文件。

```bash
cd html
python -m http.server 8000
```

浏览器打开 `http://localhost:8000/geometry.html`。

### 6.2 文件部署结构

`html/` 目录下需要以下文件：

```
html/
  geometry.html      # 自定义宿主页面
  geometry.js        # Emscripten JS 运行时
  geometry.wasm      # WASM 二进制
  geometry.data      # 资源包
```

所有文件必须在同一目录，`geometry.html` 通过 `<script async src="geometry.js">` 加载模块，JS 运行时会自动寻找同目录的 `.wasm` 和 `.data`。

### 6.3 编译后验证清单

1. 确认 `html/` 下生成了 `.js`、`.wasm`、`.data` 三个文件
2. 启动 HTTP 服务器
3. 浏览器打开页面，Loading 遮罩出现后自动消失
4. 游戏画面正常显示（canvas 800x600）
5. 音效和音乐同时播放正常
6. FPS 显示正常（标题栏或 HUD）
7. ESC 键回到标题画面（不黑屏）

## 7. 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| 页面空白/黑屏，游戏标题无 FPS | `break` 跳出循环导致 WASM 模块停止 | ESC 改为 `SetScene(SCENE_TITLE)` + `resetGame()` |
| canvas 只有 2x2 像素 | flex 布局使无固定尺寸的 canvas 坍缩 | 给 canvas 加 CSS `width: 800px; height: 600px` |
| Loading 遮罩不消失 | `onRuntimeInitialized` / `postRun` 未触发 | 用轮询检测 canvas 宽度代替 |
| 音乐和音效不能同时播放 | WASM 下 `SDL_OpenAudioDevice` 只允许一个 | GameLib.SDL.h 已自动走 Mix_Chunk 路径，确保 `-s USE_SDL_MIXER=2` 和 `--use-port=sdl2_mixer:formats=...` |
| MP3 无法加载 | SDL_mixer 默认不含 MP3 解码器 | 加 `--use-port=sdl2_mixer:formats=mp3` |
| PNG 图片无法加载 | SDL_image 默认不含 PNG 解码器 | 加 `--use-port=sdl2_image:formats=png` |
| 浏览器主线程卡死 | 未启用 Asyncify | 加 `-s ASYNCIFY=1` |
| `.data` 文件加载 404 | `file://` 协议不支持 fetch | 用 HTTP 服务器访问 |
| 多音频格式参数写法错误 | 逗号合写被替换为空格 | 分开写：`--use-port=sdl2_mixer:formats=mp3` 和 `--use-port=sdl2_mixer:formats=ogg` |