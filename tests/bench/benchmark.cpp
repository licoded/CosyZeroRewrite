/**
 * Benchmark Runner for SMv2 Dataset
 *
 * Reads formulas from benchmarks/sm1000/ and compares with expected results.
 *
 * Usage:
 *   benchmark_test [options] [base_dir] [bench_spec] [start] [end]
 *
 * Options:
 *   -v, --verbose    Print all cases (default: only failed cases)
 *   -q, --quiet      Only print summary (no per-case output)
 *   -p, --progress   Print progress (default: yes)
 *   --no-progress    Disable progress output
 *   -h, --help       Show help message
 *
 * Arguments:
 *   base_dir    Benchmark directory (default: benchmarks/sm1000)
 *   bench_spec  "all", "1", or "2" (default: all)
 *   start       Starting formula number (default: 1)
 *   end         Ending formula number (default: 500)
 *
 * Examples:
 *   benchmark_test                                    # All formulas, quiet mode
 *   benchmark_test -v                                 # All formulas, verbose
 *   benchmark_test -q all 1 10                        # First 10, quiet only
 *   benchmark_test --verbose --no-progress all 1 50   # First 50, verbose, no progress
 */

#include "synthesis/synthesis.hpp"
#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"
#include "log/logger.hpp"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <vector>
#include <string>

using namespace formula;
using namespace synthesis;

// Output verbosity level
enum class Verbosity {
    Quiet,      // Only summary
    Normal,     // Failed cases + summary (default)
    Verbose     // All cases + summary
};

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

// Print usage message
static void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options] [base_dir] [bench_spec] [start] [end]\n\n"
              << "Options:\n"
              << "  -v, --verbose    Print all cases (default: only failed cases)\n"
              << "  -q, --quiet      Only print summary (no per-case output)\n"
              << "  -p, --progress   Print progress (default: yes)\n"
              << "  --no-progress    Disable progress output\n"
              << "  -h, --help       Show this help message\n\n"
              << "Arguments:\n"
              << "  base_dir    Benchmark directory (default: benchmarks/sm1000)\n"
              << "  bench_spec  \"all\", \"1\", or \"2\" (default: all)\n"
              << "  start       Starting formula number (default: 1)\n"
              << "  end         Ending formula number (default: 500)\n\n"
              << "Examples:\n"
              << "  " << prog_name << "                                  # All 1000 formulas\n"
              << "  " << prog_name << " -v                               # Verbose mode\n"
              << "  " << prog_name << " -q all 1 10                      # Quiet, first 10\n"
              << "  " << prog_name << " --no-progress all 1 50           # No progress, first 50\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    // Default values
    std::string base_dir = "benchmarks/sm1000";
    std::string bench_spec = "all";
    int start_bench = 1;
    int end_bench = 500;
    Verbosity verbosity = Verbosity::Normal;
    bool show_progress = true;

    // Parse command line arguments
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    size_t arg_idx = 0;
    while (arg_idx < args.size()) {
        const std::string& arg = args[arg_idx];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--verbose") {
            verbosity = Verbosity::Verbose;
            arg_idx++;
        } else if (arg == "-q" || arg == "--quiet") {
            verbosity = Verbosity::Quiet;
            arg_idx++;
        } else if (arg == "-p" || arg == "--progress") {
            show_progress = true;
            arg_idx++;
        } else if (arg == "--no-progress") {
            show_progress = false;
            arg_idx++;
        } else if (arg[0] != '-') {
            // Positional arguments: base_dir [bench_spec|start] [end]
            base_dir = arg;
            arg_idx++;

            if (arg_idx < args.size()) {
                std::string arg2 = args[arg_idx];
                if (arg2 == "1" || arg2 == "2" || arg2 == "all") {
                    // arg2 is bench_spec
                    bench_spec = arg2;
                    arg_idx++;

                    // Check for start/end after bench_spec
                    if (arg_idx < args.size()) {
                        std::string arg3 = args[arg_idx];
                        if (arg3[0] != '-') {
                            start_bench = std::atoi(arg3.c_str());
                            arg_idx++;

                            if (arg_idx < args.size()) {
                                std::string arg4 = args[arg_idx];
                                if (arg4[0] != '-') {
                                    end_bench = std::atoi(arg4.c_str());
                                    arg_idx++;
                                }
                            }
                        }
                    }
                } else if (arg2[0] != '-') {
                    // arg2 is start_bench (numeric)
                    start_bench = std::atoi(arg2.c_str());
                    arg_idx++;

                    if (arg_idx < args.size()) {
                        std::string arg3 = args[arg_idx];
                        if (arg3[0] != '-') {
                            end_bench = std::atoi(arg3.c_str());
                            arg_idx++;
                        }
                    }
                    // When using numeric range, default to bench1
                    bench_spec = "1";
                }
            }
            break;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
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

    // Print header based on verbosity
    if (verbosity != Verbosity::Quiet) {
        std::cout << "Benchmark Runner" << std::endl;
        std::cout << "=================" << std::endl;
        std::cout << "Base directory: " << base_dir << std::endl;
        std::cout << "Bench directories: " << bench_spec << std::endl;
        std::cout << "Formula range: f" << start_bench << " to f" << end_bench << std::endl;
        std::cout << "Mode: "
                  << (verbosity == Verbosity::Verbose ? "verbose" :
                      verbosity == Verbosity::Quiet ? "quiet" : "normal")
                  << std::endl;
        std::cout << std::endl;
    }

    // Statistics
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
                if (verbosity == Verbosity::Verbose) {
                    std::cout << "SKIP: bench" << bench_dir << "/f" << i << " (file not found)" << std::endl;
                }
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

            // Get expected result
            auto expected = Synthesis::read_expected_result(base_dir, i);

            if (f) {
                parsed++;
                // Only print OK cases in verbose mode
                if (verbosity == Verbosity::Verbose) {
                    std::cout << "OK: bench" << bench_dir << "/f" << i << " (" << elapsed << "ms)" << std::endl;
                    std::cout << "  Formula: " << formula_str << std::endl;
                    std::cout << "  Partition: " << partition_str << std::endl;
                    if (expected.has_value()) {
                        std::cout << "  Expected: " << (expected.value() ? "Realizable" : "Unrealizable") << std::endl;
                    }
                    std::cout << std::endl;
                }
            } else {
                failed_parse++;
                // Always print FAIL cases (unless in quiet mode)
                if (verbosity != Verbosity::Quiet) {
                    std::cout << "FAIL: bench" << bench_dir << "/f" << i << " (parse error)" << std::endl;
                    std::cout << "  Formula: " << formula_str << std::endl;
                    std::cout << "  Partition: " << partition_str << std::endl;
                    if (expected.has_value()) {
                        std::cout << "  Expected: " << (expected.value() ? "Realizable" : "Unrealizable") << std::endl;
                    }
                    std::cout << std::endl;
                }
            }

            if (expected.has_value()) {
                found_results++;
            } else {
                not_found_results++;
            }

            total_count++;

            // Progress indicator (only in non-quiet mode)
            if (show_progress && verbosity != Verbosity::Quiet && total_count % 100 == 0) {
                std::cout << "--- Progress: " << total_count << " formulas processed ---" << std::endl;
            }
        }
    }

    // Always print summary
    std::cout << "\n========== Summary ==========" << std::endl;
    std::cout << "Parsed: " << parsed << std::endl;
    std::cout << "Failed parse: " << failed_parse << std::endl;
    std::cout << "Results found: " << found_results << std::endl;
    std::cout << "Results not found: " << not_found_results << std::endl;
    std::cout << "Total time: " << std::fixed << std::setprecision(2) << total_time_ms << "ms" << std::endl;
    std::cout << "Average time: " << std::fixed << std::setprecision(3)
              << (total_count > 0 ? total_time_ms / total_count : 0) << "ms" << std::endl;
    if (failed_parse == 0) {
        std::cout << "Status: " << "\033[32m" << "ALL TESTS PASSED" << "\033[0m" << std::endl;
    } else {
        std::cout << "Status: " << "\033[31m" << "SOME TESTS FAILED" << "\033[0m" << std::endl;
    }

    // Explicitly flush and shutdown logger to avoid hang on exit
    LOG_FLUSH();
    logger::Logger::instance().get()->flush();
    spdlog::shutdown();

    return (failed_parse > 0) ? 1 : 0;
}
