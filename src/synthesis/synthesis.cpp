#include "synthesis/synthesis.hpp"
#include "synthesis/game_solver.hpp"
#include "formula/formula_z3.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace synthesis {

formula::Formula* Synthesis::parse_formula(const std::string& formula_str, formula::FormulaPool& pool) {
    formula::FormulaParser parser(pool);
    formula::Formula* f = parser.parse(formula_str);
    if (parser.has_error()) {
        return nullptr;
    }
    return f;
}

bool Synthesis::load_partition(const std::string& part_file,
                               std::vector<std::string>& outputs,
                               std::vector<std::string>& inputs) {
    std::ifstream file(part_file);
    if (!file.is_open()) {
        return false;
    }

    outputs.clear();
    inputs.clear();

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (keyword == ".outputs:") {
            std::string var;
            while (iss >> var) {
                outputs.push_back(var);
            }
        } else if (keyword == ".inputs:") {
            std::string var;
            while (iss >> var) {
                inputs.push_back(var);
            }
        }
    }

    return !outputs.empty() || !inputs.empty();
}

std::optional<bool> Synthesis::is_satisfiable([[maybe_unused]] formula::Formula* f) {
#ifdef FORMULA_USE_Z3
    return formula::FormulaZ3::is_satisfiable(f, 0, 1000);  // 1s timeout for benchmark
#else
    return std::nullopt;
#endif
}

std::optional<bool> Synthesis::is_valid([[maybe_unused]] formula::Formula* f) {
#ifdef FORMULA_USE_Z3
    return formula::FormulaZ3::is_valid(f, 0, 10000);
#else
    return std::nullopt;
#endif
}

std::optional<bool> Synthesis::is_realizable([[maybe_unused]] formula::Formula* f) {
    // NOT IMPLEMENTED - This is a placeholder kept for API compatibility
    // Use is_realizable_with_partition() for actual realizability checking
    return std::nullopt;
}

std::optional<bool> Synthesis::is_realizable_with_partition(
    formula::Formula* f,
    [[maybe_unused]] const std::vector<std::string>& output_vars,
    [[maybe_unused]] const std::vector<std::string>& input_vars,
    formula::FormulaPool& pool) {
    // TODO: Use output_vars and input_vars for variable partitioning
    // For now, GameSolver uses default partitioning from the formula
    return GameSolver::is_realizable(f, pool);
}

bool Synthesis::read_benchmark(const std::string& base_dir, int bench_num,
                               std::string& formula_str,
                               std::vector<std::string>& outputs,
                               std::vector<std::string>& inputs,
                               int* bench_dir) {
    // Determine which bench directory (1 or 2)
    // bench1: f1-f500, bench2: f501-f1000
    int dir = (bench_num <= 500) ? 1 : 2;
    if (bench_dir) *bench_dir = dir;

    std::string ltlf_file = base_dir + "/bench" + std::to_string(dir) + "/f" + std::to_string(bench_num) + ".ltlf";
    std::string part_file = base_dir + "/bench" + std::to_string(dir) + "/f" + std::to_string(bench_num) + ".part";

    // Read formula
    std::ifstream ltlf(ltlf_file);
    if (!ltlf.is_open()) {
        return false;
    }

    std::getline(ltlf, formula_str);

    // Read partition
    if (!load_partition(part_file, outputs, inputs)) {
        // Default: all variables are outputs
        outputs.clear();
        inputs.clear();
    }

    return true;
}

bool Synthesis::read_benchmark_from_dir(const std::string& base_dir, int bench_dir, int bench_num,
                                        std::string& formula_str,
                                        std::vector<std::string>& outputs,
                                        std::vector<std::string>& inputs) {
    std::string ltlf_file = base_dir + "/bench" + std::to_string(bench_dir) + "/f" + std::to_string(bench_num) + ".ltlf";
    std::string part_file = base_dir + "/bench" + std::to_string(bench_dir) + "/f" + std::to_string(bench_num) + ".part";

    // Read formula
    std::ifstream ltlf(ltlf_file);
    if (!ltlf.is_open()) {
        return false;
    }

    std::getline(ltlf, formula_str);

    // Read partition
    if (!load_partition(part_file, outputs, inputs)) {
        // Default: all variables are outputs
        outputs.clear();
        inputs.clear();
    }

    return true;
}

std::optional<bool> Synthesis::read_expected_result(const std::string& base_dir, int bench_num) {
    std::string results_file = base_dir + "/results.csv";
    std::ifstream file(results_file);

    if (!file.is_open()) {
        return std::nullopt;
    }

    std::string line;
    // Skip header
    std::getline(file, line);

    std::string target_filename = "f" + std::to_string(bench_num);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string folder, filename, result;
        if (!std::getline(iss, folder, ',')) continue;
        if (!std::getline(iss, filename, ',')) continue;
        if (!std::getline(iss, result, ',')) continue;

        if (filename == target_filename) {
            // Trim whitespace and check
            if (result == "Realizable") return true;
            if (result == "Unrealizable") return false;
        }
    }

    return std::nullopt;
}

} // namespace synthesis
