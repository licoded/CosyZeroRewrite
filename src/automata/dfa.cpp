#include "automata/dfa.hpp"
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <queue>

namespace automata {

// ========== StateFormulaSet ==========

std::string StateFormulaSet::to_string(const formula::FormulaPool& pool) const {
    if (empty()) return "{}";

    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (auto f : formulas_) {
        if (!first) oss << ", ";
        oss << f->to_string();
        first = false;
    }
    oss << "}";
    return oss.str();
}

// ========== TransitionLabel ==========

std::string TransitionLabel::to_string() const {
    std::ostringstream oss;
    oss << "[";
    bool first = true;
    for (int v : positive_vars) {
        if (!first) oss << ", ";
        oss << "p" << v << "=true";
        first = false;
    }
    for (int v : negative_vars) {
        if (!first) oss << ", ";
        oss << "p" << v << "=false";
        first = false;
    }
    oss << "]";
    return oss.str();
}

// ========== DFA ==========

StateId DFA::add_state(const StateFormulaSet& formulas) {
    // Check if state already exists
    size_t h = formulas.hash();
    auto it = state_lookup_.find(h);
    if (it != state_lookup_.end()) {
        // Verify it's actually the same state (handle hash collision)
        if (states_[it->second].formulas.equals(formulas)) {
            return it->second;
        }
    }

    // Add new state
    StateId id = states_.size();
    states_.emplace_back(formulas);
    state_lookup_[h] = id;
    return id;
}

const StateFormulaSet& DFA::get_state_formulas(StateId id) const {
    if (id >= states_.size()) {
        throw std::out_of_range("Invalid state ID");
    }
    return states_[id].formulas;
}

void DFA::set_accepting(StateId id, bool accepting) {
    if (id >= states_.size()) {
        throw std::out_of_range("Invalid state ID");
    }
    states_[id].accepting = accepting;
}

bool DFA::is_accepting(StateId id) const {
    if (id >= states_.size()) {
        throw std::out_of_range("Invalid state ID");
    }
    return states_[id].accepting;
}

void DFA::add_transition(StateId from, StateId to, const TransitionLabel& label) {
    if (from >= states_.size() || to >= states_.size()) {
        throw std::out_of_range("Invalid state ID");
    }
    states_[from].transitions.emplace_back(from, to, label);
}

const std::vector<Transition>& DFA::get_transitions(StateId from) const {
    if (from >= states_.size()) {
        throw std::out_of_range("Invalid state ID");
    }
    return states_[from].transitions;
}

std::vector<StateId> DFA::get_all_states() const {
    std::vector<StateId> result;
    result.reserve(states_.size());
    for (size_t i = 0; i < states_.size(); ++i) {
        result.push_back(i);
    }
    return result;
}

std::unique_ptr<DFA> DFA::clone() const {
    auto dfa = std::make_unique<DFA>();
    dfa->states_ = states_;
    dfa->state_lookup_ = state_lookup_;
    dfa->initial_state_ = initial_state_;
    dfa->next_state_id_ = next_state_id_;
    return dfa;
}

void DFA::print(const formula::FormulaPool& pool) const {
    std::cout << "DFA with " << num_states() << " states\n";
    std::cout << "Initial: " << initial_state_ << "\n";

    for (size_t i = 0; i < states_.size(); ++i) {
        const auto& state = states_[i];
        std::cout << "State " << i;
        if (state.accepting) std::cout << " (accepting)";
        std::cout << ": " << state.formulas.to_string(pool) << "\n";

        for (const auto& trans : state.transitions) {
            std::cout << "  -> " << trans.to
                      << " " << trans.label.to_string() << "\n";
        }
    }
}

std::string DFA::to_dot(const formula::FormulaPool& pool) const {
    std::ostringstream oss;
    oss << "digraph DFA {\n";
    oss << "  rankdir=LR;\n";
    oss << "  node [shape=circle];\n";

    // Initial state marker
    oss << "  init [shape=point, style=invis];\n";
    oss << "  init -> " << initial_state_ << ";\n";

    for (size_t i = 0; i < states_.size(); ++i) {
        const auto& state = states_[i];
        if (state.accepting) {
            oss << "  " << i << " [shape=doublecircle];\n";
        }
    }

    for (size_t i = 0; i < states_.size(); ++i) {
        const auto& state = states_[i];
        for (const auto& trans : state.transitions) {
            oss << "  " << i << " -> " << trans.to
                << " [label=\"" << trans.label.to_string() << "\"];\n";
        }
    }

    oss << "}\n";
    return oss.str();
}

// ========== DFABuilder - Improved Implementation ==========

namespace {

/**
 * @brief Helper to evaluate if a formula is satisfied by an assignment
 */
struct FormulaEvaluator {
    const std::unordered_set<int>& true_vars;
    const std::unordered_set<int>& false_vars;

    bool evaluate(formula::Formula* f) const {
        if (!f) return false;

        switch (f->op()) {
            case formula::Formula::OpType::True:
                return true;
            case formula::Formula::OpType::False:
                return false;
            case formula::Formula::OpType::End:
                return true;  // End is always true at the end
            case formula::Formula::OpType::Literal: {
                int var_id = f->var_id();
                return true_vars.count(var_id) > 0;
            }
            case formula::Formula::OpType::Not: {
                return !evaluate(f->left());
            }
            case formula::Formula::OpType::And: {
                return evaluate(f->left()) && evaluate(f->right());
            }
            case formula::Formula::OpType::Or: {
                return evaluate(f->left()) || evaluate(f->right());
            }
            case formula::Formula::OpType::Next:
            case formula::Formula::OpType::Until:
            case formula::Formula::OpType::Release:
                // These are handled during state construction
                return false;
            default:
                return false;
        }
    }

    bool is_satisfied(formula::Formula* f) const {
        return evaluate(f);
    }
};

} // anonymous namespace

std::unique_ptr<DFA> DFABuilder::build_from_formula(
    formula::Formula* formula,
    formula::FormulaPool& pool
) {
    auto dfa = std::make_unique<DFA>();

    // Special case: true formula
    if (formula->is_true()) {
        StateFormulaSet accepting_state;
        accepting_state.add(pool.create_true());
        StateId id = dfa->add_state(accepting_state);
        dfa->set_initial_state(id);
        dfa->set_accepting(id, true);
        return dfa;
    }

    // Special case: false formula
    if (formula->is_false()) {
        StateFormulaSet rejecting_state;
        rejecting_state.add(pool.create_false());
        StateId id = dfa->add_state(rejecting_state);
        dfa->set_initial_state(id);
        dfa->set_accepting(id, false);
        return dfa;
    }

    // General case: use tableau construction
    // Step 1: Convert to XNF for easier handling
    formula::Formula* xnf = formula->xnf_with_end_marker(pool);
    formula::Formula* nnf_xnf = xnf->nnf(pool);

    // Step 2: Extract variables to create possible worlds
    std::unordered_set<int> all_vars;
    extract_variables(nnf_xnf, all_vars);

    // Step 3: Build initial state (contains the formula)
    StateFormulaSet initial_state;
    initial_state.add(nnf_xnf);
    expand_state(initial_state, nnf_xnf, pool);
    StateId initial_id = dfa->add_state(initial_state);
    dfa->set_initial_state(initial_id);

    // Step 4: BFS to build all reachable states
    std::queue<StateId> queue;
    std::unordered_set<size_t> visited_hashes;

    queue.push(initial_id);
    visited_hashes.insert(initial_state.hash());

    while (!queue.empty()) {
        StateId current_id = queue.front();
        queue.pop();

        const StateFormulaSet& current_set = dfa->get_state_formulas(current_id);

        // Determine if current state is accepting
        // A state is accepting if it can "finish" the trace
        bool accepting = is_accepting_state(current_set);
        dfa->set_accepting(current_id, accepting);

        // Generate successors based on X-formulas
        std::vector<StateFormulaSet> successors = compute_successors(current_set, pool);

        for (const auto& succ : successors) {
            size_t succ_hash = succ.hash();
            StateId succ_id;

            auto it = visited_hashes.find(succ_hash);
            if (it == visited_hashes.end()) {
                // New state
                succ_id = dfa->add_state(succ);
                visited_hashes.insert(succ_hash);
                queue.push(succ_id);
            } else {
                // Find existing state with same hash
                // (in practice, would search for exact match)
                succ_id = dfa->add_state(succ);  // add_state handles duplicates
            }

            // Add transition
            TransitionLabel label;  // Simplified - would be filled with assignment
            dfa->add_transition(current_id, succ_id, label);
        }
    }

    return dfa;
}

void DFABuilder::extract_variables(
    formula::Formula* f,
    std::unordered_set<int>& vars
) {
    if (!f) return;

    if (f->is_literal()) {
        vars.insert(f->var_id());
    } else if (f->is_not() && f->left()) {
        if (f->left()->is_literal()) {
            vars.insert(f->left()->var_id());
        } else {
            extract_variables(f->left(), vars);
        }
    } else {
        extract_variables(f->left(), vars);
        extract_variables(f->right(), vars);
    }
}

void DFABuilder::expand_state(
    StateFormulaSet& state,
    formula::Formula* formula,
    formula::FormulaPool& pool
) {
    // Expand the state with logical consequences
    // This is simplified - a full implementation would do more reasoning

    std::queue<formula::Formula*> to_process;
    std::unordered_set<PrimitiveFormula, PrimitiveFormulaHash, PrimitiveFormulaEqual> processed;

    // Add all formulas in state to queue
    for (auto f : state.formulas()) {
        to_process.push(f);
        processed.insert(f);
    }

    while (!to_process.empty()) {
        formula::Formula* f = to_process.front();
        to_process.pop();

        if (!f) continue;

        switch (f->op()) {
            case formula::Formula::OpType::And:
                // Add both conjuncts
                if (f->left() && !state.contains(f->left())) {
                    state.add(f->left());
                    to_process.push(f->left());
                }
                if (f->right() && !state.contains(f->right())) {
                    state.add(f->right());
                    to_process.push(f->right());
                }
                break;

            case formula::Formula::OpType::Or:
                // For now, just add the disjunction itself
                // A full implementation would create multiple states
                break;

            default:
                break;
        }
    }
}

bool DFABuilder::is_accepting_state(const StateFormulaSet& state) {
    // In LTLf, a state is accepting if:
    // 1. It does NOT contain any "eventually" formulas that are not yet satisfied
    // 2. It can end the trace (no pending obligations)

    // Simplified: state is accepting if it doesn't contain unsatisfied Until formulas
    for (auto f : state.formulas()) {
        if (f->is_until()) {
            // Until formula without right part satisfied means not accepting
            if (f->right() && !state.contains(f->right())) {
                return false;  // Still waiting for right part
            }
        }
        // If state contains false, it's rejecting
        if (f->is_false()) {
            return false;
        }
    }

    return true;
}

std::vector<StateFormulaSet> DFABuilder::compute_successors(
    const StateFormulaSet& current,
    formula::FormulaPool& pool
) {
    std::vector<StateFormulaSet> successors;

    // Extract all X-formulas and add their bodies to successor
    StateFormulaSet next_state;
    bool has_x_formula = false;

    for (auto f : current.formulas()) {
        if (f->op() == formula::Formula::OpType::Next && f->left()) {
            next_state.add(f->left());
            has_x_formula = true;
        }
    }

    // If no X-formulas, create a self-loop to sink
    if (!has_x_formula) {
        // Empty successor (terminal state)
        successors.push_back(next_state);
        return successors;
    }

    // Expand the next state
    expand_state(next_state, nullptr, pool);

    // For now, just return one successor
    // A full implementation would create multiple successors based on
    // different variable assignments
    successors.push_back(next_state);

    return successors;
}

// Legacy methods (kept for compatibility)

void DFABuilder::extract_primitives(
    formula::Formula* f,
    std::unordered_set<PrimitiveFormula, PrimitiveFormulaHash, PrimitiveFormulaEqual>& primitives
) {
    extract_variables(f, reinterpret_cast<std::unordered_set<int>&>(primitives));
    for (auto p : primitives) {
        // Already added as variable IDs, convert back
    }
}

bool DFABuilder::is_locally_consistent(const StateFormulaSet& set) {
    std::unordered_set<int> positive_literals;
    std::unordered_set<int> negative_literals;

    for (auto f : set.formulas()) {
        if (f->is_literal()) {
            int var_id = f->var_id();
            if (negative_literals.count(var_id)) return false;
            positive_literals.insert(var_id);
        } else if (f->is_not() && f->left() && f->left()->is_literal()) {
            int var_id = f->left()->var_id();
            if (positive_literals.count(var_id)) return false;
            negative_literals.insert(var_id);
        } else if (f->is_false()) {
            return false;
        }
    }
    return true;
}

bool DFABuilder::is_maximal(
    const StateFormulaSet& set,
    const std::unordered_set<PrimitiveFormula, PrimitiveFormulaHash, PrimitiveFormulaEqual>& all_primitives
) {
    for (auto p : all_primitives) {
        if (!set.contains(p)) {
            StateFormulaSet test_set = set;
            test_set.add(p);
            if (is_locally_consistent(test_set)) {
                return false;
            }
        }
    }
    return true;
}

std::vector<StateFormulaSet> DFABuilder::generate_mcs(
    const std::unordered_set<PrimitiveFormula, PrimitiveFormulaHash, PrimitiveFormulaEqual>& primitives
) {
    std::vector<StateFormulaSet> result;
    // Simplified: return empty MCSs
    return result;
}

std::vector<formula::Formula*> DFABuilder::extract_x_formulas(const StateFormulaSet& state) {
    std::vector<formula::Formula*> x_formulas;
    for (auto f : state.formulas()) {
        if (f->is_next()) {
            x_formulas.push_back(f->left());
        }
    }
    return x_formulas;
}

StateFormulaSet DFABuilder::compute_successor(
    const StateFormulaSet& current,
    const std::unordered_set<PrimitiveFormula, PrimitiveFormulaHash, PrimitiveFormulaEqual>& all_primitives
) {
    StateFormulaSet successor;
    for (auto f : current.formulas()) {
        if (f->is_next() && f->left()) {
            successor.add(f->left());
        }
    }
    return successor;
}

bool DFABuilder::satisfies_current(
    const StateFormulaSet& state,
    const std::unordered_set<int>& true_vars,
    const std::unordered_set<int>& false_vars
) {
    FormulaEvaluator eval{true_vars, false_vars};
    for (auto f : state.formulas()) {
        if (!f->is_next() && !eval.is_satisfied(f)) {
            return false;
        }
    }
    return true;
}

bool DFABuilder::is_label_consistent(
    const TransitionLabel& label,
    const StateFormulaSet& state
) {
    // Check that the label doesn't contradict any literal in the state
    for (auto f : state.formulas()) {
        if (f->is_literal()) {
            int var_id = f->var_id();
            if (label.negative_vars.count(var_id)) return false;
        } else if (f->is_not() && f->left() && f->left()->is_literal()) {
            int var_id = f->left()->var_id();
            if (label.positive_vars.count(var_id)) return false;
        }
    }
    return true;
}

} // namespace automata
