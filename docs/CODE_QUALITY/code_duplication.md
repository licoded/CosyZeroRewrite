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

## 检测方法

### 使用 clang-tidy
```bash
clang-tidy -checks='readability-duplicate-*,modernize-*' -p build/ src/*.cpp
```

### 使用 cpd (Copy-Paste Detector)
```bash
cpd --minimum-tokens 50 --files src/*.cpp include/*.hpp
```

---

## 防止代码重复的建议

1. **DRY 原则**: Don't Repeat Yourself - 相同逻辑只写一次
2. **提取公共函数**: 将重复的 3+ 行代码提取为函数
3. **使用模板**: 相似结构但不同类型的情况
4. **策略模式**: 使用函数对象/lambda 处理变化的逻辑
5. **定期审查**: 使用工具定期检测代码重复
