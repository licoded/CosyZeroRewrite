/**
 * DFA Construction Tests
 *
 * Tests for LTLf to DFA conversion using tableau construction.
 */

#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "automata/dfa.hpp"
#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"

using namespace formula;
using namespace automata;

TEST_CASE("DFA: Simple literal formula", "[dfa]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    // Simple formula: p1
    Formula* f = parser.parse("p1");
    REQUIRE(f != nullptr);

    // Build DFA
    auto dfa = DFABuilder::build_from_formula(f, pool);
    REQUIRE(dfa != nullptr);
    REQUIRE(dfa->num_states() > 0);

    // Should have at least one state
    std::cout << "DFA for 'p1': " << dfa->num_states() << " states\n";
    dfa->print(pool);
}

TEST_CASE("DFA: Eventually formula", "[dfa]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    // F p1 (eventually p1) = true U p1
    Formula* f = parser.parse("true U p1");
    REQUIRE(f != nullptr);

    auto dfa = DFABuilder::build_from_formula(f, pool);
    REQUIRE(dfa != nullptr);

    std::cout << "DFA for 'true U p1': " << dfa->num_states() << " states\n";
    dfa->print(pool);

    // Check initial state exists
    StateId init = dfa->get_initial_state();
    REQUIRE(init != INVALID_STATE_ID);
}

TEST_CASE("DFA: Always formula", "[dfa]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    // G p1 (globally p1) = false R p1
    Formula* f = parser.parse("false R p1");
    REQUIRE(f != nullptr);

    auto dfa = DFABuilder::build_from_formula(f, pool);
    REQUIRE(dfa != nullptr);

    std::cout << "DFA for 'false R p1': " << dfa->num_states() << " states\n";
    dfa->print(pool);
}

TEST_CASE("DFA: Next formula", "[dfa]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    // X p1 (next p1)
    Formula* f = parser.parse("X(p1)");
    REQUIRE(f != nullptr);

    auto dfa = DFABuilder::build_from_formula(f, pool);
    REQUIRE(dfa != nullptr);

    std::cout << "DFA for 'X(p1)': " << dfa->num_states() << " states\n";
    dfa->print(pool);
}

TEST_CASE("DFA: Until formula", "[dfa]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    // p1 U p2
    Formula* f = parser.parse("p1 U p2");
    REQUIRE(f != nullptr);

    auto dfa = DFABuilder::build_from_formula(f, pool);
    REQUIRE(dfa != nullptr);

    std::cout << "DFA for 'p1 U p2': " << dfa->num_states() << " states\n";
    dfa->print(pool);
}

TEST_CASE("DFA: Conjunction", "[dfa]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    // p1 & p2
    Formula* f = parser.parse("p1 & p2");
    REQUIRE(f != nullptr);

    auto dfa = DFABuilder::build_from_formula(f, pool);
    REQUIRE(dfa != nullptr);

    std::cout << "DFA for 'p1 & p2': " << dfa->num_states() << " states\n";
    dfa->print(pool);
}

TEST_CASE("StateFormulaSet: Basic operations", "[dfa]") {
    FormulaPool pool;

    // Create a state with a few formulas
    StateFormulaSet state;

    // First declare variables
    pool.get_or_create_variable("p1");
    pool.get_or_create_variable("p2");

    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");
    Formula* not_p1 = pool.create_not(p1);

    state.add(p1);
    state.add(p2);
    state.add(not_p1);

    // Check contains
    REQUIRE(state.contains(p1));
    REQUIRE(state.contains(p2));
    REQUIRE(state.contains(not_p1));

    // Check size
    REQUIRE(state.size() == 3);

    // Check string output
    std::string s = state.to_string(pool);
    REQUIRE(!s.empty());
    std::cout << "State: " << s << "\n";
}

TEST_CASE("TransitionLabel: Hash and equality", "[dfa]") {
    TransitionLabel label1, label2;

    label1.add_positive(1);
    label1.add_negative(2);

    label2.add_positive(1);
    label2.add_negative(2);

    // Same labels should be equal
    REQUIRE(label1 == label2);

    // Same hash
    REQUIRE(label1.hash() == label2.hash());

    // Different label
    TransitionLabel label3;
    label3.add_positive(1);
    REQUIRE(!(label1 == label3));
}
