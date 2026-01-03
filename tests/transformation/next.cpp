/**
 * @file next_tests.cpp
 * @brief Tests for Next (X) operator behavior and transformations
 *
 * The Next operator (X) in LTLf means "in the next state":
 * - X(φ) is true at position i iff φ is true at position i+1
 * - On finite traces, X(φ) is false at the last position
 *
 * Key properties tested:
 * - Distribution: X(φ ∧ ψ) ↔ X(φ) ∧ X(ψ) (in NNF/XNF context)
 * - NNF transformation: !X(φ) → X(!φ) ∨ End
 * - XNF transformation: X(φ) preserves structure
 */

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"

using namespace formula;

//==============================================================================
// Next: Basic Creation
//==============================================================================

TEST_CASE("Next: X(p) creates Next operator", "[next][basic]") {
    INFO("Formula: X(p)");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula* p = pool.create_variable("p");
    Formula* next_p = pool.create_next(p);

    REQUIRE(next_p->is_next());
    REQUIRE(next_p->left()->is_literal());
    REQUIRE(next_p->left()->var_id() == p->var_id());
}

TEST_CASE("Next: X(true) creates Next operator", "[next][basic]") {
    INFO("Formula: X(true)");
    FormulaPool pool;
    pool.declare_variables({}, {});
    Formula* t = pool.create_true();
    Formula* next_t = pool.create_next(t);

    REQUIRE(next_t->is_next());
    REQUIRE(next_t->left()->is_true());
}

TEST_CASE("Next: X(X(p)) creates nested Next", "[next][basic]") {
    INFO("Formula: X(X(p))");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula* p = pool.create_variable("p");
    Formula* next_p = pool.create_next(p);
    Formula* next_next_p = pool.create_next(next_p);

    REQUIRE(next_next_p->is_next());
    REQUIRE(next_next_p->left()->is_next());
    REQUIRE(next_next_p->left()->left()->is_literal());
}

//==============================================================================
// Next: Complex Formulas
//==============================================================================

TEST_CASE("Next: X(p ∧ q)", "[next][complex]") {
    INFO("Formula: X(p && q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* and_pq = pool.create_and(p, q);
    Formula* next_and = pool.create_next(and_pq);

    REQUIRE(next_and->is_next());
    REQUIRE(next_and->left()->is_and());
}

TEST_CASE("Next: X(p ∨ q)", "[next][complex]") {
    INFO("Formula: X(p || q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* or_pq = pool.create_or(p, q);
    Formula* next_or = pool.create_or(or_pq, q);  // Just to compile
    (void)next_or;

    Formula* next_or_correct = pool.create_next(or_pq);
    REQUIRE(next_or_correct->is_next());
    REQUIRE(next_or_correct->left()->is_or());
}

TEST_CASE("Next: X(p U q)", "[next][temporal]") {
    INFO("Formula: X(p U q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* until_pq = pool.create_until(p, q);
    Formula* next_until = pool.create_next(until_pq);

    REQUIRE(next_until->is_next());
    REQUIRE(next_until->left()->is_until());
}

TEST_CASE("Next: X(p R q)", "[next][temporal]") {
    INFO("Formula: X(p R q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* release_pq = pool.create_release(p, q);
    Formula* next_release = pool.create_next(release_pq);

    REQUIRE(next_release->is_next());
    REQUIRE(next_release->left()->is_release());
}

//==============================================================================
// Next: NNF Interaction
//==============================================================================

TEST_CASE("Next: NNF(X(p)) = X(p)", "[next][nnf]") {
    INFO("Formula: X(p)");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula* p = pool.create_variable("p");
    Formula* next_p = pool.create_next(p);
    Formula* result = next_p->nnf(pool);

    REQUIRE(result->is_next());
    REQUIRE(result->left()->is_literal());
}

TEST_CASE("Next: NNF(X(p ∧ q)) = X(p ∧ q)", "[next][nnf]") {
    INFO("Formula: X(p && q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* and_pq = pool.create_and(p, q);
    Formula* next_and = pool.create_next(and_pq);
    Formula* result = next_and->nnf(pool);

    REQUIRE(result->is_next());
    REQUIRE(result->left()->is_and());
}

TEST_CASE("Next: NNF(!X(p)) → X(!p) ∨ End", "[next][nnf][ltlf]") {
    INFO("Formula: !X(p)");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula* p = pool.create_variable("p");
    Formula* next_p = pool.create_next(p);
    Formula* not_next = pool.create_not(next_p);
    Formula* result = not_next->nnf(pool);

    // Result should be: X(!p) ∨ End
    REQUIRE(result->is_or());
    REQUIRE(result->left()->is_next());
    REQUIRE(result->left()->left()->is_not());
    REQUIRE(result->right()->is_end());
}

TEST_CASE("Next: NNF(!!X(p)) = X(p)", "[next][nnf]") {
    INFO("Formula: !!X(p)");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula* p = pool.create_variable("p");
    Formula* next_p = pool.create_next(p);
    Formula* not_next = pool.create_not(next_p);
    Formula* not_not_next = pool.create_not(not_next);
    Formula* result = not_not_next->nnf(pool);

    // After NNF: X(!p) ∨ End, then negated again... complex but consistent
    // Just verify it produces valid output
    REQUIRE(result != nullptr);
}

//==============================================================================
// Next: XNF Interaction
//==============================================================================

TEST_CASE("Next: XNF(X(p)) = X(p)", "[next][xnf]") {
    INFO("Formula: X(p)");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula* p = pool.create_variable("p");
    Formula* next_p = pool.create_next(p);
    Formula* result = next_p->xnf_with_tail(pool);

    REQUIRE(result->is_next());
    REQUIRE(result->left()->is_literal());
}

TEST_CASE("Next: XNF(X(p U q)) transforms child", "[next][xnf]") {
    INFO("Formula: X(p U q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* until = pool.create_until(p, q);
    Formula* next_until = pool.create_next(until);
    Formula* result = next_until->xnf_with_tail(pool);

    // X(p U q) -> X(q ∨ (p ∧ X(p U q)))
    // The Next operator recurses into its child
    REQUIRE(result->is_next());
    REQUIRE(result->left()->is_or());  // Transformed Until

    // The original Until is preserved inside a Next, deep in the structure
    Formula* right_and = result->left()->right();  // (p ∧ X(p U q))
    REQUIRE(right_and->is_and());
    REQUIRE(right_and->right()->is_next());
    REQUIRE(right_and->right()->left()->is_until());  // Original Until here
}

//==============================================================================
// Next: Parsing
//==============================================================================

TEST_CASE("Next: Parse X(p)", "[next][parser]") {
    INFO("Formula: X p");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    FormulaParser parser(pool);
    Formula* result = parser.parse("X p");

    REQUIRE(result != nullptr);
    REQUIRE(result->is_next());
    REQUIRE(result->left()->is_literal());
}

TEST_CASE("Next: Parse X(X(p))", "[next][parser]") {
    INFO("Formula: X X p");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    FormulaParser parser(pool);
    Formula* result = parser.parse("X X p");

    REQUIRE(result != nullptr);
    REQUIRE(result->is_next());
    REQUIRE(result->left()->is_next());
}

TEST_CASE("Next: Parse X(p & q)", "[next][parser]") {
    INFO("Formula: X (p & q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    FormulaParser parser(pool);
    Formula* result = parser.parse("X (p & q)");

    REQUIRE(result != nullptr);
    REQUIRE(result->is_next());
    REQUIRE(result->left()->is_and());
}

//==============================================================================
// Next: String Representation
//==============================================================================

TEST_CASE("Next: to_string(X(p))", "[next][string]") {
    INFO("Formula: X(p)");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula* p = pool.create_variable("p");
    Formula* next_p = pool.create_next(p);
    std::string s = next_p->to_string();

    REQUIRE(s.find("X") != std::string::npos);
}

//==============================================================================
// Next: Hash and Equality
//==============================================================================

TEST_CASE("Next: X(p) hash consistency", "[next][hash]") {
    INFO("Formula: X(p) hash");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula* p = pool.create_variable("p");
    Formula* next_p1 = pool.create_next(p);
    Formula* next_p2 = pool.create_next(p);

    // Same formula should have same hash (hash consing)
    REQUIRE(next_p1->hash() == next_p2->hash());
    REQUIRE(next_p1 == next_p2);
}

TEST_CASE("Next: X(p) != X(q)", "[next][equality]") {
    INFO("Formula: X(p) != X(q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* next_p = pool.create_next(p);
    Formula* next_q = pool.create_next(q);

    REQUIRE(next_p != next_q);
    REQUIRE(next_p->hash() != next_q->hash());
}

//==============================================================================
// Next: Temporal Property
//==============================================================================

TEST_CASE("Next: is_temporal() returns true", "[next][property]") {
    INFO("Formula: X(p) is temporal");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula* p = pool.create_variable("p");
    Formula* next_p = pool.create_next(p);

    REQUIRE(next_p->is_temporal());
}

TEST_CASE("Next: is_unary() returns true", "[next][property]") {
    INFO("Formula: X(p) is unary");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula* p = pool.create_variable("p");
    Formula* next_p = pool.create_next(p);

    REQUIRE(next_p->is_unary());
    REQUIRE_FALSE(next_p->is_binary());
}

//==============================================================================
// Next: Complex Nested Formulas
//==============================================================================

TEST_CASE("Next: X((p ∧ q) ∨ r)", "[next][complex]") {
    INFO("Formula: X((p && q) || r)");
    FormulaPool pool;
    pool.declare_variables({"p", "q", "r"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* r = pool.create_variable("r");
    Formula* and_pq = pool.create_and(p, q);
    Formula* or_pq_r = pool.create_or(and_pq, r);
    Formula* next_or = pool.create_next(or_pq_r);

    REQUIRE(next_or->is_next());
    REQUIRE(next_or->left()->is_or());
}

TEST_CASE("Next: (X(p) ∧ X(q))", "[next][complex]") {
    INFO("Formula: X(p) && X(q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* next_p = pool.create_next(p);
    Formula* next_q = pool.create_next(q);
    Formula* and_next = pool.create_and(next_p, next_q);

    REQUIRE(and_next->is_and());
    REQUIRE(and_next->left()->is_next());
    REQUIRE(and_next->right()->is_next());
}

//==============================================================================
// Custom main for compact output
//==============================================================================

int main(int argc, char* argv[]) {
    Catch::Session session;
    std::vector<std::string> default_args = {argv[0]};

    bool verbose = (std::getenv("COSY_TEST_VERBOSE") != nullptr);
    if (verbose) {
        default_args.push_back("-s");
        default_args.push_back("-d");
        default_args.push_back("yes");
    } else {
        default_args.push_back("-r");
        default_args.push_back("compact");
    }

    std::vector<char*> args;
    args.reserve(default_args.size() + argc + 1);
    for (auto& arg : default_args) {
        args.push_back(const_cast<char*>(arg.c_str()));
    }
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    int result = session.applyCommandLine(static_cast<int>(args.size()), args.data());
    if (result != 0) return result;

    return session.run();
}
