#include "formula/formula_z3.hpp"
#include <unordered_map>
#include <optional>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <chrono>

#ifdef FORMULA_USE_LOGGER
#include "log/logger.hpp"
#endif

namespace formula {

#ifdef FORMULA_USE_Z3

using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double, std::milli>;

std::optional<bool> FormulaZ3::are_equivalent(Formula* f1, Formula* f2,
                                                int max_bound,
                                                unsigned timeout_ms) {
    if (!f1 || !f2) return f1 == f2;

#ifdef FORMULA_USE_LOGGER
    LOG_INFO("Z3 are_equivalent called: max_bound={}, timeout={}ms", max_bound, timeout_ms);
#endif

    // Incremental mode with time estimation
    if (max_bound == -1) {
        return are_equivalent_incremental(f1, f2, -1, timeout_ms);
    }

    // Auto-detect bound if not specified
    int actual_bound = (max_bound <= 0)
        ? detect_bound(f1) * DEFAULT_BOUND_MULTIPLIER
        : max_bound;

#ifdef FORMULA_USE_LOGGER
    LOG_DEBUG("Auto-detected bound: {}, using: {}", detect_bound(f1), actual_bound);
#endif

    return are_equivalent_bounded(f1, f2, actual_bound, timeout_ms);
}

std::optional<bool> FormulaZ3::are_equivalent_incremental(Formula* f1, Formula* f2,
                                                           int max_bound,
                                                           unsigned timeout_ms) {
    std::vector<BmcAttempt> attempts;

    // Determine target bound
    int auto_detected = std::max(detect_bound(f1), detect_bound(f2));
    int target_bound = (max_bound <= 0) ? auto_detected * DEFAULT_BOUND_MULTIPLIER : max_bound;
    int x_depth = get_x_depth(f1, f2);

    // Count temporal operators for complexity estimation
    [[maybe_unused]] int temporal_count = count_temporal_ops(f1) + count_temporal_ops(f2);

#ifdef FORMULA_USE_LOGGER
    LOG_INFO("Incremental BMC: auto_detected_bound={}, target_bound={}, x_depth={}, temporal_ops={}",
             auto_detected, target_bound, x_depth, temporal_count);
#endif

    // Start with small bounds to measure time
    int start_bound = std::max(MIN_INCREMENTAL_START_BOUND, x_depth);

    for (int step = 0; step < MAX_INCREMENTAL_STEPS; ++step) {
        // Calculate next bound to try
        int bound;
        if (step == 0) {
            bound = start_bound;
        } else {
            // Exponential increase: 2, 4, 8, 16, ... up to target
            bound = std::min(start_bound * (1 << step), target_bound);
        }

        // Estimate time before running
        auto estimated_ms = estimate_time(attempts, bound);
        if (estimated_ms.has_value()) {
            double safe_estimate = *estimated_ms * TIME_ESTIMATION_SAFETY_FACTOR;
#ifdef FORMULA_USE_LOGGER
            LOG_DEBUG("Bound {}: estimated time {:.1f}ms (safe: {:.1f}ms)",
                     bound, *estimated_ms, safe_estimate);
#endif
            if (safe_estimate > timeout_ms) {
                // Would exceed timeout, abort
#ifdef FORMULA_USE_LOGGER
                LOG_WARN("Estimated time for bound={} is {:.1f}s, exceeding timeout of {:.1f}s. Aborting.",
                         bound, safe_estimate / 1000.0, timeout_ms / 1000.0);
#else
                std::cerr << "[Z3 BMC] Estimated time for bound=" << bound
                          << " is " << static_cast<int>(safe_estimate / 1000.0)
                          << "s, exceeding timeout of " << (timeout_ms / 1000.0) << "s. Aborting.\n";
#endif
                return std::nullopt;
            }
        }

        // Run BMC with this bound
#ifdef FORMULA_USE_LOGGER
        LOG_DEBUG("Running BMC with bound={}...", bound);
#endif
        auto start_time = Clock::now();
        auto result = are_equivalent_bounded(f1, f2, bound,
            std::min(static_cast<unsigned>(timeout_ms / 4), 30000u));  // Per-step timeout
        auto end_time = Clock::now();
        double elapsed_ms = Duration(end_time - start_time).count();

        attempts.push_back({bound, result, elapsed_ms});

#ifdef FORMULA_USE_LOGGER
        LOG_INFO("Bound {}: result={}, time={:.2f}ms",
                 bound,
                 result.has_value() ? (result.value() ? "equivalent" : "not_equiv") : "unknown",
                 elapsed_ms);
#endif

        // If we got a definitive answer, return it
        if (result.has_value()) {
            // Found counterexample or proved equivalence
#ifdef FORMULA_USE_LOGGER
            LOG_INFO("Definitive answer found: {}", result.value() ? "EQUIVALENT" : "NOT EQUIVALENT");
#endif
            return result;
        }

        // If we reached target bound, stop
        if (bound >= target_bound) {
            break;
        }
    }

    // Couldn't determine equivalence
#ifdef FORMULA_USE_LOGGER
    LOG_WARN("Unable to determine equivalence after {} attempts (up to bound {})",
             attempts.size(), attempts.back().bound);
#else
    std::cerr << "[Z3 BMC] Unable to determine equivalence after "
              << attempts.size() << " attempts (up to bound " << attempts.back().bound << ")\n";
#endif
    return std::nullopt;
}

std::optional<double> FormulaZ3::estimate_time(const std::vector<BmcAttempt>& attempts, int target_bound) {
    if (attempts.empty()) {
        return std::nullopt;
    }

    // Use the most recent attempt(s) for estimation
    // For formulas with U/R, complexity is roughly O(bound^2.5)
    // For pure X formulas, complexity is closer to O(bound)

    if (attempts.size() >= 2) {
        // Use two most recent data points to fit a power law
        const auto& a1 = attempts[attempts.size() - 2];
        const auto& a2 = attempts[attempts.size() - 1];

        if (a1.elapsed_ms > 0.1 && a2.elapsed_ms > 0.1 && a1.bound != a2.bound) {
            // Fit: time = c * bound^k
            // k = log(t2/t1) / log(b2/b1)
            double k = std::log(a2.elapsed_ms / a1.elapsed_ms) /
                      std::log(static_cast<double>(a2.bound) / a1.bound);
            double c = a1.elapsed_ms / std::pow(a1.bound, k);

            // Clamp k to reasonable range [1, 4]
            k = std::max(1.0, std::min(4.0, k));

#ifdef FORMULA_USE_LOGGER
            LOG_TRACE("Time estimation: k={:.2f}, c={:.4f}", k, c);
#endif

            double estimated = c * std::pow(target_bound, k);
            return estimated;
        }
    }

    // Fallback: use single data point with assumed exponent
    const auto& last = attempts.back();
    if (last.elapsed_ms > 0.1) {
        // Assume O(bound^2) for U/R formulas
        double k = 2.0;
        double c = last.elapsed_ms / std::pow(last.bound, k);
        return c * std::pow(target_bound, k);
    }

    return std::nullopt;
}

std::optional<bool> FormulaZ3::are_equivalent_bounded(Formula* f1, Formula* f2,
                                                       int bound,
                                                       unsigned timeout_ms) {
    try {
        z3::context ctx;
        std::unordered_map<TimeVar, z3::expr*, TimeVarHash> var_map;

        z3::solver solver(ctx);
        if (timeout_ms > 0) {
            solver.set("timeout", static_cast<unsigned>(timeout_ms));
        }

        // Convert both formulas at time 0
        z3::expr z1 = to_z3_bounded(f1, 0, bound, ctx, var_map);
        z3::expr z2 = to_z3_bounded(f2, 0, bound, ctx, var_map);

        // Check equivalence: (f1 XOR f2) must be unsatisfiable
        z3::expr xor_expr = (z1 && !z2) || (!z1 && z2);
        solver.add(xor_expr);

        z3::check_result result = solver.check();

        switch (result) {
            case z3::unsat:
                return true;   // Equivalent
            case z3::sat:
                return false;  // Found counterexample
            case z3::unknown:
                return std::nullopt;
        }

    } catch (const z3::exception& e) {
#ifdef FORMULA_USE_LOGGER
        LOG_ERROR("Z3 exception: {}", e.what());
#endif
        return std::nullopt;
    }

    return std::nullopt;
}

std::optional<bool> FormulaZ3::is_valid(Formula* f,
                                        int bound,
                                        unsigned timeout_ms) {
    if (!f) return false;

    int actual_bound = (bound <= 0) ? detect_bound(f) * DEFAULT_BOUND_MULTIPLIER : bound;

#ifdef FORMULA_USE_LOGGER
    LOG_DEBUG("is_valid: bound={}", actual_bound);
#endif

    try {
        z3::context ctx;
        std::unordered_map<TimeVar, z3::expr*, TimeVarHash> var_map;

        z3::solver solver(ctx);
        if (timeout_ms > 0) {
            solver.set("timeout", static_cast<unsigned>(timeout_ms));
        }

        z3::expr z = to_z3_bounded(f, 0, actual_bound, ctx, var_map);
        solver.add(!z);  // If !f is unsatisfiable, f is valid

        z3::check_result result = solver.check();

        switch (result) {
            case z3::unsat:
                return true;   // Valid
            case z3::sat:
                return false;  // Not valid (found counterexample)
            case z3::unknown:
                return std::nullopt;
        }

    } catch (const z3::exception&) {
        return std::nullopt;
    }

    return std::nullopt;
}

std::optional<bool> FormulaZ3::is_satisfiable(Formula* f,
                                               int bound,
                                               unsigned timeout_ms) {
    if (!f) return false;

    int actual_bound = (bound <= 0) ? detect_bound(f) * DEFAULT_BOUND_MULTIPLIER : bound;

    try {
        z3::context ctx;
        std::unordered_map<TimeVar, z3::expr*, TimeVarHash> var_map;

        z3::solver solver(ctx);
        if (timeout_ms > 0) {
            solver.set("timeout", static_cast<unsigned>(timeout_ms));
        }

        z3::expr z = to_z3_bounded(f, 0, actual_bound, ctx, var_map);
        solver.add(z);

        z3::check_result result = solver.check();

        switch (result) {
            case z3::sat:
                return true;   // Satisfiable
            case z3::unsat:
                return false;  // Unsatisfiable
            case z3::unknown:
                return std::nullopt;
        }

    } catch (const z3::exception&) {
        return std::nullopt;
    }

    return std::nullopt;
}

int FormulaZ3::get_x_depth(Formula* f1, Formula* f2) {
    return std::max(count_x_depth(f1), count_x_depth(f2));
}

int FormulaZ3::count_x_depth(Formula* f) {
    if (!f) return 0;

    switch (f->op()) {
        case Formula::OpType::True:
        case Formula::OpType::False:
        case Formula::OpType::Literal:
        case Formula::OpType::End:
            return 0;

        case Formula::OpType::Not:
            return count_x_depth(f->left());

        case Formula::OpType::And:
        case Formula::OpType::Or:
        case Formula::OpType::Until:
        case Formula::OpType::Release:
            return std::max(count_x_depth(f->left()), count_x_depth(f->right()));

        case Formula::OpType::Next:
            return 1 + count_x_depth(f->left());
    }

    return 0;
}

int FormulaZ3::detect_bound(Formula* f) {
    if (!f) return 1;

    // Bound based on formula structure
    switch (f->op()) {
        case Formula::OpType::True:
        case Formula::OpType::False:
        case Formula::OpType::Literal:
        case Formula::OpType::End:
            return 1;

        case Formula::OpType::Not:
            return detect_bound(f->left());

        case Formula::OpType::And:
        case Formula::OpType::Or:
            return std::max(detect_bound(f->left()), detect_bound(f->right()));

        case Formula::OpType::Next:
            return 1 + detect_bound(f->left());

        case Formula::OpType::Until:
        case Formula::OpType::Release: {
            int x_depth = std::max(count_x_depth(f->left()), count_x_depth(f->right()));
            int structural_depth = std::max(detect_bound(f->left()), detect_bound(f->right()));
            return structural_depth + x_depth + 2;
        }
    }

    return 1;
}

int FormulaZ3::count_temporal_ops(Formula* f) {
    if (!f) return 0;

    switch (f->op()) {
        case Formula::OpType::True:
        case Formula::OpType::False:
        case Formula::OpType::Literal:
        case Formula::OpType::End:
            return 0;

        case Formula::OpType::Not:
            return count_temporal_ops(f->left());

        case Formula::OpType::And:
        case Formula::OpType::Or:
            return count_temporal_ops(f->left()) + count_temporal_ops(f->right());

        case Formula::OpType::Next:
            return 1 + count_temporal_ops(f->left());

        case Formula::OpType::Until:
        case Formula::OpType::Release:
            return 1 + count_temporal_ops(f->left()) + count_temporal_ops(f->right());
    }

    return 0;
}

z3::expr FormulaZ3::to_z3_bounded(Formula* f,
                                   int time,
                                   int bound,
                                   z3::context& ctx,
                                   std::unordered_map<TimeVar, z3::expr*, TimeVarHash>& var_map) {
    if (!f) return ctx.bool_val(true);

    switch (f->op()) {
        case Formula::OpType::True:
            return ctx.bool_val(true);

        case Formula::OpType::False:
            return ctx.bool_val(false);

        case Formula::OpType::Literal: {
            TimeVar tv{f->var_id(), time};
            auto it = var_map.find(tv);
            if (it != var_map.end()) {
                return *(it->second);
            }
            // Create time-indexed variable: v<var_id>_<time>
            std::string name = "v" + std::to_string(f->var_id()) + "_" + std::to_string(time);
            z3::expr* var = new z3::expr(ctx.bool_const(name.c_str()));
            var_map[tv] = var;
            return *var;
        }

        case Formula::OpType::Not: {
            z3::expr left = to_z3_bounded(f->left(), time, bound, ctx, var_map);
            return !left;
        }

        case Formula::OpType::And: {
            z3::expr left = to_z3_bounded(f->left(), time, bound, ctx, var_map);
            z3::expr right = to_z3_bounded(f->right(), time, bound, ctx, var_map);
            return left && right;
        }

        case Formula::OpType::Or: {
            z3::expr left = to_z3_bounded(f->left(), time, bound, ctx, var_map);
            z3::expr right = to_z3_bounded(f->right(), time, bound, ctx, var_map);
            return left || right;
        }

        case Formula::OpType::Next: {
            // X f at time t: f at time t+1, or false if t >= bound
            if (time >= bound) {
                return ctx.bool_val(false);  // No next state at bound
            }
            return to_z3_bounded(f->left(), time + 1, bound, ctx, var_map);
        }

        case Formula::OpType::Until: {
            // f U g at time t:
            //   g_t ∨ (f_t ∧ g_{t+1}) ∨ (f_t ∧ f_{t+1} ∧ g_{t+2}) ∨ ...
            Formula* left_f = f->left();
            Formula* right_g = f->right();

            z3::expr result = ctx.bool_val(false);

            for (int t_prime = time; t_prime <= bound; ++t_prime) {
                z3::expr conj = ctx.bool_val(true);

                // f holds from time to t_prime-1
                for (int t_mid = time; t_mid < t_prime; ++t_mid) {
                    z3::expr f_at_t = to_z3_bounded(left_f, t_mid, bound, ctx, var_map);
                    conj = conj && f_at_t;
                }

                z3::expr g_at_tp = to_z3_bounded(right_g, t_prime, bound, ctx, var_map);
                conj = conj && g_at_tp;

                result = result || conj;
            }

            return result;
        }

        case Formula::OpType::Release: {
            // f R g at time t: g holds continuously until f becomes true
            // Disjunction over all possible "release points"
            Formula* left_f = f->left();
            Formula* right_g = f->right();

            z3::expr result = ctx.bool_val(false);

            for (int t_prime = time; t_prime <= bound; ++t_prime) {
                z3::expr conj = ctx.bool_val(true);

                // f is false from time to t_prime-1
                for (int t_mid = time; t_mid < t_prime; ++t_mid) {
                    z3::expr f_at_tm = to_z3_bounded(left_f, t_mid, bound, ctx, var_map);
                    conj = conj && !f_at_tm;
                }

                // g holds from time to t_prime
                for (int t_mid = time; t_mid <= t_prime; ++t_mid) {
                    z3::expr g_at_tm = to_z3_bounded(right_g, t_mid, bound, ctx, var_map);
                    conj = conj && g_at_tm;
                }

                // f is true at t_prime (or this is the last case)
                if (t_prime < bound) {
                    z3::expr f_at_tp = to_z3_bounded(left_f, t_prime, bound, ctx, var_map);
                    conj = conj && f_at_tp;
                }

                result = result || conj;
            }

            return result;
        }

        case Formula::OpType::End:
            return ctx.bool_val(false);
    }

    return ctx.bool_val(true);
}

bool FormulaZ3::has_temporal_operators(Formula* f) {
    if (!f) return false;

    switch (f->op()) {
        case Formula::OpType::True:
        case Formula::OpType::False:
        case Formula::OpType::Literal:
        case Formula::OpType::End:
            return false;

        case Formula::OpType::Not:
            return has_temporal_operators(f->left());

        case Formula::OpType::And:
        case Formula::OpType::Or:
            return has_temporal_operators(f->left()) || has_temporal_operators(f->right());

        case Formula::OpType::Next:
        case Formula::OpType::Until:
        case Formula::OpType::Release:
            return true;
    }

    return false;
}

#endif // FORMULA_USE_Z3

} // namespace formula
