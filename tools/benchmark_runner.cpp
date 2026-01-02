/**
 * @file benchmark_runner.cpp
 * @brief SMv1000 benchmark runner for LTLf synthesis with progressive timeout
 *
 * Strategy:
 *   1. First pass: 1 minute timeout per formula
 *   2. Report results, then retry timeouts with 3 minute timeout
 *   3. Report results, then retry remaining timeouts with 5 minute timeout
 *   4. Final report with all results
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

namespace fs = std::filesystem;

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
    int timeout_stage;  // 0=first pass, 1=3min retry, 2=5min retry, -1=not timed out

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

// Track pending retries
struct PendingRetry {
    std::string folder;
    std::string filename;
    fs::path ltlf_path;
    fs::path part_path;
    int previous_stage;
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
    int stage
) {
    BenchmarkResult result;
    result.folder = folder;
    result.filename = filename;
    result.success = false;
    result.timeout_stage = stage;

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

        // Create pool and load partition
        formula::FormulaPool pool;

        if (fs::exists(part_path)) {
            pool.load_from_partition(part_path.string());
        } else {
            result.error_msg = "Cannot open .part file";
            return result;
        }

        result.num_inputs = pool.num_inputs();
        result.num_outputs = pool.num_outputs();

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
        formula::FormulaParser parser(pool);
        formula::Formula* phi = parser.parse(formula_str);

        if (!phi) {
            result.error_msg = "Parse failed";
            return result;
        }

        // Run synthesis with timeout using async
        auto task = std::async(std::launch::async, [&]() -> std::pair<bool, size_t> {
            synthesis::OnTheFlyGameSolver solver(phi, pool, pool.num_outputs(), pool.num_inputs());
            bool realizable = solver.is_realizable();
            size_t states = solver.num_expanded_states();
            return {realizable, states};
        });

        auto start = std::chrono::high_resolution_clock::now();

        // Wait for result or timeout
        if (task.wait_for(std::chrono::seconds(timeout_seconds)) == std::future_status::timeout) {
            result.error_msg = "Timeout (> " + std::to_string(timeout_seconds) + "s)";
            result.success = false;
            result.time_ms = timeout_seconds * 1000.0;
            return result;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto [realizable, states] = task.get();

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
    std::cout << "SMv1000 Benchmark Runner (Progressive)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Benchmark directory: " << benchmark_dir << std::endl;
    std::cout << "Range: " << start_idx << " to " << end_idx << std::endl;
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

    // Prepare output file
    std::string timestamp = std::to_string(std::time(nullptr));
    std::string output_csv = "benchmark_results_" + timestamp + ".csv";
    std::ofstream out_csv(output_csv);
    write_csv_header(out_csv);

    // Results storage
    std::vector<BenchmarkResult> all_results;
    std::map<std::string, BenchmarkResult> results_map;  // Key: folder/filename

    // Progressive timeout stages
    std::vector<std::pair<int, const char*>> stages = {
        {60, "1 minute"},
        {180, "3 minutes"},
        {300, "5 minutes"}
    };

    for (const auto& [timeout_sec, timeout_name] : stages) {
        std::vector<PendingRetry> pending;

        std::cout << "\n========================================" << std::endl;
        std::cout << "Stage: " << timeout_name << " timeout" << std::endl;
        std::cout << "========================================" << std::endl;

        int stage_idx = &timeout_sec - &stages[0].first;

        // Determine which files to run in this stage
        if (stage_idx == 0) {
            // First stage: run all files
            for (int i = start_idx; i < end_idx; i++) {
                const auto& [folder, ltlf_path] = files[i];
                std::string filename = ltlf_path.stem().string();
                fs::path part_path = ltlf_path.parent_path() / (filename + ".part");
                pending.push_back({folder, filename, ltlf_path, part_path, -1});
            }
        } else {
            // Subsequent stages: only run previous timeouts
            for (const auto& [folder, filename, ltlf_path, part_path, prev_stage] : pending) {
                // Skip if already succeeded
                std::string key = folder + "/" + filename;
                if (results_map.count(key) > 0 && results_map[key].success) {
                    continue;
                }
                pending.push_back({folder, filename, ltlf_path, part_path, prev_stage});
            }
            // Clear pending and rebuild from results_map
            pending.clear();
            for (const auto& [key, result] : results_map) {
                if (!result.success && result.timeout_stage == stage_idx - 1) {
                    // Re-add to pending with correct paths
                    fs::path ltlf_path = benchmark_dir + "/" + result.folder + "/" + result.filename + ".ltlf";
                    fs::path part_path = benchmark_dir + "/" + result.folder + "/" + result.filename + ".part";
                    pending.push_back({result.folder, result.filename, ltlf_path, part_path, stage_idx - 1});
                }
            }
        }

        if (pending.empty()) {
            std::cout << "No files to run in this stage." << std::endl;
            break;
        }

        std::cout << "Running " << pending.size() << " benchmarks..." << std::endl;

        // Run benchmarks for this stage
        for (const auto& [folder, filename, ltlf_path, part_path, prev_stage] : pending) {
            std::string key = folder + "/" + filename;
            std::cout << "[" << stage_idx + 1 << "] " << folder << "/" << filename << "...";

            auto result = run_benchmark_with_timeout(
                folder, filename, ltlf_path, part_path,
                expected_results, timeout_sec, stage_idx
            );

            // Update results
            results_map[key] = result;

            if (result.success) {
                out_csv << result.to_csv() << "\n";
                if (result.matches) {
                    std::cout << " " << (result.computed_realizable ? "R" : "U")
                             << " (" << std::fixed << std::setprecision(1) << result.time_ms << " ms)" << std::endl;
                } else {
                    std::cout << " MISMATCH (expected " << (result.expected_realizable ? "R" : "U")
                             << ", got " << (result.computed_realizable ? "R" : "U") << ")" << std::endl;
                }
            } else {
                std::cout << " TIMEOUT (" << result.error_msg << ")" << std::endl;
            }
        }

        // Collect all results for summary
        all_results.clear();
        for (const auto& [key, result] : results_map) {
            all_results.push_back(result);
        }

        // Write stage summary
        std::ostringstream stage_title;
        stage_title << "Stage " << stage_idx + 1 << " Summary (" << timeout_name << " timeout)";
        write_summary(all_results, std::cout, stage_title.str());
    }

    out_csv.close();

    // Final summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "FINAL RESULTS" << std::endl;
    std::cout << "========================================\n";
    write_summary(all_results, std::cout, "Final Summary (All Stages)");
    std::cout << "\nResults saved to: " << output_csv << std::endl;

    return 0;
}
