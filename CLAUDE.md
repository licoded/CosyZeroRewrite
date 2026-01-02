# CosyZero Formula Module Rewrite

## 项目概述

基于 `/home/lic/files/rewrite_ltlf_codes/CosyZeroRewrite/migrationDocs/formula_redesign/` 目录下的设计文档，使用 **C++17** 和 **CMake** 实现的 Formula 模块重写。

## 技术栈

- **语言**: C++17
- **构建工具**: CMake 3.10+
- **测试框架**: 待定 (讨论中)
- **编译器**: GCC 11.4.0 / Clang

## 设计文档

所有设计文档位于 `migrationDocs/formula_redesign/` 目录：

| 文档 | 内容 |
|------|------|
| `README.md` | 设计文档导航 |
| `FORMULA_REWRITE_DESIGN.md` | 完整设计规范 |
| `IMPLEMENTATION_TASKS.md` | 实现任务清单 |
| `NNF_TRANSFORMATION.md` | NNF 转换算法 |
| `XNF_TRANSFORMATION.md` | XNF 转换算法 |
| `SIMPLIFY_ANALYSIS.md` | 化简算法分析 |
| `HASH_CONSING_ANALYSIS.md` | Hash consing 分析 |
| `FLOWCHARTS.md` | 流程图 |

## 核心组件

### Formula 类 (`include/formula/formula.hpp`)

不可变的 LTLf 公式表示：
- **OpType 枚举**: True, False, Not, And, Or, Next, Until, Release, End, Literal
- **不可变设计**: 所有字段有效不可变
- **原始指针**: 由 FormulaPool 管理内存
- **缓存哈希**: 高效比较

### FormulaPool 类 (`include/formula/formula_pool.hpp`)

公式创建和内存管理：
- **Hash Consing**: 自动去重，结构唯一性
- **变量管理**: 预分配，输出优先的变量顺序
- **RAII**: 自动内存管理

### 转换操作

| 操作 | 文件 | 复杂度 |
|------|------|--------|
| NNF | `nnf.cpp` | O(n) |
| Simplify | `simplify.cpp` | O(n) |
| XNF | `xnf.cpp` | O(n) |
| rmnext | `rmnext.cpp` | O(n) |

## 当前状态

- [x] 目录结构创建
- [x] CMakeLists.txt 配置 (C++17)
- [x] Formula 类实现
- [x] FormulaPool 类实现
- [x] NNF 转换实现
- [x] XNF 转换实现
- [x] Simplify 实现
- [x] rmnext 实现
- [x] Z3 BMC 等价性检查
- [x] 日志系统集成 (spdlog)
- [x] 所有测试添加日志输出

### 测试覆盖

| 测试套件 | 断言数 | 测试用例 | 状态 |
|---------|-------|---------|------|
| formula_tests | 66 | 31 | ✓ 全部通过 |
| parser_checker_tests | 124 | 31 | ✓ 全部通过 |
| transformation_tests | 4 | 4 (196 公式) | ✓ 全部通过 |
| random_formula_test | - | 10000 | ✓ 全部通过 |

### Fuzzing 测试

- **随机公式测试**: 10000 个随机生成的公式
  - Parser roundtrip: 10000 通过, 0 失败, 0 崩溃
  - NNF 等价性: 10000 通过, 0 失败
  - XNF 等价性: 10000 通过, 0 失败
  - Simplify 等价性: 10000 通过, 0 失败
  - NNF 幂等性: 10000 通过, 0 失败

### 已修复的问题

1. **to_verbose_string 格式**: `True/False` → `true/false` (解析器兼容)
2. **压力测试逻辑**: 修复期望值错误和随机测试逻辑
3. **变量名管理**: FormulaPool 变量声明和查询正确性

### Benchmark 数据

`benchmarks/` 目录包含 LTLf synthesis 标准测试数据：

- **benchmarks/sm1000/**: SMv2 benchmark set (1000 个测试用例)
  - `bench1/`, `bench2/`: 包含 `.ltlf` (公式) 和 `.part` (变量定义) 文件
  - `results.csv`: 标准答案 (Realizable/Unrealizable)

#### Benchmark Runner 使用

```bash
# 构建时启用 benchmark
cmake .. -DBUILD_BENCHMARK=ON
make

# 运行 benchmark (默认 f1-f100)
./benchmark_runner

# 指定范围
./benchmark_runner /path/to/sm1000 100 200
```

当前状态：
- ✓ 读取 `.ltlf` 和 `.part` 文件
- ✓ 解析公式
- ✓ 读取预期结果
- ✓ SAT 检查 (使用 Z3)
- ✗ Realizability 检查 (需要完整的 synthesis 实现)

## 构建命令

```bash
mkdir build && cd build
cmake ..
make
./formula_tests
```

### 禁止事项

- 不要在单个 commit 中混合多个无关的修改
- 不要提交调试代码或临时文件
- 不要提交前先运行 `make` 确保编译通过

### 提交流程

1. 完成一个功能模块
2. 运行测试确保通过
3. `git add` 相关文件
4. `git commit` 带清晰描述
5. 继续下一个功能

## 待办事项

### 1. LTLf Synthesis 模块分析

参考实现位于 `/home/lic/files/rewrite_ltlf_codes/Cosy_rewrite/`，已完整实现 Synthesis 功能。

#### 架构概览

```
┌─────────────────────────────────────────────────────────────┐
│                    LTLf Synthesis Pipeline                   │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  LTLf Formula ──► Parser ──► aalta_formula                   │
│       │                                                      │
│       ▼                                                      │
│  Preprocessing (NNF → Simplify → NNF)                       │
│       │                                                      │
│       ▼                                                      │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  Lydia/AALTA: LTLf → DFA Conversion                │    │
│  │  - Symbolic DFA (BDD representation)               │    │
│  │  - Explicit DFA (state enumeration)                │    │
│  └─────────────────────────────────────────────────────┘    │
│       │                                                      │
│       ▼                                                      │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  Game Solving (Tarjan SCC + BDD)                    │    │
│  │  - Build game graph from DFA                       │    │
│  │  - Find SCCs using Tarjan algorithm                │    │
│  │  - Classify states: Swin (winning) / Ewin (losing) │    │
│  │  - Backward propagation for strategy extraction    │    │
│  └─────────────────────────────────────────────────────┘    │
│       │                                                      │
│       ▼                                                      │
│  Strategy / Realizable?                                      │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

#### 核心组件

| 组件 | 文件位置 | 功能 |
|------|----------|------|
| Formula | `formula/aalta_formula.*` | 公式表示、NNF/XNF/Simplify |
| Lydia Wrapper | `lydiasyft_wrapper/` | LTLf → DFA 转换接口 |
| BDD Manager | `synutil/formula_in_bdd.*` | BDD 状态表示 |
| Tarjan Algorithm | `synutil/syn_tarjan.*` | SCC 分解 |
| Synthesis | `ltlfsyn/synthesis.*` | 主合成算法 |
| Edge Constraints | `edge_cons/` | 转移关系管理 |

#### 外部依赖

| 依赖 | 用途 | 必需性 |
|------|------|--------|
| AALTA | 公式解析和转换 | 核心 |
| Lydia | LTLf → DFA 转换 | 核心 |
| CUDD | BDD 操作 | 核心 |
| spdlog | 日志 | 可选 |

#### CosyZeroRewrite 当前状态

**已实现**:
- ✓ Formula 类 (类似 aalta_formula 的简化版)
- ✓ FormulaPool (Hash consing)
- ✓ NNF, XNF, Simplify 转换
- ✓ Z3 等价性检查
- ✓ **DFA 数据结构** (`automata/dfa.hpp`)
- ✓ **LTLf → DFA 转换** (tableau 构造)
- ✓ **Tarjan SCC 算法** (游戏求解核心)
- ✓ **游戏图构造** (`synthesis/game_solver.hpp`)
- ✓ **is_realizable 函数** (基础版本)
- ✓ Benchmark runner (`tests/benchmark_runner.cpp`)

**测试覆盖**:
| 测试套件 | 断言数 | 测试用例 | 状态 |
|---------|-------|---------|------|
| formula_tests | 66 | 31 | ✓ 全部通过 |
| parser_checker_tests | 124 | 31 | ✓ 全部通过 |
| transformation_tests | 4 | 4 (196 公式) | ✓ 全部通过 |
| dfa_tests | 22 | 8 | ✓ 全部通过 |
| synthesis_tests | 20 | 7/8 | △ 基本通过 |
| random_formula_test | 50000 | 10000 | ✓ 全部通过 |

**待完善**:
- △ DFA 接受状态判定逻辑
- △ 输入/输出变量分离 (当前简化为单一状态)
- △ 策略提取
- ✗ BDD 符号化表示 (当前使用显式状态)

#### 实现路线图

**阶段 1: 外部工具集成** (推荐优先)
1. 使用子进程调用 AALTA/Lydia 进行 LTLf → DFA 转换
2. 解析生成的 DFA (如 .dot 格式)
3. 实现简化的游戏求解算法

**阶段 2: 纯 C++ 实现** (已完成基础版本)
1. ✓ 实现 LTLf → DFA 转换 (基于现有公式类)
2. △ 集成 BDD 库进行符号化表示 (当前使用显式状态)
3. ✓ 实现 Tarjan SCC 算法
4. ✓ 实现游戏求解和策略分类
5. △ 完善接受状态判定逻辑

**阶段 3: 优化**
1. DFA 最小化
2. 组成式合成 (compositional synthesis)
3. 符号化执行优化
4. 策略提取和可视化

### 2. 其他待办任务

- [ ] 添加集成测试
- [ ] 添加性能基准测试
- [ ] 完善文档和 Doxygen 注释
- [ ] 记录设计决策到新文档文件夹

### 工作习惯

**重要：每次开始新任务前，必须先更新 TODO 列表**

- 使用 TodoWrite 工具记录任务进度
- 任务开始前标记为 in_progress
- 任务完成后立即标记为 completed
- 发现新任务及时添加到列表
