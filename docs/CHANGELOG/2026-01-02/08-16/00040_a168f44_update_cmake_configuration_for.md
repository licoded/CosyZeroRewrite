# [40] build: update CMake configuration for synthesis module

**Commit**: `a168f44` ([`a168f4470aada7f172f19be958425902e9d2e741`](https://github.com/anthropics/cosy-zero/commit/a168f4470aada7f172f19be958425902e9d2e741))
**Date**: 2026-01-02 09:14:03 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Update build configuration to include new synthesis components.

- Add automata and synthesis libraries
- Add on_the_fly_synthesis_tests target
- Link synthesis tests with formula, automata, synthesis libraries
- Enable benchmark runner

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Modified
- `cmake/Dependencies.cmake`
- `CMakeLists.txt`
- `cmake/Tests.cmake`


## Stats

- **3** files changed
- **45** insertions(+)
- **1** deletions(-)
