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
 * - Skips True (identity element), stops if False encountered (dominance)
 * - Uses hash consing: Formula* pointer equality = structural equality
 * - Does NOT detect conflicts (a & !a); handled separately by has_complementary_literals()
 *
 * @param f Formula to collect from
 * @param terms Output set of terms
 * @param has_false Output flag set to true if False encountered
 */
void collect_and_terms(Formula* f, std::unordered_set<Formula*>& terms,
                       bool& has_false) {
    if (!f) return;

    switch (f->op()) {
        case Formula::OpType::And:
            // Flatten AND chain: collect both sides
            collect_and_terms(f->left(), terms, has_false);
            collect_and_terms(f->right(), terms, has_false);
            break;
        case Formula::OpType::True:
            // Skip True (identity for AND)
            break;
        case Formula::OpType::False:
            // False in AND → entire AND is False
            has_false = true;
            break;
        default:
            // Collect term: literal or negated literal (NNF)
            terms.insert(f);
            break;
    }
}

/**
 * @brief Collect OR terms into a set (flatten OR chains)
 *
 * Implementation details:
 * - Flattens nested OR structures recursively: `(a | (b | c))` → `{a, b, c}`
 * - Skips False (identity element), stops if True encountered (dominance)
 * - Uses hash consing: Formula* pointer equality = structural equality
 * - Does NOT detect tautologies (a | !a); handled separately by has_complementary_literals()
 *
 * @param f Formula to collect from
 * @param terms Output set of terms
 * @param has_true Output flag set to true if True encountered
 */
void collect_or_terms(Formula* f, std::unordered_set<Formula*>& terms,
                      bool& has_true) {
    if (!f) return;

    switch (f->op()) {
        case Formula::OpType::Or:
            // Flatten OR chain: collect both sides
            collect_or_terms(f->left(), terms, has_true);
            collect_or_terms(f->right(), terms, has_true);
            break;
        case Formula::OpType::False:
            // Skip False (identity for OR)
            break;
        case Formula::OpType::True:
            // True in OR → entire OR is True
            has_true = true;
            break;
        default:
            // Collect term: literal or negated literal (NNF)
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

} // anonymous namespace

// ========== Main Simplify Function ==========

Formula* Formula::simplify(FormulaPool& pool) const {
    // Base cases: already simplified
    if (is_true() || is_false() || is_literal() || is_end()) {
        return const_cast<Formula*>(this);
    }

    switch (op_) {
        case Formula::OpType::Not: {
            // Simplify operand first
            Formula* simp_left = left_->simplify(pool);

            // Double negation: !!a → a
            if (simp_left->is_not()) {
                return simp_left->left();
            }

            // Not(True) → False
            if (simp_left->is_true()) {
                return pool.create_false();
            }

            // Not(False) → True
            if (simp_left->is_false()) {
                return pool.create_true();
            }

            // Default: keep Not
            if (simp_left != left_) {
                return pool.create_not(simp_left);
            }
            return const_cast<Formula*>(this);
        }

        case Formula::OpType::And: {
            // Collect and deduplicate using HashSet (O(n))
            std::unordered_set<Formula*> terms;
            bool has_false = false;

            // Collect terms from left
            Formula* simp_left = left_->simplify(pool);
            collect_and_terms(simp_left, terms, has_false);

            if (has_false) {
                return pool.create_false();
            }

            // Collect terms from right
            Formula* simp_right = right_->simplify(pool);
            collect_and_terms(simp_right, terms, has_false);

            if (has_false) {
                return pool.create_false();
            }

            // Check for conflicts (a & !a)
            if (has_complementary_literals(terms, pool)) {
                return pool.create_false();
            }

            // Rebuild chain from deduplicated terms
            return rebuild_and_chain(pool, terms);
        }

        case Formula::OpType::Or: {
            // Collect and deduplicate using HashSet (O(n))
            std::unordered_set<Formula*> terms;
            bool has_true = false;

            // Collect terms from left
            Formula* simp_left = left_->simplify(pool);
            collect_or_terms(simp_left, terms, has_true);

            if (has_true) {
                return pool.create_true();
            }

            // Collect terms from right
            Formula* simp_right = right_->simplify(pool);
            collect_or_terms(simp_right, terms, has_true);

            if (has_true) {
                return pool.create_true();
            }

            // Check for tautologies (a | !a)
            if (has_complementary_literals(terms, pool)) {
                return pool.create_true();
            }

            // Rebuild chain from deduplicated terms
            return rebuild_or_chain(pool, terms);
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
