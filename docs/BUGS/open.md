# Open Bugs / Known Issues

> 未修复的 Bug 和已知限制

---

## [Bug #001] 解析器：单字符操作符优先级问题

**发现日期**: 2026-01-02
**优先级**: 🟡 中
**状态**: 已确认

**位置**: `src/formula/formula_parser.cpp:133-136`

**问题描述**:
词法分析器在读取字母时，优先匹配单字符操作符：
- `r` → Release 操作符 (R)
- `f` → Finally 操作符 (F)
- `g` → Globally 操作符 (G)

**影响范围**:
- 变量名以 `r`/`f`/`g` 开头会被错误解析
- `req` → 被解析为 `R` + `eq`
- `fact` → 被解析为 `F` + `act`
- `goal` → 被解析为 `G` + `oal`

**复现步骤**:
```bash
./Cosy2 "req"  # 错误：Unexpected token: R
```

**临时解决方案**:
使用不以 `r`/`f`/`g` 开头的变量名：
- `req` → `quest` / `request`
- `fact` → `stmt` / `truth`
- `goal` → `target` / `aim`

**修复方案**:
修改 `tokenize()` 函数，在识别单字符操作符前先检查是否为多字符标识符：

```cpp
// 当前 (错误):
case 'r': token.type = TokenType::Release;

// 修复后:
case 'r':
    // 先 peek 下一个字符
    if (i+1 < input.size() && isalpha(input[i+1])) {
        // 继续读取完整标识符，然后判断是否为单独的 "r"
    } else {
        token.type = TokenType::Release;
    }
    break;
```

**相关文件**:
- `src/formula/formula_parser.cpp`
- `examples/response.ltlf` (使用 workaround)

---

## 统计

| 优先级 | 数量 |
|--------|------|
| 🔴 高 | 0 |
| 🟡 中 | 1 |
| 🟢 低 | 0 |
| **总计** | **1** |
