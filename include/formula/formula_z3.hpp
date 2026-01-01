#ifndef FORMULA_Z3_HPP
#define FORMULA_Z3_HPP

#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include <optional>
#include <unordered_set>

// Z3 configuration - must be included outside namespace formula
#ifdef FORMULA_USE_Z3
#include <z3++.h>
#endif

namespace formula {

/**
 * @brief Z3-based formula equivalence checker
 *
 * Uses Z3 SMT solver to determine formula equivalence by checking
 * whether (f1 XOR f2) is unsatisfiable.
 *
 * This provides exact equivalence checking for formulas with any number
 * of variables, unlike the truth table method which is limited to 4 variables.
 */
class FormulaZ3 {
public:
#ifdef FORMULA_USE_Z3
    /**
     * @brief Check if two formulas are equivalent using Z3
     * @param f1 First formula
     * @param f2 Second formula
     * @param timeout_ms Timeout in milliseconds (0 = no timeout)
     * @return true if equivalent, false if not, std::nullopt if unknown/timeout
     */
    static std::optional<bool> are_equivalent(Formula* f1, Formula* f2,
                                                unsigned timeout_ms = 5000);

    /**
     * @brief Check if a formula is valid (always true) using Z3
     * @param f Formula to check
     * @param timeout_ms Timeout in milliseconds
     * @return true if valid, false if not, std::nullopt if unknown/timeout
     */
    static std::optional<bool> is_valid(Formula* f, unsigned timeout_ms = 5000);

    /**
     * @brief Check if a formula is satisfiable using Z3
     * @param f Formula to check
     * @param timeout_ms Timeout in milliseconds
     * @return true if satisfiable, false if unsatisfiable, std::nullopt if unknown/timeout
     */
    static std::optional<bool> is_satisfiable(Formula* f, unsigned timeout_ms = 5000);

    /**
     * @brief Find a counterexample for non-equivalent formulas
     * @param f1 First formula
     * @param f2 Second formula
     * @param timeout_ms Timeout in milliseconds
     * @return Variable assignment (as set of true variables), or std::nullopt
     */
    static std::optional<std::unordered_set<int>> get_counterexample(
        Formula* f1, Formula* f2, unsigned timeout_ms = 5000);

private:
    // Convert LTLf formula to Z3 expression
    static z3::expr to_z3(Formula* f,
                          z3::context& ctx,
                          std::unordered_map<int, z3::expr*>& var_map);

#else
    // Stub implementations when Z3 is not available
    static std::optional<bool> are_equivalent(Formula*, Formula*, unsigned timeout_ms = 5000) {
        (void)timeout_ms;
        return std::nullopt;
    }
    static std::optional<bool> is_valid(Formula*, unsigned timeout_ms = 5000) {
        (void)timeout_ms;
        return std::nullopt;
    }
    static std::optional<bool> is_satisfiable(Formula*, unsigned timeout_ms = 5000) {
        (void)timeout_ms;
        return std::nullopt;
    }
    static std::optional<std::unordered_set<int>> get_counterexample(
        Formula*, Formula*, unsigned timeout_ms = 5000) {
        (void)timeout_ms;
        return std::nullopt;
    }
#endif
};

} // namespace formula

#endif // FORMULA_Z3_HPP
