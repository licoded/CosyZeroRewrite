# 改进建议优先级排序

**更新日期**: 2026-01-05

---

## 第一批：立即修复 (Critical/High)

这些是影响代码正确性或可维护性的关键问题，应优先处理。

### 1. 重命名 `create_end()` → `create_end_marker()`
- **优先级**: Critical
- **文件**: `include/automata/dfa.hpp:230-245`
- **工作量**: 5 分钟
- **影响**: 需要更新所有调用点
- **价值**: 提高代码可读性，避免误解

### 2. 明确标注 `is_realizable()` 为占位符
- **优先级**: High
- **文件**: `src/synthesis/synthesis.cpp:76-85`
- **工作量**: 5 分钟
- **影响**: 添加注释/属性
- **价值**: 避免使用者误以为功能已实现

```cpp
// TODO: NOT IMPLEMENTED - Returns std::nullopt until GameSolver integration
[[nodiscard]] std::optional<bool> is_realizable(formula::Formula* f);
```

### 3. 修复 Formula 访问器的 const 正确性
- **优先级**: High
- **文件**: `include/formula/formula.hpp:55-57`
- **工作量**: 30 分钟
- **影响**: 需要更新所有非 const 使用
- **价值**: 强制执行不可变性保证

```cpp
const Formula* left() const { return left_; }
const Formula* right() const { return right_; }
```

---

## 第二批：计划修复 (Medium)

这些是改进代码质量的重要项目，但不影响当前功能。

### 4. 重命名 `rmnext()` → `progression()`
- **优先级**: Medium
- **文件**: `include/formula/formula.hpp:157`
- **工作量**: 15 分钟
- **价值**: 使用 LTLf 文献标准术语

### 5. 重命名 `xnf_with_tail` → `xnf_with_end_marker()`
- **优先级**: Medium
- **文件**: `include/formula/formula.hpp:143`
- **工作量**: 10 分钟
- **价值**: 使用更准确的术语

### 6. 提取 `read_benchmark()` 中的重复代码
- **优先级**: Medium
- **文件**: `src/synthesis/synthesis.cpp:96-149`
- **工作量**: 30 分钟
- **价值**: 减少约 30 行重复代码

### 7. 提取 `to_string()` 和 `to_string_with_names()` 的公共逻辑
- **优先级**: Medium
- **文件**: `src/formula/formula.cpp:115-244`
- **工作量**: 45 分钟
- **价值**: 减少重复，更易维护

### 8. 优化 `get_precedence()` 使用查找表
- **优先级**: Medium
- **文件**: `src/formula/formula.cpp:31-51`
- **工作量**: 15 分钟
- **价值**: O(1) 查找而非 O(n)

```cpp
static constexpr int PRECEDENCE[] = {0, 4, 2, 1, 3, 3, 4, 5, 5, 5};
```

---

## 第三批：优化改进 (Low)

这些是锦上添花的改进，可以在有时间时处理。

### 9. 创建通用的 `hash_combine` 工具函数
- **优先级**: Low
- **文件**: `include/automata/dfa.hpp`
- **工作量**: 20 分钟
- **价值**: 代码复用，添加注释

### 10. 使用类型别名简化返回类型
- **优先级**: Low
- **文件**: `include/automata/dfa.hpp:67-68`
- **工作量**: 10 分钟
- **价值**: 提高可读性

### 11. 增强 Parser 错误信息（添加位置）
- **优先级**: Low → Medium（如果经常调试复杂公式）
- **文件**: `include/formula/formula_parser.hpp`
- **工作量**: 2-3 小时
- **价值**: 更好的调试体验

### 12. 检查并修复文件 I/O 错误处理
- **优先级**: Medium
- **文件**: `src/synthesis/synthesis.cpp`
- **工作量**: 30 分钟
- **价值**: 更健壮的代码

---

## 修复顺序建议

### Week 1: 命名和文档
1. `create_end()` → `create_end_marker()`
2. `is_realizable()` 添加占位符注释
3. `rmnext()` → `progression()`
4. `xnf_with_tail` → `xnf_with_end_marker()`

### Week 2: 代码重构
5. Formula 访问器 const 正确性
6. 提取 `read_benchmark()` 重复代码
7. 提取 `to_string()` 重复代码

### Week 3: 优化
8. `get_precedence()` 查找表
9. `hash_combine` 工具函数
10. 文件 I/O 错误处理

### As Needed: 增强
11. Parser 位置信息
12. 类型别名

---

## 工作量估算

| 批次 | 项目数 | 总工作量 | 建议时间 |
|------|--------|----------|----------|
| 第一批 | 3 | ~40 分钟 | 1 天 |
| 第二批 | 5 | ~2 小时 | 1 周 |
| 第三批 | 4 | ~3.5 小时 | 2 周 |

---

## 跟踪进度

| # | 任务 | 状态 | 负责人 | 完成日期 |
|---|------|------|--------|----------|
| 1 | `create_end()` → `create_end_marker()` | ⬜ 待办 | | |
| 2 | `is_realizable()` 占位符标注 | ⬜ 待办 | | |
| 3 | Formula 访问器 const 正确性 | ⬜ 待办 | | |
| 4 | `rmnext()` → `progression()` | ⬜ 待办 | | |
| 5 | `xnf_with_tail` → `xnf_with_end_marker()` | ⬜ 待办 | | |
| 6 | 提取 `read_benchmark()` 重复 | ⬜ 待办 | | |
| 7 | 提取 `to_string()` 重复 | ⬜ 待办 | | |
| 8 | `get_precedence()` 查找表 | ⬜ 待办 | | |
| 9 | `hash_combine` 工具函数 | ⬜ 待办 | | |
| 10 | 类型别名 | ⬜ 待办 | | |
| 11 | Parser 位置信息 | ⬜ 待办 | | |
| 12 | 文件 I/O 错误处理 | ⬜ 待办 | | |

---

## 备注

- 所有修改都应该通过完整的测试套件
- 建议每完成一个任务就提交一次 Git
- 命名变更会影响公共 API，需要仔细检查调用点
- 优先级可能根据项目需求调整
