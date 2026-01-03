/**
 * Debug test for eventually_contradiction
 * Formula: F (p1 & !p1)
 * Expected: Unrealizable
 */

#include "synthesis/on_the_fly_solver.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula.hpp"
#include "log/logger.hpp"
#include <iostream>

using namespace formula;
using namespace synthesis;

int main() {
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});

    // F (p1 & !p1) = true U (p1 & !p1)
    Formula* p1 = pool.create_variable("p1");
    Formula* not_p1 = pool.create_not(p1);
    Formula* and_f = pool.create_and(p1, not_p1);
    Formula* true_f = pool.create_true();
    Formula* fand = pool.create_until(true_f, and_f);

    std::cout << "Formula: F (p1 & !p1) = true U (p1 & !p1)" << std::endl;
    std::cout << "Formula: " << fand->to_string() << std::endl;
    std::cout << "Expected: Unrealizable" << std::endl;

    // Enable debug logging on all sinks
    logger::Logger::instance().set_level(spdlog::level::debug);
    for (auto& sink : logger::Logger::instance().get()->sinks()) {
        sink->set_level(spdlog::level::debug);
    }

    OnTheFlyGameSolver solver(fand, pool, 1, 0);
    bool result = solver.is_realizable();

    std::cout << "\n=== Result ===" << std::endl;
    std::cout << "Realizable: " << (result ? "YES" : "NO") << std::endl;
    std::cout << "Expected: NO" << std::endl;
    std::cout << "Test: " << (result == false ? "PASS" : "FAIL") << std::endl;

    return 0;
}
