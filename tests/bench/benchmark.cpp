/**
 * Benchmark Runner for SMv2 Dataset
 *
 * Reads formulas from benchmarks/sm1000/ and compares with expected results.
 * This is a data preparation step for full synthesis implementation.
 */

#include "synthesis/synthesis.hpp"
#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"
#include "log/logger.hpp"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <filesystem>

using namespace formula;
using namespace synthesis;

int main(int argc, char* argv[]) {
    std::string base_dir = "benchmarks/sm1000";
    int start_bench = 1;
    int end_bench = 100;

    if (argc > 1) {
        base_dir = argv[1];
    }
    if (argc > 2) {
        start_bench = std::atoi(argv[2]);
    }
    if (argc > 3) {
        end_bench = std::atoi(argv[3]);
    }

    LOG_INFO("Benchmark Runner starting...");
    LOG_INFO("Base directory: {}", base_dir);
    LOG_INFO("Benchmarks: {} to {}", start_bench, end_bench);

    int parsed = 0;
    int failed_parse = 0;
    int found_results = 0;
    int not_found_results = 0;
    double total_time_ms = 0;

    for (int i = start_bench; i <= end_bench; ++i) {
        std::string formula_str;
        std::vector<std::string> outputs, inputs;

        auto start = std::chrono::high_resolution_clock::now();

        // Read benchmark
        if (!Synthesis::read_benchmark(base_dir, i, formula_str, outputs, inputs)) {
            std::cout << "SKIP: f" << i << " (file not found)" << std::endl;
            continue;
        }

        // Parse formula
        FormulaPool pool;
        Formula* f = Synthesis::parse_formula(formula_str, pool);

        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double, std::milli>(end - start).count();
        total_time_ms += elapsed;

        if (f) {
            parsed++;
            std::cout << "OK: f" << i << " (" << formula_str.substr(0, 50) << "...) "
                      << "(" << elapsed << "ms)" << std::endl;

            // Check satisfiability
            auto sat = Synthesis::is_satisfiable(f);
            if (sat.has_value()) {
                std::cout << "  Satisfiable: " << (sat.value() ? "yes" : "no") << std::endl;
            }

        } else {
            failed_parse++;
            std::cout << "FAIL: f" << i << " (parse error)" << std::endl;
        }

        // Check expected result
        auto expected = Synthesis::read_expected_result(base_dir, i);
        if (expected.has_value()) {
            found_results++;
            std::cout << "  Expected: " << (expected.value() ? "Realizable" : "Unrealizable") << std::endl;
        } else {
            not_found_results++;
        }

        // Progress
        if ((i - start_bench + 1) % 10 == 0) {
            std::cout << "--- Progress: " << (i - start_bench + 1) << "/" << (end_bench - start_bench + 1) << " ---" << std::endl;
        }
    }

    LOG_INFO("Benchmark runner completed");
    LOG_INFO("Parsed: {}, Failed: {}", parsed, failed_parse);
    LOG_INFO("Results found: {}, Not found: {}", found_results, not_found_results);
    LOG_INFO("Total time: {:.1f}ms", total_time_ms);

    std::cout << "\n========== Summary ==========" << std::endl;
    std::cout << "Parsed: " << parsed << std::endl;
    std::cout << "Failed parse: " << failed_parse << std::endl;
    std::cout << "Results found: " << found_results << std::endl;
    std::cout << "Results not found: " << not_found_results << std::endl;
    std::cout << "Total time: " << total_time_ms << "ms" << std::endl;

    return (failed_parse > 0) ? 1 : 0;
}
