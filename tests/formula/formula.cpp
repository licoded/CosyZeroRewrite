#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "log/logger.hpp"

using namespace formula;

// =============================================================================
// Variable Management Tests
// =============================================================================

TEST_CASE("FormulaPool: Declare variables", "[variable][pool]") {
    FormulaPool pool;

    std::vector<std::string> outputs = {"s1", "s2"};
    std::vector<std::string> inputs = {"p1", "p2"};

    pool.declare_variables(outputs, inputs);

    REQUIRE(pool.get_variable_id("s1") == 0);
    REQUIRE(pool.get_variable_id("s2") == 1);
    REQUIRE(pool.get_variable_id("p1") == 2);
    REQUIRE(pool.get_variable_id("p2") == 3);

    REQUIRE(pool.is_output_variable(0));
    REQUIRE(pool.is_output_variable(1));
    REQUIRE(pool.is_input_variable(2));
    REQUIRE(pool.is_input_variable(3));
    REQUIRE(pool.is_fully_declared());
}

TEST_CASE("FormulaPool: Duplicate variables throw", "[variable][pool]") {
    FormulaPool pool;

    REQUIRE_THROWS_AS(
        pool.declare_variables({"s1", "s1"}, {}),
        std::invalid_argument
    );
}

TEST_CASE("FormulaPool: Overlap variables throw", "[variable][pool]") {
    FormulaPool pool;

    REQUIRE_THROWS_AS(
        pool.declare_variables({"s1"}, {"s1"}),
        std::invalid_argument
    );
}

TEST_CASE("FormulaPool: Create variable", "[variable][pool]") {
    FormulaPool pool;
    pool.declare_variables({"a", "b"}, {});

    Formula* a = pool.create_variable("a");
    Formula* b = pool.create_variable("b");

    REQUIRE(a->is_literal());
    REQUIRE(b->is_literal());
    REQUIRE(a->var_id() == 0);
    REQUIRE(b->var_id() == 1);
}

TEST_CASE("FormulaPool: Undeclared variable throws", "[variable][pool]") {
    FormulaPool pool;
    pool.declare_variables({"a"}, {});

    REQUIRE_THROWS_AS(
        pool.create_variable("b"),
        std::runtime_error
    );
}

// =============================================================================
// Formula Creation Tests
// =============================================================================

TEST_CASE("Formula: Create constants", "[formula][creation]") {
    FormulaPool pool;

    Formula* t = pool.create_true();
    Formula* f = pool.create_false();
    Formula* e = pool.create_end_marker();

    REQUIRE(t->is_true());
    REQUIRE(f->is_false());
    REQUIRE(e->is_end());

    REQUIRE(t->to_string() == "true");
    REQUIRE(f->to_string() == "false");
    REQUIRE(e->to_string() == "end");
}

TEST_CASE("Formula: Create unary operators", "[formula][creation]") {
    FormulaPool pool;
    pool.declare_variables({"a"}, {});

    Formula* a = pool.create_variable("a");
    Formula* not_a = pool.create_not(a);
    Formula* next_a = pool.create_next(a);

    REQUIRE(not_a->is_not());
    REQUIRE(next_a->is_next());
    REQUIRE(not_a->left() == a);
    REQUIRE(next_a->left() == a);

    REQUIRE(not_a->to_string() == "!v0");
    REQUIRE(next_a->to_string() == "X(v0)");
}

TEST_CASE("Formula: Create binary operators", "[formula][creation]") {
    FormulaPool pool;
    pool.declare_variables({"a", "b"}, {});

    Formula* a = pool.create_variable("a");
    Formula* b = pool.create_variable("b");

    Formula* and_ab = pool.create_and(a, b);
    Formula* or_ab = pool.create_or(a, b);
    Formula* until_ab = pool.create_until(a, b);
    Formula* release_ab = pool.create_release(a, b);

    REQUIRE(and_ab->is_and());
    REQUIRE(or_ab->is_or());
    REQUIRE(until_ab->is_until());
    REQUIRE(release_ab->is_release());

    REQUIRE(and_ab->left() == a);
    REQUIRE(and_ab->right() == b);
}

TEST_CASE("Formula: Hash consing works", "[formula][canonicalization]") {
    FormulaPool pool;
    pool.declare_variables({"a", "b"}, {});

    Formula* a = pool.create_variable("a");
    Formula* b = pool.create_variable("b");

    // Create same AND twice
    Formula* and1 = pool.create_and(a, b);
    Formula* and2 = pool.create_and(a, b);

    // Should return same pointer due to hash consing
    REQUIRE(and1 == and2);

    // Total count includes: true, false, end, a, b, and_ab (at least 5)
    REQUIRE(pool.total_count() >= 5);
    REQUIRE(pool.deduplication_ratio() >= 0.5);
}

TEST_CASE("Formula: Pool statistics", "[formula][pool]") {
    FormulaPool pool;
    pool.declare_variables({"a", "b"}, {});

    Formula* a = pool.create_variable("a");
    Formula* b = pool.create_variable("b");
    Formula* and_ab = pool.create_and(a, b);
    Formula* or_ab = pool.create_or(a, b);

    // Total includes: true, false, end, a, b, and_ab, or_ab
    REQUIRE(pool.total_count() >= 7);
    REQUIRE(pool.unique_count() == pool.total_count());

    // Create duplicate - should not increase unique count
    Formula* and2 = pool.create_and(a, b);
    REQUIRE(and2 == and_ab);
}

// =============================================================================
// NNF Transformation Tests
// =============================================================================

TEST_CASE("NNF: De Morgan's law", "[nnf][transformation]") {
    FormulaPool pool;
    pool.declare_variables({"a", "b"}, {});

    Formula* a = pool.create_variable("a");
    Formula* b = pool.create_variable("b");

    Formula* and_f = pool.create_and(a, b);
    Formula* not_f = pool.create_not(and_f);
    Formula* nnf_f = not_f->nnf(pool);

    // !(a & b) → !a | !b
    REQUIRE(nnf_f->is_or());
    REQUIRE(nnf_f->left()->is_not());
    REQUIRE(nnf_f->right()->is_not());
}

TEST_CASE("NNF: Double negation", "[nnf][transformation]") {
    FormulaPool pool;
    pool.declare_variables({"a"}, {});

    Formula* a = pool.create_variable("a");
    Formula* not_a = pool.create_not(a);
    Formula* not_not_a = pool.create_not(not_a);
    Formula* nnf_f = not_not_a->nnf(pool);

    // !!a → a
    REQUIRE(nnf_f == a);
}

TEST_CASE("NNF: Next negation with End", "[nnf][transformation]") {
    FormulaPool pool;
    pool.declare_variables({"a"}, {});

    Formula* a = pool.create_variable("a");
    Formula* next_a = pool.create_next(a);
    Formula* not_next_a = pool.create_not(next_a);
    Formula* nnf_f = not_next_a->nnf(pool);

    // !X(a) → X(!a) | End
    REQUIRE(nnf_f->is_or());
    REQUIRE(nnf_f->left()->is_next());
    REQUIRE(nnf_f->right()->is_end());
}

TEST_CASE("NNF: Until/Release duality", "[nnf][transformation]") {
    FormulaPool pool;
    pool.declare_variables({"a", "b"}, {});

    Formula* a = pool.create_variable("a");
    Formula* b = pool.create_variable("b");

    Formula* until_f = pool.create_until(a, b);
    Formula* not_f = pool.create_not(until_f);
    Formula* nnf_f = not_f->nnf(pool);

    // !(a U b) → (!a) R (!b)
    REQUIRE(nnf_f->is_release());
}

// =============================================================================
// Simplify Tests
// =============================================================================

TEST_CASE("Simplify: And with True", "[simplify]") {
    FormulaPool pool;
    pool.declare_variables({"a"}, {});

    Formula* a = pool.create_variable("a");
    Formula* true_f = pool.create_true();

    Formula* and_f = pool.create_and(a, true_f);
    Formula* simp = and_f->simplify(pool);

    // a & True → a
    REQUIRE(simp == a);
}

TEST_CASE("Simplify: And with False", "[simplify]") {
    FormulaPool pool;
    pool.declare_variables({"a"}, {});

    Formula* a = pool.create_variable("a");
    Formula* false_f = pool.create_false();

    Formula* and_f = pool.create_and(a, false_f);
    Formula* simp = and_f->simplify(pool);

    // a & False → False
    REQUIRE(simp->is_false());
}

TEST_CASE("Simplify: Or with False", "[simplify]") {
    FormulaPool pool;
    pool.declare_variables({"a"}, {});

    Formula* a = pool.create_variable("a");
    Formula* false_f = pool.create_false();

    Formula* or_f = pool.create_or(a, false_f);
    Formula* simp = or_f->simplify(pool);

    // a | False → a
    REQUIRE(simp == a);
}

TEST_CASE("Simplify: Or with True", "[simplify]") {
    FormulaPool pool;
    pool.declare_variables({"a"}, {});

    Formula* a = pool.create_variable("a");
    Formula* true_f = pool.create_true();

    Formula* or_f = pool.create_or(a, true_f);
    Formula* simp = or_f->simplify(pool);

    // a | True → True
    REQUIRE(simp->is_true());
}

TEST_CASE("Simplify: Duplicate elimination in And", "[simplify]") {
    FormulaPool pool;
    pool.declare_variables({"a"}, {});

    Formula* a = pool.create_variable("a");
    Formula* and_aa = pool.create_and(a, a);
    Formula* simp = and_aa->simplify(pool);

    // a & a → a
    REQUIRE(simp == a);
}

TEST_CASE("Simplify: Double negation", "[simplify]") {
    FormulaPool pool;
    pool.declare_variables({"a"}, {});

    Formula* a = pool.create_variable("a");
    Formula* not_a = pool.create_not(a);
    Formula* not_not_a = pool.create_not(not_a);
    Formula* simp = not_not_a->simplify(pool);

    // !!a → a
    REQUIRE(simp == a);
}

TEST_CASE("Simplify: Next with False", "[simplify]") {
    FormulaPool pool;
    Formula* false_f = pool.create_false();
    Formula* next_false = pool.create_next(false_f);
    Formula* simp = next_false->simplify(pool);

    // X(False) → False
    REQUIRE(simp->is_false());
}

// =============================================================================
// XNF Transformation Tests
// =============================================================================

TEST_CASE("XNF: Simple Until", "[xnf][transformation]") {
    FormulaPool pool;
    pool.declare_variables({"a", "b"}, {});

    Formula* a = pool.create_variable("a");
    Formula* b = pool.create_variable("b");

    Formula* until_f = pool.create_until(a, b);
    Formula* xnf_f = until_f->xnf_with_end_marker(pool);

    // a U b → (b & !End) | (a & X(a U b))
    REQUIRE(xnf_f->is_or());
}

TEST_CASE("XNF: Next distribution", "[xnf][transformation]") {
    FormulaPool pool;
    pool.declare_variables({"a", "b"}, {});

    Formula* a = pool.create_variable("a");
    Formula* b = pool.create_variable("b");

    Formula* and_ab = pool.create_and(a, b);
    Formula* next_and = pool.create_next(and_ab);
    Formula* xnf_f = next_and->xnf_with_end_marker(pool);

    // X(a & b) → X(xnf(a & b))
    REQUIRE(xnf_f->is_next());
}

// =============================================================================
// rmnext Progression Tests
// =============================================================================

TEST_CASE("rmnext: Literal in edge", "[rmnext][progression]") {
    FormulaPool pool;
    pool.declare_variables({"a"}, {});

    Formula* a = pool.create_variable("a");
    std::unordered_set<int> edge = {0};
    Formula* result = a->rmnext(pool, nullptr, edge);

    REQUIRE(result->is_true());
}

TEST_CASE("rmnext: Literal not in edge", "[rmnext][progression]") {
    FormulaPool pool;
    pool.declare_variables({"a"}, {});

    Formula* a = pool.create_variable("a");
    std::unordered_set<int> edge = {};
    Formula* result = a->rmnext(pool, nullptr, edge);

    REQUIRE(result->is_false());
}

TEST_CASE("rmnext: Next progression", "[rmnext][progression]") {
    FormulaPool pool;
    pool.declare_variables({"a"}, {});

    Formula* a = pool.create_variable("a");
    Formula* next_a = pool.create_next(a);
    std::unordered_set<int> edge = {};
    Formula* result = next_a->rmnext(pool, nullptr, edge);

    // X(a) rmnext → a & !End
    REQUIRE(result->is_and());
}

TEST_CASE("rmnext: And progression", "[rmnext][progression]") {
    FormulaPool pool;
    pool.declare_variables({"a", "b"}, {});

    Formula* a = pool.create_variable("a");
    Formula* b = pool.create_variable("b");
    Formula* and_ab = pool.create_and(a, b);

    std::unordered_set<int> edge = {0, 1};  // Both a and b are true
    Formula* result = and_ab->rmnext(pool, nullptr, edge);

    REQUIRE(result->is_true());
}

TEST_CASE("rmnext: Or progression", "[rmnext][progression]") {
    FormulaPool pool;
    pool.declare_variables({"a", "b"}, {});

    Formula* a = pool.create_variable("a");
    Formula* b = pool.create_variable("b");
    Formula* or_ab = pool.create_or(a, b);

    std::unordered_set<int> edge = {0};  // Only a is true
    Formula* result = or_ab->rmnext(pool, nullptr, edge);

    REQUIRE(result->is_true());
}

TEST_CASE("rmnext: End marker", "[rmnext][progression]") {
    FormulaPool pool;
    Formula* end = pool.create_end_marker();

    std::unordered_set<int> edge = {};
    Formula* result = end->rmnext(pool, nullptr, edge);

    // End rmnext → False
    REQUIRE(result->is_false());
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST_CASE("Integration: Full transformation pipeline", "[integration]") {
    FormulaPool pool;
    pool.declare_variables({"a", "b", "c"}, {});

    Formula* a = pool.create_variable("a");
    Formula* b = pool.create_variable("b");
    Formula* c = pool.create_variable("c");

    // Build: !(a & b) | X(c)
    Formula* and_ab = pool.create_and(a, b);
    Formula* not_and = pool.create_not(and_ab);
    Formula* next_c = pool.create_next(c);
    Formula* formula = pool.create_or(not_and, next_c);

    // Transform: NNF → Simplify → XNF
    Formula* nnf_f = formula->nnf(pool);
    Formula* simp_f = nnf_f->simplify(pool);
    Formula* xnf_f = simp_f->xnf_with_end_marker(pool);

    REQUIRE(xnf_f != nullptr);
    // Check if result is either Or or And
    bool is_valid = xnf_f->is_or() || xnf_f->is_and();
    REQUIRE(is_valid);
}

TEST_CASE("Integration: Complex formula with Next and Until", "[integration]") {
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});

    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");

    // Build: X(p) U q
    Formula* next_p = pool.create_next(p);
    Formula* until_f = pool.create_until(next_p, q);

    // Transform: NNF (already in NNF) → XNF
    Formula* xnf_f = until_f->xnf_with_end_marker(pool);

    // Should be: (q & !End) | (X(p) & X(X(p) U q))
    REQUIRE(xnf_f->is_or());
}

// =============================================================================
// Custom Main with Logger
// =============================================================================

int main(int argc, char* argv[]) {
    // Initialize logger
    LOG_INFO("Formula Tests starting...");

    // Run Catch2 tests
    int result = Catch::Session().run(argc, argv);

    // Log completion
    LOG_INFO("Formula Tests completed with exit code: {}", result);
    LOG_FLUSH();

    return result;
}
