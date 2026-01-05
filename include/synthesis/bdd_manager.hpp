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
 * The BDD is used to efficiently check the satisfiability of boolean formulas
 * over system and environment variables.
 *
 * Example:
 *   Formula: (s1 & X(!e1)) | (!s1 & X(e1))
 *   rm_next: (s1 & True) | (!s1 & True) = True
 *   If system picks s1=True, environment can pick e1=True → constraint satisfied
 *   If system picks s1=False, environment can pick e1=False → constraint satisfied
 */

#ifndef SYNTHESIS_BDD_MANAGER_HPP
#define SYNTHESIS_BDD_MANAGER_HPP

#include "formula/formula.hpp"
#include "automata/tableau.hpp"
#include <memory>
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
 * 3. Satisfiability check: Test if an assignment satisfies the BDD
 * 4. Existential check: Test if there exists an env move for a given sys move
 *
 * Implementation note: When CUDD is not available, this class falls back
 * to a simple formula evaluator (less efficient but still correct).
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
     * @brief Build BDD from formula by removing Next operators (rm_next)
     *
     * This extracts the boolean constraints from a formula by:
     * 1. Replacing X(φ) with True (removing temporal constraints)
     * 2. Converting the resulting boolean formula to BDD
     *
     * @param phi The formula (typically from TableauState::prop_atoms)
     * @param pool Formula pool for creating intermediate formulas
     * @return true if BDD construction succeeded
     */
    bool build_from_formula_rmnext(formula::Formula* phi, formula::FormulaPool& pool);

    /**
     * @brief Check if a complete assignment satisfies the BDD
     *
     * @param assignment Set of variable indices set to TRUE
     * @return true if the assignment satisfies all boolean constraints
     */
    bool satisfies(const Assignment& assignment) const;

    /**
     * @brief Check if there exists an env move that satisfies the constraint
     *
     * Given a system move (output assignment), check if there exists at least
     * one environment move (input assignment) such that the combined assignment
     * satisfies the boolean constraint.
     *
     * This is the key operation for Rule B optimization:
     * - If exists env move: sys move is SAFE (keep for consideration)
     * - If no env move exists: sys move is UNSAFE (prune immediately)
     *
     * @param sys_output System's output assignment (variables set to TRUE)
     * @return true if there exists at least one satisfying env move
     */
    bool exists_env_move_for_sys(const Assignment& sys_output) const;

    /**
     * @brief Filter system moves to only safe ones
     *
     * Given a list of candidate system moves, return only those that are safe
     * (i.e., there exists at least one env move that satisfies the constraint).
     *
     * @param sys_outputs List of system output assignments
     * @return Filtered list containing only safe system moves
     */
    std::vector<Assignment> filter_safe_moves(const std::vector<Assignment>& sys_outputs) const;

    /**
     * @brief Get the rm_next formula (for debugging)
     *
     * Returns the formula after removing Next operators.
     */
    formula::Formula* get_rmnext_formula() const { return rmnext_formula_; }

    /**
     * @brief Reset the BDD manager for a new formula
     */
    void clear();

    /**
     * @brief Get statistics about BDD operations
     */
    struct Stats {
        int num_bdd_calls = 0;
        int num_safe_moves = 0;
        int num_unsafe_moves_filtered = 0;
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
     * @brief The formula after rm_next (for fallback evaluation)
     */
    formula::Formula* rmnext_formula_;

    /**
     * @brief Statistics
     */
    mutable Stats stats_;

    /**
     * @brief Build BDD node from formula (recursive)
     *
     * @param f The formula to convert
     * @param pool Formula pool
     * @return BDD node index (or -1 on error)
     */
#ifdef FORMULA_USE_CUDD
    int build_bdd_from_formula(formula::Formula* f, formula::FormulaPool& pool);
#endif

    /**
     * @brief Fallback: Check if assignment satisfies formula (without BDD)
     *
     * Used when CUDD is not available.
     */
    bool satisfies_fallback(const Assignment& assignment, formula::Formula* f) const;

    /**
     * @brief Fallback: Check if exists env move (without BDD)
     *
     * Enumerates all possible input assignments to find one that satisfies.
     */
    bool exists_env_move_fallback(const Assignment& sys_output) const;

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
