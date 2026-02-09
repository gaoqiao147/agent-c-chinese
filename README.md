# Agent-C

一个用纯 C 语言编写的超轻量级 AI 代理，通过 OpenRouter API 与 AI 通信并执行 shell 命令。

![Agent-C Preview](preview.webp)

## 特性

- **工具调用**：AI 可以直接执行 shell 命令
- **极致优化**：macOS 上仅 4.4KB（GZEXE），Linux 上约 16KB（UPX）
- **对话记忆**：滑动窗口内存管理，高效运行
- **跨平台**：支持 macOS 和 Linux

## 快速开始

### 前置要求

- GCC 编译器
- curl 命令行工具
- OpenRouter API 密钥
- macOS：gzexe（通常预装）
- Linux：upx（可选，用于压缩）

### 构建

```bash
make
```

构建系统会自动检测平台并应用最优压缩：
- **macOS**：使用 GZEXE 压缩 → 4.4KB 二进制文件
- **Linux**：使用 UPX 压缩 → 约 16KB 二进制文件

### 设置

设置你的 OpenRouter API 密钥：

```bash
export OR_KEY=your_openrouter_api_key_here
```

### 运行

```bash
./agent-c
```

## 许可证

**CC0 - "无版权保留"**