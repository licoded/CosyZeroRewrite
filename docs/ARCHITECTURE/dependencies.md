# 外部依赖

> 项目依赖库和版本要求

---

## 核心依赖

| 依赖 | 版本 | 用途 | 必需性 | 许可证 |
|------|------|------|--------|--------|
| **CMake** | 3.10+ | 构建系统 | ✅ 必需 | BSD |
| **C++** | C++17 | 语言标准 | ✅ 必需 | - |
| **Z3** | 4.x | BMC 等价性检查 | ✅ 必需 | MIT |
| **spdlog** | 1.12+ | 日志输出 | ⚪ 可选 | MIT |
| **Catch2** | 2.x | 测试框架 | ✅ 必需 | BSL-1.0 |
| **CLI11** | 2.x | 命令行解析 | ✅ 必需 | BSD-3-Clause |

---

## 依赖详解

### Z3 (Theorem Prover)

**用途**: BMC (Bounded Model Checking) 等价性检查

**使用场景**:
- 验证转换前后公式等价性
- 测试公式语义正确性
- 随机公式测试

**安装**:
```bash
sudo apt-get install libz3-dev  # Ubuntu/Debian
brew install z3                   # macOS
```

**CMake 检测**:
```cmake
find_package(Z3 REQUIRED)
target_link_libraries(formula PRIVATE Z3::Z3)
```

### spdlog (Logging)

**用途**: 结构化日志输出

**当前状态**: 已内嵌 header-only 版本到 `include/spdlog/`

**配置**:
| 输出 | 级别 | 格式 | 说明 |
|------|------|------|------|
| 控制台 | INFO | 彩色 | 实时查看 |
| 文件 | TRACE | 纯文本 | 详细追踪 |

**文件日志**:
- **目录**: `logs/formula_YYYYMMDD_HHMMSS.log`
- **轮转**: 单文件最大 5MB，最多保留 3 个文件
- **级别**: TRACE (最详细，包含所有 DEBUG/INFO)

**使用**:
```cpp
#include "spdlog/spdlog.h"
LOG_INFO("Formula parsed: {}", formula_str);
LOG_ERROR("Parse error: {}", error_msg);
LOG_TRACE("Internal state: {}", debug_info);
```

### Catch2 (Testing)

**用途**: 单元测试框架

**特点**:
- Header-only (v2)
- BDD 风格测试
- 丰富的断言宏

**使用示例**:
```cpp
TEST_CASE("Formula creation", "[formula]") {
    FormulaPool pool;
    Formula* f = pool.create_variable("p");
    REQUIRE(f->is_literal());
}
```

### CLI11 (Command Line Parsing)

**用途**: 命令行参数解析

**当前状态**: Header-only 版本内嵌到 `deps/external/CLI/`

**特点**:
- Header-only，无需额外链接
- 自动生成格式化的帮助信息
- 支持短选项 (-f)、长选项 (--file)、位置参数
- 内置验证器（如文件存在性检查）
- 支持子命令、选项组等高级功能

**使用示例**:
```cpp
#include "CLI/CLI.hpp"

CLI::App app{"My Tool Description"};

std::string filename;
bool verbose = false;

app.add_option("-f,--file", filename, "Input file")
    ->check(CLI::ExistingFile);
app.add_flag("-v,--verbose", verbose, "Verbose output");

CLI11_PARSE(app, argc, argv);
```

**Cosy2 命令行选项**:
| 选项 | 说明 |
|------|------|
| `-f,--file <file>` | 从文件读取公式 |
| `-p,--partition <file>` | 读取变量分区文件 |
| `--trace [dir]` | 启用 trace 记录 |
| `-q,--quiet` | 静默模式（仅输出结果） |
| `--version` | 显示版本信息 |
| `-h,--help` | 显示帮助信息 |
| `formula` | 位置参数：公式字符串 |

---

## 未来依赖

### CUDD / BuDDy (BDD 库)

**用途**: 符号化状态表示

**计划**:
- [ ] 评估 CUDD vs BuDDy
- [ ] 设计 BDD 状态接口
- [ ] 实现 BDD 版本的 SCC 算法

### Lydia / AALTA (LTLf 工具)

**用途**: 对比验证和替代实现

**当前**: 代码在 `/home/lic/files/rewrite_ltlf_codes/Cosy_rewrite/`

---

## 依赖管理

### 版本锁定

```cmake
# CMakeLists.txt
find_package(Z3 4.8 REQUIRED)
find_package(spdlog 1.12)
```

### 可选依赖处理

```cmake
# spdlog 是可选的
if(spdlog_FOUND)
    target_compile_definitions(formula PRIVATE USE_SPDLOG)
    target_link_libraries(formula PRIVATE spdlog::spdlog)
else()
    message(WARNING "spdlog not found, using fallback logger")
endif()
```

---

## 平台兼容性

| 平台 | 编译器 | 状态 |
|------|--------|------|
| Linux 22.04 | GCC 11.4.0 | ✅ 测试通过 |
| Linux | Clang 14+ | ✅ 预期兼容 |
| macOS | AppleClang 15+ | ✅ 预期兼容 |
| Windows | MSVC 19.35+ | 🔄 未测试 |
