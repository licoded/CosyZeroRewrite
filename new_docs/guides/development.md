# Development Workflow

> Git 提交、Bug 修复、任务管理的标准流程

---

## Git 提交规范

### 提交频率原则

**原子性提交**：每次提交一个完整的逻辑单元

| 好的提交 | 避免的提交 |
|---------|-----------|
| `feat: implement Formula class` | `wip: various changes` |
| `fix: parser single-char operator` | `fix bugs and add features` |
| `docs: add bug fix workflow` | `update` |

### 提交消息格式

```
<type>: <简短描述>

<详细说明（可选）>

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
```

**Type 类型**：
- `feat` - 新功能
- `fix` - Bug 修复
- `docs` - 文档更新
- `refactor` - 代码重构
- `test` - 测试相关
- `chore` - 构建/工具
- `perf` - 性能优化

---

## Bug 修复流程

### 1. 记录 Bug

在 `new_docs/bugs/` 或 Issue 中记录：

```markdown
## [Bug #XXX] 简短标题

**优先级**: 🔴高 / 🟡中 / 🟢低
**位置**: `文件路径:行号`

**问题描述**: [详细描述]

**复现步骤**:
```bash
# 最小复现用例
```
```

### 2. 修复步骤

1. **理解问题** - 阅读相关代码和设计文档
2. **编写回归测试** - 先写测试，确保修复前失败
3. **实现修复** - 遵循最小改动原则
4. **运行全部测试** - 确保无回归

### 3. 回归测试规范

```cpp
TEST_CASE("Module: Bug #XXX - Description", "[regression][bugXXX]") {
    // 测试代码
}
```

---

## 任务管理

### 任务优先级

| 优先级 | 标准 | 响应时间 |
|--------|------|---------|
| 🔴 高 | 阻塞性问题 | 立即处理 |
| 🟡 中 | 重要功能 | 本周内 |
| 🟢 低 | 改进项 | 有时间再做 |

### 任务状态转换

```
pending → in_progress → completed
```

### 完成前检查

- [ ] 代码实现完成
- [ ] 编译通过
- [ ] 测试通过
- [ ] 文档已更新
