# LTLf 基础知识与核心概念

## 1. LTLf 的定义 (Linear Temporal Logic on Finite Traces)
$LTL_{f}$ 是为了适应有限行为（Finite Behaviors）场景（如 AI 规划、约束验证）而设计的，其语义在**有限迹**上进行解释。

### 语法 (Syntax)
给定原子命题集合 $\mathcal{P}$，$LTL_{f}$ 公式的形式定义如下：

$$
\phi ::= tt \mid p \mid \neg \phi \mid \phi \wedge \phi \mid \mathcal{X} \phi \mid \phi \mathcal{U} \phi
$$

* **基本算子**: $\mathcal{X}$ (Strong Next, 强 Next), $\mathcal{U}$ (Until).
* **派生算子**:
    * $ff$ (False), $\vee$ (Or).
    * $\mathcal{R}$ (Release): $\mathcal{U}$ 的对偶算子。
    * $\mathcal{N}$ (Weak Next, 弱 Next): $\mathcal{X}$ 的对偶算子。
    * $\mathcal{G}$ (Globally), $\mathcal{F}$ (Eventually).

### 语义关键点：强 Next vs 弱 Next
在有限迹的语义下，$\mathcal{X}$ 和 $\mathcal{N}$ 在迹的末尾表现不同：
* **$\mathcal{X} \phi$ (Strong Next)**: 要求必须存在下一个时刻，且下一时刻满足 $\phi$。如果在迹的最后一个状态求值 $\mathcal{X} \phi$，结果永远为 **False**。
* **$\mathcal{N} \phi$ (Weak Next)**: 如果存在下一个时刻，则要求下一时刻满足 $\phi$；如果当前是迹的最后一个状态（没有后继），结果自动为 **True**。

---

## 2. LTLf 可满足性问题 (Satisfiability Problem)
**定义**: 给定一个 $LTL_{f}$ 公式 $\phi$，可满足性问题是询问是否存在一个**有限非空迹 (finite nonempty trace)** $\xi$，使得 $\xi \models \phi$。

* **传统解法**: 将 $LTL_{f}$ 归约为标准 LTL 问题，寻找“lasso”（无限循环），但这对于寻找有限迹模型来说存在额外开销。
* **论文解法**: 使用基于 SAT 的显式状态搜索，直接在有限迹上工作。

---

## 3. 关键范式 (Normal Forms)
为了构建变迁系统，论文利用了两种范式。

### 3.1 NNF (Negated Normal Form) 与 TNF
* **NNF**: 所有的否定符号 $\neg$ 仅出现在原子命题前。
* **TNF (Tail Normal Form)**: 为了处理有限迹的结束，论文引入了一个特殊变量 $Tail$ 来标记迹的最后一个状态。公式被转换为 $tnf(\phi) = t(\phi) \wedge \mathcal{F}(Tail)$。

### 3.2 XNF (neXt Normal Form)
这是构建变迁系统的核心格式。
* **定义**: 一个公式处于 XNF，当且仅当其原子集合 $PA(\phi)$ 中不包含 $\mathcal{U}$ (Until) 或 $\mathcal{R}$ (Release) 形式的子公式。
* **转换规则**: 利用递归展开消除 $\mathcal{U}$ 和 $\mathcal{R}$，将其转化为当前状态和 Next 状态的组合：
    * $\phi_1 \mathcal{U} \phi_2 \equiv \phi_2 \vee (\phi_1 \wedge \mathcal{X}(\phi_1 \mathcal{U} \phi_2))$
    * $\phi_1 \mathcal{R} \phi_2 \equiv \phi_2 \wedge (\phi_1 \vee \mathcal{X}(\phi_1 \mathcal{R} \phi_2))$
* **目的**: XNF 将公式分离为“当前约束”和“下一时刻约束”，使得可以用 SAT 求解器计算状态转移。

---

## 4. 构建变迁系统 (LTLf Transition System)
论文并未预先构建完整的自动机（NFA/DFA），而是利用 SAT 求解器**在线 (On-the-fly)** 构建等价的变迁系统 $T_{\phi}$。这相当于将 $LTL_{f}$ 转换为自动机/图的过程。

### 系统定义 $T_{\phi} = (S, s_0, T)$
* **状态 ($S$)**: 每个状态 $s$ 是一个子公式集合（$\phi$ 的子公式），代表当前需要满足的约束。
    * 初始状态 $s_0 = \{\phi\}$。
* **变迁 ($T$)**: 利用 SAT 求解器计算。
    1.  将当前状态 $s$ 的公式合取并转为 XNF：$xnf(\bigwedge s)$。
    2.  将其视为布尔公式 $\phi^p$，其中 $\mathcal{X}\psi$ 被视为独立的布尔变量。
    3.  SAT 求解器返回一个赋值 $A$。
    4.  **后继状态**: $s_{next} = \{\psi \mid \mathcal{X}\psi \in A\}$（即赋值中所有被标记为真的 Next 子公式的内部部分）。
    5.  **边上的 Label**: 赋值 $A$ 中的字面量（Literal）部分。

### 终态与可满足性判定
* **终态 (Final State)**: 一个状态 $s$ 是终态，当且仅当 $Tail \wedge xnf(s)^p$ 是可满足的。
    * 这意味着当前状态可以合法地作为迹的终点（满足所有约束，且没有强制要求后续的 $\mathcal{X}$ 约束）。
* **判定**: $\phi$ 是可满足的，当且仅当在 $T_{\phi}$ 中存在一条从 $s_0$ 到某个终态的可达路径。