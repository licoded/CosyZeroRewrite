/**
 * Debug test for eventually_contradiction
 * Formula: F (p1 & !p1)
 * Expected: Unrealizable
 */

#include "synthesis/on_the_fly_solver.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula.hpp"
#include "log/logger.hpp"
#include "automata/tableau.hpp"
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

    // Create initial state to inspect
    auto init_state = automata::TableauState::initial(fand, pool);
    std::cout << "\n=== Initial State ===" << std::endl;
    std::cout << "phi_: " << (init_state->phi() ? init_state->phi()->to_string() : "null") << std::endl;
    std::cout << "xnf_phi_: " << (init_state->xnf_phi() ? init_state->xnf_phi()->to_string() : "null") << std::endl;
    std::cout << "prop_atoms_ size: " << init_state->prop_atoms().size() << std::endl;
    std::cout << "prop_atoms_: ";
    for (auto* f : init_state->prop_atoms()) {
        std::cout << (f ? f->to_string() : "null") << " ";
    }
    std::cout << std::endl;

    // Check empty-string acceptance
    static bool (*esa_func)(formula::Formula*) = nullptr;
    // We'll need to access the internal function or replicate logic

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
