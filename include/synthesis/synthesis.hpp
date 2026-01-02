#ifndef SYNTHESIS_SYNTHESIS_HPP
#define SYNTHESIS_SYNTHESIS_HPP

#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"
#include <string>
#include <vector>
#include <optional>

namespace synthesis {

/**
 * @brief Simple LTLf Synthesis interface
 *
 * This is a simplified interface for LTLf realizability checking.
 * Full synthesis requires DFA construction and game solving (AALTA/Lydia).
 *
 * For now, this provides:
 * - Formula parsing and validation
 * - Variable extraction
 * - Basic satisfiability checking
 *
 * Full synthesis roadmap:
 * 1. LTLf → DFA conversion (requires AALTA or Lydia)
 * 2. DFA-based game solving (Tarjan SCC + BDD)
 * 3. Strategy extraction
 */
class Synthesis {
public:
    /**
     * @brief Parse an LTLf formula from string
     * @param formula_str Formula string
     * @param pool FormulaPool to use
     * @return Parsed formula, or nullptr on error
     */
    static formula::Formula* parse_formula(const std::string& formula_str, formula::FormulaPool& pool);

    /**
     * @brief Extract input/output variables from a partition file
     * @param part_file Path to .part file
     * @param outputs Output variable names (out)
     * @param inputs Input variable names (out)
     * @return true if successful
     */
    static bool load_partition(const std::string& part_file,
                               std::vector<std::string>& outputs,
                               std::vector<std::string>& inputs);

    /**
     * @brief Check if a formula is satisfiable (can be true in some model)
     * @param f Formula to check
     * @return true if satisfiable, false if unsatisfiable, std::nullopt if unknown
     *
     * Note: This is NOT the same as realizability.
     * Realizability requires checking if the system can FORCE the formula to be true
     * regardless of environment inputs.
     */
    static std::optional<bool> is_satisfiable(formula::Formula* f);

    /**
     * @brief Check if a formula is valid (always true)
     * @param f Formula to check
     * @return true if valid, false if invalid, std::nullopt if unknown
     */
    static std::optional<bool> is_valid(formula::Formula* f);

    /**
     * @brief Simplified realizability check (placeholder)
     *
     * Full realizability checking requires:
     * - LTLf → DFA conversion
     * - Game solving on DFA
     *
     * This is a placeholder that returns std::nullopt.
     * Use external AALTA/Lydia tools for full synthesis.
     */
    static std::optional<bool> is_realizable(formula::Formula* f);

    /**
     * @brief Realizability check with explicit variable partitioning
     *
     * This uses the pure C++ implementation:
     * - LTLf → DFA conversion (tableau construction)
     * - Game graph construction
     * - Tarjan SCC algorithm
     * - Winning/losing state classification
     *
     * @param f The LTLf formula
     * @param output_vars Output variable names (system controls)
     * @param input_vars Input variable names (environment controls)
     * @param pool Formula pool
     * @return true if realizable, false if not, std::nullopt if unknown
     */
    static std::optional<bool> is_realizable_with_partition(
        formula::Formula* f,
        const std::vector<std::string>& output_vars,
        const std::vector<std::string>& input_vars,
        formula::FormulaPool& pool
    );

    /**
     * @brief Read a benchmark case from the SMv2 dataset
     * @param base_dir Base directory of benchmarks (e.g., "benchmarks/sm1000")
     * @param bench_num Benchmark number (1-1000)
     * @param formula_str Formula string (out)
     * @param outputs Output variables (out)
     * @param inputs Input variables (out)
     * @return true if successful
     */
    static bool read_benchmark(const std::string& base_dir, int bench_num,
                               std::string& formula_str,
                               std::vector<std::string>& outputs,
                               std::vector<std::string>& inputs);

    /**
     * @brief Read the expected result from results.csv
     * @param base_dir Base directory of benchmarks
     * @param bench_num Benchmark number
     * @return true if Realizable, false if Unrealizable, std::nullopt if not found
     */
    static std::optional<bool> read_expected_result(const std::string& base_dir, int bench_num);
};

} // namespace synthesis

#endif // SYNTHESIS_SYNTHESIS_HPP
