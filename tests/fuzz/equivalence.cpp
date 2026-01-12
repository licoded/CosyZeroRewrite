#include "formula/formula.hpp"
#include "formula/formula_checker.hpp"
#include "formula/formula_parser.hpp"
#include "formula/formula_pool.hpp"

#include <cstddef>
#include <cstdint>

using namespace formula;

// Generate a formula from random bytes
Formula *generate_formula(FormulaPool &pool, const uint8_t *data, size_t size, size_t &pos, int depth = 0)
{
    if (pos >= size || depth > 5)
        return pool.create_true();

    uint8_t choice = data[pos++] % 15;

    switch (choice)
    {
        case 0:
            return pool.create_true();
        case 1:
            return pool.create_false();
        case 2:
        {
            int var_id = data[pos++] % 4; // Limit to 4 variables for exact checking
            return pool.create(Formula::OpType::Literal, nullptr, nullptr, var_id);
        }
        case 3:
        {
            Formula *left = generate_formula(pool, data, size, pos, depth + 1);
            return pool.create_not(left);
        }
        case 4:
        {
            Formula *left = generate_formula(pool, data, size, pos, depth + 1);
            Formula *right = generate_formula(pool, data, size, pos, depth + 1);
            return pool.create_and(left, right);
        }
        case 5:
        {
            Formula *left = generate_formula(pool, data, size, pos, depth + 1);
            Formula *right = generate_formula(pool, data, size, pos, depth + 1);
            return pool.create_or(left, right);
        }
        case 6:
        {
            Formula *left = generate_formula(pool, data, size, pos, depth + 1);
            return pool.create_next(left);
        }
        case 7:
        {
            Formula *left = generate_formula(pool, data, size, pos, depth + 1);
            Formula *right = generate_formula(pool, data, size, pos, depth + 1);
            return pool.create_until(left, right);
        }
        case 8:
        {
            Formula *left = generate_formula(pool, data, size, pos, depth + 1);
            Formula *right = generate_formula(pool, data, size, pos, depth + 1);
            return pool.create_release(left, right);
        }
        default:
            return pool.create_true();
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Split input into two parts for two formulas
    if (size < 8)
        return 0;
    if (size > 256)
        size = 256;

    FormulaPool pool;

    size_t pos1 = 0;
    Formula *f1 = generate_formula(pool, data, size / 2, pos1);

    size_t pos2 = size / 2;
    Formula *f2 = generate_formula(pool, data + size / 2, size - size / 2, pos2);

    if (!f1 || !f2)
        return 0;

    // Check equivalence using truth table method
    bool equiv1 = FormulaChecker::are_equivalent(pool, f1, f2);

    // For the same formula, should always be equivalent
    bool equiv2 = FormulaChecker::are_equivalent(pool, f1, f1);
    if (!equiv2)
    {
        __builtin_trap(); // Formula is not equivalent to itself!
    }

    bool equiv3 = FormulaChecker::are_equivalent(pool, f2, f2);
    if (!equiv3)
    {
        __builtin_trap(); // Formula is not equivalent to itself!
    }

    // Test symmetry: equiv(f1, f2) == equiv(f2, f1)
    bool equiv_swap = FormulaChecker::are_equivalent(pool, f2, f1);
    if (equiv1 != equiv_swap)
    {
        __builtin_trap(); // Equivalence is not symmetric
    }

    // Test transitivity with a third formula
    if (equiv1)
    {
        // f1 and f2 are equivalent, check some properties
        size_t size1 = FormulaChecker::formula_size(f1);
        size_t size2 = FormulaChecker::formula_size(f2);

        // Both should have same NNF results
        Formula *nnf1 = f1->nnf(pool);
        Formula *nnf2 = f2->nnf(pool);
        if (nnf1 && nnf2)
        {
            bool nnf_equiv = FormulaChecker::are_equivalent(pool, nnf1, nnf2);
            if (!nnf_equiv)
            {
                __builtin_trap(); // Equivalent formulas have non-equivalent NNFs
            }
        }

        // Both should have same XNF results
        Formula *xnf1 = f1->xnf_with_end_marker(pool);
        Formula *xnf2 = f2->xnf_with_end_marker(pool);
        if (xnf1 && xnf2)
        {
            bool xnf_equiv = FormulaChecker::are_equivalent(pool, xnf1, xnf2);
            if (!xnf_equiv)
            {
                __builtin_trap(); // Equivalent formulas have non-equivalent XNFs
            }
        }

        // Both should have same simplified results
        Formula *simp1 = f1->simplify(pool);
        Formula *simp2 = f2->simplify(pool);
        if (simp1 && simp2)
        {
            bool simp_equiv = FormulaChecker::are_equivalent(pool, simp1, simp2);
            if (!simp_equiv)
            {
                __builtin_trap(); // Equivalent formulas have non-equivalent simplifications
            }
        }
    }

    // Test reflexivity with negation
    Formula *not_f1 = pool.create_not(f1);
    Formula *not_not_f1 = pool.create_not(not_f1);

    // !!f should be equivalent to f (at least for formulas without temporal ops in the negation path)
    if (FormulaChecker::get_variables(f1).size() <= 4)
    {
        if (f1->is_literal() || f1->is_true() || f1->is_false())
        {
            bool not_equiv = FormulaChecker::are_equivalent(pool, f1, not_not_f1);
            if (!not_equiv)
            {
                // For literals, !!f should equal f
                __builtin_trap();
            }
        }
    }

    return 0;
}
