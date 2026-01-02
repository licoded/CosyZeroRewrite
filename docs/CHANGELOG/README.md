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

### Deleted
- file4

## Stats

- X files changed
- Y insertions(+)
- Z deletions(-)
```

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
