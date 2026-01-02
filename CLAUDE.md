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

## 快速开始

### 构建

```bash
mkdir build && cd build
cmake ..
make
```

### 运行测试

```bash
./formula_tests
./parser_checker_tests
# 或运行全部测试
make test
```

### 运行 Synthesis

```bash
# 使用 Cosy2 工具
./Cosy2 -f examples/response.ltlf -p examples/response.part
```

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

### 任务管理

1. **开始任务前**: 更新 TODO 列表 (TodoWrite + `docs/TODO/*.md`)
2. **实现代码**: 遵循设计文档
3. **编写测试**: 确保覆盖率
4. **验证**: `make` + `make test`
5. **提交**: 标记 completed + `git commit`

详见: [TODO 管理流程](./docs/WORKFLOWS/todo_management.md)

### Bug 处理

1. **记录**: `docs/BUGS/open.md`
2. **修复**: 添加回归测试
3. **验证**: 所有测试通过
4. **归档**: 移至 `docs/BUGS/fixed.md`

详见: [Bug 修复流程](./docs/WORKFLOWS/bug_fix.md)

### 提交规范

```
<type>: <简短描述>

<详细说明>

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
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
