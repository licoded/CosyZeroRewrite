/**
 * @file cosy2.cpp
 * @brief CosyZero LTLf Synthesis Tool - Main Entry Point
 *
 * Usage:
 *   Cosy2 -f formula.ltlf [-p partition.part]
 *   Cosy2 "G (req -> F ack)"
 */

#include "CLI/CLI.hpp"
#include "formula/formula_parser.hpp"
#include "formula/formula_pool.hpp"
#include "io/file_utils.hpp"
#include "log/logger.hpp"
#include "synthesis/on_the_fly_solver.hpp"

#include <spdlog/fmt/fmt.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <tl/expected.hpp>
#include <unistd.h>

using namespace formula;
using namespace synthesis;
using namespace io;

//==============================================================================
// Signal Handling
//==============================================================================

void signal_handler(int signal)
{
    const char *signal_name = nullptr;
    switch (signal)
    {
        case SIGTERM:
            signal_name = "SIGTERM";
            break;
        case SIGINT:
            signal_name = "SIGINT";
            break;
        case SIGHUP:
            signal_name = "SIGHUP";
            break;
        default:
            signal_name = "UNKNOWN";
            break;
    }

    std::cerr << "\n[TIMEOUT] Received signal " << signal_name << " - synthesis interrupted (TIMEOUT)" << std::endl;
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

// Parse command line arguments (returns nullptr on parse error)
std::unique_ptr<Config> parse_command_line(int argc, char *argv[])
{
    auto config = std::make_unique<Config>();
    CLI::App app {"CosyZero LTLf Synthesis Tool - LTLf to Automata Synthesis"};
    app.set_version_flag("--version", "CosyZero v2.0");

    app.add_option("-f,--file", config->formula_file, "Read formula from file")->check(CLI::ExistingFile);
    app.add_option("-p,--partition", config->partition_file, "Read variable partition from file")
        ->check(CLI::ExistingFile);
    app.add_option("--trace", config->trace_dir, "Enable trace recording for visualization")
        ->default_str("")
        ->expected(0, 1)
        ->capture_default_str();
    app.add_flag("-q,--quiet", config->quiet, "Suppress non-essential output");
    app.add_option("formula", config->formula_str, "LTLf formula string (if -f not specified)");

    app.footer(fmt::format(R"xx(
Examples:
  Cosy2 -f response.ltlf -p response.part
  Cosy2 "G (req -> F ack)"
  Cosy2 -f formula.ltlf --trace
  Cosy2 -f formula.ltlf -p partition.part --trace custom_dir

For more information, see: https://github.com/licoded/CosyZeroRewrite
)xx"));

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError &e)
    {
        // CLI11 already printed the error/help message
        // Exit with the appropriate error code
        std::exit(app.exit(e));
    }

    return config;
}

// Print application banner
void print_banner()
{
    NOP_LOG_INFO("========================================");
    NOP_LOG_INFO("   CosyZero LTLf Synthesis Tool v2.0");
    NOP_LOG_INFO("========================================");
}

// Read formula from file (-f) or command line argument
tl::expected<std::string, std::string> read_formula(const Config &config)
{
    if (!config.formula_file.empty())
    {
        auto read_file_res = read_file_expected(config.formula_file);
        if (!read_file_res)
        {
            return tl::unexpected(fmt::format("Failed to read formula file: {}", read_file_res.error()));
        }

        std::string formula_str = clean_formula(*read_file_res);
        if (formula_str.empty())
        {
            return tl::unexpected(fmt::format("Formula file is empty after cleaning: {}", config.formula_file));
        }

        NOP_LOG_INFO("Formula from file: {}", config.formula_file);
        NOP_LOG_INFO("Formula: {}", formula_str);
        return formula_str;
    }

    const std::string &formula_str = config.formula_str;
    if (formula_str.empty())
    {
        return tl::unexpected("No formula provided. Use -f <file> or provide formula as argument");
    }

    NOP_LOG_INFO("Formula: {}", formula_str);
    return formula_str;
}

// Load partition from file
tl::expected<Partition, std::string> load_partition(const Config &config)
{
    if (config.partition_file.empty())
    {
        return tl::unexpected(fmt::format("Must specify partition file!"));
    }

    auto part_result = parse_partition_file(config.partition_file);
    if (!part_result)
    {
        return tl::unexpected(fmt::format("Failed to load partition: {}", part_result.error()));
    }

    NOP_LOG_INFO("Partition loaded: {} outputs, {} inputs", part_result->outputs.size(), part_result->inputs.size());
    return part_result;
}

// Parse formula and prepare formula pool (pool must be kept alive)
Formula *parse_formula(const std::string &formula_str, FormulaPool &pool, const Config &config)
{
    FormulaParser parser(pool);
    Formula *phi = parser.parse(formula_str);

    if (!phi)
    {
        NOP_LOG_ERROR("Error: Failed to parse formula");
        NOP_LOG_ERROR("Parser error: {}", parser.error());
        return nullptr;
    }

    if (!config.quiet)
    {
        LOG_DEBUG("Parsed: {}", phi->to_string(pool));
    }

    return phi;
}

// Run synthesis
bool run_synthesis(Formula *phi, FormulaPool &pool, const Config &config)
{
    NOP_LOG_INFO("Running on-the-fly synthesis...");

    OnTheFlyGameSolver solver(phi, pool);

    if (!config.trace_dir.empty())
    {
        solver.enable_trace(config.trace_dir);
        NOP_LOG_INFO("Trace recording enabled...");
    }

    return solver.is_realizable();
}

// Print synthesis result
void print_result(bool realizable, const Config &config)
{
    constexpr const char *sep = "========================================";
    const char *result = realizable ? "REALIZABLE" : "UNREALIZABLE";

    if (!config.quiet)
    {
        NOP_LOG_INFO(sep);
        NOP_LOG_INFO(result);
        NOP_LOG_INFO(sep);
    }
    else
    {
        NOP_LOG_INFO(result);
    }
}

//==============================================================================
// Main Entry Point
//==============================================================================

int main(int argc, char *argv[])
{
    logger::Logger::initialize();

    std::signal(SIGTERM, signal_handler);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGHUP, signal_handler);

    // Parse command line
    auto config = parse_command_line(argc, argv);
    if (!config)
        return 1;

    // Print banner
    if (!config->quiet)
        print_banner();

    // Read formula
    auto formula_result = read_formula(*config);
    if (!formula_result)
    {
        NOP_LOG_ERROR("{}", formula_result.error());
        return 1;
    }
    std::string formula_str = *formula_result;

    // Load partition
    auto partition_result = load_partition(*config);
    if (!partition_result)
    {
        NOP_LOG_ERROR("{}", partition_result.error());
        return 1;
    }
    Partition partition = *partition_result;

    FormulaPool pool;
    pool.declare_variables(partition.outputs, partition.inputs);

    // Parse formula (pool must stay alive)
    Formula *phi = parse_formula(formula_str, pool, *config);
    if (!phi)
        return 1;

    // Run synthesis
    bool realizable = run_synthesis(phi, pool, *config);

    // Print result
    print_result(realizable, *config);

    return 0;
}
