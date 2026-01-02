#ifndef AUTOMATA_DFA_HPP
#define AUTOMATA_DFA_HPP

#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <string>
#include <optional>

namespace automata {

/**
 * @brief Primitive formula (formula that can appear in a DFA state)
 *
 * In LTLf-to-DFA construction via tableau method, a state is a set of
 * "primitive" formulas - formulas that are in neXt Normal Form (XNF).
 *
 * A formula is primitive if it contains no temporal operators except X.
 */
using PrimitiveFormula = formula::Formula*;

/**
 * @brief Hash for PrimitiveFormula (uses Formula's cached hash)
 */
struct PrimitiveFormulaHash {
    size_t operator()(PrimitiveFormula f) const noexcept {
        return f->hash();
    }
};

/**
 * @brief Equality for PrimitiveFormula (uses pointer equality via hash consing)
 */
struct PrimitiveFormulaEqual {
    bool operator()(PrimitiveFormula a, PrimitiveFormula b) const noexcept {
        return a == b;  // Hash consing ensures pointer equality
    }
};

/**
 * @brief A set of primitive formulas (Maximal Consistent Set)
 *
 * A DFA state in the LTLf tableau construction is a set of formulas
 * that are mutually consistent and maximal with respect to this property.
 */
class StateFormulaSet {
private:
    std::unordered_set<PrimitiveFormula, PrimitiveFormulaHash, PrimitiveFormulaEqual> formulas_;

public:
    StateFormulaSet() = default;

    // Add a formula to the set
    void add(PrimitiveFormula f) {
        formulas_.insert(f);
    }

    // Check if a formula is in the set
    bool contains(PrimitiveFormula f) const {
        return formulas_.find(f) != formulas_.end();
    }

    // Get all formulas
    const std::unordered_set<PrimitiveFormula, PrimitiveFormulaHash, PrimitiveFormulaEqual>&
    formulas() const { return formulas_; }

    // Size of the set
    size_t size() const { return formulas_.size(); }

    // Empty check
    bool empty() const { return formulas_.empty(); }

    // Equality check (for state identification)
    bool equals(const StateFormulaSet& other) const {
        if (size() != other.size()) return false;
        for (auto f : formulas_) {
            if (!other.contains(f)) return false;
        }
        return true;
    }

    // Hash for use in hash tables
    size_t hash() const {
        size_t h = 0;
        for (auto f : formulas_) {
            h ^= f->hash() + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }

    // Convert to string (for debugging)
    std::string to_string(const formula::FormulaPool& pool) const;
};

/**
 * @brief Hash for StateFormulaSet
 */
struct StateFormulaSetHash {
    size_t operator()(const StateFormulaSet& s) const noexcept {
        return s.hash();
    }
};

/**
 * @brief Equality for StateFormulaSet
 */
struct StateFormulaSetEqual {
    bool operator()(const StateFormulaSet& a, const StateFormulaSet& b) const noexcept {
        return a.equals(b);
    }
};

/**
 * @brief A state ID in the DFA
 */
using StateId = size_t;
constexpr StateId INVALID_STATE_ID = SIZE_MAX;

/**
 * @brief A transition in the DFA
 *
 * A transition is labeled by an assignment to propositional variables.
 * For efficiency, we represent this as two sets:
 * - positive_vars: variables assigned to true
 * - negative_vars: variables assigned to false
 */
struct TransitionLabel {
    std::unordered_set<int> positive_vars;
    std::unordered_set<int> negative_vars;

    TransitionLabel() = default;

    void add_positive(int var_id) { positive_vars.insert(var_id); }
    void add_negative(int var_id) { negative_vars.insert(var_id); }

    bool operator==(const TransitionLabel& other) const {
        return positive_vars == other.positive_vars &&
               negative_vars == other.negative_vars;
    }

    size_t hash() const {
        size_t h = 0;
        for (int v : positive_vars) {
            h ^= std::hash<int>{}(v) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        for (int v : negative_vars) {
            h ^= std::hash<int>{}(-v) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }

    std::string to_string() const;
};

/**
 * @brief A single transition from one state to another
 */
struct Transition {
    StateId from;
    StateId to;
    TransitionLabel label;

    Transition(StateId f, StateId t, const TransitionLabel& l)
        : from(f), to(t), label(l) {}
};

/**
 * @brief Deterministic Finite Automaton for LTLf formulas
 *
 * This DFA represents the language of an LTLf formula.
 * It is constructed using the tableau method.
 */
class DFA {
public:
    /**
     * @brief Construct an empty DFA
     */
    DFA() : next_state_id_(0) {}

    /**
     * @brief Add a new state to the DFA
     * @return The ID of the new state
     */
    StateId add_state(const StateFormulaSet& formulas);

    /**
     * @brief Get the formula set for a state
     */
    const StateFormulaSet& get_state_formulas(StateId id) const;

    /**
     * @brief Mark a state as accepting (final)
     */
    void set_accepting(StateId id, bool accepting = true);

    /**
     * @brief Check if a state is accepting
     */
    bool is_accepting(StateId id) const;

    /**
     * @brief Add a transition between states
     */
    void add_transition(StateId from, StateId to, const TransitionLabel& label);

    /**
     * @brief Get all outgoing transitions from a state
     */
    const std::vector<Transition>& get_transitions(StateId from) const;

    /**
     * @brief Get number of states
     */
    size_t num_states() const { return states_.size(); }

    /**
     * @brief Get all state IDs
     */
    std::vector<StateId> get_all_states() const;

    /**
     * @brief Get initial state
     */
    StateId get_initial_state() const { return initial_state_; }

    /**
     * @brief Set initial state
     */
    void set_initial_state(StateId id) { initial_state_ = id; }

    /**
     * @brief Clone this DFA
     */
    std::unique_ptr<DFA> clone() const;

    /**
     * @brief Debug print
     */
    void print(const formula::FormulaPool& pool) const;

    /**
     * @brief Export to Graphviz DOT format
     */
    std::string to_dot(const formula::FormulaPool& pool) const;

private:
    struct StateInfo {
        StateFormulaSet formulas;
        std::vector<Transition> transitions;
        bool accepting = false;

        StateInfo() = default;
        explicit StateInfo(const StateFormulaSet& f) : formulas(f) {}
    };

    std::vector<StateInfo> states_;
    std::unordered_map<size_t, StateId> state_lookup_;  // hash -> id
    StateId initial_state_ = INVALID_STATE_ID;
    StateId next_state_id_ = 0;
};

/**
 * @brief Builder for constructing a DFA from an LTLf formula
 *
 * Uses the tableau construction method:
 * 1. Convert formula to XNF
 * 2. Generate Maximal Consistent Sets (MCS) as states
 * 3. Build transitions based on X-formulas
 */
class DFABuilder {
public:
    /**
     * @brief Construct a DFA from an LTLf formula
     * @param formula The input LTLf formula
     * @param pool The formula pool (for creating new formulas)
     * @return The constructed DFA
     */
    static std::unique_ptr<DFA> build_from_formula(
        formula::Formula* formula,
        formula::FormulaPool& pool
    );

private:
    /**
     * @brief Extract all primitive subformulas from a formula
     *
     * A formula is primitive if it contains no temporal operators except X.
     */
    static void extract_primitives(
        formula::Formula* f,
        std::unordered_set<PrimitiveFormula, PrimitiveFormulaHash, PrimitiveFormulaEqual>& primitives
    );

    /**
     * @brief Check if a set of formulas is locally consistent
     *
     * A set is locally consistent if it doesn't contain both p and !p for any literal.
     */
    static bool is_locally_consistent(const StateFormulaSet& set);

    /**
     * @brief Check if a set is maximal (cannot add more primitive formulas)
     */
    static bool is_maximal(
        const StateFormulaSet& set,
        const std::unordered_set<PrimitiveFormula, PrimitiveFormulaHash, PrimitiveFormulaEqual>& all_primitives
    );

    /**
     * @brief Generate all MCS (Maximal Consistent Sets) from primitive formulas
     */
    static std::vector<StateFormulaSet> generate_mcs(
        const std::unordered_set<PrimitiveFormula, PrimitiveFormulaHash, PrimitiveFormulaEqual>& primitives
    );

    /**
     * @brief Extract all X-formulas from a state
     *
     * These determine the successors of the state.
     */
    static std::vector<formula::Formula*> extract_x_formulas(const StateFormulaSet& state);

    /**
     * @brief Compute the successor state by evaluating X-formulas
     *
     * For each X(φ) in the current state, we need φ to hold in the successor.
     */
    static StateFormulaSet compute_successor(
        const StateFormulaSet& current,
        const std::unordered_set<PrimitiveFormula, PrimitiveFormulaHash, PrimitiveFormulaEqual>& all_primitives
    );

    /**
     * @brief Check if a state satisfies "current" formula (non-X part of XNF)
     */
    static bool satisfies_current(
        const StateFormulaSet& state,
        const std::unordered_set<int>& true_vars,
        const std::unordered_set<int>& false_vars
    );

    /**
     * @brief Check if a transition label is consistent with a state
     */
    static bool is_label_consistent(
        const TransitionLabel& label,
        const StateFormulaSet& state
    );

    /**
     * @brief Extract all variable IDs from a formula
     */
    static void extract_variables(
        formula::Formula* f,
        std::unordered_set<int>& vars
    );

    /**
     * @brief Expand a state with logical consequences
     */
    static void expand_state(
        StateFormulaSet& state,
        formula::Formula* formula,
        formula::FormulaPool& pool
    );

    /**
     * @brief Check if a state is accepting (can end the trace)
     */
    static bool is_accepting_state(const StateFormulaSet& state);

    /**
     * @brief Compute all successor states
     */
    static std::vector<StateFormulaSet> compute_successors(
        const StateFormulaSet& current,
        formula::FormulaPool& pool
    );
};

} // namespace automata

#endif // AUTOMATA_DFA_HPP
