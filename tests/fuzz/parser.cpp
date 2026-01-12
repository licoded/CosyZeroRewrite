#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"
#include <cstdint>
#include <cstddef>

using namespace formula;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Skip if too large (prevents excessive memory usage)
    if (size > 4096) return 0;

    // Convert bytes to string (filter out non-printable characters)
    std::string input;
    input.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        // Only allow printable ASCII characters that are valid in formulas
        char c = static_cast<char>(data[i]);
        if (c >= 32 && c < 127 &&
            (c == '!' || c == '&' || c == '|' || c == '(' || c == ')' ||
             c == 'X' || c == 'x' || c == 'U' || c == 'u' ||
             c == 'R' || c == 'r' || c == '_' ||
             (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
             (c >= '0' && c <= '9') || c == ' ')) {
            input += c;
        }
    }

    if (input.empty()) return 0;

    // Create pool and parser
    FormulaPool pool;
    FormulaParser parser(pool);

    // Parse the input
    Formula* f = parser.parse(input);

    // If parsing succeeded, exercise various operations
    if (f && !parser.has_error()) {
        // Exercise to_string (may catch crashes in string conversion)
        std::string str = f->to_string(pool);
        (void)str;  // Suppress unused warning in fuzzer mode

        // Exercise structural queries
        f->is_literal();
        f->is_not();
        f->is_and();
        f->is_or();
        f->is_next();
        f->is_until();
        f->is_release();
        f->is_true();
        f->is_false();
        f->is_end();

        if (f->left()) {
            f->left()->to_string(pool);
        }
        if (f->right()) {
            f->right()->to_string(pool);
        }

        // Exercise transformations (may find bugs)
        Formula* nnf = f->nnf(pool);
        if (nnf) {
            nnf->to_string(pool);
        }

        Formula* xnf = f->xnf_with_end_marker(pool);
        if (xnf) {
            xnf->to_string(pool);
        }

        Formula* simp = f->simplify(pool);
        if (simp) {
            simp->to_string(pool);
        }
    }

    return 0;
}
