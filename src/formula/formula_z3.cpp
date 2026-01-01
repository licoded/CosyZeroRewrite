#include "formula/formula_z3.hpp"
#include <unordered_map>
#include <optional>

namespace formula {

#ifdef FORMULA_USE_Z3

std::optional<bool> FormulaZ3::are_equivalent(Formula* f1, Formula* f2,
                                                unsigned timeout_ms) {
    if (!f1 || !f2) return f1 == f2;

    try {
        z3::context ctx;
        std::unordered_map<int, z3::expr*> var_map;

        // Create solver with timeout
        z3::solver solver(ctx);
        if (timeout_ms > 0) {
            solver.set("timeout", static_cast<unsigned>(timeout_ms));
        }

        // Convert both formulas to Z3 expressions
        z3::expr z1 = to_z3(f1, ctx, var_map);
        z3::expr z2 = to_z3(f2, ctx, var_map);

        // Check equivalence: !(f1 XOR f2) must be valid
        // Or equivalently: (f1 == f2) must be valid
        // Which means: (f1 && !f2) || (!f1 && f2) must be unsatisfiable
        z3::expr xor_expr = (z1 && !z2) || (!z1 && z2);

        solver.add(xor_expr);

        z3::check_result result = solver.check();

        switch (result) {
            case z3::unsat:
                // XOR is unsatisfiable, so formulas are equivalent
                return true;
            case z3::sat:
                // Found a counterexample
                return false;
            case z3::unknown:
                // Timeout or unknown
                return std::nullopt;
        }

    } catch (const z3::exception& e) {
        // Z3 error - return unknown
        return std::nullopt;
    }

    return std::nullopt;
}

std::optional<bool> FormulaZ3::is_valid(Formula* f, unsigned timeout_ms) {
    if (!f) return false;

    try {
        z3::context ctx;
        std::unordered_map<int, z3::expr*> var_map;

        z3::solver solver(ctx);
        if (timeout_ms > 0) {
            solver.set("timeout", static_cast<unsigned>(timeout_ms));
        }

        z3::expr z = to_z3(f, ctx, var_map);
        solver.add(!z);  // Negation: if !f is unsatisfiable, f is valid

        z3::check_result result = solver.check();

        switch (result) {
            case z3::unsat:
                return true;   // Negation is unsatisfiable, so f is valid
            case z3::sat:
                return false;  // Found a model where f is false
            case z3::unknown:
                return std::nullopt;
        }

    } catch (const z3::exception&) {
        return std::nullopt;
    }

    return std::nullopt;
}

std::optional<bool> FormulaZ3::is_satisfiable(Formula* f, unsigned timeout_ms) {
    if (!f) return false;

    try {
        z3::context ctx;
        std::unordered_map<int, z3::expr*> var_map;

        z3::solver solver(ctx);
        if (timeout_ms > 0) {
            solver.set("timeout", static_cast<unsigned>(timeout_ms));
        }

        z3::expr z = to_z3(f, ctx, var_map);
        solver.add(z);

        z3::check_result result = solver.check();

        switch (result) {
            case z3::sat:
                return true;
            case z3::unsat:
                return false;
            case z3::unknown:
                return std::nullopt;
        }

    } catch (const z3::exception&) {
        return std::nullopt;
    }

    return std::nullopt;
}

std::optional<std::unordered_set<int>> FormulaZ3::get_counterexample(
    Formula* f1, Formula* f2, unsigned timeout_ms) {
    if (!f1 || !f2) return std::nullopt;

    try {
        z3::context ctx;
        std::unordered_map<int, z3::expr*> var_map;

        z3::solver solver(ctx);
        if (timeout_ms > 0) {
            solver.set("timeout", static_cast<unsigned>(timeout_ms));
        }

        z3::expr z1 = to_z3(f1, ctx, var_map);
        z3::expr z2 = to_z3(f2, ctx, var_map);

        z3::expr xor_expr = (z1 && !z2) || (!z1 && z2);
        solver.add(xor_expr);

        z3::check_result result = solver.check();

        if (result == z3::sat) {
            // Extract model
            z3::model model = solver.get_model();
            std::unordered_set<int> true_vars;

            for (const auto& [var_id, var_expr_ptr] : var_map) {
                try {
                    z3::expr val = model.eval(*var_expr_ptr);
                    if (val.is_true()) {
                        true_vars.insert(var_id);
                    }
                } catch (...) {
                    // Variable not in model, skip
                }
            }

            return true_vars;
        }

    } catch (const z3::exception&) {
        return std::nullopt;
    }

    return std::nullopt;
}

z3::expr FormulaZ3::to_z3(Formula* f,
                          z3::context& ctx,
                          std::unordered_map<int, z3::expr*>& var_map) {
    if (!f) return ctx.bool_val(true);

    switch (f->op()) {
        case Formula::OpType::True:
            return ctx.bool_val(true);

        case Formula::OpType::False:
            return ctx.bool_val(false);

        case Formula::OpType::Literal: {
            int var_id = f->var_id();
            auto it = var_map.find(var_id);
            if (it != var_map.end()) {
                return *(it->second);
            }
            // Create new boolean variable for this formula variable
            z3::expr* var = new z3::expr(ctx.bool_const(("v" + std::to_string(var_id)).c_str()));
            var_map[var_id] = var;
            return *var;
        }

        case Formula::OpType::Not: {
            z3::expr left = to_z3(f->left(), ctx, var_map);
            return !left;
        }

        case Formula::OpType::And: {
            z3::expr left = to_z3(f->left(), ctx, var_map);
            z3::expr right = to_z3(f->right(), ctx, var_map);
            return left && right;
        }

        case Formula::OpType::Or: {
            z3::expr left = to_z3(f->left(), ctx, var_map);
            z3::expr right = to_z3(f->right(), ctx, var_map);
            return left || right;
        }

        case Formula::OpType::Next:
            // For propositional equivalence checking, X(f) ≈ f
            // (in a single-timepoint model, next is identity)
            return to_z3(f->left(), ctx, var_map);

        case Formula::OpType::Until: {
            // f U g in LTLf has complex semantics
            // For propositional approximation: f U g ≈ g | f
            // This is a conservative approximation for checking
            z3::expr left = to_z3(f->left(), ctx, var_map);
            z3::expr right = to_z3(f->right(), ctx, var_map);
            // Note: This is an approximation; full LTLf semantics require bounded model checking
            return right || left;
        }

        case Formula::OpType::Release: {
            // f R g in LTLf: g must hold until (and including) when f becomes true
            // For propositional approximation when g is constant:
            // - true R true ≈ true
            // - f R true ≈ true
            // - f R false ≈ false
            z3::expr left = to_z3(f->left(), ctx, var_map);
            z3::expr right = to_z3(f->right(), ctx, var_map);

            // Check if right is constant true
            if (right.is_true()) {
                return ctx.bool_val(true);
            }
            // Check if right is constant false
            if (right.is_false()) {
                return ctx.bool_val(false);
            }
            // Otherwise, approximate as g (conservative)
            return right;
        }

        case Formula::OpType::End:
            // End marker: false in propositional logic
            return ctx.bool_val(false);
    }

    return ctx.bool_val(true);
}

#endif // FORMULA_USE_Z3

} // namespace formula
