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
- [x] 基础单元测试 (31 测试通过)
- [x] 日志系统集成 (spdlog)
- [x] 所有测试添加日志输出

### Benchmark 数据

`benchmarks/` 目录包含 LTLf synthesis 标准测试数据：

- **benchmarks/sm1000/**: SMv2 benchmark set (1000 个测试用例)
  - `bench1/`, `bench2/`: 包含 `.ltlf` (公式) 和 `.part` (变量定义) 文件
  - `results.csv`: 标准答案 (Realizable/Unrealizable)

## 构建命令

```bash
mkdir build && cd build
cmake ..
make
./formula_tests
```

## Git 提交规范

按模块拆分提交，每次提交专注单一功能：

```bash
# 示例
git add include/formula/formula.hpp src/formula/formula.cpp
git commit -m "feat: implement Formula class with immutable design"

git add include/formula/formula_pool.hpp src/formula/formula_pool.cpp
git commit -m "feat: implement FormulaPool with hash consing"
```

## 待办事项

1. 实现 LTLf synthesis 算法
2. 集成 benchmark 测试 (使用 `benchmarks/sm1000/` 数据)
3. 实现 Synthesis 结果与 `results.csv` 标准答案对比
4. 扩展测试覆盖率
5. 文档生成 (Doxygen)
