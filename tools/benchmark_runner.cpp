/**
 * @file benchmark_runner.cpp
 * @brief SMv1000 benchmark runner for LTLf synthesis with progressive timeout and multi-threading
 *
 * Strategy:
 *   1. First pass: 1 minute timeout per formula
 *   2. Report results, then retry timeouts with 3 minute timeout
 *   3. Report results, then retry remaining timeouts with 5 minute timeout
 *   4. Final report with all results
 *
 * Multi-threading:
 *   - Uses 8 concurrent threads for parallel benchmark execution
 *   - Thread-safe output and result collection
 *
 * Usage:
 *   ./benchmark_runner [benchmark_dir] [start] [end]
 */

#include "synthesis/on_the_fly_solver.hpp"
#include "formula/formula_parser.hpp"
#include "formula/formula_pool.hpp"
#include "log/logger.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <map>
#include <vector>
#include <future>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace fs = std::filesystem;

// Number of concurrent threads
constexpr int NUM_THREADS = 8;

//==============================================================================
// Result Tracking
//==============================================================================

struct BenchmarkResult {
    std::string folder;
    std::string filename;
    std::string formula;
    int num_inputs;
    int num_outputs;
    bool expected_realizable;
    bool computed_realizable;
    bool matches;
    double time_ms;
    size_t expanded_states;
    bool success;
    std::string error_msg;
    int timeout_stage;
    int index;  // Original index for ordered output

    std::string to_csv() const {
        std::ostringstream oss;
        oss << std::quoted(folder) << ","
            << std::quoted(filename) << ","
            << std::quoted(formula) << ","
            << num_inputs << ","
            << num_outputs << ","
            << (expected_realizable ? "Realizable" : "Unrealizable") << ","
            << (success ? (computed_realizable ? "Realizable" : "Unrealizable") : "TIMEOUT") << ","
            << (success ? (matches ? "PASS" : "FAIL") : "TIMEOUT") << ","
            << std::fixed << std::setprecision(2) << time_ms << ","
            << expanded_states << ","
            << (success ? "SUCCESS" : "TIMEOUT") << ","
            << std::quoted(error_msg);
        return oss.str();
    }
};

struct PendingRetry {
    std::string folder;
    std::string filename;
    fs::path ltlf_path;
    fs::path part_path;
    int previous_stage;
    int index;
};

// Thread-safe result container
class ResultContainer {
public:
    void set_result(const std::string& key, const BenchmarkResult& result) {
        std::lock_guard<std::mutex> lock(mutex_);
        results_[key] = result;
    }

    bool get_result(const std::string& key, BenchmarkResult& result) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = results_.find(key);
        if (it != results_.end()) {
            result = it->second;
            return true;
        }
        return false;
    }

    std::vector<BenchmarkResult> get_all() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<BenchmarkResult> all;
        all.reserve(results_.size());
        for (const auto& [key, result] : results_) {
            all.push_back(result);
        }
        return all;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return results_.size();
    }

    std::vector<std::pair<std::string, BenchmarkResult>> get_timeouts_at_stage(int stage) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::pair<std::string, BenchmarkResult>> timeouts;
        for (const auto& [key, result] : results_) {
            if (!result.success && result.timeout_stage == stage) {
                timeouts.push_back({key, result});
            }
        }
        return timeouts;
    }

private:
    mutable std::mutex mutex_;
    std::map<std::string, BenchmarkResult> results_;
};

// Thread-safe output with file logging
class ThreadSafeOutput {
public:
    ThreadSafeOutput() = default;

    // Initialize log file with organized directory structure
    void init_log(const std::string& benchmark_dir) {
        (void)benchmark_dir;  // Suppress unused warning
        std::lock_guard<std::mutex> lock(mutex_);

        // Create logs directory structure: logs/benchmark/YYYY-MM-DD/HH-MM/
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;
        oss << "../logs/benchmark/"
            << std::put_time(std::localtime(&time_t), "%Y-%m-%d")
            << "/"
            << std::put_time(std::localtime(&time_t), "%H-%M");

        log_dir_ = oss.str();
        fs::create_directories(log_dir_);

        // Create log filename with timestamp
        std::ostringstream name_oss;
        name_oss << "benchmark_"
                 << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
                 << ".log";
        std::string log_name = name_oss.str();

        log_path_ = log_dir_ + "/" + log_name;
        log_file_.open(log_path_, std::ios::out);

        if (log_file_.is_open()) {
            std::cout << "Log file: " << log_path_ << std::endl;
        } else {
            std::cerr << "Warning: Could not open log file: " << log_path_ << std::endl;
        }
    }

    void print(const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << msg << std::endl;
        if (log_file_.is_open()) {
            log_file_ << msg << std::endl;
            log_file_.flush();
        }
    }

    std::ostream& stream() {
        // For complex output, acquire the lock and return cout
        // File logging is done via print() method
        lock_ = std::make_unique<std::lock_guard<std::mutex>>(mutex_);
        return std::cout;
    }

    void release() {
        lock_.reset();
    }

    void close() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }

    const std::string& log_path() const { return log_path_; }

private:
    mutable std::mutex mutex_;
    std::unique_ptr<std::lock_guard<std::mutex>> lock_;
    std::ofstream log_file_;
    std::string log_dir_;
    std::string log_path_;
};

//==============================================================================
// Expected Results
//==============================================================================

std::map<std::string, bool> load_expected_results(const std::string& benchmark_dir) {
    std::map<std::string, bool> results;

    std::string results_file = benchmark_dir + "/results.csv";
    std::ifstream f(results_file);

    if (!f.is_open()) {
        std::cerr << "Warning: Could not open " << results_file << std::endl;
        return results;
    }

    std::string line;
    std::getline(f, line);  // Skip header

    while (std::getline(f, line)) {
        std::istringstream iss(line);
        std::string folder, filename, result_str;

        std::getline(iss, folder, ',');
        std::getline(iss, filename, ',');
        std::getline(iss, result_str, ',');

        std::string key = folder + "/" + filename;
        bool realizable = (result_str == "Realizable");
        results[key] = realizable;
    }

    std::cout << "Loaded " << results.size() << " expected results from " << results_file << std::endl;
    return results;
}

//==============================================================================
// Formula Preprocessing
//==============================================================================

std::string replace_implication(const std::string& formula) {
    std::string result = formula;
    size_t pos = 0;

    while ((pos = result.find("->")) != std::string::npos) {
        size_t left_start = pos;
        int paren_count = 0;
        bool found = false;

        for (int i = pos - 1; i >= 0; i--) {
            char c = result[i];
            if (c == ')') paren_count++;
            else if (c == '(') {
                paren_count--;
                if (paren_count < 0) {
                    left_start = i;
                    found = true;
                    break;
                }
            }
        }

        if (!found && left_start == pos) {
            left_start = 0;
        }

        size_t right_end = pos + 2;
        paren_count = 0;
        found = false;

        for (size_t i = pos + 2; i < result.length(); i++) {
            char c = result[i];
            if (c == '(') paren_count++;
            else if (c == ')') {
                paren_count--;
                if (paren_count < 0) {
                    right_end = i;
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            right_end = result.length();
        }

        std::string left = result.substr(left_start, pos - left_start);
        std::string right = result.substr(pos + 2, right_end - (pos + 2));
        std::string replacement = "(!(" + left + ")) | (" + right + ")";

        result.replace(left_start, right_end - left_start, replacement);
    }

    return result;
}

//==============================================================================
// Benchmark Execution with Timeout
//==============================================================================

BenchmarkResult run_benchmark_with_timeout(
    const std::string& folder,
    const std::string& filename,
    const fs::path& ltlf_path,
    const fs::path& part_path,
    const std::map<std::string, bool>& expected_results,
    int timeout_seconds,
    int stage,
    int index
) {
    BenchmarkResult result;
    result.folder = folder;
    result.filename = filename;
    result.success = false;
    result.timeout_stage = stage;
    result.index = index;

    try {
        // Read formula
        std::ifstream ltlf_file(ltlf_path);
        if (!ltlf_file.is_open()) {
            result.error_msg = "Cannot open .ltlf file";
            return result;
        }

        std::string formula_str;
        std::getline(ltlf_file, formula_str);
        formula_str = replace_implication(formula_str);
        result.formula = formula_str;

        // Create pool on heap with shared_ptr to keep it alive for detached threads
        auto pool_ptr = std::make_shared<formula::FormulaPool>();

        if (fs::exists(part_path)) {
            pool_ptr->load_from_partition(part_path.string());
        } else {
            result.error_msg = "Cannot open .part file";
            return result;
        }

        result.num_inputs = pool_ptr->num_inputs();
        result.num_outputs = pool_ptr->num_outputs();

        // Get expected result
        std::string key = folder + "/" + filename;
        auto it = expected_results.find(key);
        if (it != expected_results.end()) {
            result.expected_realizable = it->second;
        } else {
            result.expected_realizable = false;
            result.error_msg = "No expected result found";
        }

        // Parse formula
        formula::FormulaParser parser(*pool_ptr);
        formula::Formula* phi = parser.parse(formula_str);

        if (!phi) {
            result.error_msg = "Parse failed";
            return result;
        }

        // Run synthesis with timeout using thread (not async to avoid destructor blocking)
        // Use shared_ptr to keep pool, phi, syn_result, and done alive for detached threads
        auto syn_result_ptr = std::make_shared<std::pair<bool, size_t>>(false, 0);
        auto done_ptr = std::make_shared<std::atomic<bool>>(false);
        auto start = std::chrono::high_resolution_clock::now();

        std::thread solver_thread([pool_ptr, phi, syn_result_ptr, done_ptr]() {
            synthesis::OnTheFlyGameSolver solver(phi, *pool_ptr, pool_ptr->num_outputs(), pool_ptr->num_inputs());
            syn_result_ptr->first = solver.is_realizable();
            syn_result_ptr->second = solver.num_expanded_states();
            done_ptr->store(true);
        });

        // Wait for result or timeout with minimal polling overhead
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout_seconds);
        while (!done_ptr->load() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        auto end = std::chrono::high_resolution_clock::now();

        if (!done_ptr->load()) {
            // Timeout - detach the thread and let it run in background
            // shared_ptr will keep pool and results alive until thread completes
            solver_thread.detach();
            result.error_msg = "Timeout (> " + std::to_string(timeout_seconds) + "s)";
            result.success = false;
            result.time_ms = timeout_seconds * 1000.0;
            return result;
        }

        // Task completed, join the thread
        solver_thread.join();
        auto [realizable, states] = *syn_result_ptr;

        result.computed_realizable = realizable;
        result.matches = (result.expected_realizable == realizable);
        result.time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        result.expanded_states = states;
        result.success = true;

    } catch (const std::exception& e) {
        result.error_msg = e.what();
        result.success = false;
    }

    return result;
}

//==============================================================================
// Reporting
//==============================================================================

void write_csv_header(std::ofstream& out) {
    out << "Folder,Filename,Formula,Inputs,Outputs,Expected,Computed,Match,TimeMs,ExpandedStates,Status,Error\n";
}

void write_summary(const std::vector<BenchmarkResult>& results, std::ostream& out, const std::string& title) {
    int total = static_cast<int>(results.size());
    int passed = 0;
    int failed = 0;
    int timeouts = 0;
    int errors = 0;
    double total_time = 0;
    int true_positives = 0;
    int true_negatives = 0;
    int false_positives = 0;
    int false_negatives = 0;

    for (const auto& r : results) {
        if (!r.success) {
            timeouts++;
        } else if (r.error_msg == "No expected result found") {
            errors++;
        } else if (r.matches) {
            passed++;
            if (r.computed_realizable) {
                true_positives++;
            } else {
                true_negatives++;
            }
        } else {
            failed++;
            if (r.computed_realizable) {
                false_positives++;
            } else {
                false_negatives++;
            }
        }
        total_time += r.time_ms;
    }

    out << "\n========================================\n";
    out << title << "\n";
    out << "========================================\n";
    out << "Total: " << total << "\n";
    out << "Passed: " << passed << " (" << (total > 0 ? 100.0 * passed / total : 0) << "%)\n";
    out << "Failed: " << failed << " (" << (total > 0 ? 100.0 * failed / total : 0) << "%)\n";
    out << "Timeouts: " << timeouts << " (" << (total > 0 ? 100.0 * timeouts / total : 0) << "%)\n";
    out << "Errors: " << errors << " (" << (total > 0 ? 100.0 * errors / total : 0) << "%)\n";
    out << "Total Time: " << std::fixed << std::setprecision(2) << total_time << " ms ("
        << (total_time / 1000) << " s)\n";
    out << "\nConfusion Matrix:\n";
    out << "  True Positives:  " << true_positives << " (correctly Realizable)\n";
    out << "  True Negatives:  " << true_negatives << " (correctly Unrealizable)\n";
    out << "  False Positives: " << false_positives << " (incorrectly Realizable)\n";
    out << "  False Negatives: " << false_negatives << " (incorrectly Unrealizable)\n";

    if (passed + failed > 0) {
        double accuracy = 100.0 * passed / (passed + failed);
        out << "Accuracy: " << std::fixed << std::setprecision(2) << accuracy << "%\n";
    }
}

//==============================================================================
// Parallel Benchmark Execution
//==============================================================================

void run_stage_parallel(
    const std::vector<PendingRetry>& pending,
    const std::map<std::string, bool>& expected_results,
    int timeout_seconds,
    int stage,
    ResultContainer& results,
    std::ofstream& out_csv,
    std::ofstream& out_csv_local,
    ThreadSafeOutput& output
) {
    std::atomic<int> completed(0);
    std::atomic<int> passed(0);
    std::atomic<int> failed(0);
    std::atomic<int> timed_out(0);
    const int total = static_cast<int>(pending.size());

    // Process in batches
    for (size_t batch_start = 0; batch_start < pending.size(); batch_start += NUM_THREADS) {
        std::vector<std::future<BenchmarkResult>> futures;

        // Create batch of futures
        size_t batch_end = std::min(batch_start + NUM_THREADS, pending.size());
        for (size_t i = batch_start; i < batch_end; ++i) {
            const auto& p = pending[i];
            std::string key = p.folder + "/" + p.filename;

            // Capture by value to avoid dangling reference
            futures.push_back(std::async(std::launch::async,
                [folder = p.folder, filename = p.filename, ltlf_path = p.ltlf_path,
                 part_path = p.part_path, idx = p.index,
                 &expected_results, timeout_seconds, stage]() {
                    return run_benchmark_with_timeout(
                        folder, filename, ltlf_path, part_path,
                        expected_results, timeout_seconds, stage, idx
                    );
                }));
        }

        // Wait for batch completion and collect results
        for (size_t i = 0; i < futures.size(); ++i) {
            size_t orig_idx = batch_start + i;
            const auto& p = pending[orig_idx];
            std::string key = p.folder + "/" + p.filename;

            BenchmarkResult result = futures[i].get();
            results.set_result(key, result);

            int done = ++completed;
            int current_stage_num = stage + 1;

            if (result.success) {
                out_csv << result.to_csv() << "\n";
                out_csv_local << result.to_csv() << "\n";
                out_csv.flush();
                out_csv_local.flush();

                if (result.matches) {
                    ++passed;
                    std::ostringstream msg;
                    msg << "[" << current_stage_num << "][" << done << "/" << total << "] "
                        << p.folder << "/" << p.filename << ": "
                        << (result.computed_realizable ? "R" : "U")
                        << " (" << std::fixed << std::setprecision(1) << result.time_ms << " ms)";
                    output.print(msg.str());
                } else {
                    ++failed;
                    std::ostringstream msg;
                    msg << "[" << current_stage_num << "][" << done << "/" << total << "] "
                        << p.folder << "/" << p.filename << ": MISMATCH "
                        << "(expected " << (result.expected_realizable ? "R" : "U")
                        << ", got " << (result.computed_realizable ? "R" : "U") << ")";
                    output.print(msg.str());
                }
            } else {
                ++timed_out;
                std::ostringstream msg;
                msg << "[" << current_stage_num << "][" << done << "/" << total << "] "
                    << p.folder << "/" << p.filename << ": TIMEOUT (" << result.error_msg << ")";
                output.print(msg.str());
            }
        }
    }

    // Print batch summary
    std::ostringstream summary;
    summary << "Stage " << (stage + 1) << " completed: "
            << passed << " passed, " << failed << " failed, " << timed_out << " timed out";
    output.print(summary.str());
}

//==============================================================================
// Main
//==============================================================================

int main(int argc, char* argv[]) {
    std::string benchmark_dir = "benchmarks/sm1000";
    int start_idx = 0;
    int end_idx = 1000;

    if (argc >= 2) {
        benchmark_dir = argv[1];
    }
    if (argc >= 3) {
        start_idx = std::atoi(argv[2]);
    }
    if (argc >= 4) {
        end_idx = std::atoi(argv[3]);
    }

    std::cout << "========================================" << std::endl;
    std::cout << "SMv1000 Benchmark Runner (Progressive + Multi-thread)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Benchmark directory: " << benchmark_dir << std::endl;
    std::cout << "Range: " << start_idx << " to " << end_idx << std::endl;
    std::cout << "Threads: " << NUM_THREADS << std::endl;
    std::cout << "Timeout strategy: 1min -> 3min -> 5min" << std::endl;
    std::cout << "========================================" << std::endl;

    logger::Logger::instance();

    // Load expected results
    auto expected_results = load_expected_results(benchmark_dir);

    // Collect all benchmark files
    std::vector<std::pair<std::string, fs::path>> files;
    for (const auto& entry : fs::directory_iterator(benchmark_dir)) {
        if (entry.is_directory()) {
            std::string folder = entry.path().filename().string();
            for (const auto& file : fs::directory_iterator(entry.path())) {
                if (file.path().extension() == ".ltlf") {
                    std::string filename = file.path().stem().string();
                    files.push_back({folder, file.path()});
                }
            }
        }
    }

    std::sort(files.begin(), files.end());

    std::cout << "Found " << files.size() << " benchmark files" << std::endl;

    // Apply range
    start_idx = std::max(0, std::min(start_idx, static_cast<int>(files.size())));
    end_idx = std::max(start_idx, std::min(end_idx, static_cast<int>(files.size())));

    // Create results directory with same structure as logs
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream results_oss;
    results_oss << "../results/benchmark/"
                 << std::put_time(std::localtime(&time_t), "%Y-%m-%d")
                 << "/"
                 << std::put_time(std::localtime(&time_t), "%H-%M");

    std::string results_dir = results_oss.str();
    fs::create_directories(results_dir);

    std::ostringstream name_oss;
    name_oss << "benchmark_results_"
             << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
             << ".csv";

    // Prepare output file in results directory
    std::string output_csv = results_dir + "/" + name_oss.str();
    std::ofstream out_csv(output_csv);
    write_csv_header(out_csv);

    // Also save a copy in build directory for convenience
    std::string local_csv = "benchmark_results_latest.csv";
    std::ofstream out_csv_local(local_csv);
    write_csv_header(out_csv_local);

    std::cout << "Results CSV: " << output_csv << std::endl;

    ResultContainer results;
    ThreadSafeOutput output;
    output.init_log(benchmark_dir);
    std::vector<PendingRetry> all_pending;

    // Progressive timeout stages
    std::vector<std::pair<int, const char*>> stages = {
        {60, "1 minute"},
        {180, "3 minutes"},
        {300, "5 minutes"}
    };

    for (const auto& [timeout_sec, timeout_name] : stages) {
        std::vector<PendingRetry> pending;

        output.print("\n========================================");
        output.print(std::string("Stage: ") + timeout_name + " timeout");
        output.print("========================================");

        int stage_idx = &timeout_sec - &stages[0].first;

        // Determine which files to run in this stage
        if (stage_idx == 0) {
            // First stage: run all files
            for (int i = start_idx; i < end_idx; i++) {
                const auto& [folder, ltlf_path] = files[i];
                std::string filename = ltlf_path.stem().string();
                fs::path part_path = ltlf_path.parent_path() / (filename + ".part");
                pending.push_back({folder, filename, ltlf_path, part_path, -1, i});
                all_pending.push_back({folder, filename, ltlf_path, part_path, -1, i});
            }
        } else {
            // Subsequent stages: only run previous timeouts
            auto timeouts = results.get_timeouts_at_stage(stage_idx - 1);
            for (const auto& [key, timeout_result] : timeouts) {
                // Find the original pending entry
                for (const auto& orig : all_pending) {
                    std::string orig_key = orig.folder + "/" + orig.filename;
                    if (orig_key == key) {
                        fs::path ltlf_path = benchmark_dir + "/" + orig.folder + "/" + orig.filename + ".ltlf";
                        fs::path part_path = benchmark_dir + "/" + orig.folder + "/" + orig.filename + ".part";
                        pending.push_back({orig.folder, orig.filename, ltlf_path, part_path, stage_idx - 1, orig.index});
                        break;
                    }
                }
            }
        }

        if (pending.empty()) {
            std::cout << "No files to run in this stage." << std::endl;
            break;
        }

        std::cout << "Running " << pending.size() << " benchmarks (using " << NUM_THREADS << " threads)..." << std::endl;

        // Run benchmarks in parallel
        run_stage_parallel(pending, expected_results, timeout_sec, stage_idx, results, out_csv, out_csv_local, output);

        // Collect all results for summary
        auto all_results = results.get_all();

        // Write stage summary
        std::ostringstream stage_title;
        stage_title << "Stage " << stage_idx + 1 << " Summary (" << timeout_name << " timeout)";
        write_summary(all_results, std::cout, stage_title.str());
    }

    out_csv.close();

    // Final summary
    auto final_results = results.get_all();

    // Sort by index for ordered final summary
    std::sort(final_results.begin(), final_results.end(),
        [](const BenchmarkResult& a, const BenchmarkResult& b) {
            return a.index < b.index;
        });

    output.print("\n========================================");
    output.print("FINAL RESULTS");
    output.print("========================================");
    write_summary(final_results, std::cout, "Final Summary (All Stages)");
    output.print("\nResults saved to: " + output_csv);

    output.close();

    return 0;
}
