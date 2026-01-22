# Testing Guide

> 测试策略、覆盖率目标和 benchmark 使用

---

## 测试分层

```
┌─────────────────────────────────────────────┐
│   集成测试 (E2E)                              │
│   benchmark_runner, Cosy2                   │
├─────────────────────────────────────────────┤
│   模块测试 (组件级)                           │
│   synthesis_tests, on_the_fly_synthesis_tests│
├─────────────────────────────────────────────┤
│   单元测试 (函数/类级)                        │
│   formula_tests, parser_tests, dfa_tests     │
└─────────────────────────────────────────────┘
```

---

## 测试执行顺序

**重要**: 修改代码后必须按以下顺序测试

### 1. 单元测试

```bash
make test
# 或
ctest --test-dir build
```

### 2. 小范围抽查 (20 个案例)

```bash
./build/tests/bench/benchmark_test -d tools/benchmarks/sm1000 -s 1 -e 20 -j 1
```

### 3. 全量 Benchmark (1000 个案例)

```bash
./build/tests/bench/benchmark_test -d tools/benchmarks/sm1000
```

**❌ 不要**跳过单元测试直接跑 benchmark
**✅ 必须**按顺序：单元测试 → 小范围抽查 → 全量测试

---

## Benchmark 使用

### SMv2 数据集结构

```
tools/benchmarks/sm1000/
├── bench1/           # 前 500 个公式
│   ├── f1.ltlf       # 公式文件
│   ├── f1.part       # 变量分区文件
│   └── ...
└── bench2/           # 后 500 个公式
```

### 运行命令

```bash
# 全量测试
./build/tests/bench/benchmark_test -d tools/benchmarks/sm1000

# 范围测试 (f1-f20)
./build/tests/bench/benchmark_test -d tools/benchmarks/sm1000 -s 1 -e 20

# 单目录测试
./build/tests/bench/benchmark_test -d tools/benchmarks/sm1000 -b 1
```

### .part 文件格式

```
.inputs: p1 p3
.outputs: p5 p7
```

**规则**：
- `.inputs:` 必须在 `.outputs:` 之前
- 空 inputs 必须写为 `.inputs: `（空的）
- 公式中出现但未在分区中的变量视为 outputs

---

## 回归测试规范

Bug 修复后必须添加回归测试：

```cpp
TEST_CASE("Parser: Bug #002 - Variables starting with 'r'", "[regression][bug002]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f = parser.parse("req");
    REQUIRE_FALSE(parser.has_error());
    REQUIRE(f->is_literal());
    REQUIRE(pool.has_variable("req"));
}
```

**标签规范**：
- `[regression]` - 回归测试标记
- `[bugXXX]` - 关联 Bug 编号
- `[模块名]` - 所属模块
