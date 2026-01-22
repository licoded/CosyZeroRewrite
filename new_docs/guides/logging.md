# Logging Guide

> 日志系统使用规范

---

## 概述

项目使用 **spdlog** 作为日志后端，封装在 `include/log/logger.hpp` 中。

两套日志宏：
- **`LOG_*`** - 带时间戳和日志级别前缀
- **`NOP_LOG_*`** - 用户友好输出，控制台无前缀

---

## 日志宏

### 带前缀日志 (LOG_*)

用于内部调试、追踪程序执行。

| 宏 | 级别 | 用途 |
|----|------|------|
| `LOG_TRACE(...)` | TRACE | 最详细的追踪信息 |
| `LOG_DEBUG(...)` | DEBUG | 调试信息 |
| `LOG_INFO(...)` | INFO | 一般信息 |
| `LOG_WARN(...)` | WARN | 警告信息 |
| `LOG_ERROR(...)` | ERROR | 错误信息 |
| `LOG_CRITICAL(...)` | CRITICAL | 严重错误 |

```cpp
LOG_DEBUG("OnTheFlyGameSolver: initialized with formula: {}", phi->to_string());
LOG_INFO("Phase 1 complete: expanded {} states", expanded_.size());
LOG_ERROR("Failed to open file: {}", filename);
```

**控制台输出**:
```
[12:34:56.789] [debug] OnTheFlyGameSolver: initialized with formula: ...
[12:34:57.123] [info] Phase 1 complete: expanded 42 states
```

**文件输出**:
```
[2026-01-07 12:34:56.789] [debug] [tid] OnTheFlyGameSolver: initialized with formula: ...
[2026-01-07 12:34:57.123] [info] [tid] Phase 1 complete: expanded 42 states
```

### 无前缀日志 (NOP_LOG_*)

用于面向用户的输出。

| 宏 | 用途 |
|----|------|
| `NOP_LOG_INFO(...)` | 用户信息输出 |
| `NOP_LOG_ERROR(...)` | 用户错误输出 |

```cpp
NOP_LOG_INFO("REALIZABLE");
NOP_LOG_INFO("Variables: {} outputs, {} inputs", num_out, num_in);
```

**控制台输出** (无前缀):
```
REALIZABLE
Variables: 3 outputs, 2 inputs
```

---

## 使用原则

### 1. 尽量使用日志宏，避免 cout/cerr

```cpp
// ❌ 避免
std::cout << "Processing formula" << std::endl;
std::cerr << "Error occurred" << std::endl;

// ✅ 推荐
LOG_INFO("Processing formula");
LOG_ERROR("Error occurred");
```

### 2. 用户输出使用 NOP_LOG_*

```cpp
// ✅ 用户友好的结果输出
NOP_LOG_INFO("UNREALIZABLE");
NOP_LOG_ERROR("Cannot parse formula: {}", error_msg);
```

### 3. 特殊情况使用 cerr

```cpp
// ✅ 信号处理器中必须使用 std::cerr
void signal_handler(int signal) {
    std::cerr << "[TIMEOUT] Signal " << signal << std::endl;
    _exit(124);
}

// ✅ 日志系统初始化失败时的兜底
std::cerr << "Logger initialization failed, using stderr" << std::endl;
```

---

## 日志文件

日志文件位置：`logs/formula/YYYY-MM-DD/formula_YYYYMMDD_HHMMSS.log`

- 文件大小限制：5MB
- 最多保留：3 个文件
- 自动滚动

---

## 决策流程

```
需要输出？
    │
    面向用户？→ 调试/追踪？→ 错误？
    │           │           │
NOP_LOG_*   LOG_*       LOG_ERROR / std::cerr*
```

*std::cerr 仅用于信号处理器或日志系统故障
