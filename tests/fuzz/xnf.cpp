/**
 * @file xnf_fuzz_test.cpp
 * @brief Fuzzing test for XNF (neXt Normal Form) transformation
 *
 * Generates random LTLf formulas and verifies XNF transformation correctness:
 * - XNF must also satisfy NNF rules (negations only before literals)
 * - Temporal operators (U, R) must be inside Next (X) operators
 * - At the top level (under AND/OR), only: true, false, literals, and X(...) are allowed
 *
 * Usage:
 *   ./xnf_fuzz_test                    # Default: 10000 formulas, max depth 5
 *   ./xnf_fuzz_test 5000               # 5000 formulas
 *   ./xnf_fuzz_test 1000 3             # 1000 formulas, max depth 3
 *   ./xnf_fuzz_test 1000 8 50          # 1000 formulas, depth 8, 50 variables
 *   ./xnf_fuzz_test 100 5 10 20        # 100 formulas, depth 5, 10 vars, print first 20
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
    int print_count = 0;            // Print first N formulas with XNF results
    int seed = std::random_device{}(); // Random seed
    bool verbose = false;           // Print detailed output for failures
};

// ========== XNF Validator ==========

/**
 * @brief Check NNF rules (negations only before literals)
 *
 * This helper checks NNF properties but ALLOWS U/R operators
 * (unlike the strict is_nnf which rejects U/R entirely).
 * Used for checking the content inside X(...) in XNF.
 *
 * @param f Formula to validate
 * @param error_msg Optional error message
 * @return true if NNF rules are satisfied
 */
bool check_nnf_rules(Formula* f, std::string* error_msg = nullptr) {
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
            return true;

        case Formula::OpType::Not: {
            Formula* child = f->left();
            if (!child) {
                if (error_msg) *error_msg = "Not has null child";
                return false;
            }
            // In NNF, negation only applies to literals
            if (child->op() == Formula::OpType::Literal) {
                return true;
            }
            if (error_msg) {
                std::ostringstream oss;
                oss << "Negation before non-literal: !((" << child->op_name() << "))";
                *error_msg = oss.str();
            }
            return false;
        }

        case Formula::OpType::And:
        case Formula::OpType::Or:
        case Formula::OpType::Until:
        case Formula::OpType::Release: {
            std::string left_error, right_error;
            bool left_ok = check_nnf_rules(f->left(), error_msg ? &left_error : nullptr);
            bool right_ok = check_nnf_rules(f->right(), error_msg ? &right_error : nullptr);
            if (!left_ok) {
                if (error_msg) *error_msg = "Left child: " + left_error;
                return false;
            }
            if (!right_ok) {
                if (error_msg) *error_msg = "Right child: " + right_error;
                return false;
            }
            return true;
        }

        case Formula::OpType::Next: {
            // Recurse into child to check NNF rules
            return check_nnf_rules(f->left(), error_msg);
        }
    }

    if (error_msg) *error_msg = "Unknown operator";
    return false;
}

/**
 * @brief Validates that a formula is in proper XNF (neXt Normal Form)
 *
 * XNF properties:
 * 1. Must satisfy NNF rules (negations only before literals)
 * 2. Temporal operators (U, R) only appear inside Next (X) operators
 * 3. At the top level (under AND/OR): only true, false, literals, and X(...) are allowed
 *
 * @param f Formula to validate
 * @param pool FormulaPool for variable lookup
 * @param error_msg Optional error message describing why XNF validation failed
 * @return true if formula is in XNF, false otherwise
 */
bool is_xnf(Formula* f, FormulaPool& pool, std::string* error_msg = nullptr) {
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
            // These are always in XNF
            return true;

        case Formula::OpType::Not: {
            // In XNF (which is also NNF), negation must apply directly to a literal
            Formula* child = f->left();
            if (!child) {
                if (error_msg) *error_msg = "Not has null child";
                return false;
            }

            auto child_op = child->op();
            if (child_op == Formula::OpType::Literal) {
                // !p is valid XNF
                return true;
            }

            // Negation before anything else is NOT XNF (violates NNF)
            if (error_msg) {
                std::ostringstream oss;
                oss << "Negation before non-literal: !((" << child->op_name() << "))";
                *error_msg = oss.str();
            }
            return false;
        }

        case Formula::OpType::And:
        case Formula::OpType::Or: {
            // Binary operators: recurse on both children
            std::string left_error, right_error;
            bool left_ok = is_xnf(f->left(), pool, error_msg ? &left_error : nullptr);
            bool right_ok = is_xnf(f->right(), pool, error_msg ? &right_error : nullptr);

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
            // Next operator: the child must satisfy NNF rules
            // (but U/R are allowed inside X)
            if (!f->left()) {
                if (error_msg) *error_msg = "Next has null child";
                return false;
            }
            // Check NNF rules for the child (allows U/R inside X)
            std::string child_error;
            if (!check_nnf_rules(f->left(), error_msg ? &child_error : nullptr)) {
                if (error_msg) *error_msg = "Child of X violates NNF: " + child_error;
                return false;
            }
            return true;
        }

        case Formula::OpType::Until:
        case Formula::OpType::Release: {
            // U and R are NOT allowed directly in XNF (they must be inside X)
            if (error_msg) {
                std::ostringstream oss;
                oss << "Temporal operator " << f->op_name() << " found outside Next operator";
                *error_msg = oss.str();
            }
            return false;
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
    int xnf_valid = 0;
    int xnf_invalid = 0;
    int parse_errors = 0;
    double total_time_ms = 0;

    void print_summary() const {
        std::cout << "\n========================================" << std::endl;
        std::cout << "XNF Fuzz Test Summary" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Total formulas tested: " << total << std::endl;
        std::cout << "XNF valid:            " << xnf_valid << std::endl;
        std::cout << "XNF invalid:          " << xnf_invalid << std::endl;
        std::cout << "Parse errors:          " << parse_errors << std::endl;
        if (total > 0) {
            std::cout << "XNF validity rate:    " << std::fixed << std::setprecision(2)
                      << (100.0 * xnf_valid / total) << "%" << std::endl;
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
    std::cout << "XNF Fuzzing Test" << std::endl;
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

        // Transform to XNF
        Formula* xnf_result = nullptr;
        try {
            xnf_result = original->xnf_with_end_marker(pool);
        } catch (const std::exception& e) {
            std::cerr << "[" << i << "] XNF error: " << e.what()
                     << "\n  Formula: " << formula_str << std::endl;
            continue;
        }

        // Validate XNF
        std::string error_msg;
        bool is_valid = is_xnf(xnf_result, pool, &error_msg);

        // Print formula and XNF result for first print_count formulas
        if (config.print_count > 0 && i < config.print_count) {
            std::cout << "\n[" << (i + 1) << "]" << std::endl;
            std::cout << "  Input:  " << formula_str << std::endl;
            std::cout << "  XNF:    " << xnf_result->to_string(pool) << std::endl;
            if (!is_valid) {
                std::cout << "  Error:  " << error_msg << std::endl;
            }
        }

        if (is_valid) {
            stats.xnf_valid++;
        } else {
            stats.xnf_invalid++;
            if (config.verbose) {
                std::cerr << "\n[" << i << "] XNF VALIDATION FAILED" << std::endl;
                std::cerr << "  Original:  " << formula_str << std::endl;
                std::cerr << "  XNF:       " << xnf_result->to_string(pool) << std::endl;
                std::cerr << "  Error:     " << error_msg << std::endl;
            } else {
                // Print first few failures for visibility
                if (stats.xnf_invalid <= 10) {
                    std::cout << "  [" << i << "] FAIL: " << error_msg << std::endl;
                }
            }
        }

        // Progress indicator
        if ((i + 1) % 1000 == 0 || i == config.num_formulas - 1) {
            std::cout << "Progress: " << (i + 1) << "/" << config.num_formulas
                      << " (valid: " << stats.xnf_valid
                      << ", invalid: " << stats.xnf_invalid
                      << ", parse errors: " << stats.parse_errors << ")" << std::endl;
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    stats.total_time_ms = std::chrono::duration<double, std::milli>(
        end_time - start_time).count();

    stats.print_summary();

    // Return non-zero if there were XNF validation failures
    return (stats.xnf_invalid == 0) ? 0 : 1;
}
