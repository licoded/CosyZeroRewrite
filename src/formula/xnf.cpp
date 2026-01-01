#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include <stdexcept>

namespace formula {

// ========== Main XNF Transformation ==========

/**
 * @brief Convert formula to neXt Normal Form (XNF)
 *
 * XNF: A formula is in XNF if its primitive subformulas pa(φ) only include
 * literals and Next operators (no Until/Release inside primitives).
 *
 * Key transformation rules:
 * - xnf(φ₁ U φ₂) = (xnf(φ₂) ∧ ♢true) ∨ (xnf(φ₁) ∧ X(φ₁ U φ₂))
 * - xnf(φ₁ R φ₂) = (xnf(φ₂) ∨ □false) ∧ (xnf(φ₁) ∨ X(φ₁ R φ₂))
 *
 * Where:
 * - ♢true ≡ ¬End (eventually true)
 * - □false ≡ End (always false)
 *
 * CRITICAL: The inner Until/Release inside Next is NOT recursively expanded!
 * This is handled by rmnext progression during DFA construction.
 *
 * Time complexity: O(n) - single pass expansion
 *
 * See XNF_TRANSFORMATION.md for complete specification.
 */
Formula* Formula::xnf_with_tail(FormulaPool& pool) const {
    // Base cases: already in XNF
    if (is_true() || is_false() || is_literal() || is_end() || is_not()) {
        return const_cast<Formula*>(this);
    }

    switch (op_) {
        case Formula::OpType::Next: {
            // X(φ): recurse on child
            return pool.create_next(left_->xnf_with_tail(pool));
        }

        case Formula::OpType::And: {
            // Distribute over And
            return pool.create_and(
                left_->xnf_with_tail(pool),
                right_->xnf_with_tail(pool)
            );
        }

        case Formula::OpType::Or: {
            // Distribute over Or
            return pool.create_or(
                left_->xnf_with_tail(pool),
                right_->xnf_with_tail(pool)
            );
        }

        case Formula::OpType::Until: {
            // KEY TRANSFORMATION:
            // xnf(φ₁ U φ₂) = (xnf(φ₂) ∧ ♢true) ∨ (xnf(φ₁) ∧ X(φ₁ U φ₂))
            //
            // Where ♢true = ¬End (eventually true)
            // And the inner φ₁ U φ₂ is NOT recursively transformed!

            Formula* left_xnf = left_->xnf_with_tail(pool);   // xnf(φ₁)
            Formula* right_xnf = right_->xnf_with_tail(pool);  // xnf(φ₂)

            // ♢true = ¬End
            Formula* eventually_true = pool.create_not(pool.create_end());

            // xnf(φ₂) ∧ ♢true
            Formula* right_part = pool.create_and(right_xnf, eventually_true);

            // xnf(φ₁) ∧ X(φ₁ U φ₂)
            // NOTE: Keep original Until formula, do NOT recurse!
            Formula* next_until = pool.create_next(
                const_cast<Formula*>(this)  // Original φ₁ U φ₂
            );
            Formula* left_part = pool.create_and(left_xnf, next_until);

            // (xnf(φ₂) ∧ ♢true) ∨ (xnf(φ₁) ∧ X(φ₁ U φ₂))
            return pool.create_or(right_part, left_part);
        }

        case Formula::OpType::Release: {
            // DUAL TRANSFORMATION:
            // xnf(φ₁ R φ₂) = (xnf(φ₂) ∨ □false) ∧ (xnf(φ₁) ∨ X(φ₁ R φ₂))
            //
            // Where □false = End
            // And the inner φ₁ R φ₂ is NOT recursively transformed!

            Formula* left_xnf = left_->xnf_with_tail(pool);   // xnf(φ₁)
            Formula* right_xnf = right_->xnf_with_tail(pool);  // xnf(φ₂)

            // □false = End
            Formula* always_false = pool.create_end();

            // xnf(φ₂) ∨ □false
            Formula* right_part = pool.create_or(right_xnf, always_false);

            // xnf(φ₁) ∨ X(φ₁ R φ₂)
            // NOTE: Keep original Release formula, do NOT recurse!
            // Note: We use X here (Weak Next was converted to X during parsing)
            Formula* next_release = pool.create_next(
                const_cast<Formula*>(this)  // Original φ₁ R φ₂
            );
            Formula* left_part = pool.create_or(left_xnf, next_release);

            // (xnf(φ₂) ∨ □false) ∧ (xnf(φ₁) ∨ X(φ₁ R φ₂))
            return pool.create_and(right_part, left_part);
        }

        default:
            // Should not reach here
            throw std::runtime_error("Invalid operator in XNF transformation");
    }
}

} // namespace formula
