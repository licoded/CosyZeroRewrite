// Simple fuzzer driver for testing when libFuzzer is not available
// This mimics libFuzzer behavior by running with predefined test inputs

#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"
#include "formula/formula_checker.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <functional>

using namespace formula;

// Test inputs for parser fuzzer
static const std::vector<std::string> parser_test_inputs = {
    "a",
    "!a",
    "a & b",
    "a | b",
    "X(a)",
    "!X(a)",
    "X(!a)",
    "a U b",
    "a R b",
    "!(a & b)",
    "!(a | b)",
    "X(a) U b",
    "a & b & c",
    "a | b | c",
    "((a & b) | c) U d",
    "!X(!(a & b))",
    "true",
    "false",
    "a & true",
    "a | false",
    "!!a",
    "!(!a)",
    "X(X(a))",
    "a U (b & X(c))",
    "(a | b) & (c | d)",
    "",
    "(",  // Invalid inputs
    ")",
    "!&a",
    "a U",
    "X()",
    "!!!!!a",
    "(((a)))",
    "a_b & c_d",
};

// Test inputs for transformation fuzzer
static const std::vector<std::vector<uint8_t>> transform_test_inputs = {
    {0, 1, 0},       // simple literal
    {1, 1, 0, 0},    // false
    {3, 2, 0},       // !a
    {4, 2, 0, 2, 1}, // a & b
    {5, 2, 0, 2, 1}, // a | b
    {6, 2, 0},       // X(a)
    {7, 2, 0, 2, 1}, // a U b
    {8, 2, 0, 2, 1}, // a R b
};

void run_parser_tests() {
    std::cout << "=== Running Parser Fuzzer Tests ===" << std::endl;

    size_t passed = 0;
    size_t failed = 0;
    size_t crashed = 0;

    for (const auto& input : parser_test_inputs) {
        try {
            FormulaPool pool;
            FormulaParser parser(pool);

            Formula* f = parser.parse(input);

            if (f && !parser.has_error()) {
                // Exercise operations
                std::string str = f->to_string();
                Formula* nnf = f->nnf(pool);
                Formula* xnf = f->xnf_with_tail(pool);
                Formula* simp = f->simplify(pool);
                (void)str; (void)nnf; (void)xnf; (void)simp;
                passed++;
            } else {
                // Parse error - expected for some invalid inputs
                passed++;
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception on input '" << input << "': " << e.what() << std::endl;
            crashed++;
        } catch (...) {
            std::cerr << "Unknown exception on input '" << input << "'" << std::endl;
            crashed++;
        }
    }

    std::cout << "Results: " << passed << " passed, " << failed << " failed, " << crashed << " crashed" << std::endl;
}

void run_transformation_tests() {
    std::cout << "\n=== Running Transformation Fuzzer Tests ===" << std::endl;

    std::function<Formula*(const std::vector<uint8_t>&, size_t&, int, FormulaPool&)> gen_formula;
    gen_formula = [&gen_formula](const std::vector<uint8_t>& data, size_t& pos, int depth, FormulaPool& pool) -> Formula* {
        if (pos >= data.size() || depth > 5) return pool.create_true();

        uint8_t choice = data[pos++] % 15;

        switch (choice) {
            case 0: return pool.create_true();
            case 1: return pool.create_false();
            case 2: {
                int var_id = data[pos++] % 4;
                return pool.create(Formula::OpType::Literal, nullptr, nullptr, var_id);
            }
            case 3: {
                Formula* left = gen_formula(data, pos, depth + 1, pool);
                return pool.create_not(left);
            }
            case 4: {
                Formula* left = gen_formula(data, pos, depth + 1, pool);
                Formula* right = gen_formula(data, pos, depth + 1, pool);
                return pool.create_and(left, right);
            }
            case 5: {
                Formula* left = gen_formula(data, pos, depth + 1, pool);
                Formula* right = gen_formula(data, pos, depth + 1, pool);
                return pool.create_or(left, right);
            }
            case 6: {
                Formula* left = gen_formula(data, pos, depth + 1, pool);
                return pool.create_next(left);
            }
            case 7: {
                Formula* left = gen_formula(data, pos, depth + 1, pool);
                Formula* right = gen_formula(data, pos, depth + 1, pool);
                return pool.create_until(left, right);
            }
            case 8: {
                Formula* left = gen_formula(data, pos, depth + 1, pool);
                Formula* right = gen_formula(data, pos, depth + 1, pool);
                return pool.create_release(left, right);
            }
            default: return pool.create_true();
        }
    };

    size_t passed = 0;
    size_t property_failed = 0;

    for (const auto& data : transform_test_inputs) {
        try {
            FormulaPool pool;
            size_t pos = 0;
            Formula* f = gen_formula(data, pos, 0, pool);

            if (f) {
                auto vars = FormulaChecker::get_variables(f);

                // Test NNF properties
                if (vars.size() <= 4) {
                    Formula* nnf = f->nnf(pool);
                    if (nnf) {
                        if (!FormulaChecker::are_equivalent(pool, f, nnf)) {
                            std::cerr << "BUG: NNF doesn't preserve semantics!" << std::endl;
                            property_failed++;
                        }
                        if (!FormulaChecker::is_nnf(nnf)) {
                            std::cerr << "BUG: NNF result not in NNF!" << std::endl;
                            property_failed++;
                        }
                    }
                }

                // Test XNF properties
                if (vars.size() <= 4) {
                    Formula* xnf = f->xnf_with_tail(pool);
                    if (xnf) {
                        if (!FormulaChecker::are_equivalent(pool, f, xnf)) {
                            std::cerr << "BUG: XNF doesn't preserve semantics!" << std::endl;
                            property_failed++;
                        }
                        if (!FormulaChecker::is_xnf(xnf)) {
                            std::cerr << "BUG: XNF result not in XNF!" << std::endl;
                            property_failed++;
                        }
                    }
                }

                // Test Simplify properties
                if (vars.size() <= 4) {
                    Formula* simp = f->simplify(pool);
                    if (simp) {
                        if (!FormulaChecker::are_equivalent(pool, f, simp)) {
                            std::cerr << "BUG: Simplify doesn't preserve semantics!" << std::endl;
                            property_failed++;
                        }
                    }
                }

                passed++;
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception" << std::endl;
        }
    }

    std::cout << "Results: " << passed << " passed, " << property_failed << " property violations" << std::endl;
}

void run_equivalence_tests() {
    std::cout << "\n=== Running Equivalence Fuzzer Tests ===" << std::endl;

    std::function<Formula*(const std::vector<uint8_t>&, size_t&, int, FormulaPool&)> gen_formula;
    gen_formula = [&gen_formula](const std::vector<uint8_t>& data, size_t& pos, int depth, FormulaPool& pool) -> Formula* {
        if (pos >= data.size() || depth > 5) return pool.create_true();

        uint8_t choice = data[pos++] % 15;

        switch (choice) {
            case 0: return pool.create_true();
            case 1: return pool.create_false();
            case 2: {
                int var_id = data[pos++] % 4;
                return pool.create(Formula::OpType::Literal, nullptr, nullptr, var_id);
            }
            case 3: {
                Formula* left = gen_formula(data, pos, depth + 1, pool);
                return pool.create_not(left);
            }
            case 4: {
                Formula* left = gen_formula(data, pos, depth + 1, pool);
                Formula* right = gen_formula(data, pos, depth + 1, pool);
                return pool.create_and(left, right);
            }
            case 5: {
                Formula* left = gen_formula(data, pos, depth + 1, pool);
                Formula* right = gen_formula(data, pos, depth + 1, pool);
                return pool.create_or(left, right);
            }
            case 6: {
                Formula* left = gen_formula(data, pos, depth + 1, pool);
                return pool.create_next(left);
            }
            case 7: {
                Formula* left = gen_formula(data, pos, depth + 1, pool);
                Formula* right = gen_formula(data, pos, depth + 1, pool);
                return pool.create_until(left, right);
            }
            case 8: {
                Formula* left = gen_formula(data, pos, depth + 1, pool);
                Formula* right = gen_formula(data, pos, depth + 1, pool);
                return pool.create_release(left, right);
            }
            default: return pool.create_true();
        }
    };

    size_t passed = 0;
    size_t failed = 0;

    // Create test pairs
    for (size_t i = 0; i < transform_test_inputs.size(); ++i) {
        for (size_t j = i; j < transform_test_inputs.size(); ++j) {
            try {
                FormulaPool pool;
                size_t pos1 = 0, pos2 = 0;
                Formula* f1 = gen_formula(transform_test_inputs[i], pos1, 0, pool);
                Formula* f2 = gen_formula(transform_test_inputs[j], pos2, 0, pool);

                if (f1 && f2) {
                    // Test reflexivity
                    if (!FormulaChecker::are_equivalent(pool, f1, f1)) {
                        std::cerr << "BUG: Formula not equivalent to itself!" << std::endl;
                        failed++;
                        continue;
                    }
                    if (!FormulaChecker::are_equivalent(pool, f2, f2)) {
                        std::cerr << "BUG: Formula not equivalent to itself!" << std::endl;
                        failed++;
                        continue;
                    }

                    // Test symmetry
                    bool equiv1 = FormulaChecker::are_equivalent(pool, f1, f2);
                    bool equiv2 = FormulaChecker::are_equivalent(pool, f2, f1);
                    if (equiv1 != equiv2) {
                        std::cerr << "BUG: Equivalence not symmetric!" << std::endl;
                        failed++;
                        continue;
                    }

                    passed++;
                }
            } catch (...) {
                failed++;
            }
        }
    }

    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
}

int main() {
    std::cout << "Fuzzer Test Driver (simulating libFuzzer behavior)" << std::endl;
    std::cout << "===================================================" << std::endl;

    run_parser_tests();
    run_transformation_tests();
    run_equivalence_tests();

    std::cout << "\n=== All fuzzer tests completed ===" << std::endl;
    return 0;
}
