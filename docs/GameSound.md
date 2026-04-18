# GameSound.h 技术规格文档

## 概述

`GameSound.h` 是一个轻量级多声道音频播放库，为游戏提供低延迟、多通道 WAV 音频播放能力。支持 Windows waveOut 和 SDL2 两种后端，通过软件混音实现每个声道独立音量控制。

## API 接口

```cpp
class GameSound {
public:
    GameSound();
    ~GameSound();

    // 播放 WAV 文件，返回 channel ID
    // 返回值: >0 表示 channel ID，<=0 表示错误码
    //   -1: 文件不存在或加载失败
    //   -2: 音频设备未初始化
    //   -3: 内存分配失败
    int PlayWAV(const char* filename, int repeat = 1, int volume = 1000);

    // 停止指定 channel
    // 返回值: 0 成功，-1 channel 不存在或未播放
    int StopWAV(int channel);

    // 检查 channel 是否正在播放
    // 返回值: 1 正在播放，0 未播放，-1 channel 不存在
    int IsPlaying(int channel);

    // 动态调整音量 (0-1000)
    // 返回值: 0 成功，-1 channel 不存在
    int SetVolume(int channel, int volume);

    // 停止所有 channel
    void StopAll();
};
```

## 架构设计

```
GameSound
├── WAV 缓存层 (WavCache)
│   └── unordered_map<string, WavData*>  // 文件名 → WAV 数据
├── Channel 管理层 (ChannelManager)
│   └── unordered_map<int, Channel*>     // channel ID → 播放状态
└── 音频输出层 (Backend)
    ├── Windows: waveOut API (软件混音)
    └── SDL2: SDL_Audio (软件混音)
```

## 核心数据结构

### WAV 数据缓存

```cpp
struct WavData {
    uint8_t* buffer;          // PCM 数据（跳过 WAV header）
    uint32_t size;            // PCM 数据大小（字节）
    uint32_t sample_rate;     // 采样率（如 44100）
    uint16_t channels;        // 声道数（1=单声道，2=立体声）
    uint16_t bits_per_sample; // 位深（8 或 16）
    int ref_count;            // 引用计数

    ~WavData();               // 释放 buffer
};
```

**缓存策略**：
- 文件名（`const char*`）作为 key，使用 `std::unordered_map<std::string, WavData*>` 存储
- `PlayWAV` 时检查缓存，若已存在则直接复用，`ref_count++`
- Channel 销毁时，对应 WavData 的 `ref_count--`
- `GameSound` 析构时遍历缓存，释放所有 `ref_count == 0` 的 WavData
- **注意**：即使 `ref_count > 0`，析构时也要强制释放，防止内存泄漏

### Channel 状态

```cpp
struct Channel {
    int id;                   // Channel 唯一 ID
    WavData* wav;             // 指向缓存的 WAV 数据
    uint32_t position;        // 当前播放位置（字节偏移）
    int repeat;               // 剩余重复次数（<=0 表示无限循环）
    int volume;               // 音量 (0-1000)
    bool is_playing;          // 是否正在播放

    Channel();
    void Reset();             // 重置状态以便复用
};
```

### Channel ID 分配策略

- 使用单调递增计数器 `int64_t next_channel_id_`，初始值为 1
- 每次分配时，检查当前 ID 是否已存在于 `channels_` 中
- 若存在则递增直到找到未使用的 ID
- 返回时转换为 `int`（实际游戏中不会溢出）
- **目的**：避免短时间内复用刚释放的 ID，防止上层逻辑混淆

```cpp
int AllocateChannel() {
    while (channels_.count((int)next_channel_id_)) {
        next_channel_id_++;
    }
    int id = (int)next_channel_id_;
    next_channel_id_++;
    return id;
}
```

## 音频后端实现

### Windows waveOut 方案

#### 初始化流程

```cpp
// 1. 设置音频格式
WAVEFORMATEX wfx = {0};
wfx.wFormatTag = WAVE_FORMAT_PCM;
wfx.nSamplesPerSec = 44100;
wfx.wBitsPerSample = 16;
wfx.nChannels = 2;  // 立体声输出
wfx.nBlockAlign = (wfx.wBitsPerSample / 8) * wfx.nChannels;
wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

// 2. 打开音频设备（回调模式）
waveOutOpen(&h_wave_out_, WAVE_MAPPER, &wfx, 
            (DWORD_PTR)WaveOutCallback, (DWORD_PTR)this, CALLBACK_FUNCTION);
```

#### 软件混音器设计

使用**双缓冲区 + 回调**模式：

```cpp
// 全局混音缓冲区（int32_t 防止累加溢出）
static const int BUFFER_SAMPLES = 4096;  // 输出样本数（立体声 = 2048 帧）
int32_t mix_buffer_[BUFFER_SAMPLES];     // 混合后的输出缓冲区

// waveOut 双缓冲区
WAVEHDR* wave_hdr_[2];  // 两个缓冲区交替播放

// 回调函数 - 复用已完成的缓冲区
void CALLBACK WaveOutCallback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, 
                               DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg != WOM_DONE) return;

    GameSound* sound = (GameSound*)dwInstance;
    WAVEHDR* hdr = (WAVEHDR*)dwParam1;  // 已播放完的缓冲区

    // 混音
    int16_t output_buffer[BUFFER_SAMPLES];
    sound->MixAudio(output_buffer, BUFFER_SAMPLES);

    // 复用缓冲区
    memcpy(hdr->lpData, output_buffer, BUFFER_SAMPLES * sizeof(int16_t));
    hdr->dwBufferLength = BUFFER_SAMPLES * sizeof(int16_t);
    hdr->dwFlags = 0;
    waveOutPrepareHeader(hwo, hdr, sizeof(WAVEHDR));
    waveOutWrite(hwo, hdr, sizeof(WAVEHDR));
}
```

#### 混音算法

**重要**：立体声 WAV 的样本是交错存储的 `[L0, R0, L1, R1, ...]`，需要按"帧"处理。

```cpp
void MixAudio(int16_t* output_buffer, int sample_count) {
    // 清空混音缓冲区（int32_t 防止溢出）
    memset(mix_buffer_, 0, sample_count * sizeof(int32_t));

    for (auto& pair : channels_) {
        Channel* ch = pair.second;
        if (!ch->is_playing) continue;

        float vol = ch->volume / 1000.0f;
        
        // 计算每帧字节数（所有声道）
        int bytes_per_sample = ch->wav->bits_per_sample / 8;  // 每个声道
        int bytes_per_frame = bytes_per_sample * ch->wav->channels;  // 立体声 = 4
        
        // 计算要混音的帧数（不是样本数）
        int frames_to_mix = sample_count / ch->wav->channels;
        
        // 对每一帧，读取所有声道
        for (int frame = 0; frame < frames_to_mix; frame++) {
            for (int ch_idx = 0; ch_idx < ch->wav->channels; ch_idx++) {
                int16_t sample = ReadSample(ch);
                ch->position += bytes_per_sample;
                
                // 输出索引 = 帧号 × 声道数 + 声道索引
                int out_idx = frame * ch->wav->channels + ch_idx;
                mix_buffer_[out_idx] += (int32_t)(sample * vol);
            }
        }

        // 处理循环/结束逻辑
        if (ch->position >= ch->wav->size) {
            if (ch->repeat == 0) {
                ch->position = 0;  // 无限循环
            } else if (ch->repeat > 1) {
                ch->position = 0;
                ch->repeat--;
            } else {
                ch->is_playing = false;
                ch->wav->ref_count--;
            }
        }
    }

    // 限幅处理（int32_t → int16_t）
    ClampAndConvert(mix_buffer_, output_buffer, sample_count);
}
```

#### 注意事项

1. **线程安全**：waveOut 回调在独立线程执行，访问 `channels_` 需要加锁
   - 使用 `CRITICAL_SECTION` 保护共享数据
   - 游戏主线程调用 `PlayWAV`/`StopWAV` 时需加锁

2. **低延迟优化**：
   - 使用较小的缓冲区（如 4096 样本）
   - **双缓冲区模式**：两个 `WAVEHDR` 同时提交，避免播放间隙
   - 回调复用已完成的缓冲区，无缝衔接
   - 回调函数尽量快，避免阻塞

3. **立体声帧处理**：
   - 立体声 WAV 数据是交错存储的：`[L0, R0, L1, R1, ...]`
   - 混音时必须按"帧"处理，每帧包含所有声道的样本
   - `bytes_per_frame = channels × bits_per_sample / 8`
   - 输出索引：`out_idx = frame × channels + channel_index`

4. **WAV 格式支持与重采样**：
   - 仅支持 PCM 格式（非压缩）
   - 支持 8-bit 和 16-bit 输入
   - 支持单声道和立体声输入
   - **所有 WAV 在加载时统一转换为 44100Hz / 立体声 / 16-bit**
   - 转换算法：线性插值（Linear Interpolation）
   - 单声道 → 立体声：左右声道复制相同数据

### 重采样设计

#### 为什么需要重采样

音频设备固定输出格式（44100Hz / 立体声 / 16-bit），但 WAV 文件可能具有：
- 不同采样率（如 22050Hz、11025Hz）
- 不同声道数（单声道）
- 不同位深（8-bit）

如果不做转换，混音器按设备格式读取会导致：
- 采样率不匹配：音调变高/变低，速度变快/变慢
- 声道数不匹配：左右声道数据错乱
- 位深不匹配：音量异常或噪声

#### 转换策略：加载时统一转换

**优点**：
- 混音器逻辑简化，不需要实时重采样
- 性能好，转换只在加载时做一次
- waveOut 和 SDL2 后端共用同一套转换代码

**缺点**：
- 内存占用增加（22050Hz → 44100Hz 数据量翻倍）
- 单声道 → 立体声数据翻倍

对于游戏音效（通常 < 500KB），内存开销可接受。

#### 线性插值算法

```
原始样本：  s[i]        s[i+1]
            |           |
            |           |
目标位置：  |--- frac --|
            |
         output = s[i] * (1-frac) + s[i+1] * frac
```

**步骤**：
1. 解码原始数据为 16-bit 样本数组
2. 按采样率比率进行线性插值重采样
3. 单声道 → 立体声：左右声道复制相同数据
4. 创建新的 WavData（44100Hz, 2ch, 16-bit）

**核心代码**：
```cpp
// 采样率转换
double ratio = (double)target_rate / src->sample_rate;
double src_index = 0;
while (src_index < total_src_samples - 1) {
    uint32_t i = (uint32_t)src_index;
    double frac = src_index - i;
    
    int16_t s0 = input[i];
    int16_t s1 = input[i + 1];
    output.push_back((int16_t)(s0 * (1-frac) + s1 * frac));
    
    src_index += ratio;
}

// 单声道 → 立体声
for (int i = 0; i < mono_samples; i++) {
    stereo.push_back(mono[i]);  // 左
    stereo.push_back(mono[i]);  // 右
}

// 8-bit → 16-bit
int16_t sample16 = (int16_t)((sample8 - 128) << 8);
```

### SDL2 方案

当 `_WIN32` 未定义或 `USE_SDL != 0` 时启用。

#### 初始化流程

```cpp
SDL_AudioSpec desired = {0};
desired.freq = 44100;
desired.format = AUDIO_S16SYS;
desired.channels = 2;
desired.samples = 4096;
desired.callback = SDLAudioCallback;
desired.userdata = this;

audio_device_ = SDL_OpenAudioDevice(NULL, 0, &desired, NULL, 0);
SDL_PauseAudioDevice(audio_device_, 0);  // 开始播放
```

#### 混音器设计

SDL 的回调模式与 waveOut 类似，混音算法完全相同：

```cpp
void SDLAudioCallback(void* userdata, Uint8* stream, int len) {
    GameSound* sound = (GameSound*)userdata;
    sound->MixAudio();  // 复用相同的混音逻辑
    memcpy(stream, sound->mix_buffer_, len);  // 输出混合后的音频
}
```

## 错误处理

| 错误码 | 含义 | 触发场景 |
|--------|------|---------|
| `> 0` | 成功，返回 channel ID | 正常播放 |
| `-1` | 文件不存在或加载失败 | 文件路径错误、格式不支持 |
| `-2` | 音频设备未初始化 | `GameSound` 构造失败或未调用初始化 |
| `-3` | 内存分配失败 | WAV 文件过大，无法分配缓冲区 |

## 编译说明

### Windows (waveOut)

```bash
g++ -o game.exe game.cpp -mwindows -lwinmm
```

**注意**：必须链接 `-lwinmm` 库以使用 waveOut API。

### SDL2

```bash
g++ -o game.exe game.cpp -mwindows `sdl2-config --cflags --libs`
```

或者手动指定 SDL2 路径：

```bash
g++ -o game.exe game.cpp -mwindows -DUSE_SDL=1 -I/path/to/SDL/include -L/path/to/SDL/lib -lSDL2
```

## 使用示例

```cpp
#include "GameSound.h"

GameSound sound;

// 播放爆炸音效（播放 1 次，音量 80%）
int explosion_ch = sound.PlayWAV("assets/explosion.wav", 1, 800);

// 播放 BGM（无限循环，音量 50%）
int bgm_ch = sound.PlayWAV("assets/bgm.wav", 0, 500);

// 检查爆炸音效是否还在播放
if (sound.IsPlaying(explosion_ch)) {
    // 还在播放...
}

// 动态调整 BGM 音量
sound.SetVolume(bgm_ch, 300);  // 降低音量

// 停止 BGM
sound.StopWAV(bgm_ch);

// 游戏结束时，停止所有音效
sound.StopAll();
// GameSound 析构时自动释放所有缓存的 WAV 数据
```

## 性能考虑

| 项目 | 策略 |
|------|------|
| WAV 缓存 | 同名 WAV 只加载一次，引用计数管理 |
| 混音性能 | O(N) 复杂度，N 为活跃 channel 数 |
| 内存占用 | WAV 数据 + 混音缓冲区（约 32KB） |
| 线程安全 | 回调线程 + 主线程访问需加锁 |
| 低延迟 | 小缓冲区（2048-4096 样本），双缓冲模式 |

## 实现约束

- **C++11 兼容**：使用 `std::unordered_map`、`std::mutex`、`std::string` 等 C++11 特性
- **GCC 4.9.2 兼容**：不使用 C++14/17/20 特性
- **无外部依赖**：Windows 版仅使用 Win32 API，SDL2 版仅使用 SDL2
- **线程安全**：主线程和音频回调线程并发访问需保护

## 文件结构

```
GameSound.h          # 头文件 + 实现（header-only 风格）
docs/GameSound.md    # 本文档
```

## 关键 Bug 修复记录

> **重要**：以下 Bug 已在实现中修复，但在重新生成代码时必须注意，否则会 reintroduce 这些问题。

### Bug 1: WAV 解析时 char 符号扩展导致采样率错误

**症状**：`sample_rate=4294945860`（应为 44100 或 22050）

**根因**：`char` 在 GCC 中默认是 signed，当 `header[26]` 或 `header[27]` 高位为 1 时，左移会导致符号扩展，将正值变成巨大的负数（显示为无符号大数）。

**错误代码**：
```cpp
// 错误！char 符号扩展
wav->sample_rate = (uint32_t)(header[24] | (header[25] << 8) | 
                               (header[26] << 16) | (header[27] << 24));
```

**修复代码**：
```cpp
// 正确！先转换为 uint8_t 再移位
wav->sample_rate = (uint32_t)((uint8_t)header[24] | ((uint8_t)header[25] << 8) | 
                               ((uint8_t)header[26] << 16) | ((uint8_t)header[27] << 24));
```

**影响范围**：所有 WAV 头解析（`LoadWAVFromFile` 中的 `channels`、`sample_rate`、`bits_per_sample` 字段）。

---

### Bug 4: 混音缓冲区溢出导致声音沙哑

**症状**：播放声音时声音沙哑、有爆音或杂音，与外部播放器效果明显不同。

**根因 1**：`mix_buffer_` 定义为 `int16_t`，但多声道混音时累加会溢出。

```cpp
// 错误！int16_t 累加会溢出
int16_t mix_buffer_[BUFFER_SAMPLES];

// 混音时
mix_buffer_[i] += (int16_t)adjusted_sample;  // 两个 20000 相加 = 40000，溢出！
```

**根因 2**：`ReadSample` 中读取 16-bit PCM 数据时也有符号扩展问题。

```cpp
// 错误！char 符号扩展
int16_t sample = (int16_t)(ch->wav->buffer[pos] | (ch->wav->buffer[pos + 1] << 8));
```

**修复代码**：

1. 混音缓冲区改为 int32_t：
```cpp
// 正确！使用 int32_t 防止累加溢出
int32_t mix_buffer_[BUFFER_SAMPLES];
```

2. 混音时不要截断：
```cpp
// 正确！直接累加 int32_t
int32_t adjusted_sample = (int32_t)(sample * vol);
mix_buffer_[i] += adjusted_sample;  // 不要 cast 到 int16_t！
```

3. ReadSample 添加类型转换：
```cpp
// 正确！先转换为 uint8_t
int16_t sample = (int16_t)((uint16_t)(uint8_t)ch->wav->buffer[pos] | 
                            ((uint16_t)(uint8_t)ch->wav->buffer[pos + 1] << 8));
```

4. ConvertToTargetFormat 同样需要修复：
```cpp
// 解码 PCM 数据时也要 cast
decoded[i] = (int16_t)((uint16_t)(uint8_t)src->buffer[i * 2] | 
                        ((uint16_t)(uint8_t)src->buffer[i * 2 + 1] << 8));
```

**影响**：
- 多声道同时播放时声音质量
- 所有 16-bit WAV 文件的播放质量
- 重采样后的音频质量

---

### Bug 5: 立体声样本处理错误导致声音变长和不连续

**症状**：播放声音比外部播放器更长，声音不连续，有"空档"感（几毫秒的断续）。

**根因 1**：混音器没有正确处理立体声帧。

对于立体声 WAV，数据是交错存储的：
```
[L0, R0, L1, R1, L2, R2, ...]  ← 每帧 4 字节（2 声道 × 2 字节）
```

错误代码：
```cpp
// 错误！bytes_per_sample 没考虑声道数
int bytes_per_sample = ch->wav->bits_per_sample / 8;  // 16/8 = 2，应该是 4
int samples_to_mix = sample_count;  // 错误！应该是帧数

// 导致每次只读一个声道，另一个声道被跳过
for (int i = 0; i < samples_to_mix; i++) {
    int16_t sample = ReadSample(ch);  // 只读左声道
    ch->position += bytes_per_sample;  // 只前进 2 字节
}
```

**修复代码**：

```cpp
// 正确！区分字节和帧
int bytes_per_sample = ch->wav->bits_per_sample / 8;  // 每个声道 2 字节
int bytes_per_frame = bytes_per_sample * ch->wav->channels;  // 每帧 4 字节（立体声）

// 计算要混音的帧数（不是样本数）
int frames_to_mix = sample_count / ch->wav->channels;

// 检查剩余帧数
uint32_t remaining_bytes = ch->wav->size - ch->position;
uint32_t remaining_frames = remaining_bytes / bytes_per_frame;

// 对每一帧，读取所有声道
for (int frame = 0; frame < frames_to_mix; frame++) {
    for (uint16_t ch_idx = 0; ch_idx < ch->wav->channels; ch_idx++) {
        int16_t sample = ReadSample(ch);
        ch->position += bytes_per_sample;
        
        // 计算输出索引（考虑声道交错）
        int out_idx = frame * ch->wav->channels + ch_idx;
        mix_buffer_[out_idx] += (int32_t)(sample * vol);
    }
}
```

**根因 2**：`nAvgBytesPerSec` 计算错误。

```cpp
// 错误
wfx_.nAvgBytesPerSec = wfx_.nSamplesPerSec * wfx_.nChannels;  // 44100 * 2 = 88200

// 正确
wfx_.nAvgBytesPerSec = wfx_.nSamplesPerSec * wfx_.nBlockAlign;  // 44100 * 4 = 176400
```

---

### Bug 6: 单缓冲区导致播放不连续

**症状**：声音播放不连续，有几毫秒的"空档"。

**根因**：只使用一个 `WAVEHDR` 缓冲区。当缓冲区播放完后，waveOut 需要等待回调函数准备下一个缓冲区，这中间的间隙就是"空档"。

```cpp
// 错误！单缓冲区
WAVEHDR* wave_hdr_;  // 只有一个

// 初始化时提交一个
waveOutWrite(h_wave_out_, wave_hdr_, sizeof(WAVEHDR));

// 回调中准备下一个 - 但这时有间隙！
void CALLBACK WaveOutCallback(...) {
    // 填充缓冲区...
    waveOutWrite(h_wave_out_, hdr, ...);  // 重新提交
}
```

**修复方案**：使用双缓冲区。

```cpp
// 正确！双缓冲区
WAVEHDR* wave_hdr_[2];

// 初始化时提交两个缓冲区
for (int i = 0; i < 2; i++) {
    wave_hdr_[i] = new WAVEHDR();
    wave_hdr_[i]->lpData = new char[buffer_bytes];
    waveOutPrepareHeader(h_wave_out_, wave_hdr_[i], sizeof(WAVEHDR));
    waveOutWrite(h_wave_out_, wave_hdr_[i], sizeof(WAVEHDR));
}

// 回调中复用已完成的缓冲区
void CALLBACK WaveOutCallback(..., DWORD_PTR dwParam1, ...) {
    WAVEHDR* hdr = (WAVEHDR*)dwParam1;  // 已播放完的缓冲区
    
    // 填充新音频数据
    memcpy(hdr->lpData, output_buffer, buffer_bytes);
    hdr->dwBufferLength = buffer_bytes;
    hdr->dwFlags = 0;
    
    // 重新提交 - 此时另一个缓冲区正在播放，无缝衔接
    waveOutPrepareHeader(h_wave_out_, hdr, sizeof(WAVEHDR));
    waveOutWrite(h_wave_out_, hdr, sizeof(WAVEHDR));
}
```

**关键点**：
- waveOut 的回调参数 `dwParam1` 就是已播放完的 `WAVEHDR` 指针
- 两个缓冲区交替播放，一个播放时另一个在回调中填充
- 不需要跟踪 `current_hdr_`，waveOut 自动管理

**影响**：
- 声音播放的连续性
- 避免几毫秒的播放间隙
- 提供流畅的音频体验

---

### Bug 2: waveOut 回调线程导致析构函数死锁

**症状**：关闭窗口时程序卡死，即使没有播放任何声音。

**根因**：`waveOutReset()` 调用后，Windows 仍会向回调线程发送 `WOM_DONE` 消息。回调函数继续执行 `MixAudio()`，而 `MixAudio()` 会调用 `EnterCriticalSection(&lock_)`。如果析构函数已经执行到 `DeleteCriticalSection(&lock_)`，回调线程还在尝试获取锁，导致死锁。

**时序分析**：
```
主线程:                    回调线程:
---------                  ----------
Destructor started
  waveOutReset()  ──────────────────>  收到 WOM_DONE
                                       MixAudio() 获取锁
  waveOutClose()  (卡住等待)         锁未释放...
  DeleteCriticalSection() ────────>  EnterCriticalSection() 死锁！
```

**修复方案**：

1. **添加 `closing_` 标志**：
```cpp
volatile bool closing_;  // 信号：回调函数应停止处理
```

2. **回调函数检查标志**：
```cpp
void CALLBACK WaveOutCallback(...) {
    GameSound* sound = (GameSound*)dwInstance;
    
    // 检查关闭标志，提前退出
    if (sound->closing_) {
        return;
    }
    
    // ... 正常混音和提交 ...
    
    // MixAudio 之后再次检查
    if (sound->closing_) {
        return;
    }
}
```

3. **ShutdownAudioBackend 设置标志**：
```cpp
void ShutdownAudioBackend() {
    if (h_wave_out_) {
        // 在 waveOutReset 之前设置标志
        closing_ = true;
        
        waveOutReset(h_wave_out_);
        Sleep(50);  // 给回调线程时间退出
        waveOutClose(h_wave_out_);
    }
}
```

4. **构造函数初始化**：
```cpp
GameSound::GameSound() 
    : closing_(false)  // 必须初始化
{
}
```

**关键点**：
- `closing_` 必须是 `volatile`，因为跨线程访问
- 必须在 `waveOutReset()` **之前**设置标志
- `Sleep(50)` 不是优雅的方案，但在 Windows waveOut API 下是必要的
- 回调函数中至少检查两次标志（进入时和 `MixAudio` 后）

---

### Bug 3: 内联函数无法访问私有嵌套类型

**症状**：编译错误 `'WavData' does not name a type`

**根因**：当内联函数在类外定义时，如果函数体使用了类的私有嵌套类型（如 `WavData*`、`Channel*`），编译器可能无法正确解析类型。

**错误代码**：
```cpp
class GameSound {
private:
    struct WavData { ... };
    WavData* LoadWAVFromFile(const char* filename);
};

// 错误！在类外定义，无法访问私有嵌套类型
inline GameSound::WavData* GameSound::LoadWAVFromFile(const char* filename) {
    ...
}
```

**修复方案**：将所有实现放在类内部，或使用 `#ifdef GAMESOUND_IMPLEMENTATION` 在类内部包含实现。

**正确代码**：
```cpp
class GameSound {
public:
    ...
#ifdef GAMESOUND_IMPLEMENTATION
private:
    struct WavData { ... };
    
    WavData* LoadWAVFromFile(const char* filename) {
        // 实现在类内部
        ...
    }
#endif
};
```

---

## 调试宏定义

```cpp
#ifndef GAMESOUND_DEBUG
#define GAMESOUND_DEBUG 0  // 默认关闭
#endif

#define GS_DEBUG_PRINT(fmt, ...) do { \
    if (GAMESOUND_DEBUG) printf("[GameSound] " fmt "\n", ##__VA_ARGS__); \
} while(0)
```

**使用方式**：在包含 `GameSound.h` 之前定义为 1 以启用调试输出：
```cpp
#define GAMESOUND_DEBUG 1
#define GAMESOUND_IMPLEMENTATION
#include "GameSound.h"
```
