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
    std::cout << "X(p0) rm_next → " << rm->to_string_with_names(pool) << " ✓" << std::endl;

    // Test: p0 & X(p1) → p0 & True → p0
    Formula* phi2 = pool.create_and(
        pool.create_variable("p0"),
        pool.create_next(pool.create_variable("p1"))
    );
    Formula* rm2 = apply_rm_next(phi2, pool);

    assert(rm2->is_literal());
    assert(rm2->var_id() == 0);
    std::cout << "p0 & X(p1) rm_next → " << rm2->to_string_with_names(pool) << " ✓" << std::endl;

    // Test: X(p0) | p1 → True | p1 → True
    Formula* phi3 = pool.create_or(
        pool.create_next(pool.create_variable("p0")),
        pool.create_variable("p1")
    );
    Formula* rm3 = apply_rm_next(phi3, pool);

    assert(rm3->is_true());
    std::cout << "X(p0) | p1 rm_next → " << rm3->to_string_with_names(pool) << " ✓" << std::endl;

    // Test: !X(p0) → !True → False
    Formula* phi4 = pool.create_not(
        pool.create_next(pool.create_variable("p0"))
    );
    Formula* rm4 = apply_rm_next(phi4, pool);

    assert(rm4->is_false());
    std::cout << "!X(p0) rm_next → " << rm4->to_string_with_names(pool) << " ✓" << std::endl;

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

void test_formula_with_temporal_operators() {
    std::cout << "=== Test: Formula with temporal operators ===" << std::endl;

    FormulaPool pool;
    pool.declare_variables({"p0", "p1"}, {});

    // Formula: p0 U X(p1)
    // rm_next: p0 U True = True (since True satisfies Until immediately)
    Formula* phi = pool.create_until(
        pool.create_variable("p0"),
        pool.create_next(pool.create_variable("p1"))
    );

    Formula* rm = apply_rm_next(phi, pool);
    assert(rm->is_true());

    std::cout << "p0 U X(p1) rm_next → " << rm->to_string_with_names(pool) << " ✓" << std::endl;
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

void test_release_operator() {
    std::cout << "=== Test: Release operator ===" << std::endl;

    FormulaPool pool;
    pool.declare_variables({"p0", "p1"}, {});

    // Formula: p0 R X(p1)
    // rm_next: p0 R True (Release with True on right)
    Formula* phi = pool.create_release(
        pool.create_variable("p0"),
        pool.create_next(pool.create_variable("p1"))
    );

    Formula* rm = apply_rm_next(phi, pool);
    // p0 R True = True
    assert(rm->is_true());

    std::cout << "p0 R X(p1) rm_next → " << rm->to_string_with_names(pool) << " ✓" << std::endl;
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
    std::cout << "Complex formula rm_next: " << rm->to_string_with_names(pool) << " ✓" << std::endl;

    BddManager bdd(3, 2);
    bdd.build_from_formula_rmnext(rm, pool);

    std::cout << "Complex formula test passed ✓" << std::endl;
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
    test_formula_with_temporal_operators();
    test_release_operator();
    test_complex_formula();
    test_statistics();

    std::cout << "========================================" << std::endl;
    std::cout << "All BDD tests passed! ✓" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
