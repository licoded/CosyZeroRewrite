#ifndef FORMULA_Z3_HPP
#define FORMULA_Z3_HPP

#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include <optional>
#include <unordered_set>
#include <vector>
#include <chrono>

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
     * @param max_bound Maximum unrolling bound (-1 = incremental with time estimation)
     * @param timeout_ms Total timeout in milliseconds (0 = no limit)
     * @return true if equivalent, false if not, std::nullopt if timeout/unknown
     *
     * Bound selection:
     * - max_bound = -1: incremental with 8x auto-detected bound, time-limited
     * - max_bound = 0: auto-detect (8x multiplier)
     * - max_bound > 0: use specified bound
     *
     * Time estimation:
     * - Starts with small bounds to measure actual time
     * - Extrapolates using O(bound^2.5) complexity model for U/R formulas
     * - Aborts if estimated time exceeds timeout_ms / 2
     */
    static std::optional<bool> are_equivalent(Formula* f1, Formula* f2,
                                                int max_bound = -1,
                                                unsigned timeout_ms = 300000);

    /**
     * @brief Check if a formula is valid (always true) using Z3 with BMC
     * @param f Formula to check
     * @param bound Unrolling bound (0 = 8x auto-detect)
     * @param timeout_ms Timeout in milliseconds
     * @return true if valid, false if not, std::nullopt if timeout/unknown
     */
    static std::optional<bool> is_valid(Formula* f,
                                        int bound = 0,
                                        unsigned timeout_ms = 5000);

    /**
     * @brief Check if a formula is satisfiable using Z3 with BMC
     * @param f Formula to check
     * @param bound Unrolling bound (0 = 8x auto-detect)
     * @param timeout_ms Timeout in milliseconds
     * @return true if satisfiable, false if unsatisfiable, std::nullopt if timeout/unknown
     */
    static std::optional<bool> is_satisfiable(Formula* f,
                                               int bound = 0,
                                               unsigned timeout_ms = 5000);

    // Configuration constants
    static constexpr int DEFAULT_BOUND_MULTIPLIER = 8;  // 8x auto-detected bound
    static constexpr double TIME_ESTIMATION_SAFETY_FACTOR = 2.0;  // Safety margin
    static constexpr int MIN_INCREMENTAL_START_BOUND = 2;  // Start incremental from here
    static constexpr int MAX_INCREMENTAL_STEPS = 10;  // Max incremental iterations

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

    // Result from a single BMC attempt
    struct BmcAttempt {
        int bound;
        std::optional<bool> result;
        double elapsed_ms;
    };

    // Helper for bounded equivalence checking
    static std::optional<bool> are_equivalent_bounded(Formula* f1, Formula* f2,
                                                       int bound,
                                                       unsigned timeout_ms);

    // Incremental checking with time estimation
    static std::optional<bool> are_equivalent_incremental(Formula* f1, Formula* f2,
                                                           int max_bound,
                                                           unsigned timeout_ms);

    // Estimate time for a given bound based on previous attempts
    static std::optional<double> estimate_time(const std::vector<BmcAttempt>& attempts, int target_bound);

    // Get X operator depth (minimum bound needed)
    static int get_x_depth(Formula* f1, Formula* f2);
    static int count_x_depth(Formula* f);

    // Auto-detect appropriate bound from formula structure
    static int detect_bound(Formula* f);

    // Count U/R operators (for complexity estimation)
    static int count_temporal_ops(Formula* f);

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
    static std::optional<bool> are_equivalent(Formula*, Formula*, int = -1, unsigned = 300000) {
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
