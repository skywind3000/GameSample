# GameLib API Reference

本文档提供 GameLib.h 所有公开 API 的详细参考，按功能模块组织。

每个接口按统一格式呈现：**功能介绍**、**函数声明**、**参数**、**返回值**、**备注**。

---

## 1. 窗口与主循环

### Open

创建窗口并初始化帧缓冲、输入、时间系统。`width/height` 决定固定 framebuffer 逻辑尺寸，打开后不会因窗口缩放而改变。支持 restart-safe 重开。

**函数声明**
```cpp
int Open(int width, int height, const char *title, bool center = false, bool resizable = false);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `width` | `int` | Framebuffer 逻辑宽度，范围 1~16384 |
| `height` | `int` | Framebuffer 逻辑高度，范围 1~16384 |
| `title` | `const char *` | 窗口标题，支持 UTF-8 |
| `center` | `bool` | 是否居中显示，默认 `false` |
| `resizable` | `bool` | 是否允许用户拖拽缩放和最大化，默认 `false` |

**返回值**

| 值 | 说明 |
|-----|------|
| `0` | 成功 |
| `-1` | 窗口类注册失败 |
| `-2` | 创建 DC 失败 |
| `-3` | 创建 DIB Section 失败 |
| `-4` | SelectObject 失败 |
| `-5` | UTF-8 转换失败 |
| `-6` | 创建窗口失败 |
| `-7` | 尺寸超限 |

**备注**

窗口标题支持 UTF-8（内部转 WideChar）。`resizable=true` 时窗口允许最大化，`SetMaximized` 才会生效。线程维度假设"一个窗口 + 一个主循环"。

---

### IsClosed

判断窗口是否已关闭。

**函数声明**
```cpp
bool IsClosed() const;
```

**参数**
无

**返回值**

| 值 | 说明 |
|-----|------|
| `true` | 窗口已关闭 |
| `false` | 窗口仍在运行 |

---

### Update

刷新画面并处理输入。保存上一帧按键状态、派发 Windows 消息、同步客户区尺寸和输入、提交帧缓冲到窗口、更新 deltaTime 和 FPS。

**函数声明**
```cpp
void Update();
```

**参数**
无

**返回值**
无

**备注**

这是游戏主循环中每帧必须调用的函数。若客户区与 framebuffer 同尺寸，直接 BitBlt；否则自动最近邻缩放提交。

---

### WaitFrame

帧率控制，基于绝对帧边界做节拍，使用高精度计时器维护帧起点。

**函数声明**
```cpp
void WaitFrame(int fps);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `fps` | `int` | 目标帧率，`<= 0` 时默认按 60 处理 |

**返回值**
无

**备注**

先用多媒体定时器事件或 `Sleep(1)` 粗等待，再用 `Sleep(0)` 短尾收尾减少 oversleep。

---

### GetDeltaTime

获取上一帧到当前帧的时间间隔。

**函数声明**
```cpp
double GetDeltaTime() const;
```

**参数**
无

**返回值**

帧间隔（秒），类型 `double`。

---

### GetFPS

获取当前帧率。

**函数声明**
```cpp
double GetFPS() const;
```

**参数**
无

**返回值**

当前帧率（每秒更新一次），类型 `double`。

---

### GetTime

获取运行总时间。

**函数声明**
```cpp
double GetTime() const;
```

**参数**
无

**返回值**

从 `Open()` 开始的总时间（秒），类型 `double`。在 `Open()` 前调用返回 `0.0`。

---

### GetWidth / GetHeight

获取 framebuffer 逻辑尺寸。

**函数声明**
```cpp
int GetWidth() const;
int GetHeight() const;
```

**参数**
无

**返回值**

Framebuffer 宽度/高度（像素），类型 `int`。

---

### WinResize

设置窗口客户区尺寸，不改变 framebuffer 尺寸。

**函数声明**
```cpp
void WinResize(int width, int height);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `width` | `int` | 客户区宽度（像素） |
| `height` | `int` | 客户区高度（像素） |

**返回值**
无

**备注**

对不可缩放窗口同样有效；若当前是最大化的可缩放窗口，会先还原再设置。

---

### SetMaximized

最大化或还原可缩放窗口。

**函数声明**
```cpp
void SetMaximized(bool maximized);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `maximized` | `bool` | `true` 最大化，`false` 还原 |

**返回值**
无

**备注**

仅在 `Open(..., ..., ..., ..., true)` 创建的窗口上有效。

---

### SetTitle

修改窗口标题。

**函数声明**
```cpp
void SetTitle(const char *title);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `title` | `const char *` | 新标题，支持 UTF-8 |

**返回值**
无

---

### ShowFps

在窗口标题栏显示实时 FPS。

**函数声明**
```cpp
void ShowFps(bool show);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `show` | `bool` | 是否显示 FPS |

**返回值**
无

**备注**

`show=true` 时标题栏显示为 `"原标题 (FPS: 58.8)"` 格式，每秒更新一次。

---

### ShowMouse

控制窗口客户区内的鼠标光标显示/隐藏。

**函数声明**
```cpp
void ShowMouse(bool show);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `show` | `bool` | 是否显示光标 |

**返回值**
无

**备注**

不使用全局 `ShowCursor` 引用计数，避免不同窗口/库互相干扰。窗口创建前调用也有效。

---

### ShowMessage

弹出消息框。

**函数声明**
```cpp
int ShowMessage(const char *text, const char *title = NULL, int buttons = MESSAGEBOX_OK);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `text` | `const char *` | 消息文本，支持 UTF-8 |
| `title` | `const char *` | 消息框标题，`NULL` 时使用窗口标题 |
| `buttons` | `int` | `MESSAGEBOX_OK` 或 `MESSAGEBOX_YESNO` |

**返回值**

| 值 | 说明 |
|-----|------|
| `MESSAGEBOX_RESULT_OK` | 点击 OK |
| `MESSAGEBOX_RESULT_YES` | 点击 Yes |
| `MESSAGEBOX_RESULT_NO` | 点击 No |

---

## 2. 帧缓冲

### Clear

用指定颜色填充当前裁剪矩形覆盖的帧缓冲区域。

**函数声明**
```cpp
void Clear(uint32_t color = COLOR_BLACK);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `color` | `uint32_t` | 填充颜色，ARGB 格式，默认黑色 |

**返回值**
无

**备注**

不做 Alpha 混合，即使颜色含透明通道也直接写入。

---

### SetPixel

设置指定像素颜色（带裁剪和边界检查）。

**函数声明**
```cpp
void SetPixel(int x, int y, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 像素 X 坐标 |
| `y` | `int` | 像素 Y 坐标 |
| `color` | `uint32_t` | 颜色，ARGB 格式 |

**返回值**
无

**备注**

当 `color` 的 Alpha 小于 255 时，按 source-over 规则与帧缓冲混合。

---

### GetPixel

获取指定像素颜色。

**函数声明**
```cpp
uint32_t GetPixel(int x, int y) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 像素 X 坐标 |
| `y` | `int` | 像素 Y 坐标 |

**返回值**

像素的 ARGB 颜色值，越界返回 `0`。

---

### SetClip

设置当前裁剪矩形。

**函数声明**
```cpp
void SetClip(int x, int y, int w, int h);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 裁剪矩形左上角 X |
| `y` | `int` | 裁剪矩形左上角 Y |
| `w` | `int` | 裁剪矩形宽度 |
| `h` | `int` | 裁剪矩形高度 |

**返回值**
无

**备注**

传入矩形自动与屏幕求交。`w <= 0 || h <= 0` 或求交后为空时，所有绘制函数不生效。

---

### ClearClip

清除当前裁剪，恢复整屏可见。

**函数声明**
```cpp
void ClearClip();
```

**参数**
无

**返回值**
无

---

### GetClip

读取当前有效裁剪矩形（已与屏幕求交）。

**函数声明**
```cpp
void GetClip(int *x, int *y, int *w, int *h) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int *` | 输出裁剪矩形左上角 X |
| `y` | `int *` | 输出裁剪矩形左上角 Y |
| `w` | `int *` | 输出裁剪矩形宽度 |
| `h` | `int *` | 输出裁剪矩形高度 |

**返回值**
无

---

### GetClipX / GetClipY / GetClipW / GetClipH

读取当前有效裁剪矩形的各个分量。

**函数声明**
```cpp
int GetClipX() const;
int GetClipY() const;
int GetClipW() const;
int GetClipH() const;
```

**参数**
无

**返回值**

裁剪矩形对应分量的值，类型 `int`。

---

### Screenshot

将当前 framebuffer 保存为 24-bit BMP 文件。

**函数声明**
```cpp
void Screenshot(const char *filename);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `filename` | `const char *` | 输出文件路径，支持 UTF-8 |

**返回值**
无

**备注**

从 ARGB32 提取 R/G/B，按 BGR 顺序写入 BMP，行从最后一行开始（bottom-up）。

---

## 3. 图形绘制

### DrawLine

绘制直线，使用 Bresenham 算法。

**函数声明**
```cpp
void DrawLine(int x1, int y1, int x2, int y2, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x1` | `int` | 起点 X 坐标 |
| `y1` | `int` | 起点 Y 坐标 |
| `x2` | `int` | 终点 X 坐标 |
| `y2` | `int` | 终点 Y 坐标 |
| `color` | `uint32_t` | 颜色，ARGB 格式 |

**返回值**
无

**备注**

先与当前裁剪矩形做线段裁剪，再逐点绘制。支持 Alpha 混合。

---

### DrawRect

绘制矩形边框。

**函数声明**
```cpp
void DrawRect(int x, int y, int w, int h, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 矩形左上角 X |
| `y` | `int` | 矩形左上角 Y |
| `w` | `int` | 矩形宽度 |
| `h` | `int` | 矩形高度 |
| `color` | `uint32_t` | 颜色，ARGB 格式 |

**返回值**
无

**备注**

`w <= 0 || h <= 0` 时直接返回。支持 Alpha 混合。

---

### FillRect

填充矩形区域。

**函数声明**
```cpp
void FillRect(int x, int y, int w, int h, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 矩形左上角 X |
| `y` | `int` | 矩形左上角 Y |
| `w` | `int` | 矩形宽度 |
| `h` | `int` | 矩形高度 |
| `color` | `uint32_t` | 颜色，ARGB 格式 |

**返回值**
无

**备注**

带裁剪，直接写帧缓冲。支持 Alpha 混合。

---

### DrawCircle

绘制圆形边框，使用中点圆算法。

**函数声明**
```cpp
void DrawCircle(int cx, int cy, int r, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `cx` | `int` | 圆心 X 坐标 |
| `cy` | `int` | 圆心 Y 坐标 |
| `r` | `int` | 半径 |
| `color` | `uint32_t` | 颜色，ARGB 格式 |

**返回值**
无

**备注**

按唯一对称点输出轮廓，避免半透明颜色在边界点重复混合。支持 Alpha 混合。

---

### FillCircle

填充圆形。

**函数声明**
```cpp
void FillCircle(int cx, int cy, int r, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `cx` | `int` | 圆心 X 坐标 |
| `cy` | `int` | 圆心 Y 坐标 |
| `r` | `int` | 半径 |
| `color` | `uint32_t` | 颜色，ARGB 格式 |

**返回值**
无

**备注**

复用椭圆扫描线填充路径，每行写一条水平线，避免半透明颜色重复覆盖。支持 Alpha 混合。

---

### DrawEllipse

绘制椭圆边框。

**函数声明**
```cpp
void DrawEllipse(int cx, int cy, int rx, int ry, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `cx` | `int` | 椭圆中心 X 坐标 |
| `cy` | `int` | 椭圆中心 Y 坐标 |
| `rx` | `int` | 横向半径 |
| `ry` | `int` | 纵向半径 |
| `color` | `uint32_t` | 颜色，ARGB 格式 |

**返回值**
无

**备注**

使用 midpoint ellipse 分区迭代，按唯一对称点输出轮廓。退化情况自动回退为点或直线。支持 Alpha 混合。

---

### FillEllipse

按扫描线方式填充椭圆。

**函数声明**
```cpp
void FillEllipse(int cx, int cy, int rx, int ry, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `cx` | `int` | 椭圆中心 X 坐标 |
| `cy` | `int` | 椭圆中心 Y 坐标 |
| `rx` | `int` | 横向半径 |
| `ry` | `int` | 纵向半径 |
| `color` | `uint32_t` | 颜色，ARGB 格式 |

**返回值**
无

**备注**

支持退化为点或直线。支持 Alpha 混合。

---

### DrawTriangle

绘制三角形边框（3 条 DrawLine）。

**函数声明**
```cpp
void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x1` | `int` | 第一个顶点 X |
| `y1` | `int` | 第一个顶点 Y |
| `x2` | `int` | 第二个顶点 X |
| `y2` | `int` | 第二个顶点 Y |
| `x3` | `int` | 第三个顶点 X |
| `y3` | `int` | 第三个顶点 Y |
| `color` | `uint32_t` | 颜色，ARGB 格式 |

**返回值**
无

**备注**

支持 Alpha 混合。

---

### FillTriangle

填充三角形，使用扫描线算法。

**函数声明**
```cpp
void FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x1` | `int` | 第一个顶点 X |
| `y1` | `int` | 第一个顶点 Y |
| `x2` | `int` | 第二个顶点 X |
| `y2` | `int` | 第二个顶点 Y |
| `x3` | `int` | 第三个顶点 X |
| `y3` | `int` | 第三个顶点 Y |
| `color` | `uint32_t` | 颜色，ARGB 格式 |

**返回值**
无

**备注**

先按 Y 排序三个顶点再逐行扫描。处理退化情况。边插值使用 `int64_t` 防溢出。支持 Alpha 混合。

---

## 4. 文字（内置 8x8 字体）

### DrawText

使用内嵌 8x8 位图字体绘制文字。

**函数声明**
```cpp
void DrawText(int x, int y, const char *text, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 文字左上角 X 坐标 |
| `y` | `int` | 文字左上角 Y 坐标 |
| `text` | `const char *` | 文字内容，ASCII 32~126 |
| `color` | `uint32_t` | 颜色，ARGB 格式 |

**返回值**
无

**备注**

支持 `\n` 换行（行间距 10 像素）。每个字符宽 8 像素。

---

### DrawNumber

将整数转为字符串后绘制。

**函数声明**
```cpp
void DrawNumber(int x, int y, int number, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 文字左上角 X 坐标 |
| `y` | `int` | 文字左上角 Y 坐标 |
| `number` | `int` | 整数值 |
| `color` | `uint32_t` | 颜色，ARGB 格式 |

**返回值**
无

**备注**

内部使用 `snprintf` 防溢出。

---

### DrawTextScale

放大版文字绘制，每个字体像素变为 `scale × scale` 矩形。

**函数声明**
```cpp
void DrawTextScale(int x, int y, const char *text, uint32_t color, int scale);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 文字左上角 X 坐标 |
| `y` | `int` | 文字左上角 Y 坐标 |
| `text` | `const char *` | 文字内容，ASCII 32~126 |
| `color` | `uint32_t` | 颜色，ARGB 格式 |
| `scale` | `int` | 放大倍数 |

**返回值**
无

**备注**

支持 `\n` 换行（行间距 `(8 + 2) * scale`）。

---

### DrawPrintf

格式化输出，类似 `printf`。

**函数声明**
```cpp
void DrawPrintf(int x, int y, uint32_t color, const char *fmt, ...);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 文字左上角 X 坐标 |
| `y` | `int` | 文字左上角 Y 坐标 |
| `color` | `uint32_t` | 颜色，ARGB 格式 |
| `fmt` | `const char *` | 格式字符串，支持 `%d`, `%s`, `%f` 等 |
| `...` | 可变参数 | 格式参数 |

**返回值**
无

**备注**

内部使用 `vsnprintf`（1024 字节缓冲），格式化后调用 `DrawText` 绘制。

---

### DrawPrintfScale

放大版格式化输出。

**函数声明**
```cpp
void DrawPrintfScale(int x, int y, uint32_t color, int scale, const char *fmt, ...);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 文字左上角 X 坐标 |
| `y` | `int` | 文字左上角 Y 坐标 |
| `color` | `uint32_t` | 颜色，ARGB 格式 |
| `scale` | `int` | 放大倍数 |
| `fmt` | `const char *` | 格式字符串 |
| `...` | 可变参数 | 格式参数 |

**返回值**
无

**备注**

格式化后调用 `DrawTextScale` 绘制，适合放大显示分数、标题等。

---

## 5. 字体文字渲染

### DrawTextFont

使用可缩放字体渲染文字，支持 UTF-8。

**函数声明**
```cpp
void DrawTextFont(int x, int y, const char *text, uint32_t color, const char *fontName, int fontSize);
void DrawTextFont(int x, int y, const char *text, uint32_t color, int fontSize);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 文字左上角 X 坐标 |
| `y` | `int` | 文字左上角 Y 坐标 |
| `text` | `const char *` | 文字内容，支持 UTF-8 和 `\n` |
| `color` | `uint32_t` | 颜色，ARGB 格式 |
| `fontName` | `const char *` | 字体名称（可选，不传使用默认字体） |
| `fontSize` | `int` | 字体大小（像素） |

**返回值**
无

**备注**

Windows 版内部用 GDI 实现。不传 `fontName` 时使用 `"Microsoft YaHei"`。当 `alpha = 0` 时不绘制；当 `0 < alpha < 255` 时按调用方 alpha 混合。

---

### DrawPrintfFont

字体版格式化输出。

**函数声明**
```cpp
void DrawPrintfFont(int x, int y, uint32_t color, const char *fontName, int fontSize, const char *fmt, ...);
void DrawPrintfFont(int x, int y, uint32_t color, int fontSize, const char *fmt, ...);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 文字左上角 X 坐标 |
| `y` | `int` | 文字左上角 Y 坐标 |
| `color` | `uint32_t` | 颜色，ARGB 格式 |
| `fontName` | `const char *` | 字体名称（可选） |
| `fontSize` | `int` | 字体大小（像素） |
| `fmt` | `const char *` | 格式字符串 |
| `...` | 可变参数 | 格式参数 |

**返回值**
无

**备注**

内部用 `vsnprintf` 组装文本后调用 `DrawTextFont`。

---

### GetTextWidthFont

获取文字在指定字体下的宽度。

**函数声明**
```cpp
int GetTextWidthFont(const char *text, const char *fontName, int fontSize);
int GetTextWidthFont(const char *text, int fontSize);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `text` | `const char *` | 文字内容 |
| `fontName` | `const char *` | 字体名称（可选） |
| `fontSize` | `int` | 字体大小（像素） |

**返回值**

文字宽度（像素），类型 `int`。

---

### GetTextHeightFont

获取文字在指定字体下的高度。

**函数声明**
```cpp
int GetTextHeightFont(const char *text, const char *fontName, int fontSize);
int GetTextHeightFont(const char *text, int fontSize);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `text` | `const char *` | 文字内容 |
| `fontName` | `const char *` | 字体名称（可选） |
| `fontSize` | `int` | 字体大小（像素） |

**返回值**

文字高度（像素），类型 `int`。

**备注**

支持多行文本高度计算。

---

## 6. 精灵

### CreateSprite

创建空白精灵。

**函数声明**
```cpp
int CreateSprite(int width, int height);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `width` | `int` | 精灵宽度，范围 1~16384 |
| `height` | `int` | 精灵高度，范围 1~16384 |

**返回值**

精灵 ID，失败返回 `-1`。

**备注**

像素初始化为全 0（透明黑）。

---

### LoadSprite

通用图片加载，支持 PNG/JPG/BMP/GIF/TIFF。

**函数声明**
```cpp
int LoadSprite(const char *filename);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `filename` | `const char *` | 图片路径，支持 PNG/JPG/BMP/GIF/TIFF，UTF-8 |

**返回值**

精灵 ID，失败返回 `-1`。

**备注**

首次调用时懒加载 GDI+。始终请求 32bppARGB 格式。若 GDI+ 不可用且文件为 BMP，自动回退到 `LoadSpriteBMP`。24 位图片 alpha 自动修正为 255。

---

### LoadSpriteBMP

从 BMP 文件加载精灵，支持 8/24/32-bit。

**函数声明**
```cpp
int LoadSpriteBMP(const char *filename);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `filename` | `const char *` | BMP 文件路径，支持 8/24/32-bit，UTF-8 |

**返回值**

精灵 ID，失败返回 `-1`。

**备注**

8-bit 调色板自动转换为 32-bit ARGB。处理 bottom-up / top-down 行序。失败时自动回滚精灵槽位。

---

### FreeSprite

释放精灵。

**函数声明**
```cpp
void FreeSprite(int id);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `id` | `int` | 精灵 ID |

**返回值**
无

---

### DrawSprite

绘制精灵（不透明快路径）。

**函数声明**
```cpp
void DrawSprite(int id, int x, int y);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `id` | `int` | 精灵 ID |
| `x` | `int` | 绘制位置 X |
| `y` | `int` | 绘制位置 Y |

**返回值**
无

**备注**

默认走不透明快路径，无翻转时直接逐行拷贝像素。如需透明孔洞，改用 `DrawSpriteEx` 并传入 `SPRITE_COLORKEY` 或 `SPRITE_ALPHA`。

---

### DrawSpriteEx

带标志的精灵绘制。

**函数声明**
```cpp
void DrawSpriteEx(int id, int x, int y, int flags);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `id` | `int` | 精灵 ID |
| `x` | `int` | 绘制位置 X |
| `y` | `int` | 绘制位置 Y |
| `flags` | `int` | 绘制标志，可组合 |

**返回值**
无

**备注**

标志位：`SPRITE_FLIP_H`(1) 水平翻转，`SPRITE_FLIP_V`(2) 垂直翻转，`SPRITE_COLORKEY`(4) 透明色模式，`SPRITE_ALPHA`(8) Alpha 混合。无翻转且无 Alpha 时优先逐行 `memcpy`。

---

### DrawSpriteRegion

绘制精灵的子区域（sprite sheet 切图）。

**函数声明**
```cpp
void DrawSpriteRegion(int id, int x, int y, int sx, int sy, int sw, int sh);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `id` | `int` | 精灵 ID |
| `x` | `int` | 绘制位置 X |
| `y` | `int` | 绘制位置 Y |
| `sx` | `int` | 源区域左上角 X |
| `sy` | `int` | 源区域左上角 Y |
| `sw` | `int` | 源区域宽度 |
| `sh` | `int` | 源区域高度 |

**返回值**
无

---

### DrawSpriteRegionEx

带标志绘制精灵子区域。

**函数声明**
```cpp
void DrawSpriteRegionEx(int id, int x, int y, int sx, int sy, int sw, int sh, int flags = 0);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `id` | `int` | 精灵 ID |
| `x` | `int` | 绘制位置 X |
| `y` | `int` | 绘制位置 Y |
| `sx` | `int` | 源区域左上角 X |
| `sy` | `int` | 源区域左上角 Y |
| `sw` | `int` | 源区域宽度 |
| `sh` | `int` | 源区域高度 |
| `flags` | `int` | 绘制标志，默认 0 |

**返回值**
无

---

### DrawSpriteScaled

按目标尺寸缩放绘制精灵。

**函数声明**
```cpp
void DrawSpriteScaled(int id, int x, int y, int w, int h, int flags = 0);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `id` | `int` | 精灵 ID |
| `x` | `int` | 绘制位置 X |
| `y` | `int` | 绘制位置 Y |
| `w` | `int` | 目标宽度 |
| `h` | `int` | 目标高度 |
| `flags` | `int` | 绘制标志，默认 0 |

**返回值**
无

**备注**

使用最近邻采样，适合像素风和教学场景。

---

### DrawSpriteRotated

将精灵绕自身中心旋转后绘制。

**函数声明**
```cpp
void DrawSpriteRotated(int id, int cx, int cy, double angleDeg, int flags = 0);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `id` | `int` | 精灵 ID |
| `cx` | `int` | 旋转中心 X（精灵中心落在此处） |
| `cy` | `int` | 旋转中心 Y |
| `angleDeg` | `double` | 旋转角度，> 0 顺时针 |
| `flags` | `int` | 绘制标志，默认 0 |

**返回值**
无

**备注**

使用最近邻旋转采样。翻转、Color Key、Alpha 语义与其他 `DrawSprite*` 一致。

---

### DrawSpriteFrame

按帧号绘制 sprite sheet 中的帧。

**函数声明**
```cpp
void DrawSpriteFrame(int id, int x, int y, int frameW, int frameH, int frameIndex, int flags = 0);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `id` | `int` | 精灵 ID（sprite sheet） |
| `x` | `int` | 绘制位置 X |
| `y` | `int` | 绘制位置 Y |
| `frameW` | `int` | 每帧宽度 |
| `frameH` | `int` | 每帧高度 |
| `frameIndex` | `int` | 帧号（从左到右、从上到下） |
| `flags` | `int` | 绘制标志，默认 0 |

**返回值**
无

**备注**

每行帧数由 `spriteWidth / frameW` 自动推导。

---

### DrawSpriteFrameScaled

按帧号选取子区域后缩放绘制。

**函数声明**
```cpp
void DrawSpriteFrameScaled(int id, int x, int y, int frameW, int frameH, int frameIndex, int w, int h, int flags = 0);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `id` | `int` | 精灵 ID |
| `x` | `int` | 绘制位置 X |
| `y` | `int` | 绘制位置 Y |
| `frameW` | `int` | 每帧宽度 |
| `frameH` | `int` | 每帧高度 |
| `frameIndex` | `int` | 帧号 |
| `w` | `int` | 目标宽度 |
| `h` | `int` | 目标高度 |
| `flags` | `int` | 绘制标志，默认 0 |

**返回值**
无

---

### DrawSpriteFrameRotated

按帧号选取子区域后旋转绘制。

**函数声明**
```cpp
void DrawSpriteFrameRotated(int id, int cx, int cy, int frameW, int frameH, int frameIndex, double angleDeg, int flags = 0);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `id` | `int` | 精灵 ID |
| `cx` | `int` | 旋转中心 X |
| `cy` | `int` | 旋转中心 Y |
| `frameW` | `int` | 每帧宽度 |
| `frameH` | `int` | 每帧高度 |
| `frameIndex` | `int` | 帧号 |
| `angleDeg` | `double` | 旋转角度 |
| `flags` | `int` | 绘制标志，默认 0 |

**返回值**
无

---

### SetSpritePixel

修改精灵指定像素。

**函数声明**
```cpp
void SetSpritePixel(int id, int x, int y, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `id` | `int` | 精灵 ID |
| `x` | `int` | 像素 X 坐标 |
| `y` | `int` | 像素 Y 坐标 |
| `color` | `uint32_t` | 颜色，ARGB 格式 |

**返回值**
无

---

### GetSpritePixel

读取精灵指定像素。

**函数声明**
```cpp
uint32_t GetSpritePixel(int id, int x, int y) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `id` | `int` | 精灵 ID |
| `x` | `int` | 像素 X 坐标 |
| `y` | `int` | 像素 Y 坐标 |

**返回值**

像素的 ARGB 颜色值。

---

### GetSpriteWidth / GetSpriteHeight

获取精灵尺寸。

**函数声明**
```cpp
int GetSpriteWidth(int id) const;
int GetSpriteHeight(int id) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `id` | `int` | 精灵 ID |

**返回值**

精灵宽度/高度（像素），无效 ID 返回 `0`。

---

### SetSpriteColorKey / GetSpriteColorKey

设置或读取精灵的 Color Key。

**函数声明**
```cpp
void SetSpriteColorKey(int id, uint32_t color);
uint32_t GetSpriteColorKey(int id) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `id` | `int` | 精灵 ID |
| `color` | `uint32_t` | Color Key 颜色（设置时） |

**返回值**

读取函数返回 Color Key 颜色，默认 `COLORKEY_DEFAULT`（品红 `0xFFFF00FF`）。设置函数无返回值。

**备注**

配合 `SPRITE_COLORKEY` 标志使用。

---

## 7. 输入

### IsKeyDown

检测按键是否正在按下（持续按住也返回 `true`）。

**函数声明**
```cpp
bool IsKeyDown(int key) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `key` | `int` | 按键码（如 `KEY_UP`, `KEY_A`） |

**返回值**

按键是否正在按下。

---

### IsKeyPressed

边沿检测，按键是否刚按下。

**函数声明**
```cpp
bool IsKeyPressed(int key) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `key` | `int` | 按键码 |

**返回值**

当前帧按下且上一帧未按下时返回 `true`。

---

### IsKeyReleased

边沿检测，按键是否刚松开。

**函数声明**
```cpp
bool IsKeyReleased(int key) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `key` | `int` | 按键码 |

**返回值**

当前帧未按下且上一帧按下时返回 `true`。

---

### GetMouseX / GetMouseY

获取鼠标逻辑位置（已换算到 framebuffer 坐标）。

**函数声明**
```cpp
int GetMouseX() const;
int GetMouseY() const;
```

**参数**
无

**返回值**

鼠标 X / Y 坐标，类型 `int`。

**备注**

按当前窗口缩放比例反算。窗口与 framebuffer 同尺寸时等价于普通客户区像素坐标。

---

### IsMouseDown

检测鼠标按键是否按下。

**函数声明**
```cpp
bool IsMouseDown(int button) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `button` | `int` | `MOUSE_LEFT`(0), `MOUSE_RIGHT`(1), `MOUSE_MIDDLE`(2) |

**返回值**

鼠标按键是否按下。

---

### IsMousePressed

边沿检测，鼠标按键是否刚按下。

**函数声明**
```cpp
bool IsMousePressed(int button) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `button` | `int` | 鼠标按键 |

**返回值**

当前帧按下且上一帧未按下时返回 `true`。

---

### IsMouseReleased

边沿检测，鼠标按键是否刚松开。

**函数声明**
```cpp
bool IsMouseReleased(int button) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `button` | `int` | 鼠标按键 |

**返回值**

当前帧未按下且上一帧按下时返回 `true`。

---

### GetMouseWheelDelta

获取自上次 `Update()` 以来累计的滚轮增量。

**函数声明**
```cpp
int GetMouseWheelDelta() const;
```

**参数**
无

**返回值**

滚轮增量，Windows 标准一格通常为 `120` 或 `-120`。

**备注**

读取不会清零，在 `Update()` 开始时刷新为 0。

---

### IsActive

获取窗口当前是否处于激活状态。

**函数声明**
```cpp
bool IsActive() const;
```

**参数**
无

**返回值**

窗口是否激活。

**备注**

适合在游戏失焦时暂停输入或显示暂停提示。

---

## 8. 声音

### PlayBeep

阻塞式蜂鸣。

**函数声明**
```cpp
void PlayBeep(int frequency, int duration);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `frequency` | `int` | 频率（Hz） |
| `duration` | `int` | 持续时间（毫秒） |

**返回值**
无

---

### PlayWAV

播放 WAV 音效（异步，多通道）。

**函数声明**
```cpp
int PlayWAV(const char *filename, int repeat = 1, int volume = 1000);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `filename` | `const char *` | WAV 文件路径，UTF-8 |
| `repeat` | `int` | 重复次数，-1 为无限循环，默认 1 |
| `volume` | `int` | 通道音量 0~1000，默认 1000 |

**返回值**

成功返回通道 ID（正整数），文件错误返回 -1，音频设备初始化失败返回 -2。

**备注**

使用 waveOut 软件混音器播放。同一 WAV 文件可重叠播放（每次分配独立通道）。WAV 文件按 `filename` 缓存，重复播放同一文件不重新读取。音频设备惰性初始化，首次调用时才创建 waveOut 设备。与 `PlayMusic` 独立通道，可同时播放。

---

### StopWAV

停止指定通道的 WAV 播放。

**函数声明**
```cpp
int StopWAV(int channel);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `channel` | `int` | `PlayWAV` 返回的通道 ID |

**返回值**

成功返回 1，无效通道返回 0。

---

### IsPlaying

查询指定通道是否仍在播放。

**函数声明**
```cpp
int IsPlaying(int channel);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `channel` | `int` | `PlayWAV` 返回的通道 ID |

**返回值**

正在播放返回 1，已停止或无效通道返回 0。

---

### SetVolume

设置指定通道音量。

**函数声明**
```cpp
int SetVolume(int channel, int volume);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `channel` | `int` | `PlayWAV` 返回的通道 ID |
| `volume` | `int` | 音量 0~1000 |

**返回值**

成功返回新音量值，无效通道返回 -1。

---

### StopAll

停止所有音效播放。

**函数声明**
```cpp
void StopAll();
```

**参数**
无

**返回值**
无

---

### SetMasterVolume

设置主音量。

**函数声明**
```cpp
int SetMasterVolume(int volume);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `volume` | `int` | 主音量 0~1000，自动钳制 |

**返回值**

返回实际设置的主音量值。

---

### GetMasterVolume

获取当前主音量。

**函数声明**
```cpp
int GetMasterVolume() const;
```

**参数**
无

**返回值**

当前主音量值（0~1000）。

---

### PlayMusic

使用 MCI 播放背景音乐。

**函数声明**
```cpp
bool PlayMusic(const char *filename, bool loop = true);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `filename` | `const char *` | 音乐路径，支持 MP3/MIDI/WAV，UTF-8 |
| `loop` | `bool` | 是否循环播放，默认 `true` |

**返回值**

成功返回 `true`，失败返回 `false`。

**备注**

按扩展名选择 MCI 设备：`.mp3` → `mpegvideo`，`.mid/.midi` → `sequencer`，`.wav` → `waveaudio`。MIDI 使用 notify 回调重播。拒绝包含引号和换行的文件名（防止命令注入）。同一时刻只能播放一首背景音乐。

---

### StopMusic

停止背景音乐。

**函数声明**
```cpp
void StopMusic();
```

**参数**
无

**返回值**
无

---

### IsMusicPlaying

获取背景音乐播放状态。

**函数声明**
```cpp
bool IsMusicPlaying() const;
```

**参数**
无

**返回值**

是否正在播放。

---

## 9. Tilemap

### CreateTilemap

创建瓦片地图。

**函数声明**
```cpp
int CreateTilemap(int cols, int rows, int tileSize, int tilesetId);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `cols` | `int` | 地图列数，不超过 4096 |
| `rows` | `int` | 地图行数，不超过 4096 |
| `tileSize` | `int` | 瓦片边长（像素） |
| `tilesetId` | `int` | tileset 精灵 ID |

**返回值**

地图 ID，失败返回 `-1`。

**备注**

tileset 精灵按 `tileSize` 切分瓦片，编号从 0 开始。所有格子初始化为 `-1`（空）。若 tileset 在当前尺寸下切不出任何瓦片，创建失败。

---

### SaveTilemap

保存地图为 `.glm` 文件。

**函数声明**
```cpp
bool SaveTilemap(const char *filename, int mapId) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `filename` | `const char *` | 输出路径，`.glm` 格式，UTF-8 |
| `mapId` | `int` | 地图 ID |

**返回值**

成功返回 `true`。

**备注**

第一行 `GLM1`，第二行 `tileSize rows cols`，后续每行瓦片数据。不保存 tileset 路径。

---

### LoadTilemap

从 `.glm` 文件创建地图。

**函数声明**
```cpp
int LoadTilemap(const char *filename, int tilesetId);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `filename` | `const char *` | `.glm` 文件路径，UTF-8 |
| `tilesetId` | `int` | tileset 精灵 ID |

**返回值**

新地图 ID，失败返回 `-1`。

**备注**

第一行必须是 `GLM1`（允许 UTF-8 BOM）。数据不足时补 `-1`，超出时忽略。不从文件读取 tileset。

---

### FreeTilemap

释放地图。

**函数声明**
```cpp
void FreeTilemap(int mapId);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `mapId` | `int` | 地图 ID |

**返回值**
无

**备注**

不释放 tileset 精灵（由用户通过 `FreeSprite` 管理）。

---

### SetTile

设置瓦片。

**函数声明**
```cpp
void SetTile(int mapId, int col, int row, int tileId);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `mapId` | `int` | 地图 ID |
| `col` | `int` | 瓦片列号 |
| `row` | `int` | 瓦片行号 |
| `tileId` | `int` | 瓦片编号，`-1` 为空，`< -1` 忽略 |

**返回值**
无

**备注**

超出 tileset 范围的非负 `tileId` 会原样写入，绘制时自动跳过。

---

### GetTile

读取瓦片编号。

**函数声明**
```cpp
int GetTile(int mapId, int col, int row) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `mapId` | `int` | 地图 ID |
| `col` | `int` | 瓦片列号 |
| `row` | `int` | 瓦片行号 |

**返回值**

瓦片编号，越界返回 `-1`。

---

### GetTilemapCols / GetTilemapRows

获取地图网格列数/行数。

**函数声明**
```cpp
int GetTilemapCols(int mapId) const;
int GetTilemapRows(int mapId) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `mapId` | `int` | 地图 ID |

**返回值**

列数/行数，无效 ID 返回 `0`。

---

### GetTileSize

获取瓦片边长（像素）。

**函数声明**
```cpp
int GetTileSize(int mapId) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `mapId` | `int` | 地图 ID |

**返回值**

瓦片边长，无效 ID 返回 `0`。

---

### WorldToTileCol / WorldToTileRow

像素坐标转瓦片坐标。

**函数声明**
```cpp
int WorldToTileCol(int mapId, int x) const;
int WorldToTileRow(int mapId, int y) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `mapId` | `int` | 地图 ID |
| `x` | `int` | 像素 X 坐标 |
| `y` | `int` | 像素 Y 坐标 |

**返回值**

瓦片列号/行号，向下取整。

**备注**

负坐标也能得到符合直觉的结果（如 `x = -1`, `tileSize = 16` → `-1`）。

---

### GetTileAtPixel

按像素位置读取瓦片。

**函数声明**
```cpp
int GetTileAtPixel(int mapId, int x, int y) const;
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `mapId` | `int` | 地图 ID |
| `x` | `int` | 像素 X 坐标 |
| `y` | `int` | 像素 Y 坐标 |

**返回值**

瓦片编号，越界或空返回 `-1`。

**备注**

适合脚下地面检测、像素级碰撞。

---

### FillTileRect

批量填充矩形瓦片区域。

**函数声明**
```cpp
void FillTileRect(int mapId, int col, int row, int cols, int rows, int tileId);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `mapId` | `int` | 地图 ID |
| `col` | `int` | 起始列号 |
| `row` | `int` | 起始行号 |
| `cols` | `int` | 填充列数 |
| `rows` | `int` | 填充行数 |
| `tileId` | `int` | 瓦片编号 |

**返回值**
无

**备注**

自动裁剪到地图边界。`tileId < -1` 时忽略。

---

### ClearTilemap

清空整张地图为同一瓦片。

**函数声明**
```cpp
void ClearTilemap(int mapId, int tileId = -1);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `mapId` | `int` | 地图 ID |
| `tileId` | `int` | 填充瓦片编号，默认 `-1` 表示清空 |

**返回值**
无

**备注**

`tileId < -1` 时忽略。

---

### DrawTilemap

绘制瓦片地图。

**函数声明**
```cpp
void DrawTilemap(int mapId, int x, int y, int flags = 0);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `mapId` | `int` | 地图 ID |
| `x` | `int` | 屏幕位置 X（卷轴时传 `-cameraX`） |
| `y` | `int` | 屏幕位置 Y（卷轴时传 `-cameraY`） |
| `flags` | `int` | 绘制标志，默认 0 |

**返回值**
无

**备注**

`flags` 与 `DrawSpriteEx` 一致：`0` 不透明快路径，`SPRITE_COLORKEY` 透明色，`SPRITE_ALPHA` Alpha 混合。只绘制裁剪矩形内可见瓦片，自动跳过超出 tileset 范围的格子。

---

## 10. 场景管理

### SetScene

设置下一帧要切换的场景。

**函数声明**
```cpp
void SetScene(int scene);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `scene` | `int` | 场景编号 |

**返回值**
无

**备注**

不会立即生效，在下一次 `Update()` 时处理。可通过 `SetScene(GetScene())` 重启当前场景。

---

### GetScene

获取当前场景编号。

**函数声明**
```cpp
int GetScene() const;
```

**参数**
无

**返回值**

当前场景编号，初始为 `0`。

---

### IsSceneChanged

判断本帧是否刚切换到新场景。

**函数声明**
```cpp
bool IsSceneChanged() const;
```

**参数**
无

**返回值**

新场景第一帧返回 `true`，之后返回 `false`。

**备注**

初始帧也返回 `true`，方便首帧初始化。

---

### GetPreviousScene

获取切换前的场景编号。

**函数声明**
```cpp
int GetPreviousScene() const;
```

**参数**
无

**返回值**

切换前的场景编号，首帧或未切换时返回 `0`。

---

## 11. UI 控件

### Button

立即模式按钮。

**函数声明**
```cpp
bool Button(int x, int y, int w, int h, const char *text, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 按钮左上角 X |
| `y` | `int` | 按钮左上角 Y |
| `w` | `int` | 按钮宽度 |
| `h` | `int` | 按钮高度 |
| `text` | `const char *` | 按钮文字，使用内置 8x8 字体 |
| `color` | `uint32_t` | 按钮基色 |

**返回值**

在按钮内按下并在按钮内松开左键时返回 `true`。

**备注**

视觉状态分 `normal`、`hover`、`pressed` 三种。`color` 作为基色，悬停与按下的明暗变化由库内部自动计算。按下后拖出按钮区域不触发，拖回区域内松开仍可触发。

---

### Checkbox

立即模式复选框。

**函数声明**
```cpp
bool Checkbox(int x, int y, const char *text, bool *checked);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 复选框左上角 X |
| `y` | `int` | 复选框左上角 Y |
| `text` | `const char *` | 标签文字，使用内置 8x8 字体 |
| `checked` | `bool *` | 勾选状态指针（必须非空） |

**返回值**

状态变化时返回 `true`（翻转 `*checked`）。

**备注**

点击区域覆盖 16x16 方框和文字标签。状态分 `checked`、`checked-hover`、`unchecked`、`unchecked-hover` 四种。

---

### RadioBox

立即模式单选框。

**函数声明**
```cpp
bool RadioBox(int x, int y, const char *text, int *value, int index);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 单选框左上角 X |
| `y` | `int` | 单选框左上角 Y |
| `text` | `const char *` | 标签文字 |
| `value` | `int *` | 组共享值指针（必须非空） |
| `index` | `int` | 该项编号 |

**返回值**

选中该项时返回 `true`（设置 `*value = index`）。

**备注**

同一组共享 `value` 指针实现互斥。点击区域覆盖 16x16 圆形和文字标签，选中时显示中心圆点。

---

### ToggleButton

立即模式开关按钮。

**函数声明**
```cpp
bool ToggleButton(int x, int y, int w, int h, const char *text, bool *toggled, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 按钮左上角 X |
| `y` | `int` | 按钮左上角 Y |
| `w` | `int` | 按钮宽度 |
| `h` | `int` | 按钮高度 |
| `text` | `const char *` | 按钮文字 |
| `toggled` | `bool *` | 开关状态指针（必须非空） |
| `color` | `uint32_t` | 按钮基色 |

**返回值**

状态变化时返回 `true`（翻转 `*toggled`）。

**备注**

`toggled == true` 时持续显示凹陷外观。视觉分 `normal`、`hover`、`toggled`、`toggled-hover` 四种。

---

## 12. 存档读写

所有存档函数为 `static`，通过 `GameLib::SaveInt(...)` 直接调用，不需要实例。

存档文件为纯文本格式，第一行固定 `GAMELIB_SAVE`，后续每行 `key=value`。`key` 不能为空且不能包含 `=`、`\r`、`\n`。所有路径按 UTF-8 解释。

### SaveInt

将整数写入存档。

**函数声明**
```cpp
static bool SaveInt(const char *filename, const char *key, int value);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `filename` | `const char *` | 存档路径，UTF-8 |
| `key` | `const char *` | 键名 |
| `value` | `int` | 整数值 |

**返回值**

成功返回 `true`。

**备注**

文件不存在则创建，key 已存在则覆盖。

---

### SaveFloat

将浮点数写入存档。

**函数声明**
```cpp
static bool SaveFloat(const char *filename, const char *key, float value);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `filename` | `const char *` | 存档路径，UTF-8 |
| `key` | `const char *` | 键名 |
| `value` | `float` | 浮点数值 |

**返回值**

成功返回 `true`。

**备注**

以 `%g` 格式存储。

---

### SaveString

将字符串写入存档。

**函数声明**
```cpp
static bool SaveString(const char *filename, const char *key, const char *value);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `filename` | `const char *` | 存档路径，UTF-8 |
| `key` | `const char *` | 键名 |
| `value` | `const char *` | 字符串值 |

**返回值**

成功返回 `true`。

**备注**

`\` 转义为 `\\`，`\n` 转义为 `\n`，读取时还原。

---

### LoadInt

从存档读取整数。

**函数声明**
```cpp
static int LoadInt(const char *filename, const char *key, int defaultValue = 0);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `filename` | `const char *` | 存档路径，UTF-8 |
| `key` | `const char *` | 键名 |
| `defaultValue` | `int` | 默认值 |

**返回值**

整数值，文件/key 不存在或解析失败返回 `defaultValue`。

---

### LoadFloat

从存档读取浮点数。

**函数声明**
```cpp
static float LoadFloat(const char *filename, const char *key, float defaultValue = 0.0f);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `filename` | `const char *` | 存档路径，UTF-8 |
| `key` | `const char *` | 键名 |
| `defaultValue` | `float` | 默认值 |

**返回值**

浮点数值，不存在返回 `defaultValue`。

---

### LoadString

从存档读取字符串。

**函数声明**
```cpp
static const char *LoadString(const char *filename, const char *key, const char *defaultValue = "");
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `filename` | `const char *` | 存档路径，UTF-8 |
| `key` | `const char *` | 键名 |
| `defaultValue` | `const char *` | 默认值 |

**返回值**

字符串指针，指向内部静态缓冲区（最大 1023 字符），下次调用前有效。

---

### HasSaveKey

判断存档中是否存在指定 key。

**函数声明**
```cpp
static bool HasSaveKey(const char *filename, const char *key);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `filename` | `const char *` | 存档路径，UTF-8 |
| `key` | `const char *` | 键名 |

**返回值**

存在返回 `true`。

---

### DeleteSaveKey

从存档中删除指定 key。

**函数声明**
```cpp
static bool DeleteSaveKey(const char *filename, const char *key);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `filename` | `const char *` | 存档路径，UTF-8 |
| `key` | `const char *` | 键名 |

**返回值**

成功返回 `true`。

**备注**

删除后文件只剩头部则删除整个文件。

---

### DeleteSave

删除整个存档文件。

**函数声明**
```cpp
static bool DeleteSave(const char *filename);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `filename` | `const char *` | 存档路径，UTF-8 |

**返回值**

成功返回 `true`。

---

## 13. 工具函数

所有工具函数为 `static`。

### Random

返回指定范围内的随机整数。

**函数声明**
```cpp
static int Random(int minVal, int maxVal);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `minVal` | `int` | 最小值 |
| `maxVal` | `int` | 最大值 |

**返回值**

`[minVal, maxVal]` 范围内的随机整数。

**备注**

内部使用 `rand()`，构造函数自动 `srand(time(NULL))`。

---

### RectOverlap

AABB 矩形碰撞检测。

**函数声明**
```cpp
static bool RectOverlap(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x1` | `int` | 第一个矩形左上角 X |
| `y1` | `int` | 第一个矩形左上角 Y |
| `w1` | `int` | 第一个矩形宽度 |
| `h1` | `int` | 第一个矩形高度 |
| `x2` | `int` | 第二个矩形左上角 X |
| `y2` | `int` | 第二个矩形左上角 Y |
| `w2` | `int` | 第二个矩形宽度 |
| `h2` | `int` | 第二个矩形高度 |

**返回值**

两个矩形是否重叠。

---

### CircleOverlap

圆形碰撞检测。

**函数声明**
```cpp
static bool CircleOverlap(int cx1, int cy1, int r1, int cx2, int cy2, int r2);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `cx1` | `int` | 第一个圆心 X |
| `cy1` | `int` | 第一个圆心 Y |
| `r1` | `int` | 第一个半径 |
| `cx2` | `int` | 第二个圆心 X |
| `cy2` | `int` | 第二个圆心 Y |
| `r2` | `int` | 第二个半径 |

**返回值**

两个圆是否重叠。

**备注**

使用距离平方比较，无浮点开方，`int64_t` 防溢出。

---

### PointInRect

判断点是否在矩形内。

**函数声明**
```cpp
static bool PointInRect(int px, int py, int x, int y, int w, int h);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `px` | `int` | 点 X 坐标 |
| `py` | `int` | 点 Y 坐标 |
| `x` | `int` | 矩形左上角 X |
| `y` | `int` | 矩形左上角 Y |
| `w` | `int` | 矩形宽度 |
| `h` | `int` | 矩形高度 |

**返回值**

点是否在矩形内。

---

### Distance

计算两点距离。

**函数声明**
```cpp
static float Distance(int x1, int y1, int x2, int y2);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x1` | `int` | 第一个点 X |
| `y1` | `int` | 第一个点 Y |
| `x2` | `int` | 第二个点 X |
| `y2` | `int` | 第二个点 Y |

**返回值**

两点距离（浮点），使用 `sqrtf`。

---

### DrawGrid

绘制网格线。

**函数声明**
```cpp
void DrawGrid(int x, int y, int rows, int cols, int cellSize, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 网格左上角 X |
| `y` | `int` | 网格左上角 Y |
| `rows` | `int` | 行数 |
| `cols` | `int` | 列数 |
| `cellSize` | `int` | 单元格边长 |
| `color` | `uint32_t` | 网格线颜色 |

**返回值**
无

---

### FillCell

填充网格中的一个单元格。

**函数声明**
```cpp
void FillCell(int gridX, int gridY, int row, int col, int cellSize, uint32_t color);
```

**参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| `gridX` | `int` | 网格左上角 X |
| `gridY` | `int` | 网格左上角 Y |
| `row` | `int` | 单元格行号 |
| `col` | `int` | 单元格列号 |
| `cellSize` | `int` | 单元格边长 |
| `color` | `uint32_t` | 填充颜色 |

**返回值**
无

**备注**

留 1 像素内边距避免覆盖网格线。

---

## 14. 常量参考

### 颜色常量

| 常量 | 值 | 说明 |
|------|-----|------|
| `COLOR_BLACK` | `0xFF000000` | 黑色 |
| `COLOR_WHITE` | `0xFFFFFFFF` | 白色 |
| `COLOR_RED` | `0xFFFF0000` | 红色 |
| `COLOR_GREEN` | `0xFF00FF00` | 绿色 |
| `COLOR_BLUE` | `0xFF0000FF` | 蓝色 |
| `COLOR_YELLOW` | `0xFFFFFF00` | 黄色 |
| `COLOR_CYAN` | `0xFF00FFFF` | 青色 |
| `COLOR_MAGENTA` | `0xFFFF00FF` | 品红 |
| `COLOR_ORANGE` | `0xFFFF8800` | 橙色 |
| `COLOR_PINK` | `0xFFFF88CC` | 粉色 |
| `COLOR_PURPLE` | `0xFF8800FF` | 紫色 |
| `COLOR_GRAY` | `0xFF888888` | 灰色 |
| `COLOR_DARK_GRAY` | `0xFF444444` | 深灰 |
| `COLOR_LIGHT_GRAY` | `0xFFCCCCCC` | 浅灰 |
| `COLOR_DARK_RED` | `0xFF880000` | 深红 |
| `COLOR_DARK_GREEN` | `0xFF008800` | 深绿 |
| `COLOR_DARK_BLUE` | `0xFF000088` | 深蓝 |
| `COLOR_SKY_BLUE` | `0xFF87CEEB` | 天蓝 |
| `COLOR_BROWN` | `0xFF8B4513` | 棕色 |
| `COLOR_GOLD` | `0xFFFFD700` | 金色 |
| `COLOR_TRANSPARENT` | `0x00000000` | 透明 |

### 颜色宏

| 宏 | 说明 |
|----|------|
| `COLOR_RGB(r, g, b)` | 构造不透明颜色（每分量 `& 0xFF` 防溢出） |
| `COLOR_ARGB(a, r, g, b)` | 构造 ARGB 颜色 |
| `COLOR_GET_A(c)` | 提取 Alpha 分量 |
| `COLOR_GET_R(c)` | 提取 Red 分量 |
| `COLOR_GET_G(c)` | 提取 Green 分量 |
| `COLOR_GET_B(c)` | 提取 Blue 分量 |

### 键盘常量

| 类别 | 常量 |
|------|------|
| 方向键 | `KEY_LEFT`, `KEY_RIGHT`, `KEY_UP`, `KEY_DOWN` |
| 功能键 | `KEY_SPACE`, `KEY_ENTER`, `KEY_ESCAPE`, `KEY_TAB`, `KEY_SHIFT`, `KEY_CONTROL`, `KEY_BACK` |
| 字母键 | `KEY_A` ~ `KEY_Z` (0x41~0x5A) |
| 数字键 | `KEY_0` ~ `KEY_9` (0x30~0x39) |
| F键 | `KEY_F1` ~ `KEY_F12` |

### 鼠标按键

| 常量 | 值 | 说明 |
|------|-----|------|
| `MOUSE_LEFT` | 0 | 左键 |
| `MOUSE_RIGHT` | 1 | 右键 |
| `MOUSE_MIDDLE` | 2 | 中键 |

### 精灵标志

| 常量 | 值 | 说明 |
|------|-----|------|
| `SPRITE_FLIP_H` | 1 | 水平翻转 |
| `SPRITE_FLIP_V` | 2 | 垂直翻转 |
| `SPRITE_COLORKEY` | 4 | 透明色模式 |
| `SPRITE_ALPHA` | 8 | Alpha 混合 |
| `COLORKEY_DEFAULT` | `0xFFFF00FF` | 默认透明色（品红） |

### 消息框常量

| 常量 | 值 | 说明 |
|------|-----|------|
| `MESSAGEBOX_OK` | 0 | OK 按钮布局 |
| `MESSAGEBOX_YESNO` | 1 | Yes/No 按钮布局 |
| `MESSAGEBOX_RESULT_OK` | 1 | 点击 OK |
| `MESSAGEBOX_RESULT_YES` | 2 | 点击 Yes |
| `MESSAGEBOX_RESULT_NO` | 3 | 点击 No |
