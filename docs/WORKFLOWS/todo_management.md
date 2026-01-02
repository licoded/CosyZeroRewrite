# TODO 任务管理流程

> 任务创建、跟踪、完成的标准流程

---

## 1. 任务组织结构

```
docs/TODO/
├── README.md       # 总览和模板
├── parser.md       # Parser 模块待办
├── synthesis.md    # Synthesis 模块待办
├── automata.md     # Automata 模块待办
└── general.md      # 通用待办事项
```

---

## 2. 创建任务

### 2.1 判断任务归属

| 类型 | 归属文件 |
|------|---------|
| Parser 相关 | `parser.md` |
| Synthesis/游戏求解 | `synthesis.md` |
| DFA/Tableau | `automata.md` |
| 测试、文档、构建 | `general.md` |

### 2.2 任务模板

```markdown
### [#XXX] 任务标题

**状态**: 待办 / 进行中 / 已完成
**优先级**: 🔴高 / 🟡中 / 🟢低
**预计时间**: X 小时/天

**描述**:
[任务目标说明]

**子任务**:
- [ ] 子任务 1
- [ ] 子任务 2
- [ ] 子任务 3

**依赖**:
[前置任务或依赖资源]

**相关文件**:
[涉及的代码文件]
```

---

## 3. 跟踪任务

### 3.1 使用 TodoWrite 工具

```javascript
// 任务开始时
TodoWrite({
  todos: [
    { content: "Fix parser bug", status: "in_progress", activeForm: "Fixing parser bug" },
    { content: "Add tests", status: "pending", activeForm: "Adding tests" }
  ]
})

// 任务完成时
TodoWrite({
  todos: [
    { content: "Fix parser bug", status: "completed", activeForm: "Fixed parser bug" },
    { content: "Add tests", status: "in_progress", activeForm: "Adding tests" }
  ]
})
```

### 3.2 状态转换

```
pending → in_progress → completed
    ↑                        ↓
    └────────────────────────┘
      (如果阻塞或取消)
```

---

## 4. 完成任务

### 4.1 完成前检查清单

- [ ] 代码实现完成
- [ ] `make` 编译通过
- [ ] 所有测试通过
- [ ] 新增功能有测试覆盖
- [ ] 文档已更新

### 4.2 完成后操作

1. **更新文档**: 在 `docs/TODO/*.md` 中标记为已完成
2. **TodoWrite**: 标记任务为 `completed`
3. **提交代码**: `git commit` 带清晰描述
4. **CHANGELOG**: 提交后自动生成到 `docs/CHANGELOG/` (git hook)

### 4.3 CHANGELOG 自动生成

每次 `git commit` 后，post-commit hook 会自动生成 CHANGELOG：

```
git commit -m "feat: add new feature"
# → 自动生成: docs/CHANGELOG/00017_abc123_add_new_feature.md
```

**禁用方法**:
```bash
# 临时禁用
NO_CHANGELOG=1 git commit -m "message"

# 跳过 hook
git commit --no-verify -m "message"
```

**手动生成**:
```bash
make changelog                    # 最新提交
make changelog COMMIT=HEAD~2      # 指定提交
```

详见: [CHANGELOG README](../../CHANGELOG/README.md)

### 4.4 提交格式

```
<type>: <description>

<details if needed>

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
```

---

## 5. 任务优先级指南

| 优先级 | 标准 | 响应时间 |
|--------|------|---------|
| 🔴 高 | 阻塞性问题、用户反馈的 Bug | 立即处理 |
| 🟡 中 | 重要功能、计划内的任务 | 本周内 |
| 🟢 低 | 改进项、优化 | 有时间再做 |
