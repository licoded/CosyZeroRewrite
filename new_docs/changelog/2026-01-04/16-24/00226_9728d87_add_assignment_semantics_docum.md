# [226] docs: add Assignment Semantics documentation and time record requirement

**Commit**: `9728d87` ([`9728d87cfef317801f11545e5ffd462868f3b0c9`](https://github.com/licoded/CosyZeroRewrite/commit/9728d87cfef317801f11545e5ffd462868f3b0c9))
**Date**: 2026-01-04 23:17:19 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Added documentation for Assignment Semantics design decision:
- sigma (σ) only contains variables set to TRUE
- Unselected variables are implicitly FALSE
- sys_move={p1} ≡ sys_move={p1, !p5}

Updated:
- src/automata/tableau.cpp: Added detailed comment (2026-01-04)
- docs/ARCHITECTURE/on_the_fly_algorithm.md: Added section 2.3 (2026-01-04)
- CLAUDE.md: Added documentation modification规范 (2026-01-04)

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
记录了 Assignment Semantics (赋值语义) 的关键设计决策，并规定今后所有文档修改都必须附带时间记录。

### 🔍 Technical Details
- **Assignment σ 表示方式**：只包含设置为 TRUE 的变量，未选择的变量隐式为 FALSE
  - 例如：`prop_atoms={p1,p2,p3,p5}, sys={p1,p5}` 时，`sys_move={p1}` 等价于 `p1=true, p5=false`
  - 这个设计简化了 move 表示，只需列出设置为 true 的变量
- **文档时间记录规范**：
  - 章节标题后添加日期 `(YYYY-MM-DD)`
  - 代码注释同步更新
  - 适用于架构文档、工作流程规范、CLAUDE.md 和关键代码注释

### 📊 Impact Analysis
- 无代码逻辑变更，仅文档更新
- 确保后续文档修改有明确的时间追溯

## Changes

### Modified
- `CLAUDE.md`
- `docs/ARCHITECTURE/on_the_fly_algorithm.md`
- `src/automata/tableau.cpp`


## Stats

- **3** files changed
- **98** insertions(+)
- **1** deletions(-)
