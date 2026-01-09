#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include <unordered_set>

namespace formula {

namespace {

// ========== Helper Functions for Simplification ==========

/**
 * @brief Collect AND terms into a set (flatten AND chains)
 *
 * Implementation details:
 * - Flattens nested AND structures recursively: `(a & (b & c))` → `{a, b, c}`
 * - Uses hash consing: Formula* pointer equality = structural equality
 * - Collects all terms including False (caller checks for False afterward)
 *
 * @param f Formula to collect from
 * @param terms Output set of terms
 */
void collect_and_terms(Formula* f, std::unordered_set<Formula*>& terms) {
    if (!f) return;

    switch (f->op()) {
        case Formula::OpType::And:
            // Flatten AND chain: collect both sides
            collect_and_terms(f->left(), terms);
            collect_and_terms(f->right(), terms);
            break;
        default:
            // Collect all terms (including True, False)
            terms.insert(f);
            break;
    }
}

/**
 * @brief Collect OR terms into a set (flatten OR chains)
 *
 * Implementation details:
 * - Flattens nested OR structures recursively: `(a | (b | c))` → `{a, b, c}`
 * - Uses hash consing: Formula* pointer equality = structural equality
 * - Collects all terms including False, True (caller checks for them afterward)
 * - Does NOT detect tautologies (a | !a); handled separately by has_complementary_literals()
 *
 * @param f Formula to collect from
 * @param terms Output set of terms
 */
void collect_or_terms(Formula* f, std::unordered_set<Formula*>& terms) {
    if (!f) return;

    switch (f->op()) {
        case Formula::OpType::Or:
            // Flatten OR chain: collect both sides
            collect_or_terms(f->left(), terms);
            collect_or_terms(f->right(), terms);
            break;
        default:
            // Collect all terms (including True, False)
            terms.insert(f);
            break;
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
bool has_complementary_literals(const std::unordered_set<Formula*>& terms,
                                 FormulaPool& pool) {
    std::unordered_set<Formula*> positives;  // v0, v1, ...
    std::unordered_set<Formula*> negatives;  // !v0, !v1, ...

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
 * @brief Rebuild an AND chain from a set of terms
 *
 * Creates a right-leaning chain of AND operators from the given terms.
 * Returns True if empty, single term if only one, otherwise AND chain.
 *
 * @param pool FormulaPool for creating formulas
 * @param terms Set of terms to chain
 * @return Rebuilt formula
 */
Formula* rebuild_and_chain(FormulaPool& pool,
                           const std::unordered_set<Formula*>& terms) {
    if (terms.empty()) {
        return pool.create_true();
    }

    if (terms.size() == 1) {
        return *terms.begin();
    }

    // Build right-leaning chain: a & b & c → &(a, &(b, c))
    auto it = terms.begin();
    Formula* result = *it;
    ++it;

    for (; it != terms.end(); ++it) {
        result = pool.create_and(result, *it);
    }

    return result;
}

/**
 * @brief Rebuild an OR chain from a set of terms
 *
 * Similar to rebuild_and_chain but for OR operator.
 *
 * @param pool FormulaPool for creating formulas
 * @param terms Set of terms to chain
 * @return Rebuilt formula
 */
Formula* rebuild_or_chain(FormulaPool& pool,
                          const std::unordered_set<Formula*>& terms) {
    if (terms.empty()) {
        return pool.create_false();
    }

    if (terms.size() == 1) {
        return *terms.begin();
    }

    // Build right-leaning chain: a | b | c → |(a, |(b, c))
    auto it = terms.begin();
    Formula* result = *it;
    ++it;

    for (; it != terms.end(); ++it) {
        result = pool.create_or(result, *it);
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
 * 7. X a U a → X a | a (Next distribution)
 * 8. X a U X b → X(a U b) (Next extraction)
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

    // Rule 7: X a U a → X a | a
    if (left->is_next() && left->left() == right) {
        return pool.create_or(left, right);
    }

    // Rule 8: X a U X b → X(a U b)
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
 * - X False → False
 * - X True → True (optional, was disabled in original)
 *
 * @param pool FormulaPool for creating formulas
 * @param operand Operand of Next
 * @return Simplified formula
 */
Formula* simplify_next(FormulaPool& pool, Formula* operand) {
    // X False → False
    if (operand->is_false()) {
        return pool.create_false();
    }

    // X True → True (was disabled in original, keeping that behavior)
    // if (operand->is_true()) {
    //     return pool.create_true();
    // }

    // Default: keep X(operand)
    return pool.create_next(operand);
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
    std::unordered_set<Formula*> terms;

    // Phase 1: Initial collection (flatten nested AND from original tree)
    collect_and_terms(left, terms);
    collect_and_terms(right, terms);

    // Check for False (dominance: False & anything → False)
    for (Formula* f : terms) {
        if (f->is_false()) {
            return pool.create_false();
        }
    }

    // Phase 2: Simplify each term and expand any new AND formulas
    // Single pass: simplified terms cannot generate new AND
    std::unordered_set<Formula*> new_terms;
    for (Formula* f : terms) {
        Formula* simplified = f->simplify(pool);

        // If simplification produced an AND, expand it
        if (simplified->is_and()) {
            collect_and_terms(simplified->left(), new_terms);
            collect_and_terms(simplified->right(), new_terms);
        } else if (!simplified->is_true()) {
            // Skip True (identity for AND)
            new_terms.insert(simplified);
        }
    }

    // Check for False again after simplification
    for (Formula* f : new_terms) {
        if (f->is_false()) {
            return pool.create_false();
        }
    }

    // Phase 3: Check for conflicts (a & !a)
    if (has_complementary_literals(new_terms, pool)) {
        return pool.create_false();
    }

    // Rebuild chain from deduplicated terms
    return rebuild_and_chain(pool, new_terms);
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
    std::unordered_set<Formula*> terms;

    // Phase 1: Initial collection (flatten nested OR from original tree)
    collect_or_terms(left, terms);
    collect_or_terms(right, terms);

    // Check for True (dominance: True | anything → True)
    for (Formula* f : terms) {
        if (f->is_true()) {
            return pool.create_true();
        }
    }

    // Phase 2: Simplify each term and expand any new OR formulas
    // Single pass: simplified terms cannot generate new OR
    std::unordered_set<Formula*> new_terms;
    for (Formula* f : terms) {
        Formula* simplified = f->simplify(pool);

        // If simplification produced an OR, expand it
        if (simplified->is_or()) {
            collect_or_terms(simplified->left(), new_terms);
            collect_or_terms(simplified->right(), new_terms);
        } else if (!simplified->is_false()) {
            // Skip False (identity for OR)
            new_terms.insert(simplified);
        }
    }

    // Check for True again after simplification
    for (Formula* f : new_terms) {
        if (f->is_true()) {
            return pool.create_true();
        }
    }

    // Phase 3: Check for tautologies (a | !a)
    if (has_complementary_literals(new_terms, pool)) {
        return pool.create_true();
    }

    // Rebuild chain from deduplicated terms
    return rebuild_or_chain(pool, new_terms);
}

} // anonymous namespace

// ========== Main Simplify Function ==========

Formula* Formula::simplify(FormulaPool& pool) const {
    // Base cases: already simplified
    if (is_true() || is_false() || is_literal() || is_end()) {
        return const_cast<Formula*>(this);
    }

    switch (op_) {
        case Formula::OpType::Not: {
            Formula* simp_left = left_->simplify(pool);
            return simplify_not(pool, simp_left, const_cast<Formula*>(this));
        }

        case Formula::OpType::And: {
            return simplify_and(pool, left_, right_);
        }

        case Formula::OpType::Or: {
            return simplify_or(pool, left_, right_);
        }

        case Formula::OpType::Next: {
            Formula* simp_left = left_->simplify(pool);
            return simplify_next(pool, simp_left);
        }

        case Formula::OpType::Until: {
            Formula* simp_left = left_->simplify(pool);
            Formula* simp_right = right_->simplify(pool);
            return simplify_until(pool, simp_left, simp_right);
        }

        case Formula::OpType::Release: {
            Formula* simp_left = left_->simplify(pool);
            Formula* simp_right = right_->simplify(pool);
            return simplify_release(pool, simp_left, simp_right);
        }

        default:
            // Should not reach here
            return const_cast<Formula*>(this);
    }
}

} // namespace formula
