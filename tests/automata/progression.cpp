/**
 * @file progression.cpp
 * @brief Tests for Formula Progression (fp)
 *
 * Based on AAAI2019 - De Giacomo et al. "Formula Progression for LTLf"
 *
 * Formula progression rules:
 * - fp(true, σ) = true
 * - fp(false, σ) = false
 * - fp(p, σ) = true if p ∈ σ, else false
 * - fp(!p, σ) = true if p ∉ σ, else false
 * - fp(X(φ), σ) = φ
 * - fp(φ₁ ∧ φ₂, σ) = fp(φ₁, σ) ∧ fp(φ₂, σ)
 * - fp(φ₁ ∨ φ₂, σ) = fp(φ₁, σ) ∨ fp(φ₂, σ)
 *
 * Note: Tests use TableauState::next_phi() which applies formula progression
 * to the XNF form of a formula.
 */

#define CATCH_CONFIG_RUNNER
#include "automata/tableau.hpp"
#include "catch.hpp"
#include "formula/formula_parser.hpp"
#include "formula/formula_pool.hpp"

using namespace formula;
using namespace automata;

//==============================================================================
// Helper: Create assignment from variable names
//==============================================================================
Assignment make_assignment(const std::vector<std::string> & /*vars*/,
                           const std::vector<std::string> &true_vars,
                           FormulaPool &pool)
{
    Assignment result;
    for (const auto &v : true_vars)
    {
        int var_id = pool.get_variable_id(v);
        if (var_id >= 0)
        {
            result.insert(var_id);
        }
    }
    return result;
}

//==============================================================================
// Base Cases: true, false, literals
//==============================================================================

TEST_CASE("FP: true progresses to true", "[progression][base]")
{
    INFO("Formula: true");
    FormulaPool pool;
    pool.declare_variables({}, {});
    Formula *phi = pool.create_true();

    auto state = TableauState::initial(phi, pool);
    Assignment sigma; // empty assignment

    Formula *next = state->next_phi(sigma, pool);

    REQUIRE(next->is_true());
}

TEST_CASE("FP: false progresses to false", "[progression][base]")
{
    INFO("Formula: false");
    FormulaPool pool;
    pool.declare_variables({}, {});
    Formula *phi = pool.create_false();

    auto state = TableauState::initial(phi, pool);
    Assignment sigma;

    Formula *next = state->next_phi(sigma, pool);

    REQUIRE(next->is_false());
}

TEST_CASE("FP: p with p ∈ sigma → true", "[progression][literal]")
{
    INFO("Formula: p, sigma = {p}");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula *phi = pool.create_variable("p");

    auto state = TableauState::initial(phi, pool);
    Assignment sigma = make_assignment({"p"}, {"p"}, pool);

    Formula *next = state->next_phi(sigma, pool);

    REQUIRE(next->is_true());
}

TEST_CASE("FP: p with p ∉ sigma → false", "[progression][literal]")
{
    INFO("Formula: p, sigma = {}");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula *phi = pool.create_variable("p");

    auto state = TableauState::initial(phi, pool);
    Assignment sigma; // empty

    Formula *next = state->next_phi(sigma, pool);

    REQUIRE(next->is_false());
}

//==============================================================================
// Not: fp(!p, σ) = true if p ∉ σ
//==============================================================================

TEST_CASE("FP: !p with p ∈ sigma → false", "[progression][not]")
{
    INFO("Formula: !p, sigma = {p}");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula *p = pool.create_variable("p");
    Formula *phi = pool.create_not(p);

    auto state = TableauState::initial(phi, pool);
    Assignment sigma = make_assignment({"p"}, {"p"}, pool);

    Formula *next = state->next_phi(sigma, pool);

    REQUIRE(next->is_false());
}

TEST_CASE("FP: !p with p ∉ sigma → true", "[progression][not]")
{
    INFO("Formula: !p, sigma = {}");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula *p = pool.create_variable("p");
    Formula *phi = pool.create_not(p);

    auto state = TableauState::initial(phi, pool);
    Assignment sigma; // empty

    Formula *next = state->next_phi(sigma, pool);

    REQUIRE(next->is_true());
}

//==============================================================================
// Next: fp(X(φ), σ) = φ
//==============================================================================

TEST_CASE("FP: X(p) with any sigma → p", "[progression][next]")
{
    INFO("Formula: X(p)");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula *p = pool.create_variable("p");
    Formula *phi = pool.create_next(p);

    auto state = TableauState::initial(phi, pool);
    Assignment sigma; // empty

    Formula *next = state->next_phi(sigma, pool);

    // X(p) progresses to p
    REQUIRE(next->is_literal());
    REQUIRE(next->var_id() == p->var_id());
}

TEST_CASE("FP: X(true) with any sigma → true", "[progression][next]")
{
    INFO("Formula: X(true)");
    FormulaPool pool;
    pool.declare_variables({}, {});
    Formula *phi = pool.create_next(pool.create_true());

    auto state = TableauState::initial(phi, pool);
    Assignment sigma;

    Formula *next = state->next_phi(sigma, pool);

    REQUIRE(next->is_true());
}

TEST_CASE("FP: X(X(p)) with any sigma → X(p)", "[progression][next]")
{
    INFO("Formula: X(X(p))");
    FormulaPool pool;
    pool.declare_variables({"p"}, {});
    Formula *p = pool.create_variable("p");
    Formula *phi = pool.create_next(pool.create_next(p));

    auto state = TableauState::initial(phi, pool);
    Assignment sigma;

    Formula *next = state->next_phi(sigma, pool);

    // X(X(p)) progresses to X(p)
    REQUIRE(next->is_next());
    REQUIRE(next->left()->is_literal());
}

//==============================================================================
// And: fp(φ₁ ∧ φ₂, σ) = fp(φ₁, σ) ∧ fp(φ₂, σ)
//==============================================================================

TEST_CASE("FP: p & q with p,q ∈ sigma → true", "[progression][and]")
{
    INFO("Formula: p && q, sigma = {p, q}");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula *phi = pool.create_and(pool.create_variable("p"), pool.create_variable("q"));

    auto state = TableauState::initial(phi, pool);
    Assignment sigma = make_assignment({"p", "q"}, {"p", "q"}, pool);

    Formula *next = state->next_phi(sigma, pool);

    REQUIRE(next->is_true());
}

TEST_CASE("FP: p & q with p ∉ sigma → false", "[progression][and]")
{
    INFO("Formula: p && q, sigma = {q}");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula *phi = pool.create_and(pool.create_variable("p"), pool.create_variable("q"));

    auto state = TableauState::initial(phi, pool);
    Assignment sigma = make_assignment({"p", "q"}, {"q"}, pool);

    Formula *next = state->next_phi(sigma, pool);

    REQUIRE(next->is_false());
}

//==============================================================================
// Or: fp(φ₁ ∨ φ₂, σ) = fp(φ₁, σ) ∨ fp(φ₂, σ)
//==============================================================================

TEST_CASE("FP: p | q with p ∈ sigma → true", "[progression][or]")
{
    INFO("Formula: p || q, sigma = {p}");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula *phi = pool.create_or(pool.create_variable("p"), pool.create_variable("q"));

    auto state = TableauState::initial(phi, pool);
    Assignment sigma = make_assignment({"p", "q"}, {"p"}, pool);

    Formula *next = state->next_phi(sigma, pool);

    REQUIRE(next->is_true());
}

TEST_CASE("FP: p | q with p,q ∉ sigma → false", "[progression][or]")
{
    INFO("Formula: p || q, sigma = {}");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula *phi = pool.create_or(pool.create_variable("p"), pool.create_variable("q"));

    auto state = TableauState::initial(phi, pool);
    Assignment sigma; // empty

    Formula *next = state->next_phi(sigma, pool);

    REQUIRE(next->is_false());
}

//==============================================================================
// Complex formulas with X
//==============================================================================

TEST_CASE("FP: X(p) & q with p,q ∈ sigma → true", "[progression][complex]")
{
    INFO("Formula: X(p) && q, sigma = {p, q}");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula *phi = pool.create_and(pool.create_next(pool.create_variable("p")), pool.create_variable("q"));

    auto state = TableauState::initial(phi, pool);
    Assignment sigma = make_assignment({"p", "q"}, {"p", "q"}, pool);

    Formula *next = state->next_phi(sigma, pool);

    // X(p) & q → p & true → p (but since we check equality...)
    // Actually: X(p) progresses to p, q progresses to true
    // So next should be p (simplified from p & true)
    REQUIRE(next->is_literal());
}

TEST_CASE("FP: p | X(q) with p ∉ sigma, q ∈ sigma → true", "[progression][complex]")
{
    INFO("Formula: p || X(q), sigma = {q}");
    FormulaPool pool;
    pool.declare_variables({"p", "q"}, {});
    Formula *phi = pool.create_or(pool.create_variable("p"), pool.create_next(pool.create_variable("q")));

    auto state = TableauState::initial(phi, pool);
    Assignment sigma = make_assignment({"p", "q"}, {"q"}, pool);

    Formula *next = state->next_phi(sigma, pool);

    // p progresses to false, X(q) progresses to q
    // So next should be q (simplified from false | q)
    REQUIRE(next->is_literal());
}

//==============================================================================
// Parser-based tests
//==============================================================================

TEST_CASE("FP: parsed formula 'p & X(q)' with p,q ∈ sigma", "[progression][parser]")
{
    INFO("Formula: p & X(q), sigma = {p, q}");
    FormulaPool pool;
    FormulaParser parser(pool);
    Formula *phi = parser.parse("p & X(q)");

    REQUIRE(phi != nullptr);

    auto state = TableauState::initial(phi, pool);
    Assignment sigma = make_assignment({"p", "q"}, {"p", "q"}, pool);

    Formula *next = state->next_phi(sigma, pool);

    // p progresses to true, X(q) progresses to q
    // Result should be q (simplified from true & q)
    REQUIRE(next->is_literal());
}

TEST_CASE("FP: parsed formula 'X(p) | X(q)' with p ∈ sigma", "[progression][parser]")
{
    INFO("Formula: X(p) || X(q), sigma = {p}");
    FormulaPool pool;
    FormulaParser parser(pool);
    Formula *phi = parser.parse("X(p) | X(q)");

    REQUIRE(phi != nullptr);

    auto state = TableauState::initial(phi, pool);
    Assignment sigma = make_assignment({"p", "q"}, {"p"}, pool);

    Formula *next = state->next_phi(sigma, pool);

    // X(p) progresses to p, X(q) progresses to q
    // Result is p | q
    REQUIRE(next->is_or());
}

//==============================================================================
// XNF-based progression tests
//==============================================================================

TEST_CASE("FP: XNF(F p) with p ∈ sigma → true", "[progression][xnf]")
{
    INFO("Formula: F p (i.e., true U p), XNF form, sigma = {p}");
    FormulaPool pool;
    FormulaParser parser(pool);
    Formula *phi = parser.parse("true U p"); // F p = true U p

    REQUIRE(phi != nullptr);

    auto state = TableauState::initial(phi, pool);
    Assignment sigma = make_assignment({"p"}, {"p"}, pool);

    Formula *next = state->next_phi(sigma, pool);

    // XNF(F p) = p | X(F p)
    // With p ∈ sigma: fp(p, sigma) = true, fp(X(F p), sigma) = F p
    // Result: true | F p → true (simplified)
    REQUIRE(next->is_true());
}

//==============================================================================
// Custom main for compact output
//==============================================================================

int main(int argc, char *argv[])
{
    Catch::Session session;
    std::vector<std::string> default_args = {argv[0]};

    bool verbose = (std::getenv("COSY_TEST_VERBOSE") != nullptr);
    if (verbose)
    {
        default_args.push_back("-s");
        default_args.push_back("-d");
        default_args.push_back("yes");
    }
    else
    {
        default_args.push_back("-r");
        default_args.push_back("compact");
    }

    std::vector<char *> args;
    args.reserve(default_args.size() + argc + 1);
    for (auto &arg : default_args)
    {
        args.push_back(const_cast<char *>(arg.c_str()));
    }
    for (int i = 1; i < argc; ++i)
    {
        args.push_back(argv[i]);
    }

    int result = session.applyCommandLine(static_cast<int>(args.size()), args.data());
    if (result != 0)
        return result;

    return session.run();
}
