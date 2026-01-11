# CosyZeroRewrite 项目说明

> LTLf Synthesis 的 C++17 实现

---

## 项目概述

基于 `/home/lic/files/rewrite_ltlf_codes/CosyZeroRewrite/migrationDocs/formula_redesign/` 目录下的设计文档，使用 **C++17** 和 **CMake** 实现的 LTLf Synthesis 模块重写。

**核心特性**:
- 不可变公式表示 (Hash Consing)
- Tableau-based DFA 构造
- On-the-Fly 游戏求解
- 完整的测试覆盖

---

## 快速链接

| 内容 | 位置 |
|------|------|
| **架构文档** | `docs/ARCHITECTURE/` |
| **工作流程** | `docs/WORKFLOWS/` |
| **待办事项** | `docs/TODO/` (按模块分类) |
| **已知 Bug** | `docs/BUGS/open.md` |
| **已修复 Bug** | `docs/BUGS/fixed.md` |
| **设计文档** | `migrationDocs/formula_redesign/` |
| **示例公式** | `examples/` |

---

## 技术栈

| 组件 | 选择 |
|------|------|
| 语言 | C++17 |
| 构建 | CMake 3.10+ |
| 测试 | Catch2 |
| SMT 求解器 | Z3 |
| 日志 | spdlog (内嵌) |

详见: [外部依赖](./docs/ARCHITECTURE/dependencies.md)

---

## 当前状态

### 实现进度

```
✅ Formula 模块 (100%)
✅ Parser 模块  (100%)
✅ DFA/Tableau  (100%)
✅ Synthesis 核心 (85%)
🔄 策略提取      (30%)
```

详见: [实现路线图](./docs/ARCHITECTURE/roadmap.md)

### 测试覆盖

| 测试套件 | 断言数 | 状态 |
|---------|-------|------|
| formula_tests | 66 | ✅ 全部通过 |
| parser_checker_tests | 171 | ✅ 全部通过 |
| transformation_tests | 4 (196 公式) | ✅ 全部通过 |
| dfa_tests | 22 | ✅ 全部通过 |
| tableau_state_tests | 36 | ✅ 全部通过 |
| synthesis_tests | 20 | ✅ 全部通过 |
| random_formula_test | 50000 (10000 公式) | ✅ 全部通过 |

详见: [测试策略](./docs/WORKFLOWS/testing_strategy.md)

---

## 工作流程

### Bug 修复流程

**重要：修复大 Bug 时的 Git 提交规范**

在修复复杂的 bug 时，每完成一个小阶段/进展后必须立即提交 git：
1. 每个小功能修复完成后立即 `git add` + `git commit`
2. 提交信息应清晰描述该阶段做了什么
3. 这样便于：
   - 出问题时快速回滚
   - 代码审查时清楚看到每个步骤
   - 理解问题解决的完整过程

### 复杂问题协作 (working_issues)

当遇到复杂 bug 需要深入分析时：

1. **创建问题文档**: 在 `docs/working_issues/` 下创建详细记录
   - 目录命名: `YYYY-MM-DD_AM/PM_ProblemSummary/`
   - 包含 `BUG_REPORT.md` 详细描述问题和进展

2. **及时同步进展**: 在问题文档中实时更新修复进度

3. **协作讨论**: 完成初步分析后与用户讨论解决方案

详见: [Bug 报告模板](./docs/working_issues/README.md)

---

## 快速开始

### 构建与测试（推荐方式）

从项目根目录执行：

```bash
# 配置 + 构建 + 测试（一条命令）
cmake -S . -B build && cmake --build build -j8 && ctest --test-dir build
```

**分步执行**：
```bash
# 1. 配置 CMake
cmake -S . -B build

# 2. 构建（8 线程）
cmake --build build -j8
# 或
cd build && make -j8

# 3. 运行测试
ctest --test-dir build
# 或
cmake --build build --target test
```

**常用 ctest 选项**：
```bash
ctest --test-dir build --output-on-failure   # 显示失败测试详情
ctest --test-dir build -R "formula"          # 只运行匹配正则的测试
ctest --test-dir build -j8                   # 并行运行
```

### 传统构建方式

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)  # Linux
make -j8        # macOS
```

### 运行 Synthesis

```bash
# 使用 Cosy2 工具（从项目根目录运行）
./build/output/Cosy2 -f examples/response.ltlf -p examples/response.part
```

### ⚠️ 重要：运行目录规范

**必须从项目根目录运行所有命令！**

```bash
# ✅ 正确：从项目根目录运行
cd /home/lic/files/rewrite_ltlf_codes/CosyZeroRewrite
./build/tests/bench/benchmark_test tools/benchmarks/sm1000 1 100

# ❌ 错误：从 build 目录运行
cd /home/lic/files/rewrite_ltlf_codes/CosyZeroRewrite/build
./benchmark_test ../tools/benchmarks/sm1000 1 100
```

**原因**：
- 日志文件输出到 `output/logs/` 目录
- CSV 结果输出到 `output/results/` 目录
- 从 build 目录运行时，这些文件会被输出到 `build/output/logs/` 和 `build/output/results/`
- 下次 `rm -rf build && cmake .. && make` 时，这些文件会被删除！

**规则**：
1. 始终从项目根目录运行命令
2. 可执行文件使用 `./build/xxx` 或 `build/tests/bench/benchmark_test` 等相对路径
3. 日志和结果会自动保存到项目根目录的 `output/logs/` 和 `output/results/` 下

---

## 核心组件

```
FormulaPool (内存管理)
    │
    ├── Formula (不可变公式表示)
    │       ├── NNF 转换
    │       ├── XNF 转换
    │       └── Simplify
    │
    └── FormulaParser
            └── → Formula*

TableauState (DFA 状态)
    │
    └── OnTheFlyGameSolver
            └── is_realizable() → bool
```

详见: [核心组件设计](./docs/ARCHITECTURE/components.md)

---

## 工作习惯

### ⚠️ 重要：开始工作前先更新文档

**在开始任何工作之前，必须先完成以下步骤**：

1. **更新 TODO 列表** (TodoWrite)
   - 将要做的任务拆分成具体步骤
   - 每个任务包含 content, status, activeForm

2. **更新相关文档**
   - 记录当前问题、想法、建议到 `docs/working_issues/`
   - 记录调试计划和预期结果

3. **记录用户要求**
   - 用户的新要求、习惯、偏好记录到本文件 (CLAUDE.md)
   - 确保后续会话可以了解这些约定

**原因**：
- 会话可能被中断，需要恢复上下文
- 文档记录便于后续继续工作
- 避免"忘记重新编译"等低级错误重复发生

**记录新知识的习惯**：

当在调试过程中发现新的重要知识时：
1. **立即记录**到相关文档（CLAUDE.md 或 working_issues）
2. **标注发现日期和上下文**
3. **更新测试脚本**以避免重复错误

例如：
- **编译命令**：项目使用 CMake，必须用 `cd build && make` 或 `cmake --build build`，不能在根目录直接 `make`
- Cosy partition 文件格式要求（inputs 先写，空 inputs 也要写）
- 新发现的边界情况
- 容易犯的错误模式

### ⚠️ 文档修改规范 (2026-01-04)

**规范**：
1. **章节标题后添加日期**（仅文档，不是代码注释）
   - 格式：`(YYYY-MM-DD)`
   - 示例：`### Assignment Semantics (2026-01-04)`
   - 适用于：架构文档、工作流程规范、CLAUDE.md 的新增章节

2. **代码注释不要每次都写日期**
   - ❌ 不要：`// loop counter (2026-01-04)`
   - ❌ 不要：`// Updated: 2026-01-04`
   - ✅ 可以：关键设计决策的一次性注释（如 `// Assignment Semantics: sigma only contains TRUE variables`）

3. **适用范围**
   - ✅ **加日期**：新增文档章节、重要架构设计
   - ❌ **不加日期**：普通代码修改、bug fix、重构

**原因**：
- 日期太多会淹没真正的信息
- Git 记录已经提供了时间戳
- 只在文档层面记录重要变更的时间点

### Formula 字符串表示规范 (2026-01-04)

**问题**: Formula 对象存储的是 `int var_id_` (整数 ID)，不是字符串变量名。

**存储架构**:
```
Formula 对象:
├── int var_id_          // 整数 ID: 0, 1, 2...
├── OpType op_           // 操作符类型
└── Formula* left_/right_ // 子节点

FormulaPool:
├── vector<string> var_names_     // [0]="p1", [1]="p2", ...
└── unordered_map<string, int> var_ids_  // ["p1"]=0, ...
```

**两种字符串方法**:

| 方法 | 用途 | 输出示例 | 参数 |
|------|------|---------|------|
| `to_string()` | 内部格式（v0, v1） | `!(v0 & v1)` | 无 |
| `to_string_with_names(pool)` | 原始变量名 | `!(p1 & p2)` | 需要pool |

**命名约定**: 凡是需要显示给用户看的公式输出，统一使用 `to_string_with_names(pool)`。

**历史**: 原 `to_verbose_string()` 于 2026-01-04 改名为 `to_string_with_names()` 以提高语义清晰度。

### 括号输出规则 (2026-01-04)

**核心原则**: 括号的添加确保 `to_string → parse → to_string` 的 roundtrip 正确性。

**Parser 优先级**（从低到高）：
- Or (最低)
- And
- Until/Release
- Not/Next
- Primary (Literal, True, False) (最高)

**括号规则**：

| 子节点类型 | 是否加括号 | 示例 |
|-----------|-----------|------|
| Literal/True/False | 否 | `v0`, `true` |
| Not | 否 | `!v0` |
| Next | 否（Next 自己加括号） | `X(v0)` |
| 二元操作符 (And/Or/U/R) | **是** | `(p1 \| p2) & p3` |

**Next 特殊处理**: 检查子结果是否已有括号，避免重复
- `X(p0)` → `X(p0)`
- `X(p0 & p1)` → `X(p0 & p1)`
- `X((p0 R p2))` → `X(p0 R p2)`（去重）

**输出示例**：

| 输入 | 输出 | 说明 |
|------|------|------|
| `(p1 \| p2) & p3` | `(p1 \| p2) & p3` | 保留括号 |
| `(p1 & p2) \| p3` | `(p1 & p2) \| p3` | 保留括号（清晰） |
| `(p1 U p2) & p3` | `(p1 U p2) & p3` | 保留括号（清晰） |
| `(p1 \| p2) U p3` | `(p1 \| p2) U p3` | 保留括号 |
| `X(p0)` | `X(p0)` | Next 保持括号 |
| `X(p0 & p1)` | `X(p0 & p1)` | Next + 无重复 |
| `X((p0 R p2))` | `X(p0 R p2)` | 去重复括号 |

**交流语言偏好**：

- **主要使用中文**进行交流
- **专业术语保留英文**（如 REALIZABLE, SCC, tableau）
- **代码和命令保持原样**（如英文变量名、路径）
- **输出结果按原样显示**（如日志、错误信息）

### 任务管理

1. **开始任务前**: 更新 TODO 列表 (TodoWrite + `docs/TODO/*.md`)
2. **实现代码**: 遵循设计文档
3. **编写测试**: 确保覆盖率
4. **验证**: `make` + `make test`
5. **提交**: 标记 completed + `git commit`

### ⚠️ 重要：Debug 时先讨论再修改

**在调试复杂问题时，必须先与用户讨论，确认方案后再执行！**

**流程**：
1. **分析问题** - 通过 debug 输出、代码分析等方式理解问题根因
2. **列出计划** - 提出修复计划和技术方案（包括：修改位置、修改内容、预期效果）
3. **等待确认** - 用户确认方案可行后再执行
4. **执行修改** - 按照确认的方案修改代码
5. **验证结果** - 编译、测试、确认问题解决

**原因**：
- 避免盲目尝试导致代码混乱
- 用户可能对问题有更深入的理解
- 有些改动可能影响架构设计，需要讨论

**示例格式**：
```
## 问题分析
[描述当前问题的具体表现和根因]

## 修复计划
### 方案 A: [方案名称]
- 修改位置: 文件路径:行号
- 修改内容: 具体改动
- 预期效果: 修复后应该达到的结果
- 风险评估: 可能的影响

### 方案 B: [备选方案]
...
```

**例外情况**：
- 明显的 typo、笔误 → 可以直接修复
- 简单的日志输出调整 → 可以直接修改
- 其他情况一律先讨论！

### ⚠️ Git 提交规范（编译通过后）

**重要：每次代码修改能编译通过后，必须立即提交到 git！**

**原因**：
- 每次编译通过都是一个可回退的稳定点
- 便于理解每个改动的效果
- 出问题时可以快速回滚到正确的版本

**提交流程**：
1. 代码修改完成
2. `cd build && make` 编译通过
3. `git add -A && git commit -m "..."`
4. 然后进行测试和调试

**示例**：
```bash
# 1. 修改代码
vim src/automata/tableau.cpp

# 2. 编译
cd build && make

# 3. 编译通过后立即提交！
git add -A && git commit -m "refactor: xxx"

# 4. 然后运行测试
./build/debug_eventually_contradiction
```

详见: [TODO 管理流程](./docs/WORKFLOWS/todo_management.md)

### ⚠️ 重要：测试流程规范（必须遵守！）

**修改代码后的测试顺序**：

1. **单元测试/自定义测试** - 先验证基本功能
   ```bash
   make test  # 运行所有单元测试
   ```
   - 确保所有单元测试通过
   - 确保没有引入回归问题

2. **小范围抽查** (20-50 个随机案例)
   ```bash
   ./build/tests/bench/benchmark_test tools/benchmarks/sm1000 1 50
   ```
   - 验证基本逻辑在大范围内的稳定性
   - 发现问题立即修复，不要继续

3. **全量 Benchmark** (1000 个案例)
   ```bash
   ./build/tests/bench/benchmark_test tools/benchmarks/sm1000 1 1000
   ```
   - 只有在前面阶段通过后才运行
   - 避免浪费时间在明显有问题的代码上

**重要提醒**：
- ❌ **不要**修改代码后直接跑全量 benchmark
- ❌ **不要**跳过单元测试直接跑 benchmark
- ✅ **必须**按顺序：单元测试 → 小范围抽查 → 全量测试

**⚠️ Benchmark 超时问题解决方案**（2026-01-03 记录）：

当 benchmark_runner 出现超时问题时，可能是并发执行导致的。解决方案：
1. **使用串行/单线程模式运行测试**
   - 修改测试代码使用单线程执行
   - 或者一次只测试一个公式
2. **逐步增加测试数量**
   - 先测试 1-2 个用例确认功能正常
   - 再逐步增加到 10、50、100...

```bash
# 串行测试示例（一次只测一个公式）
./build/tests/bench/benchmark_test tools/benchmarks/sm1000 1 1  # 测试1个
```

**⚠️ Benchmark 调试优先级**（2026-01-04 记录）：

当 benchmark 出现问题时，按以下优先级处理：

1. **优先修复 Wrong Result (FAIL) 案例**
   - Wrong Result = 结果与预期不符（但程序正常结束）
   - 这通常是算法逻辑错误，需要立即修复
   - 修复一个 Wrong Result 比优化 100 个超时案例更重要

2. **超时 (TIMEOUT) 案例可以暂时忽略**
   - 超时可能是性能问题，也可能是公式本身很复杂
   - 只要 Cosy 参考实现也慢，就不是我们的问题
   - 性能优化可以在算法正确后再进行

**判断标准**：
- 先用 Cosy 测试超时案例，如果 Cosy 也超时/很慢，则不是 bug
- 关注 `Status=FAIL` 的案例，这些是必须修复的

### ⚠️ 重要：Git 提交规范（修改代码后）

**每次修改代码后的提交顺序**：

1. **先提交代码**，再运行测试
   ```bash
   git add -A
   git commit -m "..."
   ```

2. **然后按测试顺序运行测试**
   - 单元测试 → 小范围抽查 → 全量 benchmark

3. **如果测试发现问题**
   - 修复问题
   - 再次提交（小改动也要提交）
   - 重新测试

**原因**：
- 每次提交都是一个可回退的快照
- 便于理解每个改动的效果
- 出问题时可以快速回滚到正确的版本

**示例**：
```bash
# 1. 修改代码
vim src/synthesis/xxx.cpp

# 2. 编译
cd build && make

# 3. 先提交！
git add -A && git commit -m "fix: xxx"

# 4. 运行单元测试
make test

# 5. 小范围测试
./build/tests/bench/benchmark_test tools/benchmarks/sm1000 1 50

# 6. 全量测试（只有前面通过后）
./build/tests/bench/benchmark_test tools/benchmarks/sm1000 1 1000
```

### Bug 处理

1. **记录**: `docs/BUGS/open.md`
2. **修复**: 添加回归测试
3. **验证**: 所有测试通过
4. **归档**: 移至 `docs/BUGS/fixed.md`

详见: [Bug 修复流程](./docs/WORKFLOWS/bug_fix.md)

### 调试流程规范

**重要：修改代码后必须重新编译再测试！**

```bash
# 确保编译成功
cd build && make 2>&1 | tail -5
```

**使用 Cosy 参考实现验证**：

Cosy 是参考实现，准确率 95%+，可用于验证公式结果：

```bash
# Cosy 路径
/home/lic/files/rewrite_ltlf_codes/Cosy_rewrite/Cosy <ltlf_file> <part_file> <comb_idx>

# comb_idx: 0=Individual Composition, 1=Incremental Composition
# 示例：
/home/lic/files/rewrite_ltlf_codes/Cosy_rewrite/Cosy /tmp/test.ltlf /tmp/test.part 0
```

**注意**：
- Cosy 的答案有 95%+ 准确率
- 可用于验证边界案例
- 如果结果不一致，需要分析是 Cosy 的 5% 错误还是我们的实现问题

**⚠️ Cosy Partition 文件格式要求**（重要！）：

Cosy 对 `.part` 文件有严格的格式要求：

1. **顺序要求**：`.inputs:` 必须在 `.outputs:` 之前（上面一行）
2. **空 inputs 要求**：即使没有 inputs，也必须写 `.inputs: `（空的）
3. **正确格式**：
   ```
   .inputs: p1 p3
   .outputs: p5 p7
   ```
   或
   ```
   .inputs:
   .outputs: p5
   ```

**错误格式**（Cosy 会返回错误结果）：
```
.outputs: p5    # ❌ outputs 不能在 inputs 前面
.inputs: p1
```

**快速测试脚本**：使用 `./tools/scripts/quick_test.sh` 进行公式验证，脚本已自动处理格式问题。

**调试顺序**（渐进式测试）：

1. **Basic Hardcode Cases** - 手工构造的简单测试
   - 测试基本逻辑正确性
   - 全部通过后才进入下一步

2. **小范围抽查** (20-50 个随机案例)
   - 验证基本逻辑在大范围内的稳定性
   - 平均正确率超过 70% 后才进入下一步

3. **全量 Benchmark** (1000 个案例)
   - 只有在前面阶段通过后才运行
   - 避免浪费时间在明显有问题的代码上

**调试方法**：
- 从**结果正确的案例**组合/变异来找到错误边界
- 对比正确 vs 错误案例，分析模式差异
- 使用 Cosy 验证边界案例
- 不要盲目全量跑，浪费时间

### 复杂问题协作 (working_issues)

对于需要深入调查的复杂 bug，使用 `docs/working_issues/` 目录进行协作追踪：

1. **创建问题目录**: `docs/working_issues/YYYY-MM-DD_AM/PM_ProblemSummary/`
2. **编写详细报告**: `BUG_REPORT.md` 包含
   - 问题描述
   - 根因分析
   - 已修复的 bug 列表
   - 进行中的问题
   - 测试结果
3. **同步进展**: 每次有新进展时更新 `BUG_REPORT.md`
4. **完成后归档**: 问题解决后移至 `docs/BUGS/fixed.md`

**当前进行中的问题**:
- `docs/working_issues/2026-01-02_PM_BenchmarkAccuracy/` - Benchmark 准确率问题 (22% → 待改进)

### 提交规范

```
<type>: <简短描述>

<详细说明>

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
```

**⚠️ 重要：CHANGELOG 占位符必须替换！**

每次 git commit 后，系统会自动生成 CHANGELOG 文件（在 `docs/CHANGELOG/` 目录下）。
**必须立即手动替换占位符**：

```markdown
## AI Analysis

### 📝 Change Summary
<!-- TODO: Add a brief summary of the change in Chinese or English -->
```

**替换为**（用中文或英文填写）：

```markdown
## AI Analysis

### 📝 Change Summary
这里用一两句话描述这次修改做了什么

### 🔍 Technical Details
- 关键技术点 1
- 关键技术点 2

### 📊 Impact Analysis
- 影响范围/组件
- 性能影响（如有）
```

**检查方法**：
```bash
# 查看 latest CHANGELOG
ls -lt docs/CHANGELOG/2026-*/ | head -5
# 打开最新文件，搜索 TODO
grep -r "<!-- TODO:" docs/CHANGELOG/
```

详见: [提交规范](./docs/WORKFLOWS/commit_practice.md)

---

## 文档结构

```
CosyZeroRewrite/
├── CLAUDE.md              # 本文件 (项目概览)
├── docs/
│   ├── ARCHITECTURE/      # 架构文档
│   │   ├── README.md
│   │   ├── pipeline.md    # Synthesis 流水线
│   │   ├── components.md  # 核心组件设计
│   │   ├── algorithms.md  # 算法复杂度
│   │   ├── dependencies.md # 外部依赖
│   │   ├── roadmap.md     # 实现路线图
│   │   └── adr/           # 架构决策记录
│   ├── WORKFLOWS/         # 工作流程
│   │   ├── README.md
│   │   ├── bug_fix.md     # Bug 修复流程
│   │   ├── todo_management.md # TODO 管理
│   │   ├── commit_practice.md   # 提交规范
│   │   └── testing_strategy.md # 测试策略
│   ├── TODO/              # 待办事项 (按模块)
│   ├── BUGS/              # Bug 记录
│   └── build/             # 构建文档
├── examples/              # 示例公式
├── include/               # 头文件
├── src/                   # 源文件
└── tests/                 # 测试文件
```
