# CHANGELOG

> 每次 Git 提交的自动记录

---

## 文件命名格式

```
序号_commitHash_简述.md
```

示例:
- `00001_452f724_add_cosy2_tool.md`
- `00002_dbf0871_fix_parser_bug.md`
- `00003_6c36489_docs_reorg.md`

## 文件内容

```markdown
# [序号] commit message

**Commit**: `hash`
**Date**: YYYY-MM-DD HH:MM:SS
**Author**: name <email>

## Description

[完整提交消息]

## Changes

### Modified
- file1
- file2

### Added
- file3

## AI Analysis

### 📝 Change Summary
[AI 分析的改动摘要]

### 🔍 Technical Details
[关键技术细节]

### 📊 Impact Analysis
[影响分析，可能包含 mermaid 图表]

### ⚠️ Notes
[注意事项]

## Stats

- X files changed
- Y insertions(+)
- Z deletions(-)
```

## AI 智能分析

除了基础的 git 信息记录，CHANGELOG 还支持 **AI 深度分析**：

### 工作流程

```bash
# 1. 正常提交代码
git commit -m "feat: xxx"
# → 基础 CHANGELOG 自动生成

# 2. 让 Claude 分析代码改动
make ai-changelog
# → Claude 读取 diff，生成技术报告并追加到 CHANGELOG
```

### AI 分析内容

| 改动规模 | 输出内容 |
|---------|---------|
| 小改动 (<50 行) | 简洁摘要 |
| 中改动 (50-200 行) | 完整格式 + 技术细节 |
| 大改动 (200+ 行) | 详细分析 + 代码示例 + 流程图 |

### 流程图类型

根据改动类型自动选择合适的 mermaid 图表：

| 改动类型 | 图表类型 |
|---------|---------|
| 架构变更 | `graph TB`, `C4Context` |
| 流程变更 | `flowchart TD` |
| 类结构变更 | `classDiagram` |
| 调用时序变更 | `sequenceDiagram` |

### 示例

参见 `00030_97e2f8f_correct_accepting_state_condit.md` 的 AI Analysis 部分，包含 Until 语义的流程图说明。

## 自动生成

### 方式 1: Git Hook (推荐)

每次 `git commit` 后自动生成 CHANGELOG。

Hook 位置: `.git/hooks/post-commit`

### 方式 2: 手动触发

```bash
# 记录最新提交
make changelog

# 记录指定提交
make changelog COMMIT=HEAD~2

# 记录多个提交
make changelog COMMIT=HEAD~5..HEAD
```

## 序号管理

序号计数器存储在: `.git/changelog-counter`

- 初始值: 0
- 每次生成 changelog 后自动递增
- 手动修改可重置序号

## 禁用自动生成

如需临时禁用 hook：

```bash
# 方式 1: 使用 --no-verify
git commit --no-verify -m "message"

# 方式 2: 删除 hook
rm .git/hooks/post-commit

# 方式 3: 设置 NO_CHANGELOG 环境变量
NO_CHANGELOG=1 git commit -m "message"
```

## 恢复 Hook

```bash
make install-changelog-hook
```
