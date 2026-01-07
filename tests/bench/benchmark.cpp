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

#include "synthesis/synthesis.hpp"
#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"
#include "parallel/thread_pool.hpp"
#include "log/logger.hpp"

// CLI11 - command line parsing
#include <CLI/CLI.hpp>

// fmt + termcolor for clean, colored output
#include <fmt/core.h>
#include "indicators/termcolor.hpp"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <random>
#include <sstream>
#include <vector>
#include <string>
#include <atomic>
#include <set>
#include <mutex>

using namespace formula;
using namespace synthesis;

// ============================================================================
// Task result structure
// ============================================================================

struct TaskResult {
    int bench_dir;
    int formula_num;
    bool success;
    double elapsed_ms;
    std::string formula_str;
    std::string partition_str;
    std::string error_msg;

    TaskResult() : bench_dir(0), formula_num(0), success(false), elapsed_ms(0.0) {}
};

// ============================================================================
// Progress display
// ============================================================================

class ProgressDisplay {
public:
    ProgressDisplay(size_t total, bool show_progress, bool show_active)
        : total_(total)
        , show_progress_(show_progress)
        , show_active_(show_active)
        , completed_(0)
        , failed_(0)
        , last_update_count_(0)
        , last_output_len_(0)
    {
        if (show_progress_) {
            std::cout << std::flush;
        }
    }

    ~ProgressDisplay() {
        if (show_progress_) {
            std::cout << std::endl;
        }
    }

    void update(int completed, int failed, const std::set<int>& active_tasks) {
        if (!show_progress_) return;

        // Update every 5 items or every 100ms
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update_time_).count();

        if (completed - last_update_count_ < 5 && elapsed < 100) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        last_update_time_ = now;
        last_update_count_ = completed;

        // Calculate progress bar width
        const int bar_width = 40;
        int filled = (completed * bar_width) / static_cast<int>(total_);
        if (filled > bar_width) filled = bar_width;

        // Build progress bar with fixed width: [===>     ]
        std::string bar;
        if (filled < bar_width) {
            bar = std::string(filled, '=') + ">" + std::string(bar_width - filled - 1, ' ');
        } else {
            bar = std::string(bar_width, '=');
        }

        // Build active tasks string (limit to 5 items)
        std::string active_str;
        if (show_active_ && !active_tasks.empty()) {
            active_str = " | ";
            int count = 0;
            for (int task : active_tasks) {
                if (count > 0) active_str += ", ";
                active_str += "f" + std::to_string(task);
                if (++count >= 5) break;
            }
            if (active_tasks.size() > 5) {
                active_str += "...";
            }
        }

        // Print with carriage return to update in place
        // Build the output string
        std::ostringstream oss;
        oss << "[" << bar << "] "
            << completed << "/" << total_
            << " (" << failed << " failed)"
            << active_str;

        std::string output = oss.str();

        // Clear any leftover characters from previous longer output
        if (output.length() < last_output_len_) {
            output += std::string(last_output_len_ - output.length(), ' ');
        }

        last_output_len_ = output.length();
        std::cout << "\r" << output << std::flush;
    }

    void finish() {
        if (show_progress_) {
            std::cout << "\r[" << std::string(40, '=') << "] "
                      << total_ << "/" << total_
                      << " (" << failed_ << " failed)"
                      << " Done!" << std::endl;
        }
    }

private:
    size_t total_;
    bool show_progress_;
    bool show_active_;
    std::atomic<int> completed_;
    std::atomic<int> failed_;
    int last_update_count_;
    size_t last_output_len_;
    std::chrono::steady_clock::time_point last_update_time_;
    std::mutex mutex_;
};

// ============================================================================
// Helper functions
// ============================================================================

// fmt_print_with_color: fmt formatting + termcolor in one call
// Usage: fmt_print_with_color(termcolor::red, "FAIL: {}\n", error_msg);
template<typename... Args>
static void fmt_print_with_color(std::ostream& (*color)(std::ostream&),
                                 fmt::format_string<Args...> fmt_str,
                                 Args&&... args) {
    std::cout << color;
    fmt::print(fmt_str, std::forward<Args>(args)...);
    std::cout << termcolor::reset;
}

static std::string join(const std::vector<std::string>& vec, const std::string& delim) {
    if (vec.empty()) return "";
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) oss << delim;
        oss << vec[i];
    }
    return oss.str();
}

static std::string make_partition_string(
    const std::vector<std::string>& inputs,
    const std::vector<std::string>& outputs)
{
    if (!inputs.empty() && !outputs.empty()) {
        return "inputs: [" + join(inputs, ", ") + "], outputs: [" + join(outputs, ", ") + "]";
    } else if (!outputs.empty()) {
        return "outputs: [" + join(outputs, ", ") + "]";
    } else if (!inputs.empty()) {
        return "inputs: [" + join(inputs, ", ") + "]";
    }
    return "(no partition)";
}

// ============================================================================
// Task processor
// ============================================================================

class BenchmarkRunner {
public:
    BenchmarkRunner(
        const std::string& base_dir,
        const std::vector<int>& bench_dirs,
        int start_num,
        int end_num,
        size_t num_jobs,
        bool show_progress,
        bool show_active,
        int sleep_per_task)
        : base_dir_(base_dir)
        , bench_dirs_(bench_dirs)
        , start_num_(start_num)
        , end_num_(end_num)
        , num_jobs_(num_jobs)
        , show_progress_(show_progress)
        , show_active_(show_active)
        , sleep_per_task_(sleep_per_task)
    {
        // Calculate total tasks
        total_tasks_ = bench_dirs_.size() * (end_num - start_num + 1);

        // Initialize progress display
        if (show_progress) {
            progress_ = std::make_unique<ProgressDisplay>(total_tasks_, show_progress, show_active);
        }
    }

    std::vector<TaskResult> run() {
        std::vector<TaskResult> results;
        results.reserve(total_tasks_);

        parallel::ThreadPool pool(num_jobs_);

        // Mutex for protecting shared access
        std::mutex results_mutex;
        std::mutex active_mutex;
        std::set<int> active_formula_nums;
        std::atomic<int> completed{0};
        std::atomic<int> failed{0};
        std::atomic<int> next_task_index{0};

        // Lambda to process a single formula
        auto process_formula = [&](int bench_dir, int formula_num) -> TaskResult {
            TaskResult result;
            result.bench_dir = bench_dir;
            result.formula_num = formula_num;

            // Add to active set
            {
                std::lock_guard<std::mutex> lock(active_mutex);
                active_formula_nums.insert(formula_num);
            }

            auto start = std::chrono::high_resolution_clock::now();

            // Read benchmark
            std::string formula_str;
            std::vector<std::string> outputs, inputs;
            if (!Synthesis::read_benchmark_from_dir(
                base_dir_, bench_dir, formula_num, formula_str, outputs, inputs))
            {
                result.success = false;
                result.error_msg = "file not found";
            } else {
                // Parse formula
                FormulaPool pool;
                Formula* f = Synthesis::parse_formula(formula_str, pool);

                auto end = std::chrono::high_resolution_clock::now();
                result.elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
                result.formula_str = formula_str;
                result.partition_str = make_partition_string(inputs, outputs);

                if (f) {
                    result.success = true;
                } else {
                    result.success = false;
                    result.error_msg = "parse error";
                }

                // Artificial delay for testing progress bar (random duration)
                if (sleep_per_task_ > 0) {
                    // Random sleep: 0 to 2*sleep_per_task_ (average = sleep_per_task_)
                    static thread_local std::mt19937 rng(std::random_device{}());
                    std::uniform_int_distribution<int> dist(0, 2 * sleep_per_task_);
                    int sleep_ms = dist(rng) * 1000; // Convert to milliseconds
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
                }
            }

            // Remove from active set
            {
                std::lock_guard<std::mutex> lock(active_mutex);
                active_formula_nums.erase(formula_num);
            }

            // Update progress
            int c = ++completed;
            if (!result.success) {
                ++failed;
            }

            if (progress_) {
                std::set<int> active_copy;
                if (show_active_) {
                    std::lock_guard<std::mutex> lock(active_mutex);
                    active_copy = active_formula_nums;
                }
                progress_->update(c, failed.load(), active_copy);
            }

            return result;
        };

        // Submit all tasks
        std::vector<std::future<TaskResult>> futures;

        for (int bench_dir : bench_dirs_) {
            for (int i = start_num_; i <= end_num_; ++i) {
                auto future = pool.submit(process_formula, bench_dir, i);
                futures.push_back(std::move(future));
            }
        }

        // Collect results
        for (auto& future : futures) {
            results.push_back(future.get());
        }

        if (progress_) {
            progress_->finish();
        }

        return results;
    }

private:
    std::string base_dir_;
    std::vector<int> bench_dirs_;
    int start_num_;
    int end_num_;
    size_t num_jobs_;
    bool show_progress_;
    bool show_active_;
    int sleep_per_task_;
    size_t total_tasks_;
    std::unique_ptr<ProgressDisplay> progress_;
};

// ============================================================================
// Configuration
// ============================================================================

struct BenchmarkConfig {
    // Paths and spec
    std::string base_dir = "benchmarks/sm1000";
    std::string bench_spec = "all";

    // Formula range
    int start_num = 1;
    int end_num = 500;

    // Execution
    size_t num_jobs = std::thread::hardware_concurrency();
    int sleep_per_task = 0;  // For testing progress bar (0 = disabled)

    // Output options
    bool verbose = false;           // Print all cases (not just failures)
    bool show_progress = true;      // Show progress bar
    bool show_active_tasks = true;  // Show active tasks in progress bar
};

// ============================================================================
// Argument Parsing
// ============================================================================

static std::vector<int> parse_bench_spec(const std::string& spec) {
    if (spec == "all") return {1, 2};
    if (spec == "1") return {1};
    if (spec == "2") return {2};
    return {};  // Invalid
}

static bool parse_arguments(int argc, char* argv[], BenchmarkConfig& config) {
    CLI::App app{"SMv2 LTLf Benchmark Runner (Parallel)"};

    // Define options (CLI11 can reference struct members directly)
    app.add_option("-d,--dir", config.base_dir, "Benchmark directory")
        ->capture_default_str();
    app.add_option("-b,--bench", config.bench_spec, "Benchmark spec (all, 1, or 2)")
        ->capture_default_str();
    app.add_option("-s,--start", config.start_num, "Starting formula number")
        ->check(CLI::Range(1, 500));
    app.add_option("-e,--end", config.end_num, "Ending formula number")
        ->check(CLI::Range(1, 500));
    app.add_option("-j,--jobs", config.num_jobs, "Number of parallel jobs")
        ->check(CLI::PositiveNumber);
    app.add_flag("-v,--verbose", config.verbose, "Print all cases (not just failures)");
    app.add_flag("--no-progress", config.show_progress, "Disable progress bar")->capture_default_str();
    app.add_flag("--no-active", config.show_active_tasks, "Don't show active tasks")->capture_default_str();
    app.add_option("--sleep", config.sleep_per_task, "Average random sleep per task (seconds, for testing)")
        ->check(CLI::Range(0, 60));

    // Parse
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    // Validate range
    if (config.start_num > config.end_num) {
        std::cerr << "Error: start number cannot be greater than end number" << std::endl;
        return false;
    }

    // Invert flags for --no-* options
    config.show_progress = !config.show_progress;
    config.show_active_tasks = !config.show_active_tasks;

    return true;
}

// ============================================================================
// Banner / Header
// ============================================================================

static void print_banner(const BenchmarkConfig& config) {
    fmt::print("========================================\n");
    fmt::print("  SMv2 Benchmark Runner (Parallel)\n");
    fmt::print("========================================\n");
    fmt::print("Base directory: {}\n", config.base_dir);
    fmt::print("Bench directories: {}\n", config.bench_spec);
    fmt::print("Formula range: f{} to f{}\n", config.start_num, config.end_num);
    fmt::print("Parallel jobs: {}\n\n", config.num_jobs);
}

// ============================================================================
// Summary Output
// ============================================================================

struct BenchmarkStats {
    int parsed = 0;             // failed = total_count - parsed
    int found_results = 0;      // not_found = total_count - found_results
    double total_time_ms = 0;
    double wall_time_ms = 0;
    size_t total_count = 0;
};

static void print_summary(const BenchmarkStats& stats) {
    const int failed = stats.total_count - stats.parsed;
    const int not_found = stats.total_count - stats.found_results;

    fmt::print("\n========== Summary ==========\n");
    fmt::print("Parsed: {}\n", stats.parsed);
    fmt::print("Failed parse: {}\n", failed);
    fmt::print("Results found: {}\n", stats.found_results);
    fmt::print("Results not found: {}\n", not_found);
    fmt::print("Total formulas: {}\n", stats.total_count);
    fmt::print("Wall time: {:.2f}ms\n", stats.wall_time_ms);
    fmt::print("CPU time: {:.2f}ms\n", stats.total_time_ms);
    if (stats.total_count > 0) {
        fmt::print("Speedup: {:.2f}x\n", stats.total_time_ms / stats.wall_time_ms);
    }
    fmt::print("Avg time per formula: {:.3f}ms\n",
              stats.total_count > 0 ? stats.total_time_ms / stats.total_count : 0);

    if (failed == 0) {
        fmt_print_with_color(termcolor::green, "Status: ALL TESTS PASSED\n");
    } else {
        fmt_print_with_color(termcolor::red, "Status: SOME TESTS FAILED\n");
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    // Parse command line arguments into config
    BenchmarkConfig config;
    if (!parse_arguments(argc, argv, config)) {
        return 1;  // Error already printed by parse_arguments
    }

    // Determine which bench directories to run
    std::vector<int> bench_dirs = parse_bench_spec(config.bench_spec);
    if (bench_dirs.empty()) {
        std::cerr << "Error: bench_spec must be 'all', '1', or '2'" << std::endl;
        return 1;
    }

    // Print banner
    print_banner(config);

    // Run benchmarks
    BenchmarkRunner runner(
        config.base_dir, bench_dirs, config.start_num, config.end_num,
        config.num_jobs, config.show_progress, config.show_active_tasks, config.sleep_per_task
    );

    auto start_time = std::chrono::high_resolution_clock::now();
    auto results = runner.run();
    auto end_time = std::chrono::high_resolution_clock::now();

    // ============================================================================
    // Step 1: Collect statistics
    // ============================================================================
    BenchmarkStats stats;
    stats.total_count = results.size();
    stats.wall_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    for (const auto& r : results) {
        stats.total_time_ms += r.elapsed_ms;
        stats.parsed += r.success ? 1 : 0;

        // Check expected result
        auto expected = Synthesis::read_expected_result(config.base_dir, r.formula_num);
        stats.found_results += expected.has_value() ? 1 : 0;
    }

    // ============================================================================
    // Step 2: Print individual results
    // ============================================================================
    // Print logic: verbose mode = print all; otherwise = print only failures
    for (const auto& r : results) {
        // Skip successes in non-verbose mode
        if (!config.verbose && r.success) continue;

        if (r.success) {
            fmt_print_with_color(termcolor::green, "OK: bench{}/f{} ({}ms)\n",
                                 r.bench_dir, r.formula_num, r.elapsed_ms);
        } else {
            fmt_print_with_color(termcolor::red, "FAIL: bench{}/f{} ({})\n",
                                 r.bench_dir, r.formula_num, r.error_msg);
        }
        fmt::print("  Formula: {}\n", r.formula_str);
        fmt::print("  Partition: {}\n\n", r.partition_str);
    }

    // ============================================================================
    // Step 3: Print summary
    // ============================================================================
    print_summary(stats);

    // Cleanup
    LOG_FLUSH();
    logger::Logger::instance().get()->flush();
    spdlog::shutdown();

    return (stats.total_count > stats.parsed) ? 1 : 0;
}
