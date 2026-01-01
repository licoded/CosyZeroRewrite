#include "formula/formula_parser.hpp"
#include <cctype>
#include <sstream>
#include <algorithm>

namespace formula {

FormulaParser::FormulaParser(FormulaPool& pool)
    : pool_(pool)
    , pos_(0)
    , use_multi_char_vars_(false)
{
}

void FormulaParser::set_variables(const std::vector<std::string>& var_names) {
    var_names_ = var_names;
    use_multi_char_vars_ = true;
}

Formula* FormulaParser::parse(const std::string& input) {
    // Reset state
    tokens_.clear();
    pos_ = 0;
    error_.clear();

    // Skip leading whitespace
    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }

    // Skip trailing whitespace
    size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }

    std::string trimmed = input.substr(start, end - start);
    if (trimmed.empty()) {
        set_error("Empty input");
        return nullptr;
    }

    // Tokenize
    tokenize(trimmed);
    if (has_error()) {
        return nullptr;
    }

    // Parse
    Formula* result = parse_formula();

    if (!has_error() && !match(TokenType::End)) {
        set_error("Unexpected token at position " +
                  std::to_string(tokens_[pos_].position));
        return nullptr;
    }

    return result;
}

// =============================================================================
// Lexer
// =============================================================================

void FormulaParser::tokenize(const std::string& input) {
    size_t i = 0;
    while (i < input.size()) {
        char c = input[i];

        // Skip whitespace
        if (std::isspace(static_cast<unsigned char>(c))) {
            ++i;
            continue;
        }

        Token token;
        token.position = i;

        // Single character tokens
        switch (c) {
            case '!':
                token.type = TokenType::Not;
                token.value = "!";
                tokens_.push_back(token);
                ++i;
                continue;

            case '&':
                token.type = TokenType::And;
                token.value = "&";
                tokens_.push_back(token);
                ++i;
                continue;

            case '|':
                token.type = TokenType::Or;
                token.value = "|";
                tokens_.push_back(token);
                ++i;
                continue;

            case '(':
                token.type = TokenType::LParen;
                token.value = "(";
                tokens_.push_back(token);
                ++i;
                continue;

            case ')':
                token.type = TokenType::RParen;
                token.value = ")";
                tokens_.push_back(token);
                ++i;
                continue;

            case 'X':
            case 'x':
                token.type = TokenType::Next;
                token.value = "X";
                tokens_.push_back(token);
                ++i;
                continue;

            case 'U':
            case 'u':
                token.type = TokenType::Until;
                token.value = "U";
                tokens_.push_back(token);
                ++i;
                continue;

            case 'R':
            case 'r':
                token.type = TokenType::Release;
                token.value = "R";
                tokens_.push_back(token);
                ++i;
                continue;
        }

        // Keywords and identifiers
        if (std::isalpha(static_cast<unsigned char>(c))) {
            size_t start = i;
            while (i < input.size() &&
                   (std::isalnum(static_cast<unsigned char>(input[i])) ||
                    input[i] == '_')) {
                ++i;
            }
            std::string value = input.substr(start, i - start);

            // Convert to lowercase for comparison
            std::string lower = value;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            if (lower == "true") {
                token.type = TokenType::True;
            } else if (lower == "false") {
                token.type = TokenType::False;
            } else {
                token.type = TokenType::Identifier;
            }
            token.value = value;  // Keep original case
            tokens_.push_back(token);
            continue;
        }

        // Unknown character
        token.type = TokenType::Error;
        token.value = std::string(1, c);
        tokens_.push_back(token);
        set_error(std::string("Unknown character: ") + c);
        return;
    }

    // Add end token
    Token end;
    end.type = TokenType::End;
    end.value = "";
    end.position = i;
    tokens_.push_back(end);
}

FormulaParser::Token FormulaParser::peek() const {
    if (pos_ < tokens_.size()) {
        return tokens_[pos_];
    }
    return Token{TokenType::End, "", 0};
}

FormulaParser::Token FormulaParser::consume() {
    if (pos_ < tokens_.size()) {
        return tokens_[pos_++];
    }
    return Token{TokenType::End, "", 0};
}

bool FormulaParser::match(TokenType type) {
    if (peek().type == type) {
        ++pos_;
        return true;
    }
    return false;
}

void FormulaParser::set_error(const std::string& msg) {
    if (error_.empty()) {
        error_ = msg;
    }
}

// =============================================================================
// Parser (Recursive Descent)
// =============================================================================

// formula ::= or_expr
Formula* FormulaParser::parse_formula() {
    return parse_or_expr();
}

// or_expr ::= and_expr ('|' and_expr)*
Formula* FormulaParser::parse_or_expr() {
    Formula* left = parse_and_expr();
    if (has_error()) return nullptr;

    while (match(TokenType::Or)) {
        Formula* right = parse_and_expr();
        if (has_error()) return nullptr;
        left = pool_.create_or(left, right);
    }

    return left;
}

// and_expr ::= binary_op ('&' binary_op)*
Formula* FormulaParser::parse_and_expr() {
    Formula* left = parse_binary_op();
    if (has_error()) return nullptr;

    while (match(TokenType::And)) {
        Formula* right = parse_binary_op();
        if (has_error()) return nullptr;
        left = pool_.create_and(left, right);
    }

    return left;
}

// binary_op ::= unary_op ('U' | 'R' unary_op)*
Formula* FormulaParser::parse_binary_op() {
    Formula* left = parse_unary_op();
    if (has_error()) return nullptr;

    while (true) {
        if (match(TokenType::Until)) {
            Formula* right = parse_unary_op();
            if (has_error()) return nullptr;
            left = pool_.create_until(left, right);
        } else if (match(TokenType::Release)) {
            Formula* right = parse_unary_op();
            if (has_error()) return nullptr;
            left = pool_.create_release(left, right);
        } else {
            break;
        }
    }

    return left;
}

// unary_op ::= ('!' | 'X')* primary
// Changed to handle both ! and X at the same level
// This allows parsing formulas like !X(a) correctly
Formula* FormulaParser::parse_unary_op() {
    // Count prefix operators (they are right-associative)
    int not_count = 0;
    int next_count = 0;

    // Consume all ! and X tokens
    while (true) {
        if (match(TokenType::Not)) {
            ++not_count;
        } else if (match(TokenType::Next)) {
            ++next_count;
        } else {
            break;
        }
    }

    // Parse the primary expression
    Formula* expr = parse_primary();
    if (has_error()) return nullptr;

    // Apply operators from right to left (right-associative)
    // X has higher precedence than !, so apply X first
    for (int i = 0; i < next_count; ++i) {
        expr = pool_.create_next(expr);
    }
    for (int i = 0; i < not_count; ++i) {
        expr = pool_.create_not(expr);
    }

    return expr;
}

// postfix ::= '!' postfix | primary
// DEPRECATED: Now merged into unary_op
Formula* FormulaParser::parse_postfix() {
    return parse_unary_op();
}

// primary ::= literal | '(' formula ')'
Formula* FormulaParser::parse_primary() {
    Token tok = peek();

    if (tok.type == TokenType::True) {
        consume();
        return pool_.create_true();
    }

    if (tok.type == TokenType::False) {
        consume();
        return pool_.create_false();
    }

    if (tok.type == TokenType::Identifier) {
        consume();
        return lookup_variable(tok.value);
    }

    if (match(TokenType::LParen)) {
        Formula* expr = parse_formula();
        if (has_error()) return nullptr;

        if (!match(TokenType::RParen)) {
            set_error("Expected ')'");
            return nullptr;
        }

        return expr;
    }

    set_error("Unexpected token: " + tok.value);
    return nullptr;
}

// =============================================================================
// Variable Lookup
// =============================================================================

Formula* FormulaParser::lookup_variable(const std::string& name) {
    // Use get_or_create_variable for auto-declaration
    int var_id = pool_.get_or_create_variable(name);
    return pool_.create(Formula::OpType::Literal, nullptr, nullptr, var_id);
}

} // namespace formula
