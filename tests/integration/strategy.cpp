/**
 * @file strategy_extraction_test.cpp
 * @brief Test strategy extraction from game solver
 */

#include "synthesis/on_the_fly_solver.hpp"
#include "synthesis/strategy.hpp"
#include "formula/formula_pool.hpp"
#include "log/logger.hpp"
#include <iostream>
#include <cassert>

using namespace synthesis;
using namespace formula;

void test_simple_strategy() {
    std::cout << "=== Test: Simple Strategy Extraction ===" << std::endl;

    FormulaPool pool;
    pool.declare_outputs({"p"});
    pool.declare_inputs({});

    // Simple formula: X(p) - "next p"
    // With p as output, this is realizable (system outputs true, then p becomes true)
    Formula* phi = pool.create_next(pool.create_variable("p"));

    OnTheFlyGameSolver solver(phi, pool, 1, 0);
    bool realizable = solver.is_realizable();

    std::cout << "Formula: " << phi->to_string() << std::endl;
    std::cout << "Realizable: " << (realizable ? "yes" : "no") << std::endl;

    assert(realizable);

    // Extract strategy
    auto strategy = Strategy::extract(solver);
    assert(strategy.has_value());

    std::cout << "Strategy extracted with " << strategy->size() << " states" << std::endl;

    // Export to JSON
    std::string json = strategy->to_json(pool);
    std::cout << "JSON output:\n" << json << std::endl;

    // Export to DOT
    std::string dot = strategy->to_dot(pool);
    std::cout << "DOT output:\n" << dot << std::endl;

    // Verify strategy
    bool verified = strategy->verify(phi, pool);
    std::cout << "Strategy verified: " << (verified ? "yes" : "no") << std::endl;

    assert(verified);

    std::cout << "PASSED" << std::endl << std::endl;
}

void test_with_inputs() {
    std::cout << "=== Test: Strategy with Inputs ===" << std::endl;

    FormulaPool pool;
    pool.declare_outputs({"p"});
    pool.declare_inputs({"q"});

    // Formula: X(p) - "next p"
    // System controls p, environment controls q
    // This is realizable since system can always output true
    Formula* phi = pool.create_next(pool.create_variable("p"));

    OnTheFlyGameSolver solver(phi, pool, 1, 1);
    bool realizable = solver.is_realizable();

    std::cout << "Formula: " << phi->to_string() << std::endl;
    std::cout << "Realizable: " << (realizable ? "yes" : "no") << std::endl;

    if (realizable) {
        auto strategy = Strategy::extract(solver);
        if (strategy) {
            std::cout << "Strategy extracted with " << strategy->size() << " states" << std::endl;

            // Get initial move
            auto initial_move = strategy->get_initial_move();
            if (initial_move) {
                std::cout << "Initial output: ";
                for (int v : initial_move->output) {
                    std::cout << pool.get_variable_name(v) << " ";
                }
                std::cout << std::endl;
            }
        }
    }

    std::cout << "PASSED" << std::endl << std::endl;
}

void test_unrealizable_formula() {
    std::cout << "=== Test: Unrealizable Formula ===" << std::endl;

    FormulaPool pool;
    pool.declare_outputs({"p"});
    pool.declare_inputs({"q"});

    // Formula: p & q - both must be true
    // But environment controls q, so system cannot guarantee this
    Formula* p = pool.create_variable("p");  // output
    Formula* q = pool.create_variable("q");  // input
    Formula* phi = pool.create_and(p, q);

    OnTheFlyGameSolver solver(phi, pool, 1, 1);
    bool realizable = solver.is_realizable();

    std::cout << "Formula: " << phi->to_string() << std::endl;
    std::cout << "Realizable: " << (realizable ? "yes" : "no") << std::endl;

    // Strategy extraction should fail
    auto strategy = Strategy::extract(solver);
    assert(!strategy.has_value());

    std::cout << "PASSED (strategy correctly not extracted)" << std::endl << std::endl;
}

int main() {
    test_simple_strategy();
    test_with_inputs();
    test_unrealizable_formula();

    std::cout << "===============================================================================" << std::endl;
    std::cout << "All strategy extraction tests passed!" << std::endl;
    return 0;
}
