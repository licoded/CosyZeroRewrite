/**
 * @file tarjan_scc_tests.cpp
 * @brief Unit tests for Tarjan SCC algorithm
 *
 * Tests the find_sccs() implementation in OnTheFlyGameSolver
 * with various graph structures.
 */

#define CATCH_CONFIG_RUNNER
#include "synthesis/on_the_fly_solver.hpp"
#include "formula/formula_pool.hpp"
#include "catch.hpp"

using namespace synthesis;
using namespace formula;
using namespace automata;

namespace {

// Test helper class for creating mock game states
struct MockDFAState {
    int id;
    size_t hash_val;

    MockDFAState(int i) : id(i), hash_val(i) {}

    // Required for GameState hash
    size_t hash() const { return hash_val; }

    // Equality comparison
    bool operator==(const MockDFAState& other) const {
        return id == other.id;
    }
};

// Hash function for MockDFAState
struct MockDFAStateHash {
    size_t operator()(const MockDFAState& s) const {
        return s.hash();
    }
};

// Pool of mock states
static std::vector<MockDFAState> mock_states;

// Helper to create a test game state
GameState make_state(int dfa_id, Player player, const std::optional<Assignment>& out = std::nullopt) {
    // Ensure we have enough mock states
    while (dfa_id >= static_cast<int>(mock_states.size())) {
        mock_states.push_back(MockDFAState(mock_states.size()));
    }
    // Create a pointer wrapper (using reinterpret_cast for test purposes)
    auto* ptr = reinterpret_cast<TableauState*>(&mock_states[dfa_id]);
    return GameState(ptr, player, out);
}

// Helper to count SCCs of each size
std::map<size_t, size_t> count_scc_sizes(const std::vector<std::vector<GameState>>& sccs) {
    std::map<size_t, size_t> result;
    for (const auto& scc : sccs) {
        result[scc.size()]++;
    }
    return result;
}

// Helper to check if two states are in the same SCC
bool same_scc(const GameState& a, const GameState& b, const std::vector<std::vector<GameState>>& sccs) {
    for (const auto& scc : sccs) {
        bool has_a = std::find(scc.begin(), scc.end(), a) != scc.end();
        bool has_b = std::find(scc.begin(), scc.end(), b) != scc.end();
        if (has_a && has_b) return true;
    }
    return false;
}

} // namespace

//==============================================================================
// Test Case 1: Single Node
//==============================================================================
TEST_CASE("Tarjan SCC: Single Node", "[tarjan][scc][basic]") {
    // Graph: a
    // Expected: 1 SCC of size 1

    FormulaPool pool;
    OnTheFlyGameSolver solver(pool.create_true(), pool, 0, 0);

    // Manually build a simple graph with one state
    auto s0 = make_state(0, Player::System);
    solver.add_test_transition(s0, {s0});  // Self-loop

    auto sccs = solver.find_sccs_for_testing();

    REQUIRE(sccs.size() == 1);
    REQUIRE(sccs[0].size() == 1);
    REQUIRE(sccs[0][0] == s0);
}

//==============================================================================
// Test Case 2: Linear Chain
//==============================================================================
TEST_CASE("Tarjan SCC: Linear Chain", "[tarjan][scc][basic]") {
    // Graph: a → b → c → d
    // Expected: 4 SCCs of size 1 (each node is its own SCC)

    FormulaPool pool;
    OnTheFlyGameSolver solver(pool.create_true(), pool, 0, 0);

    auto s0 = make_state(0, Player::System);
    auto s1 = make_state(1, Player::Environment);
    auto s2 = make_state(2, Player::System);
    auto s3 = make_state(3, Player::Environment);

    solver.add_test_transition(s0, {s1});
    solver.add_test_transition(s1, {s2});
    solver.add_test_transition(s2, {s3});
    solver.add_test_transition(s3, {});  // Terminal

    auto sccs = solver.find_sccs_for_testing();

    REQUIRE(sccs.size() == 4);

    auto sizes = count_scc_sizes(sccs);
    REQUIRE(sizes[1] == 4);  // 4 SCCs of size 1
}

//==============================================================================
// Test Case 3: Simple Cycle
//==============================================================================
TEST_CASE("Tarjan SCC: Simple Cycle", "[tarjan][scc][cycle]") {
    // Graph: a → b → c → a
    // Expected: 1 SCC of size 3

    FormulaPool pool;
    OnTheFlyGameSolver solver(pool.create_true(), pool, 0, 0);

    auto s0 = make_state(0, Player::System);
    auto s1 = make_state(1, Player::Environment);
    auto s2 = make_state(2, Player::System);

    solver.add_test_transition(s0, {s1});
    solver.add_test_transition(s1, {s2});
    solver.add_test_transition(s2, {s0});

    auto sccs = solver.find_sccs_for_testing();

    REQUIRE(sccs.size() == 1);
    REQUIRE(sccs[0].size() == 3);

    // All three states should be in the same SCC
    REQUIRE(same_scc(s0, s1, sccs));
    REQUIRE(same_scc(s1, s2, sccs));
    REQUIRE(same_scc(s0, s2, sccs));
}

//==============================================================================
// Test Case 4: Two Connected Cycles
//==============================================================================
TEST_CASE("Tarjan SCC: Two Connected Cycles", "[tarjan][scc][cycle]") {
    // Graph: a ↔ b, c ↔ d, b → c
    //        a→b→a and c→d→c are separate SCCs
    // Expected: 2 SCCs of size 2

    FormulaPool pool;
    OnTheFlyGameSolver solver(pool.create_true(), pool, 0, 0);

    auto s0 = make_state(0, Player::System);    // a
    auto s1 = make_state(1, Player::Environment); // b
    auto s2 = make_state(2, Player::System);    // c
    auto s3 = make_state(3, Player::Environment); // d

    solver.add_test_transition(s0, {s1});  // a → b
    solver.add_test_transition(s1, {s0});  // b → a (cycle 1)
    solver.add_test_transition(s1, {s2});  // b → c (connecting edge)
    solver.add_test_transition(s2, {s3});  // c → d
    solver.add_test_transition(s3, {s2});  // d → c (cycle 2)

    auto sccs = solver.find_sccs_for_testing();

    REQUIRE(sccs.size() == 2);

    auto sizes = count_scc_sizes(sccs);
    REQUIRE(sizes[2] == 2);  // 2 SCCs of size 2

    // a and b should be in the same SCC
    REQUIRE(same_scc(s0, s1, sccs));

    // c and d should be in the same SCC
    REQUIRE(same_scc(s2, s3, sccs));

    // {a, b} and {c, d} should be different SCCs
    REQUIRE(!same_scc(s0, s2, sccs));
}

//==============================================================================
// Test Case 5: Self-Loop
//==============================================================================
TEST_CASE("Tarjan SCC: Self-Loop", "[tarjan][scc][basic]") {
    // Graph: a → a
    // Expected: 1 SCC of size 1

    FormulaPool pool;
    OnTheFlyGameSolver solver(pool.create_true(), pool, 0, 0);

    auto s0 = make_state(0, Player::System);
    solver.add_test_transition(s0, {s0});

    auto sccs = solver.find_sccs_for_testing();

    REQUIRE(sccs.size() == 1);
    REQUIRE(sccs[0].size() == 1);
}

//==============================================================================
// Test Case 6: Complex DAG
//==============================================================================
TEST_CASE("Tarjan SCC: Complex DAG", "[tarjan][scc][dag]") {
    // Graph:     e
    //          / | \
    //         d  b  c
    //         |  |  |
    //         a  a  a
    // Expected: 5 SCCs of size 1 (no cycles)

    FormulaPool pool;
    OnTheFlyGameSolver solver(pool.create_true(), pool, 0, 0);

    auto sa = make_state(0, Player::System);
    auto sb = make_state(1, Player::Environment);
    auto sc = make_state(2, Player::System);
    auto sd = make_state(3, Player::Environment);
    auto se = make_state(4, Player::System);

    solver.add_test_transition(sa, {});
    solver.add_test_transition(sb, {sa});
    solver.add_test_transition(sc, {sa});
    solver.add_test_transition(sd, {sa});
    solver.add_test_transition(se, {sb, sc, sd});

    auto sccs = solver.find_sccs_for_testing();

    REQUIRE(sccs.size() == 5);

    auto sizes = count_scc_sizes(sccs);
    REQUIRE(sizes[1] == 5);  // 5 SCCs of size 1
}

//==============================================================================
// Test Case 7: Cross Pattern
//==============================================================================
TEST_CASE("Tarjan SCC: Cross Pattern", "[tarjan][scc][dag]") {
    // Graph: a → b
    //        c → d
    //        a → c
    //        b → d
    // Expected: 4 SCCs of size 1

    FormulaPool pool;
    OnTheFlyGameSolver solver(pool.create_true(), pool, 0, 0);

    auto sa = make_state(0, Player::System);
    auto sb = make_state(1, Player::Environment);
    auto sc = make_state(2, Player::System);
    auto sd = make_state(3, Player::Environment);

    solver.add_test_transition(sa, {sb, sc});
    solver.add_test_transition(sc, {sd});
    solver.add_test_transition(sb, {sd});

    auto sccs = solver.find_sccs_for_testing();

    REQUIRE(sccs.size() == 4);

    auto sizes = count_scc_sizes(sccs);
    REQUIRE(sizes[1] == 4);  // 4 SCCs of size 1
}

//==============================================================================
// Test Case 8: Diamond with Cycle Inside
//==============================================================================
TEST_CASE("Tarjan SCC: Diamond with Cycle Inside", "[tarjan][scc][complex]") {
    // Graph: a → b → d
    //        |    ↑
    //        v    |
    //        c ←──+
    //
    // Edge structure:
    //   sa → {sb, sc}
    //   sb → {sd}
    //   sc → {sd}
    //   sd → {sb}  (creates cycle: sb ↔ sd)
    //
    // Actual SCCs: {sb, sd} (2-node cycle), {sc}, {sa}
    // Total: 3 SCCs

    FormulaPool pool;
    OnTheFlyGameSolver solver(pool.create_true(), pool, 0, 0);

    auto sa = make_state(0, Player::System);
    auto sb = make_state(1, Player::Environment);
    auto sc = make_state(2, Player::System);
    auto sd = make_state(3, Player::Environment);

    solver.add_test_transition(sa, {sb, sc});
    solver.add_test_transition(sb, {sd});
    solver.add_test_transition(sc, {sd});
    solver.add_test_transition(sd, {sb});  // Creates cycle: b→d→b

    auto sccs = solver.find_sccs_for_testing();

    REQUIRE(sccs.size() == 3);

    // b and d should be in the same SCC (2-node cycle)
    REQUIRE(same_scc(sb, sd, sccs));

    // a and c should each be in their own SCC
    bool a_in_scc_with_b = same_scc(sa, sb, sccs);
    REQUIRE(!a_in_scc_with_b);

    bool c_in_scc_with_b = same_scc(sc, sb, sccs);
    REQUIRE(!c_in_scc_with_b);
}

//==============================================================================
// Test Case 9: Multiple Roots
//==============================================================================
TEST_CASE("Tarjan SCC: Multiple Roots", "[tarjan][scc][basic]") {
    // Graph: a → b
    //        c → d
    //        e → f
    // Three disconnected components
    // Expected: 6 SCCs of size 1

    FormulaPool pool;
    OnTheFlyGameSolver solver(pool.create_true(), pool, 0, 0);

    auto sa = make_state(0, Player::System);
    auto sb = make_state(1, Player::Environment);
    auto sc = make_state(2, Player::System);
    auto sd = make_state(3, Player::Environment);
    auto se = make_state(4, Player::System);
    auto sf = make_state(5, Player::Environment);

    solver.add_test_transition(sa, {sb});
    solver.add_test_transition(sc, {sd});
    solver.add_test_transition(se, {sf});

    auto sccs = solver.find_sccs_for_testing();

    REQUIRE(sccs.size() == 6);

    auto sizes = count_scc_sizes(sccs);
    REQUIRE(sizes[1] == 6);
}

//==============================================================================
// Test Case 10: Large Cycle
//==============================================================================
TEST_CASE("Tarjan SCC: Large Cycle", "[tarjan][scc][cycle]") {
    // Graph: a → b → c → d → e → f → a
    // Expected: 1 SCC of size 6

    FormulaPool pool;
    OnTheFlyGameSolver solver(pool.create_true(), pool, 0, 0);

    std::vector<GameState> states;
    for (int i = 0; i < 6; i++) {
        Player p = (i % 2 == 0) ? Player::System : Player::Environment;
        states.push_back(make_state(i, p));
    }

    // Create cycle
    for (size_t i = 0; i < states.size(); i++) {
        solver.add_test_transition(states[i], {states[(i + 1) % states.size()]});
    }

    auto sccs = solver.find_sccs_for_testing();

    REQUIRE(sccs.size() == 1);
    REQUIRE(sccs[0].size() == 6);

    // All states should be in the same SCC
    for (size_t i = 0; i < states.size() - 1; i++) {
        REQUIRE(same_scc(states[i], states[i + 1], sccs));
    }
}

//==============================================================================
// Test Case 11: SCC with Incoming and Outgoing Edges
//==============================================================================
TEST_CASE("Tarjan SCC: SCC with Incoming and Outgoing Edges", "[tarjan][scc][complex]") {
    // Graph: a → SCC(b→c→b) → d
    // Expected: 3 SCCs: {a}, {b, c}, {d}

    FormulaPool pool;
    OnTheFlyGameSolver solver(pool.create_true(), pool, 0, 0);

    auto sa = make_state(0, Player::System);
    auto sb = make_state(1, Player::Environment);
    auto sc = make_state(2, Player::System);
    auto sd = make_state(3, Player::Environment);

    solver.add_test_transition(sa, {sb});
    solver.add_test_transition(sb, {sc});
    solver.add_test_transition(sc, {sb, sd});

    auto sccs = solver.find_sccs_for_testing();

    REQUIRE(sccs.size() == 3);

    auto sizes = count_scc_sizes(sccs);
    REQUIRE(sizes[1] == 2);  // {a} and {d}
    REQUIRE(sizes[2] == 1);  // {b, c}

    // b and c should be in the same SCC
    REQUIRE(same_scc(sb, sc, sccs));
}

//==============================================================================
// Test Case 12: Empty Graph
//==============================================================================
TEST_CASE("Tarjan SCC: Empty Graph", "[tarjan][scc][edge]") {
    // Graph: (no states)
    // Expected: 0 SCCs

    FormulaPool pool;
    OnTheFlyGameSolver solver(pool.create_true(), pool, 0, 0);

    auto sccs = solver.find_sccs_for_testing();

    REQUIRE(sccs.size() == 0);
}

//==============================================================================
// Test Case 13: Topological Order Property
//==============================================================================
TEST_CASE("Tarjan SCC: Topological Order Property", "[tarjan][scc][property]") {
    // Tarjan should find SCCs in reverse topological order
    // Graph: SCC1 → SCC2 → SCC3
    // Found order should be: SCC3, SCC2, SCC1

    FormulaPool pool;
    OnTheFlyGameSolver solver(pool.create_true(), pool, 0, 0);

    auto s1 = make_state(1, Player::System);
    auto s2 = make_state(2, Player::Environment);
    auto s3 = make_state(3, Player::System);
    auto s4 = make_state(4, Player::Environment);
    auto s5 = make_state(5, Player::System);
    auto s6 = make_state(6, Player::Environment);

    // Create 3 linear SCCs (each is a single node)
    // SCC1: {s1}, SCC2: {s2}, SCC3: {s3}
    // Wait, let's create actual SCCs:
    // SCC1: {s1, s2} with cycle, SCC2: {s3, s4} with cycle, SCC3: {s5, s6} with cycle
    // Edges: SCC1 → SCC2 → SCC3

    // SCC1: s1 ↔ s2
    solver.add_test_transition(s1, {s2});
    solver.add_test_transition(s2, {s1});

    // SCC2: s3 ↔ s4
    solver.add_test_transition(s3, {s4});
    solver.add_test_transition(s4, {s3});

    // SCC3: s5 ↔ s6
    solver.add_test_transition(s5, {s6});
    solver.add_test_transition(s6, {s5});

    // Connecting edges: SCC1 → SCC2 → SCC3
    solver.add_test_transition(s2, {s3});  // SCC1 → SCC2
    solver.add_test_transition(s4, {s5});  // SCC2 → SCC3

    auto sccs = solver.find_sccs_for_testing();

    REQUIRE(sccs.size() == 3);

    // Verify each SCC
    for (const auto& scc : sccs) {
        REQUIRE(scc.size() == 2);
    }

    // SCC3 (s5, s6) should be found first (no outgoing edges within its SCC group to others)
    // SCC2 (s3, s4) should be found second
    // SCC1 (s1, s2) should be found last

    // Find which SCC contains which states
    int s1_scc_idx = -1, s3_scc_idx = -1, s5_scc_idx = -1;
    for (size_t i = 0; i < sccs.size(); i++) {
        if (std::find(sccs[i].begin(), sccs[i].end(), s1) != sccs[i].end()) s1_scc_idx = i;
        if (std::find(sccs[i].begin(), sccs[i].end(), s3) != sccs[i].end()) s3_scc_idx = i;
        if (std::find(sccs[i].begin(), sccs[i].end(), s5) != sccs[i].end()) s5_scc_idx = i;
    }

    REQUIRE(s1_scc_idx >= 0);
    REQUIRE(s3_scc_idx >= 0);
    REQUIRE(s5_scc_idx >= 0);

    // Reverse topological order: SCC3 (s5,s6), SCC2 (s3,s4), SCC1 (s1,s2)
    // So s5_scc_idx < s3_scc_idx < s1_scc_idx
    REQUIRE(s5_scc_idx < s3_scc_idx);
    REQUIRE(s3_scc_idx < s1_scc_idx);
}

//==============================================================================
// Custom Main
//==============================================================================

int main(int argc, char* argv[]) {
    return Catch::Session().run(argc, argv);
}
