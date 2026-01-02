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

        // Convert to uppercase for section comparison
        std::string upper_line = line;
        for (char& c : upper_line) c = std::toupper(c);

        if (upper_line == "OUTPUTS") {
            section = Section::Outputs;
        } else if (upper_line == "INPUTS") {
            section = Section::Inputs;
        } else {
            // Variable name
            if (section == Section::Outputs) {
                result.outputs.push_back(line);
            } else if (section == Section::Inputs) {
                result.inputs.push_back(line);
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

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-f" && i + 1 < argc) {
            formula_file = argv[++i];
        } else if (arg == "-p" && i + 1 < argc) {
            partition_file = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: Cosy2 [options]" << std::endl;
            std::cout << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -f <file>    Read formula from file" << std::endl;
            std::cout << "  -p <file>    Read variable partition from file" << std::endl;
            std::cout << "  -h, --help   Show this help message" << std::endl;
            std::cout << std::endl;
            std::cout << "If no -f is specified, the first non-option argument" << std::endl;
            std::cout << "is treated as a formula string." << std::endl;
            std::cout << std::endl;
            std::cout << "Examples:" << std::endl;
            std::cout << "  Cosy2 -f response.ltlf -p response.part" << std::endl;
            std::cout << "  Cosy2 \"G (req -> F ack)\"" << std::endl;
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

    // Set up variables for parser (combine outputs and inputs)
    if (!partition.outputs.empty() || !partition.inputs.empty()) {
        std::vector<std::string> all_vars = partition.outputs;
        all_vars.insert(all_vars.end(), partition.inputs.begin(), partition.inputs.end());
        parser.set_variables(all_vars);
    }

    Formula* phi = parser.parse(formula_str);
    if (!phi) {
        std::cerr << "Error: Failed to parse formula" << std::endl;
        std::cerr << "Parser error: " << parser.error() << std::endl;
        return 1;
    }

    std::cout << "Parsed: " << phi->to_string() << std::endl;

    // Run synthesis
    std::cout << "Running on-the-fly synthesis..." << std::endl;
    bool realizable = is_realizable_on_the_fly(phi, pool);

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
