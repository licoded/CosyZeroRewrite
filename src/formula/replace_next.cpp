#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"

#include <stdexcept>

namespace formula {

/**
 * @brief Replace all Strong Next (X[!]) subformulas with True
 *
 * This is a simplification operation for XNF formulas.
 * Input MUST be in XNF format (no Until/Release in primitives).
 *
 * Rules:
 * - True → True
 * - False → False
 * - End → End
 * - Literal p → p
 * - !φ → !replaceNext2True(φ)
 * - φ ∧ ψ → replaceNext2True(φ) ∧ replaceNext2True(ψ)
 * - φ ∨ ψ → replaceNext2True(φ) ∨ replaceNext2True(ψ)
 * - X[!] φ → True
 * - φ U ψ, φ R ψ → ERROR (should not appear in XNF)
 *
 * Time complexity: O(n) where n is formula size.
 */
Formula *Formula::replaceNext2True(FormulaPool &pool) const
{
    switch (op_)
    {
        case OpType::True:
            return pool.create_true();

        case OpType::False:
            return pool.create_false();

        case OpType::End:
            return pool.create_end_marker();

        case OpType::Literal:
            // Literals remain unchanged
            return const_cast<Formula *>(this);

        case OpType::Not:
        {
            Formula *left_next = left_->replaceNext2True(pool);

            // Simplify: !True → False, !False → True
            if (left_next->is_true())
            {
                return pool.create_false();
            }
            if (left_next->is_false())
            {
                return pool.create_true();
            }
            // If unchanged, return this
            if (left_next == left_)
            {
                return const_cast<Formula *>(this);
            }
            return pool.create_not(left_next);
        }

        case OpType::And:
        {
            Formula *left_next = left_->replaceNext2True(pool);
            Formula *right_next = right_->replaceNext2True(pool);

            // Short-circuit: ⊥ ∧ ψ → ⊥
            if (left_next->is_false() || right_next->is_false())
            {
                return pool.create_false();
            }
            // Short-circuit: ⊤ ∧ ψ → ψ
            if (left_next->is_true())
            {
                return right_next;
            }
            if (right_next->is_true())
            {
                return left_next;
            }
            // If unchanged, return this
            if (left_next == left_ && right_next == right_)
            {
                return const_cast<Formula *>(this);
            }
            return pool.create_and(left_next, right_next);
        }

        case OpType::Or:
        {
            Formula *left_next = left_->replaceNext2True(pool);
            Formula *right_next = right_->replaceNext2True(pool);

            // Short-circuit: ⊤ ∨ ψ → ⊤
            if (left_next->is_true() || right_next->is_true())
            {
                return pool.create_true();
            }
            // Short-circuit: ⊥ ∨ ψ → ψ
            if (left_next->is_false())
            {
                return right_next;
            }
            if (right_next->is_false())
            {
                return left_next;
            }
            // If unchanged, return this
            if (left_next == left_ && right_next == right_)
            {
                return const_cast<Formula *>(this);
            }
            return pool.create_or(left_next, right_next);
        }

        case OpType::Next:
            // X[!] φ → True
            return pool.create_true();

        case OpType::Until:
        case OpType::Release:
            throw std::runtime_error("Until/Release should not appear in XNF during replaceNext2True");

        default:
            throw std::runtime_error("Invalid operator in replaceNext2True");
    }
}

} // namespace formula
