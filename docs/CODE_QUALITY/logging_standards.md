# 日志规范 (Logging Standards) (2026-01-07)

> 何时使用 `std::cout`/`std::cerr` vs `LOG_*` 宏

**相关文档**: [日志使用情况分析报告](logging_usage_report.md)

---

## 核心原则

不同的输出目的应该使用不同的输出方式：

```
┌─────────────────────────────────────────────────────────────┐
│                      输出类型分类                            │
├─────────────┬───────────────┬─────────────┬────────────────┤
│ 程序输出     │ 用户界面/结果  │ LOG_OUTPUT  │ 面向最终用户    │
├─────────────┼───────────────┼─────────────┼────────────────┤
│ 错误输出     │ 错误信息      │ std::cerr   │ 必须保证可见性  │
├─────────────┼───────────────┼─────────────┼────────────────┤
│ 日志输出     │ 调试/追踪     │ LOG_* 宏    │ 带时间戳和级别  │
├─────────────┼───────────────┼─────────────┼────────────────┤
│ 测试/工具    │ 格式化+颜色   │ fmt+termcol │ 测试/benchmark  │
└─────────────┴───────────────┴─────────────┴────────────────┘
```

**重要变更 (2026-01-07)**:
- src/ 目录已统一使用 `LOG_OUTPUT` 替代 `std::cout`
- tests/bench/ 使用 `fmt + termcolor` 提高可读性

---

## 1. 程序输出 (Program Output) → `LOG_OUTPUT`

**用途**: 面向最终用户的输出，控制台显示不带前缀，同时记录到日志文件（带时间戳）。

**使用场景**:
- 命令行工具的结果输出（如 `REALIZABLE` / `UNREALIZABLE`）
- 用户交互信息（如 banner、进度提示）
- 工具的主要输出信息

**实现**: `LOG_OUTPUT` 使用单独的 spdlog logger，控制台 sink 无前缀，文件 sink 保留时间戳。

**示例**:
```cpp
// ✅ 正确：用户看到的结果
LOG_OUTPUT("REALIZABLE");

// ✅ 正确：工具 banner
LOG_OUTPUT("========================================");
LOG_OUTPUT("   CosyZero LTLf Synthesis Tool v2.0");
LOG_OUTPUT("========================================");

// ✅ 正确：使用 fmt 风格的占位符
LOG_OUTPUT("Formula: {}", formula_str);
LOG_OUTPUT("Variables declared: {} outputs, {} inputs", num_out, num_in);
```

**控制台输出** (无前缀):
```
========================================
   CosyZero LTLf Synthesis Tool v2.0
========================================
Formula: p0 && p1
REALIZABLE
```

**日志文件输出** (带时间戳，方便调试):
```
[2026-01-07 12:34:56.789] [info] [12345] ========================================
[2026-01-07 12:34:56.790] [info] [12345]    CosyZero LTLf Synthesis Tool v2.0
[2026-01-07 12:34:56.791] [info] [12345] Formula: p0 && p1
[2026-01-07 12:34:57.123] [info] [12345] REALIZABLE
```

### 1.1 数据 Dump 函数

对于调试用的数据 dump（如 `DFA::print()`、`GameSolver::print()`），可以继续使用 `std::cout`：
```cpp
// ✅ 可接受：调试 dump 函数
void DFA::print(const FormulaPool& pool) const {
    std::cout << "DFA with " << num_states() << " states\n";
    // ...
}
```

原因：这些函数通常用于交互式调试，不需要记录到日志文件。

---

## 2. 错误输出 (Error Output) → `std::cerr`

**用途**: 错误和警告信息，必须保证在任何情况下都能输出。

**使用场景**:
- 文件打开失败
- 参数解析错误
- **信号处理器**（signal handler）中的输出

**⚠️ 重要**: 信号处理器中**必须使用 `std::cerr` 和 `_exit()`**，不能使用 spdlog！

**示例**:
```cpp
// ✅ 正确：文件打开失败
std::cerr << "Error: Cannot open file: " << filename << std::endl;

// ✅ 正确：信号处理器（必须使用 std::cerr）
void signal_handler(int signal) {
    std::cerr << "\n[TIMEOUT] Received signal " << signal_name << std::endl;
    std::cerr << std::flush;
    _exit(124);  // 不能用 exit()，因为 spdlog 可能在清理时死锁
}
```

**❌ 不要用**:
```cpp
// ❌ 错误：信号处理器中不能使用 spdlog（可能死锁）
void signal_handler(int signal) {
    LOG_CRITICAL("Received signal {}", signal);  // 危险！
    _exit(124);
}
```

---

## 3. 日志输出 (Logging) → `LOG_*` 宏

**用途**: 追踪程序执行、调试信息、需要记录到文件的日志。

**使用场景**:
- 函数入口/出口追踪
- 重要状态变化
- 调试信息（由日志级别控制）
- 需要长期记录的诊断信息

**可用的日志级别**:
| 宏 | 级别 | 用途 |
|----|------|------|
| `LOG_TRACE(...)` | TRACE | 最详细的追踪信息 |
| `LOG_DEBUG(...)` | DEBUG | 调试信息 |
| `LOG_INFO(...)` | INFO | 一般信息 |
| `LOG_WARN(...)` | WARN | 警告信息 |
| `LOG_ERROR(...)` | ERROR | 错误信息 |
| `LOG_CRITICAL(...)` | CRITICAL | 严重错误 |

**示例**:
```cpp
// ✅ 正确：函数追踪
LOG_DEBUG("OnTheFlyGameSolver: initialized with formula: ", phi->to_string());

// ✅ 正确：重要状态变化
LOG_DEBUG("Phase 1 complete: expanded ", expanded_.size(), " states");

// ✅ 正确：错误记录（同时也要输出到 std::cerr）
LOG_ERROR("BDD manager initialization failed: ", e.what());
```

**❌ 不要用**:
```cpp
// ❌ 错误：调试输出不应该用 std::cerr
if (debug_classify) {
    std::cerr << "DEBUG classify_scc: state=" << s.to_string() << std::endl;
}
// 应该改为：
if (debug_classify) {
    LOG_DEBUG("classify_scc: state=", s.to_string());
}
```

---

## 4. spdlog 配置

**日志输出位置**:
- **控制台**: INFO 级别，彩色输出，格式：`[HH:MM:SS.mmm] [level] message`
- **文件**: TRACE 级别（最详细），格式：`[YYYY-MM-DD HH:MM:SS.mmm] [level] [thread] message`

**日志文件位置**: `logs/formula/YYYY-MM-DD/period/formula_YYYYMMDD_HHMMSS.log`

**初始化**:
```cpp
// 在 main() 开始时调用
logger::Logger::initialize();
```

---

## 5. 决策流程图

```
                    需要输出信息？
                         │
         ┌───────────────┼───────────────┬─────────────┐
         │               │               │             │
    面向最终用户？    是错误信息？   调试/追踪？   测试/benchmark？
         │               │               │             │
    LOG_OUTPUT       std::cerr        LOG_* 宏    fmt + termcolor
    (不含前缀)       (保证可见)      (带时间戳)   (格式化+颜色)
```

**说明**:
- `LOG_OUTPUT` - 用户面向输出（如 "REALIZABLE"），控制台无前缀，文件有记录
- `std::cerr` - 错误信息，保证可见性
- `LOG_*` - 内部日志，带时间戳和级别
- `fmt + termcolor` - 测试工具需要格式化和颜色高亮时使用

---

## 6. 特殊场景

### 6.1 条件调试输出

**❌ 旧方式**（不推荐）:
```cpp
static const bool debug_classify = (std::getenv("COSY_DEBUG_CLASSIFY") != nullptr);
if (debug_classify) {
    std::cerr << "DEBUG: " << message << std::endl;
}
```

**✅ 新方式**（推荐）:
```cpp
// 使用 LOG_DEBUG，通过设置日志级别控制
LOG_DEBUG("classify_scc: state={}", s.to_string());
// 运行时设置环境变量：SPDLOG_LEVEL=DEBUG
```

### 6.2 LOG_OUTPUT 的优势

使用 `LOG_OUTPUT` 而不是 `std::cout` 的好处：
1. **统一基础设施** - 所有输出都通过 spdlog，便于管理
2. **自动记录到文件** - 用户输出也会被记录，方便调试
3. **线程安全** - spdlog 内部有锁保证
4. **可配置** - 可以轻松重定向输出

---

## 7. 当前问题清单

| 文件 | 行号 | 问题 | 状态 |
|------|------|------|------|
| `src/synthesis/on_the_fly_solver.cpp` | 670 | 已修复：条件调试用 `std::cerr` | ✅ 已改为 `LOG_DEBUG` |
| `src/cosy2.cpp` | - | 已修复：使用 `std::cout` | ✅ 已改为 `LOG_OUTPUT` |
| `src/automata/dfa.cpp` | - | 已修复：`DFA::print()` 使用 `std::cout` | ✅ 已改为 `LOG_OUTPUT` |
| `src/synthesis/game_solver.cpp` | - | 已修复：`GameGraph::print()` 使用 `std::cout` | ✅ 已改为 `LOG_OUTPUT` |

**src/ 目录状态**: ✅ 已完全移除 `std::cout`，全部使用 `LOG_OUTPUT`

---

## 8. 测试/工具输出 → `fmt + termcolor` (2026-01-07)

**用途**: 测试程序和 benchmark 工具中需要格式化输出和颜色高亮的场景。

**使用场景**:
- Benchmark 结果输出（OK 绿色，FAIL 红色）
- 进度条和统计信息
- 需要格式化浮点数（如 `{:.2f}`）的输出

**为什么用 fmt 而不是 std::cout**:
1. **类型安全的格式化**: `fmt::print("{:.2f}", value)` vs `std::cout << std::fixed << std::setprecision(2)`
2. **简洁的占位符语法**: `fmt::print("Parsed: {}\n", count)` vs `std::cout << "Parsed: " << count << std::endl`
3. **可读性更高**: 格式字符串一目了然，不需要追踪 `<<` 链

**辅助函数** (tests/bench/benchmark.cpp):
```cpp
// fmt_print_with_color: fmt formatting + termcolor in one call
template<typename... Args>
static void fmt_print_with_color(std::ostream& (*color)(std::ostream&),
                                 fmt::format_string<Args...> fmt_str,
                                 Args&&... args) {
    std::cout << color;
    fmt::print(fmt_str, std::forward<Args>(args)...);
    std::cout << termcolor::reset;
}
```

**示例用法**:
```cpp
// ✅ 推荐：使用辅助函数，一行搞定
fmt_print_with_color(termcolor::green, "OK: bench{}/f{} ({}ms)\n",
                     bench_dir, formula_num, elapsed_ms);
fmt_print_with_color(termcolor::red, "FAIL: bench{}/f{} ({})\n",
                     bench_dir, formula_num, error_msg);

// ✅ 正确：无颜色格式化输出
fmt::print("Wall time: {:.2f}ms\n", elapsed_ms);
fmt::print("Speedup: {:.2f}x\n", total_time_ms / elapsed_ms);
```

**可用颜色**:
```cpp
termcolor::green     // 成功/OK
termcolor::red       // 失败/ERROR
termcolor::yellow    // 警告
termcolor::blue      // 信息
termcolor::reset     // 重置为默认颜色
```

---

## 9. 代码审查检查清单

- [ ] 用户面向输出用 `LOG_OUTPUT`（不含前缀，但记录到日志文件）
- [ ] 错误信息用 `std::cerr`
- [ ] 信号处理器只用 `std::cerr` 和 `_exit()`
- [ ] 调试/追踪信息用 `LOG_*` 宏
- [ ] 不在信号处理器中使用 spdlog
- [ ] 不在用户输出中使用 `LOG_INFO` 等带前缀的宏
- [ ] 测试/benchmark 中需要颜色时，用 `std::cout << termcolor::X` + `fmt::print(...)`

---

## 附录 A: 为什么宏用 `do { ... } while(0)` 包裹？

`LOG_*` 宏使用 `do { ... } while(0)` 包裹，这是 C/C++ 宏定义的标准最佳实践：

```cpp
#define LOG_DEBUG(...) do { \
    if (auto lg = logger::Logger::instance().get()) lg->debug(__VA_ARGS__); \
} while(0)
```

### 原因

1. **使宏像函数一样安全使用** - 可以在 `if-else` 语句中使用，不会破坏配对
2. **必须加分号** - 调用后 `;` 成为 `while(0);` 的一部分，符合函数调用习惯
3. **只执行一次** - `while(0)` 保证循环体只执行一次

### 对比

```cpp
// ❌ 错误写法：破坏 if-else 配对
#define BAD_LOG(msg) if (auto lg = get()) lg->debug(msg)

if (error)
    BAD_LOG("error");   // 展开后有个 if，会吃掉下面的 else！
else
    BAD_LOG("fatal");

// ✅ 正确写法：do-while(0)
#define GOOD_LOG(msg) do { \
    if (auto lg = get()) lg->debug(msg); \
} while(0)

if (error)
    GOOD_LOG("error");   // 展开：do { ... } while(0);
else
    GOOD_LOG("fatal");   // ✅ else 正确匹配
```

这是 Linux 内核和许多大型项目中的标准写法。

