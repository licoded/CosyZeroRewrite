# Logging Standards

> 何时使用 `std::cout`/`std::cerr` vs `LOG_*` 宏

---

## 核心原则

| 输出类型 | 用途 | 方式 |
|---------|------|------|
| 程序输出 | 用户界面/结果 | `LOG_OUTPUT` |
| 错误输出 | 错误信息 | `std::cerr` |
| 日志输出 | 调试/追踪 | `LOG_*` 宏 |
| 测试输出 | 格式化+颜色 | `fmt + termcolor` |

---

## 1. 程序输出 → `LOG_OUTPUT`

**用途**: 面向最终用户的输出，控制台无前缀，文件带时间戳

```cpp
// ✅ 正确
LOG_OUTPUT("REALIZABLE");
LOG_OUTPUT("Formula: {}", formula_str);
LOG_OUTPUT("Variables: {} outputs, {} inputs", num_out, num_in);
```

**控制台输出** (无前缀):
```
REALIZABLE
Formula: p0 && p1
```

**日志文件输出** (带时间戳):
```
[2026-01-07 12:34:56.789] [info] REALIZABLE
```

---

## 2. 错误输出 → `std::cerr`

**用途**: 错误和警告信息，必须保证可见性

```cpp
// ✅ 正确：文件打开失败
std::cerr << "Error: Cannot open file: " << filename << std::endl;

// ✅ 正确：信号处理器（必须使用 std::cerr）
void signal_handler(int signal) {
    std::cerr << "[TIMEOUT] Received signal " << signal << std::endl;
    std::cerr << std::flush;
    _exit(124);
}
```

**⚠️ 重要**: 信号处理器中**必须使用 `std::cerr` 和 `_exit()`**，不能使用 spdlog！

---

## 3. 日志输出 → `LOG_*` 宏

**用途**: 追踪程序执行、调试信息

| 宏 | 级别 | 用途 |
|----|------|------|
| `LOG_TRACE(...)` | TRACE | 最详细的追踪信息 |
| `LOG_DEBUG(...)` | DEBUG | 调试信息 |
| `LOG_INFO(...)` | INFO | 一般信息 |
| `LOG_WARN(...)` | WARN | 警告信息 |
| `LOG_ERROR(...)` | ERROR | 错误信息 |
| `LOG_CRITICAL(...)` | CRITICAL | 严重错误 |

```cpp
// ✅ 正确：函数追踪
LOG_DEBUG("OnTheFlyGameSolver: initialized with formula: {}", phi->to_string());

// ✅ 正确：重要状态变化
LOG_INFO("Phase 1 complete: expanded {} states", expanded_.size());
```

---

## 4. 测试输出 → `fmt + termcolor`

**用途**: benchmark 结果输出（颜色高亮）

```cpp
// fmt + termcolor 组合
fmt_print_with_color(termcolor::green, "OK: bench{}/f{} ({}ms)\n", bench_dir, n, ms);
fmt_print_with_color(termcolor::red, "FAIL: bench{}/f{} ({})\n", bench_dir, n, err);

// 无颜色格式化
fmt::print("Wall time: {:.2f}ms\n", elapsed_ms);
```

---

## 决策流程

```
需要输出信息？
    │
    面向用户？→ 是错误？→ 调试/追踪？→ 测试/benchmark？
    │           │           │             │
LOG_OUTPUT   std::cerr    LOG_* 宏    fmt + termcolor
```
