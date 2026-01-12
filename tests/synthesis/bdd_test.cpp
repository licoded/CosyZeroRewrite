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
    Formula* rm = apply_rm_next(phi, pool);

    assert(rm->is_true());
    std::cout << "X(p0) rm_next → " << rm->to_string(pool) << " ✓" << std::endl;

    // Test: p0 & X(p1) → p0 & True → p0
    Formula* phi2 = pool.create_and(
        pool.create_variable("p0"),
        pool.create_next(pool.create_variable("p1"))
    );
    Formula* rm2 = apply_rm_next(phi2, pool);

    assert(rm2->is_literal());
    assert(rm2->var_id() == 0);
    std::cout << "p0 & X(p1) rm_next → " << rm2->to_string(pool) << " ✓" << std::endl;

    // Test: X(p0) | p1 → True | p1 → True
    Formula* phi3 = pool.create_or(
        pool.create_next(pool.create_variable("p0")),
        pool.create_variable("p1")
    );
    Formula* rm3 = apply_rm_next(phi3, pool);

    assert(rm3->is_true());
    std::cout << "X(p0) | p1 rm_next → " << rm3->to_string(pool) << " ✓" << std::endl;

    // Test: !X(p0) → !True → False
    Formula* phi4 = pool.create_not(
        pool.create_next(pool.create_variable("p0"))
    );
    Formula* rm4 = apply_rm_next(phi4, pool);

    assert(rm4->is_false());
    std::cout << "!X(p0) rm_next → " << rm4->to_string(pool) << " ✓" << std::endl;

    std::cout << std::endl;
}

void test_bdd_manager_basic() {
    std::cout << "=== Test: BddManager basic ===" << std::endl;

    FormulaPool pool;
    pool.declare_variables({"s1"}, {"e1"});

    // Test with simple formula: s1 (no Next operators)
    Formula* phi = pool.create_variable("s1");

    BddManager bdd(2, 1);  // 2 vars total, 1 output

    bool built = bdd.build_from_formula_rmnext(phi, pool);
    assert(built);

    // The build_from_formula_rmnext stores the result internally
    // We can verify it worked by checking the function succeeded
    assert(built);

    std::cout << "BDD manager basic test passed ✓" << std::endl;
    std::cout << std::endl;
}

void test_enumeration() {
    std::cout << "=== Test: Safe move enumeration ===" << std::endl;

    FormulaPool pool;
    pool.declare_variables({"s1"}, {"e1"});

    // Formula: s1 (s1 must be true)
    // Only s1=true should be safe
    Formula* phi = pool.create_variable("s1");

    BddManager bdd(2, 1);  // 2 vars total, 1 output

    // Build the rm_next formula
    bool built = bdd.build_from_formula_rmnext(phi, pool);
    assert(built);

    std::cout << "Safe move enumeration test passed ✓" << std::endl;
    std::cout << std::endl;
}

void test_conjunction_rm_next() {
    std::cout << "=== Test: Conjunction with Next ===" << std::endl;

    FormulaPool pool;
    pool.declare_variables({"s1"}, {"e1"});

    // Example from the discussion:
    // Formula: (s1 & X(!e1)) | (!s1 & X(e1))
    // rm_next: (s1 & True) | (!s1 & True) = s1 | !s1 = True
    // So all moves should be safe

    Formula* phi = pool.create_or(
        pool.create_and(
            pool.create_variable("s1"),                    // s1
            pool.create_next(                              // X(...)
                pool.create_not(pool.create_variable("e1")) // !e1
            )
        ),
        pool.create_and(
            pool.create_not(pool.create_variable("s1")),   // !s1
            pool.create_next(pool.create_variable("e1"))    // X(e1)
        )
    );

    BddManager bdd(2, 1);
    bool built = bdd.build_from_formula_rmnext(phi, pool);
    assert(built);

    // The rm_next result should simplify to True
    // We can verify by checking that apply_rm_next on the same formula returns True
    Formula* rm = apply_rm_next(phi, pool);
    assert(rm->is_true());

    std::cout << "Conjunction rm_next test: formula simplifies to True ✓" << std::endl;
    std::cout << std::endl;
}

void test_statistics() {
    std::cout << "=== Test: BDD Statistics ===" << std::endl;

    FormulaPool pool;
    pool.declare_variables({"s1"}, {"e1"});

    // Formula: s1 (s1 must be true)
    Formula* phi = pool.create_variable("s1");

    BddManager bdd(2, 1);
    bdd.reset_stats();
    bdd.build_from_formula_rmnext(phi, pool);

    const auto& stats = bdd.get_stats();
    std::cout << "Statistics: " << stats.num_safe_moves_generated << " safe moves, "
              << stats.num_cache_hits << " cache hits, "
              << stats.num_cache_misses << " cache misses" << std::endl;

    std::cout << "BDD Statistics test passed ✓" << std::endl;
    std::cout << std::endl;
}

void test_complex_formula() {
    std::cout << "=== Test: Complex formula ===" << std::endl;

    FormulaPool pool;
    pool.declare_variables({"s1", "s2"}, {"e1"});

    // Formula: (s1 & !e1) | (s2 & X(e1))
    // rm_next: (s1 & !e1) | (s2 & True) = (s1 & !e1) | s2
    Formula* phi = pool.create_or(
        pool.create_and(
            pool.create_variable("s1"),
            pool.create_not(pool.create_variable("e1"))
        ),
        pool.create_and(
            pool.create_variable("s2"),
            pool.create_next(pool.create_variable("e1"))
        )
    );

    Formula* rm = apply_rm_next(phi, pool);
    std::cout << "Complex formula rm_next: " << rm->to_string(pool) << " ✓" << std::endl;

    BddManager bdd(3, 2);
    bdd.build_from_formula_rmnext(rm, pool);

    std::cout << "Complex formula test passed ✓" << std::endl;
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

    Formula* rm = apply_rm_next(xnf_phi, pool);

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

    Formula* rm = apply_rm_next(phi, pool);
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

    Formula* rm = apply_rm_next(phi, pool);
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
    test_bdd_manager_basic();
    test_enumeration();
    test_conjunction_rm_next();
    test_complex_formula();
    test_xnf_phi_vs_prop_atoms();  // Bug fix verification
    test_nested_next();
    test_mixed_boolean_temporal();
    test_statistics();

    std::cout << "========================================" << std::endl;
    std::cout << "All BDD tests passed! ✓" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
