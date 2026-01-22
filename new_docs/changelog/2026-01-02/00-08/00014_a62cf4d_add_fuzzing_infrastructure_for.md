# [14] Add fuzzing infrastructure for formula library

**Commit**: `a62cf4d` ([`a62cf4d3abc57c75562ca6f598f197c810687143`](https://github.com/licoded/CosyZeroRewrite/commit/a62cf4d3abc57c75562ca6f598f197c810687143))
**Date**: 2026-01-02 01:31:49 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Fuzzer targets (require clang with libFuzzer):
- parser_fuzzer: Tests formula string parsing
- transformation_fuzzer: Tests transformation invariants
- equivalence_fuzzer: Tests equivalence checker properties

Fallback for GCC (no libFuzzer):
- fuzz_harness: Standalone harness for AFL++ or other fuzzers
- fuzz_driver: Test driver that simulates fuzzer behavior

Organized fuzz code in tests/fuzz/ subdirectory

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
建立 **Fuzzing 测试基础设施**，通过随机输入发现边界情况和隐藏 bug。支持 **libFuzzer** (Clang) 和 **AFL++** (GCC) 两种引擎，提供三种 Fuzzer 目标。

### 🔍 Technical Details

**Fuzzer 目标**：
| Fuzzer | 测试目标 | 不变量 |
|--------|---------|--------|
| parser_fuzzer | 解析器 | parse → string → parse 等价 |
| transformation_fuzzer | 转换 | NNF² = NNF, Simplify 幂等 |
| equivalence_fuzzer | 等价性检查 | 自反性、对称性、传递性 |

**编译条件**：
```cmake
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # 启用 libFuzzer
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # 使用 AFL++ harness
endif()
```

### 📊 Impact Analysis
- **范围**: `tests/fuzz/` 目录
- **影响**: 提高代码鲁棒性，后续演化压力测试 (10000 随机公式)
- **已知限制**: GCC 无 libFuzzer，需要外部 fuzzing 工具配合

`★ Insight ─────────────────────────────────────`
- **Fuzzing 策略**: 重点测试转换不变量 (如 NNF 幂等性)，这些是正确性关键
- **随机公式生成**: 后续在 random_formula_test 中实现 10000 公式测试
`─────────────────────────────────────────────────`

## Changes

### Added
- `tests/fuzz/fuzz_driver.cpp`
- `tests/fuzz/fuzz_equivalence.cpp`
- `tests/fuzz/fuzz_harness.cpp`
- `tests/fuzz/fuzz_parser.cpp`
- `tests/fuzz/fuzz_transformations.cpp`

### Modified
- `CMakeLists.txt`

## Stats

- **6** files changed
- **888** insertions(+)
- **0** deletions(-)
