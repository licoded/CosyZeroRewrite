/**
 * @file bdd_manager.cpp
 * @brief Implementation of BDD Manager for Safe System Move optimization
 */

#include "synthesis/bdd_manager.hpp"
#include "formula/formula_pool.hpp"
#include "log/logger.hpp"
#include <algorithm>
#include <cassert>
#include <functional>
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
#ifdef FORMULA_USE_CUDD
    // Dereference and clear BDD cache
    if (cudd_) {
        for (auto& entry : state_bdd_cache_) {
            if (entry.second) {
                Cudd_RecursiveDeref(cudd_->mgr, entry.second);
            }
        }
    }
    state_bdd_cache_.clear();
#endif
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

#ifdef FORMULA_USE_CUDD
    if (is_available()) {
        // Check BDD cache first
        auto bdd_it = state_bdd_cache_.find(state);
        DdNode* bdd = nullptr;

        if (bdd_it != state_bdd_cache_.end()) {
            bdd = bdd_it->second;
            stats_.num_cache_hits++;
        } else {
            // Build BDD from rm_next formula
            formula::Formula* rmnext_formula = get_rmnext_formula(state);
            // get_or_build_bdd_for_state just returned true, so this must be non-null
            assert(rmnext_formula != nullptr && "rmnext_formula should be in cache after successful build");

            // Build BDD from formula
            bdd = build_bdd_from_formula(rmnext_formula, pool);

            // Cache the BDD
            state_bdd_cache_[state] = bdd;
            stats_.num_cache_misses++;
            stats_.num_bdd_calls++;
        }

        // Apply universal quantification on INPUT variables
        // safe(sys_output) = ∀ env_input. formula(sys_output, env_input)
        //
        // Build cube of input variables for universal abstraction
        // env_cube = e1 ∧ e2 ∧ ... ∧ em (OR of all input variables)
        // Note: CUDD's cube for abstraction is the conjunction of variables
        DdNode* env_cube = Cudd_ReadLogicZero(cudd_->mgr);
        Cudd_Ref(env_cube);

        for (int i = num_outputs_; i < num_variables_; ++i) {
            DdNode* var = Cudd_bddIthVar(cudd_->mgr, i);
            // OR with var to build the cube
            DdNode* new_cube = Cudd_bddOr(cudd_->mgr, env_cube, var);
            Cudd_Ref(new_cube);
            Cudd_RecursiveDeref(cudd_->mgr, env_cube);
            env_cube = new_cube;
        }

        // Apply universal abstraction: ∀ env_vars. phi(sys, env)
        // Result is a BDD over sys variables only
        DdNode* safe_sys = Cudd_bddUnivAbstract(cudd_->mgr, bdd, env_cube);
        Cudd_Ref(safe_sys);
        Cudd_RecursiveDeref(cudd_->mgr, env_cube);

        // Enumerate all satisfying assignments for output variables
        std::vector<Assignment> safe_moves =
            enumerate_bdd_satisfying_assignments(safe_sys, relevant_output_var_ids);

        Cudd_RecursiveDeref(cudd_->mgr, safe_sys);

        stats_.num_safe_moves_generated += safe_moves.size();

        LOG_DEBUG("BddManager: enumerated ", safe_moves.size(), " safe sys moves using BDD");
        return safe_moves;
    }
#endif

    // Fallback: use formula evaluation (when CUDD is not available)
    return enumerate_safe_moves_fallback(current_formula_, relevant_output_var_ids);
}

std::vector<Assignment> BddManager::enumerate_all_output_assignments(
    const std::set<int>& relevant_output_var_ids) const {

    std::vector<Assignment> all_moves;
    std::vector<int> output_vars(relevant_output_var_ids.begin(),
                                 relevant_output_var_ids.end());

    int n = output_vars.size();
    if (n == 0) {
        all_moves.push_back({});
    } else {
        for (uint32_t mask = 0; mask < static_cast<uint32_t>(1 << n); ++mask) {
            Assignment assignment;
            for (int i = 0; i < n; ++i) {
                if (mask & (1u << i)) {
                    assignment.insert(output_vars[i]);
                }
            }
            all_moves.push_back(std::move(assignment));
        }
    }
    return all_moves;
}

std::vector<Assignment> BddManager::enumerate_safe_moves_fallback(
    formula::Formula* rmnext_formula,
    const std::set<int>& relevant_output_var_ids) const {

    // This should never happen if get_or_build_bdd_for_state was called correctly
    // apply_rm_next() always returns a valid Formula*, never nullptr
    assert(rmnext_formula != nullptr && "rmnext_formula should never be nullptr");

    std::vector<Assignment> safe_moves;

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

        // Check if ALL env moves satisfy the formula (universal quantification)
        // safe(sys_output) = ∀ env_input. evaluate(rmnext_formula, sys_output ∪ env_input)
        bool all_env_safe = true;
        int num_inputs = num_variables_ - num_outputs_;

        if (num_inputs == 0) {
            // No input variables, check directly
            if (!evaluate_formula(rmnext_formula, sys_output)) {
                all_env_safe = false;
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

                // If ANY env input makes formula false, sys_output is NOT safe
                if (!evaluate_formula(rmnext_formula, combined)) {
                    all_env_safe = false;
                    break;
                }
            }
        }

        if (all_env_safe) {
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

//==============================================================================
// BDD-based Implementation (CUDD)
//==============================================================================

#ifdef FORMULA_USE_CUDD

DdNode* BddManager::build_bdd_from_formula(formula::Formula* f, formula::FormulaPool& pool) {
    if (!f || !cudd_) {
        return Cudd_ReadLogicZero(cudd_->mgr);  // Return FALSE (referenced)
    }

    using OpType = formula::Formula::OpType;

    switch (f->op()) {
        case OpType::True:
            return Cudd_ReadOne(cudd_->mgr);

        case OpType::False:
            return Cudd_ReadLogicZero(cudd_->mgr);

        case OpType::Literal: {
            // Variable index in BDD
            int var_id = f->var_id();
            DdNode* var = Cudd_bddIthVar(cudd_->mgr, var_id);
            Cudd_Ref(var);  // Reference for caller
            return var;
        }

        case OpType::Not: {
            DdNode* left_bdd = build_bdd_from_formula(f->left(), pool);
            DdNode* result = Cudd_Not(left_bdd);
            Cudd_Ref(result);
            Cudd_RecursiveDeref(cudd_->mgr, left_bdd);
            return result;
        }

        case OpType::And: {
            DdNode* left_bdd = build_bdd_from_formula(f->left(), pool);
            DdNode* right_bdd = build_bdd_from_formula(f->right(), pool);
            DdNode* result = Cudd_bddAnd(cudd_->mgr, left_bdd, right_bdd);
            Cudd_Ref(result);
            Cudd_RecursiveDeref(cudd_->mgr, left_bdd);
            Cudd_RecursiveDeref(cudd_->mgr, right_bdd);
            return result;
        }

        case OpType::Or: {
            DdNode* left_bdd = build_bdd_from_formula(f->left(), pool);
            DdNode* right_bdd = build_bdd_from_formula(f->right(), pool);
            DdNode* result = Cudd_bddOr(cudd_->mgr, left_bdd, right_bdd);
            Cudd_Ref(result);
            Cudd_RecursiveDeref(cudd_->mgr, left_bdd);
            Cudd_RecursiveDeref(cudd_->mgr, right_bdd);
            return result;
        }

        case OpType::Until: {
            // Boolean Until: φ U ψ ≡ ψ ∨ φ
            DdNode* left_bdd = build_bdd_from_formula(f->left(), pool);
            DdNode* right_bdd = build_bdd_from_formula(f->right(), pool);
            DdNode* result = Cudd_bddOr(cudd_->mgr, left_bdd, right_bdd);
            Cudd_Ref(result);
            Cudd_RecursiveDeref(cudd_->mgr, left_bdd);
            Cudd_RecursiveDeref(cudd_->mgr, right_bdd);
            return result;
        }

        case OpType::Release: {
            // Boolean Release: φ R ψ ≡ ψ ∧ φ
            DdNode* left_bdd = build_bdd_from_formula(f->left(), pool);
            DdNode* right_bdd = build_bdd_from_formula(f->right(), pool);
            DdNode* result = Cudd_bddAnd(cudd_->mgr, left_bdd, right_bdd);
            Cudd_Ref(result);
            Cudd_RecursiveDeref(cudd_->mgr, left_bdd);
            Cudd_RecursiveDeref(cudd_->mgr, right_bdd);
            return result;
        }

        case OpType::Next:
        case OpType::End:
            return Cudd_ReadLogicZero(cudd_->mgr);

        default:
            return Cudd_ReadLogicZero(cudd_->mgr);
    }
}

std::vector<Assignment> BddManager::enumerate_bdd_satisfying_assignments(
    DdNode* bdd,
    const std::set<int>& relevant_var_ids) const {

    std::vector<Assignment> assignments;

    if (!cudd_ || !bdd) {
        return assignments;
    }

    // Convert std::set to vector for indexed access
    std::vector<int> var_ids(relevant_var_ids.begin(), relevant_var_ids.end());
    int n = var_ids.size();

    if (n == 0) {
        // No relevant variables: check if BDD is non-empty
        if (bdd != Cudd_ReadLogicZero(cudd_->mgr)) {
            assignments.push_back({});
        }
        return assignments;
    }

    // Create a cube of variables for abstraction (all variables not in relevant_var_ids)
    // We want to enumerate only assignments for relevant variables
    // So we abstract away (existential) the irrelevant variables

    // Build cube of irrelevant variables
    DdNode* cube = Cudd_ReadOne(cudd_->mgr);
    Cudd_Ref(cube);

    for (int i = 0; i < num_variables_; ++i) {
        if (relevant_var_ids.find(i) == relevant_var_ids.end()) {
            DdNode* var = Cudd_bddIthVar(cudd_->mgr, i);
            DdNode* new_cube = Cudd_bddAnd(cudd_->mgr, cube, var);
            Cudd_Ref(new_cube);
            Cudd_RecursiveDeref(cudd_->mgr, cube);
            cube = new_cube;
        }
    }

    // Abstract away irrelevant variables
    DdNode* abstracted = Cudd_bddExistAbstract(cudd_->mgr, bdd, cube);
    Cudd_Ref(abstracted);
    Cudd_RecursiveDeref(cudd_->mgr, cube);

    // Enumerate all satisfying assignments of the abstracted BDD
    // Use recursive enumeration
    std::function<void(DdNode*, int, Assignment&)> enumerate =
        [&](DdNode* node, int var_idx, Assignment& current) {
        if (node == Cudd_ReadLogicZero(cudd_->mgr)) {
            return;  // False branch
        }
        if (node == Cudd_ReadOne(cudd_->mgr) || var_idx >= n) {
            // Reached terminal or processed all variables
            assignments.push_back(current);
            return;
        }

        // Get current BDD variable index
        int bdd_var = Cudd_NodeReadIndex(node);

        // Find next relevant variable to process
        while (var_idx < n && var_ids[var_idx] < bdd_var) {
            // Variable var_ids[var_idx] doesn't appear in BDD
            // Can be either 0 or 1 - enumerate both
            // First: try 0 (don't add to assignment)
            enumerate(node, var_idx + 1, current);
            // Then: try 1 (add to assignment)
            current.insert(var_ids[var_idx]);
            enumerate(node, var_idx + 1, current);
            current.erase(var_ids[var_idx]);
            return;
        }

        if (var_idx >= n) {
            assignments.push_back(current);
            return;
        }

        if (var_ids[var_idx] == bdd_var) {
            // This variable is in the BDD
            DdNode* then_branch = Cudd_T(node);
            DdNode* else_branch = Cudd_E(node);

            // Try FALSE branch (else)
            enumerate(else_branch, var_idx + 1, current);

            // Try TRUE branch (then)
            current.insert(var_ids[var_idx]);
            enumerate(then_branch, var_idx + 1, current);
            current.erase(var_ids[var_idx]);
        } else {
            // var_ids[var_idx] > bdd_var, meaning bdd_var is not in relevant set
            // Skip it and continue with the then branch (since else would be 0)
            enumerate(node, var_idx, current);
        }
    };

    Assignment current;
    enumerate(abstracted, 0, current);

    Cudd_RecursiveDeref(cudd_->mgr, abstracted);

    return assignments;
}

#endif // FORMULA_USE_CUDD

} // namespace synthesis
