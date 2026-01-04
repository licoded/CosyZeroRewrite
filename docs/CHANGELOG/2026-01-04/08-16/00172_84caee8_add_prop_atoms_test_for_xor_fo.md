# [172] test: add prop_atoms test for XOR formula (p & !q) | (!p & q)

**Commit**: `84caee8` ([`84caee8f09a1bc986e362933dfddf3e3dfa2e22f`](https://github.com/licoded/CosyZeroRewrite/commit/84caee8f09a1bc986e362933dfddf3e3dfa2e22f))
**Date**: 2026-01-04 10:30:31 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

添加对异或公式 (p & !q) || (!p & q) 的 prop_atoms 测试。
预期结果: {p, q} - Not 穿透后 And/Or 扩展为两个原子命题。

同时更新 CLAUDE.md 编译规范：强制使用 make -j$(nproc) 开启多线程编译。

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
添加 XOR 公式测试用例，同时规范编译命令必须使用多线程。

### 🔍 Technical Details
- **XOR 公式测试**: `(p & !q) | (!p & q)` 是异或(XOR)的标准展开形式
- **Prop atoms 语义**: Not 穿透(`PA(!φ)=PA(φ)`)，And/Or 取并集
- **测试结果**: 从 20 个测试用例增加到 21 个，断言从 62 增加到 67

### 📊 Impact Analysis
- **测试覆盖**: 新增 `[xor]` 标签，可单独运行该测试
- **编译规范**: 强制 `make -j$(nproc)` 避免编译过慢

## Changes

### Modified
- `CLAUDE.md`
- `tests/integration/prop_atoms.cpp`


## Stats

- **2** files changed
- **33** insertions(+)
- **2** deletions(-)
