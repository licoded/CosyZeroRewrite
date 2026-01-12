/**
 * Benchmark Runner for SMv2 Dataset (Parallel Version)
 *
 * Features:
 * - Multi-threaded execution using std::async-based thread pool
 * - CLI11 command-line parsing
 * - indicators progress bars
 * - Real-time display of active and pending tasks
 *
 * Usage:
 *   benchmark_test [options]
 *
 * Options:
 *   -j, --jobs <N>       Number of parallel jobs (default: CPU count)
 *   -v, --verbose        Print all cases
 *   -q, --quiet          Only print summary
 *   --no-progress        Disable progress bar
 *   --no-active          Don't show active tasks
 *   -h, --help           Show help
 *
 * === Updated: 2026-01-05 ===
 */

#include "formula/formula.hpp"
#include "formula/formula_parser.hpp"
#include "formula/formula_pool.hpp"
#include "io/file_utils.hpp"
#include "log/logger.hpp"
#include "parallel/thread_pool.hpp"
#include "synthesis/on_the_fly_solver.hpp"

#include <tl/expected.hpp>

// CLI11 - command line parsing
#include <CLI/CLI.hpp>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace formula;
using namespace io;

// ============================================================================
// Benchmark helper functions (previously in Synthesis class)
// ============================================================================

namespace {

/**
 * @brief Parse an LTLf formula from string
 */
Formula *parse_formula(const std::string &formula_str, FormulaPool &pool)
{
    FormulaParser parser(pool);
    Formula *f = parser.parse(formula_str);
    if (parser.has_error())
    {
        return nullptr;
    }
    return f;
}

/**
 * @brief Read expected result from results.csv file
 */
std::optional<bool> read_expected_result(const std::string &base_dir, int bench_num)
{
    std::string results_file = base_dir + "/results.csv";
    auto read_file_res = read_file_expected(results_file);
    if (!read_file_res)
    {
        NOP_LOG_ERROR(read_file_res.error());
        return std::nullopt;
    }

    std::istringstream iss(*read_file_res);
    std::string line;
    // Skip header
    std::getline(iss, line);

    std::string target_filename = "f" + std::to_string(bench_num);

    while (std::getline(iss, line))
    {
        if (line.empty())
            continue;

        std::istringstream line_ss(line);
        std::string folder, filename, result;
        if (!std::getline(line_ss, folder, ','))
            continue;
        if (!std::getline(line_ss, filename, ','))
            continue;
        if (!std::getline(line_ss, result, ','))
            continue;

        if (filename == target_filename)
        {
            // Trim whitespace and check
            if (result == "Realizable")
                return true;
            if (result == "Unrealizable")
                return false;
        }
    }

    return std::nullopt;
}

struct Benchmark {
    std::string formula;
    io::Partition partition;
};

/**
 * @brief Read benchmark file (formula + partition) from specific directory
 * @return tl::expected<Benchmark, std::string> - Benchmark on success, error message on failure
 */
tl::expected<Benchmark, std::string> read_benchmark_from_dir(const std::string &base_dir, int bench_dir, int bench_num)
{
    std::string ltlf_file = fmt::format("{}/bench{}/f{}.ltlf", base_dir, bench_dir, bench_num);
    std::string part_file = fmt::format("{}/bench{}/f{}.part", base_dir, bench_dir, bench_num);

    // Read formula
    auto read_file_res = read_file_expected(ltlf_file);
    if (!read_file_res)
    {
        return tl::unexpected(fmt::format("Cannot read formula file: {}; {}", ltlf_file, read_file_res.error()));
    }

    // Read partition (required)
    auto partition = parse_partition_file(part_file);
    if (!partition)
    {
        return tl::unexpected(fmt::format("Cannot read partition file: {} - {}", part_file, partition.error()));
    }

    return Benchmark {trim(*read_file_res), *partition};
}

} // anonymous namespace

// ============================================================================
// Task result structure
// ============================================================================

struct TaskResult {
    int bench_dir;
    int formula_index; // Formula file number (f1, f2, f3, ...)
    bool success;
    double elapsed_ms;
    std::string formula_str;
    std::string partition_str;
    std::string error_msg;
    std::optional<bool> realizable; // synthesis result

    TaskResult() : bench_dir(0), formula_index(0), success(false), elapsed_ms(0.0) {}
};

// ============================================================================
// Helper functions
// ============================================================================

static std::string make_partition_string(const std::vector<std::string> &inputs,
                                         const std::vector<std::string> &outputs)
{
    if (!inputs.empty() && !outputs.empty())
    {
        return fmt::format("inputs: [{}], outputs: [{}]", fmt::join(inputs, ", "), fmt::join(outputs, ", "));
    }
    else if (!outputs.empty())
    {
        return fmt::format("outputs: [{}]", fmt::join(outputs, ", "));
    }
    else if (!inputs.empty())
    {
        return fmt::format("inputs: [{}]", fmt::join(inputs, ", "));
    }
    return "(no partition)";
}

// ============================================================================
// Task processor
// ============================================================================

class BenchmarkRunner
{
  public:
    BenchmarkRunner(
        const std::string &base_dir, const std::vector<int> &bench_dirs, int start_num, int end_num, size_t num_jobs)
        : base_dir_(base_dir), bench_dirs_(bench_dirs), start_num_(start_num), end_num_(end_num), num_jobs_(num_jobs)
    {
        // Calculate total tasks
        total_tasks_ = bench_dirs_.size() * (end_num - start_num + 1);
    }

    std::vector<TaskResult> run()
    {
        std::vector<TaskResult> results;
        results.reserve(total_tasks_);

        parallel::ThreadPool pool(num_jobs_);

        // Mutex for protecting shared access
        std::mutex results_mutex;
        std::mutex active_mutex;
        std::set<int> active_formula_indexs;
        std::atomic<int> completed {0};
        std::atomic<int> failed {0};
        // Lambda to process a single formula
        auto process_formula = [&](int bench_dir, int formula_index) -> TaskResult {
            TaskResult result;
            result.bench_dir = bench_dir;
            result.formula_index = formula_index;

            // Add to active set
            {
                std::lock_guard<std::mutex> lock(active_mutex);
                active_formula_indexs.insert(formula_index);
            }

            auto start = std::chrono::high_resolution_clock::now();

            // Read benchmark
            auto benchmark = read_benchmark_from_dir(base_dir_, bench_dir, formula_index);
            if (!benchmark)
            {
                result.success = false;
                result.error_msg = benchmark.error();
            }
            else
            {
                const auto &outputs = benchmark->partition.outputs;
                const auto &inputs = benchmark->partition.inputs;

                // Parse formula
                FormulaPool pool;
                Formula *f = parse_formula(benchmark->formula, pool);

                if (!f)
                {
                    auto end = std::chrono::high_resolution_clock::now();
                    result.elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
                    result.formula_str = benchmark->formula;
                    result.partition_str = make_partition_string(inputs, outputs);
                    result.success = false;
                    result.error_msg = "parse error";
                }
                else
                {
                    // Declare variables and run synthesis
                    pool.declare_variables(outputs, inputs);

                    synthesis::OnTheFlyGameSolver solver(f, pool);
                    bool realizable = solver.is_realizable();

                    auto end = std::chrono::high_resolution_clock::now();
                    result.elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
                    result.formula_str = benchmark->formula;
                    result.partition_str = make_partition_string(inputs, outputs);
                    result.success = true;
                    result.realizable = realizable;
                }
            }

            // Remove from active set
            {
                std::lock_guard<std::mutex> lock(active_mutex);
                active_formula_indexs.erase(formula_index);
            }

            // Update progress
            ++completed;
            if (!result.success)
            {
                ++failed;
            }

            return result;
        };

        // Submit all tasks
        std::vector<std::future<TaskResult>> futures;

        for (int bench_dir : bench_dirs_)
        {
            for (int i = start_num_; i <= end_num_; ++i)
            {
                auto future = pool.submit(process_formula, bench_dir, i);
                futures.push_back(std::move(future));
            }
        }

        // Collect results
        for (auto &future : futures)
        {
            results.push_back(future.get());
        }

        return results;
    }

  private:
    std::string base_dir_;
    std::vector<int> bench_dirs_;
    int start_num_;
    int end_num_;
    size_t num_jobs_;
    size_t total_tasks_;
};

// ============================================================================
// Configuration
// ============================================================================

struct BenchmarkConfig {
    // Paths and spec
    std::string base_dir = "tools/benchmarks/sm1000";
    std::string bench_spec = "all";

    // Formula range
    int start_num = 1;
    int end_num = 500;

    // Execution
    size_t num_jobs = std::thread::hardware_concurrency();

    // Output options
    bool verbose = false; // Print all cases (not just failures)
};

// ============================================================================
// Argument Parsing
// ============================================================================

static std::vector<int> parse_bench_spec(const std::string &spec)
{
    if (spec == "all")
        return {1, 2};
    if (spec == "1")
        return {1};
    if (spec == "2")
        return {2};
    return {}; // Invalid
}

static bool parse_arguments(int argc, char *argv[], BenchmarkConfig &config)
{
    CLI::App app {"SMv2 LTLf Benchmark Runner (Parallel)"};

    // Define options (CLI11 can reference struct members directly)
    app.add_option("-d,--dir", config.base_dir, "Benchmark directory")->capture_default_str();
    app.add_option("-b,--bench", config.bench_spec, "Benchmark spec (all, 1, or 2)")->capture_default_str();
    app.add_option("-s,--start", config.start_num, "Starting formula number")->check(CLI::Range(1, 500));
    app.add_option("-e,--end", config.end_num, "Ending formula number")->check(CLI::Range(1, 500));
    app.add_option("-j,--jobs", config.num_jobs, "Number of parallel jobs")->check(CLI::PositiveNumber);
    app.add_flag("-v,--verbose", config.verbose, "Print all cases (not just failures)");

    // Parse
    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError &e)
    {
        return app.exit(e);
    }

    // Validate range
    if (config.start_num > config.end_num)
    {
        std::cerr << "Error: start number cannot be greater than end number" << std::endl;
        return false;
    }

    return true;
}

// ============================================================================
// Banner / Header
// ============================================================================

static void print_banner(const BenchmarkConfig &config)
{
    NOP_LOG_INFO("========================================");
    NOP_LOG_INFO("  SMv2 Benchmark Runner (Parallel)");
    NOP_LOG_INFO("========================================");
    NOP_LOG_INFO("Base directory: {}", config.base_dir);
    NOP_LOG_INFO("Bench directories: {}", config.bench_spec);
    NOP_LOG_INFO("Formula range: f{} to f{}", config.start_num, config.end_num);
    NOP_LOG_INFO("Parallel jobs: {}", config.num_jobs);
}

// ============================================================================
// Summary Output
// ============================================================================

struct BenchmarkStats {
    int parsed = 0;
    int realizable = 0;
    int unrealizable = 0;
    int timeout = 0;
    int found_results = 0; // matched with results.csv
    double total_time_ms = 0;
    double wall_time_ms = 0;
    size_t total_count = 0;
};

static void print_summary(const BenchmarkStats &stats)
{
    const int failed = stats.total_count - stats.parsed;
    const int not_found = stats.total_count - stats.found_results;

    NOP_LOG_INFO("========== Summary ==========");
    NOP_LOG_INFO("Parsed: {}", stats.parsed);
    NOP_LOG_INFO("Failed parse: {}", failed);
    NOP_LOG_INFO("Realizable: {}", stats.realizable);
    NOP_LOG_INFO("Unrealizable: {}", stats.unrealizable);
    NOP_LOG_INFO("Results found: {}", stats.found_results);
    NOP_LOG_INFO("Results not found: {}", not_found);
    NOP_LOG_INFO("Total formulas: {}", stats.total_count);
    NOP_LOG_INFO("Wall time: {:.2f}ms", stats.wall_time_ms);
    NOP_LOG_INFO("CPU time: {:.2f}ms", stats.total_time_ms);
    if (stats.total_count > 0)
    {
        NOP_LOG_INFO("Speedup: {:.2f}x", stats.total_time_ms / stats.wall_time_ms);
    }
    NOP_LOG_INFO("Avg time per formula: {:.3f}ms", stats.total_count > 0 ? stats.total_time_ms / stats.total_count : 0);

    if (failed == 0)
    {
        NOP_LOG_INFO("Status: ALL TESTS PASSED");
    }
    else
    {
        NOP_LOG_INFO("Status: SOME TESTS FAILED");
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char *argv[])
{
    // Parse command line arguments into config
    BenchmarkConfig config;
    if (!parse_arguments(argc, argv, config))
    {
        return 1; // Error already printed by parse_arguments
    }

    // Determine which bench directories to run
    std::vector<int> bench_dirs = parse_bench_spec(config.bench_spec);
    if (bench_dirs.empty())
    {
        std::cerr << "Error: bench_spec must be 'all', '1', or '2'" << std::endl;
        return 1;
    }

    // Print banner
    print_banner(config);

    // Run benchmarks
    BenchmarkRunner runner(config.base_dir, bench_dirs, config.start_num, config.end_num, config.num_jobs);

    auto start_time = std::chrono::high_resolution_clock::now();
    auto results = runner.run();
    auto end_time = std::chrono::high_resolution_clock::now();

    // ============================================================================
    // Step 1: Collect statistics
    // ============================================================================
    BenchmarkStats stats;
    stats.total_count = results.size();
    stats.wall_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    for (const auto &r : results)
    {
        stats.total_time_ms += r.elapsed_ms;
        stats.parsed += r.success ? 1 : 0;

        if (r.realizable.has_value())
        {
            if (r.realizable.value())
            {
                stats.realizable++;
            }
            else
            {
                stats.unrealizable++;
            }
        }

        // Check expected result
        auto expected = read_expected_result(config.base_dir, r.formula_index);
        stats.found_results += expected.has_value() ? 1 : 0;
    }

    // ============================================================================
    // Step 2: Print individual results
    // ============================================================================
    // Print logic: verbose mode = print all; otherwise = print only failures
    for (const auto &r : results)
    {
        // Skip successes in non-verbose mode
        if (!config.verbose && r.success)
            continue;

        if (r.success)
        {
            const char *result_str = "UNKNOWN";
            if (r.realizable.has_value())
            {
                result_str = r.realizable.value() ? "REALIZABLE" : "UNREALIZABLE";
            }
            NOP_LOG_INFO("OK: bench{}/f{} ({}ms) {}", r.bench_dir, r.formula_index, r.elapsed_ms, result_str);
        }
        else
        {
            NOP_LOG_INFO("FAIL: bench{}/f{} ({})", r.bench_dir, r.formula_index, r.error_msg);
        }
        NOP_LOG_INFO("  Formula: {}", r.formula_str);
        NOP_LOG_INFO("  Partition: {}", r.partition_str);
    }

    // ============================================================================
    // Step 3: Print summary
    // ============================================================================
    print_summary(stats);

    // Cleanup
    LOG_FLUSH();
    logger::Logger::instance().logger()->flush();
    spdlog::shutdown();

    return (static_cast<int>(stats.total_count) > stats.parsed) ? 1 : 0;
}
