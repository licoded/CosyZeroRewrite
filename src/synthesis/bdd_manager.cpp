/**
 * @file bdd_manager.cpp
 * @brief Implementation of BDD Manager for Safe System Move optimization
 */

#include "synthesis/bdd_manager.hpp"
#include "formula/formula_pool.hpp"
#include "log/logger.hpp"
#include <algorithm>
#include <stdexcept>

namespace synthesis {

//==============================================================================
// rm_next Operation
//==============================================================================

namespace {

/**
 * @brief Helper: Recursively apply rm_next transformation
 *
 * Replaces X(φ) with True to extract boolean constraints.
 */
formula::Formula* apply_rm_next_impl(formula::Formula* f, formula::FormulaPool& pool) {
    if (!f) {
        return pool.create_false();
    }

    using OpType = formula::Formula::OpType;

    switch (f->op()) {
        case OpType::True:
        case OpType::False:
        case OpType::Literal:
            // Base cases: keep as is
            return f;

        case OpType::Not: {
            // !(X(φ)) → !True → False
            if (f->left()->is_next()) {
                return pool.create_false();
            }
            // !(φ) → !(rm_next(φ))
            formula::Formula* left_rm = apply_rm_next_impl(f->left(), pool);
            if (left_rm == f->left()) {
                return f;  // No change
            }
            return pool.create_not(left_rm);
        }

        case OpType::And: {
            // φ ∧ X(ψ) → φ ∧ True → φ
            // X(φ) ∧ ψ → True ∧ ψ → ψ
            formula::Formula* left_rm = apply_rm_next_impl(f->left(), pool);
            formula::Formula* right_rm = apply_rm_next_impl(f->right(), pool);

            // Simplify: True ∧ φ → φ, φ ∧ True → φ
            if (left_rm->is_true()) return right_rm;
            if (right_rm->is_true()) return left_rm;
            if (left_rm->is_false() || right_rm->is_false()) {
                return pool.create_false();
            }

            return pool.create_and(left_rm, right_rm);
        }

        case OpType::Or: {
            // φ ∨ X(ψ) → φ ∨ True → True
            // X(φ) ∨ ψ → True ∨ ψ → True
            formula::Formula* left_rm = apply_rm_next_impl(f->left(), pool);
            formula::Formula* right_rm = apply_rm_next_impl(f->right(), pool);

            // Simplify: True ∨ φ → True, φ ∨ True → True
            if (left_rm->is_true() || right_rm->is_true()) {
                return pool.create_true();
            }
            if (left_rm->is_false()) return right_rm;
            if (right_rm->is_false()) return left_rm;

            return pool.create_or(left_rm, right_rm);
        }

        case OpType::Next: {
            // X(φ) → True
            return pool.create_true();
        }

        case OpType::Until: {
            // φ U ψ → rm_next(φ) U rm_next(ψ) if no X in the way
            // But typically U is expanded to XNF before this
            // For safety, apply rm_next to both sides and keep U
            formula::Formula* left_rm = apply_rm_next_impl(f->left(), pool);
            formula::Formula* right_rm = apply_rm_next_impl(f->right(), pool);

            // If either side is True after rm_next, simplify
            // True U ψ ≡ ψ (since True U ψ is satisfied immediately)
            // φ U True ≡ True
            if (left_rm->is_true()) {
                return right_rm;
            }
            if (right_rm->is_true()) {
                return pool.create_true();
            }
            if (left_rm->is_false()) {
                return right_rm;  // False U ψ ≡ ψ
            }
            if (right_rm->is_false()) {
                return pool.create_false();  // φ U False ≡ False
            }

            // Keep Until (it becomes a boolean constraint)
            return pool.create_until(left_rm, right_rm);
        }

        case OpType::Release: {
            // φ R ψ → rm_next(φ) R rm_next(ψ)
            formula::Formula* left_rm = apply_rm_next_impl(f->left(), pool);
            formula::Formula* right_rm = apply_rm_next_impl(f->right(), pool);

            // Simplify:
            // φ R True ≡ True
            // True R ψ ≡ ψ
            // φ R False ≡ False (unless φ is True)
            // False R ψ ≡ ψ
            if (right_rm->is_true()) {
                return pool.create_true();
            }
            if (left_rm->is_true()) {
                return right_rm;
            }
            if (right_rm->is_false()) {
                return left_rm->is_true() ? pool.create_true() : pool.create_false();
            }
            if (left_rm->is_false()) {
                return right_rm;
            }

            return pool.create_release(left_rm, right_rm);
        }

        case OpType::End:
            // End marker - treat as False in boolean context
            return pool.create_false();

        default:
            return f;
    }
}

/**
 * @brief Check if formula contains any temporal operators
 */
bool contains_temporal(formula::Formula* f) {
    if (!f) return false;

    if (f->is_next() || f->is_until() || f->is_release()) {
        return true;
    }

    return contains_temporal(f->left()) || contains_temporal(f->right());
}

} // anonymous namespace

formula::Formula* apply_rm_next(formula::Formula* f, formula::FormulaPool& pool) {
    if (!f) {
        return pool.create_false();
    }

    formula::Formula* result = apply_rm_next_impl(f, pool);

    // Apply simplify to handle cases like s1 | !s1 → True
    result = result->simplify(pool);

    LOG_DEBUG("rm_next: ", f->to_string(), " → ", result->to_string());

    return result;
}

//==============================================================================
// BddManager Implementation
//==============================================================================

#ifdef FORMULA_USE_CUDD

/**
 * @brief Internal wrapper for CUDD manager
 */
struct BddManager::CuddManager {
    DdManager* mgr = nullptr;
    DdNode* bdd = nullptr;  // The current BDD root
    std::unordered_map<int, DdNode*> var_nodes;  // Cache for variable BDD nodes

    CuddManager(int num_vars) {
        mgr = Cudd_Init(num_vars, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
        if (!mgr) {
            throw std::runtime_error("Failed to initialize CUDD manager");
        }
        Cudd_AutodynEnable(mgr, CUDD_REORDER_SIFT);  // Enable dynamic reordering
    }

    ~CuddManager() {
        if (mgr) {
            Cudd_Quit(mgr);
        }
    }

    // Get or create BDD node for a variable
    DdNode* get_var_node(int var_idx) {
        auto it = var_nodes.find(var_idx);
        if (it != var_nodes.end()) {
            return it->second;
        }
        DdNode* node = Cudd_bddIthVar(mgr, var_idx);
        Cudd_Ref(node);
        var_nodes[var_idx] = node;
        return node;
    }
};

#endif // FORMULA_USE_CUDD

BddManager::BddManager(int num_variables, int num_outputs)
    : num_variables_(num_variables)
    , num_outputs_(num_outputs)
    , rmnext_formula_(nullptr)
    , stats_{}
{
#ifdef FORMULA_USE_CUDD
    try {
        cudd_ = std::make_unique<CuddManager>(num_variables);
        LOG_DEBUG("BddManager: initialized CUDD with ", num_variables, " variables");
    } catch (const std::exception& e) {
        LOG_WARN("BddManager: failed to initialize CUDD: ", e.what());
        cudd_.reset();
    }
#else
    LOG_DEBUG("BddManager: CUDD not available, using fallback implementation");
#endif
}

BddManager::~BddManager() {
    clear();
}

bool BddManager::is_available() const {
#ifdef FORMULA_USE_CUDD
    return cudd_ != nullptr;
#else
    return false;
#endif
}

void BddManager::clear() {
    rmnext_formula_ = nullptr;

#ifdef FORMULA_USE_CUDD
    if (cudd_ && cudd_->bdd) {
        Cudd_RecursiveDeref(cudd_->mgr, cudd_->bdd);
        cudd_->bdd = nullptr;
    }
#endif
}

bool BddManager::build_from_formula_rmnext(formula::Formula* phi, formula::FormulaPool& pool) {
    clear();

    // Apply rm_next transformation
    rmnext_formula_ = apply_rm_next(phi, pool);

    // If result is True, all assignments are safe
    // If result is False, no assignment is safe
    if (rmnext_formula_->is_true()) {
        LOG_DEBUG("BddManager: rm_next result is True, all moves are safe");
        return true;
    }
    if (rmnext_formula_->is_false()) {
        LOG_DEBUG("BddManager: rm_next result is False, NO safe moves");
        return true;  // BDD built successfully (but will reject all)
    }

#ifdef FORMULA_USE_CUDD
    if (cudd_) {
        int bdd_node = build_bdd_from_formula(rmnext_formula_, pool);
        if (bdd_node < 0) {
            LOG_WARN("BddManager: failed to build BDD, using fallback");
            // Fall through to formula-based evaluation
        } else {
            LOG_DEBUG("BddManager: BDD built successfully from formula");
            return true;
        }
    }
#endif

    // Fallback: just store the rm_next formula for evaluation
    LOG_DEBUG("BddManager: using fallback evaluation");
    return true;
}

#ifdef FORMULA_USE_CUDD

int BddManager::build_bdd_from_formula(formula::Formula* f, formula::FormulaPool& pool) {
    if (!cudd_ || !f) return -1;

    using OpType = formula::Formula::OpType;

    DdManager* mgr = cudd_->mgr;
    DdNode* result = nullptr;

    switch (f->op()) {
        case OpType::True:
            result = Cudd_ReadOne(mgr);
            Cudd_Ref(result);
            break;

        case OpType::False:
            result = Cudd_ReadLogicZero(mgr);
            Cudd_Ref(result);
            break;

        case OpType::Literal: {
            int var_idx = f->var_id();
            if (var_idx < 0 || var_idx >= num_variables_) {
                LOG_WARN("BddManager: invalid variable index: ", var_idx);
                return -1;
            }
            result = cudd_->get_var_node(var_idx);
            break;
        }

        case OpType::Not: {
            int left_idx = build_bdd_from_formula(f->left(), pool);
            if (left_idx < 0) return -1;
            DdNode* left = cudd_->get_var_node(left_idx);
            // Actually, we need to recursively build BDD for the subformula
            // Let's rewrite this properly
            return -1;  // Placeholder
        }

        case OpType::And: {
            // Build BDD for both sides
            // This is a simplified version - a full implementation would
            // recursively build sub-BDDs and combine them
            // For now, use fallback for complex formulas
            return -1;
        }

        case OpType::Or:
            // Similar to And
            return -1;

        default:
            return -1;
    }

    if (result) {
        // Store the BDD root
        if (cudd_->bdd) {
            Cudd_RecursiveDeref(mgr, cudd_->bdd);
        }
        cudd_->bdd = result;
        return 0;
    }

    return -1;
}

#endif // FORMULA_USE_CUDD

bool BddManager::satisfies(const Assignment& assignment) const {
    stats_.num_bdd_calls++;

    if (!rmnext_formula_) {
        // No constraint built yet, accept all
        return true;
    }

    // If result is True, all assignments satisfy
    if (rmnext_formula_->is_true()) {
        return true;
    }

    // If result is False, no assignments satisfy
    if (rmnext_formula_->is_false()) {
        return false;
    }

#ifdef FORMULA_USE_CUDD
    if (cudd_ && cudd_->bdd) {
        // TODO: Implement CUDD-based evaluation
        // For now, use fallback
        return satisfies_fallback(assignment, rmnext_formula_);
    }
#endif

    return satisfies_fallback(assignment, rmnext_formula_);
}

bool BddManager::satisfies_fallback(const Assignment& assignment, formula::Formula* f) const {
    return evaluate_formula(f, assignment);
}

bool BddManager::evaluate_formula(formula::Formula* f, const Assignment& assignment) const {
    if (!f) return false;

    using OpType = formula::Formula::OpType;

    switch (f->op()) {
        case OpType::True:
            return true;
        case OpType::False:
            return false;

        case OpType::Literal:
            return assignment.count(f->var_id()) > 0;

        case OpType::Not:
            return !evaluate_formula(f->left(), assignment);

        case OpType::And:
            return evaluate_formula(f->left(), assignment) &&
                   evaluate_formula(f->right(), assignment);

        case OpType::Or:
            return evaluate_formula(f->left(), assignment) ||
                   evaluate_formula(f->right(), assignment);

        case OpType::Until: {
            // Boolean Until: φ U ψ ≡ ψ ∨ (φ ∧ X(φ U ψ))
            // In boolean context (no X), this becomes: ψ ∨ φ
            return evaluate_formula(f->right(), assignment) ||
                   evaluate_formula(f->left(), assignment);
        }

        case OpType::Release: {
            // Boolean Release: φ R ψ ≡ ψ ∧ (φ ∨ X(φ R ψ))
            // In boolean context: ψ ∧ φ
            return evaluate_formula(f->right(), assignment) &&
                   evaluate_formula(f->left(), assignment);
        }

        case OpType::Next:
            // Should have been removed by rm_next
            // Treat as True (conservative)
            return true;

        case OpType::End:
            return false;

        default:
            return false;
    }
}

bool BddManager::exists_env_move_for_sys(const Assignment& sys_output) const {
    stats_.num_bdd_calls++;

    if (!rmnext_formula_) {
        // No constraint, all sys moves are safe
        return true;
    }

    // If rm_next result is True, always safe
    if (rmnext_formula_->is_true()) {
        return true;
    }

    // If rm_next result is False, never safe
    if (rmnext_formula_->is_false()) {
        return false;
    }

#ifdef FORMULA_USE_CUDD
    if (cudd_ && cudd_->bdd) {
        // TODO: Implement efficient BDD-based existential quantification
        // For now, use fallback
        return exists_env_move_fallback(sys_output);
    }
#endif

    return exists_env_move_fallback(sys_output);
}

bool BddManager::exists_env_move_fallback(const Assignment& sys_output) const {
    // Enumerate all possible input assignments
    // and check if any satisfy the constraint when combined with sys_output

    // Count input variables (variables from num_outputs_ to num_variables_)
    int num_inputs = num_variables_ - num_outputs_;

    if (num_inputs == 0) {
        // No input variables, just check the sys_output directly
        return satisfies(sys_output);
    }

    // Enumerate all 2^num_inputs input assignments
    // For small numbers of inputs, this is feasible
    // For larger numbers, we should use BDD
    if (num_inputs > 10) {
        LOG_WARN("BddManager: enumerating 2^", num_inputs,
                 " input assignments - consider using CUDD");
    }

    // Generate all input assignments
    std::vector<Assignment> input_assignments;
    input_assignments.reserve(1 << num_inputs);

    for (uint32_t mask = 0; mask < static_cast<uint32_t>(1 << num_inputs); ++mask) {
        Assignment input;
        for (int i = 0; i < num_inputs; ++i) {
            if (mask & (1u << i)) {
                input.insert(num_outputs_ + i);
            }
        }
        input_assignments.push_back(std::move(input));
    }

    // Check if any input assignment satisfies the constraint
    for (const auto& input : input_assignments) {
        // Combine sys_output and input
        Assignment combined = sys_output;
        combined.insert(input.begin(), input.end());

        if (satisfies(combined)) {
            return true;  // Found a satisfying env move
        }
    }

    // Don't increment stats here - filter_safe_moves handles it
    return false;  // No satisfying env move found
}

std::vector<Assignment> BddManager::filter_safe_moves(
    const std::vector<Assignment>& sys_outputs) const {

    std::vector<Assignment> safe_moves;

    for (const auto& sys_out : sys_outputs) {
        if (exists_env_move_for_sys(sys_out)) {
            safe_moves.push_back(sys_out);
            stats_.num_safe_moves++;
        } else {
            stats_.num_unsafe_moves_filtered++;
            LOG_DEBUG("BddManager: filtered unsafe sys move");
        }
    }

    LOG_DEBUG("BddManager: filtered ", sys_outputs.size() - safe_moves.size(),
              " unsafe moves (", safe_moves.size(), " safe remaining)");

    return safe_moves;
}

} // namespace synthesis
