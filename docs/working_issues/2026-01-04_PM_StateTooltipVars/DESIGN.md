# State Tooltip 显示可用变量集合

**日期**: 2026-01-04 下午
**状态**: Design
**优先级**: Medium

---

## 需求

在节点 tooltip 中显示当前状态下**可用的变量集合**：

```
┌─────────────────────────────────────┐
│ S0 (initial)                        │
├─────────────────────────────────────┤
│ Classification: Unknown             │
│ Type: System                        │
│ Formula (phi): (true U p)           │
├─────────────────────────────────────┤
│ sys vars = {p}                      │  ← System 可从 {p} 中选子集
│ env vars = {}                       │  ← Environment 可选变量
└─────────────────────────────────────┘
```

## 数据来源

### 从现有数据推断（不需要后端修改）

**数据源**:
- `state_data.prop_atoms`: 当前状态的命题原子集
- `partition.outputs`: 全局输出变量（sys vars）
- `partition.inputs`: 全局输入变量（env vars）

**逻辑**:
1. 从 `prop_atoms` 中提取**字面量**（如 "p", "q"）
2. 过滤掉子公式（如 "X(...)", 含 |& 的复杂公式）
3. 用 `partition` 区分 outputs/inputs

### prop_atoms 示例

```json
"S0": {
  "prop_atoms": ["X((true U p))", "p"]
}
```

- "X((true U p))" → 子公式，不是变量
- "p" → 字面量，是变量

## 实现方案

### 前端函数

```typescript
/**
 * 从 prop_atoms 中提取字面量变量名
 */
function extractLiterals(propAtoms: string[]): string[] {
  return propAtoms.filter(atom => {
    const trimmed = atom.trim();
    // 排除子公式（包含 X(, |, &, 等）
    if (trimmed.startsWith('X(')) return false;
    if (trimmed.includes('|') || trimmed.includes('&')) return false;
    if (trimmed.startsWith('(')) return false;
    return trimmed.length > 0;
  });
}

/**
 * 获取当前状态的可用变量
 */
function getAvailableVars(state: StateData, partition: TracePartition) {
  const literals = extractLiterals(state.prop_atoms || []);
  const outputs = partition.outputs || [];
  const inputs = partition.inputs || [];

  const sysVars = literals.filter(l => outputs.includes(l));
  const envVars = literals.filter(l => inputs.includes(l));

  return { sysVars, envVars };
}
```

### 模板修改

```vue
<!-- 可用变量集合 -->
<div class="tooltip-row">
  <span class="tooltip-label">sys vars =</span>
  <span class="tooltip-value">{{ formatVarSet(availableVars.sysVars) }}</span>
</div>
<div class="tooltip-row">
  <span class="tooltip-label">env vars =</span>
  <span class="tooltip-value">{{ formatVarSet(availableVars.envVars) }}</span>
</div>
```

## 示例

### 公式: `(true U p)`, partition: outputs=[p], inputs=[]

| 状态 | prop_atoms | sys_vars | env_vars |
|------|-----------|----------|----------|
| S0 | `["X((true U p))", "p"]` | `{p}` | `{}` |
| E0 | `["X((true U p))", "p"]` | `{p}` | `{}` |
| S1 (phi=true) | `[]` | `{}` | `{}` |
| E1 (phi=true) | `[]` | `{}` | `{}` |

### 多变量公式

```
Formula: G (p1 -> F ack)
Partition: inputs=[p1], outputs=[ack]
```

| 状态 | prop_atoms | sys_vars | env_vars |
|------|-----------|----------|----------|
| S0 | `["ack", "p1", ...]` | `{ack}` | `{p1}` |
| E0 | `["ack", "p1", ...]` | `{ack}` | `{p1}` |

## 关键点

1. **变量随状态变化**: 不是全局 partition，是当前状态 prop_atoms 中可用的
2. **字面量提取**: 只提取原子变量，过滤复杂子公式
3. **用 partition 区分**: outputs → sys_vars, inputs → env_vars
4. **空集处理**: phi=true 时 prop_atoms=[]，显示 sys_vars={}, env_vars={}
