/**
 * @file tableau.cpp
 * @brief Implementation of on-the-fly tableau construction
 */

#include "automata/tableau.hpp"
#include "log/logger.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace automata {

//==============================================================================
// FormulaEqual implementation
//==============================================================================

bool FormulaEqual::operator()(formula::Formula* a, formula::Formula* b) const noexcept {
    if (a == b) return true;
    if (!a || !b) return false;

    // Fast path: different hash means different formulas
    if (a->hash() != b->hash()) return false;

    // Structural comparison
    if (a->op() != b->op()) return false;
    if (a->var_id() != b->var_id()) return false;

    // Compare children
    formula::Formula* a_left = a->left();
    formula::Formula* a_right = a->right();
    formula::Formula* b_left = b->left();
    formula::Formula* b_right = b->right();

    if (a_left == b_left && a_right == b_right) return true;
    if (!a_left || !b_left || !a_right || !b_right) return false;

    return (*this)(a_left, b_left) && (*this)(a_right, b_right);
}

//==============================================================================
// TableauState
//==============================================================================

namespace {

// Helper: compute hash for a set of formulas
size_t compute_formula_set_hash(const TableauState::FormulaSet& formulas) {
    size_t h = 0;
    for (formula::Formula* f : formulas) {
        if (f) {
            h ^= f->hash() + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
    }
    return h;
}

// Helper: check if a formula is a literal (positive or negative)
bool is_literal(formula::Formula* f) {
    if (!f) return false;
    if (f->op() == formula::Formula::OpType::Literal) return true;
    if (f->op() == formula::Formula::OpType::Not) {
        formula::Formula* child = f->left();
        return child && child->op() == formula::Formula::OpType::Literal;
    }
    return false;
}

// Helper: evaluate literal against assignment
// Returns true if the literal should be kept in the next state
bool literal_value(formula::Formula* f, const Assignment& assignment) {
    if (!f) return true;  // Keep non-formulas

    if (f->op() == formula::Formula::OpType::Literal) {
        // Positive literal: true if variable is in assignment
        return assignment.count(f->var_id()) > 0;
    }

    if (f->op() == formula::Formula::OpType::Not) {
        formula::Formula* child = f->left();
        if (child && child->op() == formula::Formula::OpType::Literal) {
            // Negative literal !v: true if variable is NOT in assignment
            return assignment.count(child->var_id()) == 0;
        }
    }

    // Non-literals are kept
    return true;
}

} // namespace

std::unique_ptr<TableauState> TableauState::initial(formula::Formula* phi, formula::FormulaPool& pool) {
    FormulaSet formulas;
    formulas.insert(phi);
    return std::unique_ptr<TableauState>(new TableauState(std::move(formulas)));
}

TableauState::TableauState(FormulaSet formulas)
    : formulas_(std::move(formulas)), hash_(compute_formula_set_hash(formulas_)) {
}

bool TableauState::is_locally_consistent() const {
    // Rule: false in state → inconsistent
    for (formula::Formula* f : formulas_) {
        if (f && f->is_false()) {
            return false;
        }
    }

    // Check for contradictory literals (p and !p)
    for (formula::Formula* f : formulas_) {
        if (!f) continue;

        // Check if this is a positive literal
        if (f->op() == formula::Formula::OpType::Literal) {
            int var_id = f->var_id();

            // Check if !var is also in the state
            for (formula::Formula* g : formulas_) {
                if (!g) continue;
                if (g->op() == formula::Formula::OpType::Not) {
                    formula::Formula* child = g->left();
                    if (child && child->op() == formula::Formula::OpType::Literal &&
                        child->var_id() == var_id) {
                        return false;  // Contradiction: v and !v
                    }
                }
            }
        }
    }

    // For each formula, check consistency rules
    for (formula::Formula* f : formulas_) {
        if (!f) continue;

        switch (f->op()) {
        case formula::Formula::OpType::And: {
            // (ψ1 ∧ ψ2) in Γ → ψ1 ∈ Γ AND ψ2 ∈ Γ
            formula::Formula* left = f->left();
            formula::Formula* right = f->right();
            if (formulas_.count(left) == 0 || formulas_.count(right) == 0) {
                return false;
            }
            break;
        }
        case formula::Formula::OpType::Or: {
            // (ψ1 ∨ ψ2) in Γ → ψ1 ∈ Γ OR ψ2 ∈ Γ
            formula::Formula* left = f->left();
            formula::Formula* right = f->right();
            if (formulas_.count(left) == 0 && formulas_.count(right) == 0) {
                return false;
            }
            break;
        }
        case formula::Formula::OpType::Until: {
            // (ψ1 U ψ2) in Γ → ψ2 ∈ Γ OR (ψ1 ∈ Γ AND (ψ1 U ψ2) ∈ Γ)
            formula::Formula* left = f->left();
            formula::Formula* right = f->right();
            bool has_right = formulas_.count(right) > 0;
            bool has_left = formulas_.count(left) > 0;
            if (!has_right && !has_left) {
                return false;
            }
            break;
        }
        case formula::Formula::OpType::Release: {
            // (ψ1 R ψ2) in Γ → ψ2 ∈ Γ (ψ1 will be provided via Release continuation)
            // Only need to check right side for local consistency
            formula::Formula* right = f->right();
            // Special case: if right is false, then this Release is unsatisfiable
            // unless left is also false (which makes the whole thing equivalent to false)
            if (right && right->is_false()) {
                // (ψ1 R false) requires ψ1 to hold forever
                // For local consistency, we need ψ1 in state OR it will be provided later
                // The Release continuation will provide ψ1 in subsequent steps
                // So we only check that we don't have an explicit requirement for !ψ1
                // For now, let's just say it's consistent (the game will handle it)
                return true;  // Consistent, but may be unrealizable depending on game
            }
            if (formulas_.count(right) == 0) {
                return false;
            }
            break;
        }
        default:
            // Other operators are fine
            break;
        }
    }

    return true;
}

bool TableauState::is_accepting() const {
    // Check for false
    for (formula::Formula* f : formulas_) {
        if (f && f->is_false()) {
            return false;
        }
    }

    // Check for local consistency - inconsistent states are not accepting
    if (!is_locally_consistent()) {
        return false;
    }

    // Check Until formulas: in LTLf, all Until must have right side satisfied
    // If ψ1 U ψ2 is in state but ψ2 is not, the Until is still waiting
    for (formula::Formula* f : formulas_) {
        if (f && f->op() == formula::Formula::OpType::Until) {
            formula::Formula* right = f->right();
            // If Until is still in state, right side must be satisfied
            if (formulas_.count(right) == 0) {
                return false;  // Until still waiting for right side
            }
        }
    }

    return true;
}

std::vector<formula::Formula*> TableauState::get_old_formulas() const {
    std::vector<formula::Formula*> result;

    for (formula::Formula* f : formulas_) {
        if (!f) continue;

        // Keep formulas that are not purely temporal
        switch (f->op()) {
        case formula::Formula::OpType::Next:
        case formula::Formula::OpType::Until:
        case formula::Formula::OpType::Release:
            // Not in old()
            break;
        default:
            // In old()
            result.push_back(f);
            break;
        }
    }

    return result;
}

std::vector<formula::Formula*> TableauState::get_next_formulas() const {
    std::vector<formula::Formula*> result;

    for (formula::Formula* f : formulas_) {
        if (!f) continue;

        switch (f->op()) {
        case formula::Formula::OpType::Next: {
            // ○ψ → ψ
            result.push_back(f->left());
            break;
        }
        case formula::Formula::OpType::Release: {
            // (ψ1 R ψ2) → ψ2 (Release continues)
            result.push_back(f->right());
            break;
        }
        default:
            break;
        }
    }

    return result;
}

bool TableauState::is_temporal(formula::Formula* f) {
    if (!f) return false;
    auto op = f->op();
    return op == formula::Formula::OpType::Next ||
           op == formula::Formula::OpType::Until ||
           op == formula::Formula::OpType::Release;
}

TableauState::FormulaSet
TableauState::evaluate_literals(const FormulaSet& formulas, const Assignment& assignment) {
    FormulaSet result;

    for (formula::Formula* f : formulas) {
        if (!f) continue;

        if (is_literal(f)) {
            // Only keep literal if it evaluates to true
            if (literal_value(f, assignment)) {
                result.insert(f);
            }
        } else {
            // Non-literals are always kept
            result.insert(f);
        }
    }

    return result;
}

std::unique_ptr<TableauState>
TableauState::next(const Assignment& assignment, formula::FormulaPool& pool) const {
    // Step 1: Evaluate literals against assignment
    auto current_formulas = evaluate_literals(formulas_, assignment);

    // Step 2: Build next state formula set
    // Γ' = old(Γ) ∪ next(Γ)

    FormulaSet next_formulas;

    // Add old formulas (non-temporal)
    for (formula::Formula* f : current_formulas) {
        if (!is_temporal(f)) {
            next_formulas.insert(f);
        }
    }

    // Add next formulas (under Next, released by Release)
    for (formula::Formula* f : current_formulas) {
        if (f->op() == formula::Formula::OpType::Next) {
            // ○ψ → ψ
            if (f->left()) {
                next_formulas.insert(f->left());
            }
        } else if (f->op() == formula::Formula::OpType::Release) {
            // (ψ1 R ψ2) → ψ2 AND (ψ1 R ψ2) continues
            // Release must keep both right side and the Release itself
            if (f->right()) {
                next_formulas.insert(f->right());
            }
            // Keep the Release formula for next iteration
            next_formulas.insert(f);
        }
    }

    // Until formulas are handled specially:
    // (ψ1 U ψ2) in Γ means: either ψ2 is true (until satisfied), or ψ1 continues
    for (formula::Formula* f : current_formulas) {
        if (f->op() == formula::Formula::OpType::Until) {
            formula::Formula* right = f->right();  // ψ2

            // Check if right side is true
            bool right_true = false;
            if (is_literal(right)) {
                right_true = literal_value(right, assignment);
            } else if (right && right->is_true()) {
                right_true = true;
            } else if (current_formulas.count(right) > 0 && !is_temporal(right)) {
                // right is in current formulas (non-temporal)
                right_true = true;
            }

            if (!right_true) {
                // ψ2 not true, keep (ψ1 U ψ2) for next iteration
                next_formulas.insert(f);
            }
        }
    }

    return std::unique_ptr<TableauState>(new TableauState(std::move(next_formulas)));
}

bool TableauState::operator==(const TableauState& other) const {
    if (this == &other) return true;
    if (hash_ != other.hash_) return false;
    return formulas_ == other.formulas_;
}

std::string TableauState::to_string() const {
    std::ostringstream oss;
    oss << "{";

    bool first = true;
    for (formula::Formula* f : formulas_) {
        if (!first) oss << ", ";
        first = false;

        if (f) {
            oss << f->to_string();
        } else {
            oss << "null";
        }
    }

    oss << "}";
    return oss.str();
}

size_t TableauState::compute_hash(const FormulaSet& formulas) {
    return compute_formula_set_hash(formulas);
}

//==============================================================================
// TableauStatePool
//==============================================================================

TableauState* TableauStatePool::get_or_create(TableauState::FormulaSet formulas) {
    // Create a temporary state to check for existence
    auto temp = std::unique_ptr<TableauState>(new TableauState(std::move(formulas)));

    // Check if equivalent state exists
    auto it = states_.find(temp.get());
    if (it != states_.end()) {
        return *it;
    }

    // Add new state
    TableauState* raw_ptr = temp.get();
    states_.insert(raw_ptr);

    // Move into storage for ownership
    storage_.push_back(std::move(temp));

    LOG_DEBUG("TableauStatePool: created new state, total=", states_.size());

    return raw_ptr;
}

void TableauStatePool::clear() {
    states_.clear();
    storage_.clear();
}

//==============================================================================
// OnTheFlyDFA
//==============================================================================

OnTheFlyDFA::OnTheFlyDFA(formula::Formula* phi, formula::FormulaPool& pool)
    : pool_(pool),
      transition_cache_(16, CacheKeyHash{}, CacheKeyEqual{}) {
    LOG_DEBUG("OnTheFlyDFA: constructing from formula: ", phi->to_string());

    // Convert to NNF first
    formula::Formula* nnf_phi = phi->nnf(pool_);
    LOG_DEBUG("OnTheFlyDFA: NNF: ", nnf_phi->to_string());

    // Create initial state
    auto init_state = TableauState::initial(nnf_phi, pool_);
    initial_state_ = state_pool_.get_or_create(std::move(init_state->formulas()));

    LOG_DEBUG("OnTheFlyDFA: initial state: ", initial_state_->to_string());
}

TableauState* OnTheFlyDFA::successor(TableauState* q, const Assignment& assignment) {
    // Check cache
    auto key = std::make_pair(q, assignment);
    auto it = transition_cache_.find(key);
    if (it != transition_cache_.end()) {
        return it->second;
    }

    LOG_DEBUG("OnTheFlyDFA: computing successor");

    // Compute next state
    auto next_state = q->next(assignment, pool_);

    // Get or create from pool
    TableauState* result = state_pool_.get_or_create(std::move(next_state->formulas()));

    // Cache and track
    transition_cache_[key] = result;
    expanded_states_.insert(q);

    LOG_DEBUG("OnTheFlyDFA: successor = ", result->to_string());

    return result;
}

//==============================================================================
// AssignmentGenerator
//==============================================================================

AssignmentGenerator::AssignmentGenerator(int num_variables)
    : num_variables_(num_variables) {
}

std::vector<Assignment> AssignmentGenerator::all_assignments() const {
    std::vector<Assignment> result;
    int num = 1 << num_variables_;  // 2^n

    result.reserve(num);
    for (int i = 0; i < num; ++i) {
        result.push_back(from_bitset(i, num_variables_));
    }

    return result;
}

Assignment AssignmentGenerator::from_bitset(int i, int num_variables) {
    Assignment result;
    for (int v = 0; v < num_variables; ++v) {
        if (i & (1 << v)) {
            result.insert(v);
        }
    }
    return result;
}

std::string AssignmentGenerator::to_string(const Assignment& a, int num_variables) {
    std::ostringstream oss;
    oss << "{";
    for (int v = 0; v < num_variables; ++v) {
        if (v > 0) oss << ", ";
        oss << "x" << v << "=" << (a.count(v) ? "1" : "0");
    }
    oss << "}";
    return oss.str();
}

} // namespace automata
