# Fixed Bugs / Resolved Issues

> 已修复的 Bug 和已解决的问题

---

## [Bug #003] Synthesis: 状态传播规则和 SCC 分类算法错误

**发现日期**: 2026-01-02
**修复日期**: 2026-01-02
**影响版本**: v1.0
**优先级**: 🔴 高

**问题描述**:
两个严重错误导致状态分类不正确：

1. **Environment 状态传播规则错误** (line 442-449)
   - 错误：`all_ewin → Ewin`, `has_swin → Swin`
   - 正确：`has_ewin → Ewin`, `all_swin → Swin`

2. **SCC 分类算法错误**
   - 错误：整个 SCC 统一标记为相同分类
   - 正确：使用不动点迭代分别判断每个状态

**修复方案**:

1. **修复传播规则** (`src/synthesis/on_the_fly_solver.cpp:420-453`):
```cpp
// Environment 回合：Environment 选择输入
// 任一后继是 Ewin → Environment 选它 → 当前是 Ewin
// 所有后继是 Swin → Environment 没法避免 → 当前是 Swin
if (has_ewin) {
    classification_[state] = StateClass::Ewin;
} else if (all_swin && !succs.empty()) {
    classification_[state] = StateClass::Swin;
}
```

2. **实现 SCC 内部不动点算法** (`classify_scc`):
   - 初始化：接受状态作为 Swin 种子
   - 迭代：找前继，用传播规则判断新的 Swin
   - 终止：没有新的 Swin
   - 剩余：标记为 Ewin

**测试结果**:
- `synthesis_tests`: 21 assertions, 8 test cases ✅
- `on_the_fly_synthesis_tests`: 12/12 tests passed ✅

**相关提交**: 待提交

---

## [Bug #002] 解析器：单字符操作符优先级问题

**发现日期**: 2026-01-02
**修复日期**: 2026-01-02
**影响版本**: v1.0
**优先级**: 🟡 中

**问题描述**:
词法分析器在读取字母时，优先匹配单字符操作符，导致以 `r`/`f`/`g` 开头的变量名被错误解析：
- `r` → Release 操作符 (R)
- `f` → Finally 操作符 (F)
- `g` → Globally 操作符 (G)

**影响范围**:
- `req` → 被解析为 `R` + `eq` (错误)
- `fact` → 被解析为 `F` + `act` (错误)
- `goal` → 被解析为 `G` + `oal` (错误)

**修复方案**:
在 `tokenize()` 函数中添加 **peek-ahead** 逻辑：
- 遇到 `r`/`f`/`g` 时，先检查下一个字符是否为字母
- 如果是字母，则作为标识符开头处理
- 如果不是或只有单个字符，才作为操作符处理

**修复代码** (`src/formula/formula_parser.cpp:135-156`):
```cpp
// Handle ambiguous single-char operators that could start identifiers
// 'r' could be Release (R) or start of identifier like "req", "read"
// 'f' could be Finally (F) or start of identifier like "fact", "flag"
// 'g' could be Globally (G) or start of identifier like "goal", "get"
// Peek ahead: if followed by another letter, it's an identifier start
if ((c == 'R' || c == 'r' || c == 'F' || c == 'f' || c == 'G' || c == 'g') &&
    i + 1 < input.size() &&
    std::isalpha(static_cast<unsigned char>(input[i + 1]))) {
    // This is the start of a multi-character identifier, fall through to identifier handling
} else if (c == 'R' || c == 'r') {
    token.type = TokenType::Release;
    token.value = "R";
    tokens_.push_back(token);
    ++i;
    continue;
} else if (c == 'F' || c == 'f') {
    // F is handled in the identifier section for syntactic sugar consistency
    // Fall through to identifier handling
} else if (c == 'G' || c == 'g') {
    // G is handled in the identifier section for syntactic sugar consistency
    // Fall through to identifier handling
}
```

**回归测试**:
在 `tests/parser_checker_tests.cpp` 中添加了 5 个回归测试用例：
- `Parser: Bug #001 - Variables starting with 'r'`
- `Parser: Bug #001 - Variables starting with 'f'`
- `Parser: Bug #001 - Variables starting with 'g'`
- `Parser: Bug #001 - Single-char R/F/G still work as operators`
- `Parser: Bug #001 - Mixed formulas work correctly`

**相关提交**: 待提交

---

## [Bug #001] TableauState 接受状态条件错误

**修复日期**: 2026-01-02
**影响版本**: v1.0

**问题**:
`is_accepting()` 对 Until 公式的检查不正确：
```cpp
// 错误：只要 ψ₁ 或 ψ₂ 有一个在 Γ 中就接受
if (formulas_.count(right) == 0 && formulas_.count(left) == 0)
    return false;
```

**修复**:
在 LTLf 中，`ψ₁ U ψ₂` 要求 ψ₂ 必须最终为真。如果状态中有 `ψ₁ U ψ₂` 但 ψ₂ 不在状态中，说明 Until 还在等待，不能是接受状态。
```cpp
// 正确：必须有 ψ₂ 才能接受
if (formulas_.count(right) == 0)
    return false;  // Until still waiting for right side
```

**相关提交**: `97e2f8f` - fix: correct accepting state condition for Until formulas

---

## 统计

| 年份 | 修复数量 |
|------|---------|
| 2026 | 3 |
| **总计** | **3** |
