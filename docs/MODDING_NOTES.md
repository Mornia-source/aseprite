# Aseprite 私有分支改造记录（MODDING NOTES）

> **本文件是"实时台账"**。每次改动官方文件，必须在 §2 表格里加一行。
> upstream 更新后先读 §2 → 逐条核对 → 再 rebase。

- 分支：`mods/main`（工作分支），`main`（跟随 upstream，只做 fast-forward）
- upstream：`https://github.com/aseprite/aseprite.git`
- 最后同步的 upstream commit：`375989a61`（Fix crash using Export Sprite Sheet from a sprite w/selection）
- 最后同步日期：2026-09-10（合并 38 个提交，**零冲突**，见 §18）
- 最后更新本文档：2026-07-27

---

## 1. 目标功能

| # | 功能 | 状态 |
|---|---|---|
| F1 | PSD 原生支持（导入修复 + **导出实现**） | 进行中 |
| F2 | 时间轴每个图层行前显示该图层缩略图 | 进行中 |
| F3 | Ctrl+单击缩略图 → 选中该图层内容边界为选区 | 进行中 |

**F1 帧策略（已定）**：只导出**当前帧**。Aseprite 图层结构 → PSD 图层/图层组一一映射，不做"帧转图层组"。

---

## 2. 需要改动的官方文件（接缝台账）★核心★

改动原则：官方文件里只留 **`#ifdef ENABLE_MODS` 包裹的最小调用**，逻辑全在 `src/app/mods/` 里。

| # | 官方文件 | 改动内容 | 冲突风险 | 状态 |
|---|---|---|---|---|
| S1 | [CMakeLists.txt](../CMakeLists.txt) `:95` | `ENABLE_PSD` 默认 `off`→`on`；新增 `ENABLE_MODS` option | 低 | ✅ |
| S2 | [src/app/CMakeLists.txt](../src/app/CMakeLists.txt) `:158` | 加入 `mods/` 源文件 + `-DENABLE_MODS` | 低 | ✅ |
| S3 | [src/app/file/psd_format.cpp](../src/app/file/psd_format.cpp) `:65,:439` | flags 加 `FILE_SUPPORT_SAVE\|LAYERS\|RGB\|RGBA`；`onSave` 转调 `mods::psd_encode()` | 低 | ☐ |
| S4 | [src/app/ui/timeline/timeline.cpp](../src/app/ui/timeline/timeline.cpp) | **重灾区**，见 §3 逐点清单 | **高** | ✅ |
| S5 | [src/app/commands/commands_list.h](../src/app/commands/commands_list.h) | 注册 `SelectLayerBounds` 命令（可选，供 Lua 调用） | 低 | ☐ |
| S6 | [data/gui.xml](../data/gui.xml) | 菜单项 + 快捷键（可选） | 中 | ☐ |
| S7 | [data/strings/en.ini](../data/strings/en.ini) | 新增字符串 key | 低 | ☐ |
| S8 | [src/CMakeLists.txt](../src/CMakeLists.txt) `:155` | 把 `README.md`/`AUTHORS.md`/`EULA.txt`/`docs/LICENSES.md` 的复制改成 `if(EXISTS)` 可选（本仓库删掉了其中几个，原代码会 configure 报错） | 中 | ✅ |
| S9 | [src/app/file/psd_format.cpp](../src/app/file/psd_format.cpp) | PSD bug 修复 B/C/D，见 §12 | 中 | ✅ |
| **S10** | **[src/psd/decoder.cpp](../src/psd/decoder.cpp)** | PSD bug 修复 A，见 §12。submodule 已 fork 至 `Mornia-source/psd`，见 §28 | 中 | ✅ |
| S11 | [theme.xml](../data/extensions/aseprite-theme/theme.xml) + [dark/theme.xml](../data/extensions/aseprite-theme/dark/theme.xml) | `<fonts>` 块换成 Zfull-GB 像素字体（照搬官方发行版），见 §13 | 中 | ✅ |
| S12 | [timeline.cpp](../src/app/ui/timeline/timeline.cpp) | 表头缩略图开关按钮 `PART_HEADER_THUMBNAILS`，见 §3 的 P9~P12 | 高 | ✅ |
| S13 | [data/strings/](../data/strings/) | 提交官方发布的 23 种语言（`ENABLE_I18N_STRINGS` 保持官方默认 off），见 §15 | 低 | ✅ |
| **S14** | [src/app/CMakeLists.txt](../src/app/CMakeLists.txt) | 给 `dio-lib` 补 `-DENABLE_PSD`，修上游缺陷，见 §18.3 | 中 | ✅ |
| S15 | [src/app/ui/context_bar.cpp](../src/app/ui/context_bar.cpp) | 画笔/轮廓等工具的不透明度（3 处接缝），见 §19 | 中 | ✅ |
| S16 | [color_bar.h](../src/app/ui/color_bar.h) + [color_bar.cpp](../src/app/ui/color_bar.cpp) | 把调色板色条插到调色板上方，见 §20 | 中 | ✅ |
| S17 | [commands_list.h](../src/app/commands/commands_list.h) | 注册 `ShowPaletteBars` 命令 | 低 | ✅ |
| S18 | [data/gui.xml](../data/gui.xml) + [en.ini](../data/strings/en.ini) + [zh_Hans.ini](../data/strings/zh_Hans.ini) | 视图菜单项 + 字符串 | 中 | ✅ |
| S19 | [skin_theme.cpp](../src/app/ui/skin/skin_theme.cpp) | 萨卡兹语的字体替换（2 处接缝），见 §22 | 中 | ✅ |
| S20 | [main_window.cpp](../src/app/ui/main_window.cpp) | 语言变化时重载主题（字体热切换），见 §23 | 中 | ✅ |
| S21 | [xml_translator.h/.cpp](../src/app/i18n/xml_translator.cpp) + [widget_loader.cpp](../src/app/widget_loader.cpp) + main_window.cpp | **界面文本实时重译**，见 §23 | 高 | ✅ |
| S22 | [CMakeLists.txt](../CMakeLists.txt) + [src/ver/CMakeLists.txt](../src/ver/CMakeLists.txt) + [src/ver/info.c](../src/ver/info.c) | `MODS_UPDATE_URL`：更新检查指向自建服务器，见 §24 | 低 | ✅ |
| S23 | [data/gui.xml](../data/gui.xml) | 帮助菜单只保留「关于」，见 §26 | 中 | ✅ |
| S24 | [about.xml](../data/widgets/about.xml) + en.ini + zh_Hans.ini | 「关于」增加本构建的说明与声明，见 §26 | 中 | ✅ |
| S25 | [CMakeLists.txt](../CMakeLists.txt) + [src/app/CMakeLists.txt](../src/app/CMakeLists.txt) + [src/app/app.cpp](../src/app/app.cpp) | `MODS_TITLE_SUFFIX`：标题栏后缀，见 §26.3 | 低 | ✅ |

### 我们自己的文件（无冲突风险，续）

| 文件 | 作用 |
|---|---|
| [run.cmd](../run.cmd) | **一键 configure+编译+启动**，见 §7.5 |
| [package.cmd](../package.cmd) | **打包成可移植 zip**，见 §21 |
| [data/mods/icons/layer_thumbnails.png](../data/mods/icons/layer_thumbnails.png) | 缩略图开关图标（24×12，两帧，黑色蒙版） |

### 我们自己的文件（无冲突风险）

| 文件 | 作用 |
|---|---|
| [src/app/mods/ui/layer_thumbnail.h](../src/app/mods/ui/layer_thumbnail.h) | F2 接口 |
| [src/app/mods/ui/layer_thumbnail.cpp](../src/app/mods/ui/layer_thumbnail.cpp) | F2 实现 + LRU 缓存 |

---

## 3. timeline.cpp 接缝逐点清单（S4 展开）

新增 `PART_ROW_THUMBNAIL`，插入在 **eye 图标之前**（即最左列）。
以下行号基于 upstream `35c35e645`，rebase 后需重新定位。

**统一手法**：在 include 区定义宏，官方代码里只出现 `MODS_THUMB_W()` 一个 token：

```cpp
#ifdef ENABLE_MODS
  #include "app/mods/ui/layer_thumbnail.h"
  #define MODS_THUMB_W() (mods::layer_thumbnail_width(headerBoxWidth()))
#else
  #define MODS_THUMB_W() 0
#endif
```
关掉 `ENABLE_MODS` 时展开为 `0`，列布局与官方**逐像素一致**。

| 点 | 位置 | 要做什么 | 状态 |
|---|---|---|---|
| P0 | include 区（`app/thumbnails.h` 之后） | 上面的宏定义 | ✅ |
| P1 | `enum` 中 `PART_ROW` 之后 | 加 `PART_ROW_THUMBNAIL` | ✅ |
| P2a | `getPartBounds()` `PART_HEADER_EYE/PADLOCK/CONTINUOUS/GEAR/ONIONSKIN/LAYER` | x 全部 `+ MODS_THUMB_W()`（**否则表头开关与图层列错位**） | ✅ |
| P2b | `getPartBounds()` `PART_ROW_*` | 新增 `case PART_ROW_THUMBNAIL`（x=`bounds.x`, w=`MODS_THUMB_W()`）；EYE/PADLOCK/CONTINUOUS/TEXT 的 x 全部 `+ MODS_THUMB_W()` | ✅ |
| P3 | `hitTest()` | 在 EYE 判断**之前**加 THUMBNAIL 命中判断 | ✅ |
| P4 | `drawLayer()` 画 eye 之前 | `drawPart(timelineLayer())` 画底 + `mods::draw_layer_thumbnail(...)` | ✅ |
| P5 | `onMouseDown` / `kMouseUp` 右键菜单 / `kDoubleClick` | `case PART_ROW_THUMBNAIL:` fallthrough 到 `PART_ROW_TEXT`（→ 白送 Ctrl+单击选边界、右键图层菜单、双击图层属性） | ✅ |
| P6 | `onDrag()` switch | 加入 fallthrough 列表 | ✅ |
| P7 | `updateStatusBar()` | 加提示文本（说明 Ctrl/Shift/Alt 组合） | ✅ |
| P8 | `setFrame()` 内 `invalidateFrame` 之后 | **换帧要让缩略图列失效**（`invalidateFrame` 只覆盖 cel 列） | ✅ |
| P9 | `enum` 中 `PART_HEADER_GEAR` 之前 | 加 `PART_HEADER_THUMBNAILS`（开关按钮） | ✅ |
| P10 | `getPartBounds()` 表头 | 新增 `case PART_HEADER_THUMBNAILS`（x=`3*hbw`, w=`MODS_HDR_W()`） | ✅ |
| P11 | `hitTest()` 表头链 | 在 GEAR **之前**判断（gear 现在右移了一格） | ✅ |
| P12 | `drawHeader()` / `onMouseDown` / `updateStatusBar()` | 绘制按钮 + 点击切换（`layout()`+`invalidate()`）+ 状态栏提示 | ✅ |

**两个宽度宏的区别（重要）**：
```cpp
#define MODS_THUMB_W() (mods::layer_thumbnail_width(headerBoxWidth()))  // 关闭时 = 0，列收起
#define MODS_HDR_W()   (headerBoxWidth())                              // 恒定，否则关掉后按钮消失、无法再开
```
表头 gear/onionskin/layer 用 `MODS_HDR_W()`；图层行的 text 用 `MODS_THUMB_W()`。
关闭时表头会比图层行多一格，视觉上有轻微错位，但保证按钮永远可点。

> ⚠️ **P2a/P2b 是最容易被 upstream 打断的点**。upstream 若调整图层面板列布局，先看这里。
> ⚠️ `invalidateLayer()` 用的是 `PART_ROW`（整行宽 `separatorX()`），已覆盖缩略图列，**无需改**。

### 已知小瑕疵（暂不修）
- 图层面板默认宽度（pref `general.timelineLayerPanelWidth`）没为新列加宽，
  首次启用时图层名会被挤窄一格。拖一下分隔条即可，且会持久化。
  不改的原因：改 pref 默认值只影响全新安装，加"自动加宽"逻辑反而是个易错的有状态接缝。

---

## 4. 关键发现（省了大量工作）

### 4.1 PSD 支持已存在一半 —— 但是一个 2021 年停摆的贡献
- 解析库内置：[src/psd/](../src/psd/)（decoder.cpp / image_resources.cpp），是**独立 submodule**（aseprite/psd 仓库），**不是**第三方依赖
- 导入已实现：[psd_format.cpp:402](../src/app/file/psd_format.cpp:402) `onLoad`，含混合模式映射表
- 导出**完全没写**：[psd_format.cpp:439](../src/app/file/psd_format.cpp:439) 直接 `return false`
- 官方默认关闭：`option(ENABLE_PSD "Enable experimental support for .psd files" off)`

**为什么官方不启用** —— `git log -- src/psd` 说得很清楚：

```
2021-10-18  9a12eb584  Initial to decode PSD files
2021-10-20  7745921a2  Optimize image transparency detection
2021-10-21  6e00c74f5  Add image processing scanline by scanline
2021-11-03  174ec2863  Add support for frame-by-frame animation
2021-12-15  72cf9c12f  Merge branch 'beta-psd' into beta
2025-04-17  d9a138357  Update modules        ← 唯一后续，纯机械维护
```
两个月冲到能读简单 PSD 就停了，之后四年只有 clang-format 和 submodule 版本号更新。
半吊子的导入比没有导入更糟（静默出错 = 支持地狱），所以锁在开关后面标 experimental。

### 4.1.1 ★ZIP 压缩是 F1 导入侧的硬门槛★

| 缺口 | 证据 | 后果 |
|---|---|---|
| **ZIP 压缩完全没实现** | [decoder.cpp:1175](../src/psd/decoder.cpp:1175) `case ZIPWithoutPrediction: // TODO break;`（1179 同）| ⚠️ **致命**。Photoshop 对 16/32bit 必用 ZIP，8bit 也常用。命中就 `break` 走掉 —— **静默产出空图层，不报错** |
| CMYK/Lab/Duotone/Multichannel | [decoder.cpp:50-57](../src/psd/decoder.cpp:50) 头部照单全收，但无色彩转换路径 | 颜色错乱 |
| 附加图层信息丢弃 | [decoder.cpp:416](../src/psd/decoder.cpp:416) `// TODO: what to do with the data read in this segment?`（444/445/483/693 同） | 调整图层 / 图层蒙版 / 智能对象 解析完就丢 |
| 自定义通道数 | [decoder.cpp:738](../src/psd/decoder.cpp:738) `throw "Invalid number of channels" // TODO` | 直接抛异常 |

**好消息**：`third_party/zlib` 已在构建里（`ZLIB::ZLIB` → `zlibstatic` 别名，
[third_party/CMakeLists.txt:17](../third_party/CMakeLists.txt:17)），接 inflate 即可，**不用新增依赖**。

### 4.2 F3 的核心逻辑 upstream 已有！
[src/app/util/layer_boundaries.h](../src/app/util/layer_boundaries.h)：
```cpp
enum SelectLayerBoundariesOp { REPLACE, ADD, SUBTRACT, INTERSECT };
void select_layer_boundaries(doc::Layer*, doc::frame_t, SelectLayerBoundariesOp);
```
且 **timeline.cpp:779 已经把 Ctrl+单击图层名 绑定到它**（`is_select_layer_in_canvas_key_pressed`，
macOS 为 Cmd，其他平台 Ctrl；修饰键组合：Alt+Shift=INTERSECT，见 timeline.cpp:166）。

**结论：F3 = 把已有行为接到新的 thumbnail part 上，几行代码。**

### 4.3 缩略图基础设施已有
[src/app/thumbnails.h](../src/app/thumbnails.h)：`thumb::get_cel_thumbnail(display, cel, scaleUpToFit, fitInSize)`
- 已用于 cel 格子内缩略图（timeline.cpp:2593，受 `docPref().thumbnails.enabled()` 控制）
- **无缓存**，每次调用都 `render::Render` 重渲染 → 图层列每帧重绘会卡
- ⇒ 我们在 `mods/` 里加一层 LRU 缓存，key = `(cel->id(), cel->version(), cel->image()->version(), size)`
  （`doc::Object::version()` 见 [src/doc/object.h:27](../src/doc/object.h:27)）

### 4.4 选区 API
`doc::Mask::fromImage(image, origin, alphaThreshold)` — [src/doc/mask.h:78](../src/doc/mask.h:78)
参考用法见 [cmd_mask_content.cpp](../src/app/commands/cmd_mask_content.cpp)

---

## 5. 外部参考资料

### 5.1 PSD 导出参考实现（★F1 的规格书★）
`C:\Users\Cherry\AppData\Roaming\Aseprite\scripts\Export as psd.lua`

- **不是官方脚本**。第三方 MIT 项目：
  - 作者：Mooncake Sugar (Tsukina-7mochi)，v1.3.2
  - 主页：https://github.com/Tsukina-7mochi/aseprite-scripts/blob/master/psd/
  - License：MIT ⇒ 可移植，**须保留署名**
- 这是一份**已验证可用**的 PSD 写入实现，直接当规格书移植成 C++。已踩过的坑：

| 要点 | 脚本位置 | 说明 |
|---|---|---|
| PackBits/RLE 压缩 | L64-141 | 每行独立压缩，奇数长度补 `\x80` |
| 文件头 | L278-287 | `8BPS`, ver=1, 6字节reserved=0, channels=4, H, W, depth=8, colorMode=3 |
| 通道顺序与 ID | L501-504 | R=0, G=1, B=2, **A=0xFFFF** |
| 图层组闭合记录 | L415-447 | 名字为 `</Layer {name} >`，`lsct` data=3（bounding divider） |
| 图层组起始记录 | L543-568 | `lsct` data=1(展开)/2(折叠) |
| 空图层 | L464-486 | bounds 全 0，channel 数据 `\x00`×8，size 全 2 |
| 图层名 | L167-175 | Pascal string，pad 到 4 字节倍数，maxLen 128 |
| 不透明度合成 | L492 | `floor(layer.opacity/255 * cel.opacity)` |
| 混合模式表 | L201-221 | Aseprite BlendMode → PSD 4字符码（**导出方向，与 psd_format.cpp 的表互为反向**）|
| 合成图像段 | L620-712 | 删掉隐藏图层 → flatten → 全画布 RLE（画布外补 0）|
| Layer&Mask 段长度 | L599-615 | 奇数需补 1 字节 |

### 5.2 PSD 导入参考
`C:\Users\Cherry\AppData\Roaming\Aseprite\scripts\import from psd.lua`
- 配套脚本（非官方，基于上面那个写的），只支持 depth=8 / colorMode=3
- 有用的点：`lsct` 解析（L425-449）、UTF-16BE→UTF-8 图层名转换（L16-43）、`unpackBits`（L191）
- 优先级低 —— 我们的 C++ decoder 已比它完整

---

## 6. 插件（Lua）兼容性守则

硬性要求：**不能破坏原有插件支持**。

- ✅ 只**新增** Lua API/命令，绝不修改已有签名
- ✅ 新命令走标准 `Command` 注册，插件可 `app.command.SelectLayerBounds()`
- ⚠️ 新增快捷键前检查是否与常用插件冲突
- ⚠️ 改 `data/gui.xml` 时只加节点，不动已有 id
- 🔁 每次改完跑 `tests/` 回归

---

## 7. 构建（本机已验证的完整配方）

### 7.1 本机环境（2026-07-27 实测）

| 项 | 值 |
|---|---|
| MSVC | 14.44.35207，**VS2022 Build Tools**（非 Community）<br>`C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools` |
| Windows SDK | 10.0.26100.0 |
| cmake | `C:\Program Files\CMake\bin\cmake.exe` |
| ninja | `C:\Users\Cherry\AppData\Local\Programs\Python\Python310\Scripts\ninja.exe` |
| Skia | `m124-08a5439a6b`（版本号取自 [laf/misc/skia-tag.txt](../laf/misc/skia-tag.txt)） |
| Skia 位置 | `C:\Users\Cherry\deps\skia` |
| submodules | 已全部 checkout，**无需** `git submodule update --init` |

### 7.2 Skia 依赖

预编译包，**不需要** depot_tools 从源码编译。仅 27.4 MB：
```
https://github.com/aseprite/skia/releases/download/m124-08a5439a6b/Skia-Windows-Release-x64.zip
```
URL 生成规则见 [laf/misc/skia-url.sh](../laf/misc/skia-url.sh)（版本号从 `skia-tag.txt` 读）。
GitHub **直连可用**，本机代理（混合端口 10808）备用但实测未用上。
解压后须有 `out/Release-x64/skia.lib`（约 29 MB）。

### 7.3 配置 + 编译

必须先起 MSVC 环境，所以落盘成两个脚本（都在仓库根，已加进 .gitignore 待办）：

`_configure.cmd`：
```bat
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "C:\Users\Cherry\Documents\My Games\Terraria\tModLoader\ModSources\aseprite"
cmake -B build -G Ninja ^
  -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
  -DLAF_BACKEND=skia ^
  -DSKIA_DIR=C:/Users/Cherry/deps/skia ^
  -DSKIA_LIBRARY_DIR=C:/Users/Cherry/deps/skia/out/Release-x64 ^
  -DSKIA_LIBRARY=C:/Users/Cherry/deps/skia/out/Release-x64/skia.lib ^
  -DENABLE_PSD=ON ^
  -DENABLE_MODS=ON
```

`_build.cmd`：
```bat
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "C:\Users\Cherry\Documents\My Games\Terraria\tModLoader\ModSources\aseprite"
ninja -C build aseprite
```

### 7.5 一键脚本 [run.cmd](../run.cmd) ★日常就用这个★

```
run.cmd                 configure(按需) + 编译 + 启动
run.cmd <文件>          同上，并打开指定文件
run.cmd --build         只编译，不启动
run.cmd --reconfigure   强制重新 configure（**新增 data/ 文件后必须用**）
run.cmd --clean         删掉 build\ 从头来
```

脚本内置的几个要点（都是踩过坑才加的）：
- 自动加载 vcvars64（已有 `cl.exe` 则跳过）
- **编译前自动关闭正在运行的 aseprite.exe** —— 否则链接器报
  `LNK1104: 无法打开文件 bin\aseprite.exe`
- Skia / vcvars 缺失时给出明确错误而不是一堆看不懂的输出
- 用 `set "BUILDRC=%ERRORLEVEL%"` 显式取 ninja 退出码
  （`if errorlevel 1` 在此处不可靠，实测编译失败仍会走到成功分支）
- ⚠️ **批处理陷阱**：`echo %VCVARS%` 在 `if (...)` 块内会因路径含 `(x86)` 的
  右括号提前闭合代码块，报 `\Microsoft was unexpected at this time.`
  → 必须用延迟展开 `!VCVARS!`

### 7.4 踩过的坑

- **首次 configure 要 ~707 秒**（大量 libarchive/curl 的 try_compile 探测）。
  之后有缓存，重跑 configure+generate 只要 ~13 秒。
- **generate 阶段会因缺文件失败**：`EULA.txt` / `README.md` / `docs/LICENSES.md`
  在本工作区被删了，而 [src/CMakeLists.txt](../src/CMakeLists.txt) 硬引用它们。
  已用接缝 S8（`if(EXISTS)` 守卫）解决。`INSTALL.md` **不**影响构建。

---

## 8. upstream 同步流程

```
git fetch upstream
git checkout main && git merge --ff-only upstream/main
git checkout mods/main && git rebase main
```
冲突时按 §2/§3 台账逐条核对。重点看 P2。

---

## 9. 变更日志

| 日期 | 内容 |
|---|---|
| 2026-07-27 | 建立本文档；完成现状盘点（§4）；确定 F1 帧策略=当前帧 |
| 2026-07-27 | 完成 S1/S2 骨架（`ENABLE_MODS` 开关 + `src/app/mods/`） |
| 2026-07-27 | 完成 **F2 + F3**：S4 全部 9 个接缝点 + `mods/ui/layer_thumbnail.*` |
| 2026-07-27 | 加接缝 S8（CMake 可选 data 文件）；`.gitignore` 加 `_configure.cmd`/`_build.cmd` |
| 2026-07-27 | ✅ **编译通过**（1566/1566，0 error），`aseprite.exe` 21.7 MB |
| 2026-07-27 | ✅ **F2/F3 GUI 实测全部通过**，见 §11 |
| 2026-07-27 | 缩略图列位置改为「链接列之后、图层名之前」（原在最左）。表头 eye/padlock/continuous 恢复官方位置，只有 gear/onionskin/layer 右移 → **接缝更小** |
| 2026-07-27 | ✅ **修复 PSD 打开闪退 + 中文图层名乱码**（同一根因），见 §12 |
| 2026-07-27 | ✅ **中文像素字体**：照搬官方发行版的 Zfull-GB.ttf 方案（接缝 S11），见 §13 |
| 2026-07-27 | ✅ **表头缩略图开关按钮**（接缝 S12，P9~P12）。图标走运行时着色的自有 PNG，不动 sheet.png / theme.xml 的图标部分 |
| 2026-07-27 | ✅ **字体回退链**（S11 续，§13.5）；实测中/俄/韩/阿 4 种文字全部像素渲染、同基线 |
| 2026-07-27 | ✅ **多语言支持**（S13）：`ENABLE_I18N_STRINGS=ON`，得到 44 种语言，见 §15 |
| 2026-07-27 | ✅ **PSD 图层可见性**修复，见 §12.7 |
| 2026-07-27 | ✅ **一键脚本 run.cmd**，见 §7.5 |
| 2026-07-27 | ✅ **代码冗余审查**，见 §16 |
| 2026-07-27 | ❌ 确认 **PSD 图层蒙版尚未支持**，方案已记录，见 §12.8 |

---

## 12. PSD 打开闪退 + 中文乱码：根因与修复（2026-07-27）

### 12.1 复现样本
`C:\Users\Cherry\Downloads\信仰搅拌机-泰拉小人.psd`（89,994 字节）
- 8bit / RGB / 268×80 / 22 图层 / 无图层组
- 压缩：Raw 54 通道 + RLE 34 通道（**不含 ZIP**，所以与 §4.1.1 无关）
- layer count 字段为 **负数 -22**（PSD 规范：负数表示首个 alpha 通道存合并结果透明度）
  → decoder 已正确处理（[decoder.cpp:763](../src/psd/decoder.cpp:763)），**不是** bug
- 22 个图层全部带 `luni`（Unicode 名）附加信息块

### 12.2 定位过程（可复用的方法）
| 步骤 | 结论 |
|---|---|
| `-b file.psd --save-as out.png` | ✅ 成功 → **解码器没问题，是 GUI 路径** |
| `--verbose` 日志 | 崩在 `APP: Finish launching...` 之后 → UI 构建/首绘阶段 |
| 退出码 `0xC000001D` | STATUS_ILLEGAL_INSTRUCTION = MSVC `std::terminate` 的 `ud2` → **未捕获异常** |
| A/B 测试：`[Mods] LayerThumbnails=0` | ❌ 仍崩 → **与我们的缩略图代码无关** |
| 清 doc-pref ini / 换文件名 | ❌ 仍崩 → 与配置无关 |
| **把图层名原地改成 ASCII（长度不变）** | ✅ **不崩** → **根因 = 图层名编码** |

> 💡 **方法论**：批处理 vs GUI 的差异是绝佳的二分线 —— 批处理不渲染文字。
> 「原地改字节、保持长度」是定位编码类崩溃的高效手段。

### 12.3 根因
PSD 把图层名存**两份**：
1. 传统 Pascal 字符串 —— **系统代码页**（本例 GBK），无法可靠推断
2. `luni` 附加块 —— UTF-16BE 真正的 Unicode 名

upstream 在 [decoder.cpp:592](../src/psd/decoder.cpp:592) **解析了 `luni` 但只打了个 TRACE 就丢掉**，
于是图层名用了 GBK 字节。后果**两个**：
- 显示成乱码（`ͼ�� 3` 而不是 `图层 3`）
- GBK 字节**不是合法 UTF-8**，喂给 Skia/HarfBuzz 文字排版 → 抛异常 → 未捕获 → **闪退**

### 12.4 四处修复
| # | 文件 | 修复 |
|---|---|---|
| **A** | [src/psd/decoder.cpp](../src/psd/decoder.cpp) | 新增自包含 `mods_wide_to_utf8()`（含 UTF-16 代理对处理），把 `luni` 结果赋给 `layerRecord.name`。⚠️ submodule |
| **B** | [psd_format.cpp](../src/app/file/psd_format.cpp) | 新增 `psd_sanitize_utf8()`，非法 UTF-8 字节替换为 `?`。**这是止住闪退的兜底**，覆盖无 `luni` 的老 PSD（Photoshop 6 之前） |
| **C** | [psd_format.cpp](../src/app/file/psd_format.cpp) | `ImageSpec(..., header.width, header.width)` → `header.height`。upstream 笔误，非方形 PSD 画布高度全错（本例 268×80 被读成 268×268） |
| **D** | [psd_format.cpp](../src/app/file/psd_format.cpp) | 重名图层分支 `cel(frame_t(0))->imageRef()` 未查空指针，改为 `else if (Cel* cel = ...)` |

### 12.5 验证结果
| 项 | 修复前 | 修复后 |
|---|---|---|
| GUI 打开 | 闪退 `0xC000001D` | ✅ 正常 |
| 画布尺寸 | 268×268 | ✅ 268×80 |
| 图层名 | `ͼ�� 3` | ✅ `图层 3`，字节核验 `E5 9B BE E5 B1 82 ...` 逐字正确 |

> ⚠️ 探测脚本用 Lua `%q` 打印会把 `0x9D` 之类字节转义成 `\157`，看起来像乱码但其实是**打印假象**。
> 核验编码要 dump 十六进制。

### 12.6 顺带发现（未修）
- 打开 Lua 脚本导出的 PSD 时，时间轴末尾多出 `Layer 1` / `Group` / `Frame 1` 三个虚假图层
  —— decoder 没正确跳过图层组的 `lsct` 闭合记录

### 12.7 图层可见性丢失（已修）

**问题**：PSD 的图层隐藏状态导入后全丢，所有图层都是显示。

**原因**：[psd_format.cpp](../src/app/file/psd_format.cpp) 的 `onBeginLayer` 里
有一行被**注释掉**的 `// m_currentLayer->setVisible(layerRecord.isVisible());`
—— 未完成的功能，不是死代码。

**为什么当初被注释掉**（不能简单地取消注释）：
`onEndLayer` 里有 PSD 逐帧动画的支持分支。对动画 PSD，图层的"隐藏"位表示
**"该图层不在这一帧出现"**，那个分支已经把它表达成 cel 的有无了。
若同时 `setVisible(false)`，整个图层会消失，动画就没了。

**正确改法**：只在**未走动画分支**时应用可见性：
```cpp
const bool usesFrameAnimation = (!m_framesInfo.empty() &&
                                 layerRecord.inFrames.size() == m_framesInfo.size());
if (m_currentLayer && !usesFrameAnimation)
  m_currentLayer->setVisible(layerRecord.isVisible());
```
图层组同样带这个标志位，`isCloseGroup()` 分支里也补上了 `setVisible()`。

**验证**（`信仰搅拌机-泰拉小人.psd`）：18 显示 / 4 隐藏，
与 Python 解析出的 flags 位（`图层 3`/`图层 2`/`图层 7`/`图层 9` 的 bit1=1）**完全一致**。

### 12.8 ❌ 图层蒙版：**尚未支持**

明确回答：**没有做兼容**。现状：
- [decoder.cpp:931](../src/psd/decoder.cpp:931) 直接**跳过**每图层的蒙版数据段：
  ```cpp
  uint32_t maskLength = read32();
  m_file->seek(m_file->tell() + maskLength);   // 蒙版参数全丢
  ```
- 蒙版**像素**在通道数据里（`ChannelID::UserSuppliedMask = -2`、
  `RealUserSuppliedMask = -3`，[psd.h:233](../src/psd/psd.h:233)），
  decoder 会把这些通道 ID 塞进 `img.channels`，但
  [psd_format.cpp](../src/app/file/psd_format.cpp) 的通道分发只认
  Red/Green/Blue/Alpha/TransparencyMask，`-2`/`-3` 落到 if/else 链末尾被静默丢弃
- 只有**全局**蒙版信息被解析（`readGlobalMaskInfo`），那是另一回事

**实现方案（待做）**：Aseprite 没有"图层蒙版"这个概念，所以只能**烘进 alpha**：
1. decoder 里读出蒙版矩形（top/left/bottom/right）+ 默认色 + 参数，加进 `LayerRecord`
2. 分发 `-2` 通道的像素数据给 delegate
3. psd_format 里把蒙版值乘进图层 alpha（注意蒙版矩形与图层矩形**不一定重合**，
   矩形外用蒙版的默认色填充）

工作量与 encoder 相当，且同样落在 submodule 里。

---

## 13. 中文像素字体：官方同款方案（已解决）

### 13.1 走过的弯路（记下来免得重犯）
我一开始把中文显示问题归因为 `fonts.xml` 的回退链配置。**这是错的**：
`data/fonts/fonts.xml` 与官方安装版**逐字节相同**（仅 CRLF/LF 差异），官方也没装中文语言包。
用户直接指出"官方发行版显示汉字没问题"——是对的，配置不是原因。

### 13.2 真正的答案：官方发行版换掉了整个默认字体
官方安装目录里藏着一个 **6 MB 的字体文件**：
```
C:\Program Files\Aseprite\data\extensions\aseprite-theme\Zfull-GB.ttf
C:\Program Files\Aseprite\data\extensions\aseprite-theme\dark\Zfull-GB.ttf
```
`Zfull-GB` = **Zpix（最像素）的简体 GB 全字集版**，一个真正的点阵/像素 TrueType 字体，覆盖 CJK。

官方发行版的 `theme.xml` 把默认字体整体换成了它：
```xml
<fonts>
    <font name="Zfull-GB" type="truetype" antialias="false" file="./Zfull-GB.ttf"></font>
    <font id="default" font="Zfull-GB" size="10"/>
    <font id="mini"    font="Zfull-GB" size="10"/>
</fonts>
```
关键点：
- **`antialias="false"`** —— theme.xml 支持这个属性，保证像素感
- default 和 mini **同一字体同一字号 10**，所以拉丁字符和汉字字号、基线完全一致
- **git 仓库里没有这个 ttf**，仓库的 theme.xml 仍用点阵 `aseprite_font.png`（无 CJK 字形）
  → 这就是"仓库自建版"和"官方发行版"的差异所在

官方 `data/strings/` 里有 `zh_Hans.ini`，中文是官方支持语言，字体是为它配套塞进去的。

### 13.3 我们的做法（接缝 S11）
1. 从官方安装目录复制 `Zfull-GB.ttf` 到 `data/extensions/aseprite-theme/` 和 `dark/`
   （md5 `55f2c9a22f5d166731c35e706edea600`，两处同一文件）
2. 两个 `theme.xml` 的 `<fonts>` 块照抄官方

> 📌 `data/` 的复制是 `file(GLOB_RECURSE)`（[src/CMakeLists.txt:134](../src/CMakeLists.txt:134)），
> **glob 在 configure 时求值** —— 新增 data 文件后必须重跑 configure，否则不会被复制到 `bin/data/`。
> 不需要为此加接缝。

> ⚖️ 许可：Zpix 字体文件与官方发行版完全一致。自用构建无问题；若要分发需确认 Zpix 授权条款。

### 13.4 修复前后
| | 修复前 | 修复后 |
|---|---|---|
| 汉字 | 回退到 14px 系统矢量字，比行高还大、上下裁切、抗锯齿 | ✅ 像素字体，与数字同字号同基线 |
| 拉丁字符 | 点阵 `aseprite_font.png` | 同一 Zfull-GB，观感一致 |

### 13.5 ★为什么没下载额外字体，其他语言也能像素显示★

这一节推翻了我最初两次错误的解释，最终结论经过实测。

**第一步：Zfull-GB 到底覆盖哪些文字**（直接解析 TTF 的 cmap 表，22973 个码位）

| Unicode 区块 | 覆盖率 |
|---|---|
| CJK 统一汉字 | **99.6%** (20910/20992) |
| 假名 | 92.2% |
| 拉丁（基本+扩展） | 60.0% |
| 希腊 | 50.7% |
| 西里尔 | 36.7% |
| **谚文音节** | **0%** (0/11172) |
| **阿拉伯 / 泰文 / 希伯来 / 天城文** | **0%** |

所以中/俄/希腊/日假名是**字体自带的真像素字形**；
而韩语、阿拉伯语**字体里一个字形都没有**。

**第二步：那谚文是谁画的？**
- ❌ 不是我加的 `<fallback>` —— **实测把回退整块删掉，韩语显示完全不变**
- ✅ 是 **Skia 排版器的逐字符自动字体替换**：
  [laf/text/skia_text_blob_shaper.cpp:135](../laf/text/skia_text_blob_shaper.cpp:135)
  ```cpp
  auto fontRun = SkShaper::MakeFontMgrRunIterator(text.c_str(), text.size(),
                                                  skFont, skFontMgr,
                                                  "Arial",  // Fallback
                                                  ...);
  ```
  主字体缺字形时，Skia 通过 `SkFontMgr`（Windows 后端 = DirectWrite）
  向系统要一个有该字形的字体 —— 用的是**操作系统预装字体**
  （韩语→Malgun Gothic，阿拉伯语→Segoe UI 等），所以**不需要下载任何东西**。

**第三步：为什么替补的矢量字看起来也是像素风、还同基线**
- 替补字体沿用同一个 `SkFont` 尺寸 → **字号一致、基线一致**
- 渲染沿用我们设的 **`antialias="false"`** → 无抗锯齿、硬边缘
- 再叠加 `screen_scale=2` 的最近邻整数放大 → 更显块状

⇒ **`<fallback>` 元素是多余的，已删除**（符合去冗余原则）。

### 13.6 顺带记录：两个 antialias 默认值不一致（upstream 的坑）
| 分支 | `antialias` 默认 |
|---|---|
| `type="truetype"`（[skin_theme.cpp:257](../src/app/ui/skin/skin_theme.cpp:257)） | **true** |
| `system="字体名"`（[skin_theme.cpp:192](../src/app/ui/skin/skin_theme.cpp:192)） | **false** |

同一个属性名，两个分支默认值相反。用 `system=` 时想开抗锯齿必须显式写 `antialias="true"`。

另外 `theme.xml` 的 `<font>` **确实**支持 `<fallback size="N">` 子元素
（[skin_theme.cpp:346](../src/app/ui/skin/skin_theme.cpp:346)，`size` 生效于
[font_data.cpp:92](../src/app/fonts/font_data.cpp:92)）—— 只是本例用不上。
若将来需要**指定**替补字体（而非交给系统挑），这就是入口，且 `size` 必须与主字体一致。

### 13.7 实测结果（4 种文字，全部像素渲染、同字号同基线）
| 语言 | 字形来源 | 结果 |
|---|---|---|
| 简体中文 `zh_Hans` | Zfull-GB 自带 | ✅ `文件(F) 编辑(E) 精灵(S) 图层(L) 帧(R) 选择(T)` |
| 俄语 `ru` | Zfull-GB 自带 | ✅ `Файл(F) Редактировать(E) Спрайт(S)` |
| 韩语 `ko` | **系统字体替换** | ✅ `파일 편집 스프라이트 레이어 프레임` |
| 阿拉伯语 `ar` | **系统字体替换** | ✅ `مساعدة عرض تحدو الإطار الطبقة` |

---

## 15. 多语言支持（接缝 S13）

**问题**：我们的构建没有语言可选。
`data/strings/` 仓库里**只有 `en.ini`**（官方发行版有 24 个），
而 `Strings::availableLanguages()`（[strings.cpp:52](../src/app/i18n/strings.cpp:52)）
扫描 `data/strings/` 和 `data/strings.git/` 里的 `*.ini` —— 只有一个文件，选择器自然是空的。

**做法**：打开官方机制 `ENABLE_I18N_STRINGS=ON`。它用 CMake FetchContent
克隆 `https://github.com/aseprite/strings.git` 到 `bin/data/strings.git`
（[src/CMakeLists.txt:111](../src/CMakeLists.txt:111)）。

选它而不是从官方安装目录复制 24 个 ini 的理由：**这是官方机制且自动跟随上游**，
符合硬性要求 1；复制快照会过时。

**结果**：拿到 **44 种语言**（比官方发行版的 24 个还多，因为直接克隆了上游最新仓库）。
切换入口：`编辑 > 偏好设置 > 常规 > Language`，或直接改
`%APPDATA%\Aseprite\aseprite.ini` 的 `[general] language=`。

> ⚠️ configure 时需要网络（克隆 strings 仓库）。离线环境要退回 `-DENABLE_I18N_STRINGS=OFF`。

---

## 16. 代码冗余审查（2026-07-27）

对本分支所有改动做了一遍自查，发现并处理：

| 发现 | 处理 |
|---|---|
| `clear_layer_thumbnail_cache()` 已定义但**从未被调用**（死代码） | 接到 `Timeline::onInitTheme()` 上。缓存的 surface 绑定显示色彩空间、图标用主题色着色，主题切换本就该丢弃缓存 —— 这个函数是有用的，不该删 |
| `decoder.cpp` 里我加的 `#include <string>` 与 `psd.h` 重复 | 删除 |
| `draw_layer_thumbnails_toggle()` 里构造 `gfx::Rect src` 只为取 4 个分量 | 改成直接用 `srcx` 整数 |
| `psd_format.cpp` 那行被注释掉的 `// setVisible(...)` | 不是死代码而是**未完成功能**，已正确实现（见 §12.7） |
| theme.xml 里我加的 `<fallback>` 块 | **实测证明无效果**（删掉后韩语显示不变），Skia 已自带自动替换 → 删除，见 §13.5 |

其余检查通过：所有新增的 include 都被使用；两个宽度宏都有使用点；
`kMaxCacheEntries`、`psd_sanitize_utf8`、`mods_wide_to_utf8` 均有调用。

---

## 17. （历史记录）最初对中文问题的错误分析

修好乱码后中文能正确显示，但**字号明显偏大、上下被裁切**，与相邻 ASCII 数字不匹配。

根因在 [data/fonts/fonts.xml](../data/fonts/fonts.xml) 的回退链：
```
Aseprite (点阵像素字体 aseprite_font.png，仅拉丁字符，字高约 9px)
  └─ fallback → Unicode  size=14      ← 汉字走这里，14px TrueType，比行高还大
       └─ fallback → Unicode Japanese (msjh.ttc = 微軟正黑體，繁体取向)
```

**已排除**：`fonts.xml` 与官方安装版**逐字节相同**（仅 CRLF/LF 行尾差异），
官方也没有中文语言包。所以**不是我们构建的配置问题**，是 upstream 的通用行为。

可选修法：
1. 只调 `size`（14 → 10/11）—— 一行改动，能解决裁切和大小不匹配，但仍是抗锯齿矢量字
2. 换**像素中文字体**（Ark Pixel 方舟像素 / Fusion Pixel 缝合像素 / Zpix 最像素，均开源）
   + `FontHinting` 关抗锯齿（[font_info.h:85](../src/app/fonts/font_info.h:85) 有 `antialias()`）
   —— 真正像素化，但需引入字体文件并确认许可

---

## 10. 待办

- [x] 首次编译验证 F2/F3
- [x] 缩略图列开关的 UI 入口（表头按钮，S12）
- [x] PSD 图层可见性（§12.7）
- [x] 多语言 + 字体回退（S11/S13）
- [x] 一键编译启动脚本（run.cmd）
- [x] 代码冗余审查（§16）
- [ ] **F1：`mods/file/psd_encoder.cpp` 导出**，按 §5.1 表格移植 ← 下一步
- [ ] **F1：S3 接缝**（flags + `onSave` 转调）← 下一步
- [ ] F1：**图层蒙版**，方案见 §12.8（submodule 改动）
- [ ] F1：修 `lsct` 虚假图层 bug（§12.6）
- [ ] F1：**ZIP 解压**（见 §4.1.1，用 `third_party/zlib`），否则 16/32bit 的真实 PS 文件会静默出错
- [ ] F1：CMYK/Lab 色彩模式转换
- [x] 缩略图开关图标 —— 占位图已被采纳为正式图标，不再更换
- [ ] S5：`SelectLayerBounds` 命令注册（让 Lua 插件也能调）
- [ ] 跑 `tests/` 回归，确认插件支持无损
- [ ] 考虑：图层组行是否合成子图层缩略图（当前留空）

---

## 11. F2/F3 验证记录（2026-07-27）

### 11.1 截屏的坑 ★
Aseprite 用 Skia/GPU 合成，**普通 GDI 截屏抓到全黑**。必须用
`PrintWindow(hwnd, hdc, PW_RENDERFULLCONTENT=2)` 抓 DWM 合成结果。
另外 NVIDIA overlay 会不时抢焦点，自动化点击前需要
`BringWindowToTop` + `SetForegroundWindow` 重新置前。

### 11.2 测试素材
用 Lua API 批处理生成（顺便验证了插件/脚本支持无损）：
- `build/bin/mods_test.aseprite` —— 4 图层 + 1 图层组，内容位置各异
- `build/bin/mods_frames.aseprite` —— 2 图层 × 2 帧，帧间内容位置不同 + 帧2 缺 cel

生成脚本思路：`Sprite(32,32)` → `newLayer/newGroup/newCel` → 逐像素 `drawPixel` → `saveAs`。

### 11.3 结果

| 验证项 | 结果 |
|---|---|
| 缩略图列出现在最左侧 | ✅ |
| 表头开关（眼睛/锁/连续/齿轮/洋葱皮）与各行图标对齐 | ✅ P2a 的整体右移正确 |
| 图层组行（无 cel）留空且不崩 | ✅ |
| 缩略图内容/位置正确（画布内相对位置保留） | ✅ |
| 活动行高亮覆盖缩略图格 | ✅ |
| **Ctrl+单击缩略图 → 精确选中图层内容边界** | ✅ 蚂蚁线贴合 10×10 内容，非整画布 |
| **Ctrl+Shift+单击 → 加选** | ✅ 两个图层边界同时选中 |
| 状态栏提示（P7） | ✅ `Layer 'GreenBar' -- Ctrl+click to select its content (...)` |
| **换帧后缩略图刷新（P8）** | ✅ 红块位置随帧变化；帧 2 无 cel 的图层变空白 |
| Lua 脚本引擎（插件支持） | ✅ `-b --script` 正常执行 |


---

## 18. 首次 upstream 同步实测（2026-09-10）

`35c35e645` → `375989a61`，**38 个提交，约 7 周**。

### 18.1 结果：零冲突 ✅
接缝最小化策略经受住了第一次真实同步。上游对我们接缝文件的改动情况：

| 接缝文件 | 上游改动次数 |
|---|---|
| `src/app/ui/timeline/timeline.cpp`（12 个改动点，最大接缝） | **0** |
| `src/app/file/psd_format.cpp` | **0** |
| `data/extensions/aseprite-theme/theme.xml` ×2 | **0** |
| 根 `CMakeLists.txt` / `src/CMakeLists.txt` / `.gitignore` | **0** |
| `src/psd` submodule 指针 | **没动** |
| `src/app/CMakeLists.txt` | 2（git 自动合并成功） |

### 18.2 流程（已验证可用）
```
# 1. submodule 先提交（它最脆弱，游离改动会被 submodule update 冲掉）
cd src/psd && git add -A && git commit && cd ../..
# 2. 主仓库按逻辑分组提交
git add ... && git commit
# 3. 合并
git fetch upstream && git merge upstream/main
```
⚠️ **合并前工作区必须干净**。我们和上游都改了 `src/app/CMakeLists.txt`，
脏工作区会让 merge 直接拒绝。

### 18.3 ★同步中发现的上游缺陷（S14）★

上游 `443209de5 Avoid registering extensions for disabled formats` 给
[src/dio/detect_format.cpp](../src/dio/detect_format.cpp) 加了：
```cpp
#ifdef ENABLE_PSD
  if (ext == "psd" || ext == "psb") return FileFormat::PSD_IMAGE;
#endif
```
但 CMake 里**只补了 WebP 的定义**：
```cmake
target_compile_definitions(dio-lib PUBLIC -DENABLE_WEBP)   # 有
if(ENABLE_PSD)
  target_compile_definitions(app-lib PUBLIC -DENABLE_PSD)  # 只给了 app-lib
endif()
```
`detect_format.cpp` 属于 **dio-lib**，拿不到 `ENABLE_PSD` →
**即使 `ENABLE_PSD=ON`，PSD 文件也完全不被识别**。
上游因为默认 `off` 所以没暴露。已加 `target_compile_definitions(dio-lib PUBLIC -DENABLE_PSD)`。

> 📌 **教训**：同步后不能只看"编译过了"就算完。
> 上游给某个特性加编译期守卫时，要确认**所有**用到该守卫的 target 都拿到了宏。

### 18.4 另一个坑：`bin/data/strings.git` 陈旧产物
关掉 `ENABLE_I18N_STRINGS` 后，`build/bin/data/strings.git/` **不会自动删除**，
而 `Strings::availableLanguages()` 同时扫描 `strings/` 和 `strings.git/`
→ 那 21 种未发布语言又回来了。需手动 `rm -rf build/bin/data/strings.git`
（或 `run.cmd --clean`）。

### 18.5 同步后验证清单（全部完成）
- [x] 编译通过
- [x] PSD 仍能识别并正确导入（尺寸/中文名/可见性）
- [x] 语言数 = 23
- [x] 缩略图列 / 开关按钮 / 中文字体 GUI 复验
- [x] `tests/` 回归：**67 个 Lua 脚本测试 + 4 个 CLI 测试全部通过**
- [x] 已推送到 `origin`（Mornia-source/aseprite）

### 18.6 跑测试套件的方法（本仓库路径含空格，有坑）
`tests/run-tests.sh` 多处 `$ASEPRITE`、`cd $1` 未加引号，含空格的路径会崩。
本卷未启用 8.3 短名，解决办法是建一个无空格路径的目录联接：
```powershell
New-Item -ItemType Junction -Path "$env:TEMPsetest" -Target "<repo>uildin"
```
```bash
cd tests   # 必须在 tests/ 目录内，脚本用相对路径 scripts/...
ASEPRITE=/c/Users/Cherry/AppData/Local/Temp/asetest/aseprite.exe bash run-tests.sh
```
`cli/save-as.sh` 仍会因 `cd $1` 失败 —— **环境问题，不是回归**。
脚本遇错即 `exit 1`，所以能跑到它就说明前面全过了。


---

## 19. 工具不透明度（接缝 S15）

**需求**：画笔、橡皮擦、轮廓工具在顶部选项栏加不透明度。

### 19.1 现状调查（结论和直觉相反）
`InkOpacityField` **本来就在** context bar 里，只是被这个条件挡住：
```cpp
showOpacity = supportOpacity && ((isPaint && (hasInkWithOpacity || hasImageBrush)) || isEffect);
hasInkWithOpacity = ((isPaint && tools::inkHasOpacity(toolPref->ink())) || isEffect);
```
而 `inkHasOpacity()` 只对 `ALPHA_COMPOSITING` / `LOCK_ALPHA` 为真，
工具默认墨水却是 `SIMPLE`（[pref.xml:354](../data/pref.xml)）。

**★ 橡皮擦其实早就支持了 ★** —— `EraserInk::isEffect()` 返回 **true**
（[inks.h:334](../src/app/tools/inks.h)），一个事实连锁解决三处：
- `showOpacity` 里 `|| isEffect` 成立 → **控件本来就显示**
- `tool_loop_impl.cpp:268` 的 `!m_ink->isEffect()` → 不透明度**不会被打回 255**
- `adjustToolInkDependingOnSelectedInkType` 要求 `isPaint && !isEffect`
  → 橡皮擦的墨水**不会被替换**
且 EraserInk 已完整实现不透明度（255 走 CopyInk，否则 Transparent/MergeInk）。

⇒ 真正缺的只有**画笔和轮廓**这类停留在 SIMPLE 墨水的 paint 工具。

### 19.2 为什么不能只是"把控件显示出来"
[tool_loop_impl.cpp:268](../src/app/ui/editor/tool_loop_impl.cpp:268)：
```cpp
// Ignore opacity for these inks
if (!tools::inkHasOpacity(params.inkType) && m_brush->type() != kImageBrushType &&
    !m_ink->isEffect()) {
  m_opacity = 255;
}
```
SIMPLE 墨水是直接替换像素的，不透明度**被强制打回 255**。
只显示控件会得到一个"调了没反应"的假控件。

### 19.3 做法（方案 A：自动提升墨水）
`src/app/mods/tools/tool_opacity.{h,cpp}`：
- `tool_offers_opacity(tool)` —— 对"纯 paint 工具"（`isPaint && !isEffect`）返回真
- `promote_ink_for_opacity(tool, opacity)` —— 不透明度 < 255 且墨水为 SIMPLE 时，
  提升为 `ALPHA_COMPOSITING`（正是手动操作等价的墨水），并遵循 `shareInk` 偏好

**只升不降**：调回 255 不会改墨水，避免把用户刻意选的墨水悄悄改掉。

**为什么只对 `isPaint && !isEffect` 提升**：因为只有这类工具的墨水会被
`adjustToolInkDependingOnSelectedInkType` 按 inkType 重映射。
对 effect 类（橡皮擦/模糊/涂抹）改 inkType 无意义，
且橡皮擦若被误判成 paint 会**变成画笔**——这是本次最大的坑。

### 19.4 三处接缝（context_bar.cpp）
| 点 | 位置 | 内容 |
|---|---|---|
| 1 | include 区 | `#ifdef ENABLE_MODS` 引入头文件 |
| 2 | `InkOpacityField::onValueChange()` 末尾 | 调 `promote_ink_for_opacity()` |
| 3 | `showOpacity` 表达式 | `\|\| mods::tool_offers_opacity(tool)` |

### 19.5 验证
铅笔选中时选项栏出现「不透明度：100%」（原本完全没有该项）。

### 19.6 踩坑：CMake 缓存变量不随 option() 默认值变化
把 `ENABLE_I18N_STRINGS` 的默认值改回 `off` 后，**缓存里仍是 ON**
（`option()` 不覆盖已存在的缓存项），FetchContent 继续尝试更新已被删除的
`strings.git` 目录并报 `Failed to get the hash for HEAD`。
⇒ `run.cmd` 现在**显式传 `-DENABLE_I18N_STRINGS=OFF`**，并需清理
`build/_deps/clone_strings-*`。改选项默认值时都要注意这一点。


---

## 20. 调色板色条（接缝 S16/S17/S18）

**需求**：三条细长色条固定在现有调色板上方；两端各一个方形色块，
左键单击色块填入前景色、右键填入背景色；两端都有色时中间出现**两条**渐变带，
左键取色到前景、右键到背景；视图菜单可开关，默认开。

### 20.1 设计取舍
- **两条渐变带 = 两种插值**：上=RGB 线性，下=HSV（色相走最短弧）。
  两者只在**中间调**分歧，而中间调正是取色最常用的区域 —— 一次给出两种选择。
- **持久化用全局配置**（`[Mods] PaletteBarN{L,R}`），不写进 .aseprite。
  理由：这是取色工具而非文档数据，切换文档应保留；也不污染文件格式。
  代价：Aseprite 只在**正常退出**时写 ini，被强杀（如 `run.cmd` 的 taskkill）会丢。
- **位置**：`ColorBar` 是垂直 Box，插在 `m_tilesHBox` 与 `m_splitter` 之间
  → 视觉上正好在调色板上方。

### 20.2 UI 必须用官方绘制函数
最初用 `fillRect`+`drawRect` 手绘，风格与软件割裂。正确做法：
- 色块：`draw_color_button()`（[modules/gfx.h](../src/app/modules/gfx.h)）——
  官方色块渲染，自带主题描边、透明棋盘格、hover 态
- 渐变带：套同一组 9 宫格边框 `theme->parts.colorbar0..3`
  （即 `draw_color_button` 内部用的那组）

### 20.3 ★两个坐标/绘制陷阱★

**(1) `childrenBounds()` 是父坐标，不是 client 坐标**
`onPaint` 用 client 坐标（原点 0,0），而 `childrenBounds()` 返回相对父控件的坐标。
用错会导致**背景画对了、内容全部画到可视区外**（现象：控件占了位但看不到东西）。
→ 用 `clientChildrenBounds()`；鼠标坐标也要 `- bounds().origin()` 转过去。

**(2) `gfx::Rect::shrink()` 原地修改并返回 `*this`**
```cpp
const gfx::Rect inner = area.shrink(scale);  // ← area 也被缩小了！
```
[laf/gfx/rect.h:215](../laf/gfx/rect.h:215) 签名是 `RectT& shrink(const T&)`。
后果：边框画在已缩小的矩形上，渐变色**溢出到边框外**。
→ 先复制再 shrink。

**(3) 圆角边框 + 方形填充 = 四角溢出**
边框是圆角矩形，颜色填的是方形，四个角会露在圆角外。
→ `clear_rounded_corners()` 把四角各 1×guiscale 的方块用背景色补回。

### 20.4 验证
- 三条色条显示在调色板上方，行间留白 ✅
- 左键/右键填色块 ✅
- 两条渐变带（RGB / HSV 可辨） ✅
- 从渐变带左键取色到前景 ✅
- `app.command.ShowPaletteBars()` 可被 **Lua 插件调用** ✅，菜单加载无告警 ✅


---

## 21. 打包（[package.cmd](../package.cmd)）

```
package.cmd            先编译再打包到 distpackage.cmd --nobuild  直接打包 buildin 现有产物
```

产出 `dist\Aseprite-mods-<版本>-win64{,.zip}`，约 **16.5 MB**。

### 21.1 包内容
| 项 | 说明 |
|---|---|
| `aseprite.exe` | 21 MB |
| `icudtl.dat` | Skia 的 ICU 数据 |
| `data/` | 主题、字体、23 种语言、widgets、扩展、`data/mods/icons` |
| `README.txt` | 功能说明 + 已知限制 + 许可声明 |

**刻意排除**：`*.pdb`（约 160 MB 调试符号）、`gen.exe`（构建期代码生成器）、
`data/strings.git`（关掉 `ENABLE_I18N_STRINGS` 后残留的空目录，
但**语言扫描器仍会读它**，见 §18.4）、测试用的 `*.lua` / `*.psd`。

### 21.2 自包含性（已验证）
`dumpbin /dependents` 显示只依赖 Windows 系统 DLL —— 静态链接
（`CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`），**无需 VC 运行库**。
实测解压到全新目录可直接运行：版本号正确、PSD 导入正常
（268×80 / 中文图层名 / 4 隐藏）、`app.command.ShowPaletteBars()` 可调用、GUI 中文正常。

### 21.3 版本号
来自 `aseprite.exe --version`，而该字符串由 **configure 时**的 `git describe` 生成
（上游 `cac270ceb`）。所以**提交之后要重新 configure**，否则版本号是旧的。
带 `-dirty` 后缀表示打包时工作区不干净。

### 21.4 ⚠️ 许可
Aseprite 由 Igara Studio S.A. 以 **EULA** 授权，**不是**开源许可：
- **2(b) Distribution**：不得向第三方分发副本
- **2(g) Source code**：只能为**个人目的**编译和修改源码

即"自己编译自用/备份"允许，**分发编译产物给他人是明文禁止的**。
`README.txt` 里附了这段声明和官方购买链接。


---

## 22. 萨卡兹语（整活功能，接缝 S19）

在语言列表里加一个 **`Sakarz 萨卡兹语`**：文本用**英文原文**，靠**换字体**把拉丁字母
显示成《明日方舟》卡兹戴尔文字。

### 22.1 做法
| 组成 | 内容 |
|---|---|
| `data/strings/sarkaz.ini` | en.ini 的逐字复制，只改 `[_] display_name` |
| `data/mods/fonts/EndfieldByButan.ttf` | 14.2 KB，**238 个码位，完整覆盖可打印 ASCII**（字母+数字+标点） |
| 接缝 S19 | 语言为 `sarkaz` 时，把主题字体换成该字体 |

**语言显示名从 ini 的 `[_] display_name` 读取**
（[strings.cpp:71](../src/app/i18n/strings.cpp:71)），不需要改代码。

### 22.2 ★接缝位置选得好★
[skin_theme.cpp](../src/app/ui/skin/skin_theme.cpp) 里**上游本来就有**
"用户自定义字体覆盖主题字体"的逻辑（`pref.theme.font()` / `miniFont()`）。
我们的覆盖紧跟其后，且**只在用户没有显式设置字体时**生效
—— 用户的字体选择永远优先。改动只有 include + 一个 `#ifdef` 块。

### 22.3 字体选择的演进
| 字体 | 大小 | 码位 | 结果 |
|---|---|---|---|
| ~~Sarkaz.ttf~~ | 4.7 KB | 54 | 仅 A-Z/a-z。数字标点回退系统字体，**数值保持可读** |
| **EndfieldByButan.ttf** | 14.2 KB | 238 | 全 ASCII。**数字也变成异世界文字**，数值框不可读 —— 沉浸感更强 |

⚠️ **退路**：两种字体都不含 CJK，所以语言名里的 `萨卡兹语` 四个汉字会经
Skia 自动替换（§13.5）用系统字体渲染 —— 界面全变成异世界文字后，
仍能靠这四个汉字在语言下拉里找回来。**这是有意为之，别把显示名改成纯拉丁。**

### 22.4 像素化
`FontData::setAntialias(false)` + `FontHinting::Normal`。
实测菜单栏文本区**只有 2 种颜色**（纯黑 + 背景色），边缘为硬像素，
与主题的点阵字体观感一致。

### 22.5 切换方式
`编辑 > 偏好设置 > 常规 > Language`，或改
`%APPDATA%\Asepriteseprite.ini` 的 `[general] language = sarkaz`。

> 📌 换 data/ 下的文件后要 `run.cmd --reconfigure`（glob 在 configure 时求值，§13.3），
> 且**旧文件不会从 `build/bin` 自动删除** —— 换字体时需手动清理，否则会混进打包。


---

## 23. 语言切换的实时生效（接缝 S20 / S21）

### 23.1 两个独立问题
| 现象 | 根因 | 接缝 |
|---|---|---|
| 切走后仍是萨卡兹**字体** | 字体在主题加载时选定，换语言不重载主题 | S20 |
| 切换后**文本**不变 | XML 文本在构造时就把 string id 丢掉了 | S21 |

### 23.2 S20：字体
模块内用 `g_applied` 记录"当前主题是用萨卡兹字体构建的"，
与 `sarkaz_language_active()` 比对，不一致就 `ui::set_theme()` 重新生成。

⚠️ **必须延迟执行**：`Strings::LanguageChange` 有多个监听者
（[tool_box.cpp:171](../src/app/tools/tool_box.cpp:171) 重载工具、
[keyboard_shortcuts.cpp:95](../src/app/ui/keyboard_shortcuts.cpp:95) 重置键表、
main_window 重载菜单）。`set_theme()` 会重建所有控件，
同步调用等于在信号级联**中途**把 UI 抽走。
→ 用 `ui::execute_from_ui_thread()` 推迟到当前事件之后。

### 23.3 ★S21：文本重译 —— 根因是信息被丢弃★
```cpp
// xml_translator.cpp:21 -- 构造时解析，只返回译文
if (value[0] == '@') return Strings::Translate(value + 1);
// widget_loader.cpp -- 只存译文
widget->setText(m_xmlTranslator(elem, "text"));
```
控件身上**没有保留 string id**，所以语言变了之后**任何刷新都不可能重译**。
这就是为什么 stock `MainWindow::onLanguageChange()` 只有
`m_menuBar->reload()` —— 菜单栏能变是因为它整个从 gui.xml 重建。
换主题走同一个 `ui::set_theme()`，同样不重译。**上游没有全局重译路径可复用。**

**做法**：把丢掉的 id 记下来。
| 位置 | 内容 |
|---|---|
| `XmlTranslator::stringId()` | 新增，返回解析后的 id（prefix 规则留在原处，避免在调用点复制） |
| `mods/i18n/retranslate.*` | `I18nTextProperty` 挂在控件上存 id；`retranslate_all()` 递归重查 |
| `widget_loader.cpp` | 2 处 `setText` 之后挂属性（buttonset item + 通用属性填充） |
| `main_window.cpp` | `onLanguageChange()` 里对 manager 根控件调用 |

只在文本真的变了时才 `setText`，避免让整棵树无谓失效。

**覆盖不到**（需重启）：C++ 里直接 `setText(Strings::xxx())` 的地方、
tooltip（存在 TooltipManager 而非控件上）、已构造的下拉项。

---

## 24. 自建更新服务器（接缝 S22）

### 24.1 官方已有的基础设施（几乎不用自己写）
| 组件 | 位置 |
|---|---|
| HTTP 请求 + XML 响应解析 | [src/updater/check_update.cpp](../src/updater/check_update.cpp) |
| 后台线程、`waitdays` 节流、启动次数统计 | [src/app/check_update.cpp](../src/app/check_update.cpp) |
| 回调 `onNewUpdate(url, version)` | [check_update_delegate.h](../src/app/check_update_delegate.h) |
| 首页"有新版本"提示按钮 | home_view |

需要 `ENABLE_UPDATER=ON`（上游默认就是 on）。

### 24.2 为什么不用官方的 `CUSTOM_WEBSITE_URL`
[info.c:19](../src/ver/info.c:19) 里它替换的是 `WEBSITE` **本身**，
而下载页、贡献者页都是从 `WEBSITE` 拼出来的：
```c
#define WEBSITE_DOWNLOAD     WEBSITE "download/"
#define WEBSITE_CONTRIBUTORS WEBSITE "contributors/"
#define WEBSITE_UPDATE       WEBSITE "update/?xml=1"
```
用它会把这些链接一并指向我们的服务器。
⇒ 新增 `MODS_UPDATE_URL`，**只 `#undef` + 重定义 `WEBSITE_UPDATE`**。

### 24.3 用法
```
cmake -B build -DMODS_UPDATE_URL="https://你的域名/aseprite/update?xml=1"
```
⚠️ **URL 必须自带查询串**（`?xml=1` 之类）——
[check_update.cpp:104](../src/updater/check_update.cpp:104) 直接往后拼 `&uuid=...`，
没有 `?` 会拼成非法 URL。

服务器返回的 XML：
```xml
<update latest="0" type="major" version="1.3.18.5-99"
        url="https://你的域名/aseprite-mods-1.3.18.5-99-win64.zip" waitdays="1" />
```
- `latest="1"` → 已是最新
- `type` → `critical` / `major`
- 版本比较用 `base::Version`，`localVersion < serverVersion` 才提示

### 24.4 实测（本地 Python 服务器）
```
REQ: /update?xml=1&inits=263&exits=262
```
首页右上角出现「新的 Aseprite v9.9.9 可用！」按钮 —— **整条链路验证通过**。

### 24.5 还没做的：下载与安装
官方只做到**通知**，`onNewUpdate` 把 url/version 存进 preferences 就结束了。
要真正"自动在线更新"还需要：
| # | 内容 |
|---|---|
| B | 下载 zip（复用 `net::HttpRequest`，带进度） |
| C | **完整性校验** —— 至少 SHA-256，最好签名验证（公钥编进程序） |
| D | 应用更新 —— Windows 上程序不能覆盖自身，需要一个 helper：主程序退出后替换文件再重启 |
| E | UI：提示 / 进度 / 失败回滚 |

> ⚠️ **安全**：这是一条能在本机下载并执行任意代码的通道。
> 服务器被入侵或走明文 HTTP 等于交出机器。HTTPS + 校验和 + 签名**不是可选项**。
> ⚠️ **许可**：更新分发的仍是编译后的 Aseprite 二进制，受 EULA 2(b) 约束（见 §21.4）。

---

## 25. 冗余审查（第二轮，2026-09-10）

统计 mods 模块导出的 13 个函数在模块外的调用点：

| 结果 | 处理 |
|---|---|
| 12 个有调用（1~4 处） | 保留 |
| `sarkaz_language_active()` **0 处** | 只在模块内用了 2 次却暴露在公开头文件 → 收进匿名 namespace |

### 25.1 上游同步难度（相对 upstream/main）
改动的**官方文件只有 11 个**：
`timeline.cpp`(+154) / `psd_format.cpp`(+90) / `theme.xml`×2(+35) /
`main_window.cpp`(+21) / `context_bar.cpp`(+18) / `skin_theme.cpp`(+17) /
`app/CMakeLists.txt`(+17) / `color_bar.*`(+28) / `commands_list.h`+`gui.xml`(+4) /
`ver/*`(S22)

上次合并 38 个上游提交**零冲突**，纪律有效。三个风险点：
1. **`timeline.cpp`** —— 12 个改动点集中在列布局，上游若重构图层面板必冲突
2. **`src/psd` submodule** —— 提交只在本地，仓库对外不可构建
3. **`widget_loader.cpp` / `xml_translator`（S21 新增）** —— UI 基础设施，动的频率低但一动就是核心路径


---

## 26. 帮助菜单精简 + 关于对话框 + 标题栏（接缝 S23/S24/S25）

### 26.1 帮助菜单只留「关于」（S23）
原有 8 个条目全部指向 aseprite.org 上描述**官方产品**的资源，
而本构建不是官方产品，留着会误导。

**安全性已验证**：[app_menus.cpp:396-405](../src/app/app_menus.cpp:396) 会在
非 DRM 构建下删除 `enter_license` 相关条目 ——
用的是 `findChild()`（返回空则 `delete nullptr`，安全）
和 `m_groups.find()`（找不到就跳过 erase）。
另外 `help_readme`/`help_docs`/`help_news`/`help_about` 这些 group **代码里零引用**。
⇒ 删掉这些条目不会破坏任何逻辑。**但 `<menu id="help_menu">` 必须保留**
（[app_menus.cpp:791](../src/app/app_menus.cpp:791) 用它设置 `m_helpMenuitem`）。

### 26.2 关于对话框（S24）
官方内容**原样保留**（标题、作者/译者/开源项目链接、Igara 版权、官网），
下方用分隔线隔开我们自己的部分：私有构建声明、新增功能摘要、
**与 Igara Studio 无从属关系**的声明、EULA 与购买正版提示、本构建源码链接。

文案做成可翻译字符串（`[about]` 段的 `mods_*`，en + zh_Hans），
与软件其余部分一致 —— about.xml 的 string id prefix 是 widget id `about`
（[widget_loader.cpp:93](../src/app/widget_loader.cpp:93)）。

「Open Source Projects」链接**已删除** —— 它指向 `docs/LICENSES.md`，
而本工作区删掉了该文件，链接在我们的构建里是死的。

> ⚠️ `about.xml.h` 由 gen 从 about.xml 生成，删掉控件后 `window.licenses()`
> 成员就不存在了，**必须同步删掉 [cmd_about.cpp](../src/app/commands/cmd_about.cpp)
> 里的 Click 绑定**，否则编译失败。

### 26.3 标题栏后缀（S25）
```
cmake -B build -DMODS_TITLE_SUFFIX="内部使用  严禁外传"
```
在 [app.cpp:820](../src/app/app.cpp:820) 追加，位置在架构后缀
（`(x64)`/`(x86)`）**之后**，保证永远在最末。
结果：`Aseprite v1.3.18.5-16-g... - 内部使用  严禁外传`

---

## 27. 造字语言（§22 扩展为三种）

`sarkaz_font.*` 已重构为表驱动的 [conlang_font.*](../src/app/mods/ui/conlang_font.cpp)，
避免为每种语言复制一份模块。新增语言只需往 `kFonts` 表加一行 + 一个 ini。

| 语言 | 显示名 | 文本底本 | 字体 | 覆盖 |
|---|---|---|---|---|
| `sarkaz` | Sarkaz 萨卡兹语（Kazdel） | 英文 | EndfieldByButan.ttf | 全部可打印 ASCII（含数字） |
| `seaborn` | Seaborn 海嗣文（Ægir） | 英文 | AgeFonts001.ttf | 字母+数字，标点仅 ` %-./` |
| `farnorth` | Far North Runes 极北卢恩文字（Sami） | **挪威尼诺斯克语** | FarNorthRunes-Heavy.ttf | 全 ASCII + ÆØÅÄÖ + **Þ Ð** |

### 27.1 ★为什么 farnorth 用尼诺斯克语而不是芬兰语★
原计划用芬兰语，但上游 strings 仓库的 **`fi.ini` 只有 2 字节 —— 是空占位文件**，
芬兰语翻译根本没做。可选的北欧语言实际完成度：

| 语言 | 大小 |
|---|---|
| `fi`（芬兰语） | **2 bytes（空）** |
| `nn`（尼诺斯克） | 75,874 |
| `da`（丹麦语） | 75,414 |
| `sv`（瑞典语） | 78,315（**不在官方 23 种内**） |

选 `nn`：完整、**已在我们发布的 23 种语言内**（无需额外引入）、
北欧语系、且 FarNorthRunes 覆盖它全部特殊字符。

### 27.2 退路仍在
三个字体都不含 CJK，所以显示名里的中文（`萨卡兹语`/`海嗣文`/`极北卢恩文字`）
会经 Skia 自动替换用系统字体渲染 —— 界面全变成异世界文字后，
仍能靠中文在语言下拉里找回来。**别把显示名改成纯拉丁。**


---

## 28. src/psd submodule 的 fork（2026-09-10，已解决）

### 28.1 问题
中文图层名的修复（§12.4 的 A）在 `src/psd` 里，而 `.gitmodules` 指向
`https://github.com/aseprite/psd.git` —— 我们的提交 `080e999` **不在那里**。

后果（当时已取证：`git branch -r --contains 080e999` 返回空）：

| 场景 | 结果 |
|---|---|
| 本机开发 | 正常（对象在本地 submodule 仓库里） |
| 他人 / 换机器 `clone --recursive` | ❌ 拉不到该 SHA，`src/psd` 空目录，**编译失败** |
| 本机误跑 `git submodule update` | ❌ 试图 checkout 不存在的 SHA，**修复被冲掉** |
| GitHub 网页 | 显示为无法解析的断链 |

即：**仓库处于"只有这一台机器能编译"的状态**。

### 28.2 处理
```
gh repo fork aseprite/psd --clone=false     # -> Mornia-source/psd
cd src/psd && git push fork HEAD:main       # 推上我们的提交
# .gitmodules: url -> https://github.com/Mornia-source/psd.git
git submodule sync -- src/psd
git remote set-url origin <fork>            # submodule 内部也切过去
```

> ✅ **许可**：`src/psd` 是 **MIT**（LICENSE.txt: *"free of charge... to use, copy,
> modify, merge, publish"*），fork 并公开发布**完全合规** ——
> 与外层受 EULA 约束的 Aseprite 主体不同，这一点要分清。

### 28.3 验证（全新克隆实测）
```
git clone --depth 1 https://github.com/Mornia-source/aseprite.git <tmp>
git submodule update --init src/psd
→ Submodule path 'src/psd': checked out '080e999...'
→ decoder.cpp 中 mods_wide_to_utf8 引用 2 处 ✓
```
**仓库现在对外可构建。**

### 28.4 后续同步上游 psd 的方法
fork 里保留 upstream 远端即可：
```
cd src/psd
git remote add upstream https://github.com/aseprite/psd.git
git fetch upstream && git rebase upstream/main
git push --force-with-lease origin main
cd ../.. && git add src/psd && git commit    # 更新父仓库的 gitlink
```
