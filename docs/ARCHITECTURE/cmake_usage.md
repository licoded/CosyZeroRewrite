# CMake Usage Standard

## 目录结构

```
CosyZeroRewrite/
├── deps/                    # 第三方依赖
│   ├── catch2/             # Catch2 测试框架
│   └── README.md
├── include/                 # 项目头文件
├── src/                     # 项目源文件
├── tests/                   # 测试源文件（按功能分组）
├── cmake/                   # CMake 配置文件
└── docs/                    # 文档
```

## 输出结构

编译后，`build/` 目录结构：

```
build/
├── libformula.a            # 静态库
├── output/                 # 工具可执行文件
│   ├── Cosy2              # LTLf 合成工具
│   └── benchmark_runner   # 基准测试工具
└── tests/                  # 测试可执行文件（与 tests/ 源码结构一致）
    ├── formula/            # formula_test, transformation_test
    ├── parser/             # parser_test
    ├── transformation/     # nnf_test, xnf_test, next_test
    ├── automata/           # dfa_test, tarjan_test
    ├── synthesis/          # synthesis_test, on_the_fly_test
    ├── integration/        # prop_atoms_test, io_separation_test, strategy_test
    ├── fuzz/               # nnf_fuzz, xnf_fuzz, random_fuzz
    ├── debug/              # eventually_contradiction, failing_tests
    └── bench/              # benchmark_test, stress_test
```

## 标准编译流程

```bash
# 1. 配置
mkdir build && cd build
cmake ..

# 2. 编译
make -j8

# 3. 运行测试
make test
# 或单独运行
./tests/formula/formula_test
./tests/synthesis/on_the_fly_test

# 4. 运行工具
./output/Cosy2 -f examples/response.ltlf -p examples/response.part
```

## CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_TESTS` | ON | 构建单元测试 |
| `BUILD_STRESS_TEST` | OFF | 构建压力测试（运行数小时） |
| `BUILD_DEBUG_TESTS` | ON | 构建调试测试 |
| `BUILD_BENCH_TESTS` | ON | 构建基准测试 |
| `BUILD_FUZZER` | OFF | 构建 libFuzzer 目标 |
| `BUILD_BENCHMARK` | ON | 构建 benchmark_runner 工具 |
| `USE_Z3` | ON | 启用 Z3 SMT 求解器 |
| `USE_SPDLOG` | ON | 启用 spdlog 日志 |

### 使用选项

```bash
# 只构建核心库，不构建测试
cmake -DBUILD_TESTS=OFF ..

# 构建压力测试
cmake -DBUILD_STRESS_TEST=ON ..

# 禁用调试测试
cmake -DBUILD_DEBUG_TESTS=OFF ..
```

## 添加新测试

### 方式 1: 使用 Catch2（推荐）

在 `tests/<category>/test_name.cpp` 添加测试文件：

```cpp
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "formula/formula_pool.hpp"

TEST_CASE("test name", "[tag]") {
    // 测试代码
}
```

在 `cmake/Tests.cmake` 中添加：

```cmake
add_cosy_test(test_name_<category> <category> tests/<category>/test_name.cpp)
```

输出到：`build/tests/<category>/test_name_<category>`

### 方式 2: Standalone 测试

```cmake
add_cosy_test_standalone(test_name <category> tests/<category>/test_name.cpp)
```

## 添加新工具

在 `CMakeLists.txt` 添加：

```cmake
add_executable(MyTool src/mytool.cpp)
target_include_directories(MyTool PRIVATE include)
target_link_libraries(MyTool PRIVATE formula)
```

输出到：`build/output/MyTool`

## 依赖管理

### 添加新的 header-only 库

1. 将库文件放到 `deps/<libname>/`
2. 更新 `deps/README.md`
3. 在 CMake 中引用：

```cmake
add_library(<libname> INTERFACE)
target_include_directories(<libname> INTERFACE ${PROJECT_SOURCE_DIR}/deps/<libname>)
```

## 常用命令

```bash
# 清理重建
rm -rf build && mkdir build && cd build && cmake .. && make

# 只编译特定目标
make formula_test
make Cosy2

# 查看所有可用的目标
make help

# 运行特定测试
./tests/<category>/<test_name>

# Verbose 模式运行测试
./tests/<category>/<test_name> -s -d yes
```

## 设计原则

1. **输出分离**: 工具和测试分开到不同目录
2. **结构镜像**: `build/tests/` 结构与 `tests/` 一致
3. **依赖统一**: 所有第三方依赖放在 `deps/`
4. **命名一致**: 测试目标名使用 `<name>_<category>` 后缀避免冲突
