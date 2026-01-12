/**
 * Transformation Equivalence Tests
 * ================================
 *
 * Tests that verify formula transformations preserve semantics:
 * 1. Parse → to_string → re-parse should be equivalent
 * 2. NNF transformation should be equivalent to original
 * 3. XNF transformation should be equivalent to original
 *
 * Uses FormulaChecker for equivalence checking.
 * Failed cases are logged to logs/ with full context.
 *
 * Author: Claude Code
 * Date: 2026-01-02
 */

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "formula/formula.hpp"
#include "formula/formula_checker.hpp"
#include "formula/formula_parser.hpp"
#include "formula/formula_pool.hpp"
#include "log/logger.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace formula;
using namespace std::chrono;

// ========== Global State ==========
struct TestStats {
    int total = 0;
    int passed = 0;
    int failed = 0;
    double total_time_ms = 0;
};

TestStats stats;
std::ofstream failure_log;
std::ofstream summary_log;
steady_clock::time_point test_start_time;

// ========== Helper Functions ==========
std::string get_timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto ms_count = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&timer);

    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    std::snprintf(buf + std::strlen(buf), sizeof(buf) - std::strlen(buf), ".%03d", static_cast<int>(ms_count.count()));
    return std::string(buf);
}

// Find project root directory (containing CMakeLists.txt)
std::string find_project_root()
{
    std::string current = ".";
    for (int i = 0; i < 5; ++i)
    {
        std::string cmake_path = current + "/CMakeLists.txt";
        std::ifstream f(cmake_path);
        if (f.good())
        {
            return current == "." ? "" : current;
        }
        current = "../" + current;
    }
    return "";
}

void log_failure(const std::string &test_name,
                 const std::string &formula_str,
                 const std::string &transformed_str,
                 const std::string &expected,
                 const std::string &actual,
                 double elapsed_ms)
{
    if (failure_log.is_open())
    {
        failure_log << "========== FAILURE ==========\n";
        failure_log << "Timestamp: " << get_timestamp() << "\n";
        failure_log << "Test Name: " << test_name << "\n";
        failure_log << "Expected: " << expected << "\n";
        failure_log << "Actual:   " << actual << "\n";
        failure_log << "Elapsed:  " << std::fixed << std::setprecision(2) << elapsed_ms << "ms\n";
        failure_log << "\n--- Original Formula ---\n";
        failure_log << formula_str << "\n";
        failure_log << "\n--- Transformed Formula ---\n";
        failure_log << transformed_str << "\n";
        failure_log << "================================\n\n";
        failure_log.flush();
    }
}

void log_test_start(const std::string &test_name)
{
    LOG_INFO("[{}] Starting...", test_name);
}

void log_test_result(const std::string &test_name, bool passed, double elapsed_ms)
{
    if (passed)
    {
        LOG_INFO("[{}] PASSED ({:.2f}ms)", test_name, elapsed_ms);
    }
    else
    {
        LOG_WARN("[{}] FAILED ({:.2f}ms)", test_name, elapsed_ms);
    }
}

// ========== Test Cases ==========
std::vector<std::string> generate_test_formulas()
{
    return {
        // Simple literals
        "p1",
        "!p1",

        // Propositional combinations
        "p1 & p2",
        "p1 | p2",
        "(p1 & p2) | p3",
        "(p1 | p2) & p3",
        "p1 & p2 & p3 & p4",
        "p1 | p2 | p3 | p4",

        // Negation
        "!(p1 & p2)",
        "!(p1 | p2)",
        "!(!p1)",
        "!(!(!p1))",
        "!!p1",

        // Next operator
        "X(p1)",
        "X(X(p1))",
        "X(p1 & p2)",
        "X(p1) | X(p2)",
        "X(X(X(p1)))",
        "X(true)",
        "X(false)",

        // Until operator
        "p1 U p2",
        "(p1 | p2) U p3",
        "p1 U (p2 & p3)",
        "true U p1",
        "p1 U true",
        "p1 U false",

        // Release operator
        "p1 R p2",
        "(p1 | p2) R p3",
        "p1 R (p2 & p3)",
        "false R p1",

        // Complex formulas
        "(p1 U p2) & (p3 U p4)",
        "X(p1) U p2",
        "p1 U X(p2)",
        "!(p1 U p2)",
        "!X(p1)",
        "!(p1 R p2)",

        // Nested structures
        "((p1 & p2) | (p3 & p4)) U p5",
        "X((p1 | p2) & p3)",
        "(p1 U p2) U p3",
        "(p1 R p2) R p3",

        // De Morgan candidates
        "!((p1 & p2) | (p3 & p4))",
        "!((p1 | p2) & (p3 | p4))",
        "!(p1 & p2 & p3)",

        // Edge cases
        "true",
        "false",
        "!true",
        "!false",
        "true & false",
        "true | false",
    };
}

// ========== Test 1: Parse → to_string → Re-parse Equivalence ==========
TEST_CASE("Transformation: Parse → to_string → Re-parse", "[transformation][string_roundtrip]")
{
    FormulaPool pool;
    FormulaParser parser(pool);

    log_test_start("String Roundtrip (verbose)");

    int passed = 0;
    int failed = 0;
    double total_time = 0;

    for (const auto &formula_str : generate_test_formulas())
    {
        stats.total++;

        auto start = steady_clock::now();

        try
        {
            // Parse original formula
            Formula *f1 = parser.parse(formula_str);

            // Convert to string with variable names and parse again
            std::string f1_str = f1->to_string(pool);
            Formula *f2 = parser.parse(f1_str);

            // Check equivalence using FormulaChecker
            bool result = FormulaChecker::are_equivalent(pool, f1, f2);

            auto end = steady_clock::now();
            double elapsed = duration<double, std::milli>(end - start).count();
            total_time += elapsed;
            stats.total_time_ms += elapsed;

            if (result)
            {
                passed++;
                stats.passed++;
                LOG_TRACE("PASS: {} → {} ({}ms)", formula_str, f1_str, elapsed);
            }
            else
            {
                failed++;
                stats.failed++;
                LOG_DEBUG("FAIL: {} → {} (NOT EQUIV) ({}ms)", formula_str, f1_str, elapsed);
                log_failure("StringRoundtrip", formula_str, f1_str, "equivalent", "not-equivalent", elapsed);
            }
        }
        catch (const std::exception &e)
        {
            failed++;
            stats.failed++;
            auto end = steady_clock::now();
            double elapsed = duration<double, std::milli>(end - start).count();
            LOG_DEBUG("ERROR: {} - {} ({}ms)", formula_str, e.what(), elapsed);
            log_failure("StringRoundtrip", formula_str, e.what(), "no error", "exception", elapsed);
        }
    }

    log_test_result("String Roundtrip (verbose)", failed == 0, total_time);

    // Summary to log file
    if (summary_log.is_open())
    {
        summary_log << "\n=== String Roundtrip Test ===\n";
        summary_log << "Total: " << (passed + failed) << ", Passed: " << passed << ", Failed: " << failed << "\n";
        summary_log << "Time: " << total_time << "ms\n";
        summary_log.flush();
    }

    REQUIRE(failed == 0);
}

// ========== Test 2: NNF Transformation Equivalence ==========
TEST_CASE("Transformation: NNF preserves semantics", "[transformation][nnf]")
{
    FormulaPool pool;
    FormulaParser parser(pool);

    log_test_start("NNF Transformation");

    int passed = 0;
    int failed = 0;
    double total_time = 0;

    for (const auto &formula_str : generate_test_formulas())
    {
        stats.total++;

        auto start = steady_clock::now();

        try
        {
            // Parse original formula
            Formula *f1 = parser.parse(formula_str);

            // Transform to NNF
            Formula *f2 = f1->nnf(pool);

            // Check equivalence using FormulaChecker
            bool result = FormulaChecker::are_equivalent(pool, f1, f2);

            auto end = steady_clock::now();
            double elapsed = duration<double, std::milli>(end - start).count();
            total_time += elapsed;
            stats.total_time_ms += elapsed;

            if (result)
            {
                passed++;
                stats.passed++;
                LOG_TRACE("PASS: {} → NNF ({}ms)", formula_str, elapsed);
            }
            else
            {
                failed++;
                stats.failed++;
                LOG_DEBUG("FAIL: {} → NNF (NOT EQUIV) ({}ms)", formula_str, elapsed);
                log_failure("NNF_Transform", formula_str, f2->to_string(pool), "equivalent", "not-equivalent", elapsed);
            }
        }
        catch (const std::exception &e)
        {
            failed++;
            stats.failed++;
            auto end = steady_clock::now();
            double elapsed = duration<double, std::milli>(end - start).count();
            LOG_DEBUG("ERROR: {} - {} ({}ms)", formula_str, e.what(), elapsed);
            log_failure("NNF_Transform", formula_str, e.what(), "no error", "exception", elapsed);
        }
    }

    log_test_result("NNF Transformation", failed == 0, total_time);

    if (summary_log.is_open())
    {
        summary_log << "\n=== NNF Transformation Test ===\n";
        summary_log << "Total: " << (passed + failed) << ", Passed: " << passed << ", Failed: " << failed << "\n";
        summary_log << "Time: " << total_time << "ms\n";
        summary_log.flush();
    }

    REQUIRE(failed == 0);
}

// ========== Test 3: XNF Transformation Equivalence ==========
TEST_CASE("Transformation: XNF preserves semantics", "[transformation][xnf]")
{
    FormulaPool pool;
    FormulaParser parser(pool);

    log_test_start("XNF Transformation");

    int passed = 0;
    int failed = 0;
    double total_time = 0;

    for (const auto &formula_str : generate_test_formulas())
    {
        stats.total++;

        auto start = steady_clock::now();

        try
        {
            // Parse original formula
            Formula *f1 = parser.parse(formula_str);

            // Transform to XNF
            Formula *f2 = f1->xnf_with_end_marker(pool);

            // Check equivalence using FormulaChecker
            bool result = FormulaChecker::are_equivalent(pool, f1, f2);

            auto end = steady_clock::now();
            double elapsed = duration<double, std::milli>(end - start).count();
            total_time += elapsed;
            stats.total_time_ms += elapsed;

            if (result)
            {
                passed++;
                stats.passed++;
                LOG_TRACE("PASS: {} → XNF ({}ms)", formula_str, elapsed);
            }
            else
            {
                failed++;
                stats.failed++;
                LOG_DEBUG("FAIL: {} → XNF (NOT EQUIV) ({}ms)", formula_str, elapsed);
                log_failure("XNF_Transform", formula_str, f2->to_string(pool), "equivalent", "not-equivalent", elapsed);
            }
        }
        catch (const std::exception &e)
        {
            failed++;
            stats.failed++;
            auto end = steady_clock::now();
            double elapsed = duration<double, std::milli>(end - start).count();
            LOG_DEBUG("ERROR: {} - {} ({}ms)", formula_str, e.what(), elapsed);
            log_failure("XNF_Transform", formula_str, e.what(), "no error", "exception", elapsed);
        }
    }

    log_test_result("XNF Transformation", failed == 0, total_time);

    if (summary_log.is_open())
    {
        summary_log << "\n=== XNF Transformation Test ===\n";
        summary_log << "Total: " << (passed + failed) << ", Passed: " << passed << ", Failed: " << failed << "\n";
        summary_log << "Time: " << total_time << "ms\n";
        summary_log.flush();
    }

    REQUIRE(failed == 0);
}

// ========== Test 4: Full Pipeline Equivalence ==========
TEST_CASE("Transformation: Full pipeline (NNF → Simplify → XNF)", "[transformation][pipeline]")
{
    FormulaPool pool;
    FormulaParser parser(pool);

    log_test_start("Full Pipeline");

    int passed = 0;
    int failed = 0;
    double total_time = 0;

    for (const auto &formula_str : generate_test_formulas())
    {
        stats.total++;

        auto start = steady_clock::now();

        try
        {
            // Parse original formula
            Formula *f1 = parser.parse(formula_str);

            // Full transformation pipeline
            Formula *f_nnf = f1->nnf(pool);
            Formula *f_simp = f_nnf->simplify(pool);
            Formula *f_xnf = f_simp->xnf_with_end_marker(pool);

            // Check equivalence using FormulaChecker
            bool result = FormulaChecker::are_equivalent(pool, f1, f_xnf);

            auto end = steady_clock::now();
            double elapsed = duration<double, std::milli>(end - start).count();
            total_time += elapsed;
            stats.total_time_ms += elapsed;

            if (result)
            {
                passed++;
                stats.passed++;
                LOG_TRACE("PASS: {} → Full Pipeline ({}ms)", formula_str, elapsed);
            }
            else
            {
                failed++;
                stats.failed++;
                LOG_DEBUG("FAIL: {} → Full Pipeline (NOT EQUIV) ({}ms)", formula_str, elapsed);
                std::string pipeline_desc = "NNF: " + f_nnf->to_string(pool) + "\nSimplify: " + f_simp->to_string(pool)
                                            + "\nXNF: " + f_xnf->to_string(pool);
                log_failure("Full_Pipeline", formula_str, pipeline_desc, "equivalent", "not-equivalent", elapsed);
            }
        }
        catch (const std::exception &e)
        {
            failed++;
            stats.failed++;
            auto end = steady_clock::now();
            double elapsed = duration<double, std::milli>(end - start).count();
            LOG_DEBUG("ERROR: {} - {} ({}ms)", formula_str, e.what(), elapsed);
            log_failure("Full_Pipeline", formula_str, e.what(), "no error", "exception", elapsed);
        }
    }

    log_test_result("Full Pipeline", failed == 0, total_time);

    if (summary_log.is_open())
    {
        summary_log << "\n=== Full Pipeline Test ===\n";
        summary_log << "Total: " << (passed + failed) << ", Passed: " << passed << ", Failed: " << failed << "\n";
        summary_log << "Time: " << total_time << "ms\n";
        summary_log.flush();
    }

    REQUIRE(failed == 0);
}

// ========== Main ==========
int main(int argc, char *argv[])
{
    test_start_time = steady_clock::now();

    // Find project root and set up log directory
    std::string project_root = find_project_root();
    std::string log_dir = project_root.empty() ? "logs" : project_root + "/logs";
    std::filesystem::create_directories(log_dir);

    // Initialize logger
    LOG_INFO("Transformation Equivalence Tests starting...");
    LOG_INFO("Log directory: {}", log_dir);

    // Open log files
    char time_buf[64];
    auto now = std::chrono::system_clock::now();
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&timer);
    std::strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", &tm);

    std::string failure_log_path = log_dir + "/transform_failures_" + std::string(time_buf) + ".log";
    std::string summary_log_path = log_dir + "/transform_summary_" + std::string(time_buf) + ".log";

    failure_log.open(failure_log_path);
    if (failure_log.is_open())
    {
        failure_log << "Transformation Equivalence Tests - Failure Log\n";
        failure_log << "Start time: " << get_timestamp() << "\n\n";
        std::cout << "Failure log: " << failure_log_path << "\n";
    }

    summary_log.open(summary_log_path);
    if (summary_log.is_open())
    {
        summary_log << "Transformation Equivalence Tests - Summary\n";
        summary_log << "Start time: " << get_timestamp() << "\n";
        std::cout << "Summary log: " << summary_log_path << "\n";
    }

    std::cout << "\n========== Transformation Equivalence Tests ==========\n\n";

    // Run Catch2 tests
    int result = Catch::Session().run(argc, argv);

    // Final summary
    double total_time = duration<double, std::milli>(steady_clock::now() - test_start_time).count() / 1000.0;

    LOG_INFO("Tests completed");
    LOG_INFO("Total time: {:.1f} seconds", total_time);
    LOG_INFO("Tests run: {}", stats.total);
    LOG_INFO("Passed: {}", stats.passed);
    LOG_INFO("Failed: {}", stats.failed);

    if (summary_log.is_open())
    {
        summary_log << "\n========== Final Summary ==========\n";
        summary_log << "End time: " << get_timestamp() << "\n";
        summary_log << "Total time: " << total_time << " seconds\n";
        summary_log << "Tests run: " << stats.total << "\n";
        summary_log << "Passed: " << stats.passed << "\n";
        summary_log << "Failed: " << stats.failed << "\n";
        summary_log.close();
    }

    if (failure_log.is_open())
    {
        failure_log.close();
    }

    LOG_FLUSH();

    return result;
}
