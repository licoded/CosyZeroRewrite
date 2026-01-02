# Git 提交规范

> 提交频率、格式和检查清单

---

## 1. 提交频率原则

### 1.1 原子性提交

**每次提交一个完整的逻辑单元**：

✅ 好的提交：
```
feat: implement Formula class with immutable design
fix: correct accepting state condition for Until formulas
docs: add bug fix workflow documentation
```

❌ 避免的提交：
```
wip: various changes
fix bugs and add features
update
```

### 1.2 模块化提交

不同模块的修改分开提交：

```bash
# 顺序示例
git add src/formula/formula.cpp
git commit -m "fix: correct accepting state condition"

git add tests/tableau_state_test.cpp
git commit -m "test: add regression tests for Until accepting"

git add docs/BUGS/fixed.md
git commit -m "docs: update bug documentation"
```

### 1.3 提交时机

- ✅ 完成一个独立功能后
- ✅ 修复一个 Bug 后
- ✅ 通过所有测试后
- ❌ 不要在测试失败时提交
- ❌ 不要在代码未编译通过时提交

---

## 2. 提交消息格式

### 2.1 标准格式

```
<type>: <简短描述>

<详细说明（可选）>

<相关 Issue 或引用（可选）>

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
```

### 2.2 Type 类型

| Type | 说明 | 示例 |
|------|------|------|
| `feat` | 新功能 | `feat: implement on-the-fly synthesis` |
| `fix` | Bug 修复 | `fix: parser single-char operator precedence` |
| `docs` | 文档更新 | `docs: add bug fix workflow` |
| `refactor` | 代码重构 | `refactor: simplify FormulaPool API` |
| `test` | 测试相关 | `test: add regression tests for bug #002` |
| `chore` | 构建/工具 | `chore: upgrade CMake to 3.15` |
| `perf` | 性能优化 | `perf: optimize hash consing` |

### 2.3 描述规范

简短描述：
- 使用中文
- 不超过 50 字
- 使用祈使句（"修复"而非"修复了"）

详细说明：
- 列出关键改动
- 说明原因和影响
- 可包含代码片段

---

## 3. 提交后 CHANGELOG

每次提交后自动生成 CHANGELOG 记录到 `docs/CHANGELOG/`。

### 3.1 自动生成 (默认)

通过 post-commit hook 自动触发：

```bash
git commit -m "feat: add new feature"
# → 自动生成: docs/CHANGELOG/00004_abc123_add_new_feature.md
# → 自动提交: docs: add CHANGELOG for abc123
```

**防无限循环机制**：Hook 会检测 CHANGELOG-only 提交（commit message 包含 `CHANGELOG`），跳过再次生成，避免递归。

### 3.2 文件命名

```
序号_commitHash_简述.md
```

示例:
- `00001_452f724_add_cosy2_tool.md`
- `00002_dbf0871_fix_parser_bug.md`
- `00003_6c36489_docs_reorg.md`

### 3.3 文件内容

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

## Stats

- X files changed
- Y insertions(+)
- Z deletions(-)
```

### 3.4 禁用/手动控制

```bash
# 临时禁用自动生成
NO_CHANGELOG=1 git commit -m "message"

# 或使用 --no-verify (跳过所有 hook)
git commit --no-verify -m "message"
```

### 3.5 手动生成 CHANGELOG

```bash
# 记录最新提交
make changelog

# 记录指定提交
make changelog COMMIT=HEAD~2

# 或直接运行脚本
./scripts/changelog.sh HEAD
```

### 3.6 恢复 Hook

```bash
make install-changelog-hook
```

详见: [CHANGELOG README](../../CHANGELOG/README.md)

---

## 4. 提交前检查清单

### 4.1 必须检查

- [ ] `make` 编译通过，无警告
- [ ] 所有测试通过 (`make test`)
- [ ] 新代码有测试覆盖
- [ ] 代码符合项目风格

### 4.2 文档检查

- [ ] API 变更更新了头文件注释
- [ ] Bug 修复更新了 `docs/BUGS/`
- [ ] 功能完成更新了 `docs/TODO/`
- [ ] 重要决策记录在 `docs/ARCHITECTURE/adr/`

### 4.3 检查脚本

```bash
#!/bin/bash
# pre-commit-check.sh

echo "Running pre-commit checks..."

# 编译
make -j4 || exit 1

# 测试
make test || exit 1

# 检查是否有待提交的文档变更
git diff --cached --name-only | grep -q "docs/" || {
    echo "Warning: No documentation changes"
}

echo "All checks passed!"
```

---

## 5. 常见模式

### 5.1 Bug 修复提交

```
fix: parser single-char operator precedence bug (#002)

Fixed bug where variables starting with 'r', 'f', or 'g' were
incorrectly parsed as Release/Finally/Globally operators.

Changes:
- Added peek-ahead logic in tokenizer
- Added 5 regression tests

Test results: 171 assertions in 36 test cases (was 124 in 31)

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
```

### 5.2 新功能提交

```
feat: implement on-the-fly LTLf synthesis

Implemented on-the-fly game construction algorithm based on
arXiv:2408.07324. The algorithm lazily expands the game graph
and can terminate early when realizability is determined.

Key changes:
- Added OnTheFlyGameSolver class
- Implemented lazy state expansion
- Added early termination for unrealizable formulas

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
```

### 5.3 文档更新提交

```
docs: reorganize TODO and BUGS into docs/ directory

- Create docs/TODO/ with module-specific task files
- Create docs/BUGS/ with open.md and fixed.md
- Simplify CLAUDE.md by removing inline sections

This improves organization and makes it easier to track
tasks and bugs by module.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
```
