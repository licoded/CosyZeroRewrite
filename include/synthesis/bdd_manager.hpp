/**
 * @file bdd_manager.hpp
 * @brief BDD Manager for Safe System Move optimization (Rule B)
 *
 * Based on: arXiv:2408.07324 - "On-the-fly Synthesis for LTL over Finite Traces"
 *
 * Rule B (Safe System Move):
 * For a TableauState φ, extract boolean constraints by removing all X() operators
 * (replacing them with True). A system move is safe if there exists at least one
 * environment move that satisfies these boolean constraints.
 *
 * The BDD is used to efficiently enumerate all safe system moves directly,
 * avoiding the need to enumerate all possible moves and then filter.
 */

#ifndef SYNTHESIS_BDD_MANAGER_HPP
#define SYNTHESIS_BDD_MANAGER_HPP

#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "automata/tableau.hpp"
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef FORMULA_USE_CUDD
#include <cudd.h>
#endif

namespace synthesis {

/**
 * @brief Assignment represented as variable indices set to TRUE
 *
 * This matches the automata::Assignment type from tableau.hpp.
 * Variables not in the set are assumed to be FALSE.
 */
using Assignment = std::unordered_set<int>;

/**
 * @brief BDD Manager for Safe System Move optimization
 *
 * This class provides:
 * 1. rm_next operation: Extract boolean constraints by replacing X() with True
 * 2. BDD construction: Build a BDD from the boolean formula
 * 3. Direct enumeration: Enumerate all safe system moves from BDD
 * 4. State caching: Cache BDDs per TableauState to avoid rebuilding
 *
 * Key change: Instead of enumerating all moves and filtering,
 * we directly enumerate only the safe moves from the BDD.
 */
class BddManager {
public:
    /**
     * @brief Construct BDD manager
     * @param num_variables Total number of variables (inputs + outputs)
     * @param num_outputs Number of output (system) variables
     */
    explicit BddManager(int num_variables, int num_outputs);

    /**
     * @brief Destructor - cleanup CUDD manager
     */
    ~BddManager();

    /**
     * @brief Check if BDD is available (CUDD found)
     */
    bool is_available() const;

    /**
     * @brief Get or build BDD for a TableauState
     *
     * Caches the BDD per state to avoid rebuilding.
     * Returns true if the BDD was built (or retrieved from cache).
     *
     * @param state The TableauState
     * @param pool Formula pool for creating intermediate formulas
     * @return true if BDD is ready for use
     */
    bool get_or_build_bdd_for_state(automata::TableauState* state,
                                    formula::FormulaPool& pool);

    /**
     * @brief Get or build BDD for a formula
     *
     * @param phi The formula
     * @param pool Formula pool for creating intermediate formulas
     * @return true if BDD is ready for use
     */
    bool build_from_formula_rmnext(formula::Formula* phi, formula::FormulaPool& pool);

    /**
     * @brief Directly enumerate all safe system moves for a state
     *
     * This is the PRIMARY method to use. It directly enumerates only
     * the system moves that are safe (i.e., for which there exists
     * at least one env move satisfying the constraint).
     *
     * This replaces the old "enumerate all, then filter" approach.
     *
     * @param state The TableauState
     * @param relevant_output_var_ids Only consider these output variables
     * @param pool Formula pool
     * @return List of safe system output assignments
     */
    std::vector<Assignment> enumerate_safe_sys_moves(
        automata::TableauState* state,
        const std::set<int>& relevant_output_var_ids,
        formula::FormulaPool& pool);

    /**
     * @brief Get the rm_next formula for a state (for debugging)
     *
     * Returns the formula after removing Next operators.
     */
    formula::Formula* get_rmnext_formula(automata::TableauState* state) const;

    /**
     * @brief Clear the BDD cache
     */
    void clear_cache();

    /**
     * @brief Get statistics about BDD operations
     */
    struct Stats {
        int num_bdd_calls = 0;
        int num_safe_moves_generated = 0;
        int num_cache_hits = 0;
        int num_cache_misses = 0;
    };
    const Stats& get_stats() const { return stats_; }
    void reset_stats() { stats_ = {}; }

private:
#ifdef FORMULA_USE_CUDD
    /**
     * @brief CUDD manager (opaque pointer, defined in cpp)
     */
    struct CuddManager;
    std::unique_ptr<CuddManager> cudd_;
#endif

    /**
     * @brief Total number of variables
     */
    int num_variables_;

    /**
     * @brief Number of output (system) variables
     */
    int num_outputs_;

    /**
     * @brief BDD cache per TableauState
     *
     * Maps state pointer to its rm_next formula.
     * This allows different states to share/cache their formulas.
     */
    std::unordered_map<automata::TableauState*, formula::Formula*> state_formula_cache_;

    /**
     * @brief Current active formula (for fallback operations)
     */
    formula::Formula* current_formula_;

#ifdef FORMULA_USE_CUDD
    /**
     * @brief BDD cache per TableauState (actual BDD nodes)
     *
     * Maps state pointer to its BDD representation.
     */
    std::unordered_map<automata::TableauState*, DdNode*> state_bdd_cache_;

    /**
     * @brief Build BDD from a boolean formula
     *
     * @param f The formula (must be boolean, no Next/Until/Release)
     * @param pool Formula pool
     * @return BDD node (referenced, caller must deref)
     */
    DdNode* build_bdd_from_formula(formula::Formula* f, formula::FormulaPool& pool);

    /**
     * @brief Enumerate all satisfying assignments of a BDD
     *
     * @param bdd The BDD node
     * @param relevant_var_ids Only consider these variables
     * @return List of assignments (each is a set of variable IDs set to TRUE)
     */
    std::vector<Assignment> enumerate_bdd_satisfying_assignments(
        DdNode* bdd,
        const std::set<int>& relevant_var_ids) const;
#endif

    /**
     * @brief Statistics
     */
    mutable Stats stats_;

    /**
     * @brief Enumerate all possible output assignments
     *
     * @param relevant_output_var_ids Variables to enumerate
     * @return All 2^k possible assignments
     */
    std::vector<Assignment> enumerate_all_output_assignments(
        const std::set<int>& relevant_output_var_ids) const;

    /**
     * @brief Fallback: Enumerate safe moves using formula evaluation
     *
     * Used when CUDD is not available or BDD construction is complex.
     */
    std::vector<Assignment> enumerate_safe_moves_fallback(
        formula::Formula* rmnext_formula,
        const std::set<int>& relevant_output_var_ids) const;

    /**
     * @brief Evaluate a formula on an assignment
     *
     * @param f The formula to evaluate
     * @param assignment Set of variables set to TRUE
     * @return true if formula evaluates to true
     */
    bool evaluate_formula(formula::Formula* f, const Assignment& assignment) const;
};

/**
 * @brief Apply rm_next transformation to a formula
 *
 * Removes all Next operators by replacing X(φ) with True.
 * This extracts the boolean constraints from a temporal formula.
 *
 * Examples:
 *   X(p) → True
 *   p & X(q) → p & True → p
 *   X(p) | q → True | q → True
 *   !(X(p)) → !True → False
 *
 * @param f The input formula
 * @param pool Formula pool for creating new formulas
 * @return New formula with all X() replaced by True
 */
formula::Formula* apply_rm_next(formula::Formula* f, formula::FormulaPool& pool);

} // namespace synthesis

#endif // SYNTHESIS_BDD_MANAGER_HPP
