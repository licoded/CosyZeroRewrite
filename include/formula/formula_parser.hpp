#ifndef FORMULA_PARSER_HPP
#define FORMULA_PARSER_HPP

#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include <string>
#include <vector>

namespace formula {

/**
 * @brief Recursive descent parser for LTLf formulas
 *
 * Supported syntax:
 *   - Literals: a, b, var_name
 *   - Constants: true, false
 *   - Negation: !expr
 *   - And: expr & expr
 *   - Or: expr | expr
 *   - Implies: expr -> expr (parsed as !expr | expr)
 *   - Next: X(expr)
 *   - Until: expr U expr
 *   - Release: expr R expr
 *   - Parentheses: (expr)
 *
 * Precedence (highest to lowest):
 *   1. X, !, literals
 *   2. &
 *   3. |
 *   4. ->
 *   5. U, R
 *
 * Grammar:
 *   formula       ::= implies_expr
 *   implies_expr  ::= or_expr ('->' or_expr)*
 *   or_expr       ::= and_expr ('|' and_expr)*
 *   and_expr      ::= binary_op ('&' binary_op)*
 *   binary_op     ::= unary_op ('U' | 'R' unary_op)*
 *   unary_op      ::= 'X'? postfix
 *   postfix       ::= '!' postfix | primary
 *   primary       ::= literal | '(' formula ')'
 *   literal       ::= identifier | 'true' | 'false'
 */
class FormulaParser {
public:
    explicit FormulaParser(FormulaPool& pool);
    ~FormulaParser() = default;

    /**
     * @brief Parse a formula from string
     * @param input Input string (e.g., "!(a & b) | X(c)")
     * @return Parsed formula, or nullptr if error
     *
     * Use error() to get error message on failure.
     */
    Formula* parse(const std::string& input);

    /**
     * @brief Get last error message
     * @return Error description, empty if no error
     */
    const std::string& error() const { return error_; }

    /**
     * @brief Check if last parse had error
     * @return true if error occurred
     */
    bool has_error() const { return !error_.empty(); }

    /**
     * @brief Set variable name mapping for multi-character names
     * @param var_names List of variable names
     *
     * By default, single-letter variables are recognized.
     * Call this to support multi-character variable names.
     */
    void set_variables(const std::vector<std::string>& var_names);

private:
    // Token types
    enum class TokenType {
        Identifier,
        True,
        False,
        Not,         // !
        And,         // &
        Or,          // |
        Implies,     // ->
        Next,        // X
        Until,       // U
        Release,     // R
        Finally,     // F (eventually, syntactic sugar for true U ...)
        Globally,    // G (globally, syntactic sugar for false R ...)
        LParen,      // (
        RParen,      // )
        End,
        Error
    };

    struct Token {
        TokenType type;
        std::string value;
        size_t position;
    };

    // Lexer
    void tokenize(const std::string& input);
    Token peek() const;
    Token consume();
    bool match(TokenType type);
    void set_error(const std::string& msg);

    // Parser functions (recursive descent)
    Formula* parse_formula();
    Formula* parse_implies_expr();
    Formula* parse_or_expr();
    Formula* parse_and_expr();
    Formula* parse_binary_op();
    Formula* parse_unary_op();
    Formula* parse_postfix();
    Formula* parse_primary();

    // Variable lookup
    Formula* lookup_variable(const std::string& name);

    FormulaPool& pool_;
    std::vector<Token> tokens_;
    size_t pos_;
    std::string error_;

    // Multi-character variable support
    std::vector<std::string> var_names_;
    bool use_multi_char_vars_;
};

} // namespace formula

#endif // FORMULA_PARSER_HPP
