/**
 * @file bdd_test.cpp
 * @brief Tests for BDD Manager and Safe System Move optimization (Rule B)
 */

#include "synthesis/bdd_manager.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"
#include <iostream>
#include <cassert>
#include <set>

using namespace synthesis;
using namespace formula;

void test_rm_next_basic() {
    std::cout << "=== Test: rm_next basic ===" << std::endl;

    FormulaPool pool;
    pool.declare_variables({"p0", "p1"}, {});

    // Test: X(p0) → True
    Formula* phi = pool.create_next(pool.create_variable("p0"));
    Formula* rm = phi->replaceNext2True(pool);

    assert(rm->is_true());
    std::cout << "X(p0) rm_next → " << rm->to_string(pool) << " ✓" << std::endl;

    // Test: p0 & X(p1) → p0 & True → p0
    Formula* phi2 = pool.create_and(
        pool.create_variable("p0"),
        pool.create_next(pool.create_variable("p1"))
    );
    Formula* rm2 = phi2->replaceNext2True(pool);

    assert(rm2->is_literal());
    assert(rm2->var_id() == 0);
    std::cout << "p0 & X(p1) rm_next → " << rm2->to_string(pool) << " ✓" << std::endl;

    // Test: X(p0) | p1 → True | p1 → True
    Formula* phi3 = pool.create_or(
        pool.create_next(pool.create_variable("p0")),
        pool.create_variable("p1")
    );
    Formula* rm3 = phi3->replaceNext2True(pool);

    assert(rm3->is_true());
    std::cout << "X(p0) | p1 rm_next → " << rm3->to_string(pool) << " ✓" << std::endl;

    // Test: !X(p0) → !True → False
    Formula* phi4 = pool.create_not(
        pool.create_next(pool.create_variable("p0"))
    );
    Formula* rm4 = phi4->replaceNext2True(pool);

    assert(rm4->is_false());
    std::cout << "!X(p0) rm_next → " << rm4->to_string(pool) << " ✓" << std::endl;

    std::cout << std::endl;
}

void test_xnf_phi_vs_prop_atoms() {
    std::cout << "=== Test: xnf_phi vs prop_atoms (bug fix verification) ===" << std::endl;

    FormulaPool pool;
    pool.declare_variables({"p0", "p1", "p2"}, {});

    // This test verifies the fix for the bug where prop_atoms was used instead of xnf_phi
    // Example: xnf_phi = (p0 & X(p1)) | p2
    // prop_atoms (flattened) = {p0, X(p1), p2}
    // Old bug: skipping Next gives p0 & p2 (WRONG - lost the OR structure)
    // Correct: (p0 & True) | p2 = p0 | p2

    Formula* xnf_phi = pool.create_or(
        pool.create_and(
            pool.create_variable("p0"),
            pool.create_next(pool.create_variable("p1"))
        ),
        pool.create_variable("p2")
    );

    Formula* rm = xnf_phi->replaceNext2True(pool);

    // Result should be p0 | p2 (or p2 | p0 depending on simplification)
    // Key point: it should be an OR, not AND
    assert(rm->op() == Formula::OpType::Or);

    // Check that both p0 and p2 are in the result (as literals or subformulas)
    bool has_p0 = false;
    bool has_p2 = false;

    if (rm->left()->is_literal() && rm->left()->var_id() == 0) has_p0 = true;
    if (rm->right()->is_literal() && rm->right()->var_id() == 2) has_p2 = true;
    // Also check the other side of OR
    if (rm->left()->is_literal() && rm->left()->var_id() == 2) has_p2 = true;
    if (rm->right()->is_literal() && rm->right()->var_id() == 0) has_p0 = true;

    assert(has_p0 && has_p2);

    std::cout << "(p0 & X(p1)) | p2 rm_next → " << rm->to_string(pool) << " ✓" << std::endl;
    std::cout << "Correctly preserved OR structure (not flattened to AND) ✓" << std::endl;
    std::cout << std::endl;
}

void test_nested_next() {
    std::cout << "=== Test: Nested Next operators ===" << std::endl;

    FormulaPool pool;
    pool.declare_variables({"p0"}, {});

    // X(X(p0)) → True → True
    Formula* phi = pool.create_next(
        pool.create_next(pool.create_variable("p0"))
    );

    Formula* rm = phi->replaceNext2True(pool);
    assert(rm->is_true());

    std::cout << "X(X(p0)) rm_next → " << rm->to_string(pool) << " ✓" << std::endl;
    std::cout << std::endl;
}

void test_mixed_boolean_temporal() {
    std::cout << "=== Test: Mixed boolean and temporal ===" << std::endl;

    FormulaPool pool;
    pool.declare_variables({"a", "b", "c"}, {});

    // (a | X(b)) & (X(c) | !b)
    // rm_next: (a | True) & (True | !b) = True & True = True
    Formula* phi = pool.create_and(
        pool.create_or(
            pool.create_variable("a"),
            pool.create_next(pool.create_variable("b"))
        ),
        pool.create_or(
            pool.create_next(pool.create_variable("c")),
            pool.create_not(pool.create_variable("b"))
        )
    );

    Formula* rm = phi->replaceNext2True(pool);
    assert(rm->is_true());

    std::cout << "(a | X(b)) & (X(c) | !b) rm_next → " << rm->to_string(pool) << " ✓" << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "BDD Manager Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    test_rm_next_basic();
    test_xnf_phi_vs_prop_atoms();  // Bug fix verification
    test_nested_next();
    test_mixed_boolean_temporal();

    std::cout << "========================================" << std::endl;
    std::cout << "All BDD tests passed! ✓" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
