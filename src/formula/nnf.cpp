#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include <stdexcept>

namespace formula {

namespace {

/**
 * @brief Helper function for NNF transformation of negated formulas
 *
 * This function handles the core NNF transformation rules when pushing
 * negations inward through the formula tree.
 *
 * Key rules implemented:
 * - Double negation: !!φ → φ
 * - De Morgan's laws: !(φ ∧ ψ) → !φ ∨ !ψ, !(φ ∨ ψ) → !φ ∧ !ψ
 * - Temporal duality: !(φ U ψ) → (!φ) R (!ψ), !(φ R ψ) → (!φ) U (!ψ)
 * - LTLf Next negation: !X(φ) → X(!φ) ∨ End
 *
 * See NNF_TRANSFORMATION.md for complete specification.
 */
Formula* to_nnf_not(FormulaPool& pool, Formula* f) {
    if (!f) {
        return pool.create_false();
    }

    switch (f->op()) {
        case Formula::OpType::True:
            return pool.create_false();

        case Formula::OpType::False:
            return pool.create_true();

        case Formula::OpType::End:
            // Not(End) is already in NNF
            // End is like a literal for negation purposes
            return pool.create_not(f);

        case Formula::OpType::Literal:
            // Not(a) where a is atomic variable - already in NNF
            return pool.create_not(f);

        case Formula::OpType::Not:
            // Double negation: Not(Not(φ)) → φ
            // Recursively transform the inner formula
            return f->left()->nnf(pool);

        case Formula::OpType::And:
            // De Morgan: Not(φ ∧ ψ) → (!φ) ∨ (!ψ)
            return pool.create_or(
                to_nnf_not(pool, f->left()),
                to_nnf_not(pool, f->right())
            );

        case Formula::OpType::Or:
            // De Morgan: Not(φ ∨ ψ) → (!φ) ∧ (!ψ)
            return pool.create_and(
                to_nnf_not(pool, f->left()),
                to_nnf_not(pool, f->right())
            );

        case Formula::OpType::Next:
            // LTLf finite trace: Not(X(φ)) → X(!φ) ∨ End
            // This is the ONLY rule that differs from standard LTL!
            return pool.create_or(
                pool.create_next(
                    to_nnf_not(pool, f->left())
                ),
                pool.create_end()  // End marker for finite traces
            );

        case Formula::OpType::Until:
            // Duality: Not(φ U ψ) → (!φ) R (!ψ)
            return pool.create_release(
                to_nnf_not(pool, f->left()),
                to_nnf_not(pool, f->right())
            );

        case Formula::OpType::Release:
            // Duality: Not(φ R ψ) → (!φ) U (!ψ)
            return pool.create_until(
                to_nnf_not(pool, f->left()),
                to_nnf_not(pool, f->right())
            );
    }

    // Should never reach here
    throw std::runtime_error("Invalid operator in NNF transformation");
}

} // anonymous namespace

// ========== Main NNF Transformation ==========

Formula* Formula::nnf(FormulaPool& pool) const {
    // Base cases: already in NNF
    if (is_true() || is_false() || is_literal() || is_end()) {
        return const_cast<Formula*>(this);
    }

    switch (op_) {
        case Formula::OpType::Not: {
            // Handle negation using helper function
            return to_nnf_not(pool, left_);
        }

        case Formula::OpType::And:
        case Formula::OpType::Or:
        case Formula::OpType::Until:
        case Formula::OpType::Release:
            // Structure-preserving: recurse on children
            return pool.create(
                op_,
                left_->nnf(pool),
                right_->nnf(pool)
            );

        case Formula::OpType::Next:
            // Structure-preserving: recurse on child
            return pool.create_next(left_->nnf(pool));

        default:
            // Should not reach here for constants/literals
            return const_cast<Formula*>(this);
    }
}

} // namespace formula
