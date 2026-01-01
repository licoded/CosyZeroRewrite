#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"
#include "formula/formula_checker.hpp"
#include "log/logger.hpp"

using namespace formula;

// =============================================================================
// Parser Tests
// =============================================================================

TEST_CASE("Parser: Simple literals", "[parser]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f = parser.parse("a");
    REQUIRE_FALSE(parser.has_error());
    REQUIRE(f->is_literal());
    REQUIRE(f->var_id() == 0);
}

TEST_CASE("Parser: Constants", "[parser]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    REQUIRE(parser.parse("true")->is_true());
    REQUIRE(parser.parse("false")->is_false());
}

TEST_CASE("Parser: Negation", "[parser]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f = parser.parse("!a");
    REQUIRE_FALSE(parser.has_error());
    REQUIRE(f->is_not());
    REQUIRE(f->left()->is_literal());
}

TEST_CASE("Parser: And", "[parser]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f = parser.parse("a & b");
    REQUIRE_FALSE(parser.has_error());
    REQUIRE(f->is_and());
    REQUIRE(f->left()->is_literal());
    REQUIRE(f->right()->is_literal());
}

TEST_CASE("Parser: Or", "[parser]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f = parser.parse("a | b");
    REQUIRE_FALSE(parser.has_error());
    REQUIRE(f->is_or());
}

TEST_CASE("Parser: Next", "[parser]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f = parser.parse("X(a)");
    REQUIRE_FALSE(parser.has_error());
    REQUIRE(f->is_next());
    REQUIRE(f->left()->is_literal());
}

TEST_CASE("Parser: Until", "[parser]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f = parser.parse("a U b");
    REQUIRE_FALSE(parser.has_error());
    REQUIRE(f->is_until());
}

TEST_CASE("Parser: Release", "[parser]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f = parser.parse("a R b");
    REQUIRE_FALSE(parser.has_error());
    REQUIRE(f->is_release());
}

TEST_CASE("Parser: Complex formula with parentheses", "[parser]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f = parser.parse("!(a & b) | X(c)");
    REQUIRE_FALSE(parser.has_error());
    REQUIRE(f->is_or());
}

TEST_CASE("Parser: Nested parentheses", "[parser]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f = parser.parse("((a & b) | c) U d");
    REQUIRE_FALSE(parser.has_error());
    REQUIRE(f->is_until());
}

TEST_CASE("Parser: Multi-char variables", "[parser]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    parser.set_variables({"var1", "var2", "state1"});
    Formula* f = parser.parse("var1 & var2");
    REQUIRE_FALSE(parser.has_error());
    REQUIRE(f->is_and());
}

TEST_CASE("Parser: Auto-declare single char variables", "[parser]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    // Variables a, b, c should be auto-declared
    Formula* f = parser.parse("a & b | c");
    REQUIRE_FALSE(parser.has_error());
    REQUIRE(pool.has_variable("a"));
    REQUIRE(pool.has_variable("b"));
    REQUIRE(pool.has_variable("c"));
}

// =============================================================================
// Equivalence Checking Tests
// =============================================================================

TEST_CASE("Checker: Are equivalent - simple cases", "[checker][equiv]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f1 = parser.parse("a & b");
    Formula* f2 = parser.parse("b & a");  // Commutative
    REQUIRE(FormulaChecker::are_equivalent(pool, f1, f2));

    Formula* f3 = parser.parse("a | b");
    Formula* f4 = parser.parse("b | a");
    REQUIRE(FormulaChecker::are_equivalent(pool, f3, f4));
}

TEST_CASE("Checker: Are equivalent - De Morgan", "[checker][equiv]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f1 = parser.parse("!(a & b)");
    Formula* f2 = parser.parse("!a | !b");
    REQUIRE(FormulaChecker::are_equivalent(pool, f1, f2));

    Formula* f3 = parser.parse("!(a | b)");
    Formula* f4 = parser.parse("!a & !b");
    REQUIRE(FormulaChecker::are_equivalent(pool, f3, f4));
}

TEST_CASE("Checker: Are equivalent - Double negation", "[checker][equiv]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f1 = parser.parse("!!a");
    Formula* f2 = parser.parse("a");
    REQUIRE(FormulaChecker::are_equivalent(pool, f1, f2));
}

TEST_CASE("Checker: Are equivalent - Idempotent", "[checker][equiv]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f1 = parser.parse("a & a");
    Formula* f2 = parser.parse("a");
    REQUIRE(FormulaChecker::are_equivalent(pool, f1, f2));

    Formula* f3 = parser.parse("a | a");
    Formula* f4 = parser.parse("a");
    REQUIRE(FormulaChecker::are_equivalent(pool, f3, f4));
}

TEST_CASE("Checker: NOT equivalent - counterexample", "[checker][equiv]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f1 = parser.parse("a & b");
    Formula* f2 = parser.parse("a | b");
    REQUIRE_FALSE(FormulaChecker::are_equivalent(pool, f1, f2));
}

// =============================================================================
// NNF Property Checking Tests
// =============================================================================

TEST_CASE("Checker: is_nnf - literals", "[checker][nnf]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f1 = parser.parse("a");
    REQUIRE(FormulaChecker::is_nnf(f1));

    Formula* f2 = parser.parse("!a");
    REQUIRE(FormulaChecker::is_nnf(f2));
}

TEST_CASE("Checker: is_nnf - in NNF form", "[checker][nnf]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f1 = parser.parse("!a | !b");
    REQUIRE(FormulaChecker::is_nnf(f1));

    Formula* f2 = parser.parse("X(!a)");
    REQUIRE(FormulaChecker::is_nnf(f2));
}

TEST_CASE("Checker: is_nnf - not in NNF form", "[checker][nnf]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f1 = parser.parse("!(a & b)");
    REQUIRE_FALSE(FormulaChecker::is_nnf(f1));

    Formula* f2 = parser.parse("!(a | b)");
    REQUIRE_FALSE(FormulaChecker::is_nnf(f2));

    Formula* f3 = parser.parse("!X(a)");
    REQUIRE_FALSE(FormulaChecker::is_nnf(f3));
}

TEST_CASE("Checker: NNF transformation produces NNF", "[checker][nnf]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f1 = parser.parse("!(a & b)");
    Formula* nnf1 = f1->nnf(pool);
    REQUIRE(FormulaChecker::is_nnf(nnf1));

    Formula* f2 = parser.parse("!X(a)");
    Formula* nnf2 = f2->nnf(pool);
    REQUIRE(FormulaChecker::is_nnf(nnf2));
}

// =============================================================================
// XNF Property Checking Tests
// =============================================================================

TEST_CASE("Checker: XNF produces XNF", "[checker][xnf]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f1 = parser.parse("a U b");
    Formula* xnf1 = f1->xnf_with_tail(pool);
    REQUIRE(FormulaChecker::is_xnf(xnf1));

    Formula* f2 = parser.parse("a R b");
    Formula* xnf2 = f2->xnf_with_tail(pool);
    REQUIRE(FormulaChecker::is_xnf(xnf2));
}

// =============================================================================
// Formula Analysis Tests
// =============================================================================

TEST_CASE("Checker: formula_size", "[checker][analysis]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f1 = parser.parse("a");
    REQUIRE(FormulaChecker::formula_size(f1) == 1);

    Formula* f2 = parser.parse("a & b");
    REQUIRE(FormulaChecker::formula_size(f2) == 3);  // & + a + b

    Formula* f3 = parser.parse("(a & b) | c");
    REQUIRE(FormulaChecker::formula_size(f3) == 5);  // | + & + a + b + c
}

TEST_CASE("Checker: formula_depth", "[checker][analysis]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f1 = parser.parse("a");
    REQUIRE(FormulaChecker::formula_depth(f1) == 1);

    Formula* f2 = parser.parse("a & b");
    REQUIRE(FormulaChecker::formula_depth(f2) == 2);

    Formula* f3 = parser.parse("X(a & b)");
    REQUIRE(FormulaChecker::formula_depth(f3) == 3);
}

TEST_CASE("Checker: get_variables", "[checker][analysis]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f = parser.parse("(a & b) | X(a)");
    auto vars = FormulaChecker::get_variables(f);

    REQUIRE(vars.size() == 2);
    REQUIRE(vars.count(0) == 1);  // a
    REQUIRE(vars.count(1) == 1);  // b
}

TEST_CASE("Checker: get_literals", "[checker][analysis]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f = parser.parse("(a & !b) | c");
    auto literals = FormulaChecker::get_literals(f);

    REQUIRE(literals.size() == 3);
}

// =============================================================================
// Property-Based Tests (Simplified)
// =============================================================================

TEST_CASE("Property: NNF preserves semantics", "[property][nnf]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    // Test multiple formulas
    std::vector<std::string> formulas = {
        "a",
        "!a",
        "a & b",
        "a | b",
        "!(a & b)",
        "!(a | b)",
        "!X(a)",
        "X(a)",
        "a U b",
        "a R b"
    };

    for (const auto& formula_str : formulas) {
        Formula* f = parser.parse(formula_str);
        Formula* nnf_f = f->nnf(pool);

        // NNF should preserve semantics
        REQUIRE(FormulaChecker::are_equivalent(pool, f, nnf_f));

        // NNF should actually be in NNF
        REQUIRE(FormulaChecker::is_nnf(nnf_f));
    }
}

TEST_CASE("Property: XNF preserves semantics", "[property][xnf]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    std::vector<std::string> formulas = {
        "a",
        "X(a)",
        "a & b",
        "a | b",
        "a U b",
        "a R b",
        "X(a) U b",
        "X(a & b)"
    };

    for (const auto& formula_str : formulas) {
        Formula* f = parser.parse(formula_str);
        Formula* xnf_f = f->xnf_with_tail(pool);

        // XNF should preserve semantics
        REQUIRE(FormulaChecker::are_equivalent(pool, f, xnf_f));

        // XNF should actually be in XNF
        REQUIRE(FormulaChecker::is_xnf(xnf_f));
    }
}

TEST_CASE("Property: Simplify preserves semantics", "[property][simplify]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    std::vector<std::string> formulas = {
        "a & true",
        "a | false",
        "a & a",
        "!!a",
        "true & a",
        "false | a",
        "(a & b) & (b & a)"
    };

    for (const auto& formula_str : formulas) {
        Formula* f = parser.parse(formula_str);
        Formula* simp_f = f->simplify(pool);

        // Simplify should preserve semantics
        REQUIRE(FormulaChecker::are_equivalent(pool, f, simp_f));

        // Simplify should not increase size
        REQUIRE(FormulaChecker::formula_size(simp_f) <=
                FormulaChecker::formula_size(f));
    }
}

TEST_CASE("Property: Simplify is idempotent", "[property][simplify]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    std::vector<std::string> formulas = {
        "a",
        "a & b & c",
        "a | b | c",
        "!(!a)",
        "X(X(a))"
    };

    for (const auto& formula_str : formulas) {
        Formula* f = parser.parse(formula_str);
        Formula* simp1 = f->simplify(pool);
        Formula* simp2 = simp1->simplify(pool);

        // Simplify twice should give same result as once
        REQUIRE(FormulaChecker::are_equivalent(pool, simp1, simp2));
        REQUIRE(FormulaChecker::formula_size(simp1) == FormulaChecker::formula_size(simp2));
    }
}

TEST_CASE("Property: NNF is idempotent", "[property][nnf]") {
    FormulaPool pool;
    FormulaParser parser(pool);

    std::vector<std::string> formulas = {
        "a",
        "a & b",
        "a | b",
        "X(a)"
    };

    for (const auto& formula_str : formulas) {
        Formula* f = parser.parse(formula_str);
        Formula* nnf1 = f->nnf(pool);
        Formula* nnf2 = nnf1->nnf(pool);

        // NNF twice should give equivalent result
        REQUIRE(FormulaChecker::are_equivalent(pool, nnf1, nnf2));
    }
}

// =============================================================================
// Custom Main with Logger
// =============================================================================

int main(int argc, char* argv[]) {
    // Initialize logger
    LOG_INFO("Parser & Checker Tests starting...");

    // Run Catch2 tests
    int result = Catch::Session().run(argc, argv);

    // Log completion
    LOG_INFO("Parser & Checker Tests completed with exit code: {}", result);
    LOG_FLUSH();

    return result;
}
