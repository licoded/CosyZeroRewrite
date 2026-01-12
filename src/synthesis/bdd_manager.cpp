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
#include <set>
#include <stdexcept>

namespace synthesis {

//==============================================================================
// BddManager Implementation
//==============================================================================

/**
 * @brief Internal wrapper for CUDD manager
 *
 * Note: Full CUDD BDD construction is complex and requires careful handling
 * of macro conflicts (CUDD defines TRUE/FALSE macros).
 * For now, we use a simplified implementation that just tracks availability.
 */
struct BddManager::CuddManager {
    DdManager *mgr = nullptr;

    CuddManager(int num_vars)
    {
        mgr = Cudd_Init(num_vars, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
        if (!mgr)
        {
            throw std::runtime_error("Failed to initialize CUDD manager");
        }
        Cudd_AutodynEnable(mgr, CUDD_REORDER_SIFT);
    }

    ~CuddManager()
    {
        if (mgr)
        {
            Cudd_Quit(mgr);
        }
    }
};

namespace {

/**
 * @brief Build a cube by OR-ing multiple variables together
 *
 * Creates a BDD representing: var0 | var1 | var2 | ...
 *
 * @param mgr CUDD manager
 * @param var_ids List of variable indices to OR together
 * @return Referenced BDD node (caller must deref)
 */
DdNode *build_or_cube(DdManager *mgr, const std::vector<int> &var_ids)
{
    // Start with LogicZero (identity for OR)
    DdNode *cube = Cudd_ReadLogicZero(mgr);
    Cudd_Ref(cube);

    for (int var_id : var_ids)
    {
        DdNode *var = Cudd_bddIthVar(mgr, var_id);
        DdNode *new_cube = Cudd_bddOr(mgr, cube, var);
        Cudd_Ref(new_cube);
        Cudd_RecursiveDeref(mgr, cube);
        cube = new_cube;
    }

    return cube;
}

/**
 * @brief Build a cube by AND-ing multiple variables together
 *
 * Creates a BDD representing: var0 & var1 & var2 & ...
 *
 * @param mgr CUDD manager
 * @param var_ids List of variable indices to AND together
 * @return Referenced BDD node (caller must deref)
 */
DdNode *build_and_cube(DdManager *mgr, const std::vector<int> &var_ids)
{
    // Start with One (identity for AND)
    DdNode *cube = Cudd_ReadOne(mgr);
    Cudd_Ref(cube);

    for (int var_id : var_ids)
    {
        DdNode *var = Cudd_bddIthVar(mgr, var_id);
        DdNode *new_cube = Cudd_bddAnd(mgr, cube, var);
        Cudd_Ref(new_cube);
        Cudd_RecursiveDeref(mgr, cube);
        cube = new_cube;
    }

    return cube;
}

/**
 * @brief Generic helper for binary BDD operations with automatic ref/deref
 *
 * @tparam BinOp Binary operation type (e.g., Cudd_bddAnd, Cudd_bddOr)
 * @param mgr CUDD manager
 * @param left Left operand BDD
 * @param right Right operand BDD
 * @param op The binary operation to apply
 * @return Referenced result BDD node (caller must deref)
 */
template <typename BinOp> DdNode *apply_binary_bdd_op(DdManager *mgr, DdNode *left, DdNode *right, BinOp &&op)
{
    DdNode *result = op(mgr, left, right);
    Cudd_Ref(result);
    Cudd_RecursiveDeref(mgr, left);
    Cudd_RecursiveDeref(mgr, right);
    return result;
}

} // anonymous namespace

BddManager::BddManager(int num_variables, int num_outputs)
    : num_variables_(num_variables), num_outputs_(num_outputs), current_formula_(nullptr), stats_ {}
{
    try
    {
        cudd_ = std::make_unique<CuddManager>(num_variables);
        LOG_INFO("BddManager: CUDD BDD optimization available");
    }
    catch (const std::exception &e)
    {
        LOG_WARN("BddManager: CUDD initialization failed: ", e.what());
        cudd_.reset();
    }
}

BddManager::~BddManager()
{
    clear_cache();
}

void BddManager::clear_cache()
{
    state_formula_cache_.clear();
    // Dereference and clear BDD cache
    if (cudd_)
    {
        for (auto &entry : state_bdd_cache_)
        {
            if (entry.second)
            {
                Cudd_RecursiveDeref(cudd_->mgr, entry.second);
            }
        }
    }
    state_bdd_cache_.clear();
    current_formula_ = nullptr;
}

bool BddManager::get_or_build_bdd_for_state(automata::TableauState *state, formula::FormulaPool &pool)
{
    // Check cache first
    auto it = state_formula_cache_.find(state);
    if (it != state_formula_cache_.end())
    {
        current_formula_ = it->second;
        stats_.num_cache_hits++;
        LOG_DEBUG("BddManager: cache hit for state");
        return true;
    }

    stats_.num_cache_misses++;

    formula::Formula *xnf_phi = state->xnf_phi();
    current_formula_ = xnf_phi->replaceNext2True(pool);
    current_formula_ = current_formula_->simplify(pool);
    state_formula_cache_[state] = current_formula_;

    LOG_DEBUG("BddManager: built rm_next formula for state");
    return true;
}

formula::Formula *BddManager::get_rmnext_formula(automata::TableauState *state) const
{
    auto it = state_formula_cache_.find(state);
    return (it != state_formula_cache_.end()) ? it->second : nullptr;
}

std::vector<Assignment> BddManager::enumerate_safe_sys_moves(automata::TableauState *state,
                                                             const std::set<int> &relevant_output_var_ids,
                                                             formula::FormulaPool &pool)
{
    // Get or build the rm_next formula for this state
    if (!get_or_build_bdd_for_state(state, pool))
    {
        LOG_WARN("BddManager: failed to get BDD for state");
        // Return all moves as safe (conservative)
        std::vector<Assignment> all_moves = {{}};
        return all_moves;
    }

    // Check BDD cache first
    auto bdd_it = state_bdd_cache_.find(state);
    DdNode *bdd = nullptr;

    if (bdd_it != state_bdd_cache_.end())
    {
        bdd = bdd_it->second;
        stats_.num_cache_hits++;
    }
    else
    {
        // Build BDD from rm_next formula
        formula::Formula *rmnext_formula = get_rmnext_formula(state);
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
    // env_cube = e1 ∨ e2 ∨ ... ∨ em (OR of all input variables)
    // Note: CUDD's cube for abstraction uses OR
    std::vector<int> input_var_ids;
    for (int i = num_outputs_; i < num_variables_; ++i)
    {
        input_var_ids.push_back(i);
    }
    DdNode *env_cube = build_or_cube(cudd_->mgr, input_var_ids);

    // Apply universal abstraction: ∀ env_vars. phi(sys, env)
    // Result is a BDD over sys variables only
    DdNode *safe_sys = Cudd_bddUnivAbstract(cudd_->mgr, bdd, env_cube);
    Cudd_Ref(safe_sys);
    Cudd_RecursiveDeref(cudd_->mgr, env_cube);

    // Enumerate all satisfying assignments for output variables
    std::vector<Assignment> safe_moves = enumerate_bdd_satisfying_assignments(safe_sys, relevant_output_var_ids);

    Cudd_RecursiveDeref(cudd_->mgr, safe_sys);

    stats_.num_safe_moves_generated += safe_moves.size();

    LOG_DEBUG("BddManager: enumerated ", safe_moves.size(), " safe sys moves using BDD");
    return safe_moves;
}

//==============================================================================
// BDD-based Implementation (CUDD)
//==============================================================================

DdNode *BddManager::build_bdd_from_formula(formula::Formula *f, formula::FormulaPool &pool)
{
    if (!f || !cudd_)
    {
        return Cudd_ReadLogicZero(cudd_->mgr); // Return FALSE (referenced)
    }

    using OpType = formula::Formula::OpType;

    switch (f->op())
    {
        case OpType::True:
            return Cudd_ReadOne(cudd_->mgr);

        case OpType::False:
            return Cudd_ReadLogicZero(cudd_->mgr);

        case OpType::Literal:
        {
            // Variable index in BDD
            int var_id = f->var_id();
            DdNode *var = Cudd_bddIthVar(cudd_->mgr, var_id);
            Cudd_Ref(var); // Reference for caller
            return var;
        }

        case OpType::Not:
        {
            DdNode *left_bdd = build_bdd_from_formula(f->left(), pool);
            DdNode *result = Cudd_Not(left_bdd);
            Cudd_Ref(result);
            Cudd_RecursiveDeref(cudd_->mgr, left_bdd);
            return result;
        }

        case OpType::And:
        {
            DdNode *left_bdd = build_bdd_from_formula(f->left(), pool);
            DdNode *right_bdd = build_bdd_from_formula(f->right(), pool);
            return apply_binary_bdd_op(cudd_->mgr, left_bdd, right_bdd, Cudd_bddAnd);
        }

        case OpType::Or:
        {
            DdNode *left_bdd = build_bdd_from_formula(f->left(), pool);
            DdNode *right_bdd = build_bdd_from_formula(f->right(), pool);
            return apply_binary_bdd_op(cudd_->mgr, left_bdd, right_bdd, Cudd_bddOr);
        }

        case OpType::Until:
        {
            // Boolean Until: φ U ψ ≡ ψ ∨ φ
            DdNode *left_bdd = build_bdd_from_formula(f->left(), pool);
            DdNode *right_bdd = build_bdd_from_formula(f->right(), pool);
            return apply_binary_bdd_op(cudd_->mgr, left_bdd, right_bdd, Cudd_bddOr);
        }

        case OpType::Release:
        {
            // Boolean Release: φ R ψ ≡ ψ ∧ φ
            DdNode *left_bdd = build_bdd_from_formula(f->left(), pool);
            DdNode *right_bdd = build_bdd_from_formula(f->right(), pool);
            return apply_binary_bdd_op(cudd_->mgr, left_bdd, right_bdd, Cudd_bddAnd);
        }

        case OpType::Next:
        case OpType::End:
            return Cudd_ReadLogicZero(cudd_->mgr);

        default:
            return Cudd_ReadLogicZero(cudd_->mgr);
    }
}

std::vector<Assignment> BddManager::enumerate_bdd_satisfying_assignments(DdNode *bdd,
                                                                         const std::set<int> &relevant_var_ids) const
{
    std::vector<Assignment> assignments;

    if (!cudd_ || !bdd)
    {
        return assignments;
    }

    // Convert std::set to vector for indexed access
    std::vector<int> var_ids(relevant_var_ids.begin(), relevant_var_ids.end());
    int n = var_ids.size();

    // Create a cube of variables for abstraction (all variables not in relevant_var_ids)
    // We want to enumerate only assignments for relevant variables
    // So we abstract away (existential) the irrelevant variables

    // Build cube of irrelevant variables (using AND)
    std::vector<int> irrelevant_var_ids;
    for (int i = 0; i < num_variables_; ++i)
    {
        if (relevant_var_ids.find(i) == relevant_var_ids.end())
        {
            irrelevant_var_ids.push_back(i);
        }
    }
    DdNode *cube = build_and_cube(cudd_->mgr, irrelevant_var_ids);

    // Abstract away irrelevant variables
    DdNode *abstracted = Cudd_bddExistAbstract(cudd_->mgr, bdd, cube);
    Cudd_Ref(abstracted);
    Cudd_RecursiveDeref(cudd_->mgr, cube);

    // After abstraction, check result
    if (n == 0)
    {
        // No relevant variables: abstracted must be TRUE or FALSE
        // (since we abstracted all variables)
        if (abstracted == Cudd_ReadOne(cudd_->mgr))
        {
            assignments.push_back({});
        }
        else if (abstracted != Cudd_ReadLogicZero(cudd_->mgr))
        {
            // Should never happen: after abstracting all variables,
            // result must be a constant (TRUE or FALSE)
            assert(false && "BDD after full abstraction must be TRUE or FALSE");
        }
        Cudd_RecursiveDeref(cudd_->mgr, abstracted);
        return assignments;
    }

    // Enumerate all satisfying assignments of the abstracted BDD
    // Use recursive enumeration
    //
    // Key insight: var_idx >= n is a NORMAL termination condition, not an error.
    // It means we've processed all relevant variables (var_ids[0..n-1]), so the
    // current assignment is complete and should be saved.
    std::function<void(DdNode *, int, Assignment &)> enumerate = [&](DdNode *node, int var_idx, Assignment &current) {
        if (node == Cudd_ReadLogicZero(cudd_->mgr))
        {
            return; // False branch - unsatisfiable
        }
        if (node == Cudd_ReadOne(cudd_->mgr) || var_idx >= n)
        {
            // Terminated successfully: reached TRUE terminal OR processed all relevant vars
            assignments.push_back(current);
            return;
        }

        // Get current BDD variable index
        int bdd_var = Cudd_NodeReadIndex(node);

        // Find next relevant variable to process
        while (var_idx < n && var_ids[var_idx] < bdd_var)
        {
            // Variable var_ids[var_idx] doesn't appear in BDD (free variable)
            // It can be either 0 or 1 - enumerate both possibilities
            // First: try 0 (don't add to assignment)
            enumerate(node, var_idx + 1, current);
            // Then: try 1 (add to assignment)
            current.insert(var_ids[var_idx]);
            enumerate(node, var_idx + 1, current);
            current.erase(var_ids[var_idx]);
            return;
        }

        // After while loop, either var_idx >= n (all vars processed, NORMAL)
        // or var_ids[var_idx] >= bdd_var (need to match BDD structure)
        if (var_idx >= n)
        {
            // All relevant variables processed - current assignment is complete
            assignments.push_back(current);
            return;
        }

        if (var_ids[var_idx] == bdd_var)
        {
            // This variable is in the BDD
            DdNode *then_branch = Cudd_T(node);
            DdNode *else_branch = Cudd_E(node);

            // Try FALSE branch (else)
            enumerate(else_branch, var_idx + 1, current);

            // Try TRUE branch (then)
            current.insert(var_ids[var_idx]);
            enumerate(then_branch, var_idx + 1, current);
            current.erase(var_ids[var_idx]);
        }
        else
        {
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

} // namespace synthesis
