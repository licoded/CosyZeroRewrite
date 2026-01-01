#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include <stdexcept>

namespace formula {

namespace {

/**
 * @brief Evaluate a literal on an edge
 *
 * @param pool FormulaPool for creating constants
 * @param edge The edge formula (conjunction of literals)
 * @param var_id Variable ID to evaluate
 * @param all_vars Set of all variable IDs
 * @return True if variable satisfied by edge, False otherwise
 *
 * The edge is a formula representing variable assignments.
 * A literal p is satisfied by edge if p ∈ edge.
 * A negated literal !p is satisfied if p ∉ edge.
 */
Formula* evaluate_literal(FormulaPool& pool, Formula* edge, int var_id,
                          const std::unordered_set<int>& all_vars) {
    if (!edge) {
        // No edge means all variables are false
        return pool.create_false();
    }

    // Check if variable is in edge (satisfied)
    bool var_satisfied = all_vars.count(var_id) > 0;

    return var_satisfied ? pool.create_true() : pool.create_false();
}

/**
 * @brief Evaluate a negated literal on an edge
 *
 * @param pool FormulaPool for creating constants
 * @param edge The edge formula
 * @param var_id Variable ID to evaluate
 * @param all_vars Set of all variable IDs
 * @return True if negation satisfied, False otherwise
 */
Formula* evaluate_not_literal(FormulaPool& pool, Formula* edge, int var_id,
                              const std::unordered_set<int>& all_vars) {
    // !p is satisfied if p ∉ edge
    bool var_satisfied = all_vars.count(var_id) > 0;
    return var_satisfied ? pool.create_false() : pool.create_true();
}

/**
 * @brief Check if edge satisfies a formula
 *
 * This is a simplified version that checks if a literal is in the edge.
 * The edge is represented as a set of variable IDs that are true.
 *
 * @param f Formula to evaluate (should be literal or negated literal)
 * @param all_vars Set of true variable IDs in the edge
 * @return true if formula satisfied by edge
 */
bool edge_satisfies(Formula* f, const std::unordered_set<int>& all_vars) {
    if (!f) return false;

    if (f->is_literal()) {
        return all_vars.count(f->var_id()) > 0;
    }

    if (f->is_not() && f->left()->is_literal()) {
        // !p is satisfied if p is NOT in edge
        return all_vars.count(f->left()->var_id()) == 0;
    }

    return false;
}

/**
 * @brief Propagate rmnext over AND with short-circuit
 *
 * @param pool FormulaPool for creating formulas
 * @param left Left operand
 * @param right Right operand
 * @param edge Edge formula
 * @param all_vars Set of all variable IDs
 * @return Progressed formula
 */
Formula* rmnext_and(FormulaPool& pool, Formula* left, Formula* right,
                    Formula* edge, const std::unordered_set<int>& all_vars) {
    Formula* left_next = left->rmnext(pool, edge, all_vars);

    // Short-circuit: ⊥ ∧ ψ → ⊥
    if (left_next->is_false()) {
        return pool.create_false();
    }

    Formula* right_next = right->rmnext(pool, edge, all_vars);

    // Short-circuit: ⊤ ∧ ψ → ψ
    if (left_next->is_true()) {
        return right_next;
    }
    if (right_next->is_true()) {
        return left_next;
    }

    return pool.create_and(left_next, right_next);
}

/**
 * @brief Propagate rmnext over OR with short-circuit
 *
 * @param pool FormulaPool for creating formulas
 * @param left Left operand
 * @param right Right operand
 * @param edge Edge formula
 * @param all_vars Set of all variable IDs
 * @return Progressed formula
 */
Formula* rmnext_or(FormulaPool& pool, Formula* left, Formula* right,
                   Formula* edge, const std::unordered_set<int>& all_vars) {
    Formula* left_next = left->rmnext(pool, edge, all_vars);

    // Short-circuit: ⊤ ∨ ψ → ⊤
    if (left_next->is_true()) {
        return pool.create_true();
    }

    Formula* right_next = right->rmnext(pool, edge, all_vars);

    // Short-circuit: ⊥ ∨ ψ → ψ
    if (left_next->is_false()) {
        return right_next;
    }
    if (right_next->is_false()) {
        return left_next;
    }

    return pool.create_or(left_next, right_next);
}

} // anonymous namespace

// ========== Main rmnext Function ==========

/**
 * @brief Apply formula progression (rmnext)
 *
 * Computes the next state formula after applying a transition.
 * Input MUST be in XNF format (no Until/Release in primitives).
 *
 * Progression rules by operator:
 *
 * | Operator | Rule | Notes |
 * |----------|------|-------|
 * | Literal p | ⊤ if p ∈ edge, else ⊥ | Variable satisfied by assignment |
 * | Not p | ⊤ if p ∉ edge, else ⊥ | Negation satisfied |
 * | φ ∧ ψ | rmnext(φ) ∧ rmnext(ψ) | Distribute over AND |
 * | φ ∨ ψ | rmnext(φ) ∨ rmnext(ψ) | Distribute over OR |
 * | X φ | φ ∧ ¬End | Strong Next: must continue |
 * | End | ⊥ | End marker: no next state |
 * | U, R | ERROR | Should not appear in XNF |
 *
 * @param pool FormulaPool for creating formulas
 * @param edge The transition edge (assignment to variables)
 * @param all_vars Set of variable IDs that are true in the edge
 * @return New formula for the next state
 *
 * Time complexity: O(n) where n is formula size
 *
 * See FORMULA_OPERATIONS.md for complete rmnext specification.
 */
Formula* Formula::rmnext(FormulaPool& pool, Formula* edge,
                         const std::unordered_set<int>& all_vars) const {
    // Base cases
    if (is_true()) {
        return pool.create_true();
    }

    if (is_false()) {
        return pool.create_false();
    }

    if (is_end()) {
        // End marker: no next state
        return pool.create_false();
    }

    if (is_literal()) {
        // Literal p: ⊤ if p ∈ edge, else ⊥
        return all_vars.count(var_id_) > 0
            ? pool.create_true()
            : pool.create_false();
    }

    if (is_not()) {
        // Not(p): ⊤ if p ∉ edge, else ⊥
        if (left_->is_literal()) {
            return all_vars.count(left_->var_id_) == 0
                ? pool.create_true()
                : pool.create_false();
        }
        // For complex negation, recurse
        Formula* left_next = left_->rmnext(pool, edge, all_vars);
        if (left_next->is_true()) {
            return pool.create_false();
        }
        if (left_next->is_false()) {
            return pool.create_true();
        }
        return pool.create_not(left_next);
    }

    if (is_and()) {
        return rmnext_and(pool, left_, right_, edge, all_vars);
    }

    if (is_or()) {
        return rmnext_or(pool, left_, right_, edge, all_vars);
    }

    if (is_next()) {
        // X φ → φ (in standard LTL)
        // But for LTLf with finite traces: X φ rmnext edge → φ ∧ ¬End
        // The ¬End ensures we don't progress past the end
        Formula* child_next = left_->rmnext(pool, edge, all_vars);
        return pool.create_and(child_next, pool.create_not(pool.create_end()));
    }

    if (is_until() || is_release()) {
        // Should not appear in XNF! These should have been expanded
        // If we reach here, it's a programming error
        throw std::runtime_error(
            "Until/Release should not appear in XNF during rmnext");
    }

    // Should not reach here
    throw std::runtime_error("Invalid operator in rmnext");
}

} // namespace formula
