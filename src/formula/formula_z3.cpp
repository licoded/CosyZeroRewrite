#include "formula/formula_z3.hpp"
#include <unordered_map>
#include <optional>
#include <algorithm>

namespace formula {

#ifdef FORMULA_USE_Z3

std::optional<bool> FormulaZ3::are_equivalent(Formula* f1, Formula* f2,
                                                int max_bound,
                                                unsigned timeout_ms) {
    if (!f1 || !f2) return f1 == f2;

    // Incremental checking: start small, increase until counterexample or max bound
    if (max_bound == -1) {
        // Start with minimum bound (X operator depth)
        int min_bound = get_x_depth(f1, f2);
        int auto_max_bound = std::max(detect_bound(f1), detect_bound(f2));

        // Try incrementally: min_bound, min_bound*2, ..., auto_max_bound
        for (int bound = min_bound; bound <= auto_max_bound; bound = std::min(bound + 2, auto_max_bound + 1)) {
            auto result = are_equivalent_bounded(f1, f2, bound, timeout_ms / std::max(1, auto_max_bound / 2));
            if (result.has_value()) {
                // If we found a counterexample (sat), it's definitely not equivalent
                // If we proved equivalence (unsat), it's definitely equivalent
                return result;
            }
            // If timeout/unknown, try with larger bound
        }
        return std::nullopt;  // Couldn't determine with any bound up to auto_max_bound
    }

    // Auto-detect bound if not specified
    int actual_bound = (max_bound <= 0) ? std::max(detect_bound(f1), detect_bound(f2)) : max_bound;

    return are_equivalent_bounded(f1, f2, actual_bound, timeout_ms);
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

    } catch (const z3::exception&) {
        return std::nullopt;
    }

    return std::nullopt;
}

std::optional<bool> FormulaZ3::is_valid(Formula* f,
                                        int bound,
                                        unsigned timeout_ms) {
    if (!f) return false;

    int actual_bound = (bound <= 0) ? detect_bound(f) : bound;

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

    int actual_bound = (bound <= 0) ? detect_bound(f) : bound;

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
    // For U/R: need enough steps for the "eventually" part to happen
    // For nested X: need at least the X depth
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
            // For f U g or f R g, we need enough steps for:
            // 1. The nested structure
            // 2. The temporal aspect (g might be satisfied at various times)
            // Heuristic: max(left depth, right depth) + X depth + 2
            int x_depth = std::max(count_x_depth(f->left()), count_x_depth(f->right()));
            int structural_depth = std::max(detect_bound(f->left()), detect_bound(f->right()));
            return structural_depth + x_depth + 2;
        }
    }

    return 1;
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
