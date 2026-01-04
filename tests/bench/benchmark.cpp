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
#include <sstream>
#include <vector>

using namespace formula;
using namespace synthesis;

// Helper function to join strings
static std::string join(const std::vector<std::string>& vec, const std::string& delim) {
    if (vec.empty()) return "";
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) oss << delim;
        oss << vec[i];
    }
    return oss.str();
}

int main(int argc, char* argv[]) {
    std::string base_dir = "benchmarks/sm1000";
    std::string bench_spec = "all";  // "all", "1", "2"
    int start_bench = 1;
    int end_bench = 500;

    // Parse arguments:
    // argv[1]: base_dir (optional)
    // argv[2]: bench_spec or start_bench (optional)
    // argv[3]: end_bench (optional, only if argv[2] is a number)
    if (argc > 1) {
        base_dir = argv[1];
    }
    if (argc > 2) {
        std::string arg2 = argv[2];
        if (arg2 == "1" || arg2 == "2" || arg2 == "all") {
            bench_spec = arg2;
        } else {
            start_bench = std::atoi(arg2.c_str());
            if (argc > 3) {
                end_bench = std::atoi(argv[3]);
            }
            bench_spec = "1";  // default to bench1 when using numeric range
        }
    }

    // Determine which bench directories to run
    std::vector<int> bench_dirs;
    if (bench_spec == "all") {
        bench_dirs = {1, 2};
    } else if (bench_spec == "1") {
        bench_dirs = {1};
    } else if (bench_spec == "2") {
        bench_dirs = {2};
    }

    LOG_INFO("Benchmark Runner starting...");
    LOG_INFO("Base directory: {}", base_dir);
    LOG_INFO("Bench directories: {}", bench_spec);
    LOG_INFO("Formula range: f{} to f{}", start_bench, end_bench);

    int parsed = 0;
    int failed_parse = 0;
    int found_results = 0;
    int not_found_results = 0;
    double total_time_ms = 0;
    int total_count = 0;

    for (int bench_dir : bench_dirs) {
        for (int i = start_bench; i <= end_bench; ++i) {
            std::string formula_str;
            std::vector<std::string> outputs, inputs;

            auto start = std::chrono::high_resolution_clock::now();

            // Read benchmark from specific directory
            if (!Synthesis::read_benchmark_from_dir(base_dir, bench_dir, i, formula_str, outputs, inputs)) {
                std::cout << "SKIP: bench" << bench_dir << "/f" << i << " (file not found)" << std::endl;
                continue;
            }

        // Parse formula
        FormulaPool pool;
        Formula* f = Synthesis::parse_formula(formula_str, pool);

        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double, std::milli>(end - start).count();
        total_time_ms += elapsed;

        // Build partition string
        std::string partition_str;
        if (!inputs.empty() && !outputs.empty()) {
            partition_str = "inputs: [" + join(inputs, ", ") + "], outputs: [" + join(outputs, ", ") + "]";
        } else if (!outputs.empty()) {
            partition_str = "outputs: [" + join(outputs, ", ") + "]";
        } else if (!inputs.empty()) {
            partition_str = "inputs: [" + join(inputs, ", ") + "]";
        } else {
            partition_str = "(no partition)";
        }

        if (f) {
            parsed++;
            std::cout << "OK: bench" << bench_dir << "/f" << i << " (" << elapsed << "ms)" << std::endl;
            std::cout << "  Formula: " << formula_str << std::endl;
            std::cout << "  Partition: " << partition_str << std::endl;

            // TODO: Check satisfiability (disabled due to Z3 timeout issues)
            // auto sat = Synthesis::is_satisfiable(f);
            // if (sat.has_value()) {
            //     std::cout << "  Satisfiable: " << (sat.value() ? "yes" : "no") << std::endl;
            // }

        } else {
            failed_parse++;
            std::cout << "FAIL: bench" << bench_dir << "/f" << i << " (parse error)" << std::endl;
            std::cout << "  Formula: " << formula_str << std::endl;
            std::cout << "  Partition: " << partition_str << std::endl;
        }

        // Check expected result
        auto expected = Synthesis::read_expected_result(base_dir, i);
        if (expected.has_value()) {
            found_results++;
            std::cout << "  Expected: " << (expected.value() ? "Realizable" : "Unrealizable") << std::endl;
        } else {
            not_found_results++;
        }

        total_count++;

        std::cout << std::endl;  // Blank line between entries

        // Progress
        if (total_count % 10 == 0) {
            std::cout << "--- Progress: " << total_count << " formulas processed ---" << std::endl;
        }
    }  // end for (int i = start_bench; ...)
    }  // end for (int bench_dir : bench_dirs)

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

    // Explicitly flush and shutdown logger to avoid hang on exit
    LOG_FLUSH();
    logger::Logger::instance().get()->flush();
    spdlog::shutdown();

    return (failed_parse > 0) ? 1 : 0;
}
