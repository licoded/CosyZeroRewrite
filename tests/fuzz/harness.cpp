#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"
#include "formula/formula_checker.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <functional>

using namespace formula;

// Simple fuzzer harness that reads from file or stdin
// Can be used with AFL++ or other fuzzing tools

void test_parser_input(const std::string& input) {
    FormulaPool pool;
    FormulaParser parser(pool);

    Formula* f = parser.parse(input);

    if (f && !parser.has_error()) {
        // Exercise various operations
        std::string str = f->to_string(pool);
        Formula* nnf = f->nnf(pool);
        Formula* xnf = f->xnf_with_end_marker(pool);
        Formula* simp = f->simplify(pool);
        (void)str; (void)nnf; (void)xnf; (void)simp;  // Suppress unused warnings

        // Check properties for small formulas
        auto vars = FormulaChecker::get_variables(f);
        if (vars.size() <= 4) {
            if (nnf && !FormulaChecker::is_nnf(nnf)) {
                std::cerr << "BUG: NNF result not in NNF form!" << std::endl;
                abort();
            }
            if (xnf && !FormulaChecker::is_xnf(xnf)) {
                std::cerr << "BUG: XNF result not in XNF form!" << std::endl;
                abort();
            }
        }
    }
}

void test_formula_generation(const std::string& data) {
    // Generate formula from input bytes
    FormulaPool pool;
    size_t pos = 0;

    // Use std::function with recursive call through pointer
    std::function<Formula*(size_t&, int)> gen_formula;
    gen_formula = [&pool, &data, &gen_formula](size_t& pos, int depth) -> Formula* {
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
                Formula* left = gen_formula(pos, depth + 1);
                return pool.create_not(left);
            }
            case 4: {
                Formula* left = gen_formula(pos, depth + 1);
                Formula* right = gen_formula(pos, depth + 1);
                return pool.create_and(left, right);
            }
            case 5: {
                Formula* left = gen_formula(pos, depth + 1);
                Formula* right = gen_formula(pos, depth + 1);
                return pool.create_or(left, right);
            }
            case 6: {
                Formula* left = gen_formula(pos, depth + 1);
                return pool.create_next(left);
            }
            case 7: {
                Formula* left = gen_formula(pos, depth + 1);
                Formula* right = gen_formula(pos, depth + 1);
                return pool.create_until(left, right);
            }
            case 8: {
                Formula* left = gen_formula(pos, depth + 1);
                Formula* right = gen_formula(pos, depth + 1);
                return pool.create_release(left, right);
            }
            default: return pool.create_true();
        }
    };

    Formula* f = gen_formula(pos, 0);
    if (f) {
        // Test transformations
        Formula* nnf = f->nnf(pool);
        Formula* xnf = f->xnf_with_end_marker(pool);
        Formula* simp = f->simplify(pool);

        // Test equivalence
        auto vars = FormulaChecker::get_variables(f);
        if (vars.size() <= 4) {
            if (nnf) {
                if (!FormulaChecker::are_equivalent(pool, f, nnf)) {
                    std::cerr << "BUG: NNF doesn't preserve semantics!" << std::endl;
                    abort();
                }
            }
            if (xnf) {
                if (!FormulaChecker::are_equivalent(pool, f, xnf)) {
                    std::cerr << "BUG: XNF doesn't preserve semantics!" << std::endl;
                    abort();
                }
            }
            if (simp) {
                if (!FormulaChecker::are_equivalent(pool, f, simp)) {
                    std::cerr << "BUG: Simplify doesn't preserve semantics!" << std::endl;
                    abort();
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file> [--gen]" << std::endl;
        std::cerr << "  --gen  : Generate formula from bytes instead of parsing" << std::endl;
        return 1;
    }

    bool use_generator = (argc > 2 && std::string(argv[2]) == "--gen");

    std::string filename = argv[1];
    std::ifstream file(filename, std::ios::binary);

    if (!file) {
        std::cerr << "Error: Cannot open file: " << filename << std::endl;
        return 1;
    }

    std::string data((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

    if (use_generator) {
        test_formula_generation(data);
    } else {
        test_parser_input(data);
    }

    return 0;
}
