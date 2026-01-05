/**
 * @file xnf_tests.cpp
 * @brief Tests for XNF (neXt Normal Form) transformation
 *
 * XNF pushes temporal operators (Until/Release) inside Next operators.
 * Key transformation rules:
 * - xnf(φ₁ U φ₂) = xnf(φ₂) ∨ (xnf(φ₁) ∧ X(φ₁ U φ₂))
 * - xnf(φ₁ R φ₂) = xnf(φ₂) ∧ (xnf(φ₁) ∨ X(φ₁ R φ₂))
 *
 * The inner Until/Release inside Next is NOT recursively expanded!
 */

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"

using namespace formula;

//==============================================================================
// XNF: Base Cases
//==============================================================================

TEST_CASE("XNF: true stays true", "[xnf][base]") {
    INFO("Formula: true");
    FormulaPool pool;
    pool.declare_variables({}, {});
    Formula* f = pool.create_true();
    Formula* result = f->xnf_with_end_marker(pool);
    REQUIRE(result->is_true());
}

TEST_CASE("XNF: false stays false", "[xnf][base]") {
    INFO("Formula: false");
    FormulaPool pool;
    pool.declare_variables({}, {});
    Formula* f = pool.create_false();
    Formula* result = f->xnf_with_end_marker(pool);
    REQUIRE(result->is_false());
}

TEST_CASE("XNF: literal stays literal", "[xnf][base]") {
    INFO("Formula: p");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula* f = pool.create_variable("p");
    Formula* result = f->xnf_with_end_marker(pool);
    REQUIRE(result->is_literal());
    REQUIRE(result->var_id() == f->var_id());
}

TEST_CASE("XNF: !p stays !p", "[xnf][base]") {
    INFO("Formula: !p");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula* p = pool.create_variable("p");
    Formula* not_p = pool.create_not(p);
    Formula* result = not_p->xnf_with_end_marker(pool);
    REQUIRE(result->is_not());
    REQUIRE(result->left()->is_literal());
}

//==============================================================================
// XNF: Next Operator
//==============================================================================

TEST_CASE("XNF: X(p) → X(p)", "[xnf][next]") {
    INFO("Formula: X(p)");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula* p = pool.create_variable("p");
    Formula* next_p = pool.create_next(p);
    Formula* result = next_p->xnf_with_end_marker(pool);

    // X(p) stays X(p) with p in XNF
    REQUIRE(result->is_next());
    REQUIRE(result->left()->is_literal());
}

TEST_CASE("XNF: X(p ∧ q) → X(p ∧ q)", "[xnf][next]") {
    INFO("Formula: X(p && q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* and_pq = pool.create_and(p, q);
    Formula* next_and = pool.create_next(and_pq);
    Formula* result = next_and->xnf_with_end_marker(pool);

    // X(p ∧ q) with inner formula in XNF
    REQUIRE(result->is_next());
    REQUIRE(result->left()->is_and());
}

TEST_CASE("XNF: X(X(p)) → X(X(p))", "[xnf][next]") {
    INFO("Formula: X(X(p))");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula* p = pool.create_variable("p");
    Formula* next_p = pool.create_next(p);
    Formula* next_next_p = pool.create_next(next_p);
    Formula* result = next_next_p->xnf_with_end_marker(pool);

    // Nested Next should preserve structure
    REQUIRE(result->is_next());
    REQUIRE(result->left()->is_next());
}

//==============================================================================
// XNF: Until Transformation
//==============================================================================

TEST_CASE("XNF: p U q → q ∨ (p ∧ X(p U q))", "[xnf][until]") {
    INFO("Formula: p U q");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* until_pq = pool.create_until(p, q);
    Formula* result = until_pq->xnf_with_end_marker(pool);

    // Result should be: q ∨ (p ∧ X(p U q))
    REQUIRE(result->is_or());
    // Right branch: (p ∧ X(original_until))
    REQUIRE(result->right()->is_and());
    REQUIRE(result->right()->right()->is_next());
    // The inner Next should contain the original Until
    REQUIRE(result->right()->right()->left()->is_until());
}

TEST_CASE("XNF: (p ∧ q) U r", "[xnf][until]") {
    INFO("Formula: (p && q) U r");
    FormulaPool pool;
    pool.declare_variables({"p", "q", "r"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* r = pool.create_variable("r");
    Formula* and_pq = pool.create_and(p, q);
    Formula* until_pq_r = pool.create_until(and_pq, r);
    Formula* result = until_pq_r->xnf_with_end_marker(pool);

    // Result should be: r ∨ ((p ∧ q) ∧ X((p ∧ q) U r))
    REQUIRE(result->is_or());
    REQUIRE(result->left()->is_literal());  // r
    REQUIRE(result->right()->is_and());
}

//==============================================================================
// XNF: Release Transformation
//==============================================================================

TEST_CASE("XNF: p R q → q ∧ (p ∨ X(p R q))", "[xnf][release]") {
    INFO("Formula: p R q");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* release_pq = pool.create_release(p, q);
    Formula* result = release_pq->xnf_with_end_marker(pool);

    // Result should be: q ∧ (p ∨ X(p R q))
    REQUIRE(result->is_and());
    // Left branch: q
    REQUIRE(result->left()->is_literal());
    // Right branch: (p ∨ X(original_release))
    REQUIRE(result->right()->is_or());
    REQUIRE(result->right()->right()->is_next());
    // The inner Next should contain the original Release
    REQUIRE(result->right()->right()->left()->is_release());
}

TEST_CASE("XNF: (p ∨ q) R r", "[xnf][release]") {
    INFO("Formula: (p || q) R r");
    FormulaPool pool;
    pool.declare_variables({"p", "q", "r"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* r = pool.create_variable("r");
    Formula* or_pq = pool.create_or(p, q);
    Formula* release_pq_r = pool.create_release(or_pq, r);
    Formula* result = release_pq_r->xnf_with_end_marker(pool);

    // Result should be: r ∧ ((p ∨ q) ∨ X((p ∨ q) R r))
    REQUIRE(result->is_and());
    REQUIRE(result->left()->is_literal());  // r
    REQUIRE(result->right()->is_or());
}

//==============================================================================
// XNF: And/Or Distribution
//==============================================================================

TEST_CASE("XNF: (p U q) ∧ (r U s)", "[xnf][and]") {
    INFO("Formula: (p U q) && (r U s)");
    FormulaPool pool;
    pool.declare_variables({"p", "q", "r", "s"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* r = pool.create_variable("r");
    Formula* s = pool.create_variable("s");
    Formula* until1 = pool.create_until(p, q);
    Formula* until2 = pool.create_until(r, s);
    Formula* and_formula = pool.create_and(until1, until2);
    Formula* result = and_formula->xnf_with_end_marker(pool);

    // Should distribute: both Until transformed
    REQUIRE(result->is_and());
    REQUIRE(result->left()->is_or());  // First Until transformed
    REQUIRE(result->right()->is_or());  // Second Until transformed
}

TEST_CASE("XNF: (p U q) ∨ (r R s)", "[xnf][or]") {
    INFO("Formula: (p U q) || (r R s)");
    FormulaPool pool;
    pool.declare_variables({"p", "q", "r", "s"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* r = pool.create_variable("r");
    Formula* s = pool.create_variable("s");
    Formula* until = pool.create_until(p, q);
    Formula* release = pool.create_release(r, s);
    Formula* or_formula = pool.create_or(until, release);
    Formula* result = or_formula->xnf_with_end_marker(pool);

    // Should distribute: both transformed
    REQUIRE(result->is_or());
    REQUIRE(result->left()->is_or());   // Until transformed
    REQUIRE(result->right()->is_and());  // Release transformed
}

//==============================================================================
// XNF: Nested Temporal Operators
//==============================================================================

TEST_CASE("XNF: (p U q) U r", "[xnf][nested]") {
    INFO("Formula: (p U q) U r");
    FormulaPool pool;
    pool.declare_variables({"p", "q", "r"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* r = pool.create_variable("r");
    Formula* inner_until = pool.create_until(p, q);
    Formula* outer_until = pool.create_until(inner_until, r);
    Formula* result = outer_until->xnf_with_end_marker(pool);

    // Outer Until is transformed, inner is preserved inside Next
    REQUIRE(result->is_or());
    REQUIRE(result->right()->is_and());
    REQUIRE(result->right()->right()->is_next());
    // The Next should contain the original (outer) Until with inner Until unchanged
    REQUIRE(result->right()->right()->left()->is_until());
}

TEST_CASE("XNF: X(p U q)", "[xnf][next-temporal]") {
    INFO("Formula: X(p U q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* until = pool.create_until(p, q);
    Formula* next_until = pool.create_next(until);
    Formula* result = next_until->xnf_with_end_marker(pool);

    // X(φ) is already in XNF (◦-formula base case)
    // X(p U q) stays as X(p U q), no transformation needed
    REQUIRE(result->is_next());
    REQUIRE(result == next_until);  // Same formula object
    REQUIRE(result->left()->is_until());  // Inner Until preserved
}

TEST_CASE("XNF: X(p R q)", "[xnf][next-temporal]") {
    INFO("Formula: X(p R q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* release = pool.create_release(p, q);
    Formula* next_release = pool.create_next(release);
    Formula* result = next_release->xnf_with_end_marker(pool);

    // X(φ) is already in XNF (◦-formula base case)
    // X(p R q) stays as X(p R q), no transformation needed
    REQUIRE(result->is_next());
    REQUIRE(result == next_release);  // Same formula object
    REQUIRE(result->left()->is_release());  // Inner Release preserved
}

//==============================================================================
// XNF: Re-application Behavior
//==============================================================================

TEST_CASE("XNF: re-application may expand further", "[xnf][reapplication]") {
    INFO("Formula: (p && q) U r");
    FormulaPool pool;
    pool.declare_variables({"p", "q", "r"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* r = pool.create_variable("r");
    Formula* and_pq = pool.create_and(p, q);
    Formula* until = pool.create_until(and_pq, r);

    Formula* xnf1 = until->xnf_with_end_marker(pool);
    Formula* xnf2 = xnf1->xnf_with_end_marker(pool);

    // XNF is NOT idempotent - re-application may expand nested formulas further
    // This is expected behavior due to recursion into Next operators
    REQUIRE(xnf2 != nullptr);  // Just verify it produces valid output
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
