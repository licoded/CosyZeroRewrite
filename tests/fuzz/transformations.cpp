#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"
#include "formula/formula_checker.hpp"
#include <cstdint>
#include <cstddef>

using namespace formula;

// Generate a formula from random bytes
Formula* generate_formula(FormulaPool& pool, const uint8_t* data, size_t size, size_t& pos) {
    if (pos >= size) return pool.create_true();

    uint8_t choice = data[pos++] % 20;

    switch (choice) {
        case 0: return pool.create_true();
        case 1: return pool.create_false();
        case 2: {
            int var_id = data[pos++] % 10;
            return pool.create(Formula::OpType::Literal, nullptr, nullptr, var_id);
        }
        case 3: {
            Formula* left = generate_formula(pool, data, size, pos);
            return pool.create_not(left);
        }
        case 4: {
            Formula* left = generate_formula(pool, data, size, pos);
            Formula* right = generate_formula(pool, data, size, pos);
            return pool.create_and(left, right);
        }
        case 5: {
            Formula* left = generate_formula(pool, data, size, pos);
            Formula* right = generate_formula(pool, data, size, pos);
            return pool.create_or(left, right);
        }
        case 6: {
            Formula* left = generate_formula(pool, data, size, pos);
            return pool.create_next(left);
        }
        case 7: {
            Formula* left = generate_formula(pool, data, size, pos);
            Formula* right = generate_formula(pool, data, size, pos);
            return pool.create_until(left, right);
        }
        case 8: {
            Formula* left = generate_formula(pool, data, size, pos);
            Formula* right = generate_formula(pool, data, size, pos);
            return pool.create_release(left, right);
        }
        default: return pool.create_true();
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Limit input size to prevent excessive recursion
    if (size > 256) return 0;
    if (size < 4) return 0;

    FormulaPool pool;
    size_t pos = 0;

    // Generate formula from random bytes
    Formula* f = generate_formula(pool, data, size, pos);

    if (!f) return 0;

    // Property 1: NNF should preserve semantics (for small formulas)
    if (FormulaChecker::get_variables(f).size() <= 4) {
        Formula* nnf = f->nnf(pool);
        if (nnf) {
            // Should be equivalent
            bool equiv = FormulaChecker::are_equivalent(pool, f, nnf);
            if (!equiv) {
                // Found a bug: NNF doesn't preserve semantics
                __builtin_trap();
            }
            // Should actually be in NNF
            if (!FormulaChecker::is_nnf(nnf)) {
                __builtin_trap();
            }
        }
    }

    // Property 2: XNF should preserve semantics
    if (FormulaChecker::get_variables(f).size() <= 4) {
        Formula* xnf = f->xnf_with_tail(pool);
        if (xnf) {
            bool equiv = FormulaChecker::are_equivalent(pool, f, xnf);
            if (!equiv) {
                __builtin_trap();  // XNF doesn't preserve semantics
            }
            if (!FormulaChecker::is_xnf(xnf)) {
                __builtin_trap();  // Result is not in XNF
            }
        }
    }

    // Property 3: Simplify should preserve semantics
    if (FormulaChecker::get_variables(f).size() <= 4) {
        Formula* simp = f->simplify(pool);
        if (simp) {
            bool equiv = FormulaChecker::are_equivalent(pool, f, simp);
            if (!equiv) {
                __builtin_trap();  // Simplify doesn't preserve semantics
            }
        }
    }

    // Property 4: NNF twice = NNF once (idempotence)
    Formula* nnf1 = f->nnf(pool);
    if (nnf1) {
        Formula* nnf2 = nnf1->nnf(pool);
        if (nnf2) {
            bool equiv = FormulaChecker::are_equivalent(pool, nnf1, nnf2);
            if (!equiv) {
                __builtin_trap();  // NNF is not idempotent
            }
        }
    }

    // Property 5: Simplify twice = Simplify once (idempotence)
    Formula* simp1 = f->simplify(pool);
    if (simp1) {
        Formula* simp2 = simp1->simplify(pool);
        if (simp2) {
            bool equiv = FormulaChecker::are_equivalent(pool, simp1, simp2);
            if (!equiv) {
                __builtin_trap();  // Simplify is not idempotent
            }
        }
    }

    return 0;
}
