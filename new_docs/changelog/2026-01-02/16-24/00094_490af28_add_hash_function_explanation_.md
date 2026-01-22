# [94] docs: add hash function explanation and collision detection TODO

**Commit**: `490af28` ([`490af285acbb1fe6abc9289c112b66c2152dcfe7`](https://github.com/licoded/CosyZeroRewrite/commit/490af285acbb1fe6abc9289c112b66c2152dcfe7))
**Date**: 2026-01-02 17:57:11 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Added section 6.2 to components.md explaining:
- Boost.Hash combination algorithm principles
- Magic constant 0x9e3779b9 (golden ratio related)
- Why XOR-only hash is bad (order/duplication issues)
- Collision probability for 64-bit hash
- Algorithm source (Boost, C++ standard library)

Added low-priority TODO for runtime hash collision statistics
in docs/TODO/automata.md

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
<!-- TODO: Add a brief summary of the change in Chinese or English -->

### 🔍 Technical Details
<!-- Optional: Add technical details, root cause, or implementation notes -->

### 📊 Impact Analysis
<!-- Optional: Add impact scope, affected components, or performance notes -->

## Changes

### Modified
- `docs/ARCHITECTURE/components.md`
- `docs/TODO/automata.md`


## Stats

- **2** files changed
- **160** insertions(+)
- **3** deletions(-)
