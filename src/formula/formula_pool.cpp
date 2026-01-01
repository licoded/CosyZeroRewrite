#include "formula/formula_pool.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace formula {

// ========== Hash Function ==========

size_t FormulaPool::compute_hash(Formula::OpType op, Formula* left,
                                 Formula* right, int var_id) {
    // Hash combination using boost::hash_combine style
    size_t h = static_cast<size_t>(op);

    if (left) {
        h ^= left->hash() + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    if (right) {
        h ^= right->hash() + 0x9e3779b9 + (h << 6) + (h >> 2);
    }

    if (op == Formula::OpType::Literal) {
        h ^= static_cast<size_t>(var_id) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }

    return h;
}

// ========== Equality Function ==========

bool FormulaPool::FormulaEqual::operator()(Formula* a, Formula* b) const noexcept {
    // Fast path: same pointer
    if (a == b) return true;

    // One is null, other is not
    if (!a || !b) return false;

    // Structural comparison: hash + op + children + var_id
    return a->hash() == b->hash() &&
           a->op() == b->op() &&
           a->left() == b->left() &&
           a->right() == b->right() &&
           a->var_id() == b->var_id();
}

// ========== Validation ==========

void FormulaPool::validate_create(Formula::OpType op, Formula* left,
                                 Formula* right, int var_id) {
    switch (op) {
        case Formula::OpType::True:
        case Formula::OpType::False:
        case Formula::OpType::End:
            if (left || right || var_id >= 0) {
                throw std::invalid_argument("Constants cannot have children or var_id");
            }
            break;

        case Formula::OpType::Not:
        case Formula::OpType::Next:
            if (!left) {
                throw std::invalid_argument("Unary operator requires left child");
            }
            if (right) {
                throw std::invalid_argument("Unary operator cannot have right child");
            }
            if (var_id >= 0) {
                throw std::invalid_argument("Unary operator cannot have var_id");
            }
            break;

        case Formula::OpType::And:
        case Formula::OpType::Or:
        case Formula::OpType::Until:
        case Formula::OpType::Release:
            if (!left || !right) {
                throw std::invalid_argument("Binary operator requires both children");
            }
            if (var_id >= 0) {
                throw std::invalid_argument("Binary operator cannot have var_id");
            }
            break;

        case Formula::OpType::Literal:
            if (left || right) {
                throw std::invalid_argument("Literal cannot have children");
            }
            if (var_id < 0) {
                throw std::invalid_argument("Literal requires var_id");
            }
            break;
    }
}

// ========== Construction ==========

FormulaPool::FormulaPool()
    : var_names_()
    , var_ids_()
    , num_outputs_(0)
    , num_inputs_(0)
    , outputs_declared_(false)
    , inputs_declared_(false)
    , true_marker_(nullptr)
    , false_marker_(nullptr)
    , end_marker_(nullptr)
    , moved_from_(false)
{
    // Create singleton constants
    true_marker_ = create(Formula::OpType::True, nullptr, nullptr, -1);
    false_marker_ = create(Formula::OpType::False, nullptr, nullptr, -1);
    end_marker_ = create(Formula::OpType::End, nullptr, nullptr, -1);
}

FormulaPool::~FormulaPool() {
    clear();
}

FormulaPool::FormulaPool(FormulaPool&& other) noexcept
    : unique_table_(std::move(other.unique_table_))
    , formulas_(std::move(other.formulas_))
    , var_names_(std::move(other.var_names_))
    , var_ids_(std::move(other.var_ids_))
    , num_outputs_(other.num_outputs_)
    , num_inputs_(other.num_inputs_)
    , outputs_declared_(other.outputs_declared_)
    , inputs_declared_(other.inputs_declared_)
    , true_marker_(other.true_marker_)
    , false_marker_(other.false_marker_)
    , end_marker_(other.end_marker_)
    , moved_from_(false)
{
    other.moved_from_ = true;
    other.true_marker_ = nullptr;
    other.false_marker_ = nullptr;
    other.end_marker_ = nullptr;
}

FormulaPool& FormulaPool::operator=(FormulaPool&& other) noexcept {
    if (this != &other) {
        clear();

        unique_table_ = std::move(other.unique_table_);
        formulas_ = std::move(other.formulas_);
        var_names_ = std::move(other.var_names_);
        var_ids_ = std::move(other.var_ids_);
        num_outputs_ = other.num_outputs_;
        num_inputs_ = other.num_inputs_;
        outputs_declared_ = other.outputs_declared_;
        inputs_declared_ = other.inputs_declared_;
        true_marker_ = other.true_marker_;
        false_marker_ = other.false_marker_;
        end_marker_ = other.end_marker_;

        other.moved_from_ = true;
        other.true_marker_ = nullptr;
        other.false_marker_ = nullptr;
        other.end_marker_ = nullptr;
    }
    return *this;
}

// ========== Variable Management ==========

void FormulaPool::declare_variables(const std::vector<std::string>& outputs,
                                    const std::vector<std::string>& inputs) {
    if (outputs_declared_ || inputs_declared_) {
        throw std::runtime_error("Variables already declared");
    }

    // Check for duplicates within outputs
    std::unordered_set<std::string> seen;
    for (const auto& name : outputs) {
        if (!seen.insert(name).second) {
            throw std::invalid_argument("Duplicate output: " + name);
        }
    }

    // Check for duplicates within inputs
    for (const auto& name : inputs) {
        if (!seen.insert(name).second) {
            throw std::invalid_argument(
                "Duplicate input (also in outputs): " + name);
        }
    }

    // Declare outputs first
    declare_outputs(outputs);
    declare_inputs(inputs);
}

void FormulaPool::declare_outputs(const std::vector<std::string>& outputs) {
    if (outputs_declared_) {
        throw std::runtime_error("Outputs already declared");
    }

    for (const auto& name : outputs) {
        int id = static_cast<int>(var_names_.size());
        var_names_.push_back(name);
        var_ids_[name] = id;
        num_outputs_++;
    }

    outputs_declared_ = true;
}

void FormulaPool::declare_inputs(const std::vector<std::string>& inputs) {
    if (inputs_declared_) {
        throw std::runtime_error("Inputs already declared");
    }

    for (const auto& name : inputs) {
        int id = static_cast<int>(var_names_.size());
        var_names_.push_back(name);
        var_ids_[name] = id;
        num_inputs_++;
    }

    inputs_declared_ = true;
}

void FormulaPool::load_from_partition(const std::string& partition_file) {
    std::ifstream file(partition_file);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open partition file: " + partition_file);
    }

    std::vector<std::string> outputs;
    std::vector<std::string> inputs;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string token;

        if (line.find(".outputs:") == 0) {
            size_t pos = line.find(':');
            std::string rest = line.substr(pos + 1);
            std::istringstream oss(rest);
            while (oss >> token) {
                if (!token.empty()) outputs.push_back(token);
            }
        } else if (line.find(".inputs:") == 0) {
            size_t pos = line.find(':');
            std::string rest = line.substr(pos + 1);
            std::istringstream oss(rest);
            while (oss >> token) {
                if (!token.empty()) inputs.push_back(token);
            }
        }
    }

    declare_variables(outputs, inputs);
}

void FormulaPool::extract_variables_from_formula(Formula* root) {
    if (!root) return;

    std::unordered_set<std::string> literals;

    // Helper function to collect literals recursively
    std::function<void(Formula*)> collect = [&](Formula* f) {
        if (!f) return;

        if (f->is_literal()) {
            std::string name = "v" + std::to_string(f->var_id());
            // Try to get existing name if variable already declared
            if (f->var_id() < static_cast<int>(var_names_.size())) {
                literals.insert(var_names_[f->var_id()]);
            } else {
                literals.insert(name);
            }
        }

        if (f->left()) collect(f->left());
        if (f->right()) collect(f->right());
    };

    collect(root);

    // Declare all literals as outputs
    if (!literals.empty()) {
        std::vector<std::string> output_list(literals.begin(), literals.end());
        if (!outputs_declared_) {
            declare_outputs(output_list);
        }
        if (!inputs_declared_) {
            declare_inputs({});  // No inputs
        }
    }
}

// ========== Variable Info ==========

int FormulaPool::get_variable_id(const std::string& name) const {
    auto it = var_ids_.find(name);
    if (it == var_ids_.end()) {
        throw std::runtime_error("Variable not declared: " + name);
    }
    return it->second;
}

std::string FormulaPool::get_variable_name(int var_id) const {
    if (var_id < 0 || var_id >= static_cast<int>(var_names_.size())) {
        throw std::runtime_error("Invalid variable ID: " + std::to_string(var_id));
    }
    return var_names_[var_id];
}

// ========== Formula Creation ==========

Formula* FormulaPool::create(Formula::OpType op, Formula* left,
                             Formula* right, int var_id) {
    validate_create(op, left, right, var_id);

    // Check for singleton constants
    if (op == Formula::OpType::True && true_marker_) {
        return true_marker_;
    }
    if (op == Formula::OpType::False && false_marker_) {
        return false_marker_;
    }
    if (op == Formula::OpType::End && end_marker_) {
        return end_marker_;
    }

    // Compute hash for lookup
    size_t h = compute_hash(op, left, right, var_id);

    // Create temporary key for lookup (pool_index = 0 is invalid, used only for lookup)
    Formula key(op, left, right, var_id, h, 0);

    // Search in unique table
    auto it = unique_table_.find(&key);
    if (it != unique_table_.end()) {
        return *it;  // Found existing, return it
    }

    // Not found, create new formula with unique pool_index
    size_t pool_index = formulas_.size();
    Formula* new_formula = new Formula(op, left, right, var_id, h, pool_index);

    // Take ownership
    formulas_.emplace_back(new_formula);

    // Add to unique table
    unique_table_.insert(new_formula);

    return new_formula;
}

Formula* FormulaPool::create_variable(const std::string& name) {
    int var_id = get_variable_id(name);
    return create(Formula::OpType::Literal, nullptr, nullptr, var_id);
}

Formula* FormulaPool::create_true() {
    return true_marker_;
}

Formula* FormulaPool::create_false() {
    return false_marker_;
}

Formula* FormulaPool::create_not(Formula* operand) {
    return create(Formula::OpType::Not, operand, nullptr, -1);
}

Formula* FormulaPool::create_and(Formula* left, Formula* right) {
    return create(Formula::OpType::And, left, right, -1);
}

Formula* FormulaPool::create_or(Formula* left, Formula* right) {
    return create(Formula::OpType::Or, left, right, -1);
}

Formula* FormulaPool::create_next(Formula* operand) {
    return create(Formula::OpType::Next, operand, nullptr, -1);
}

Formula* FormulaPool::create_until(Formula* left, Formula* right) {
    return create(Formula::OpType::Until, left, right, -1);
}

Formula* FormulaPool::create_release(Formula* left, Formula* right) {
    return create(Formula::OpType::Release, left, right, -1);
}

Formula* FormulaPool::create_end() {
    return end_marker_;
}

// ========== Resource Management ==========

void FormulaPool::clear() {
    // Clear unique table first (formulas_ owns the memory)
    unique_table_.clear();
    formulas_.clear();

    // Reset singleton markers (will be recreated if needed)
    true_marker_ = nullptr;
    false_marker_ = nullptr;
    end_marker_ = nullptr;
}

} // namespace formula
