# ADR-004: 暂不升级 spdlog 到最新版本

## 状态
有效

## 上下文

项目当前使用 **spdlog 1.12.0** (内嵌 **fmt 9.1.0**)，而 spdlog 最新版本是 **1.14.1** (内嵌 **fmt 11.0.2**)。

在代码优化过程中，发现 `fmt::join` 在旧版本中不支持带 lambda 转换器的语法：
```cpp
// 想要的语法 (fmt 11+ 理论支持)
fmt::join(vec, [](const auto& x) { return x->to_string_with_names(pool); }, ",");

// 当前版本 (fmt 9.1) 只能这样
std::vector<std::string> tmp;
for (auto& f : vec) tmp.push_back(f->to_string_with_names(pool));
fmt::join(tmp, ",");
```

是否应该升级 spdlog 以获得更好的 fmt 功能？

## 决策

**暂不升级 spdlog**，继续使用当前版本 (1.12.0 + fmt 9.1.0)。

## 理由

### 版本对比

| 组件 | 当前版本 | 最新版本 | 差距 |
|------|----------|----------|------|
| spdlog | 1.12.0 | 1.14.1 | 2 个小版本 |
| bundled fmt | 9.1.0 | 11.0.2 | 2 个大版本 |

### 升级代价 vs 收益

| 方面 | 详情 |
|------|------|
| **破坏性变更** | fmt 10.0 将 `fmt::join` 移至 `fmt/ranges.h`，需修改 5 个 `.cpp` 文件 |
| **测试成本** | 需要重新运行所有单元测试 + 小范围抽查 + 全量 benchmark |
| **实际收益** | 较小 - 即使升级，`fmt::join` 的 lambda 转换器仍需 C++20 range formatter |
| **风险** | 中等 - fmt API 变化可能导致编译错误 |

### 现有方案已足够

当前使用 `ostringstream` 或临时 `vector<string>` 方案已经能很好地解决问题：
```cpp
// 当前可用的方案 (fmt 9.1)
std::vector<std::string> tmp;
for (auto& f : assignments) {
    tmp.push_back(f->to_string_with_names(pool));
}
debug_log("Assignments: {}", fmt::join(tmp, ","));
```

### 收益 < 代价

- **升级能带来的**: 支持 `fmt::join` 的更多特性
- **项目实际需求**: 不需要这些特性，现有方案已经够用
- **维护成本**: 升级后需要维护额外的头文件引用

## 升级时机 (未来考虑)

满足以下任一条件时可重新考虑升级：

1. **项目升级到 C++20** - 可充分利用 fmt 11 的 range formatting 特性
2. **需要 fmt 11 的新特性** - 项目确实需要新功能
3. **当前版本出现安全漏洞** - 安全问题优先
4. **依赖库要求** - 其他依赖项强制要求更新 spdlog

## 如果未来升级

### 需要修改的文件

```
src/automata/dfa.cpp
src/automata/tableau.cpp
src/synthesis/on_the_fly_solver.cpp
src/synthesis/strategy.cpp
src/synthesis/trace_exporter.cpp
```

### 需要添加的头文件

```cpp
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/bundled/ranges.h>  // 新增
```

### 测试计划

1. 运行所有单元测试 (`make test`)
2. 小范围抽查 (50 个随机案例)
3. 全量 benchmark (1000 个案例)

## 参考信息

- [spdlog Releases](https://github.com/gabime/spdlog/releases)
- [fmt ChangeLog](https://github.com/fmtlib/fmt/blob/master/ChangeLog.md)
- [spdlog Issue #3081 - C++17/C++11 compatibility](https://github.com/gabime/spdlog/issues/3081)
- [spdlog Issue #1757 - fmt::join discussion](https://github.com/gabime/spdlog/issues/1757)

## 记录日期
2026-01-10

## 相关决策
- 无
