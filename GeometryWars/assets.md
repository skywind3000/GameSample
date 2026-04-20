# Geometry Wars - 资源列表

本游戏使用的所有音效资源。

## 音频资源

所有音效文件位于 `assets/` 目录下，格式为 WAV。

### 实际存在的音效文件

| 文件名 | 类型 | 用途 |
|--------|------|------|
| `explosion-01.wav` ~ `explosion-08.wav` | 爆炸音效 | 敌人爆炸、玩家死亡等爆炸效果（8个变体） |
| `shoot-01.wav` ~ `shoot-04.wav` | 射击音效 | 玩家武器射击效果（4个变体） |
| `spawn-01.wav` ~ `spawn-08.wav` | 生成音效 | 敌人出现/生成提示音（8个变体） |
| `powerup-01.wav` | 道具音效 | 拾取道具/升级效果 |
| `rise-01.wav` ~ `rise-07.wav` | 升级音效 | 等级提升/状态增强提示音（7个变体） |

### 代码中引用的音效文件

以下音效在 `geometry.cpp` 中被引用，预期位于 `assets/sound/` 子目录中：

| 变量名 | 文件名 | 用途 | 触发时机 |
|--------|--------|------|---------|
| `sounds.click` | `click.wav` | 射击音效 | 玩家射击时（代码中已定义但未实际调用） |
| `sounds.hit` | `hit.wav` | 击中音效 | 子弹命中敌人时 |
| `sounds.coin` | `coin.wav` | 击杀音效 | 敌人被消灭时 |
| `sounds.explosion` | `explosion.wav` | 死亡音效 | 玩家死亡时（SCENE_DEATH） |
| `sounds.gameOver` | `game_over.wav` | 游戏结束音效 | 进入结算画面时（SCENE_GAME_OVER） |
| `sounds.noteHigh` | `note_do_high.wav` | 开始游戏提示音 | 进入战斗场景时（SCENE_COMBAT） |
| `sounds.victory` | `victory.wav` | 胜利音效 | （代码中已定义但未实际调用） |

### 音效映射关系

实际存在的音效文件可以通过随机选择变体的方式，为代码中引用的音效提供丰富的变化：

| 代码引用 | 可用变体 | 数量 |
|---------|---------|------|
| 爆炸类音效 | `explosion-01.wav` ~ `explosion-08.wav` | 8个 |
| 射击类音效 | `shoot-01.wav` ~ `shoot-04.wav` | 4个 |
| 生成类音效 | `spawn-01.wav` ~ `spawn-08.wav` | 8个 |
| 道具类音效 | `powerup-01.wav` | 1个 |
| 升级类音效 | `rise-01.wav` ~ `rise-07.wav` | 7个 |

## 资源总计

| 类型 | 数量 |
|------|------|
| 爆炸音效 | 8 个 |
| 射击音效 | 4 个 |
| 生成音效 | 8 个 |
| 道具音效 | 1 个 |
| 升级音效 | 7 个 |
| **总计** | **28 个 WAV 文件** |
