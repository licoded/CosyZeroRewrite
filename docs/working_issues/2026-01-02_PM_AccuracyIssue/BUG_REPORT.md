# Bug Report: Low Benchmark Accuracy (56% vs expected ~100%)

**Date**: 2026-01-02 14:05
**Updated**: 2026-01-02 15:00
**Severity**: Critical
**Status**: INVESTIGATING - New fixes added, accuracy decreased

## Problem Description

CosyZeroRewrite LTLf synthesis benchmark shows only **56.25% accuracy** (50 samples) on SMv1000 benchmark, while the reference Cosy implementation achieves **100% accuracy** on the same benchmark.

## Test Results

### Cosy Reference Implementation
```
Total test cases: 1000
Correct results: 1000
Incorrect results: 0
Accuracy: 100.00%
```

### CosyZeroRewrite
```
Total test cases: 458 (partial run)
Passed: 294 (64%)
Failed: 164 (36%)
True Positives: 249
True Negatives: 45
False Positives: 65
False Negatives: 99
Accuracy: 64.2%
```

## Error Pattern

The errors show a systematic pattern of **reversed results** (Realizable ↔ Unrealizable):

| Formula | Expected | Computed | Error Type |
|---------|----------|----------|------------|
| f11 | Realizable | Unrealizable | False Negative |
| f12 | Unrealizable | Realizable | False Positive |
| f15 | Unrealizable | Realizable | False Positive |
| f17 | Unrealizable | Realizable | False Positive |
| f18 | Realizable | Unrealizable | False Negative |
| f102 | Realizable | Unrealizable | False Negative |
| f104 | Unrealizable | Realizable | False Positive |

This pattern suggests a **fundamental algorithmic issue** rather than random errors.

## Potential Root Causes

1. **SCC Classification Logic** (`src/synthesis/on_the_fly_solver.cpp:304`)
   - The `try_classify_scc` function may have incorrect acceptance criteria
   - Swin/Ewin classification may be reversed

2. **I/O Separation Handling** (`src/automata/tableau.cpp:569`)
   - The `is_accepting` function handles non-temporal states with input/output separation
   - May be incorrectly rejecting valid accepting states

3. **Release Formula Semantics**
   - Previous bug fixes for Release formulas may be incomplete
   - LTLf Release semantics may not be fully captured

4. **Initial State Classification**
   - Final realizability check: `return result == StateClass::Swin;`
   - May need to check for different conditions

## Debugging Methodology Lessons Learned

**CRITICAL LESSON (2026-01-02 14:10)**: Do NOT rely solely on final accuracy metrics to judge if a fix is correct.

When investigating systematic bugs:
1. ❌ **Wrong approach**: Make a change → run full benchmark → check accuracy
   - If accuracy drops, conclude the fix was wrong
   - This doesn't tell you WHY the fix is wrong

2. ✅ **Correct approach**:
   - Add debug logging to trace execution
   - Construct minimal test cases
   - Compare intermediate states with reference implementation
   - Verify each component independently

**Example of incorrect reasoning**:
- Modified Environment turn classification logic
- Ran benchmark: 55% accuracy (down from 64%)
- Concluded: "the fix was wrong"
- **Problem**: The original 64% might have been from OTHER bugs. The 55% might be moving in a different direction.

**Correct approach**:
- Verify the game theory semantics for Environment turn
- Compare with reference implementation's logic
- Test on individual formulas with debug output

## Investigation Steps

1. [ ] Compare SCC classification between Cosy and CosyZeroRewrite for specific failing formulas
2. [ ] Add debug output to trace classification through the algorithm
3. [ ] Review the propagation logic in `propagate_classification()`
4. [ ] Check if accepting state computation is correct
5. [ ] **[NEW]** Construct minimal test cases for each component
6. [ ] **[NEW]** Add detailed logging for f11 to trace execution path

## Debug Attempts Log

### Attempt 1: Environment turn classification (14:10)
- **Hypothesis**: Environment turn logic had `has_swin` instead of `all_swin`
- **Change**: Modified `propagate_classification()` lines 389-422
- **Result**: 55% accuracy (down from 64%)
- **Conclusion**: Inconclusive - need more systematic debugging
- **Status**: REVERTED

### Attempt 2: Temporal formula input dependency check (14:20)
- **Hypothesis**: Temporal formulas (Release, Until, Next) don't check input dependencies
- **Changes**:
  - Added check for Release formulas: if ψ requires input true, reject
  - Added check for Until formulas: if ψ requires input true, reject
  - Added check for Next formulas: if φ requires input true, reject
- **Test Results**:
  - G(p5) where p5 is input: PASS ✓ (was failing before)
  - G(p6) where p6 is output: PASS ✓
  - X(p5) where p5 is input: Still FAIL (Realizable, should be Unrealizable)
  - X(p6) where p6 is output: PASS ✓
- **Conclusion**: Partial fix - Release works, but Next still has issues
- **Root Cause Found**: Next literal evaluation path - when p5=false, state becomes {} which is accepting

### Attempt 3: Empty state handling (14:25)
- **Hypothesis**: Empty states should NOT be accepting for synthesis
- **Change**: Added check: if state is empty, return false from is_accepting()
- **Test Results**:
  - X(p5) where p5 is input: PASS ✓ (now correctly Unrealizable)
  - All simple tests: PASS ✓
  - Small benchmark (50 formulas): 55% accuracy (worse than 64%)
- **Conclusion**: Fix is too aggressive - breaks other cases
- **Status**: REVERTED

### Attempt 4: Add false when Next literal fails (14:28)
- **Hypothesis**: When Next literal is not satisfied, add false instead of empty state
- **Change**: Modified Next handling to add `pool.create_false()` when literal is not satisfied
- **Test Results**:
  - Small benchmark (30 formulas): 51.72% accuracy (worse than 64%)
  - Introduced new issues with false propagation
- **Conclusion**: This approach introduces more problems
- **Status**: REVERTED

### Attempt 5: Simplified approach - Release only (14:30)
- **Hypothesis**: Focus on what we know works - Release input dependency check
- **Current State**:
  - Only Release formulas check for input dependency
  - Removed Until/Next checks (they were causing issues)
  - Removed empty state rejection (too aggressive)
  - Removed false addition (caused new issues)
- **Test Results**:
  - Simple tests: ALL PASS ✓
    - X(p6) output: Realizable ✓
    - G(p5) input: Unrealizable ✓
    - G(p6) output: Realizable ✓
  - Small benchmark (50 formulas): 61.22% accuracy (improvement over 55%, but still below 64%)
- **Known Issue**: "free(): invalid pointer" error at end of benchmark run
- **Confusion Matrix (50 formulas)**:
  - True Positives: 21 (correctly Realizable)
  - True Negatives: 9 (correctly Unrealizable)
  - False Positives: 7 (incorrectly Realizable)
  - False Negatives: 12 (incorrectly Unrealizable)

### Key Findings

1. **Release Formula Fix SUCCESSFUL**: G(p5) where p5 is input is now correctly Unrealizable
2. **Empty State Problem**: When p5=false from state {v0}, the successor is {} (empty).
   - Empty state was marked as accepting
   - This allows Environment to "win" by making literals false
   - Fixing this is tricky - rejecting ALL empty states breaks other formulas
   - Need to distinguish "good empty" (formula satisfied) vs "bad empty" (formula failed)

3. **Next Formula Challenge**: X(p5) where p5 is input still returns Realizable (should be Unrealizable)
   - The issue is in the game structure/solving, not the accepting check
   - May require deeper changes to the algorithm

## Next Steps

1. [ ] Distinguish between "good empty" (formula satisfied) and "bad empty" (formula failed)
2. [ ] Track Next obligations through state transitions
3. [ ] Compare with Cosy reference implementation for Until/Next handling
4. [ ] **[URGENT]** Fix "free(): invalid pointer" error appearing at benchmark end

## Files Modified

- `src/automata/tableau.cpp`:
  - Line 590-597: Release formula input dependency check (KEEP - works correctly)
  - Other changes (Until/Next checks, empty state rejection, false addition): REVERTED

## SESSION SUMMARY (For Continuation)

**Current Code State**:
- Only Release formulas check for input dependency in `is_accepting()`
- Simple tests pass: X(p6), G(p5), G(p6)
- Benchmark accuracy: 61.22% on 50 formulas (partial improvement)
- Known issue: "free(): invalid pointer" error

**Key Insight**: The 64% → 61% fluctuation suggests small sample size. Run full benchmark (1000 formulas) for meaningful comparison.

**Recommended Next Approach**:
1. Fix the "free(): invalid pointer" error first (may be affecting results)
2. Run full SMv1000 benchmark to get accurate baseline
3. If still below 90%, consider deeper algorithm changes rather than accepting check tweaks

- `src/automata/tableau.cpp`:
  - Line 579-582: Added empty state rejection
  - Line 584-617: Added temporal formula input dependency checks

---

## Attempt 6: Next Formula Input Dependency Check (16:00) ✅ SUCCESS

### Hypothesis
The `OnTheFlyDFA::is_accepting()` method checked Release formulas for input dependencies but **did NOT check Next formulas**. For X(input), the Next formula requires the input to be true in the next state, which the system can't guarantee.

### Changes Made

#### 1. `src/automata/tableau.cpp` - Line 590-597
Added Next formula input dependency check in `OnTheFlyDFA::is_accepting()`:

```cpp
// Check Next: Xψ requires ψ to be true in the next state
// If ψ requires any input to be true, system can't guarantee it
if (f->op() == formula::Formula::OpType::Next && f->left()) {
    if (requires_input_true(f->left(), num_outputs_)) {
        LOG_DEBUG("OnTheFlyDFA: temporal Next formula requires input true");
        return false;
    }
}
```

#### 2. `src/automata/tableau.cpp` - Line 692-723
Modified `OnTheFlyDFA::successor()` to add false when input literals fail:

```cpp
// For synthesis: check if any input literals will be false
// If an input literal is false, the formula fails → return false state
bool has_failed_input_literal = false;
for (formula::Formula* f : q->formulas()) {
    if (!f) continue;
    if (f->op() == formula::Formula::OpType::Literal) {
        int var_id = f->var_id();
        if (var_id >= num_outputs_) {
            bool literal_true = assignment.count(var_id) > 0;
            if (!literal_true) {
                has_failed_input_literal = true;
                break;
            }
        }
    }
}

// If we had a failed input literal and next state is empty, add false
if (has_failed_input_literal && next_formulas.empty()) {
    next_formulas.insert(pool_.create_false());
}
```

#### 3. `src/synthesis/on_the_fly_solver.cpp` - Line 171-214
Fixed terminal state classification to distinguish System vs Environment turn:

```cpp
// Terminal state semantics:
// - System turn, no successors: System can't move, loses → Ewin
// - Environment turn, no successors: Environment can't move
//   - If accepting: System wins → Swin
//   - If not accepting: formula violated, System loses → Ewin
```

#### 4. `include/automata/tableau.hpp` - Line 146
Added friend declaration for OnTheFlyDFA to access private formulas_:

```cpp
friend class OnTheFlyDFA;  // Allow access to formulas_ for synthesis
```

### Test Results
All simple tests pass:
- X(p6) where p6 is output: Realizable ✓
- X(p5) where p5 is input: **UNREALIZABLE** ✓ (was failing before)
- G(p5) where p5 is input: Unrealizable ✓
- G(p6) where p6 is output: Realizable ✓

### Conclusion
**Next formula input dependency check FIXED the X(p5) issue!**

The root cause was that temporal states with Next formulas containing input dependencies were incorrectly marked as accepting. This allowed the Environment to force a "bad empty" state that was marked as winning for System.

### Status
- ✅ X(p5) bug fixed
- ✅ Terminal state classification fixed
- ✅ Failed input literal handling fixed
- ⏳ Full benchmark pending (interrupted)

---

## SESSION SUMMARY (2026-01-02 Afternoon Continuation)

### Key Fixes Applied

1. **Next Formula Input Dependency Check** (`tableau.cpp:590-597`)
   - `is_accepting()` now checks Next formulas for input dependencies
   - Fixes: X(input) now correctly UNREALIZABLE

2. **Failed Input Literal Handling** (`tableau.cpp:692-723`)
   - `successor()` adds false when input literals fail
   - Prevents "bad empty" states from being marked as accepting

3. **Terminal State Classification** (`on_the_fly_solver.cpp:171-214`)
   - Distinguishes System vs Environment turn for terminal states
   - Correct game semantics for terminal states

4. **Friend Declaration** (`tableau.hpp:146`)
   - OnTheFlyDFA can access TableauState::formulas_

### Files Modified
- `src/automata/tableau.cpp`: Lines 146, 590-597, 692-723
- `src/synthesis/on_the_fly_solver.cpp`: Lines 171-214
- `include/automata/tableau.hpp`: Line 146
- `CLAUDE.md`: Added "运行目录规范" section

### Current Code State
- All simple tests passing
- Next formula input dependency: CHECKED ✓
- Release formula input dependency: CHECKED ✓ (from Attempt 2)
- Terminal state classification: FIXED ✓
- Failed input literal handling: FIXED ✓

### Next Steps (For New Session)
1. Run full SMv1000 benchmark from project root:
   ```bash
   cd /home/lic/files/rewrite_ltlf_codes/CosyZeroRewrite
   ./build/benchmark_runner benchmarks/sm1000 1 1000
   ```

2. Check if accuracy improved from 64%

3. Investigate any remaining failing formulas

### Important Notes
- **Always run from project root** - see CLAUDE.md "运行目录规范"
- Logs go to `logs/benchmark/YYYY-MM-DD/HH-MM/`
- CSV results go to `results/benchmark/YYYY-MM-DD/HH-MM/`

---

## 调试计划 (2026-01-02 15:00)

### 当前状态
- **准确率**: 56.25% (50 samples)
- **False Positives**: 2 (incorrectly Realizable) - 比之前好！
- **False Negatives**: 19 (incorrectly Unrealizable) - 变差了！

### 已修复
- f112 `(p5) & (F(p8))`: U ✓
- f104, f115, f118, f129: 也修复了 ✓

### 新问题
- 输入字面量检查可能**过于保守**
- 状态包含 `{p5, F(p6)}` 时直接拒绝，但这可能不总是正确

### 调试策略（渐进式）

1. **Basic Hardcode Cases** - 手工构造简单测试
   - 构造只含输入/输出的基本公式
   - 测试各种时态算子的组合
   - **全部通过后才进入下一步**

2. **从正确案例变异**
   - 找到 f100, f101, f105, f107 等正确的案例
   - 分析它们的共同特征
   - 逐步引入变化，找出失败边界

3. **小范围抽查** (20-50 个)
   - 正确率 > 70% 才进入全量测试

4. **全量 Benchmark**
   - 只有前面阶段通过才运行

### 待验证的假设
1. 是否所有时态状态中的输入字面量都应该拒绝？
2. Or/And 结构如何影响输入字面量的处理？
3. NNF 转换后的公式结构是否正确？

### 下一步
1. 创建 basic_hardcode_test.cpp
2. 对比正确 vs 错误案例的 NNF 结构
3. 分析是否需要更精细的输入依赖检查
