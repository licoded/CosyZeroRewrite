#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include <sstream>

namespace formula {

// ========== Helper Functions for Smart Parentheses ==========

// Check if a string is already wrapped in parentheses
// Handles cases like "(expr)" but not "(expr)(more)" or nested "(expr"
static bool is_wrapped_in_parens(const std::string& s) {
    if (s.empty()) return false;
    if (s[0] != '(') return false;
    if (s.back() != ')') return false;

    // Check that the closing paren matches the opening one
    // (simple check: if we remove the outer parens, the content should be balanced)
    int depth = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '(') depth++;
        else if (s[i] == ')') depth--;
        if (depth == 0 && i < s.size() - 1) {
            // Found closing paren before end - not wrapped
            return false;
        }
    }
    return depth == 0;
}

// Returns operator precedence for output formatting (HIGHER number = HIGHER precedence = binds tighter)
// Parser order (low to high): Or(1) < And(2) < Until/Release(3) < Not/Next(4) < Primary(5)
static int get_precedence(Formula::OpType op) {
    switch (op) {
        case Formula::OpType::Or:
            return 1;
        case Formula::OpType::And:
            return 2;
        case Formula::OpType::Until:
        case Formula::OpType::Release:
            return 3;
        case Formula::OpType::Not:
        case Formula::OpType::Next:
            return 4;
        case Formula::OpType::Literal:
        case Formula::OpType::True:
        case Formula::OpType::False:
            return 5;  // Highest (primary)
        default:
            return 0;
    }
}

// Check if a child expression needs parentheses when used as a child of parent
// Rule: Binary operators (And/Or/Until/Release) as children always get parentheses
// This ensures clarity and correct roundtrip parsing
static bool needs_parentheses(Formula* child, Formula::OpType parent_op, bool is_left_child) {
    if (!child) return false;

    // Literals, true, false never need parentheses
    if (child->op() == Formula::OpType::Literal ||
        child->op() == Formula::OpType::True ||
        child->op() == Formula::OpType::False) {
        return false;
    }

    // Not and Next never need parentheses (they are prefix operators)
    if (child->op() == Formula::OpType::Not || child->op() == Formula::OpType::Next) {
        return false;
    }

    // Binary operators (And/Or/Until/Release) as children always need parentheses
    // This ensures: (p1 & p2) | p3 outputs (p1 & p2) | p3, not p1 & p2 | p3
    // And: (p1 | p2) & p3 outputs (p1 | p2) & p3, not p1 | p2 & p3
    if (child->op() == Formula::OpType::And ||
        child->op() == Formula::OpType::Or ||
        child->op() == Formula::OpType::Until ||
        child->op() == Formula::OpType::Release) {
        return true;
    }

    return false;
}

// ========== Private Constructor ==========

Formula::Formula(OpType op, Formula* left, Formula* right,
                 int var_id, size_t hash, size_t pool_index)
    : op_(op)
    , left_(left)
    , right_(right)
    , var_id_(var_id)
    , hash_(hash)
    , pool_index_(pool_index)
{
}

// ========== String Representation ==========

const char* Formula::op_name() const {
    switch (op_) {
        case OpType::True:    return "True";
        case OpType::False:   return "False";
        case OpType::Not:     return "!";
        case OpType::And:     return "&";
        case OpType::Or:      return "|";
        case OpType::Next:    return "X";
        case OpType::Until:   return "U";
        case OpType::Release: return "R";
        case OpType::End:     return "End";
        case OpType::Literal: return "Literal";
        default:              return "?";
    }
}

std::string Formula::to_string() const {
    std::ostringstream oss;

    switch (op_) {
        case OpType::True:
            return "true";

        case OpType::False:
            return "false";

        case OpType::End:
            return "end";

        case OpType::Literal:
            oss << "v" << var_id_;
            return oss.str();

        case OpType::Not:
            oss << "!";
            if (left_) {
                bool needs_paren = needs_parentheses(left_, OpType::Not, false);
                if (needs_paren) oss << "(";
                oss << left_->to_string();
                if (needs_paren) oss << ")";
            }
            return oss.str();

        case OpType::And:
        case OpType::Or:
        case OpType::Until:
        case OpType::Release: {
            // Left child
            if (left_) {
                bool left_needs_paren = needs_parentheses(left_, op_, true);
                if (left_needs_paren) oss << "(";
                oss << left_->to_string();
                if (left_needs_paren) oss << ")";
            }
            oss << " " << op_name() << " ";
            // Right child
            if (right_) {
                bool right_needs_paren = needs_parentheses(right_, op_, false);
                if (right_needs_paren) oss << "(";
                oss << right_->to_string();
                if (right_needs_paren) oss << ")";
            }
            return oss.str();
        }

        case OpType::Next:
            // Next outputs X(...), but skip parens if child already has them
            oss << "X";
            if (left_) {
                std::string child_str = left_->to_string();
                if (is_wrapped_in_parens(child_str)) {
                    oss << child_str;  // Already has parens, reuse them
                } else {
                    oss << "(" << child_str << ")";
                }
            }
            return oss.str();
    }

    return "?";
}

std::string Formula::to_string_with_names(const FormulaPool& pool) const {
    std::ostringstream oss;

    switch (op_) {
        case OpType::True:
            return "true";

        case OpType::False:
            return "false";

        case OpType::End:
            return "End";

        case OpType::Literal:
            return pool.get_variable_name(var_id_);

        case OpType::Not:
            oss << "!";
            if (left_) {
                bool needs_paren = needs_parentheses(left_, OpType::Not, false);
                if (needs_paren) oss << "(";
                oss << left_->to_string_with_names(pool);
                if (needs_paren) oss << ")";
            }
            return oss.str();

        case OpType::And:
        case OpType::Or:
        case OpType::Until:
        case OpType::Release: {
            // Left child
            if (left_) {
                bool left_needs_paren = needs_parentheses(left_, op_, true);
                if (left_needs_paren) oss << "(";
                oss << left_->to_string_with_names(pool);
                if (left_needs_paren) oss << ")";
            }
            oss << " " << op_name() << " ";
            // Right child
            if (right_) {
                bool right_needs_paren = needs_parentheses(right_, op_, false);
                if (right_needs_paren) oss << "(";
                oss << right_->to_string_with_names(pool);
                if (right_needs_paren) oss << ")";
            }
            return oss.str();
        }

        case OpType::Next:
            // Next outputs X(...), but skip parens if child already has them
            oss << "X";
            if (left_) {
                std::string child_str = left_->to_string_with_names(pool);
                if (is_wrapped_in_parens(child_str)) {
                    oss << child_str;  // Already has parens, reuse them
                } else {
                    oss << "(" << child_str << ")";
                }
            }
            return oss.str();
    }

    return "?";
}

// ========== Operations (implemented in separate files) ==========
// nnf() - implemented in nnf.cpp
// simplify() - implemented in simplify.cpp
// xnf_with_tail() - implemented in xnf.cpp
// rmnext() - implemented in rmnext.cpp

} // namespace formula
