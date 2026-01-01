#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include <sstream>

namespace formula {

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
            oss << "!" << (left_ ? left_->to_string() : "?");
            return oss.str();

        case OpType::And:
        case OpType::Or:
        case OpType::Until:
        case OpType::Release: {
            oss << "(";
            if (left_) oss << left_->to_string();
            oss << " " << op_name() << " ";
            if (right_) oss << right_->to_string();
            oss << ")";
            return oss.str();
        }

        case OpType::Next:
            oss << "X(" << (left_ ? left_->to_string() : "?") << ")";
            return oss.str();
    }

    return "?";
}

std::string Formula::to_verbose_string(const FormulaPool& pool) const {
    std::ostringstream oss;

    switch (op_) {
        case OpType::True:
            return "True";

        case OpType::False:
            return "False";

        case OpType::End:
            return "End";

        case OpType::Literal:
            return pool.get_variable_name(var_id_);

        case OpType::Not:
            oss << "!(" << (left_ ? left_->to_verbose_string(pool) : "?") << ")";
            return oss.str();

        case OpType::And:
        case OpType::Or:
        case OpType::Until:
        case OpType::Release: {
            oss << "(";
            if (left_) oss << left_->to_verbose_string(pool);
            oss << " " << op_name() << " ";
            if (right_) oss << right_->to_verbose_string(pool);
            oss << ")";
            return oss.str();
        }

        case OpType::Next:
            oss << "X(" << (left_ ? left_->to_verbose_string(pool) : "?") << ")";
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
