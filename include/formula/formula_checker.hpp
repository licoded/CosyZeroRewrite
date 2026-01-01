#ifndef FORMULA_CHECKER_HPP
#define FORMULA_CHECKER_HPP

#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace formula {

/**
 * @brief Formula equivalence and property checking utilities
 *
 * Provides:
 * - Semantic equivalence checking (truth table for small formulas)
 * - Statistical equivalence (sampling for large formulas)
 * - Property checking (NNF, XNF, etc.)
 * - Formula analysis utilities
 */
class FormulaChecker {
public:
    // =========================================================================
    // Equivalence Checking
    // =========================================================================

    /**
     * @brief Check if two formulas are semantically equivalent (exact)
     * @param pool FormulaPool for creating temporary formulas
     * @param f1 First formula
     * @param f2 Second formula
     * @return true if formulas are equivalent
     *
     * Uses truth table enumeration. Suitable for formulas with ≤ 4 variables.
     * For formulas with more variables, use likely_equivalent().
     *
     * Time complexity: O(2^n * size(f)) where n is number of variables.
     */
    static bool are_equivalent(FormulaPool& pool, Formula* f1, Formula* f2);

    /**
     * @brief Check if two formulas are likely equivalent (statistical)
     * @param pool FormulaPool for creating temporary formulas
     * @param f1 First formula
     * @param f2 Second formula
     * @param samples Number of random assignments to test
     * @return true if formulas agree on all samples
     *
     * Uses random model sampling. Suitable for formulas with > 4 variables.
     * May produce false positives (unlikely), but never false negatives if
     * a counterexample is found.
     *
     * Time complexity: O(samples * size(f))
     */
    static bool likely_equivalent(FormulaPool& pool, Formula* f1, Formula* f2,
                                   size_t samples = 1000);

    // =========================================================================
    // Property Checking
    // =========================================================================

    /**
     * @brief Check if formula is in Negation Normal Form
     * @param f Formula to check
     * @return true if formula is in NNF
     *
     * NNF property: All negations appear only directly in front of literals.
     * - No negation in front of And/Or/Until/Release/Next
     * - !literal is allowed
     */
    static bool is_nnf(Formula* f);

    /**
     * @brief Check if formula is in neXt Normal Form
     * @param f Formula to check
     * @return true if formula is in XNF
     *
     * XNF property: Primitive subformulas pa(f) contain only literals and Next.
     * - No Until/Release in primitive subformulas
     * - Temporal formulas can only appear inside Next operators
     */
    static bool is_xnf(Formula* f);

    /**
     * @brief Check if formula is a literal
     * @param f Formula to check
     * @return true if formula is a literal (variable or negated variable)
     */
    static bool is_literal(Formula* f);

    /**
     * @brief Check if formula is a temporal formula
     * @param f Formula to check
     * @return true if formula contains temporal operators
     */
    static bool is_temporal(Formula* f);

    // =========================================================================
    // Formula Analysis
    // =========================================================================

    /**
     * @brief Count the number of nodes in the formula AST
     * @param f Formula to analyze
     * @return Number of nodes
     */
    static size_t formula_size(Formula* f);

    /**
     * @brief Get the depth of the formula AST
     * @param f Formula to analyze
     * @return Maximum depth
     */
    static size_t formula_depth(Formula* f);

    /**
     * @brief Get all unique variable IDs in the formula
     * @param f Formula to analyze
     * @return Set of variable IDs
     */
    static std::unordered_set<int> get_variables(Formula* f);

    /**
     * @brief Get all literal formulas in the formula
     * @param f Formula to analyze
     * @return Vector of literal formulas
     */
    static std::vector<Formula*> get_literals(Formula* f);

    /**
     * @brief Get all primitive subformulas pa(f)
     * @param f Formula to analyze
     * @return Set of primitive subformulas
     *
     * Primitive subformulas are literals and temporal subformulas
     * whose primary connective is temporal.
     */
    static std::unordered_set<Formula*> get_primitive_subformulas(Formula* f);

private:
    // =========================================================================
    // Internal helpers
    // =========================================================================

    // Truth table evaluation
    struct Assignment {
        std::unordered_set<int> true_vars;
    };

    static bool evaluate(Formula* f, const Assignment& assignment);
    static void generate_assignments(const std::unordered_set<int>& vars,
                                     size_t index,
                                     std::vector<int>& var_list,
                                     Assignment& current,
                                     std::vector<Assignment>& result);

    // Property checking helpers
    static bool check_nnf_recursive(Formula* f);
    static bool check_xnf_recursive(Formula* f, std::unordered_set<Formula*>& visited, bool inside_next);

    // Analysis helpers
    static void collect_variables(Formula* f, std::unordered_set<int>& vars);
    static void collect_literals(Formula* f, std::vector<Formula*>& literals);
    static void collect_primitives(Formula* f, std::unordered_set<Formula*>& primitives);
    static size_t compute_depth(Formula* f);
};

} // namespace formula

#endif // FORMULA_CHECKER_HPP
