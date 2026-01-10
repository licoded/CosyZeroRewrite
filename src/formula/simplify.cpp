#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include <set>

namespace formula {

namespace {

// ========== Helper Functions for Simplification ==========

/**
 * @brief Collect binary operator terms into a set (flatten chains)
 *
 * Flattens nested AND/OR structures recursively: `(a & (b & c))` → `{a, b, c}`
 * Uses hash consing: Formula* pointer equality = structural equality
 * Collects all terms including True, False (caller checks for them afterward)
 *
 * Note: Uses std::set (not unordered_set) to ensure deterministic iteration order
 * for consistent formula construction and hash consing.
 *
 * @param f Formula to collect from
 * @param terms Output set of terms
 * @param op The operator type to flatten (And or Or)
 */
void collect_binary_terms(Formula* f, std::set<Formula*>& terms,
                          Formula::OpType op) {
    if (!f) return;

    if (f->op() == op) {
        // Flatten chain: collect both sides
        collect_binary_terms(f->left(), terms, op);
        collect_binary_terms(f->right(), terms, op);
    } else {
        // Collect all terms (including True, False)
        terms.insert(f);
    }
}

/**
 * @brief Check if terms contain complementary literals (a and !a)
 *
 * For AND: detects conflicts (a & !a → False)
 * For OR: detects tautologies (a | !a → True)
 *
 * Algorithm: separate positive/negative literals, check for pairs.
 * Due to hash consing, Not(a) has a unique pointer for identical negations.
 *
 * @param terms Set of terms to check (assumed NNF)
 * @param pool FormulaPool for creating Not formulas
 * @return true if complementary pair found
 */
bool has_complementary_literals(const std::set<Formula*>& terms,
                                 FormulaPool& pool) {
    std::set<Formula*> positives;  // v0, v1, ...
    std::set<Formula*> negatives;  // !v0, !v1, ...

    for (Formula* f : terms) {
        if (f->is_not()) {
            // Negative literal: !a
            if (positives.find(f->left()) != positives.end()) {
                return true;  // a and !a both present
            }
            negatives.insert(f);
        } else if (f->is_literal()) {
            // Positive literal: a
            // Check if !a exists by creating Not(a) and looking it up
            Formula* negated = pool.create_not(f);
            if (negatives.find(negated) != negatives.end()) {
                return true;  // !a and a both present
            }
            positives.insert(f);
        }
        // Non-literal terms in NNF are ignored
        // (e.g., X(a), (a & b) - these don't have simple complements)
    }
    return false;
}

/**
 * @brief Rebuild a binary operator chain from a set of terms
 *
 * Creates a right-leaning chain of AND or OR operators from the given terms.
 * - For AND: returns True if empty, single term if only one, otherwise AND chain
 * - For OR: returns False if empty, single term if only one, otherwise OR chain
 *
 * Note: Uses std::set for deterministic iteration order, ensuring consistent
 * formula construction for hash consing.
 *
 * @param pool FormulaPool for creating formulas
 * @param terms Set of terms to chain
 * @param op Operator type (OpType::And or OpType::Or)
 * @return Rebuilt formula
 */
Formula* rebuild_chain(FormulaPool& pool,
                       const std::set<Formula*>& terms,
                       Formula::OpType op) {
    if (terms.empty()) {
        return (op == Formula::OpType::And) ? pool.create_true() : pool.create_false();
    }

    if (terms.size() == 1) {
        return *terms.begin();
    }

    // Build right-leaning chain: a & b & c → &(a, &(b, c))
    auto it = terms.begin();
    Formula* result = *it;
    ++it;

    for (; it != terms.end(); ++it) {
        if (op == Formula::OpType::And) {
            result = pool.create_and(result, *it);
        } else {
            result = pool.create_or(result, *it);
        }
    }

    return result;
}

/**
 * @brief Simplify Until formula using algebraic rules
 *
 * Rules (in order):
 * 1. False U a → a
 * 2. a U False → False
 * 3. a U True → True
 * 4. a U (a | ...) → a | ... (right absorption)
 * 5. a U (a U b) → a U b (left Until absorption)
 * 6. a U (b U a) → b U a (right Until absorption)
 * 7. (X[!] a) U a → a | X[!] a (Next distribution)
 *    - (X[!] a) U a 等价于 a | X[!] a
 *    - 语义: "a 从下一时刻一直为真，直到 a 为真" → "现在 a 为真，或者下一时刻开始 a 一直为真"
 * 8. (X[!] a) U (X[!] b) → X[!](a U b) (Next extraction)
 *    - (X[!] a) U (X[!] b) 等价于 X[!](a U b)
 *    - 语义: 两边都有 strong next，可以提取出来
 *
 * @param pool FormulaPool for creating formulas
 * @param left Left operand
 * @param right Right operand
 * @return Simplified formula
 */
Formula* simplify_until(FormulaPool& pool, Formula* left, Formula* right) {
    // Rule 1: False U a → a
    if (left->is_false()) {
        return right;
    }

    // Rule 2: a U False → False
    if (right->is_false()) {
        return pool.create_false();
    }

    // Rule 3: a U True → True
    if (right->is_true()) {
        return pool.create_true();
    }

    // Rule 4: a U (a | ...) → a | ... (right absorption)
    if (right->is_or()) {
        if (right->left() == left || right->right() == left) {
            return right;  // Simplify the right side
        }
    }

    // Rule 5: a U (a U b) → a U b
    if (right->is_until() && right->left() == left) {
        return pool.create_until(left, right->right());
    }

    // Rule 6: a U (b U a) → b U a
    if (right->is_until() && right->right() == left) {
        return right;  // b U a
    }

    // Rule 7: (X[!] a) U a → a | X[!] a
    if (left->is_next() && left->left() == right) {
        return pool.create_or(left, right);
    }

    // Rule 8: (X[!] a) U (X[!] b) → X[!](a U b)
    if (left->is_next() && right->is_next()) {
        return pool.create_next(
            pool.create_until(left->left(), right->left())
        );
    }

    // Default: no simplification
    return pool.create_until(left, right);
}

/**
 * @brief Simplify Release formula using algebraic rules
 *
 * Rules (in order):
 * 1. True R a → a
 * 2. a R False → False
 * 3. a R True → True
 * 4. a R (a & ...) → a & ... (right absorption)
 * 5. (a | ...) R a → a (left absorption)
 * 6. !a R a → False R a (not absorption)
 *
 * @param pool FormulaPool for creating formulas
 * @param left Left operand
 * @param right Right operand
 * @return Simplified formula
 */
Formula* simplify_release(FormulaPool& pool, Formula* left, Formula* right) {
    // Rule 1: True R a → a
    if (left->is_true()) {
        return right;
    }

    // Rule 2: a R False → False
    if (right->is_false()) {
        return pool.create_false();
    }

    // Rule 3: a R True → True
    if (right->is_true()) {
        return pool.create_true();
    }

    // Rule 4: a R (a & ...) → a & ... (right absorption)
    if (right->is_and()) {
        if (right->left() == left || right->right() == left) {
            return right;
        }
    }

    // Rule 5: (a | ...) R a → a (left absorption)
    if (left->is_or()) {
        if (left->left() == right || left->right() == right) {
            return right;
        }
    }

    // Rule 6: !a R a → False R a
    if (left->is_not() && left->left() == right) {
        return pool.create_release(pool.create_false(), right);
    }

    // Default: no simplification
    return pool.create_release(left, right);
}

/**
 * @brief Simplify Next formula
 *
 * Rules:
 * - X[!] False → False
 * - X[!] True → True (optional, was disabled in original)
 *
 * @param pool FormulaPool for creating formulas
 * @param operand Operand of Next (already simplified)
 * @param original Pointer to original formula for identity check
 * @return Simplified formula
 */
Formula* simplify_next(FormulaPool& pool, Formula* operand, Formula* original) {
    // X[!] False → False
    if (operand->is_false()) {
        return pool.create_false();
    }

    // X[!] True → True (was disabled in original, keeping that behavior)
    // if (operand->is_true()) {
    //     return pool.create_true();
    // }

    // Default: keep X[!](operand)
    if (operand != original->left()) {
        return pool.create_next(operand);
    }
    return original;
}

/**
 * @brief Simplify NOT formula with double negation and constant folding
 *
 * Rules:
 * - !!a → a (double negation)
 * - !True → False
 * - !False → True
 *
 * @param pool FormulaPool for creating formulas
 * @param operand Operand of Not (already simplified)
 * @param original Pointer to original formula for identity check
 * @return Simplified formula
 */
Formula* simplify_not(FormulaPool& pool, Formula* operand, Formula* original) {
    // Double negation: !!a → a
    if (operand->is_not()) {
        return operand->left();
    }

    // Not(True) → False
    if (operand->is_true()) {
        return pool.create_false();
    }

    // Not(False) → True
    if (operand->is_false()) {
        return pool.create_true();
    }

    // Default: keep Not
    if (operand != original->left()) {
        return pool.create_not(operand);
    }
    return original;
}

/**
 * @brief Simplify AND formula with deduplication and conflict detection
 *
 * Process:
 * 1. Collect terms from both sides (flatten nested AND)
 * 2. Simplify each term
 * 3. If simplification produces new AND, expand and re-collect
 * 4. Check for conflicts (a & !a)
 * 5. Rebuild flattened chain
 *
 * Note: Single pass is sufficient because simplified terms are already
 * in normal form (no new AND can be generated from them).
 *
 * @param pool FormulaPool for creating formulas
 * @param left Left operand (not yet simplified)
 * @param right Right operand (not yet simplified)
 * @return Simplified formula
 */
Formula* simplify_and(FormulaPool& pool, Formula* left, Formula* right) {
    std::set<Formula*> terms;

    // Phase 1: Initial collection (flatten nested AND from original tree)
    collect_binary_terms(left, terms, Formula::OpType::And);
    collect_binary_terms(right, terms, Formula::OpType::And);

    // Check for False (dominance: False & anything → False)
    Formula* false_f = pool.create_false();
    if (terms.find(false_f) != terms.end()) {
        return pool.create_false();
    }

    // Phase 2: Simplify each term and expand any new AND formulas
    // Single pass: simplified terms cannot generate new AND
    std::set<Formula*> new_terms;
    for (Formula* f : terms) {
        Formula* simplified = f->simplify(pool);

        // If simplification produced an AND, expand it
        if (simplified->is_and()) {
            collect_binary_terms(simplified->left(), new_terms, Formula::OpType::And);
            collect_binary_terms(simplified->right(), new_terms, Formula::OpType::And);
        } else if (!simplified->is_true()) {
            // Skip True (identity for AND)
            new_terms.insert(simplified);
        }
    }

    // Check for False again after simplification
    if (new_terms.find(false_f) != new_terms.end()) {
        return pool.create_false();
    }

    // Phase 3: Check for conflicts (a & !a)
    if (has_complementary_literals(new_terms, pool)) {
        return pool.create_false();
    }

    // Rebuild chain from deduplicated terms
    return rebuild_chain(pool, new_terms, Formula::OpType::And);
}

/**
 * @brief Simplify OR formula with deduplication and tautology detection
 *
 * Process:
 * 1. Collect terms from both sides (flatten nested OR)
 * 2. Simplify each term
 * 3. If simplification produces new OR, expand and re-collect
 * 4. Check for tautologies (a | !a)
 * 5. Rebuild flattened chain
 *
 * Note: Single pass is sufficient because simplified terms are already
 * in normal form (no new OR can be generated from them).
 *
 * @param pool FormulaPool for creating formulas
 * @param left Left operand (not yet simplified)
 * @param right Right operand (not yet simplified)
 * @return Simplified formula
 */
Formula* simplify_or(FormulaPool& pool, Formula* left, Formula* right) {
    std::set<Formula*> terms;

    // Phase 1: Initial collection (flatten nested OR from original tree)
    collect_binary_terms(left, terms, Formula::OpType::Or);
    collect_binary_terms(right, terms, Formula::OpType::Or);

    // Check for True (dominance: True | anything → True)
    Formula* true_f = pool.create_true();
    if (terms.find(true_f) != terms.end()) {
        return pool.create_true();
    }

    // Phase 2: Simplify each term and expand any new OR formulas
    // Single pass: simplified terms cannot generate new OR
    std::set<Formula*> new_terms;
    for (Formula* f : terms) {
        Formula* simplified = f->simplify(pool);

        // If simplification produced an OR, expand it
        if (simplified->is_or()) {
            collect_binary_terms(simplified->left(), new_terms, Formula::OpType::Or);
            collect_binary_terms(simplified->right(), new_terms, Formula::OpType::Or);
        } else if (!simplified->is_false()) {
            // Skip False (identity for OR)
            new_terms.insert(simplified);
        }
    }

    // Check for True again after simplification
    if (new_terms.find(true_f) != new_terms.end()) {
        return pool.create_true();
    }

    // Phase 3: Check for tautologies (a | !a)
    if (has_complementary_literals(new_terms, pool)) {
        return pool.create_true();
    }

    // Rebuild chain from deduplicated terms
    return rebuild_chain(pool, new_terms, Formula::OpType::Or);
}

} // anonymous namespace

// ========== Main Simplify Function ==========

Formula* Formula::simplify(FormulaPool& pool) const {
    // Check cache first (like aalta's _simp optimization)
    if (simp_ != nullptr) {
        return simp_;
    }

    // Base cases: already simplified
    if (is_true() || is_false() || is_literal() || is_end()) {
        return const_cast<Formula*>(this);
    }

    Formula* result = nullptr;

    switch (op_) {
        case Formula::OpType::Not: {
            Formula* simp_left = left_->simplify(pool);
            result = simplify_not(pool, simp_left, const_cast<Formula*>(this));
            break;
        }

        case Formula::OpType::And: {
            result = simplify_and(pool, left_, right_);
            break;
        }

        case Formula::OpType::Or: {
            result = simplify_or(pool, left_, right_);
            break;
        }

        case Formula::OpType::Next: {
            Formula* simp_left = left_->simplify(pool);
            result = simplify_next(pool, simp_left, const_cast<Formula*>(this));
            break;
        }

        case Formula::OpType::Until: {
            Formula* simp_left = left_->simplify(pool);
            Formula* simp_right = right_->simplify(pool);
            result = simplify_until(pool, simp_left, simp_right);
            break;
        }

        case Formula::OpType::Release: {
            Formula* simp_left = left_->simplify(pool);
            Formula* simp_right = right_->simplify(pool);
            result = simplify_release(pool, simp_left, simp_right);
            break;
        }

        default:
            // Should not reach here
            return const_cast<Formula*>(this);
    }

    // Cache the result
    simp_ = result;
    return result;
}

} // namespace formula
