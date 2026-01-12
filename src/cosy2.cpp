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
#include "io/file_utils.hpp"
#include "CLI/CLI.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <csignal>
#include <atomic>
#include <unistd.h>

#include <spdlog/fmt/fmt.h>

using namespace formula;
using namespace synthesis;
using namespace io;

//==============================================================================
// Signal Handling
//==============================================================================

void signal_handler(int signal) {
    const char* signal_name = nullptr;
    switch (signal) {
        case SIGTERM: signal_name = "SIGTERM"; break;
        case SIGINT:  signal_name = "SIGINT"; break;
        case SIGHUP:  signal_name = "SIGHUP"; break;
        default:       signal_name = "UNKNOWN"; break;
    }

    std::cerr << "\n[TIMEOUT] Received signal " << signal_name
              << " - synthesis interrupted (TIMEOUT)" << std::endl;
    std::cerr << "[TIMEOUT] Result: TIMEOUT" << std::endl;
    std::cerr << std::flush;

    _exit(124);
}

//==============================================================================
// Configuration
//==============================================================================

struct Config {
    std::string formula_file;
    std::string partition_file;
    std::string trace_dir;
    std::string formula_str;
    bool quiet = false;
};

//==============================================================================
// Helper Functions
//==============================================================================

// Parse command line arguments
std::unique_ptr<Config> parse_command_line(int argc, char* argv[]) {
    (void)argc; (void)argv;  // Used by CLI11
    auto config = std::make_unique<Config>();
    CLI::App app{"CosyZero LTLf Synthesis Tool - LTLf to Automata Synthesis"};
    app.set_version_flag("--version", "CosyZero v2.0");

    app.add_option("-f,--file", config->formula_file, "Read formula from file")
        ->check(CLI::ExistingFile);
    app.add_option("-p,--partition", config->partition_file, "Read variable partition from file")
        ->check(CLI::ExistingFile);
    app.add_option("--trace", config->trace_dir, "Enable trace recording for visualization")
        ->default_str("")->expected(0, 1)->capture_default_str();
    app.add_flag("-q,--quiet", config->quiet, "Suppress non-essential output");
    app.add_option("formula", config->formula_str, "LTLf formula string (if -f not specified)");

    app.footer(fmt::format(R"xx(
Examples:
  Cosy2 -f response.ltlf -p response.part
  Cosy2 "G (req -> F ack)"
  Cosy2 -f formula.ltlf --trace
  Cosy2 -f formula.ltlf -p partition.part --trace custom_dir

For more information, see: https://github.com/your-repo/CosyZero
)xx"));

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError&) {
        return nullptr;
    }

    return config;
}

// Print application banner
void print_banner() {
    NOP_LOG_INFO("========================================");
    NOP_LOG_INFO("   CosyZero LTLf Synthesis Tool v2.0");
    NOP_LOG_INFO("========================================");
}

// Read formula from file or use provided string
std::string read_formula(const Config& config) {
    std::string formula_str;

    if (!config.formula_file.empty()) {
        std::string raw = read_file(config.formula_file);
        if (raw.empty()) {
            NOP_LOG_ERROR("Error: Failed to read formula file: {}", config.formula_file);
            return "";
        }
        formula_str = clean_formula(raw);
        NOP_LOG_INFO("Formula from file: {}", config.formula_file);
    } else {
        formula_str = config.formula_str;
    }

    if (formula_str.empty()) {
        NOP_LOG_ERROR("Error: No formula provided");
        NOP_LOG_ERROR("Run 'Cosy2 --help' for usage information.");
        return "";
    }

    NOP_LOG_INFO("Formula: {}", formula_str);
    return formula_str;
}

// Load partition from file
Partition load_partition(const Config& config) {
    Partition partition;

    if (!config.partition_file.empty()) {
        auto part_opt = parse_partition_file(config.partition_file);
        if (part_opt) {
            partition = *part_opt;
            NOP_LOG_INFO("Partition loaded: {} outputs, {} inputs",
                       partition.outputs.size(), partition.inputs.size());
        }
    }

    return partition;
}

// Parse formula and prepare formula pool (pool must be kept alive)
Formula* parse_formula(const std::string& formula_str,
                        FormulaPool& pool,
                        const Partition& partition,
                        bool quiet) {
    if (!partition.outputs.empty() || !partition.inputs.empty()) {
        pool.declare_variables(partition.outputs, partition.inputs);
        if (!quiet) {
            NOP_LOG_INFO("Variables declared: {} outputs, {} inputs",
                       pool.num_outputs(), pool.num_inputs());
        }
    }

    FormulaParser parser(pool);
    Formula* phi = parser.parse(formula_str);

    if (!phi) {
        NOP_LOG_ERROR("Error: Failed to parse formula");
        NOP_LOG_ERROR("Parser error: {}", parser.error());
        return nullptr;
    }

    if (!quiet) {
        LOG_DEBUG("Parsed: {}", phi->to_string());
        LOG_DEBUG("Parsed (with names): {}", phi->to_string_with_names(pool));
        LOG_DEBUG("Variable mapping:");
        for (int i = 0; i < pool.num_outputs() + pool.num_inputs(); ++i) {
            std::string var_name = pool.get_variable_name(i);
            std::string type = (i < pool.num_outputs()) ? "output" : "input";
            LOG_DEBUG("  v{} = {} ({})", i, var_name, type);
        }
    }

    return phi;
}

// Run synthesis
bool run_synthesis(Formula* phi, FormulaPool& pool, const Config& config) {
    NOP_LOG_INFO("Running on-the-fly synthesis...");

    int num_outputs = pool.num_outputs();
    int num_inputs = pool.num_inputs();

    if (num_outputs == 0 && num_inputs == 0) {
        num_outputs = static_cast<int>(Formula::collect_variables(phi).size());
    }

    OnTheFlyGameSolver solver(phi, pool, num_outputs, num_inputs);

    if (!config.trace_dir.empty()) {
        solver.enable_trace(config.trace_dir);
        LOG_DEBUG("Trace recording enabled...");
    }

    return solver.is_realizable();
}

// Print synthesis result
void print_result(bool realizable) {
    NOP_LOG_INFO("========================================");
    NOP_LOG_INFO(realizable ? "REALIZABLE" : "UNREALIZABLE");
    NOP_LOG_INFO("========================================");
}

//==============================================================================
// Main Entry Point
//==============================================================================

int main(int argc, char* argv[]) {
    logger::Logger::initialize();

    std::signal(SIGTERM, signal_handler);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGHUP, signal_handler);

    // Parse command line
    auto config = parse_command_line(argc, argv);
    if (!config) return 1;

    // Print banner
    if (!config->quiet) print_banner();

    // Read formula
    std::string formula_str = read_formula(*config);
    if (formula_str.empty()) return 1;

    // Load partition
    Partition partition = load_partition(*config);

    // Parse formula (pool must stay alive)
    FormulaPool pool;
    Formula* phi = parse_formula(formula_str, pool, partition, config->quiet);
    if (!phi) return 1;

    // Run synthesis
    bool realizable = run_synthesis(phi, pool, *config);

    // Print result
    if (!config->quiet) {
        print_result(realizable);
    } else {
        NOP_LOG_INFO(realizable ? "REALIZABLE" : "UNREALIZABLE");
    }

    return realizable ? 0 : 1;
}
