# Debugging Guide

> DFA/Game Graph 可视化调试方法

---

## DOT 格式输出

在 `on_the_fly_solver.cpp` 的 `is_realizable()` 中添加 DOT 输出：

```cpp
std::cerr << "=== Game Graph DOT ===" << std::endl;
std::cerr << "digraph Game {" << std::endl;
std::cerr << "  rankdir=LR;" << std::endl;

// 打印节点
for (const auto& pair : successors_) {
    const GameState& s = pair.first;
    bool is_accepting = dfa_.is_accepting(s.dfa_state);
    bool is_terminal = pair.second.empty();

    std::string shape = (s.player == Player::System) ? "box" : "ellipse";
    std::string color = is_accepting ? "green" : (is_terminal ? "blue" : "red");

    std::cerr << "  " << id << " [shape=\"" << shape << "\", color=\"" << color
              << "\", label=\"" << s.dfa_state->to_string() << "\"];" << std::endl;
}

// 打印边
for (const auto& pair : successors_) {
    for (const auto& succ : pair.second) {
        std::cerr << "  " << from_id << " -> " << to_id << ";" << std::endl;
    }
}

std::cerr << "}" << std::endl;
```

---

## 图例

| 形状 | 含义 |
|------|------|
| `box` | System 状态 |
| `ellipse` | Environment 状态 |

| 颜色 | 含义 |
|------|------|
| `green` | accepting 状态 |
| `red` | non-accepting 状态 |
| `blue` | terminal 状态 |

---

## 可视化工具

- **在线**: https://dreampuf.github.io/GraphvizOnline/
- **本地**: `dot -Tpng game.dot -o game.png`
