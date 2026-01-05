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

## jscpd 检测结果详细分析 (2026-01-06)

### 1. trace_exporter.cpp - 8 处重复

**问题模式**: "获取当前 SubStep" 前置检查

```cpp
// 在多个函数中重复出现:
if (!enabled_ || current_stage_index_ < 0) return;

TraceStage& stage = stages_[current_stage_index_];
if (stage.sub_steps.empty()) return;

SubStep& step = stage.sub_steps.back();
// 然后操作 step.xxx
```

**改进方案**: 提取辅助函数

```cpp
// 在 trace_exporter.hpp 中添加
class TraceExporter {
private:
    // 使用模板 + lambda 模式
    template<typename F>
    void with_current_sub_step(F&& func) {
        if (!enabled_ || current_stage_index_ < 0) return;
        TraceStage& stage = stages_[current_stage_index_];
        if (stage.sub_steps.empty()) return;
        func(stage.sub_steps.back());
    }
};

// 使用示例:
void TraceExporter::set_scc_id(const std::string& scc_id) {
    with_current_sub_step([&](SubStep& step) {
        step.highlights.scc_id = scc_id;
    });
}
```

**收益**: 减少约 40-50 行重复代码。

---

### 2. bdd_manager.cpp - 3 处重复

**问题模式 A**: BDD 操作的 reference/deref 模式

```cpp
// 在 And/Or/Until/Release 中重复:
DdNode* left_bdd = build_bdd_from_formula(f->left(), pool);
DdNode* right_bdd = build_bdd_from_formula(f->right(), pool);
DdNode* result = Cudd_bddXxx(cudd_->mgr, left_bdd, right_bdd);
Cudd_Ref(result);
Cudd_RecursiveDeref(cudd_->mgr, left_bdd);
Cudd_RecursiveDeref(cudd_->mgr, right_bdd);
return result;
```

**改进方案**: 使用 RAII 包装器

```cpp
// RAII BDD 指针包装器
class ScopedBDD {
    DdManager* mgr_;
    DdNode* node_;
public:
    ScopedBDD(DdManager* mgr, DdNode* node) : mgr_(mgr), node_(node) {
        Cudd_Ref(node);
    }
    ~ScopedBDD() {
        Cudd_RecursiveDeref(mgr_, node_);
    }
    DdNode* get() const { return node_; }
    // 禁止拷贝，允许移动...
    ScopedBDD(const ScopedBDD&) = delete;
    ScopedBDD& operator=(const ScopedBDD&) = delete;
};

// 使用模板辅助函数:
template<typename BinOp>
DdNode* build_binary_bdd(BddManager* mgr, Formula* f, FormulaPool& pool, BinOp&& op) {
    ScopedBDD left(mgr->cudd_->mgr, mgr->build_bdd_from_formula(f->left(), pool));
    ScopedBDD right(mgr->cudd_->mgr, mgr->build_bdd_from_formula(f->right(), pool));
    DdNode* result = op(mgr->cudd_->mgr, left.get(), right.get());
    Cudd_Ref(result);
    return result;
}
```

**问题模式 B**: 枚举所有输出的位掩码生成

```cpp
// 在 enumerate_all_output_assignments 和 enumerate_safe_moves_fallback 中重复:
std::vector<int> output_vars(relevant_output_var_ids.begin(), ...);
int n = output_vars.size();
for (uint32_t mask = 0; mask < static_cast<uint32_t>(1 << n); ++mask) {
    Assignment assignment;
    for (int i = 0; i < n; ++i) {
        if (mask & (1u << i)) {
            assignment.insert(output_vars[i]);
        }
    }
    all_moves.push_back(std::move(assignment));
}
```

**改进方案**: 提取为独立函数

```cpp
namespace {
    std::vector<Assignment> generate_all_subsets(const std::set<int>& vars) {
        std::vector<int> var_list(vars.begin(), vars.end());
        std::vector<Assignment> result;
        int n = var_list.size();

        result.reserve(1u << n);
        for (uint32_t mask = 0; mask < static_cast<uint32_t>(1 << n); ++mask) {
            Assignment assignment;
            for (int i = 0; i < n; ++i) {
                if (mask & (1u << i)) {
                    assignment.insert(var_list[i]);
                }
            }
            result.push_back(std::move(assignment));
        }
        return result;
    }
}
```

---

### 3. formula_z3.cpp - 3 处重复

**问题**: Z3 转换中的相似结构

**改进方向**:
- 将 Z3 表达式构建的重复模式提取为模板函数
- 使用类型统一的 Z3 上下文管理

---

### 4. 跨文件重复: cosy2.cpp ↔ on_the_fly_solver.cpp

**问题**: 收集公式中所有变量的 lambda 函数

```cpp
std::unordered_set<int> vars;
std::function<void(Formula*)> collect = [&](Formula* f) {
    if (!f) return;
    if (f->op() == Formula::OpType::Literal) {
        vars.insert(f->var_id());
    } else {
        collect(f->left());
        collect(f->right());
    }
};
collect(phi);
```

**改进方案**: 将此功能添加到 Formula 类

```cpp
// 在 formula.hpp 中添加静态方法:
class Formula {
public:
    static std::unordered_set<int> collect_variables(Formula* f) {
        std::unordered_set<int> vars;
        std::function<void(Formula*)> collect = [&](Formula* f) {
            if (!f) return;
            if (f->is_literal()) {
                vars.insert(f->var_id());
            } else {
                collect(f->left());
                collect(f->right());
            }
        };
        collect(f);
        return vars;
    }
};

// 使用简化为:
auto vars = Formula::collect_variables(phi);
num_outputs = static_cast<int>(vars.size());
```

**收益**: 消除跨文件重复，提高代码可维护性。

---

### 5. on_the_fly_solver.cpp - 3 处重复 (SCC 分类)

**问题**: SCC 分类逻辑中的相似代码

**改进方向**:
- 策略模式：将不同 SCC 类型的处理提取为独立的 handler
- 使用类型分发器而非重复的 if-else 链

---

## 改进优先级总结

| 优先级 | 文件 | 问题 | 建议 | 预计减少行数 |
|--------|------|------|------|--------------|
| **高** | cosy2.cpp / on_the_fly_solver.cpp | 变量收集 lambda | 添加 `Formula::collect_variables()` | ~20 行 |
| **高** | bdd_manager.cpp | BDD reference 模式 | 使用 RAII 包装器 | ~30 行 |
| **中** | bdd_manager.cpp | 位掩码生成 | 提取 `generate_all_subsets()` | ~25 行 |
| **中** | trace_exporter.cpp | 前置检查 | `with_current_sub_step()` 模板 | ~40 行 |
| **低** | formula_z3.cpp | Z3 转换模式 | 模板化 Z3 构建 | ~20 行 |

**总计可减少**: ~135 行重复代码

---

## 代码重复的本质

1. **"前置检查"模式** (trace_exporter): 可用模板+lambda 或辅助函数消除
2. **"资源管理"模式** (bdd_manager): RAII 是 C++ 的最佳实践
3. **"算法逻辑"模式** (跨文件重复): 应该提取为公共工具函数

---

## 防止代码重复的建议
