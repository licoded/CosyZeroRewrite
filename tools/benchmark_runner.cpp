/**
 * @file benchmark_runner.cpp
 * @brief SMv1000 benchmark runner for LTLf synthesis
 *
 * Usage:
 *   ./benchmark_runner [benchmark_dir] [start] [end]
 *
 * Example:
 *   ./benchmark_runner benchmarks/sm1000 0 100  # Run first 100 benchmarks
 *   ./benchmark_runner benchmarks/sm1000          # Run all 1000 benchmarks
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

    std::string to_csv() const {
        std::ostringstream oss;
        oss << std::quoted(folder) << ","
            << std::quoted(filename) << ","
            << std::quoted(formula) << ","
            << num_inputs << ","
            << num_outputs << ","
            << (expected_realizable ? "Realizable" : "Unrealizable") << ","
            << (computed_realizable ? "Realizable" : "Unrealizable") << ","
            << (matches ? "PASS" : "FAIL") << ","
            << std::fixed << std::setprecision(2) << time_ms << ","
            << expanded_states << ","
            << (success ? "SUCCESS" : "ERROR") << ","
            << std::quoted(error_msg);
        return oss.str();
    }
};

// Expected results from results.csv
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

// Preprocess formula: replace -> (implication) with | (or)
// a -> b becomes !a | b
std::string preprocess_formula(const std::string& formula) {
    std::string result;
    size_t i = 0;

    while (i < formula.length()) {
        // Check for implication operator ->
        if (i + 1 < formula.length() && formula[i] == '-' && formula[i + 1] == '>') {
            // Need to find the left operand and wrap it in not()
            // For simplicity, we'll just replace -> with | (or) and add not handling in a more complex way
            // Actually, -> is right-associative, so we need to handle it properly
            // For now, let's just mark it for manual review
            result += " | ";  // Placeholder - real implementation would need full parser
            i += 2;
        } else {
            result += formula[i];
            i++;
        }
    }

    return result;
}

// Simple -> replacement: replace occurrences of "(expr1) -> (expr2)" with "(!(expr1)) | (expr2)"
// This is a simplified approach - works for well-formed formulas
std::string replace_implication(const std::string& formula) {
    std::string result = formula;
    size_t pos = 0;

    // Keep replacing until no more -> found
    while ((pos = result.find("->")) != std::string::npos) {
        // Find the left operand (going backwards from ->)
        size_t left_start = pos;
        int paren_count = 0;
        bool found = false;

        // Find the start of the left operand
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
            // No parentheses found, use the entire string before ->
            left_start = 0;
        }

        // Find the right operand (going forward from ->)
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

        // Extract left and right operands
        std::string left = result.substr(left_start, pos - left_start);
        std::string right = result.substr(pos + 2, right_end - (pos + 2));

        // Replace: -> with |, and wrap left in !
        std::string replacement = "(!(" + left + ")) | (" + right + ")";

        result.replace(left_start, right_end - left_start, replacement);
    }

    return result;
}

//==============================================================================
// Benchmark Execution
//==============================================================================

BenchmarkResult run_benchmark(
    const std::string& folder,
    const std::string& filename,
    const fs::path& ltlf_path,
    const fs::path& part_path,
    const std::map<std::string, bool>& expected_results
) {
    BenchmarkResult result;
    result.folder = folder;
    result.filename = filename;
    result.success = false;

    try {
        // Read formula
        std::ifstream ltlf_file(ltlf_path);
        if (!ltlf_file.is_open()) {
            result.error_msg = "Cannot open .ltlf file";
            return result;
        }

        std::string formula_str;
        std::getline(ltlf_file, formula_str);

        // Preprocess formula: replace -> (implication) with equivalent
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
            result.expected_realizable = false;  // Default
            result.error_msg = "No expected result found";
        }

        // Parse formula
        formula::FormulaParser parser(pool);
        formula::Formula* phi = parser.parse(formula_str);

        if (!phi) {
            result.error_msg = "Parse failed";
            return result;
        }

        // Run synthesis with timing
        auto start = std::chrono::high_resolution_clock::now();
        bool realizable = synthesis::is_realizable_on_the_fly(phi, pool);
        auto end = std::chrono::high_resolution_clock::now();

        result.computed_realizable = realizable;
        result.matches = (result.expected_realizable == realizable);
        result.time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        result.success = true;

        // Get expanded states count (if available)
        // Note: OnTheFlyGameSolver doesn't expose this in the current API,
        // so we skip it for now

    } catch (const std::exception& e) {
        result.error_msg = e.what();
        result.success = false;
    }

    return result;
}

//==============================================================================
// CSV Output
//==============================================================================

void write_csv_header(std::ofstream& out) {
    out << "Folder,Filename,Formula,Inputs,Outputs,Expected,Computed,Match,TimeMs,ExpandedStates,Status,Error\n";
}

void write_summary(const std::vector<BenchmarkResult>& results, std::ostream& out) {
    int total = static_cast<int>(results.size());
    int passed = 0;
    int failed = 0;
    int errors = 0;
    double total_time = 0;
    int true_positives = 0;  // Correctly identified as Realizable
    int true_negatives = 0;  // Correctly identified as Unrealizable
    int false_positives = 0; // Incorrectly identified as Realizable
    int false_negatives = 0; // Incorrectly identified as Unrealizable

    for (const auto& r : results) {
        if (!r.success) {
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
    out << "Summary\n";
    out << "========================================\n";
    out << "Total: " << total << "\n";
    out << "Passed: " << passed << " (" << (100.0 * passed / total) << "%)\n";
    out << "Failed: " << failed << " (" << (100.0 * failed / total) << "%)\n";
    out << "Errors: " << errors << " (" << (100.0 * errors / total) << "%)\n";
    out << "Total Time: " << std::fixed << std::setprecision(2) << total_time << " ms\n";
    out << "Average Time: " << (total_time / total) << " ms\n";
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
    int end_idx = 1000;  // Default: run all

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
    std::cout << "SMv1000 Benchmark Runner" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Benchmark directory: " << benchmark_dir << std::endl;
    std::cout << "Range: " << start_idx << " to " << end_idx << std::endl;

    // Initialize logging (minimal output)
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

    // Sort files for consistent ordering
    std::sort(files.begin(), files.end());

    std::cout << "Found " << files.size() << " benchmark files" << std::endl;

    // Apply range
    start_idx = std::max(0, std::min(start_idx, static_cast<int>(files.size())));
    end_idx = std::max(start_idx, std::min(end_idx, static_cast<int>(files.size())));

    std::cout << "Running " << (end_idx - start_idx) << " benchmarks..." << std::endl;

    // Prepare output file
    std::string timestamp = std::to_string(std::time(nullptr));
    std::string output_csv = "benchmark_results_" + timestamp + ".csv";
    std::ofstream out_csv(output_csv);
    write_csv_header(out_csv);

    // Run benchmarks
    std::vector<BenchmarkResult> results;

    for (int i = start_idx; i < end_idx; i++) {
        const auto& [folder, ltlf_path] = files[i];
        std::string filename = ltlf_path.stem().string();
        fs::path part_path = ltlf_path.parent_path() / (filename + ".part");

        std::cout << "[" << (i + 1) << "/" << end_idx << "] " << folder << "/" << filename << "...";

        auto result = run_benchmark(folder, filename, ltlf_path, part_path, expected_results);
        results.push_back(result);

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
            std::cout << " ERROR: " << result.error_msg << std::endl;
        }
    }

    out_csv.close();

    // Write summary
    write_summary(results, std::cout);

    std::cout << "\nResults saved to: " << output_csv << std::endl;

    return 0;
}
