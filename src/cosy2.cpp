/**
 * @file cosy2.cpp
 * @brief CosyZero LTLf Synthesis Tool - Main Entry Point
 *
 * Usage:
 *   Cosy2 -f formula.ltlf [-p partition.part]
 *   Cosy2 "G (req -> F ack)"
 */

#include "synthesis/on_the_fly_solver.hpp"
#include "formula/formula_parser.hpp"
#include "formula/formula_pool.hpp"
#include "log/logger.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <chrono>
#include <iomanip>
#include <ctime>

using namespace formula;
using namespace synthesis;

//==============================================================================
// File Reading Helpers
//==============================================================================

static std::string read_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file: " << filename << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

//==============================================================================
// Partition File Parser
//==============================================================================

struct Partition {
    std::vector<std::string> outputs;
    std::vector<std::string> inputs;
};

static Partition parse_partition_file(const std::string& filename) {
    Partition result;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: Cannot open partition file: " << filename << std::endl;
        return result;
    }

    enum class Section { None, Outputs, Inputs };
    Section section = Section::None;
    std::string line;

    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        // Remove leading/trailing whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        size_t end = line.find_last_not_of(" \t\r\n");
        if (end != std::string::npos) line = line.substr(0, end + 1);

        // Check for section headers (with or without dot, with or without colon)
        std::string upper_line = line;
        for (char& c : upper_line) c = std::toupper(c);

        if (upper_line == ".OUTPUTS:" || upper_line == "OUTPUTS:" ||
            upper_line == ".OUTPUTS" || upper_line == "OUTPUTS") {
            section = Section::Outputs;
            continue;
        } else if (upper_line == ".INPUTS:" || upper_line == "INPUTS:" ||
                   upper_line == ".INPUTS" || upper_line == "INPUTS") {
            section = Section::Inputs;
            continue;
        }

        // Check if line starts with a section header followed by variables
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string prefix = line.substr(0, colon_pos);
            std::string upper_prefix = prefix;
            for (char& c : upper_prefix) c = std::toupper(c);

            if (upper_prefix == ".OUTPUTS" || upper_prefix == "OUTPUTS") {
                section = Section::Outputs;
                // Extract variables after colon
                std::string rest = line.substr(colon_pos + 1);
                std::istringstream iss(rest);
                std::string token;
                while (iss >> token) {
                    if (!token.empty()) result.outputs.push_back(token);
                }
                continue;
            } else if (upper_prefix == ".INPUTS" || upper_prefix == "INPUTS") {
                section = Section::Inputs;
                // Extract variables after colon
                std::string rest = line.substr(colon_pos + 1);
                std::istringstream iss(rest);
                std::string token;
                while (iss >> token) {
                    if (!token.empty()) result.inputs.push_back(token);
                }
                continue;
            }
        }

        // Variable name (standalone, not on same line as section header)
        if (section == Section::Outputs) {
            std::istringstream iss(line);
            std::string token;
            while (iss >> token) {
                if (!token.empty()) result.outputs.push_back(token);
            }
        } else if (section == Section::Inputs) {
            std::istringstream iss(line);
            std::string token;
            while (iss >> token) {
                if (!token.empty()) result.inputs.push_back(token);
            }
        }
    }

    std::cout << "Parsed partition: " << result.outputs.size()
              << " outputs, " << result.inputs.size() << " inputs" << std::endl;

    return result;
}

//==============================================================================
// Formula Cleaning (remove comments and extra whitespace)
//==============================================================================

static std::string clean_formula(const std::string& raw) {
    std::string result;
    bool in_comment = false;

    for (size_t i = 0; i < raw.size(); ++i) {
        if (in_comment) {
            if (raw[i] == '\n') in_comment = false;
            continue;
        }

        if (raw[i] == '#') {
            in_comment = true;
            continue;
        }

        // Replace newlines with spaces
        if (raw[i] == '\n' || raw[i] == '\r') {
            result += ' ';
        } else {
            result += raw[i];
        }
    }

    // Trim leading/trailing whitespace
    size_t start = result.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = result.find_last_not_of(" \t\n\r");
    return result.substr(start, end - start + 1);
}

//==============================================================================
// Main
//==============================================================================

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "   CosyZero LTLf Synthesis Tool v2.0" << std::endl;
    std::cout << "========================================" << std::endl;

    // Parse command line arguments
    std::string formula_str;
    std::string formula_file;
    std::string partition_file;
    std::string trace_dir;  // Empty means use default
    bool enable_trace = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-f" && i + 1 < argc) {
            formula_file = argv[++i];
        } else if (arg == "-p" && i + 1 < argc) {
            partition_file = argv[++i];
        } else if (arg == "--trace") {
            enable_trace = true;
            // Optional: custom trace directory
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                trace_dir = argv[++i];
            }
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: Cosy2 [options]" << std::endl;
            std::cout << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -f <file>    Read formula from file" << std::endl;
            std::cout << "  -p <file>    Read variable partition from file" << std::endl;
            std::cout << "  --trace [dir] Enable trace recording for visualization" << std::endl;
            std::cout << "                (default dir: results/trace_*/)" << std::endl;
            std::cout << "  -h, --help   Show this help message" << std::endl;
            std::cout << std::endl;
            std::cout << "If no -f is specified, the first non-option argument" << std::endl;
            std::cout << "is treated as a formula string." << std::endl;
            std::cout << std::endl;
            std::cout << "Examples:" << std::endl;
            std::cout << "  Cosy2 -f response.ltlf -p response.part" << std::endl;
            std::cout << "  Cosy2 \"G (req -> F ack)\"" << std::endl;
            std::cout << "  Cosy2 -f formula.ltlf --trace" << std::endl;
            return 0;
        } else if (arg[0] != '-') {
            // Non-option argument: treat as formula string
            formula_str = arg;
        }
    }

    // Read formula from file if specified
    if (!formula_file.empty()) {
        std::string raw = read_file(formula_file);
        if (raw.empty()) {
            std::cerr << "Error: Failed to read formula file" << std::endl;
            return 1;
        }
        formula_str = clean_formula(raw);
        std::cout << "Formula from file: " << formula_file << std::endl;
    }

    // Check if formula is provided
    if (formula_str.empty()) {
        std::cerr << "Error: No formula provided" << std::endl;
        std::cerr << "Usage: Cosy2 -f <file> | Cosy2 \"<formula>\"" << std::endl;
        return 1;
    }

    std::cout << "Formula: " << formula_str << std::endl;

    // Parse partition if provided
    Partition partition;
    if (!partition_file.empty()) {
        partition = parse_partition_file(partition_file);
    }

    // Create formula pool and declare variables
    FormulaPool pool;
    if (!partition.outputs.empty() || !partition.inputs.empty()) {
        pool.declare_variables(partition.outputs, partition.inputs);
        std::cout << "Variables declared: "
                  << pool.num_outputs() << " outputs, "
                  << pool.num_inputs() << " inputs" << std::endl;
    }

    // Parse formula
    FormulaParser parser(pool);
    // Parser now auto-loads variables from pool, no need for set_variables()

    Formula* phi = parser.parse(formula_str);
    if (!phi) {
        std::cerr << "Error: Failed to parse formula" << std::endl;
        std::cerr << "Parser error: " << parser.error() << std::endl;
        return 1;
    }

    std::cout << "Parsed: " << phi->to_string() << std::endl;
    std::cout << "Parsed (with names): " << phi->to_string_with_names(pool) << std::endl;

    // Show variable mapping
    std::cout << "Variable mapping:" << std::endl;
    for (int i = 0; i < pool.num_outputs() + pool.num_inputs(); ++i) {
        std::string var_name = pool.get_variable_name(i);
        std::cout << "  v" << i << " = " << var_name;
        if (i < pool.num_outputs()) {
            std::cout << " (output)";
        } else {
            std::cout << " (input)";
        }
        std::cout << std::endl;
    }

    // Run synthesis
    std::cout << "Running on-the-fly synthesis..." << std::endl;

    // Create solver directly (not using convenience function)
    // so we can access the solver for game graph export
    int num_outputs = pool.num_outputs();
    int num_inputs = pool.num_inputs();

    // If variables not declared, extract them from the formula
    if (num_outputs == 0 && num_inputs == 0) {
        std::unordered_set<int> vars;
        std::function<void(Formula*)> collect = [&](Formula* f) {
            if (!f) return;
            if (f->op() == Formula::OpType::Literal) {
                vars.insert(f->var_id());
            } else {
                collect(f->left());
                collect(f->right());
            }
        };
        collect(phi);
        num_outputs = static_cast<int>(vars.size());
    }

    OnTheFlyGameSolver solver(phi, pool, num_outputs, num_inputs);

    // Enable trace if requested
    if (enable_trace) {
        solver.enable_trace(trace_dir);
        std::cout << "Trace recording enabled..." << std::endl;
    }

    bool realizable = solver.is_realizable();

    // Export game graph if environment variable is set
    const char* debug_graph = std::getenv("COSY_DEBUG_GAME_GRAPH");
    if (debug_graph && std::string(debug_graph) == "1") {
        std::cout << "Exporting game graph..." << std::endl;

        // Generate timestamp for filename
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now;
        localtime_r(&time_t_now, &tm_now);

        // Format: results/game_graph/YYYY-MM-DD/HH-period/game_graph_YYYYMMDD_HHMMSS
        char time_buf[64];
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d", &tm_now);
        std::string date_dir = time_buf;

        std::strftime(time_buf, sizeof(time_buf), "%H", &tm_now);
        int hour = std::atoi(time_buf);

        // Period: AM (00-11) or PM (12-23)
        std::string period = (hour < 12) ? "AM" : "PM";
        std::strftime(time_buf, sizeof(time_buf), "%I", &tm_now);
        std::string hour_str = time_buf;
        // Remove leading zero
        if (hour_str[0] == '0') hour_str = hour_str.substr(1);

        std::string subdir = hour_str + "-" + period;

        std::strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", &tm_now);
        std::string timestamp = time_buf;

        std::string base_path = "results/game_graph/" + date_dir + "/" + subdir + "/game_graph_" + timestamp;

        if (solver.write_dot(base_path)) {
            std::cout << "  Game graph exported:" << std::endl;
            std::cout << "    DOT:  " << base_path << ".dot" << std::endl;
            std::cout << "    JSON: " << base_path << ".json" << std::endl;
            std::cout << "    HTML: " << base_path << ".html (interactive)" << std::endl;
        } else {
            std::cerr << "  Warning: Failed to export game graph" << std::endl;
        }
    }

    // Output result
    std::cout << "========================================" << std::endl;
    if (realizable) {
        std::cout << "   REALIZABLE" << std::endl;
    } else {
        std::cout << "   UNREALIZABLE" << std::endl;
    }
    std::cout << "========================================" << std::endl;

    return realizable ? 0 : 1;
}
