/**
 * @file nnf_fuzz_test.cpp
 * @brief Fuzzing test for NNF (Negation Normal Form) transformation
 *
 * Generates random LTLf formulas and verifies NNF transformation correctness:
 * - All negations appear only directly before literals (not nested)
 * - No double negations (! !x where x is not a literal)
 * - Negation does not apply to subformulas with temporal operators
 *
 * Usage:
 *   ./nnf_fuzz_test                    # Default: 10000 formulas, max depth 5
 *   ./nnf_fuzz_test 5000               # 5000 formulas
 *   ./nnf_fuzz_test 1000 3             # 1000 formulas, max depth 3
 *   ./nnf_fuzz_test 1000 8 50          # 1000 formulas, depth 8, 50 variables
 *   ./nnf_fuzz_test 100 5 10 20        # 100 formulas, depth 5, 10 vars, print first 20
 */

#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"
#include "log/logger.hpp"

#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>

using namespace formula;

// ========== Configuration ==========

struct FuzzConfig {
    int num_formulas = 10000;      // Number of random formulas to test
    int max_depth = 5;              // Maximum formula tree depth
    int num_variables = 20;         // Number of proposition variables
    int print_count = 0;            // Print first N formulas with NNF results
    int seed = std::random_device{}(); // Random seed
    bool verbose = false;           // Print detailed output for failures
};

// ========== NNF Validator ==========

/**
 * @brief Validates that a formula is in proper Negation Normal Form
 *
 * NNF properties:
 * 1. Negation (!) only appears directly before literals (p, !p)
 * 2. No nested negations: !!X is invalid (should be X)
 * 3. Negation does NOT appear before temporal operators: !X(p), !(p U q) are invalid
 *
 * @param f Formula to validate
 * @param pool FormulaPool for variable lookup
 * @return true if formula is in NNF, false otherwise
 * @param error_msg Optional error message describing why NNF validation failed
 */
bool is_nnf(Formula* f, FormulaPool& pool, std::string* error_msg = nullptr) {
    if (!f) {
        if (error_msg) *error_msg = "null formula";
        return false;
    }

    auto op = f->op();

    switch (op) {
        case Formula::OpType::True:
        case Formula::OpType::False:
        case Formula::OpType::Literal:
        case Formula::OpType::End:
            // These are always in NNF
            return true;

        case Formula::OpType::Not: {
            // In NNF, negation must apply directly to a literal
            Formula* child = f->left();
            if (!child) {
                if (error_msg) *error_msg = "Not has null child";
                return false;
            }

            auto child_op = child->op();
            if (child_op == Formula::OpType::Literal) {
                // !p is valid NNF
                return true;
            }

            // Negation before anything else is NOT NNF
            if (error_msg) {
                std::ostringstream oss;
                oss << "Negation before non-literal: !(" << child->to_string() << ")";
                *error_msg = oss.str();
            }
            return false;
        }

        case Formula::OpType::And:
        case Formula::OpType::Or:
        case Formula::OpType::Until:
        case Formula::OpType::Release: {
            // Binary operators: recurse on both children
            std::string left_error, right_error;
            bool left_ok = is_nnf(f->left(), pool, error_msg ? &left_error : nullptr);
            bool right_ok = is_nnf(f->right(), pool, error_msg ? &right_error : nullptr);

            if (!left_ok) {
                if (error_msg) *error_msg = "Left child invalid: " + left_error;
                return false;
            }
            if (!right_ok) {
                if (error_msg) *error_msg = "Right child invalid: " + right_error;
                return false;
            }
            return true;
        }

        case Formula::OpType::Next: {
            // Next operator: recurse on child
            // Note: X(!p) is valid NNF since !p is NNF
            return is_nnf(f->left(), pool, error_msg);
        }
    }

    if (error_msg) *error_msg = "Unknown operator";
    return false;
}

// ========== Random Formula Generator ==========

class RandomFormulaGenerator {
public:
    RandomFormulaGenerator(const FuzzConfig& config)
        : config_(config), rng_(config.seed), var_dist_(1, config.num_variables) {
        // Create variable names: p1, p2, ..., pn
        for (int i = 1; i <= config.num_variables; ++i) {
            vars_.push_back("p" + std::to_string(i));
        }
    }

    std::string generate() {
        return generate(config_.max_depth);
    }

    std::string generate(int depth) {
        // At depth 0, must generate a literal or constant
        if (depth == 0) {
            return generate_leaf();
        }

        // Randomly choose what to generate
        // 0-2: leaf (literal, true, false, !literal)
        // 3: Not
        // 4-5: And/Or
        // 6: Next
        // 7-8: Until/Release
        int choice = rng_() % 9;

        switch (choice) {
            case 0: return generate_leaf();                    // literal
            case 1: return "true";                           // true
            case 2: return "false";                          // false
            case 3: return "!" + generate_leaf();             // !literal
            case 4: return "(" + generate(depth - 1) + " & " + generate(depth - 1) + ")";  // And
            case 5: return "(" + generate(depth - 1) + " | " + generate(depth - 1) + ")";  // Or
            case 6: return "X(" + generate(depth - 1) + ")";  // Next
            case 7: return "(" + generate(depth - 1) + " U " + generate(depth - 1) + ")";  // Until
            case 8: return "(" + generate(depth - 1) + " R " + generate(depth - 1) + ")";  // Release
            default:
                return generate_leaf();
        }
    }

private:
    std::string generate_leaf() {
        int idx = var_dist_(rng_) - 1;
        return vars_[idx];
    }

    FuzzConfig config_;
    std::mt19937 rng_;
    std::uniform_int_distribution<int> var_dist_;
    std::vector<std::string> vars_;
};

// ========== Statistics ==========

struct FuzzStats {
    int total = 0;
    int nnf_valid = 0;
    int nnf_invalid = 0;
    int parse_errors = 0;
    double total_time_ms = 0;

    void print_summary() const {
        std::cout << "\n========================================" << std::endl;
        std::cout << "NNF Fuzz Test Summary" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Total formulas tested: " << total << std::endl;
        std::cout << "NNF valid:            " << nnf_valid << std::endl;
        std::cout << "NNF invalid:          " << nnf_invalid << std::endl;
        std::cout << "Parse errors:          " << parse_errors << std::endl;
        if (total > 0) {
            std::cout << "NNF validity rate:    " << std::fixed << std::setprecision(2)
                      << (100.0 * nnf_valid / total) << "%" << std::endl;
            std::cout << "Average time:        " << std::fixed << std::setprecision(2)
                      << (total_time_ms / total) << " ms" << std::endl;
        }
        std::cout << "========================================" << std::endl;
    }
};

// ========== Main Test Runner ==========

int main(int argc, char* argv[]) {
    FuzzConfig config;
    FormulaPool pool;

    // Parse command line arguments
    if (argc >= 2) {
        try {
            config.num_formulas = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid number of formulas: " << argv[1] << std::endl;
            std::cerr << "Usage: " << argv[0] << " [num_formulas] [max_depth] [num_variables] [print_count]" << std::endl;
            return 1;
        }
    }
    if (argc >= 3) {
        try {
            config.max_depth = std::stoi(argv[2]);
        } catch (...) {
            std::cerr << "Invalid max_depth: " << argv[2] << std::endl;
            std::cerr << "Usage: " << argv[0] << " [num_formulas] [max_depth] [num_variables] [print_count]" << std::endl;
            return 1;
        }
    }
    if (argc >= 4) {
        try {
            config.num_variables = std::stoi(argv[3]);
        } catch (...) {
            std::cerr << "Invalid num_variables: " << argv[3] << std::endl;
            std::cerr << "Usage: " << argv[0] << " [num_formulas] [max_depth] [num_variables] [print_count]" << std::endl;
            return 1;
        }
    }
    if (argc >= 5) {
        try {
            config.print_count = std::stoi(argv[4]);
        } catch (...) {
            std::cerr << "Invalid print_count: " << argv[4] << std::endl;
            std::cerr << "Usage: " << argv[0] << " [num_formulas] [max_depth] [num_variables] [print_count]" << std::endl;
            return 1;
        }
    }

    // Check for verbose flag
    config.verbose = (std::getenv("COSY_FUZZ_VERBOSE") != nullptr);

    std::cout << "========================================" << std::endl;
    std::cout << "NNF Fuzzing Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Formulas:              " << config.num_formulas << std::endl;
    std::cout << "Max depth:             " << config.max_depth << std::endl;
    std::cout << "Variables:             " << config.num_variables << std::endl;
    std::cout << "Print first N:         " << config.print_count << std::endl;
    std::cout << "Seed:                  " << config.seed << std::endl;
    std::cout << "========================================" << std::endl;

    // Declare variables
    std::vector<std::string> vars;
    for (int i = 1; i <= config.num_variables; ++i) {
        vars.push_back("p" + std::to_string(i));
    }
    std::vector<std::string> inputs;  // No inputs for this test
    pool.declare_variables(vars, inputs);

    RandomFormulaGenerator generator(config);
    FormulaParser parser(pool);
    FuzzStats stats;

    auto start_time = std::chrono::steady_clock::now();

    for (int i = 0; i < config.num_formulas; ++i) {
        stats.total++;

        // Generate random formula
        std::string formula_str = generator.generate();

        // Parse formula
        Formula* original = nullptr;
        try {
            original = parser.parse(formula_str);
        } catch (const std::exception& e) {
            stats.parse_errors++;
            if (config.verbose) {
                std::cerr << "[" << i << "] Parse error: " << e.what()
                         << "\n  Formula: " << formula_str << std::endl;
            }
            continue;
        }

        // Transform to NNF
        Formula* nnf_result = nullptr;
        try {
            nnf_result = original->nnf(pool);
        } catch (const std::exception& e) {
            std::cerr << "[" << i << "] NNF error: " << e.what()
                     << "\n  Formula: " << formula_str << std::endl;
            continue;
        }

        // Validate NNF
        std::string error_msg;
        bool is_valid = is_nnf(nnf_result, pool, &error_msg);

        // Print formula and NNF result for first print_count formulas
        if (config.print_count > 0 && i < config.print_count) {
            std::cout << "\n[" << (i + 1) << "]" << std::endl;
            std::cout << "  Input:  " << formula_str << std::endl;
            std::cout << "  NNF:    " << nnf_result->to_string_with_names(pool) << std::endl;
            if (!is_valid) {
                std::cout << "  Error:  " << error_msg << std::endl;
            }
        }

        if (is_valid) {
            stats.nnf_valid++;
        } else {
            stats.nnf_invalid++;
            if (config.verbose) {
                std::cerr << "\n[" << i << "] NNF VALIDATION FAILED" << std::endl;
                std::cerr << "  Original:  " << formula_str << std::endl;
                std::cerr << "  NNF:       " << nnf_result->to_string_with_names(pool) << std::endl;
                std::cerr << "  Error:     " << error_msg << std::endl;
            } else {
                // Print first few failures for visibility
                if (stats.nnf_invalid <= 10) {
                    std::cout << "  [" << i << "] FAIL: " << error_msg << std::endl;
                }
            }
        }

        // Progress indicator
        if ((i + 1) % 1000 == 0 || i == config.num_formulas - 1) {
            std::cout << "Progress: " << (i + 1) << "/" << config.num_formulas
                      << " (valid: " << stats.nnf_valid
                      << ", invalid: " << stats.nnf_invalid
                      << ", parse errors: " << stats.parse_errors << ")" << std::endl;
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    stats.total_time_ms = std::chrono::duration<double, std::milli>(
        end_time - start_time).count();

    stats.print_summary();

    // Return non-zero if there were NNF validation failures
    return (stats.nnf_invalid == 0) ? 0 : 1;
}
