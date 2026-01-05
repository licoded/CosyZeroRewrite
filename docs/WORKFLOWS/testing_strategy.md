# 测试策略

> 测试分层、覆盖率目标和回归测试规范

---

## 1. 测试分层

```
┌─────────────────────────────────────────────┐
│   集成测试 (E2E)                              │
│   benchmark_runner, Cosy2                   │  ← 端到端验证
├─────────────────────────────────────────────┤
│   模块测试 (组件级)                           │
│   synthesis_tests, on_the_fly_synthesis_tests│  ← 组件交互
├─────────────────────────────────────────────┤
│   单元测试 (函数/类级)                        │
│   formula_tests, parser_tests, dfa_tests     │  ← 独立功能
├─────────────────────────────────────────────┤
│   随机测试 (属性/模糊)                        │
│   random_formula_test                       │  ← 边界情况
└─────────────────────────────────────────────┘
```

---

## 2. 测试套件

| 测试套件 | 覆盖范围 | 断言数 | 测试用例 |
|---------|---------|-------|---------|
| formula_tests | Formula 类 API | 66 | 31 |
| parser_checker_tests | Parser + 等价性检查 | 171 | 36 |
| transformation_tests | NNF/XNF/Simplify | 4 | 4 (196 公式) |
| dfa_tests | DFA 基础功能 | 22 | 8 |
| tableau_state_tests | Tableau 状态逻辑 | 36 | 29 |
| synthesis_tests | Synthesis 算法 | 20 | 7/8 |
| on_the_fly_synthesis_tests | On-the-fly 算法 | - | - |
| random_formula_test | 随机公式模糊测试 | 50000 | 10000 |

---

## 3. 覆盖率目标

| 模块 | 目标 | 当前 | 状态 |
|------|------|------|------|
| Formula | 90% | ~95% | ✅ |
| Parser | 85% | ~90% | ✅ |
| Transformations | 90% | ~95% | ✅ |
| DFA/Tableau | 80% | ~85% | ✅ |
| Synthesis | 70% | ~65% | 🟡 |

---

## 4. 回归测试规范

### 4.1 Bug 修复回归测试

修复 Bug 后必须添加回归测试：

```cpp
// 标签格式
TEST_CASE("Module: Bug #XXX - Brief description", "[tag1][regression][bugXXX]") {
    // 测试代码
}
```

**标签规范**:
- `[regression]`: 回归测试标记
- `[bugXXX]`: 关联 Bug 编号
- `[模块名]`: 所属模块

### 4.2 回归测试示例

```cpp
TEST_CASE("Parser: Bug #002 - Variables starting with 'r'", "[parser][regression][bug002]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    // Before fix: would fail (parsed as Release + "eq")
    // After fix: passes (parsed as identifier "req")
    Formula* f = parser.parse("req");
    REQUIRE_FALSE(parser.has_error());
    REQUIRE(f->is_literal());
    REQUIRE(pool.has_variable("req"));
}
```

### 4.3 回归测试验证

```bash
# 1. 修复前先写测试，确保失败
make test
# Expected: FAIL

# 2. 修复后运行测试，确保通过
make test
# Expected: PASS

# 3. 运行所有测试，确保无回归
make test
# Expected: All PASS
```

---

## 5. 随机测试

### 5.1 Fuzzing 策略

```cpp
// 随机生成 10000 个公式，验证：
for (int i = 0; i < 10000; i++) {
    Formula* f = generate_random_formula();

    // Parser roundtrip
    REQUIRE(roundtrip(f));

    // NNF 等价性
    REQUIRE(equivalent(f, f->nnf(pool)));

    // XNF 等价性
    REQUIRE(equivalent(f, f->xnf(pool)));

    // Simplify 等价性
    REQUIRE(equivalent(f, f->simplify(pool)));

    // NNF 幂等性
    REQUIRE(equivalent(f->nnf(pool), f->nnf(pool)->nnf(pool)));
}
```

### 5.2 属性测试

| 属性 | 描述 | 验证方法 |
|------|------|---------|
| 等价性 | 转换保持语义 | Z3 BMC |
| 幂等性 | 两次转换结果相同 | 结构相等 |
| 规范化 | NNF/XNF 满足定义 | 静态检查 |

---

## 6. 运行测试

```bash
# 编译
cd build && cmake .. && make

# 运行所有测试
make test

# 运行单个测试套件
./formula_tests
./parser_checker_tests

# 运行带标签的测试
./parser_checker_tests "[regression]"

# 详细输出
./formula_tests -s

# 随机测试
./random_formula_test
```

---

## 7. 代码修改后的测试流程

**重要**: 修改代码后必须按以下顺序进行测试：

### 7.1 测试顺序

1. **单元测试/自定义测试** - 先验证基本功能
   ```bash
   make test  # 运行所有单元测试
   ```
   - 确保所有单元测试通过
   - 确保没有引入回归问题

2. **小范围抽查** (20 个随机案例)
   ```bash
   ./build/tests/bench/benchmark_test tools/benchmarks/sm1000 1 20
   ```
   - 验证基本逻辑在大范围内的稳定性
   - 发现问题立即修复，不要继续

3. **全量 Benchmark** (1000 个案例)
   ```bash
   ./build/benchmark_runner benchmarks/sm1000 1 1000
   ```
   - 只有在前面阶段通过后才运行
   - 避免浪费时间在明显有问题的代码上

### 7.2 重要提醒

- ❌ **不要**修改代码后直接跑全量 benchmark
- ❌ **不要**跳过单元测试直接跑 benchmark
- ✅ **必须**按顺序：单元测试 → 小范围抽查(20) → 全量测试

---

## 8. 测试编写指南

### 8.1 单元测试模板

```cpp
TEST_CASE("Feature: Brief description", "[module]") {
    FormulaPool pool;

    SECTION("case 1") {
        // Arrange
        Formula* input = parser.parse("formula");

        // Act
        Formula* result = operation(input);

        // Assert
        REQUIRE(result != nullptr);
        REQUIRE(result->is_expected_type());
    }

    SECTION("case 2") {
        // Another test case
    }
}
```

### 8.2 测试命名

- `TEST_CASE("Parser: Simple literals", "[parser]")` - 功能描述
- `TEST_CASE("Formula: Bug #001 - Hash collision", "[formula][regression]")` - Bug 修复
- `TEST_CASE("Property: NNF preserves semantics", "[property][nnf]")` - 属性测试
