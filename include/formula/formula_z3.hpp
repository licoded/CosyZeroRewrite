#ifndef FORMULA_Z3_HPP
#define FORMULA_Z3_HPP

#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include <optional>
#include <unordered_set>
#include <vector>

// Z3 configuration - must be included outside namespace formula
#ifdef FORMULA_USE_Z3
#include <z3++.h>
#endif

namespace formula {

/**
 * @brief Z3-based formula equivalence checker with BMC for LTLf
 *
 * Uses Bounded Model Checking to handle temporal operators:
 * - Time is unrolled up to k steps (where k is formula depth or specified)
 * - Each proposition p becomes p_0, p_1, ..., p_k
 * - Temporal operators are expanded according to LTLf semantics
 *
 * Supported operators: !, &, |, X, U, R
 */
class FormulaZ3 {
public:
#ifdef FORMULA_USE_Z3
    /**
     * @brief Check if two formulas are equivalent using Z3 with BMC
     * @param f1 First formula
     * @param f2 Second formula
     * @param max_bound Maximum unrolling bound (0 = auto-detect, -1 = incremental)
     * @param timeout_ms Timeout in milliseconds (0 = no timeout)
     * @return true if equivalent, false if not, std::nullopt if timeout/unknown
     *
     * Bound selection:
     * - If max_bound = -1: use incremental checking (start small, increase as needed)
     * - If max_bound = 0: auto-detect from formula structure
     * - If max_bound > 0: use specified bound
     */
    static std::optional<bool> are_equivalent(Formula* f1, Formula* f2,
                                                int max_bound = 0,
                                                unsigned timeout_ms = 5000);

    /**
     * @brief Check if a formula is valid (always true) using Z3 with BMC
     * @param f Formula to check
     * @param bound Unrolling bound (0 = auto-detect)
     * @param timeout_ms Timeout in milliseconds
     * @return true if valid, false if not, std::nullopt if timeout/unknown
     */
    static std::optional<bool> is_valid(Formula* f,
                                        int bound = 0,
                                        unsigned timeout_ms = 5000);

    /**
     * @brief Check if a formula is satisfiable using Z3 with BMC
     * @param f Formula to check
     * @param bound Unrolling bound (0 = auto-detect)
     * @param timeout_ms Timeout in milliseconds
     * @return true if satisfiable, false if unsatisfiable, std::nullopt if timeout/unknown
     */
    static std::optional<bool> is_satisfiable(Formula* f,
                                               int bound = 0,
                                               unsigned timeout_ms = 5000);

private:
    // Time-indexed variable: var_id at time step
    struct TimeVar {
        int var_id;
        int time;

        bool operator==(const TimeVar& other) const {
            return var_id == other.var_id && time == other.time;
        }
    };

    struct TimeVarHash {
        size_t operator()(const TimeVar& tv) const {
            return static_cast<size_t>(tv.var_id) * 31 + static_cast<size_t>(tv.time);
        }
    };

    // Helper for bounded equivalence checking
    static std::optional<bool> are_equivalent_bounded(Formula* f1, Formula* f2,
                                                       int bound,
                                                       unsigned timeout_ms);

    // Get X operator depth (minimum bound needed)
    static int get_x_depth(Formula* f1, Formula* f2);
    static int count_x_depth(Formula* f);

    // Auto-detect appropriate bound from formula structure
    static int detect_bound(Formula* f);

    // Convert LTLf formula to Z3 expression at specific time with bounded unrolling
    static z3::expr to_z3_bounded(Formula* f,
                                   int time,
                                   int bound,
                                   z3::context& ctx,
                                   std::unordered_map<TimeVar, z3::expr*, TimeVarHash>& var_map);

    // Check if formula contains temporal operators
    static bool has_temporal_operators(Formula* f);

#else
    // Stub implementations when Z3 is not available
    static std::optional<bool> are_equivalent(Formula*, Formula*, int = 0, unsigned = 5000) {
        return std::nullopt;
    }
    static std::optional<bool> is_valid(Formula*, int = 0, unsigned = 5000) {
        return std::nullopt;
    }
    static std::optional<bool> is_satisfiable(Formula*, int = 0, unsigned = 5000) {
        return std::nullopt;
    }
#endif
};

} // namespace formula

#endif // FORMULA_Z3_HPP
