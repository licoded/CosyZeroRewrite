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
 * - xnf(φ₁ U φ₂) = xnf(φ₂) ∨ (xnf(φ₁) ∧ X(φ₁ U φ₂))  // Until: CANNOT accept empty string
 * - xnf(φ₁ R φ₂) = xnf(φ₂) ∧ (xnf(φ₁) ∨ X(φ₁ R φ₂))  // Release: CAN accept empty string
 *
 * Where:
 * - X is strong next (implicitly !End) - cannot accept empty string
 * - For Release, empty string acceptance is checked during transition generation
 *   (when φ₂ is satisfied, Release can terminate)
 *
 * CRITICAL: The inner Until/Release inside Next is NOT recursively expanded!
 * This is handled by rmnext progression during DFA construction.
 *
 * Time complexity: O(n) - single pass expansion
 *
 * See docs/ARCHITECTURE/xnf_detailed.md for complete specification.
 */
Formula *Formula::xnf_with_end_marker(FormulaPool &pool) const
{
    // Base cases: already in XNF
    // X(φ) is a base case: Next formulas are already in XNF (◦-formulas)
    // See docs/ARCHITECTURE/xnf_detailed.md for definition
    if (is_true() || is_false() || is_literal() || is_end() || is_not() || is_next())
    {
        return const_cast<Formula *>(this);
    }

    switch (op_)
    {
        case Formula::OpType::Next:
        {
            // Unreachable - handled in base cases above
            // X(φ) stays as X(φ), no recursion needed
            return const_cast<Formula *>(this);
        }

        case Formula::OpType::And:
        {
            // Distribute over And
            return pool.create_and(left_->xnf_with_end_marker(pool), right_->xnf_with_end_marker(pool));
        }

        case Formula::OpType::Or:
        {
            // Distribute over Or
            return pool.create_or(left_->xnf_with_end_marker(pool), right_->xnf_with_end_marker(pool));
        }

        case Formula::OpType::Until:
        {
            // KEY TRANSFORMATION:
            // xnf(φ₁ U φ₂) = xnf(φ₂) ∨ (xnf(φ₁) ∧ X(φ₁ U φ₂))
            //
            // Where X is strong next (implicitly !End)
            // And the inner φ₁ U φ₂ is NOT recursively transformed!

            Formula *left_xnf = left_->xnf_with_end_marker(pool);   // xnf(φ₁)
            Formula *right_xnf = right_->xnf_with_end_marker(pool); // xnf(φ₂)

            // xnf(φ₁) ∧ X(φ₁ U φ₂)
            // NOTE: Keep original Until formula, do NOT recurse!
            Formula *next_until = pool.create_next(const_cast<Formula *>(this) // Original φ₁ U φ₂
            );
            Formula *left_part = pool.create_and(left_xnf, next_until);

            // xnf(φ₂) ∨ (xnf(φ₁) ∧ X(φ₁ U φ₂))
            // No ¬End constraint needed - X implicitly enforces !End
            return pool.create_or(right_xnf, left_part);
        }

        case Formula::OpType::Release:
        {
            // DUAL TRANSFORMATION:
            // xnf(φ₁ R φ₂) = xnf(φ₂) ∧ (xnf(φ₁) ∨ X(φ₁ R φ₂))
            //
            // Release CAN accept empty string when φ₂ is satisfied
            // Empty string check is done during transition generation
            // And the inner φ₁ R φ₂ is NOT recursively transformed!

            Formula *left_xnf = left_->xnf_with_end_marker(pool);   // xnf(φ₁)
            Formula *right_xnf = right_->xnf_with_end_marker(pool); // xnf(φ₂)

            // xnf(φ₁) ∨ X(φ₁ R φ₂)
            // NOTE: Keep original Release formula, do NOT recurse!
            Formula *next_release = pool.create_next(const_cast<Formula *>(this) // Original φ₁ R φ₂
            );
            Formula *left_part = pool.create_or(left_xnf, next_release);

            // xnf(φ₂) ∧ (xnf(φ₁) ∨ X(φ₁ R φ₂))
            // No explicit End marker - empty string acceptance is implicit
            return pool.create_and(right_xnf, left_part);
        }

        default:
            // Should not reach here
            throw std::runtime_error("Invalid operator in XNF transformation");
    }
}

} // namespace formula
