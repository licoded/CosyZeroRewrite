/**
 * Debug failing on_the_fly_synthesis_tests
 */

#include "synthesis/on_the_fly_solver.hpp"
#include "formula/formula_pool.hpp"
#include "log/logger.hpp"
#include <iostream>

using namespace formula;
using namespace synthesis;

// Helper function to check realizability using OnTheFlyGameSolver
static bool check_realizable(Formula* phi, FormulaPool& pool) {
    OnTheFlyGameSolver solver(phi, pool);
    return solver.is_realizable();
}

int main() {
    logger::Logger::instance().set_level(spdlog::level::debug);

    // Test 1: X p1
    std::cout << "=== Test 1: X p1 ===" << std::endl;
    {
        FormulaPool pool;
        pool.declare_variables({"p1"}, {});
        Formula* p1 = pool.create_variable("p1");
        Formula* next_p1 = pool.create_next(p1);
        std::cout << "Formula: " << next_p1->to_string(pool) << std::endl;
        bool result = check_realizable(next_p1, pool);
        std::cout << "Result: " << (result ? "YES" : "NO") << std::endl;
        std::cout << "Expected: YES" << std::endl;
        std::cout << "Test: " << (result ? "PASS" : "FAIL") << std::endl;
    }

    // Test 2: G p1 (false R p1)
    std::cout << "\n=== Test 2: G p1 ===" << std::endl;
    {
        FormulaPool pool;
        pool.declare_variables({"p1"}, {});
        Formula* p1 = pool.create_variable("p1");
        Formula* false_f = pool.create_false();
        Formula* gp1 = pool.create_release(false_f, p1);
        std::cout << "Formula: " << gp1->to_string(pool) << std::endl;
        bool result = check_realizable(gp1, pool);
        std::cout << "Result: " << (result ? "YES" : "NO") << std::endl;
        std::cout << "Expected: YES" << std::endl;
        std::cout << "Test: " << (result ? "PASS" : "FAIL") << std::endl;
    }

    // Test 3: p1 & !p1
    std::cout << "\n=== Test 3: p1 & !p1 ===" << std::endl;
    {
        FormulaPool pool;
        pool.declare_variables({"p1"}, {});
        Formula* p1 = pool.create_variable("p1");
        Formula* not_p1 = pool.create_not(p1);
        Formula* and_f = pool.create_and(p1, not_p1);
        std::cout << "Formula: " << and_f->to_string(pool) << std::endl;
        std::cout << "  p1 op=" << (int)p1->op() << ", var_id=" << p1->var_id() << std::endl;
        std::cout << "  not_p1 op=" << (int)not_p1->op() << std::endl;
        if (not_p1->op() == formula::Formula::OpType::Not && not_p1->left()) {
            std::cout << "  not_p1->left op=" << (int)not_p1->left()->op() << ", var_id=" << not_p1->left()->var_id() << std::endl;
        }
        std::cout << "  and_f left op=" << (int)and_f->left()->op() << ", var_id=" << and_f->left()->var_id() << std::endl;
        std::cout << "  and_f right op=" << (int)and_f->right()->op() << std::endl;
        if (and_f->right()->op() == formula::Formula::OpType::Not && and_f->right()->left()) {
            std::cout << "  and_f->right->left op=" << (int)and_f->right()->left()->op() << ", var_id=" << and_f->right()->left()->var_id() << std::endl;
        }
        bool result = check_realizable(and_f, pool);
        std::cout << "Result: " << (result ? "YES" : "NO") << std::endl;
        std::cout << "Expected: NO" << std::endl;
        std::cout << "Test: " << (!result ? "PASS" : "FAIL") << std::endl;
    }

    // Test 4: F (p1 & !p1)
    std::cout << "\n=== Test 4: F (p1 & !p1) ===" << std::endl;
    {
        FormulaPool pool;
        pool.declare_variables({"p1"}, {});
        Formula* p1 = pool.create_variable("p1");
        Formula* not_p1 = pool.create_not(p1);
        Formula* and_f = pool.create_and(p1, not_p1);
        Formula* true_f = pool.create_true();
        Formula* fand = pool.create_until(true_f, and_f);
        std::cout << "Formula: " << fand->to_string(pool) << std::endl;
        bool result = check_realizable(fand, pool);
        std::cout << "Result: " << (result ? "YES" : "NO") << std::endl;
        std::cout << "Expected: NO" << std::endl;
        std::cout << "Test: " << (!result ? "PASS" : "FAIL") << std::endl;
    }

    return 0;
}
