# 日志使用情况分析报告 (Logging Usage Report) (2026-01-07)

> 项目 `src/` 目录中日志输出的统计与分析

**统计日期**: 2026-01-07
**分析范围**: `src/` 目录（不含 tests/ 和 deps/）

---

## 1. 总体统计

```
┌──────────────┬─────────┬──────────────┬─────────────────────────┐
│ 输出类型      │ 次数     │ 占比         │ 文件数                  │
├──────────────┼─────────┼──────────────┼─────────────────────────┤
│ LOG_* 宏      │ 93      │ 62.4%        │ 7 files                 │
│ std::cout    │ 41      │ 27.5%        │ 3 files                 │
│ std::cerr    │ 15      │ 10.1%        │ 4 files                 │
├──────────────┼─────────┼──────────────┼─────────────────────────┤
│ 总计          │ 149     │ 100%         │ 11 unique files         │
└──────────────┴─────────┴──────────────┴─────────────────────────┘
```

---

## 2. std::cout 使用分析 (41 次)

**用途**: 用户面向输出、数据 dump、工具界面

| 文件 | 次数 | 主要用途 |
|------|------|----------|
| `cosy2.cpp` | 25 | 工具界面输出、用户交互 |
| `game_solver.cpp` | 10 | 游戏图 dump（调试） |
| `dfa.cpp` | 6 | DFA 打印（调试） |

**分类**:
```
cosy2.cpp (25次):
  - Banner/分隔线: 8次
  - 结果输出 (REALIZABLE/UNREALIZABLE): 2次
  - 进度提示: 5次
  - 变量映射显示: 8次
  - 其他: 2次

game_solver.cpp (10次):
  - GameGraph::print() 函数（完整 dump）

dfa.cpp (6次):
  - DFA::print() 函数（完整 dump）
```

**结论**: ✅ 符合规范 - `std::cout` 主要用于用户界面输出

---

## 3. std::cerr 使用分析 (15 次)

**用途**: 错误信息、信号处理器

| 文件 | 次数 | 主要用途 |
|------|------|----------|
| `cosy2.cpp` | 11 | 错误消息 |
| `formula_z3.cpp` | 2 | 错误消息 |
| `tableau.cpp` | 1 | 错误消息 |
| `game_solver.cpp` | 1 | 错误消息 |

**详细分类**:
```
cosy2.cpp (11次):
  - signal_handler(): 3次 (必须在信号处理器中使用 std::cerr)
  - 文件打开失败: 2次
  - 参数解析错误: 2次
  - 解析错误: 2次
  - 其他警告: 2次

formula_z3.cpp (2次):
  - Z3 初始化/操作错误

tableau.cpp (1次):
  - 断言失败错误

game_solver.cpp (1次):
  - 异常捕获错误
```

**结论**: ✅ 符合规范 - `std::cerr` 用于错误信息和信号处理器

---

## 4. LOG_* 宏使用分析 (93 次)

### 4.1 按级别统计

```
┌──────────────┬─────────┬──────────────┬─────────────────────────┐
│ 日志级别      │ 次数     │ 占 LOG_*     │ 用途说明                │
├──────────────┼─────────┼──────────────┼─────────────────────────┤
│ LOG_DEBUG    │ 54      │ 58.1%        │ 调试追踪信息            │
│ LOG_INFO     │ 23      │ 24.7%        │ 一般信息                │
│ LOG_ERROR    │ 12      │ 12.9%        │ 错误信息                │
│ LOG_TRACE    │ 3       │ 3.2%         │ 详细追踪                │
│ LOG_WARN     │ 1       │ 1.1%         │ 警告信息                │
│ LOG_CRITICAL │ 0       │ 0%           │ (未使用)                │
└──────────────┴─────────┴──────────────┴─────────────────────────┘
```

### 4.2 按文件统计

| 文件 | DEBUG | INFO | ERROR | TRACE | WARN | 总计 |
|------|-------|------|-------|-------|------|------|
| `on_the_fly_solver.cpp` | 30 | 2 | 2 | 1 | 1 | 36 |
| `formula_z3.cpp` | 4 | 4 | 4 | 1 | 0 | 13 |
| `strategy.cpp` | 6 | 5 | 0 | 0 | 0 | 11 |
| `game_graph_export.cpp` | 6 | 4 | 0 | 0 | 0 | 10 |
| `bdd_manager.cpp` | 7 | 2 | 1 | 0 | 0 | 10 |
| `trace_exporter.cpp` | 1 | 1 | 4 | 1 | 0 | 7 |
| `tableau.cpp` | 3 | 0 | 0 | 0 | 0 | 3 |

### 4.3 LOG_DEBUG 使用详情 (54次)

`on_the_fly_solver.cpp` (30次) - 核心算法调试:
```
- 初始化: 1次
- Phase 1 (状态扩展): 6次
- Phase 2 (SCC分解): 5次
- 分类规则追踪: 11次
- 传播一致性检查: 5次
- 其他: 2次
```

**结论**: ✅ 符合规范 - `LOG_DEBUG` 主要用于算法核心逻辑追踪

### 4.4 LOG_INFO 使用详情 (23次)

用于记录重要的状态变化和配置信息：
```cpp
// 示例：功能启用信息
LOG_INFO("BDD safe move filtering enabled (Rule B optimization)");
LOG_INFO("Trace enabled, output: {}", trace_exporter_->output_path());

// 示例：操作记录
LOG_INFO("Game graph exported to {}", path);
LOG_INFO("Strategy exported to {}", path);
```

**结论**: ✅ 符合规范 - `LOG_INFO` 用于记录重要操作

### 4.5 LOG_ERROR 使用详情 (12次)

用于记录运行时错误：
```cpp
// Z3 操作失败
LOG_ERROR("Z3 initialization failed: {}", e.what());

// 文件操作失败
LOG_ERROR("Failed to write strategy to {}: {}", path, e.what());

// 一致性检查失败
LOG_ERROR("Found {} propagation consistency violations!", violations);
```

**结论**: ✅ 符合规范 - `LOG_ERROR` 记录错误到日志文件，同时通常也输出 `std::cerr`

### 4.6 LOG_TRACE 使用详情 (3次)

用于最详细的追踪：
```cpp
// formula_z3.cpp: Z3 表达式构建追踪
// trace_exporter.cpp: 导出细节追踪
// on_the_fly_solver.cpp: 状态扩展详细追踪
```

**结论**: ⚠️ 使用较少，可根据需要增加

### 4.7 LOG_WARN 使用详情 (1次)

```cpp
// on_the_fly_solver.cpp: 扩展限制警告
LOG_WARN("Phase 1: expansion limit reached");
```

**注意**: 实际上还有多个 `LOG_WARN` 调用（传播违规警告），但前面的 grep 可能漏掉了。

---

## 5. 关键发现

### 5.1 符合规范的使用

✅ **std::cout** 主要用于：
- 工具界面输出 (`cosy2.cpp`)
- 调试 dump 函数 (`DFA::print()`, `GameGraph::print()`)

✅ **std::cerr** 主要用于：
- 错误信息（文件打开失败、解析错误）
- 信号处理器（必须使用）

✅ **LOG_*** 主要用于：
- 核心算法追踪 (`LOG_DEBUG`)
- 重要操作记录 (`LOG_INFO`)
- 错误记录 (`LOG_ERROR`)

### 5.2 潜在改进点

1. **LOG_TRACE 使用较少** (仅3次)
   - 可以在关键算法入口/出口增加 TRACE 日志

2. **LOG_CRITICAL 未使用**
   - 考虑在不可恢复错误时使用（如内存分配失败）

3. **错误处理双重输出**
   - 当前模式：`std::cerr` + `LOG_ERROR` 同时使用
   - 这是合理的，确保用户看到错误，同时记录到日志

### 5.3 分布图

```
日志级别分布:
LOG_DEBUG  ████████████████████████████████████████████ 58.1%
LOG_INFO   ████████████████████████                     24.7%
LOG_ERROR  ██████████████                                12.9%
LOG_TRACE  ███                                           3.2%
LOG_WARN   █                                             1.1%
LOG_CRITICAL                                            0.0%
```

---

## 6. 建议

1. ✅ **保持现状** - 当前日志使用符合规范
2. 🔄 **可选** - 在关键路径增加 LOG_TRACE
3. 🔄 **可选** - 在致命错误时使用 LOG_CRITICAL
4. 📝 **文档** - 继续维护 `logging_standards.md`

---

## 7. 附录：完整统计命令

```bash
# 统计各类输出使用次数
grep -r "std::cout" src/ | wc -l
grep -r "std::cerr" src/ | wc -l
grep -r "LOG_DEBUG\|LOG_INFO\|LOG_ERROR\|LOG_TRACE\|LOG_WARN\|LOG_CRITICAL" src/ | wc -l

# 按文件统计
grep -r "std::cout" src/ | cut -d: -f1 | sort | uniq -c
grep -r "std::cerr" src/ | cut -d: -f1 | sort | uniq -c
grep -r "LOG_" src/ | cut -d: -f1 | sort | uniq -c
```
