/**
 * @file io_separation_test.cpp
 * @brief Test input/output variable separation
 */

#include "synthesis/on_the_fly_solver.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"
#include <iostream>
#include <cassert>
#include <fstream>

using namespace formula;
using namespace synthesis;

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

#define TEST(name) void test_##name()
#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        std::cerr << "FAILED: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return; \
    } \
} while(0)
#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))
#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        std::cerr << "FAILED: " << #a << " == " << #b << " (" << (a) << " vs " << (b) << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return; \
    } \
} while(0)

//==============================================================================
// Partition File Tests
//==============================================================================

TEST(part_file_parsing) {
    FormulaPool pool;

    // Create a temporary .part file
    std::string part_file = "/tmp/test_io.part";
    std::ofstream f(part_file);
    f << ".inputs: p0 p1" << std::endl;
    f << ".outputs: p2 p3" << std::endl;
    f.close();

    pool.load_from_partition(part_file);

    ASSERT_EQ(pool.num_inputs(), 2);
    ASSERT_EQ(pool.num_outputs(), 2);
    ASSERT_EQ(pool.num_variables(), 4);

    // Check variable IDs
    ASSERT_EQ(pool.get_variable_id("p0"), 2);  // First input, after outputs
    ASSERT_EQ(pool.get_variable_id("p1"), 3);  // Second input
    ASSERT_EQ(pool.get_variable_id("p2"), 0);  // First output
    ASSERT_EQ(pool.get_variable_id("p3"), 1);  // Second output

    // Check type checks
    ASSERT_TRUE(pool.is_output_variable(0));
    ASSERT_TRUE(pool.is_output_variable(1));
    ASSERT_TRUE(pool.is_input_variable(2));
    ASSERT_TRUE(pool.is_input_variable(3));

    std::cout << "PASS: part_file_parsing" << std::endl;
}

TEST(part_file_synthesis_simple) {
    // Test with simple formula: p2 (output must be true)
    FormulaPool pool;

    std::string part_file = "/tmp/test_io2.part";
    std::ofstream f(part_file);
    f << ".inputs: p0" << std::endl;
    f << ".outputs: p2" << std::endl;
    f.close();

    pool.load_from_partition(part_file);

    // Formula: p2 (output must be true)
    Formula* p2 = pool.create_variable("p2");
    bool result = check_realizable(p2, pool);

    ASSERT_TRUE(result);  // System can set p2 = true
    std::cout << "PASS: part_file_synthesis_simple" << std::endl;
}

TEST(part_file_synthesis_response) {
    // Test implies formula: p2 -> p3 = !p2 | p3
    // Environment controls p2 (input), system controls p3 (output)
    // Verified with Cosy reference: Realizable

    FormulaPool pool;

    std::string part_file = "/tmp/test_io3.part";
    std::ofstream f(part_file);
    f << ".inputs: p0 p2" << std::endl;   // p2 is input (request)
    f << ".outputs: p1 p3" << std::endl;  // p3 is output (response)
    f.close();

    pool.load_from_partition(part_file);

    // p2 -> p3 = !p2 | p3
    Formula* p2 = pool.create_variable("p2");  // Input
    Formula* p3 = pool.create_variable("p3");  // Output
    Formula* not_p2 = pool.create_not(p2);
    Formula* implies = pool.create_or(not_p2, p3);  // !p2 | p3

    bool result = check_realizable(implies, pool);

    ASSERT_TRUE(result);  // Realizable: verified with Cosy reference
    std::cout << "PASS: part_file_synthesis_response" << std::endl;
}

//==============================================================================
// Variable ID Assignment Tests
//==============================================================================

TEST(variable_id_ordering) {
    FormulaPool pool;

    // Declare outputs first, then inputs
    std::vector<std::string> outputs = {"a", "b", "c"};
    std::vector<std::string> inputs = {"x", "y"};

    pool.declare_variables(outputs, inputs);

    // Outputs should get IDs 0, 1, 2
    ASSERT_EQ(pool.get_variable_id("a"), 0);
    ASSERT_EQ(pool.get_variable_id("b"), 1);
    ASSERT_EQ(pool.get_variable_id("c"), 2);

    // Inputs should get IDs 3, 4
    ASSERT_EQ(pool.get_variable_id("x"), 3);
    ASSERT_EQ(pool.get_variable_id("y"), 4);

    ASSERT_TRUE(pool.is_output_variable(0));
    ASSERT_TRUE(pool.is_output_variable(1));
    ASSERT_TRUE(pool.is_output_variable(2));
    ASSERT_TRUE(pool.is_input_variable(3));
    ASSERT_TRUE(pool.is_input_variable(4));

    std::cout << "PASS: variable_id_ordering" << std::endl;
}

//==============================================================================
// Main
//==============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "I/O Separation Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    int passed = 0;
    int total = 0;

    #define RUN_TEST(name) do { \
        total++; \
        try { \
            test_##name(); \
            passed++; \
        } catch (...) { \
            std::cerr << "EXCEPTION in test_" << #name << std::endl; \
        } \
    } while(0)

    RUN_TEST(part_file_parsing);
    RUN_TEST(part_file_synthesis_simple);
    RUN_TEST(part_file_synthesis_response);
    RUN_TEST(variable_id_ordering);

    std::cout << "========================================" << std::endl;
    std::cout << "Results: " << passed << "/" << total << " tests passed" << std::endl;
    std::cout << "========================================" << std::endl;

    return (passed == total) ? 0 : 1;
}
