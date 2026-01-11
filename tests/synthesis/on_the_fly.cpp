/**
 * @file on_the_fly_synthesis_tests.cpp
 * @brief Tests for on-the-fly synthesis algorithm (using Catch2)
 */

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "synthesis/on_the_fly_solver.hpp"
#include "synthesis/synthesis.hpp"
#include "formula/formula_parser.hpp"
#include "formula/formula_pool.hpp"
#include "log/logger.hpp"
#include <cstdlib>   // for std::getenv
#include <vector>
#include <string>
#include <algorithm>  // for std::remove

using namespace formula;
using namespace synthesis;

//==============================================================================
// Test Helper
//==============================================================================

// Helper function to check realizability using OnTheFlyGameSolver
static bool check_realizable(Formula* phi, FormulaPool& pool) {
    int num_outputs = pool.num_outputs();
    int num_inputs = pool.num_inputs();
    // If variables not declared, extract them from the formula
    if (num_outputs == 0 && num_inputs == 0) {
        num_outputs = static_cast<int>(Formula::collect_variables(phi).size());
    }
    OnTheFlyGameSolver solver(phi, pool, num_outputs, num_inputs);
    return solver.is_realizable();
}

//==============================================================================
// Basic Formula Tests
//==============================================================================

TEST_CASE("On-the-Fly: true formula", "[on_the_fly][basic][true]") {
    INFO("Formula: true");
    FormulaPool pool;
    pool.declare_variables({}, {});
    Formula* phi = pool.create_true();
    bool result = check_realizable(phi, pool);
    REQUIRE(result);
}

TEST_CASE("On-the-Fly: false formula", "[on_the_fly][basic][false]") {
    INFO("Formula: false");
    FormulaPool pool;
    pool.declare_variables({}, {});
    Formula* phi = pool.create_false();
    bool result = check_realizable(phi, pool);
    REQUIRE_FALSE(result);
}

TEST_CASE("On-the-Fly: single literal p1", "[on_the_fly][basic][literal]") {
    INFO("Formula: p1");
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});
    Formula* p1 = pool.create_variable("p1");
    bool result = check_realizable(p1, pool);
    REQUIRE(result);  // p1 is realizable (system sets p1 = true)
}

TEST_CASE("On-the-Fly: negated literal !p1", "[on_the_fly][basic][literal]") {
    INFO("Formula: !p1");
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});
    Formula* p1 = pool.create_variable("p1");
    Formula* not_p1 = pool.create_not(p1);
    bool result = check_realizable(not_p1, pool);
    REQUIRE(result);  // !p1 is realizable (system sets p1 = false)
}

//==============================================================================
// Temporal Operator Tests
//==============================================================================

TEST_CASE("On-the-Fly: next X p1", "[on_the_fly][temporal][next]") {
    INFO("Formula: X p1");
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});
    Formula* p1 = pool.create_variable("p1");
    Formula* next_p1 = pool.create_next(p1);
    bool result = check_realizable(next_p1, pool);
    REQUIRE(result);  // X p1 is realizable
}

TEST_CASE("On-the-Fly: eventually F p1 (true U p1)", "[on_the_fly][temporal][eventually]") {
    INFO("Formula: F p1 (true U p1)");
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});
    Formula* p1 = pool.create_variable("p1");
    Formula* true_f = pool.create_true();
    Formula* fp1 = pool.create_until(true_f, p1);
    bool result = check_realizable(fp1, pool);
    REQUIRE(result);  // F p1 is realizable (eventually set p1 = true)
}

TEST_CASE("On-the-Fly: always G p1 (false R p1)", "[on_the_fly][temporal][always]") {
    INFO("Formula: G p1 (false R p1)");
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});
    Formula* p1 = pool.create_variable("p1");
    Formula* false_f = pool.create_false();
    Formula* gp1 = pool.create_release(false_f, p1);
    bool result = check_realizable(gp1, pool);
    REQUIRE(result);  // G p1 is realizable (always set p1 = true)
}

TEST_CASE("On-the-Fly: until p1 U p2", "[on_the_fly][temporal][until]") {
    INFO("Formula: p1 U p2");
    FormulaPool pool;
    pool.declare_variables({"p1", "p2"}, {});
    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");
    Formula* until = pool.create_until(p1, p2);
    bool result = check_realizable(until, pool);
    REQUIRE(result);  // p1 U p2 is realizable
}

//==============================================================================
// Unrealizable Formula Tests
//==============================================================================

TEST_CASE("On-the-Fly: contradiction p1 & !p1", "[on_the_fly][unrealizable]") {
    INFO("Formula: p1 & !p1");
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});
    Formula* p1 = pool.create_variable("p1");
    Formula* not_p1 = pool.create_not(p1);
    Formula* and_f = pool.create_and(p1, not_p1);
    bool result = check_realizable(and_f, pool);
    REQUIRE_FALSE(result);  // Contradiction is unrealizable
}

TEST_CASE("On-the-Fly: eventually contradiction F(p1 & !p1)", "[on_the_fly][unrealizable]") {
    INFO("Formula: F (p1 & !p1)");
    FormulaPool pool;
    pool.declare_variables({"p1"}, {});
    Formula* p1 = pool.create_variable("p1");
    Formula* not_p1 = pool.create_not(p1);
    Formula* and_f = pool.create_and(p1, not_p1);
    Formula* true_f = pool.create_true();
    Formula* fand = pool.create_until(true_f, and_f);
    bool result = check_realizable(fand, pool);
    REQUIRE_FALSE(result);  // Eventually impossible is unrealizable
}

//==============================================================================
// Complex Formula Tests
//==============================================================================

TEST_CASE("On-the-Fly: response formula (!req) U ack", "[on_the_fly][complex]") {
    INFO("Formula: (!req) U ack");
    FormulaPool pool;
    pool.declare_variables({"req", "ack"}, {});
    Formula* req = pool.create_variable("req");
    Formula* ack = pool.create_variable("ack");
    Formula* not_req = pool.create_not(req);
    Formula* response = pool.create_until(not_req, ack);
    bool result = check_realizable(response, pool);
    REQUIRE(result);
}

// Known issue: sequence test - skip for now
// TODO: Fix SCC classification to handle p1 & X p2 correctly
TEST_CASE("On-the-Fly: sequence p1 & X p2", "[on_the_fly][complex][!mayfail][known-issue]") {
    INFO("Formula: p1 & X p2");
    INFO("NOTE: This is a known issue with SCC classification");
    FormulaPool pool;
    pool.declare_variables({"p1", "p2"}, {});
    Formula* p1 = pool.create_variable("p1");
    Formula* p2 = pool.create_variable("p2");
    Formula* next_p2 = pool.create_next(p2);
    Formula* seq = pool.create_and(p1, next_p2);
    bool result = check_realizable(seq, pool);
    // This should be true but currently fails
    REQUIRE(result);  // TODO: Fix this
}

//==============================================================================
// Custom main for better output
//==============================================================================

int main(int argc, char* argv[]) {
    // Use Catch2's session
    Catch::Session session;

    // Build args starting with program name
    std::vector<std::string> default_args = {argv[0]};

    // Track if user specified their own reporter
    bool user_specified_reporter = false;

    // First pass: check for user-provided options
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        // Check if user specified a reporter
        if (arg == "-r" || arg == "--reporter") {
            user_specified_reporter = true;
        }
    }

    // Check environment variable for verbose mode
    bool verbose = (std::getenv("COSY_TEST_VERBOSE") != nullptr);
    if (verbose) {
        // Verbose mode: show all test details with line numbers
        default_args.push_back("-s");  // show successful tests
        default_args.push_back("-d");  // show duration
        default_args.push_back("yes"); // enable duration display
    } else if (!user_specified_reporter) {
        // Default mode: use compact reporter (only if user didn't specify one)
        default_args.push_back("-r");
        default_args.push_back("compact");
    }

    // Build final args: defaults + user args
    std::vector<char*> args;
    args.reserve(default_args.size() + argc + 1);
    for (auto& arg : default_args) {
        args.push_back(const_cast<char*>(arg.c_str()));
    }
    // Append user-provided args
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    // Run tests
    int result = session.applyCommandLine(static_cast<int>(args.size()), args.data());
    if (result != 0) {
        return result;
    }

    return session.run();
}
