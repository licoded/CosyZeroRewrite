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
    // Auto-load variable names from pool for multi-char variable support
    var_names_ = pool_.get_all_variable_names();
    use_multi_char_vars_ = !var_names_.empty();
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

            case '-':
                // Check for -> (implies)
                if (i + 1 < input.size() && input[i + 1] == '>') {
                    token.type = TokenType::Implies;
                    token.value = "->";
                    tokens_.push_back(token);
                    i += 2;
                    continue;
                }
                // Otherwise, - is not a valid token (fall through to error)
                break;

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
                // Only uppercase X is Next operator
                token.type = TokenType::Next;
                token.value = "X";
                tokens_.push_back(token);
                ++i;
                continue;

            case 'U':
                // Only uppercase U is Until operator
                token.type = TokenType::Until;
                token.value = "U";
                tokens_.push_back(token);
                ++i;
                continue;

        }

        // Handle ambiguous single-char operators that could start identifiers
        // 'F' could be Finally or start of identifier
        // 'G' could be Globally or start of identifier
        // 'R' could be Release or start of identifier
        // Peek ahead: if followed by another letter, it's an identifier start
        if ((c == 'R' || c == 'F' || c == 'G') &&
            i + 1 < input.size() &&
            std::isalpha(static_cast<unsigned char>(input[i + 1]))) {
            // This is the start of a multi-character identifier, fall through to identifier handling
        } else if (c == 'R') {
            token.type = TokenType::Release;
            token.value = "R";
            tokens_.push_back(token);
            ++i;
            continue;
        } else if (c == 'F') {
            // F is handled in the identifier section for syntactic sugar consistency
            // Fall through to identifier handling
        } else if (c == 'G') {
            // G is handled in the identifier section for syntactic sugar consistency
            // Fall through to identifier handling
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

            // Check for keywords (case-sensitive: only uppercase F/G are operators)
            if (value == "true") {
                token.type = TokenType::True;
            } else if (value == "false") {
                token.type = TokenType::False;
            } else if (value == "F") {
                // F (finally/eventually) is syntactic sugar for true U ...
                token.type = TokenType::Finally;
            } else if (value == "G") {
                // G (globally) is syntactic sugar for false R ...
                token.type = TokenType::Globally;
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

// formula ::= implies_expr
Formula* FormulaParser::parse_formula() {
    return parse_implies_expr();
}

// implies_expr ::= or_expr ('->' or_expr)*
// Implies is right-associative and has lower precedence than Or
// a -> b -> c is parsed as a -> (b -> c), which becomes !a | (!b | c)
Formula* FormulaParser::parse_implies_expr() {
    // Parse left side (or_expr)
    Formula* left = parse_or_expr();
    if (has_error()) return nullptr;

    // Collect all implies in a list (for right-associativity)
    std::vector<Formula*> operands;
    operands.push_back(left);

    while (match(TokenType::Implies)) {
        Formula* right = parse_or_expr();
        if (has_error()) return nullptr;
        operands.push_back(right);
    }

    // Build right-associative tree: a -> b -> c = !a | (!b | c)
    // Process from right to left
    for (size_t i = operands.size() - 1; i > 0; --i) {
        Formula* lhs = operands[i - 1];
        Formula* rhs = operands[i];
        // lhs -> rhs = !lhs | rhs
        Formula* negated_lhs = pool_.create_not(lhs);
        operands[i - 1] = pool_.create_or(negated_lhs, rhs);
    }

    return operands[0];
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

// primary ::= literal | '(' formula ')' | 'F' '(' formula ')' | 'G' '(' formula ')'
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

    // Handle F(expr) - syntactic sugar for true U expr
    if (tok.type == TokenType::Finally) {
        consume();  // consume F
        if (!match(TokenType::LParen)) {
            set_error("Expected '(' after F");
            return nullptr;
        }
        Formula* expr = parse_formula();
        if (has_error()) return nullptr;
        if (!match(TokenType::RParen)) {
            set_error("Expected ')' after F(...)");
            return nullptr;
        }
        // F(expr) = true U expr
        return pool_.create_until(pool_.create_true(), expr);
    }

    // Handle G(expr) - syntactic sugar for false R expr
    if (tok.type == TokenType::Globally) {
        consume();  // consume G
        if (!match(TokenType::LParen)) {
            set_error("Expected '(' after G");
            return nullptr;
        }
        Formula* expr = parse_formula();
        if (has_error()) return nullptr;
        if (!match(TokenType::RParen)) {
            set_error("Expected ')' after G(...)");
            return nullptr;
        }
        // G(expr) = false R expr
        return pool_.create_release(pool_.create_false(), expr);
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
