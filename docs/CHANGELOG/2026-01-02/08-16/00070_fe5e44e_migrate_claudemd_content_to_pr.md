# [70] docs: migrate claude.md content to proper locations

**Commit**: `fe5e44e` ([`fe5e44ec8a86572cdadefc5b8df113d7f387e58c`](https://github.com/licoded/CosyZeroRewrite/commit/fe5e44ec8a86572cdadefc5b8df113d7f387e58c))
**Date**: 2026-01-02 11:28:54 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Delete claude.md (lowercase)
- Create docs/ARCHITECTURE/cmake.md (CMake modularization rules)
- Add BMC implementation details to docs/ARCHITECTURE/algorithms.md
- Add logging config details to docs/ARCHITECTURE/dependencies.md
- Create docs/ARCHITECTURE/adr/004-exception-hierarchy.md

This preserves valuable technical content from claude.md in
organized documentation files.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
文档重组：将 claude.md 的内容迁移到合适的文档位置，包括 CMake 模块化规则、BMC 实现细节、日志配置详情和异常层级 ADR。

### 🔍 Technical Details

**迁移路径**：
- `claude.md` (deleted) → 多个目标文档
- CMake 规则 → `docs/ARCHITECTURE/cmake.md`
- BMC 实现 → `docs/ARCHITECTURE/algorithms.md`
- 日志配置 → `docs/ARCHITECTURE/dependencies.md`
- 异常设计 → `docs/ARCHITECTURE/adr/004-exception-hierarchy.md`

## Changes

### Added
- `docs/ARCHITECTURE/adr/004-exception-hierarchy.md`
- `docs/ARCHITECTURE/cmake.md`


### Modified
- `docs/ARCHITECTURE/algorithms.md`
- `docs/ARCHITECTURE/dependencies.md`


### Deleted
- `claude.md`


## Stats

- **5** files changed
- **311** insertions(+)
- **155** deletions(-)
