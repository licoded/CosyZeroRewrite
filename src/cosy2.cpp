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
#include "CLI/CLI.hpp"  // CLI11 command line parsing
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <csignal>
#include <atomic>
#include <unistd.h>  // for _exit()

using namespace formula;
using namespace synthesis;

//==============================================================================
// Signal Handling for Timeout
//==============================================================================

void signal_handler(int signal) {
    const char* signal_name = nullptr;
    switch (signal) {
        case SIGTERM: signal_name = "SIGTERM"; break;  // timeout command
        case SIGINT:  signal_name = "SIGINT"; break;  // Ctrl+C
        case SIGHUP:  signal_name = "SIGHUP"; break;  // hangup
        default:       signal_name = "UNKNOWN"; break;
    }

    std::cerr << "\n[TIMEOUT] Received signal " << signal_name
              << " - synthesis interrupted (TIMEOUT)" << std::endl;
    std::cerr << "[TIMEOUT] Result: TIMEOUT" << std::endl;
    std::cerr << std::flush;

    // Exit immediately - we cannot gracefully exit from a signal handler
    _exit(124);  // 124 is timeout's exit code convention
}

//==============================================================================
// File Reading Helpers
//==============================================================================

static std::string read_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        NOP_LOG_ERROR("Error: Cannot open file: {}", filename);
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
        NOP_LOG_ERROR("Warning: Cannot open partition file: {}", filename);
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

    NOP_LOG_INFO("Parsed partition: {} outputs, {} inputs",
               result.outputs.size(), result.inputs.size());

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
    // Initialize logger first (creates log directory if needed)
    logger::Logger::initialize();

    // Register signal handlers for timeout/interrupt detection
    std::signal(SIGTERM, signal_handler);  // timeout command
    std::signal(SIGINT, signal_handler);   // Ctrl+C
    std::signal(SIGHUP, signal_handler);   // hangup

    //==========================================================================
    // Command Line Parsing (using CLI11)
    //==========================================================================
    CLI::App app{"CosyZero LTLf Synthesis Tool - LTLf to Automata Synthesis"};

    // Version flag
    app.set_version_flag("--version", "CosyZero v2.0");

    // Options
    std::string formula_file;
    std::string partition_file;
    std::string trace_dir;
    bool quiet = false;

    // Formula input (either -f file or positional argument)
    app.add_option("-f,--file", formula_file, "Read formula from file")
        ->check(CLI::ExistingFile);

    // Partition file
    app.add_option("-p,--partition", partition_file, "Read variable partition from file")
        ->check(CLI::ExistingFile);

    // Trace recording
    app.add_option("--trace", trace_dir, "Enable trace recording for visualization (optional: custom directory)")
        ->default_str("")
        ->expected(0, 1)
        ->capture_default_str();

    // Quiet mode
    app.add_flag("-q,--quiet", quiet, "Suppress non-essential output");

    // Positional argument: formula string (if no -f specified)
    std::string formula_str;
    app.add_option("formula", formula_str, "LTLf formula string (if -f not specified)");

    // Footer with examples
    app.footer(
        "\nExamples:\n"
        "  Cosy2 -f response.ltlf -p response.part\n"
        "  Cosy2 \"G (req -> F ack)\"\n"
        "  Cosy2 -f formula.ltlf --trace\n"
        "  Cosy2 -f formula.ltlf -p partition.part --trace custom_dir\n"
        "\nFor more information, see: https://github.com/your-repo/CosyZero"
    );

    // Parse command line arguments
    CLI11_PARSE(app, argc, argv);

    //==========================================================================
    // Banner
    //==========================================================================
    if (!quiet) {
        NOP_LOG_INFO("========================================");
        NOP_LOG_INFO("   CosyZero LTLf Synthesis Tool v2.0");
        NOP_LOG_INFO("========================================");
    }

    //==========================================================================
    // Read formula from file if specified
    //==========================================================================
    if (!formula_file.empty()) {
        std::string raw = read_file(formula_file);
        if (raw.empty()) {
            NOP_LOG_ERROR("Error: Failed to read formula file");
            return 1;
        }
        formula_str = clean_formula(raw);
        if (!quiet) NOP_LOG_INFO("Formula from file: {}", formula_file);
    }

    // Check if formula is provided
    if (formula_str.empty()) {
        NOP_LOG_ERROR("Error: No formula provided");
        NOP_LOG_ERROR("Run 'Cosy2 --help' for usage information.");
        return 1;
    }

    if (!quiet) NOP_LOG_INFO("Formula: {}", formula_str);

    //==========================================================================
    // Parse partition if provided
    //==========================================================================
    Partition partition;
    if (!partition_file.empty()) {
        partition = parse_partition_file(partition_file);
    }

    //==========================================================================
    // Create formula pool and declare variables
    //==========================================================================
    FormulaPool pool;
    if (!partition.outputs.empty() || !partition.inputs.empty()) {
        pool.declare_variables(partition.outputs, partition.inputs);
        if (!quiet) {
            NOP_LOG_INFO("Variables declared: {} outputs, {} inputs",
                       pool.num_outputs(), pool.num_inputs());
        }
    }

    //==========================================================================
    // Parse formula
    //==========================================================================
    FormulaParser parser(pool);
    // Parser now auto-loads variables from pool, no need for set_variables()

    Formula* phi = parser.parse(formula_str);
    if (!phi) {
        NOP_LOG_ERROR("Error: Failed to parse formula");
        NOP_LOG_ERROR("Parser error: {}", parser.error());
        return 1;
    }

    if (!quiet) {
        LOG_DEBUG("Parsed: {}", phi->to_string());
        LOG_DEBUG("Parsed (with names): {}", phi->to_string_with_names(pool));

        // Show variable mapping
        LOG_DEBUG("Variable mapping:");
        for (int i = 0; i < pool.num_outputs() + pool.num_inputs(); ++i) {
            std::string var_name = pool.get_variable_name(i);
            std::string type = (i < pool.num_outputs()) ? "output" : "input";
            LOG_DEBUG("  v{} = {} ({})", i, var_name, type);
        }
    }

    //==========================================================================
    // Run synthesis
    //==========================================================================
    if (!quiet) NOP_LOG_INFO("Running on-the-fly synthesis...");

    // Create solver directly (not using convenience function)
    // so we can access the solver for game graph export
    int num_outputs = pool.num_outputs();
    int num_inputs = pool.num_inputs();

    // If variables not declared, extract them from the formula
    if (num_outputs == 0 && num_inputs == 0) {
        num_outputs = static_cast<int>(Formula::collect_variables(phi).size());
    }

    OnTheFlyGameSolver solver(phi, pool, num_outputs, num_inputs);

    // Enable trace if requested (trace_dir non-empty means trace enabled)
    if (!trace_dir.empty()) {
        solver.enable_trace(trace_dir);
        if (!quiet) LOG_DEBUG("Trace recording enabled...");
    }

    bool realizable = solver.is_realizable();

    //==========================================================================
    // Output result
    //==========================================================================
    if (!quiet) {
        NOP_LOG_INFO("========================================");
    }
    NOP_LOG_INFO(realizable ? "REALIZABLE" : "UNREALIZABLE");
    if (!quiet) {
        NOP_LOG_INFO("========================================");
    }

    return realizable ? 0 : 1;
}
