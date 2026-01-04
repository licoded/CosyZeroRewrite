# 目录结构分析报告

**日期**: 2026-01-05
**问题**: 项目目录管理不够清晰，存在遗弃/误追踪的文件

---

## 一、当前目录结构概览

```
CosyZeroRewrite/
├── benchmarks/           ✅ 正常 (2001 个追踪文件)
├── build/                ⚠️  应被忽略
├── build_fuzz/           ❌ 应被忽略但存在
├── benchmark_visualization/  ❌ 遗弃的旧版本
├── cmake/                ✅ 正常 (CMake 模块)
├── deps/                 ✅ 正常 (外部依赖)
├── docs/                 ⚠️  有空/遗弃子目录
├── examples/             ✅ 正常
├── include/              ✅ 正常
├── logs/                 ⚠️  有旧日志文件
├── migrationDocs/        ⚠️  旧设计文档
├── resources/            ❌ 误追踪的旧构建目录
├── results/              ⚠️  应被忽略
├── scripts/              ⚠️  未追踪到 git
├── src/                  ✅ 正常
├── tests/                ✅ 正常
├── tmp/                  ❌ 应被忽略但存在
├── tools/                ⚠️  可能遗弃
├── visualization/        ✅ 正常 (新版 Web 项目)
├── .gitignore
├── CMakeLists.txt
├── CLAUDE.md
└── Makefile
```

---

## 二、问题分类

### 2.1 误追踪到 Git 的目录/文件

| 目录/文件 | 问题描述 | 追踪文件数 |
|-----------|----------|-----------|
| `resources/` | 旧的构建目录，包含 CMakeCache.txt, libformula.a, 测试二进制等 | 23 个 |
| `logs/` | 只有 `.gitkeep` 应该被追踪，但有旧日志文件 | 1 个 (.gitkeep) |

**`resources/` 的详细内容**:
```
resources/
├── CMakeCache.txt       # CMake 生成文件，不应追踪
├── CMakeFiles/          # CMake 生成文件，不应追踪
├── cmake_install.cmake  # CMake 生成文件，不应追踪
├── CTestTestfile.cmake  # CMake 生成文件，不应追踪
├── libformula.a         # 编译产物，不应追踪
├── Makefile             # Make 生成文件，不应追踪
├── output/              # 包含 Cosy2, benchmark_runner 等二进制
└── tests/               # 包含所有测试二进制
```

### 2.2 遗弃的目录/文件

| 目录/文件 | 遗弃原因 | 建议 |
|-----------|----------|------|
| `benchmark_visualization/` | 旧的 HTML 报告，已被 `visualization/` Web 项目取代 | 删除 |
| `tools/benchmark_runner.cpp` | 新版本在 `tests/bench/benchmark.cpp` | 删除或归档 |
| `tools/visualize_results.py` | 旧的可视化脚本 | 删除或归档 |
| `docs/build/` | 空目录 | 删除 |
| `docs/TRACE_VISUALIZATION/` | 关于旧 HTML 可视化的文档 | 归档到 visualization/ 或删除 |

### 2.3 本地存在但应被忽略的目录

| 目录 | 内容 | 状态 |
|------|------|------|
| `tmp/` | 旧的测试二进制 (test_f11, test_simple 等) | 在 .gitignore 中但仍有文件 |
| `logs/` | 大量旧日志文件 | 只有 .gitkeep 应被追踪 |
| `build_fuzz/` | 旧的 fuzz 构建目录 | 在 .gitignore 中但目录仍存在 |
| `results/` | 测试结果输出 | 在 .gitignore 中正确配置 |
| `benchmark_results_latest.csv` | Benchmark 结果 CSV | 在根目录，应被忽略 |

### 2.4 未追踪到 Git 的目录

| 目录 | 内容 | 问题 |
|------|------|------|
| `scripts/` | 7 个脚本文件 (changelog.sh, quick_test.sh 等) | 未追踪！ |

### 2.5 可能有历史价值的目录

| 目录 | 内容 | 建议 |
|------|------|------|
| `migrationDocs/` | 28 个设计文档，记录了从 Cosy 到 CosyZeroRewrite 的迁移过程 | 保留在 docs/ARCHITECTURE/ 作为历史参考 |

---

## 三、优化方案

### 方案 A: 激进清理 (推荐用于生产准备)

1. **从 Git 移除误追踪的文件**:
   ```bash
   git rm -r --cached resources/
   git rm --cached logs/.gitkeep  # 然后重新添加
   ```

2. **删除本地遗留文件**:
   ```bash
   rm -rf tmp/ build_fuzz/ benchmark_visualization/
   rm -f benchmark_results_latest.csv
   rm -f logs/*.log logs/formula_* logs/transform_* logs/tableau_*
   ```

3. **删除遗弃目录**:
   ```bash
   rm -rf benchmark_visualization/
   rm -f tools/benchmark_runner.cpp
   rm -f docs/TRACE_VISUALIZATION/*
   rmdir docs/TRACE_VISUALIZATION
   rmdir docs/build
   ```

4. **迁移 scripts/ 到 Git**:
   ```bash
   git add scripts/
   ```

5. **迁移 migrationDocs/**:
   ```bash
   mv migrationDocs docs/ARCHITECTURE/migration_docs/
   ```

### 方案 B: 保守清理 (保留历史参考)

1. **从 Git 移除误追踪的构建产物** (同方案 A)

2. **保留遗弃文件但移到 archive/**:
   ```bash
   mkdir -p archive/old_visualization
   mv benchmark_visualization/ archive/old_visualization/
   mv tools/visualize_results.py archive/old_visualization/
   mv docs/TRACE_VISUALIZATION/ archive/old_visualization/docs/
   ```

3. **保留 migrationDocs 作为历史参考** (不移动)

### 方案 C: 归档方案 (推荐)

创建一个清晰的目录结构，将历史材料归档:

```
CosyZeroRewrite/
├── archive/
│   ├── old_builds/         # resources/ 的备份说明
│   ├── old_visualization/   # benchmark_visualization/
│   └── old_tools/           # tools/ 的旧内容
├── docs/
│   ├── ARCHITECTURE/
│   │   └── migration_docs/  # 从 migrationDocs/ 迁移
│   ├── BUGS/
│   ├── CHANGELOG/
│   ├── WORKFLOWS/
│   └── ...                  # 清理空/遗弃子目录
├── scripts/                 # 追踪到 git
├── src/
├── tests/
├── visualization/           # 新版 Web 项目
└── ...
```

---

## 四、建议的 .gitignore 更新

```gitignore
# ... 现有内容 ...

# 旧的构建目录（如果有人误创建）
resources/

# 根目录的临时文件
*.csv
*.tmp

# Scripts 目录应该被追踪（不添加到 ignore）
```

---

## 五、执行计划

### 阶段 1: 清理本地遗留文件 (无风险)
- 清空 tmp/
- 清空 logs/ 中的旧日志（保留 .gitkeep）
- 删除 build_fuzz/

### 阶段 2: 移除误追踪的文件 (需要讨论)
- 从 Git 移除 resources/
- 更新 .gitignore

### 阶段 3: 重组目录结构 (需要讨论)
- 决定 migrationDocs/ 的去留
- 决定 scripts/ 是否追踪
- 清理遗弃目录

### 阶段 4: 验证和提交
- 确保构建正常
- 确保测试运行正常
- 提交更改

---

## 六、需要讨论的问题

1. **migrationDocs/** 是否保留？如果保留，放在哪里？
   - 选项 A: 保留在当前位置
   - 选项 B: 移到 docs/ARCHITECTURE/migration_docs/

2. **scripts/** 是否追踪到 Git？
   - 这些脚本很有用，应该被追踪

3. **benchmark_visualization/** 完全删除还是归档？

4. **docs/TRACE_VISUALIZATION/** 是否还需要？（已被新版 Web 项目取代）

---

## 七、推荐方案总结

**推荐采用方案 C（归档方案）**，具体步骤：

1. ✅ 清理本地遗留文件（tmp, logs, build_fuzz）
2. ✅ 从 Git 移除 resources/，更新 .gitignore
3. ✅ 追踪 scripts/ 到 Git
4. ✅ 将 migrationDocs/ 移到 docs/ARCHITECTURE/migration_docs/
5. ✅ 删除 benchmark_visualization/ 和 docs/TRACE_VISUALIZATION/
6. ✅ 删除空的 docs/build/
7. ✅ 清理 tools/ 中的遗弃文件（或移动到 archive/）

