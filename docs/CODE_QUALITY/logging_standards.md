# 日志规范 (Logging Standards) (2026-01-07)

> 何时使用 `std::cout`/`std::cerr` vs `LOG_*` 宏

---

## 核心原则

不同的输出目的应该使用不同的输出方式：

```
┌─────────────────────────────────────────────────────────────┐
│                      输出类型分类                            │
├─────────────┬───────────────┬─────────────┬────────────────┤
│ 程序输出     │ 用户界面/结果  │ std::cout   │ 面向最终用户    │
├─────────────┼───────────────┼─────────────┼────────────────┤
│ 错误输出     │ 错误信息      │ std::cerr   │ 必须保证可见性  │
├─────────────┼───────────────┼─────────────┼────────────────┤
│ 日志输出     │ 调试/追踪     │ LOG_* 宏    │ 带时间戳和级别  │
└─────────────┴───────────────┴─────────────┴────────────────┘
```

---

## 1. 程序输出 (Program Output) → `std::cout`

**用途**: 面向最终用户的输出，不应该有日志前缀或时间戳。

**使用场景**:
- 命令行工具的结果输出（如 `REALIZABLE` / `UNREALIZABLE`）
- 用户交互信息（如 banner、进度提示）
- 数据 dump（如 `DFA::print()` 用于调试查看状态）

**示例**:
```cpp
// ✅ 正确：用户看到的结果
std::cout << "REALIZABLE" << std::endl;

// ✅ 正确：工具 banner
std::cout << "========================================" << std::endl;
std::cout << "   CosyZero LTLf Synthesis Tool v2.0" << std::endl;

// ✅ 正确：数据 dump（调试用途，不需要日志前缀）
void DFA::print(const FormulaPool& pool) const {
    std::cout << "DFA with " << num_states() << " states\n";
    // ...
}
```

**❌ 不要用**:
```cpp
// ❌ 错误：用户输出不应该有日志前缀
LOG_INFO("REALIZABLE");  // 输出: [12:34:56.789] [info] REALIZABLE
```

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
         ┌───────────────┼───────────────┐
         │               │               │
    面向最终用户？    是错误信息？   调试/追踪？
         │               │               │
    std::cout        std::cerr        LOG_* 宏
    (不含前缀)       (保证可见)      (带时间戳)
```

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
LOG_DEBUG("classify_scc: state=", s.to_string());
// 运行时设置环境变量：SPDLOG_LEVEL=DEBUG
```

### 6.2 同时输出到用户和日志

```cpp
// 输出给用户
std::cout << "Formula: " << formula_str << std::endl;

// 同时记录到日志
LOG_INFO("Processing formula: ", formula_str);
```

---

## 7. 当前问题清单

| 文件 | 行号 | 问题 | 修复方案 |
|------|------|------|----------|
| `src/synthesis/on_the_fly_solver.cpp` | 673 | 条件调试用 `std::cerr` | 改为 `LOG_DEBUG` |

---

## 8. 代码审查检查清单

- [ ] 用户面向输出用 `std::cout`，不带日志前缀
- [ ] 错误信息用 `std::cerr`
- [ ] 信号处理器只用 `std::cerr` 和 `_exit()`
- [ ] 调试/追踪信息用 `LOG_*` 宏
- [ ] 不在信号处理器中使用 spdlog
- [ ] 不在用户输出中使用 `LOG_*` 宏
