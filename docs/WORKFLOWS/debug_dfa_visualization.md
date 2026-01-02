# DFA/Game Graph 可视化调试方法

## 概述

在调试 LTLf Synthesis 时，可视化游戏图（Game Graph）可以帮助理解：
- DFA 状态的内容
- 游戏状态的转换
- 哪些状态是 accepting/terminal

## 使用方法

在 `on_the_fly_solver.cpp` 的 `is_realizable()` 函数中，Phase 1 完成后添加 DOT 格式输出：

```cpp
// Debug: output game graph in DOT format for visualization
std::cerr << "=== Game Graph DOT ===" << std::endl;
std::cerr << "digraph Game {" << std::endl;
std::cerr << "  rankdir=LR;" << std::endl;

int state_id = 0;
std::unordered_map<GameState, int, GameStateHash, GameStateEqual> state_ids;
for (const auto& pair : successors_) {
    state_ids[pair.first] = state_id++;
}

// Print nodes
for (const auto& pair : successors_) {
    const GameState& s = pair.first;
    int id = state_ids[s];
    const std::vector<GameState>& succs = pair.second;

    bool is_accepting = dfa_.is_accepting(s.dfa_state);
    bool is_terminal = succs.empty();

    std::string dfa_formulas = s.dfa_state->to_string();
    if (dfa_formulas.length() > 50) {
        dfa_formulas = dfa_formulas.substr(0, 47) + "...";
    }

    std::string shape = (s.player == Player::System) ? "box" : "ellipse";
    std::string color = is_accepting ? "green" : "red";
    if (is_terminal) color = "blue";

    std::cerr << "  " << id << " [shape=\"" << shape << "\", color=\"" << color
              << "\", label=\"" << id << " (" << (s.player == Player::System ? "Sys" : "Env")
              << ")\\nDFA: " << dfa_formulas
              << "\\nacc=" << is_accepting
              << ", term=" << is_terminal
              << "\"];" << std::endl;
}

// Print edges
for (const auto& pair : successors_) {
    const GameState& s = pair.first;
    int from_id = state_ids[s];
    const std::vector<GameState>& succs = pair.second;

    for (const auto& succ : succs) {
        int to_id = state_ids[succ];
        std::cerr << "  " << from_id << " -> " << to_id << ";" << std::endl;
    }
}

std::cerr << "}" << std::endl;
std::cerr << "=== End Game Graph DOT ===" << std::endl;
```

## 输出示例

对于公式 `!p1`（p1 是系统变量）：

```
digraph Game {
  rankdir=LR;
  0 [shape="ellipse", color="blue", label="0 (Env)\nDFA: {!v0}\nacc=1, term=1"];
  1 [shape="ellipse", color="blue", label="1 (Env)\nDFA: {!v0}\nacc=1, term=1"];
  2 [shape="box", color="green", label="2 (Sys)\nDFA: {!v0}\nacc=1, term=0"];
  2 -> 0;
  2 -> 1;
}
```

## 图例

- **形状**: 
  - `box`: System 状态
  - `ellipse`: Environment 状态
- **颜色**:
  - `green`: accepting 状态
  - `red`: non-accepting 状态
  - `blue`: terminal 状态
- **标签**:
  - `acc=1`: accepting
  - `term=1`: terminal (无后继)

## 可视化工具

可以使用以下工具查看 DOT 图：
- 在线: https://dreampuf.github.io/GraphvizOnline/
- 本地: `dot -Tpng game.dot -o game.png`

## 使用场景

1. **调试单个公式**：理解游戏图结构
2. **验证修改**：对比修改前后的游戏图
3. **分析边界案例**：找出为什么某些公式返回错误结果
