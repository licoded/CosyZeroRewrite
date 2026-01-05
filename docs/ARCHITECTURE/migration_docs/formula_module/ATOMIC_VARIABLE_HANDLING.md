# Atomic Variable Handling in Current Implementation

**Date**: 2025-01-01
**Status**: Analysis and Discussion Record

---

## 1. Current Implementation Analysis

### 1.1 Static Data Structures

The current implementation uses **global static variables** to manage atomic variables (literals):

**In [aalta_formula.h:143-144](../../lib/deps/formula/aalta_formula.h#L143-L144)**:
```cpp
static std::vector<std::string> names;  // Store operator names and atomic variable names
static hash_map<std::string, int> ids;   // Name to position mapping
```

**In [aalta_formula.h:225](../../lib/deps/formula/aalta_formula.h#L225)**:
```cpp
static int _max_id;  // Maximum ID counter
```

### 1.2 Initialization Process

**In [aalta_formula.cpp:706-720](../../lib/deps/formula/aalta_formula.cpp#L706-L720)**:
```cpp
if (names.empty())
{
    // Pre-reserve IDs 0-11 for operators and constants
    names.push_back ("true");       // ID 0
    names.push_back ("false");      // ID 1
    names.push_back ("Literal");    // ID 2
    names.push_back ("!");          // ID 3
    names.push_back ("|");          // ID 4
    names.push_back ("&");          // ID 5
    names.push_back ("X[!]");       // ID 6
    names.push_back ("X");          // ID 7 (weak Next, for LTLf)
    names.push_back ("U");          // ID 8
    names.push_back ("R");          // ID 9
    names.push_back ("Undefined");  // ID 10
}
```

**Key Point**: IDs 0-10 are **pre-allocated** for operators and constants.

### 1.3 Atomic Variable Creation

**In [aalta_formula.cpp:1152-1168](../../lib/deps/formula/aalta_formula.cpp#L1152-L1168)**:
```cpp
inline void
aalta_formula::build_atom (const char *name, bool is_not)
{
    int id;
    hash_map<std::string, int>::const_iterator it = ids.find (name);

    if (it == ids.end ())
    {
        // Variable name not seen before, add it
        id = names.size ();          // Next available ID
        ids[name] = id;              // Record name->ID mapping
        names.push_back (name);      // Append name to vector
    }
    else
    {
        // Variable already exists, reuse ID
        id = it->second;
    }

    if (is_not)
        _op = Not, _right = aalta_formula (id, NULL, NULL).unique ();
    else
        _op = id;
}
```

### 1.4 Variable Name to ID Resolution

**In [aalta_formula.cpp:50-53](../../lib/deps/formula/aalta_formula.cpp#L50-L53)**:
```cpp
std::string
aalta_formula::get_name (int index)
{
    return names[index];
}
```

### 1.5 How Variables Are Created (From Input)

**Variable names come from parsing LTLf formulas**:

1. Input files (e.g., `tools/benchmarks/sm1000/bench1/f78.ltlf`):
   ```
   (p3) | (X((p6) & ((p8) | (X(!(p8)))))) | ((p9) & (F(!(p8))))
   ```

2. Variables like `p3`, `p6`, `p8`, `p9` are **dynamically created** when:
   - Parser encounters them in formula string
   - `build_atom()` is called with the variable name
   - New ID is assigned (ID >= 11) if first occurrence
   - Same ID is reused if variable already seen

**Example Flow**:
```
Input: "p3 | p6"
→ Parser sees "p3"
→ build_atom("p3", false)
→ ids = {"p3": 11}, names = [..., "p3"]  (ID 11)
→ Parser sees "p6"
→ build_atom("p6", false)
→ ids = {"p3": 11, "p6": 12}, names = [..., "p3", "p6"]  (ID 12)
```

### 1.6 Special Case: Until Formula Variables

**In [aalta_formula.cpp:2235-2237](../../lib/deps/formula/aalta_formula.cpp#L2235-L2237)**:
```cpp
// Auto-generated variables for Until formulas
std::string name = "FOR_UNTIL_";
int pos = _until_map.size ();
name += convert_to_string (pos);
id = names.size ();
ids[name] = id;
names.push_back (name);
```

These are **synthetic variables** created during XNF transformation to represent Until subformulas.

---

## 2. Current Design Evaluation

### 2.1 Strengths

1. **Automatic Deduplication**: `hash_map<std::string, int> ids` ensures same variable name gets same ID
2. **Simple API**: `get_name(id)` and `build_atom(name)` are straightforward
3. **Global Scope**: All formulas share same variable space, consistent naming across entire DFA
4. **Zero Configuration**: No need to pre-declare variables

### 2.2 Weaknesses

1. **Global Mutable State**:
   - `static std::vector<std::string> names` is global
   - `static hash_map<std::string, int> ids` is global
   - Violates modern C++ best practices (hard to test, thread-unsafe)
   - **Cannot handle multiple DFA contexts simultaneously** (though user noted only single DFA needed)

2. **No Lifecycle Management**:
   - Variables persist forever once created
   - No way to clear variables between DFA constructions
   - Memory grows with number of unique variables seen

3. **Tight Coupling**:
   - Variable management is tightly coupled to `aalta_formula` class
   - Hard to replace with BDD (which has its own variable ordering)

4. **Mixed Responsibilities**:
   - `names` vector stores both operators (IDs 0-10) AND variables (IDs 11+)
   - Confusing: `get_name(5)` returns "&", not a variable name
   - Hard to distinguish "is this ID a variable or operator?"

5. **Inefficient for BDD Migration**:
   - BDD requires variables to be pre-allocated with fixed ordering
   - Current design assigns IDs on-the-fly as variables appear
   - Variable order depends on parsing order, not semantic grouping

---

## 3. Comparison: treeFormula vs bddFormula

### 3.1 treeFormula Approach (Current Design)

**Variable Allocation**: **On-demand** (lazy allocation)

- Variables are created when first seen in formula string
- IDs assigned sequentially: 11, 12, 13, ...
- Variable order = parsing order
- No pre-allocation needed

**Pros**:
- Simple, zero configuration
- Works for any variable names
- No wasted space for unused variables

**Cons**:
- Variable order unpredictable (depends on parsing)
- Hard to migrate to BDD (BDD needs fixed ordering)
- Global mutable state

### 3.2 bddFormula Approach (Hypothetical)

**Variable Allocation**: **Pre-allocated** (eager allocation)

- All variables (x1, x2, ..., y1, y2, ...) declared upfront
- IDs assigned in fixed order (e.g., x1=0, x2=1, y1=100, y2=101)
- Variable order = declared order (semantic grouping)
- Must know variable set in advance

**Pros**:
- Consistent variable ordering
- Easy to migrate to BDD (BDD variables match tree variables)
- No global mutable state (can be per-DFA)
- Better cache locality (sequential IDs)

**Cons**:
- Must know all variables before parsing
- Unused variables waste space
- Requires variable declaration phase

---

## 4. Key Design Question

**Question**: Should atomic variables (x1, x2, y1, y2, ...) be pre-allocated in treeFormula design?

### 4.1 Context

From the formula module redesign, we know:
- **Future goal**: May replace treeFormula with BDD
- **Current constraint**: Only single DFA context needed
- **Priority**: Sub-formula sharing and deduplication

### 4.2 Analysis

**Arguments FOR Pre-allocation**:
1. **BDD Compatibility**: Pre-allocated variables match BDD's expected design
2. **Consistent Ordering**: Variables have stable IDs regardless of parsing order
3. **Per-DFA Scoping**: Can move global state into FormulaPool (per-DFA context)
4. **Performance**: Sequential IDs = better cache locality

**Arguments AGAINST Pre-allocation**:
1. **Requires Variable Declaration**: Must parse partition file BEFORE formula file
2. **Wasted Space**: Allocate IDs for variables that might not appear in formula
3. **Complexity**: Need two-phase parsing (declare variables, then parse formula)
4. **Limited Benefit for Tree**: Tree formulas don't care about variable ordering (only BDD does)

### 4.3 Current Code's Implicit Decision

The current implementation **does NOT pre-allocate variables**:
- Variables are created on-demand in `build_atom()`
- IDs assigned sequentially as variables appear
- Variable order = parsing order

This suggests the original author valued simplicity over BDD compatibility.

---

## 5. Updated Implementation Design

### 5.1 API Design

```cpp
class FormulaPool {
private:
    std::vector<std::string> var_names_;  // Only variables (not operators)
    hash_map<std::string, int> var_ids_;  // name -> ID mapping
    int num_outputs_;                      // Count of output (system) variables
    int num_inputs_;                       // Count of input (environment) variables
    bool outputs_declared_;
    bool inputs_declared_;

public:
    // Method 1: Declare from partition file (preferred)
    void load_from_partition(const std::string& partition_file_path);

    // Method 2: Declare from parameter lists (flexible API)
    void declare_variables(const std::vector<std::string>& outputs,
                          const std::vector<std::string>& inputs);

    // Method 3: Declare separately (order doesn't matter)
    void declare_outputs(const std::vector<std::string>& outputs);
    void declare_inputs(const std::vector<std::string>& inputs);

    // Method 4: Auto-extract from formula (fallback for testing)
    void extract_variables_from_formula(Formula* formula_root);

    // Validation
    bool is_fully_declared() const {
        return outputs_declared_ && inputs_declared_;
    }

    // Variable lookup
    int get_variable_id(const std::string& name) const;

    // Variable info
    bool is_output_variable(int var_id) const {
        return var_id >= 0 && var_id < num_outputs_;
    }

    bool is_input_variable(int var_id) const {
        return var_id >= num_outputs_ && var_id < num_outputs_ + num_inputs_;
    }

    // Formula creation
    Formula* create_variable(const std::string& name);
};
```

### 5.2 Variable Declaration Flow

**Scenario 1: With Partition File** (normal use case)
```
1. Read partition file:
   .outputs: s1 s2 s3
   .inputs:  p1 p3 p5

2. FormulaPool::declare_outputs({"s1", "s2", "s3"})

3. FormulaPool::declare_inputs({"p1", "p3", "p5"})

4. Validate: is_fully_declared() == true

5. Parse formula using pre-declared variables
```

**Scenario 2: Without Partition File** (testing use case)
```
1. Parse formula string: "(p3) | (X(p6 & p8))"

2. Build formula tree

3. FormulaPool::extract_variables_from_formula(formula_root)
   → Extracts: {p3, p6, p8}
   → All treated as outputs

4. Validate: is_fully_declared() == true

5. No inputs (environment variables)
```

**Scenario 3: Mixed Order Declaration**
```
1. FormulaPool::declare_inputs({"p1", "p3"})
   → inputs_declared_ = true, outputs_declared_ = false

2. FormulaPool::declare_outputs({"s1", "s2"})
   → inputs_declared_ = true, outputs_declared_ = true

3. Validate: is_fully_declared() == true

4. Proceed with parsing
```

### 5.3 Implementation Priority

**High Priority** (Must implement):
1. Move variable management into FormulaPool
2. Per-DFA variable scoping
3. Variable name → ID mapping

**Medium Priority** (Must implement):
4. Pre-allocate variables from partition file
5. Two-phase parsing (declare then parse)
6. Validation (is_fully_declared)

**Low Priority** (Must implement, can defer):
7. Separate operator enum from variable IDs
8. Output-first ordering (outputs before inputs)
9. Fallback auto-extraction for testing

**All priorities must be implemented** - low priority means "can defer implementation", not "optional".

### 5.4 Variable ID Strategy

**Current**: Mixed operator/variable IDs
```
IDs 0-10: Operators and constants
IDs 11+:  Variables
```

**Recommended**: Separate operator and variable IDs
```cpp
class Formula {
    int op_;      // Operator type (enum: And, Or, Not, Literal, ...)
    int var_id_;  // Variable ID (only used when op_ == Literal)
};

// Operators are in separate enum (no IDs)
// Variables start from ID 0 (clean separation)
```

**Benefits**:
- Clear semantic: `op_ == Literal` means this is a variable
- No confusion: `get_name(5)` won't return "&"
- Easy to iterate variables: `for (int i = 0; i < num_vars; ++i)`

### 5.3 Synthetic Variables (FOR_UNTIL_*)

**Current approach**: Auto-generated during XNF transformation

**Recommendation**: Keep this approach even with pre-allocation
- These are temporary variables, not user-visible
- Create them on-demand in a separate "synthetic variable" namespace
- Or eliminate them entirely (alternative XNF implementation)

---

## 6. Design Rationale

### 6.1 Why Output-First Ordering?

**Question**: Why allocate outputs (system) before inputs (environment)?

**Answer**:
1. **System-centric perspective**: In synthesis, we synthesize the system/agent (outputs)
2. **Primary focus**: Outputs are what we control and design strategies for
3. **BDD alignment**: When using BDD for synthesis, system variables are typically ordered first
4. **Intuition**: It's natural to think "what the system does" before "what the environment does"

**Example**:
```
Formula: "The system (s1, s2) must ensure safety regardless of environment (p1, p2)"

Variable ordering (output-first):
  s1 → ID 0  (system action 1)
  s2 → ID 1  (system action 2)
  p1 → ID 2  (environment input 1)
  p2 → ID 3  (environment input 2)
```

### 6.2 Why Support Auto-Extraction?

**Question**: Why allow formulas without partition files?

**Answer**:
1. **Testing convenience**: Unit tests can test formula transformations without partition boilerplate
2. **Exploratory analysis**: Quick formula evaluation without variable classification
3. **Backward compatibility**: Existing test cases may not have partition files
4. **Simplification**: Some formula operations (e.g., simplification, XNF) don't need environment/system distinction

**Use Case Example**:
```cpp
// Test simplification without partition
Formula* f = Formula::parse("(p1 & p2) | p1");
Formula* simplified = f->simplify();
// No partition needed - all variables treated as outputs
```

### 6.3 Why Flexible Declaration Order?

**Question**: Why allow declare_inputs() before declare_outputs() or vice versa?

**Answer**:
1. **API flexibility**: Different callers may have data in different orders
2. **Incremental building**: Can declare variables as they become available
3. **Error resilience**: Can declare in any order, validation happens at is_fully_declared()
4. **Implementation simplicity**: Internal ordering is fixed regardless of declaration order

**Example**:
```cpp
// Both are equivalent:
pool.declare_outputs({"s1", "s2"});
pool.declare_inputs({"p1", "p2"});

// OR (order doesn't matter):
pool.declare_inputs({"p1", "p2"});
pool.declare_outputs({"s1", "s2"});

// Internal state is always: outputs first, then inputs
```

---

## 7. Final Decision: Option B (Pre-allocate Variables) ⭐

**Decision Date**: 2025-01-01
**Decision**: **选择方案B - 预分配变量**

### User Decision

After reviewing the analysis, user decided to adopt **Option B (Pre-allocate Variables)** for the treeFormula redesign.

### Background Knowledge: LTLf Synthesis Context

**Environment vs System Variables**:
- **Inputs (环境变量)**: Variables controlled by the environment
- **Outputs (系统变量)**: Variables controlled by the system/agent
- **Agent**: Refers to the system (outputs) that we synthesize strategies for

**LTLf Synthesis Definition**:
- Given an LTLf formula over environment (inputs) and system (outputs) variables
- Goal: Find a strategy for the system (outputs) such that the formula is satisfied
  - **regardless** of how the environment (inputs) behaves
- The system/agent must react correctly to all possible environment behaviors

**Partition File Example**:
```
.inputs: p1 p3 p5      # Environment variables (uncontrollable)
.outputs: s1 s2 s3      # System/agent variables (controllable)
```

### Rationale

User confirmed the following key points:

1. **Two-phase parsing is acceptable**:
   - Phase 1: Read partition file → declare variables (inputs + outputs)
   - Phase 2: Parse formula file → use pre-declared variables
   - This aligns with BDD migration goals
   - **Ordering flexibility**: Can parse inputs before outputs OR outputs before inputs
   - **Validation**: Must check both inputs and outputs are available before proceeding

2. **Input/output grouping should be maintained**:
   - **All output (system/agent) variables allocated first** (s1, s2, s3, ...)
   - **All input (environment) variables allocated after** (p1, p3, p5, ...)
   - Fixed ordering: outputs[0..m-1], inputs[0..n-1]
   - **Rationale**: System/agent variables are primary (what we synthesize)

3. **Backward compatibility IS required**:
   - **Must support formulas without partition files** (for testing use cases)
   - **Fallback behavior**: If no partition provided:
     - Extract all variables from formula automatically
     - Treat all variables as outputs (system/agent variables)
     - No input (environment) variables
   - **API**: Accept inputs/outputs as parameters (not just file paths)

4. **Operator/Variable ID separation**:
   - Operators use enum type (separate from variable IDs)
   - Variables start from ID 0 (clean separation)
   - No more mixed IDs (0-10 operators, 11+ variables)

5. **All priority levels must be implemented**:
   - **High priority**: Move variable management into FormulaPool (per-DFA scoping)
   - **Medium priority**: Pre-allocate variables from partition file
   - **Low priority**: Separate operator enum from variable IDs (can be deferred but must implement)

### Implementation Plan

Based on this decision, the new design will:

```cpp
class FormulaPool {
    // Variable management
    std::vector<std::string> var_names_;  // Only variables (not operators)
    hash_map<std::string, int> var_ids_;  // name -> ID mapping
    int num_outputs_;                      // Count of output (system) variables
    int num_inputs_;                       // Count of input (environment) variables

    // Two-phase initialization (flexible ordering)
    void declare_variables(const std::vector<std::string>& outputs,
                          const std::vector<std::string>& inputs);

    // Or declare separately (order doesn't matter)
    void declare_outputs(const std::vector<std::string>& outputs);
    void declare_inputs(const std::vector<std::string>& inputs);

    // Validation: check if both declared
    bool is_fully_declared() const;

    // Fallback: auto-extract from formula (when no partition provided)
    void extract_variables_from_formula(Formula* root);

    // Formula creation uses pre-declared variables only
    Formula* create_variable(const std::string& name);
};
```

**Variable ID Assignment** (outputs first, then inputs):
```
Partition file:
  .outputs: s1 s2 s3     # System/agent variables (controllable)
  .inputs:  p1 p3 p5     # Environment variables (uncontrollable)

Variable ID assignment:
  outputs[0] (s1) → ID 0
  outputs[1] (s2) → ID 1
  outputs[2] (s3) → ID 2
  inputs[0]  (p1) → ID 3
  inputs[1]  (p3) → ID 4
  inputs[2]  (p5) → ID 5
```

**Fallback Behavior** (no partition file):
```
Formula: (p3) | (X((p6) & ((p8) | (X(!(p8)))))) | ((p9) & (F(!(p8))))

Auto-extraction:
  - Find all variables: {p3, p6, p8, p9}
  - Treat all as outputs (system variables)

Variable ID assignment:
  outputs[0] (p3) → ID 0
  outputs[1] (p6) → ID 1
  outputs[2] (p8) → ID 2
  outputs[3] (p9) → ID 3
  (no inputs)
```

**Operator Representation**:
```cpp
enum class OpType {
    True, False, Not, And, Or, Next, Until, Release, Literal, ...
};

class Formula {
    OpType op_;      // Operator type (separate enum)
    int var_id_;     // Variable ID (only when op_ == OpType::Literal)
};
```

### Benefits Confirmed

1. ✅ **BDD-ready**: Fixed variable ordering matches BDD design
2. ✅ **Per-DFA scoping**: FormulaPool owns variables (aligned with MEMORY_MANAGEMENT_DECISION.md)
3. ✅ **Clear separation**: Operators are enum, variables have dedicated IDs
4. ✅ **No global state**: Each DFA has its own variable space
5. ✅ **System-first ordering**: Outputs (system/agent) variables prioritized
6. ✅ **Testing support**: Fallback auto-extraction enables testing without partition files
7. ✅ **Flexible API**: Accepts both file paths and direct parameter lists

### Synthetic Variables (FOR_UNTIL_*)

**Decision**: Create on-demand (not pre-allocated)

Rationale:
- These are temporary/internal variables, not user-visible
- Created during XNF transformation
- Can be kept in separate namespace or eliminated with alternative XNF implementation
- Does not conflict with pre-allocated user variables

---

## 8. Related Code Locations

### Variable Creation
- [aalta_formula.cpp:706-720](../../lib/deps/formula/aalta_formula.cpp#L706-L720) - Initialization (pre-allocate IDs 0-10)
- [aalta_formula.cpp:1152-1168](../../lib/deps/formula/aalta_formula.cpp#L1152-L1168) - build_atom (on-demand variable creation)
- [aalta_formula.cpp:2235-2237](../../lib/deps/formula/aalta_formula.cpp#L2235-L2237) - Until formula variable creation

### Variable Lookup
- [aalta_formula.cpp:50-53](../../lib/deps/formula/aalta_formula.cpp#L50-L53) - get_name

### Static Data Structures
- [aalta_formula.h:143-144](../../lib/deps/formula/aalta_formula.h#L143-L144) - names and ids
- [aalta_formula.h:225](../../lib/deps/formula/aalta_formula.h#L225) - _max_id

### Input Files
- Example LTLf formula: [tools/benchmarks/sm1000/bench1/f78.ltlf](../../tools/benchmarks/sm1000/bench1/f78.ltlf)
- Example partition: [tools/benchmarks/sm1000/bench1/f447.part](../../tools/benchmarks/sm1000/bench1/f447.part)

---

## 9. Summary

| Aspect | Current Implementation | New Implementation (Option B) |
|--------|----------------------|------------------------------|
| Allocation Strategy | On-demand (lazy) | Pre-allocated (eager) ⭐ |
| Variable Scope | Global (process-wide) | Per-DFA (FormulaPool) ⭐ |
| ID Assignment | Sequential as parsed | Fixed: outputs first, then inputs ⭐ |
| Operator/Variable Mixing | Yes (IDs 0-10 mixed) | No (separate enum) ⭐ |
| Variable Ordering | Parse-order dependent | System-first (outputs before inputs) ⭐ |
| Partition Required | No (auto-detect) | Optional (auto-extract fallback) ⭐ |
| API Design | Single method | Multiple flexible methods ⭐ |
| BDD Compatibility | Low (depends on parse order) | High (fixed ordering) ⭐ |
| Testing Support | N/A | Yes (auto-extraction for tests) ⭐ |

**Status**: ✅ Decision confirmed - Implementing Option B with all priorities

**Key Updates**:
- Outputs (system/agent) variables allocated before inputs (environment)
- Fallback auto-extraction for testing without partition files
- Flexible declaration order (outputs/inputs in any order)
- All priority levels must be implemented (high/medium/low)
