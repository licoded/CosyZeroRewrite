/**
 * @file bdd_manager.cpp
 * @brief Implementation of BDD Manager for Safe System Move optimization
 */

#include "synthesis/bdd_manager.hpp"
#include "formula/formula_pool.hpp"
#include "log/logger.hpp"
#include <algorithm>
#include <stdexcept>
#include <set>

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
 *
 * Note: Full CUDD BDD construction is complex and requires careful handling
 * of macro conflicts (CUDD defines TRUE/FALSE macros).
 * For now, we use a simplified implementation that just tracks availability.
 */
struct BddManager::CuddManager {
    DdManager* mgr = nullptr;

    CuddManager(int num_vars) {
        mgr = Cudd_Init(num_vars, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
        if (!mgr) {
            throw std::runtime_error("Failed to initialize CUDD manager");
        }
        Cudd_AutodynEnable(mgr, CUDD_REORDER_SIFT);
    }

    ~CuddManager() {
        if (mgr) {
            Cudd_Quit(mgr);
        }
    }
};

#endif // FORMULA_USE_CUDD

BddManager::BddManager(int num_variables, int num_outputs)
    : num_variables_(num_variables)
    , num_outputs_(num_outputs)
    , current_formula_(nullptr)
    , stats_{}
{
#ifdef FORMULA_USE_CUDD
    try {
        cudd_ = std::make_unique<CuddManager>(num_variables);
        LOG_INFO("BddManager: CUDD BDD optimization available");
    } catch (const std::exception& e) {
        LOG_WARN("BddManager: CUDD initialization failed: ", e.what());
        cudd_.reset();
    }
#else
    LOG_DEBUG("BddManager: using fallback enumeration (CUDD not available)");
#endif
}

BddManager::~BddManager() {
    clear_cache();
}

bool BddManager::is_available() const {
#ifdef FORMULA_USE_CUDD
    return cudd_ != nullptr;
#else
    return false;
#endif
}

void BddManager::clear_cache() {
    state_formula_cache_.clear();
    current_formula_ = nullptr;
}

bool BddManager::get_or_build_bdd_for_state(automata::TableauState* state,
                                            formula::FormulaPool& pool) {
    // Check cache first
    auto it = state_formula_cache_.find(state);
    if (it != state_formula_cache_.end()) {
        current_formula_ = it->second;
        stats_.num_cache_hits++;
        LOG_DEBUG("BddManager: cache hit for state");
        return true;
    }

    stats_.num_cache_misses++;

    // Build formula from xnf_phi and apply rm_next transformation
    formula::Formula* xnf_phi = state->xnf_phi();
    if (!xnf_phi) {
        // No constraints: all moves are safe
        state_formula_cache_[state] = pool.create_true();
        current_formula_ = pool.create_true();
        LOG_DEBUG("BddManager: no xnf_phi, using True");
        return true;
    }

    // Apply rm_next transformation directly to xnf_phi
    // This correctly handles X(φ) → True substitution
    formula::Formula* rmnext_formula = apply_rm_next(xnf_phi, pool);

    // Cache the result
    state_formula_cache_[state] = rmnext_formula;
    current_formula_ = rmnext_formula;

    LOG_DEBUG("BddManager: built rm_next formula for state");
    return true;
}

bool BddManager::build_from_formula_rmnext(formula::Formula* phi, formula::FormulaPool& pool) {
    if (!phi) {
        current_formula_ = pool.create_false();
        return false;
    }

    // Apply rm_next transformation
    current_formula_ = apply_rm_next(phi, pool);

    // If result is True, all assignments are safe
    // If result is False, no assignment is safe
    return true;
}

formula::Formula* BddManager::get_rmnext_formula(automata::TableauState* state) const {
    auto it = state_formula_cache_.find(state);
    return (it != state_formula_cache_.end()) ? it->second : nullptr;
}

std::vector<Assignment> BddManager::enumerate_safe_sys_moves(
    automata::TableauState* state,
    const std::set<int>& relevant_output_var_ids,
    formula::FormulaPool& pool) {

    // Get or build the rm_next formula for this state
    if (!get_or_build_bdd_for_state(state, pool)) {
        LOG_WARN("BddManager: failed to get BDD for state");
        // Return all moves as safe (conservative)
        std::vector<Assignment> all_moves = {{}};
        return all_moves;
    }

    // Use fallback enumeration (works even with CUDD available for now)
    return enumerate_safe_moves_fallback(current_formula_, relevant_output_var_ids);
}

std::vector<Assignment> BddManager::enumerate_safe_moves_fallback(
    formula::Formula* rmnext_formula,
    const std::set<int>& relevant_output_var_ids) const {

    std::vector<Assignment> safe_moves;

    if (!rmnext_formula) {
        // No constraint, all moves are safe
        safe_moves.push_back({});
        return safe_moves;
    }

    if (rmnext_formula->is_true()) {
        // Enumerate all possible assignments for relevant variables
        std::vector<int> output_vars(relevant_output_var_ids.begin(),
                                     relevant_output_var_ids.end());

        // Generate all 2^k assignments
        int n = output_vars.size();
        if (n == 0) {
            safe_moves.push_back({});
        } else {
            for (uint32_t mask = 0; mask < static_cast<uint32_t>(1 << n); ++mask) {
                Assignment assignment;
                for (int i = 0; i < n; ++i) {
                    if (mask & (1u << i)) {
                        assignment.insert(output_vars[i]);
                    }
                }
                safe_moves.push_back(std::move(assignment));
            }
        }
        return safe_moves;
    }

    if (rmnext_formula->is_false()) {
        // No safe moves
        return {};
    }

    // For other formulas, enumerate and check each
    std::vector<int> output_vars(relevant_output_var_ids.begin(),
                                 relevant_output_var_ids.end());
    int n = output_vars.size();

    // Enumerate all output assignments
    for (uint32_t mask = 0; mask < static_cast<uint32_t>(1 << n); ++mask) {
        Assignment sys_output;
        for (int i = 0; i < n; ++i) {
            if (mask & (1u << i)) {
                sys_output.insert(output_vars[i]);
            }
        }

        // Check if there exists an env move that satisfies
        bool has_safe_env = false;
        int num_inputs = num_variables_ - num_outputs_;

        if (num_inputs == 0) {
            // No input variables, check directly
            if (evaluate_formula(rmnext_formula, sys_output)) {
                has_safe_env = true;
            }
        } else {
            // Enumerate all input assignments
            for (uint32_t env_mask = 0; env_mask < static_cast<uint32_t>(1 << num_inputs); ++env_mask) {
                Assignment combined = sys_output;
                for (int i = 0; i < num_inputs; ++i) {
                    if (env_mask & (1u << i)) {
                        combined.insert(num_outputs_ + i);
                    }
                }

                if (evaluate_formula(rmnext_formula, combined)) {
                    has_safe_env = true;
                    break;
                }
            }
        }

        if (has_safe_env) {
            safe_moves.push_back(std::move(sys_output));
        }
    }

    LOG_DEBUG("BddManager fallback: enumerated ", safe_moves.size(), " safe moves");
    return safe_moves;
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
            // Boolean Until: φ U ψ ≡ ψ ∨ φ
            return evaluate_formula(f->right(), assignment) ||
                   evaluate_formula(f->left(), assignment);
        }

        case OpType::Release: {
            // Boolean Release: φ R ψ ≡ ψ ∧ φ
            return evaluate_formula(f->right(), assignment) &&
                   evaluate_formula(f->left(), assignment);
        }

        case OpType::Next:
            // Should have been removed by rm_next
            return true;

        case OpType::End:
            return false;

        default:
            return false;
    }
}

} // namespace synthesis
