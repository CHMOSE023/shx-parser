# SHX 字体解析库

用于解析 AutoCAD SHX 字体文件的库，提供 **C++** 实现。

本项目移植自 [shx-parser（TS）](https://github.com/mlightcad/shx-parser)

---

## 目录

- [功能特性](#功能特性)
- [支持的字体类型](#支持的字体类型)
- [项目构建](#项目构建)
  - [环境要求](#环境要求)
  - [构建](#构建)
  - [API 参考（C++）](#api-参考c)
  - [运行示例](#运行示例)
- [架构说明](#架构说明)
- [许可证](#许可证)

---

## 功能特性

- 解析 SHX 字体文件，提取完整字体数据
- 支持三种字体类型：Shapes、Bigfont（含扩展大字体）、Unifont
- 按需解析 + 形状缓存，性能优异
- 支持 Windows / Linux / macOS（C++17，无第三方依赖）
- 完善的单元测试覆盖

---

## 支持的字体类型

| 类型      | 文件头标识           | 说明                                         |
| --------- | -------------------- | -------------------------------------------- |
| `shapes`  | `AutoCAD-86 shapes`  | 标准形状字体，最常见                         |
| `bigfont` | `AutoCAD-86 bigfont` | 大字体（中日韩等多字节字符集），含扩展大字体 |
| `unifont` | `AutoCAD-86 unifont` | Unicode 字体                                 |

---

## 项目构建

无第三方运行时依赖。

### 环境要求

| 项目             | 要求                                  |
| ---------------- | ------------------------------------- |
| CMake            | ≥ 3.16                                |
| C++ 标准         | C++17                                 |
| 编译器           | MSVC 2019+、GCC 9+、Clang 10+         |
| 单元测试（可选） | 自动通过 FetchContent 下载 GoogleTest |

### 构建

```bash

# 配置（默认启用测试和示例）
cmake -S . -B build

# 仅构建库（跳过测试和示例，无需联网）
cmake -S . -B build -DSHX_BUILD_TESTS=OFF -DSHX_BUILD_EXAMPLES=OFF

# 编译
cmake --build build --config Release

# 运行单元测试
cd build && ctest -C Release --output-on-failure
```
 

#### CMake 选项

| 选项                 | 默认值 | 说明                     |
| -------------------- | ------ | ------------------------ |
| `SHX_BUILD_TESTS`    | `ON`   | 构建 GoogleTest 单元测试 |
| `SHX_BUILD_EXAMPLES` | `ON`   | 构建示例程序             |
| `SHX_BUILD_SHARED`   | `OFF`  | 构建动态库（默认静态库） |

### API 参考（C++）

所有符号位于 `shx` 命名空间，统一通过 `#include "shx_parser.h"` 引入。

 

### 运行示例

示例程序会加载 `data/` 目录下的两个字体文件，将解析信息打印到控制台，并将 `"ABCDEF123"` 渲染为 SVG 文件保存到 `cpp/examples/`。

```bash
# 构建并运行
cmake -S cpp -B cpp/build -DSHX_BUILD_TESTS=OFF
cmake --build cpp/build --config Release --target shx_example
./cpp/build/examples/Release/shx_example.exe
```

控制台输出示例：

```
========================================
Reading font file: .../data/ISO.shx
File size: 4063 bytes

Font Information:
----------------
Font Type   : shapes
Header      : AutoCAD-86
Version     : 1.0
Info        : ISO, Font 09/22/86 ...
Orientation : vertical
Base Up     : 18 / Base Down: 6
Height      : 24 / Width   : 24
Shapes count: 127
Available codes (first 30): 0, 1, 10, 13, 32, 33, 34, ...
----------------

Shape details for "ABCDEF123" at size 12:
  Char 'A' (code 65)  polylines=2  lastPoint=(9.00, 0.00)
    polyline[0]  pts=3  start=(6.00,0.00)  end=(0.00,0.00)
    polyline[1]  pts=2  start=(1.00,2.50)  end=(5.00,2.50)
  ...

SVG saved to: .../cpp/examples/iso_output.svg
```

---

## 架构说明

两个版本共享相同的解析管线设计：

```
二进制数据（ArrayBuffer / vector<uint8_t>）
    │
    ▼
FileReader          — 底层字节读取（Little-Endian）
    │
    ▼
HeaderParser        — 解析文件头，识别字体类型
    │
    ▼
ContentParserFactory
  ├── ShapeContentParser   — SHAPES 类型：顺序读取字符目录
  ├── BigfontContentParser — BIGFONT 类型：偏移量表随机访问 + 扩展大字体
  └── UnifontContentParser — UNIFONT 类型：Unicode 字符目录 + 标签跳过
    │
    ▼
FontData            — 结构化元数据 + 字符码 → 字节码映射
    │
    ▼
ShapeParser         — SHP 字节码虚拟机（按需解析 + LRU 缓存）
  ├── 特殊命令 0x00–0x0F：钢笔抬放、弧线、子形状、位移、缩放、压栈
  └── 向量命令 0x10–0xFF：高4位=长度，低4位=16方向之一
    │
    ▼
ShxShape            — 折线几何 + 包围盒 + SVG 输出
```

### 关键设计点

| 设计                      | 说明                                                                   |
| ------------------------- | ---------------------------------------------------------------------- |
| 按需解析                  | 字符首次访问时才解析字节码，避免启动时全量解析                         |
| 形状缓存                  | 解析后的基础形状（未缩放）存入 `Map / unordered_map`，重复访问直接缩放 |
| 扩展大字体子形状          | 命令 `7,0,primitive#,x,y,w,h` — 子形状归一化后按比例缩放并偏移插入     |
| Y 轴约定                  | 内部坐标系 Y 轴向上；渲染到 SVG 时需翻转：`y_svg = -y + offset`        |
| BIGFONT normalizeToOrigin | `ShxFont.getCharShape` 对 BIGFONT 类型额外调用一次原点对齐             |

---
 
