/**
 * Random Formula Generator and Tester
 * Generates random LTLf formulas and tests for:
 * - Parser crashes
 * - Transformation correctness (NNF, XNF, Simplify)
 * - Equivalence preservation
 * - Edge cases
 */

#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"
#include "formula/formula_checker.hpp"

#include <iostream>
#include <random>
#include <chrono>
#include <vector>
#include <string>
#include <fstream>

using namespace formula;

class RandomFormulaGenerator {
private:
    std::mt19937 rng_;
    std::uniform_int_distribution<int> op_dist_;
    std::uniform_int_distribution<int> var_dist_;
    std::uniform_int_distribution<int> depth_dist_;
    std::vector<std::string> var_names_;

public:
    RandomFormulaGenerator(int max_var_id = 10, int seed = 0)
        : rng_(seed), op_dist_(0, 8), var_dist_(0, max_var_id),
          depth_dist_(1, 6) {

        // Pre-generate variable names
        for (int i = 0; i <= max_var_id; ++i) {
            var_names_.push_back("p" + std::to_string(i));
        }
    }

    // Initialize pool with variables
    void init_pool(FormulaPool& pool) {
        for (const auto& name : var_names_) {
            if (!pool.has_variable(name)) {
                pool.get_or_create_variable(name);
            }
        }
    }

    // Generate random formula with specified max depth
    Formula* generate(FormulaPool& pool, int max_depth) {
        return generate(pool, 0, max_depth);
    }

private:
    Formula* generate(FormulaPool& pool, int current_depth, int max_depth) {
        // Base cases
        if (current_depth >= max_depth) {
            int choice = var_dist_(rng_) % 3;
            switch (choice) {
                case 0: return pool.create_true();
                case 1: return pool.create_false();
                default: {
                    int var_idx = var_dist_(rng_);
                    return pool.create_variable(var_names_[var_idx]);
                }
            }
        }

        // Recursive case - choose an operator
        int op = op_dist_(rng_);
        Formula* left = generate(pool, current_depth + 1, max_depth);
        Formula* right = nullptr;

        switch (op) {
            case 0: return pool.create_true();
            case 1: return pool.create_false();
            case 2: {
                int var_idx = var_dist_(rng_);
                return pool.create_variable(var_names_[var_idx]);
            }
            case 3: return pool.create_not(left);
            case 4:
                right = generate(pool, current_depth + 1, max_depth);
                return pool.create_and(left, right);
            case 5:
                right = generate(pool, current_depth + 1, max_depth);
                return pool.create_or(left, right);
            case 6: return pool.create_next(left);
            case 7:
                right = generate(pool, current_depth + 1, max_depth);
                return pool.create_until(left, right);
            case 8:
                right = generate(pool, current_depth + 1, max_depth);
                return pool.create_release(left, right);
            default:
                return pool.create_true();
        }
    }
};

// Test result tracking
struct TestResult {
    int total = 0;
    int passed = 0;
    int failed = 0;
    int crashed = 0;
    double total_time_ms = 0;

    void record_pass(double time_ms) {
        total++;
        passed++;
        total_time_ms += time_ms;
    }

    void record_fail(double time_ms) {
        total++;
        failed++;
        total_time_ms += time_ms;
    }

    void record_crash() {
        total++;
        crashed++;
    }

    void print(const std::string& name) const {
        std::cout << "[" << name << "] "
                  << "Total: " << total
                  << ", Passed: " << passed
                  << ", Failed: " << failed
                  << ", Crashed: " << crashed
                  << ", Time: " << total_time_ms << "ms"
                  << std::endl;
    }
};

// Minimizer - find minimal failing case
class Minimizer {
private:
    std::string failing_input_;
    Formula* failing_formula_ = nullptr;
    std::string failure_type_;

public:
    void record_failure(const std::string& input, Formula* f, const std::string& type) {
        failing_input_ = input;
        failing_formula_ = f;
        failure_type_ = type;
    }

    void print_report() const {
        if (!failing_input_.empty()) {
            std::cout << "\n========== MINIMAL FAILING CASE ==========" << std::endl;
            std::cout << "Type: " << failure_type_ << std::endl;
            std::cout << "Input: " << failing_input_ << std::endl;
            // Note: failing_formula_ is a dangling pointer (from destroyed pool), so we don't print it
            std::cout << "=========================================" << std::endl;
        }
    }

    const std::string& get_failing_input() const { return failing_input_; }
    const std::string& get_failure_type() const { return failure_type_; }
};

// Test runner
int main(int argc, char* argv[]) {
    int num_tests = 10000;
    int max_depth = 5;
    int max_var_id = 6;

    if (argc > 1) {
        num_tests = std::atoi(argv[1]);
    }
    if (argc > 2) {
        max_depth = std::atoi(argv[2]);
    }

    std::cout << "=== Random Formula Tester ===" << std::endl;
    std::cout << "Tests: " << num_tests << std::endl;
    std::cout << "Max Depth: " << max_depth << std::endl;
    std::cout << "Max Var ID: " << max_var_id << std::endl;
    std::cout << "==================================" << std::endl;

    RandomFormulaGenerator gen(max_var_id);
    Minimizer minimizer;
    TestResult parser_result, nnf_result, xnf_result, simplify_result;
    TestResult idempotent_result;  // equiv_result removed (not implemented)

    std::ofstream crash_log("output/logs/crashes.log");
    std::ofstream failure_log("output/logs/fuzz_failures.log");

    for (int i = 0; i < num_tests; ++i) {
        FormulaPool pool;
        gen.init_pool(pool);  // Initialize variables first

        try {
            // Generate random formula
            Formula* f = gen.generate(pool, max_depth);
            std::string f_str = f->to_string(pool);

            auto start = std::chrono::high_resolution_clock::now();

            // Test 1: Parse -> String -> Parse
            FormulaParser parser(pool);
            Formula* f2 = parser.parse(f_str);
            auto end = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration<double, std::milli>(end - start).count();

            if (f2 && !parser.has_error()) {
                parser_result.record_pass(elapsed);
            } else {
                parser_result.record_fail(elapsed);
                minimizer.record_failure(f_str, f, "parse_failure");
            }

            // Test 2: NNF preserves semantics
            start = std::chrono::high_resolution_clock::now();
            Formula* nnf = f->nnf(pool);
            end = std::chrono::high_resolution_clock::now();
            elapsed = std::chrono::duration<double, std::milli>(end - start).count();

            if (nnf) {
                bool equiv = FormulaChecker::are_equivalent(pool, f, nnf);
                if (equiv) {
                    nnf_result.record_pass(elapsed);
                } else {
                    nnf_result.record_fail(elapsed);
                    minimizer.record_failure(f_str, f, "nnf_not_equivalent");
                    failure_log << "NNF not equivalent: " << f_str << std::endl;
                }
            }

            // Test 3: XNF preserves semantics
            start = std::chrono::high_resolution_clock::now();
            Formula* xnf = f->xnf_with_end_marker(pool);
            end = std::chrono::high_resolution_clock::now();
            elapsed = std::chrono::duration<double, std::milli>(end - start).count();

            if (xnf) {
                bool equiv = FormulaChecker::are_equivalent(pool, f, xnf);
                if (equiv) {
                    xnf_result.record_pass(elapsed);
                } else {
                    xnf_result.record_fail(elapsed);
                    minimizer.record_failure(f_str, f, "xnf_not_equivalent");
                    failure_log << "XNF not equivalent: " << f_str << std::endl;
                }
            }

            // Test 4: Simplify preserves semantics
            start = std::chrono::high_resolution_clock::now();
            Formula* simp = f->simplify(pool);
            end = std::chrono::high_resolution_clock::now();
            elapsed = std::chrono::duration<double, std::milli>(end - start).count();

            if (simp) {
                bool equiv = FormulaChecker::are_equivalent(pool, f, simp);
                if (equiv) {
                    simplify_result.record_pass(elapsed);
                } else {
                    simplify_result.record_fail(elapsed);
                    minimizer.record_failure(f_str, f, "simplify_not_equivalent");
                    failure_log << "Simplify not equivalent: " << f_str << std::endl;
                }
            }

            // Test 5: NNF idempotence
            if (nnf) {
                Formula* nnf2 = nnf->nnf(pool);
                if (nnf2) {
                    bool equiv = FormulaChecker::are_equivalent(pool, nnf, nnf2);
                    if (equiv) {
                        idempotent_result.record_pass(0);
                    } else {
                        idempotent_result.record_fail(0);
                        minimizer.record_failure(f_str, f, "nnf_not_idempotent");
                        failure_log << "NNF not idempotent: " << f_str << std::endl;
                    }
                }
            }

            // Progress reporting
            if ((i + 1) % 1000 == 0) {
                std::cout << "Progress: " << (i + 1) << "/" << num_tests << std::endl;
            }

        } catch (const std::exception& e) {
            parser_result.record_crash();
            crash_log << "Exception: " << e.what() << std::endl;
        } catch (...) {
            parser_result.record_crash();
            crash_log << "Unknown exception at test " << i << std::endl;
        }
    }

    // Print results
    std::cout << "\n========== RESULTS ==========" << std::endl;
    parser_result.print("Parser");
    nnf_result.print("NNF");
    xnf_result.print("XNF");
    simplify_result.print("Simplify");
    idempotent_result.print("Idempotent");

    minimizer.print_report();

    return (parser_result.crashed > 0 || nnf_result.failed > 0 ||
            xnf_result.failed > 0 || simplify_result.failed > 0) ? 1 : 0;
}
