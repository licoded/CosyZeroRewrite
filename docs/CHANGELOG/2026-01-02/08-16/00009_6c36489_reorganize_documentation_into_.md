# [9] docs: reorganize documentation into ARCHITECTURE and WORKFLOWS folders

**Commit**: `6c36489` ([`6c3648974c739e20de10c2e7381f32796285c917`](https://github.com/anthropics/cosy-zero/commit/6c3648974c739e20de10c2e7381f32796285c917))
**Date**: 2026-01-02 10:28:00 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Reorganized documentation for better structure and maintainability:

ARCHITECTURE/ folder:
- pipeline.md: LTLf Synthesis complete pipeline
- components.md: Formula, FormulaPool, TableauState design
- algorithms.md: Algorithm complexity analysis
- dependencies.md: External dependencies and versions
- roadmap.md: Implementation roadmap
- adr/: Architecture Decision Records (3 decisions)

WORKFLOWS/ folder:
- bug_fix.md: Bug fix process with flowchart
- todo_management.md: TODO tracking workflow
- commit_practice.md: Git commit conventions
- testing_strategy.md: Test coverage and strategy

CLAUDE.md changes:
- Simplified to project overview only
- Removed inline technical details
- Added references to detailed documentation
- Improved section organization

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `docs/ARCHITECTURE/adr/001-raw-pointers.md`
- `docs/ARCHITECTURE/adr/002-tableau-choice.md`
- `docs/ARCHITECTURE/adr/003-explicit-state-first.md`
- `docs/ARCHITECTURE/adr/README.md`
- `docs/ARCHITECTURE/algorithms.md`
- `docs/ARCHITECTURE/components.md`
- `docs/ARCHITECTURE/dependencies.md`
- `docs/ARCHITECTURE/pipeline.md`
- `docs/ARCHITECTURE/README.md`
- `docs/ARCHITECTURE/roadmap.md`
- `docs/WORKFLOWS/bug_fix.md`
- `docs/WORKFLOWS/commit_practice.md`
- `docs/WORKFLOWS/README.md`
- `docs/WORKFLOWS/testing_strategy.md`
- `docs/WORKFLOWS/todo_management.md`


### Modified
- `CLAUDE.md`


## Stats

- **16** files changed
- **1900** insertions(+)
- **226** deletions(-)
