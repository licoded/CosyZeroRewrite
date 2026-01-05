# 代码重复分析

---

## Medium 问题

### 1. `read_benchmark()` 和 `read_benchmark_from_dir()` 重复

**位置**: `src/synthesis/synthesis.cpp:96-125`, `127-149`

**问题描述**:
两个函数共享几乎相同的文件读取逻辑：
```cpp
// read_benchmark - 自动判断目录
bool Synthesis::read_benchmark(const std::string& base_dir, int bench_num,
                               std::string& formula_str,
                               std::vector<std::string>& outputs,
                               std::vector<std::string>& inputs,
                               int* bench_dir) {
    int dir = (bench_num <= 500) ? 1 : 2;
    if (bench_dir) *bench_dir = dir;

    std::string ltlf_file = base_dir + "/bench" + std::to_string(dir) + "/f" + std::to_string(bench_num) + ".ltlf";
    std::string part_file = base_dir + "/bench" + std::to_string(dir) + "/f" + std::to_string(bench_num) + ".part";

    // ... 文件读取逻辑 ...
}

// read_benchmark_from_dir - 指定目录
bool Synthesis::read_benchmark_from_dir(const std::string& base_dir, int bench_dir, int bench_num,
                                        std::string& formula_str,
                                        std::vector<std::string>& outputs,
                                        std::vector<std::string>& inputs) {
    std::string ltlf_file = base_dir + "/bench" + std::to_string(bench_dir) + "/f" + std::to_string(bench_num) + ".ltlf";
    std::string part_file = base_dir + "/bench" + std::to_string(bench_dir) + "/f" + std::to_string(bench_num) + ".part";

    // ... 完全相同的文件读取逻辑 ...
}
```

**建议修复**:
```cpp
// 私有辅助函数，包含公共逻辑
namespace {
    struct BenchmarkFiles {
        std::string ltlf_file;
        std::string part_file;
    };

    BenchmarkFiles make_benchmark_paths(const std::string& base_dir, int bench_dir, int bench_num) {
        return {
            base_dir + "/bench" + std::to_string(bench_dir) + "/f" + std::to_string(bench_num) + ".ltlf",
            base_dir + "/bench" + std::to_string(bench_dir) + "/f" + std::to_string(bench_num) + ".part"
        };
    }

    bool read_benchmark_files(const BenchmarkFiles& files,
                              std::string& formula_str,
                              std::vector<std::string>& outputs,
                              std::vector<std::string>& inputs) {
        // 统一的文件读取逻辑
        std::ifstream ltlf(files.ltlf_file);
        std::ifstream part(files.part_file);
        // ...
    }
}

// 公共接口使用辅助函数
bool Synthesis::read_benchmark(const std::string& base_dir, int bench_num,
                               std::string& formula_str,
                               std::vector<std::string>& outputs,
                               std::vector<std::string>& inputs,
                               int* bench_dir) {
    int dir = (bench_num <= 500) ? 1 : 2;
    if (bench_dir) *bench_dir = dir;
    auto files = make_benchmark_paths(base_dir, dir, bench_num);
    return read_benchmark_files(files, formula_str, outputs, inputs);
}

bool Synthesis::read_benchmark_from_dir(const std::string& base_dir, int bench_dir, int bench_num,
                                        std::string& formula_str,
                                        std::vector<std::string>& outputs,
                                        std::vector<std::string>& inputs) {
    auto files = make_benchmark_paths(base_dir, bench_dir, bench_num);
    return read_benchmark_files(files, formula_str, outputs, inputs);
}
```

**收益**:
- 减少代码重复约 30 行
- 单一位置维护文件读取逻辑
- 更容易添加错误处理和日志

---

### 2. `to_string()` 和 `to_string_with_names()` 重复

**位置**: `src/formula/formula.cpp:115-179`, `181-244`

**问题描述**:
两个函数结构几乎相同，只是变量名处理不同：
```cpp
std::string Formula::to_string() const {
    // 使用 var_id (v0, v1, v2, ...)
    // 递归构建字符串
}

std::string Formula::to_string_with_names(const FormulaPool& pool) const {
    // 使用 pool.get_variable_name(var_id) (p1, p2, p3, ...)
    // 完全相同的递归结构
}
```

**建议修复**:
```cpp
// 私有辅助函数，使用可调用对象处理变量名
namespace {
    using VarNameResolver = std::function<std::string(int)>;

    std::string to_string_with_resolver(const Formula* f,
                                        const VarNameResolver& resolver) {
        if (!f) return "nullptr";

        switch (f->op()) {
            case Formula::OpType::Literal:
                if (f->sign()) return "!" + resolver(f->var_id());
                return resolver(f->var_id());
            // ... 其他情况完全相同
        }
    }
}

// 公共接口
std::string Formula::to_string() const {
    return to_string_with_resolver(this, [](int var_id) {
        return "v" + std::to_string(var_id);
    });
}

std::string Formula::to_string_with_names(const FormulaPool& pool) const {
    return to_string_with_resolver(this, [&pool](int var_id) {
        return pool.get_variable_name(var_id);
    });
}
```

---

## Low 问题

### 3. 进度条更新模式重复

**位置**: `tests/bench/benchmark.cpp` 中的 `ProgressDisplay` 类

**问题描述**:
进度更新逻辑在多个地方重复，可以抽象为通用模式。

**建议**: 考虑将 `ProgressDisplay` 提取到独立的头文件，供其他测试使用。

---

## 检测方法 (2026-01-06)

### 方法 1: jscpd (JavaScript Copy-Paste Detector)

**安装**:
```bash
npm install -g jscpd
```

**使用**:
```bash
# 基本检测 (min-tokens=50)
jscpd src/ include/ --min-tokens 50 --format cpp

# 生成 JSON 报告
jscpd src/ include/ --min-tokens 50 --format cpp --output /tmp/jscpd_report
```

**特点**:
- 基于 token 分析，适合检测字面代码重复
- 支持 C++ 和多种语言
- 输出格式友好，支持 JSON/HTML 报告

**项目当前状态 (2026-01-06)**:
| 指标 | 数值 |
|------|------|
| 分析文件数 | 19 |
| 总代码行数 | 8,592 |
| 总 token 数 | 70,449 |
| 发现重复 | 21 处 |
| 重复行数 | 216 (2.51%) |
| 重复 token | 2,325 (3.3%) |

**主要重复位置**:
- `trace_exporter.cpp` - 8 处 (格式化输出代码)
- `on_the_fly_solver.cpp` - 3 处 (SCC 分类逻辑)
- `bdd_manager.cpp` - 3 处 (BDD 操作模式)
- `formula_z3.cpp` - 3 处 (Z3 转换模式)
- `game_graph_export.cpp` - 1 处 (图导出代码)
- `formula_pool.cpp` - 1 处 (hash 计算)
- `cosy2.cpp` / `on_the_fly_solver.cpp` - 1 处 (跨文件重复)

---

### 方法 2: lizard (代码复杂度分析)

**安装**:
```bash
pip3 install --user lizard
```

**使用**:
```bash
# 基本分析
lizard src/ include/ -l cpp

# 高复杂度警告 (CCN > 20)
lizard src/ include/ -l cpp --CCN 20
```

**特点**:
- 主要分析圈复杂度 (CCN)
- 间接反映代码重复和结构问题
- 可以检测过长的函数

**项目当前状态 (2026-01-06)**:
| 指标 | 数值 |
|------|------|
| 总函数数 | 402 |
| 平均 CCN | 4.7 |
| 平均函数长度 | 15.4 NLOC |
| 高复杂度函数 (CCN>20) | 16 个 |

**高复杂度函数 (CCN > 20)**:
| 函数 | CCN | 长度 | 文件 |
|------|-----|------|------|
| `classify_scc` | 40 | 186 | `on_the_fly_solver.cpp:643` |
| `to_html` | 22 | 348 | `game_graph_export.cpp:306` |
| `main` | 33 | 198 | `cosy2.cpp:176` |
| `parse_partition_file` | 32 | 88 | `cosy2.cpp:49` |
| `apply_rm_next_impl` | 31 | 125 | `bdd_manager.cpp:28` |
| `to_z3_bounded` | 29 | 119 | `formula_z3.cpp:398` |
| `tokenize` | 29 | 148 | `formula_parser.cpp:69` |
| `check_propagation_consistency` | 29 | 96 | `on_the_fly_solver.cpp:834` |
| `is_realizable` | 26 | 150 | `on_the_fly_solver.cpp:117` |
| `to_json` | 28 | 131 | `game_graph_export.cpp:147` |
| `validate_create` | 23 | 46 | `formula_pool.cpp:93` |
| `to_string_impl` | 22 | 65 | `formula.cpp:117` |
| `write_json` | 22 | 147 | `trace_exporter.cpp:703` |
| `to_dot` | 22 | 120 | `game_graph_export.cpp:22` |
| `get_assignment_label` | 21 | 60 | `on_the_fly_solver.hpp:189` |
| `formula_progression` | 28 | 94 | `tableau.cpp:218` |

---

### 方法 3: clang-tidy (冗余代码检测)

**安装**:
```bash
# Ubuntu/Debian
sudo apt-get install clang-tidy

# 或使用特定版本 (如 clang-tidy-14)
sudo apt-get install clang-tidy-14
```

**使用 (本项目可用命令)**:
```bash
# 确保 compile_commands.json 存在
cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. && cd ..

# 检测冗余代码 (需要添加头文件路径)
clang-tidy-14 -p build \
  -checks='readability-duplicate-include,misc-redundant-expression,bugprone-redundant-branch-condition,readability-redundant-control-flow' \
  --extra-arg=-I/usr/include/c++/11 \
  --extra-arg=-I/usr/include/x86_64-linux-gnu/c++/11 \
  src/formula/*.cpp src/synthesis/*.cpp src/automata/*.cpp
```

**特点**:
- 基于 AST 分析，适合检测特定 C++ 模式
- 主要检测冗余表达式、重复 include、冗余控制流
- 不适合检测跨文件的字面代码重复

**可用检查类别**:
- `readability-duplicate-include` - 检测重复的 #include
- `misc-redundant-expression` - 检测冗余表达式
- `bugprone-redundant-branch-condition` - 检测冗余分支条件
- `readability-redundant-control-flow` - 检测冗余控制流
- `readability-redundant-*.**` - 其他冗余相关检查

**注意**: clang-tidy 没有专门的代码重复检测检查（像 jscpd 那种 token-based 的跨文件重复检测），更适合检测函数内部的冗余模式。

---

## 工具对比 (2026-01-06)

| 工具 | 类型 | 检测能力 | 适合场景 |
|------|------|----------|----------|
| **jscpd** | Token-based | 字面代码重复 | 跨文件 copy-paste 检测 |
| **lizard** | Complexity-based | 圈复杂度 | 发现需要重构的复杂函数 |
| **clang-tidy** | AST-based | 冗余模式 | 函数内部的代码质量问题 |

**推荐使用顺序**:
1. 先用 **jscpd** 检测字面重复
2. 再用 **lizard** 发现高复杂度函数
3. 最后用 **clang-tidy** 检测特定代码模式问题

---

## 防止代码重复的建议

1. **DRY 原则**: Don't Repeat Yourself - 相同逻辑只写一次
2. **提取公共函数**: 将重复的 3+ 行代码提取为函数
3. **使用模板**: 相似结构但不同类型的情况
4. **策略模式**: 使用函数对象/lambda 处理变化的逻辑
5. **定期审查**: 使用工具定期检测代码重复
