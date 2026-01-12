/**
 * @file io_separation_test.cpp
 * @brief Test input/output variable separation
 */

#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "formula/formula_parser.hpp"
#include "formula/formula_pool.hpp"
#include "synthesis/on_the_fly_solver.hpp"

#include <fstream>

using namespace formula;
using namespace synthesis;

// Helper function to check realizability using OnTheFlyGameSolver
static bool check_realizable(Formula *phi, FormulaPool &pool)
{
    OnTheFlyGameSolver solver(phi, pool);
    return solver.is_realizable();
}

//==============================================================================
// Partition File Tests
//==============================================================================

TEST_CASE("Partition file parsing", "[io_separation]")
{
    FormulaPool pool;

    // Create a temporary .part file
    std::string part_file = "/tmp/test_io.part";
    std::ofstream f(part_file);
    f << ".inputs: p0 p1" << std::endl;
    f << ".outputs: p2 p3" << std::endl;
    f.close();

    pool.load_from_partition(part_file);

    REQUIRE(pool.num_inputs() == 2);
    REQUIRE(pool.num_outputs() == 2);
    REQUIRE(pool.num_variables() == 4);

    // Check variable IDs
    REQUIRE(pool.get_variable_id("p0") == 2); // First input, after outputs
    REQUIRE(pool.get_variable_id("p1") == 3); // Second input
    REQUIRE(pool.get_variable_id("p2") == 0); // First output
    REQUIRE(pool.get_variable_id("p3") == 1); // Second output

    // Check type checks
    REQUIRE(pool.is_output_variable(0));
    REQUIRE(pool.is_output_variable(1));
    REQUIRE(pool.is_input_variable(2));
    REQUIRE(pool.is_input_variable(3));
}

TEST_CASE("Partition file synthesis simple", "[io_separation]")
{
    // Test with simple formula: p2 (output must be true)
    FormulaPool pool;

    std::string part_file = "/tmp/test_io2.part";
    std::ofstream f(part_file);
    f << ".inputs: p0" << std::endl;
    f << ".outputs: p2" << std::endl;
    f.close();

    pool.load_from_partition(part_file);

    // Formula: p2 (output must be true)
    Formula *p2 = pool.create_variable("p2");
    bool result = check_realizable(p2, pool);

    REQUIRE(result); // System can set p2 = true
}

TEST_CASE("Partition file synthesis response", "[io_separation]")
{
    // Test implies formula: p2 -> p3 = !p2 | p3
    // Environment controls p2 (input), system controls p3 (output)
    // Verified with Cosy reference: Realizable

    FormulaPool pool;

    std::string part_file = "/tmp/test_io3.part";
    std::ofstream f(part_file);
    f << ".inputs: p0 p2" << std::endl;  // p2 is input (request)
    f << ".outputs: p1 p3" << std::endl; // p3 is output (response)
    f.close();

    pool.load_from_partition(part_file);

    // p2 -> p3 = !p2 | p3
    Formula *p2 = pool.create_variable("p2"); // Input
    Formula *p3 = pool.create_variable("p3"); // Output
    Formula *not_p2 = pool.create_not(p2);
    Formula *implies = pool.create_or(not_p2, p3); // !p2 | p3

    bool result = check_realizable(implies, pool);

    REQUIRE(result); // Realizable: verified with Cosy reference
}

//==============================================================================
// Variable ID Assignment Tests
//==============================================================================

TEST_CASE("Variable ID ordering", "[io_separation]")
{
    FormulaPool pool;

    // Declare outputs first, then inputs
    std::vector<std::string> outputs = {"a", "b", "c"};
    std::vector<std::string> inputs = {"x", "y"};

    pool.declare_variables(outputs, inputs);

    // Outputs should get IDs 0, 1, 2
    REQUIRE(pool.get_variable_id("a") == 0);
    REQUIRE(pool.get_variable_id("b") == 1);
    REQUIRE(pool.get_variable_id("c") == 2);

    // Inputs should get IDs 3, 4
    REQUIRE(pool.get_variable_id("x") == 3);
    REQUIRE(pool.get_variable_id("y") == 4);

    REQUIRE(pool.is_output_variable(0));
    REQUIRE(pool.is_output_variable(1));
    REQUIRE(pool.is_output_variable(2));
    REQUIRE(pool.is_input_variable(3));
    REQUIRE(pool.is_input_variable(4));
}
