/**
 * @file nnf_tests.cpp
 * @brief Tests for NNF (Negation Normal Form) transformation
 *
 * NNF pushes negations inward so that negation only applies to literals.
 * Key transformation rules:
 * - !!φ → φ (double negation)
 * - !(φ ∧ ψ) → !φ ∨ !ψ (De Morgan)
 * - !(φ ∨ ψ) → !φ ∧ !ψ (De Morgan)
 * - !(φ U ψ) → (!φ) R (!ψ) (temporal duality)
 * - !(φ R ψ) → (!φ) U (!ψ) (temporal duality)
 * - !X(φ) → X(!φ) ∨ End (LTLf specific)
 */

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"

using namespace formula;

//==============================================================================
// NNF: Base Cases
//==============================================================================

TEST_CASE("NNF: true stays true", "[nnf][base]") {
    INFO("Formula: true");
    FormulaPool pool;
    pool.declare_variables({}, {});
    Formula* f = pool.create_true();
    Formula* result = f->nnf(pool);
    REQUIRE(result->is_true());
}

TEST_CASE("NNF: false stays false", "[nnf][base]") {
    INFO("Formula: false");
    FormulaPool pool;
    pool.declare_variables({}, {});
    Formula* f = pool.create_false();
    Formula* result = f->nnf(pool);
    REQUIRE(result->is_false());
}

TEST_CASE("NNF: literal stays literal", "[nnf][base]") {
    INFO("Formula: p");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula* f = pool.create_variable("p");
    Formula* result = f->nnf(pool);
    REQUIRE(result->is_literal());
    REQUIRE(result->var_id() == f->var_id());
}

//==============================================================================
// NNF: Double Negation
//==============================================================================

TEST_CASE("NNF: !!p → p", "[nnf][double-negation]") {
    INFO("Formula: !!p");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula* p = pool.create_variable("p");
    Formula* not_p = pool.create_not(p);
    Formula* not_not_p = pool.create_not(not_p);
    Formula* result = not_not_p->nnf(pool);
    REQUIRE(result->is_literal());
    REQUIRE(result->var_id() == p->var_id());
}

TEST_CASE("NNF: !!true → true", "[nnf][double-negation]") {
    INFO("Formula: !!true");
    FormulaPool pool;
    pool.declare_variables({}, {});
    Formula* t = pool.create_true();
    Formula* not_t = pool.create_not(t);
    Formula* not_not_t = pool.create_not(not_t);
    Formula* result = not_not_t->nnf(pool);
    REQUIRE(result->is_true());
}

//==============================================================================
// NNF: De Morgan's Laws
//==============================================================================

TEST_CASE("NNF: !(p ∧ q) → !p ∨ !q", "[nnf][demorgan]") {
    INFO("Formula: !(p && q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* and_pq = pool.create_and(p, q);
    Formula* not_and = pool.create_not(and_pq);
    Formula* result = not_and->nnf(pool);

    // Result should be: !p ∨ !q
    REQUIRE(result->is_or());
    REQUIRE(result->left()->is_not());
    REQUIRE(result->left()->left()->is_literal());
    REQUIRE(result->right()->is_not());
    REQUIRE(result->right()->left()->is_literal());
}

TEST_CASE("NNF: !(p ∨ q) → !p ∧ !q", "[nnf][demorgan]") {
    INFO("Formula: !(p || q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* or_pq = pool.create_or(p, q);
    Formula* not_or = pool.create_not(or_pq);
    Formula* result = not_or->nnf(pool);

    // Result should be: !p ∧ !q
    REQUIRE(result->is_and());
    REQUIRE(result->left()->is_not());
    REQUIRE(result->left()->left()->is_literal());
    REQUIRE(result->right()->is_not());
    REQUIRE(result->right()->left()->is_literal());
}

//==============================================================================
// NNF: Temporal Duality
//==============================================================================

TEST_CASE("NNF: !(p U q) → (!p) R (!q)", "[nnf][temporal]") {
    INFO("Formula: !(p U q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* until_pq = pool.create_until(p, q);
    Formula* not_until = pool.create_not(until_pq);
    Formula* result = not_until->nnf(pool);

    // Result should be: (!p) R (!q)
    REQUIRE(result->is_release());
    REQUIRE(result->left()->is_not());
    REQUIRE(result->right()->is_not());
}

TEST_CASE("NNF: !(p R q) → (!p) U (!q)", "[nnf][temporal]") {
    INFO("Formula: !(p R q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* release_pq = pool.create_release(p, q);
    Formula* not_release = pool.create_not(release_pq);
    Formula* result = not_release->nnf(pool);

    // Result should be: (!p) U (!q)
    REQUIRE(result->is_until());
    REQUIRE(result->left()->is_not());
    REQUIRE(result->right()->is_not());
}

//==============================================================================
// NNF: Next Negation (LTLf specific)
//==============================================================================

TEST_CASE("NNF: !X(p) → X(!p) ∨ End", "[nnf][next][ltlf]") {
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

//==============================================================================
// NNF: Complex Nested Formulas
//==============================================================================

TEST_CASE("NNF: !(p ∧ (q ∨ r)) → !p ∨ (!q ∧ !r)", "[nnf][complex]") {
    INFO("Formula: !(p && (q || r))");
    FormulaPool pool;
    pool.declare_variables({"p", "q", "r"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* r = pool.create_variable("r");
    Formula* or_qr = pool.create_or(q, r);
    Formula* and_p_qr = pool.create_and(p, or_qr);
    Formula* not_and = pool.create_not(and_p_qr);
    Formula* result = not_and->nnf(pool);

    // Result should be: !p ∨ (!q ∧ !r)
    REQUIRE(result->is_or());
    REQUIRE(result->left()->is_not());
    REQUIRE(result->right()->is_and());
}

TEST_CASE("NNF: X(p ∧ q) preserves structure", "[nnf][temporal]") {
    INFO("Formula: X(p && q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* and_pq = pool.create_and(p, q);
    Formula* next_and = pool.create_next(and_pq);
    Formula* result = next_and->nnf(pool);

    // Result should be: X(p ∧ q) (structure preserved)
    REQUIRE(result->is_next());
    REQUIRE(result->left()->is_and());
}

TEST_CASE("NNF: (p U q) ∧ r preserves structure", "[nnf][temporal]") {
    INFO("Formula: (p U q) && r");
    FormulaPool pool;
    pool.declare_variables({"p", "q", "r"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* r = pool.create_variable("r");
    Formula* until_pq = pool.create_until(p, q);
    Formula* and_until_r = pool.create_and(until_pq, r);
    Formula* result = and_until_r->nnf(pool);

    // Structure should be preserved
    REQUIRE(result->is_and());
    REQUIRE(result->left()->is_until());
}

//==============================================================================
// NNF: Idempotence
//==============================================================================

TEST_CASE("NNF: nnf(nnf(φ)) = nnf(φ)", "[nnf][idempotent]") {
    INFO("Formula: !!!(p && q)");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula* p = pool.create_variable("p");
    Formula* q = pool.create_variable("q");
    Formula* and_pq = pool.create_and(p, q);
    Formula* not_and = pool.create_not(and_pq);
    Formula* not_not_and = pool.create_not(not_and);
    Formula* not_not_not_and = pool.create_not(not_not_and);

    Formula* nnf1 = not_not_not_and->nnf(pool);
    Formula* nnf2 = nnf1->nnf(pool);

    // Second NNF should be equivalent to first
    REQUIRE(nnf2->to_string() == nnf1->to_string());
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
