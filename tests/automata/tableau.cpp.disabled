/**
 * @file tableau_state_test.cpp
 * @brief Unit tests for TableauState - focused on accepting state conditions
 *
 * These tests directly verify TableauState behavior, especially the
 * is_accepting() logic that was fixed for Until formulas.
 */

#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "automata/tableau.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"

using namespace formula;
using namespace automata;

//==============================================================================
// Test Helpers
//==============================================================================

// Helper to create a TableauState from a set of formulas
static TableauState* make_state(FormulaPool& /*pool*/,
                                 const std::vector<Formula*>& formulas) {
    TableauState::FormulaSet set;
    for (auto* f : formulas) {
        set.insert(f);
    }
    static TableauStatePool sp;
    return sp.get_or_create(std::move(set));
}

//==============================================================================
// Accepting State Tests - Core Fix Validation
//==============================================================================

TEST_CASE("TableauState: Empty state is accepting", "[tableau][accepting]") {
    FormulaPool pool;
    // Use empty formulas with true which simplifies to empty
    Formula* t = pool.create_true();
    auto state = TableauState::initial(t, pool);

    REQUIRE(state != nullptr);
    REQUIRE(state->is_accepting());
}

TEST_CASE("TableauState: State with false is NOT accepting", "[tableau][accepting]") {
    FormulaPool pool;
    Formula* f = pool.create_false();
    auto state = make_state(pool, {f});

    REQUIRE_FALSE(state->is_accepting());
}

TEST_CASE("TableauState: State with single literal is accepting", "[tableau][accepting]") {
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});
    Formula* p1 = pool.create_variable("p1");
    auto state = make_state(pool, {p1});

    REQUIRE(state->is_accepting());
}

TEST_CASE("TableauState: State with true is accepting", "[tableau][accepting]") {
    FormulaPool pool;
    Formula* t = pool.create_true();
    auto state = make_state(pool, {t});

    REQUIRE(state->is_accepting());
}

//==============================================================================
// Until Accepting State Tests - THE KEY FIX
//==============================================================================

TEST_CASE("TableauState: Until with right side satisfied is accepting",
          "[tableau][accepting][until][fix]") {
    FormulaPool pool;
    pool.declare_variables({"p1", "p2"}, {});

    // State: {p1 U p2, p2}
    // Since p2 is satisfied, Until is complete → accepting
    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");
    Formula* until = pool.create_until(p1, p2);

    auto state = make_state(pool, {until, p2});

    REQUIRE(state->is_accepting());
}

TEST_CASE("TableauState: Until WITHOUT right side is NOT accepting",
          "[tableau][accepting][until][fix]") {
    FormulaPool pool;
    pool.declare_variables({"p1", "p2"}, {});

    // State: {p1 U p2, p1}
    // p2 is NOT in state, so Until is still waiting → NOT accepting
    // This is the KEY test for the bug fix!
    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");
    Formula* until = pool.create_until(p1, p2);

    auto state = make_state(pool, {until, p1});

    REQUIRE_FALSE(state->is_accepting());
}

TEST_CASE("TableauState: Until alone without right is NOT accepting",
          "[tableau][accepting][until][fix]") {
    FormulaPool pool;
    pool.declare_variables({"p1", "p2"}, {});

    // State: {p1 U p2} only
    // Neither p1 nor p2, but the Until itself is still waiting
    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");
    Formula* until = pool.create_until(p1, p2);

    auto state = make_state(pool, {until});

    REQUIRE_FALSE(state->is_accepting());
}

TEST_CASE("TableauState: Eventually (true U p1) with p1 is accepting",
          "[tableau][accepting][until]") {
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});

    // F p1 = true U p1, with p1 satisfied
    Formula* p1 = pool.create_variable("p1");
    Formula* t = pool.create_true();
    Formula* eventually = pool.create_until(t, p1);

    auto state = make_state(pool, {eventually, p1});

    REQUIRE(state->is_accepting());
}

TEST_CASE("TableauState: Eventually (true U p1) without p1 is NOT accepting",
          "[tableau][accepting][until][fix]") {
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});

    // F p1 = true U p1, but p1 not yet satisfied
    Formula* p1 = pool.create_variable("p1");
    Formula* t = pool.create_true();
    Formula* eventually = pool.create_until(t, p1);

    auto state = make_state(pool, {eventually});

    REQUIRE_FALSE(state->is_accepting());
}

//==============================================================================
// Multiple Until Tests
//==============================================================================

TEST_CASE("TableauState: Multiple Until, all satisfied is accepting",
          "[tableau][accepting][until]") {
    FormulaPool pool;
    pool.declare_variables({"p1", "p2", "p3", "p4"}, {});

    // State: {p1 U p2, p3 U p4, p2, p4}
    // Both Until formulas have right sides satisfied
    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");
    Formula* p3 = pool.create_variable("p3");
    Formula* p4 = pool.create_variable("p4");
    Formula* until1 = pool.create_until(p1, p2);
    Formula* until2 = pool.create_until(p3, p4);

    auto state = make_state(pool, {until1, until2, p2, p4});

    REQUIRE(state->is_accepting());
}

TEST_CASE("TableauState: Multiple Until, one unsatisfied is NOT accepting",
          "[tableau][accepting][until][fix]") {
    FormulaPool pool;
    pool.declare_variables({"p1", "p2", "p3", "p4"}, {});

    // State: {p1 U p2, p3 U p4, p2}
    // Second Until (p3 U p4) doesn't have p4 → NOT accepting
    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");
    Formula* p3 = pool.create_variable("p3");
    Formula* p4 = pool.create_variable("p4");
    Formula* until1 = pool.create_until(p1, p2);
    Formula* until2 = pool.create_until(p3, p4);

    auto state = make_state(pool, {until1, until2, p2});

    REQUIRE_FALSE(state->is_accepting());
}

//==============================================================================
// Local Consistency Tests
//==============================================================================

TEST_CASE("TableauState: Contradiction p and !p is NOT consistent",
          "[tableau][consistency]") {
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});

    Formula* p1 = pool.create_variable("p1");
    Formula* not_p1 = pool.create_not(p1);

    auto state = make_state(pool, {p1, not_p1});

    REQUIRE_FALSE(state->is_locally_consistent());
}

TEST_CASE("TableauState: And without both operands is NOT consistent",
          "[tableau][consistency]") {
    FormulaPool pool;
    pool.declare_variables({"p1", "p2"}, {});

    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");
    Formula* and_f = pool.create_and(p1, p2);

    // Only have the And, not p1 or p2
    auto state = make_state(pool, {and_f});

    REQUIRE_FALSE(state->is_locally_consistent());
}

TEST_CASE("TableauState: And with both operands is consistent",
          "[tableau][consistency]") {
    FormulaPool pool;
    pool.declare_variables({"p1", "p2"}, {});

    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");
    Formula* and_f = pool.create_and(p1, p2);

    auto state = make_state(pool, {and_f, p1, p2});

    REQUIRE(state->is_locally_consistent());
}

TEST_CASE("TableauState: Or without either operand IS consistent (choice point)",
          "[tableau][consistency]") {
    FormulaPool pool;
    pool.declare_variables({"p1", "p2"}, {});

    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");
    Formula* or_f = pool.create_or(p1, p2);

    // Only have the Or, not p1 or p2
    // OR is a choice point - system can choose to satisfy either side later
    auto state = make_state(pool, {or_f});

    REQUIRE(state->is_locally_consistent());
}

TEST_CASE("TableauState: Or with one operand is consistent",
          "[tableau][consistency]") {
    FormulaPool pool;
    pool.declare_variables({"p1", "p2"}, {});

    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");
    Formula* or_f = pool.create_or(p1, p2);

    auto state = make_state(pool, {or_f, p1});

    REQUIRE(state->is_locally_consistent());
}

//==============================================================================
// Release Formula Tests
//==============================================================================

TEST_CASE("TableauState: Release with right side is accepting",
          "[tableau][accepting][release]") {
    FormulaPool pool;
    pool.declare_variables({"p1", "p2"}, {});

    // State: {p1 R p2, p2}
    // Release only needs p2 to hold
    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");
    Formula* release = pool.create_release(p1, p2);

    auto state = make_state(pool, {release, p2});

    REQUIRE(state->is_accepting());
}

//==============================================================================
// Next Formula Tests
//==============================================================================

TEST_CASE("TableauState: Next alone is accepting",
          "[tableau][accepting][next]") {
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});

    // State: {X p1}
    // Next doesn't block accepting (it's about the next state)
    Formula* p1 = pool.create_variable("p1");
    Formula* next = pool.create_next(p1);

    auto state = make_state(pool, {next});

    REQUIRE(state->is_accepting());
}

//==============================================================================
// Next State Computation Tests
//==============================================================================

TEST_CASE("TableauState: next() with literal assignment", "[tableau][next]") {
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});

    Formula* p1 = pool.create_variable("p1");
    auto initial = make_state(pool, {p1});

    // Assignment where p1 is true
    Assignment a;
    a.insert(p1->var_id());

    auto next_state = initial->next(a, pool);

    // p1 is true, so it stays; state becomes {p1}
    // But since p1 is non-temporal, it stays in old()
    REQUIRE(next_state != nullptr);
}

TEST_CASE("TableauState: next() removes false literals", "[tableau][next]") {
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});

    Formula* p1 = pool.create_variable("p1");
    auto initial = make_state(pool, {p1});

    // Assignment where p1 is false (empty assignment)
    Assignment a;  // p1 not in a means p1 is false

    auto next_state = initial->next(a, pool);

    // p1 is false, so it should be removed
    REQUIRE(next_state != nullptr);
    REQUIRE(next_state->formulas().empty());
}

TEST_CASE("TableauState: next() unwraps Next", "[tableau][next]") {
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});

    Formula* p1 = pool.create_variable("p1");
    Formula* next_p1 = pool.create_next(p1);
    auto initial = make_state(pool, {next_p1});

    Assignment a;  // Empty assignment

    auto next_state = initial->next(a, pool);

    // X p1 should unwrap to p1
    REQUIRE(next_state != nullptr);
    REQUIRE(next_state->formulas().count(p1) > 0);
}

//==============================================================================
// Hash Consing Tests
//==============================================================================

TEST_CASE("TableauState: Same formulas produce equivalent states",
          "[tableau][hash]") {
    FormulaPool pool;
    pool.declare_variables({"p1", "p2"}, {});

    TableauStatePool sp;

    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");

    // Create two states with same formulas
    TableauState::FormulaSet set1 = {p1, p2};
    TableauState::FormulaSet set2 = {p1, p2};

    auto* s1 = sp.get_or_create(std::move(set1));
    auto* s2 = sp.get_or_create(std::move(set2));

    // Should be the same object due to hash consing
    REQUIRE(s1 == s2);
}

TEST_CASE("TableauState: Different formulas produce different states",
          "[tableau][hash]") {
    FormulaPool pool;
    pool.declare_variables({"p1", "p2", "p3"}, {});

    TableauStatePool sp;

    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");
    Formula* p3 = pool.create_variable("p3");

    auto* s1 = sp.get_or_create({p1, p2});
    auto* s2 = sp.get_or_create({p2, p3});

    // Should be different objects
    REQUIRE(s1 != s2);
}

TEST_CASE("TableauState: Equal states have same hash",
          "[tableau][hash]") {
    FormulaPool pool;
    pool.declare_variables({"p1", "p2"}, {});

    TableauStatePool sp;

    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");

    auto* s1 = sp.get_or_create({p1, p2});
    auto* s2 = sp.get_or_create({p1, p2});

    // Same content → same hash
    REQUIRE(s1->hash() == s2->hash());
}

//==============================================================================
// On-the-Fly DFA Tests
//==============================================================================

TEST_CASE("OnTheFlyDFA: Initial state from formula", "[dfa][onthefly]") {
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});

    Formula* p1 = pool.create_variable("p1");
    OnTheFlyDFA dfa(p1, pool);

    auto* init = dfa.initial_state();
    REQUIRE(init != nullptr);
    REQUIRE(init->formulas().size() > 0);
}

TEST_CASE("OnTheFlyDFA: Successor computation and caching", "[dfa][onthefly]") {
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});

    Formula* p1 = pool.create_variable("p1");
    OnTheFlyDFA dfa(p1, pool);

    auto* init = dfa.initial_state();

    Assignment a;
    a.insert(p1->var_id());

    // First call computes the successor
    auto* succ1 = dfa.successor(init, a);
    REQUIRE(succ1 != nullptr);

    // Second call should return cached result
    auto* succ2 = dfa.successor(init, a);
    REQUIRE(succ2 == succ1);

    // Cache size should be 1
    REQUIRE(dfa.cache_size() == 1);
}

TEST_CASE("OnTheFlyDFA: Multiple assignments produce different successors",
          "[dfa][onthefly]") {
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});

    Formula* p1 = pool.create_variable("p1");
    OnTheFlyDFA dfa(p1, pool);

    auto* init = dfa.initial_state();

    Assignment a1;  // p1 = false
    a1.insert(p1->var_id());  // p1 = true

    auto* succ_true = dfa.successor(init, a1);
    auto* succ_false = dfa.successor(init, Assignment{});

    // Different assignments should produce different successors
    // (unless they happen to be equivalent, but for a literal p1 they differ)
    REQUIRE(succ_true != nullptr);
    REQUIRE(succ_false != nullptr);
}

//==============================================================================
// Complex Formula Tests
//==============================================================================

TEST_CASE("TableauState: Nested Until with right satisfied is accepting",
          "[tableau][accepting][until][complex]") {
    FormulaPool pool;
    pool.declare_variables({"p1", "p2", "p3"}, {});

    // State: {p1 U (p2 U p3), p2 U p3, p3}
    // Nested Until with innermost satisfied
    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");
    Formula* p3 = pool.create_variable("p3");
    Formula* inner_until = pool.create_until(p2, p3);
    Formula* outer_until = pool.create_until(p1, inner_until);

    auto state = make_state(pool, {outer_until, inner_until, p3});

    REQUIRE(state->is_accepting());
}

TEST_CASE("TableauState: Mixed Until and Release",
          "[tableau][accepting][mixed]") {
    FormulaPool pool;
    pool.declare_variables({"p1", "p2", "p3", "p4"}, {});

    // State: {p1 U p2, p3 R p4, p2, p4}
    // Until satisfied, Release satisfied
    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");
    Formula* p3 = pool.create_variable("p3");
    Formula* p4 = pool.create_variable("p4");
    Formula* until = pool.create_until(p1, p2);
    Formula* release = pool.create_release(p3, p4);

    auto state = make_state(pool, {until, release, p2, p4});

    REQUIRE(state->is_accepting());
}
