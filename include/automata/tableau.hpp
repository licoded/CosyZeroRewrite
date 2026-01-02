/**
 * @file tableau.hpp
 * @brief On-the-fly tableau construction for LTLf synthesis
 *
 * Based on: arXiv:2408.07324 - "On-the-fly Synthesis for LTL over Finite Traces"
 *
 * Each DFA state is represented as a set of subformulas (Tableau state).
 * States are expanded on-demand during game solving, avoiding full DFA construction.
 */

#ifndef AUTOMATA_TABLEAU_HPP
#define AUTOMATA_TABLEAU_HPP

#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include <unordered_set>
#include <vector>
#include <memory>
#include <functional>

namespace automata {

/**
 * @brief Assignment of truth values to variables
 *
 * Represented as a set of variable IDs that are true.
 * Empty set = all variables false.
 */
using Assignment = std::unordered_set<int>;

/**
 * @brief Hash function for Assignment
 */
struct AssignmentHash {
    size_t operator()(const Assignment& a) const {
        size_t h = 0;
        for (int v : a) {
            h ^= std::hash<int>{}(v) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }
};

/**
 * @brief Hash function for Formula pointers
 */
struct FormulaHash {
    size_t operator()(formula::Formula* f) const noexcept {
        return f ? f->hash() : 0;
    }
};

/**
 * @brief Equality function for Formula pointers (structural equality)
 */
struct FormulaEqual {
    bool operator()(formula::Formula* a, formula::Formula* b) const noexcept;
};

/**
 * @brief Tableau state - a set of subformulas
 *
 * Represents a DFA state in the tableau construction.
 * Each state contains a set of subformulas that are "active" at this point.
 */
class TableauState {
public:
    using FormulaSet = std::unordered_set<formula::Formula*, FormulaHash, FormulaEqual>;

    /**
     * @brief Create the initial tableau state from a formula
     * @param phi The LTLf formula (will be converted to NNF)
     * @param pool Formula pool for creating new formulas
     * @return Initial tableau state {phi}
     */
    static std::unique_ptr<TableauState> initial(formula::Formula* phi, formula::FormulaPool& pool);

    /**
     * @brief Check if state is locally consistent (Tableau 1 rules)
     *
     * A state is locally consistent if:
     * - false is NOT in the state
     * - For each (ψ1 ∧ ψ2) in state: both ψ1 and ψ2 are in state
     * - For each (ψ1 ∨ ψ2) in state: at least one of ψ1 or ψ2 is in state
     * - For each (ψ1 U ψ2) in state: ψ2 is in state OR (ψ1 AND (ψ1 U ψ2) are in state)
     * - For each (ψ1 R ψ2) in state: (ψ1 AND ψ2) are in state OR (ψ2 AND (ψ1 R ψ2) are in state)
     *
     * @return true if state is locally consistent
     */
    bool is_locally_consistent() const;

    /**
     * @brief Check if state is accepting (satisfied)
     *
     * A state is accepting if:
     * - false is NOT in the state
     * - For each (ψ1 U ψ2) in state: ψ2 is in state OR ψ1 is in state
     *
     * @return true if state is accepting
     */
    bool is_accepting() const;

    /**
     * @brief Expand to next state given an assignment
     *
     * Computes Γ' = old(Γ) ∪ next(Γ) where:
     * - old(Γ) = formulas that are not Next, Until, or Release
     * - next(Γ) = subformulas under Next + released Until formulas
     *
     * @param assignment Set of variable IDs that are true
     * @param pool Formula pool for creating new formulas
     * @return Next tableau state
     */
    std::unique_ptr<TableauState> next(const Assignment& assignment,
                                        formula::FormulaPool& pool) const;

    /**
     * @brief Get all formulas in this state
     */
    const FormulaSet& formulas() const {
        return formulas_;
    }

    /**
     * @brief Get hash value of this state
     */
    size_t hash() const { return hash_; }

    /**
     * @brief Check equality of two tableau states
     */
    bool operator==(const TableauState& other) const;

    /**
     * @brief String representation for debugging
     */
    std::string to_string() const;

    /**
     * @brief Check if a formula is purely temporal (starts with Next/Until/Release)
     */
    static bool is_temporal(formula::Formula* f);

    // Friend declarations for pool access
    friend class TableauStatePool;
    friend class OnTheFlyDFA;  // Allow access to formulas_ for synthesis

private:
    /**
     * @brief Private constructor - use initial() or next() factory methods
     */
    explicit TableauState(FormulaSet formulas);

    /**
     * @brief Compute hash for the formula set
     */
    static size_t compute_hash(const FormulaSet& formulas);

    /**
     * @brief Get "old" formulas - those without Next, Until, Release at top level
     */
    std::vector<formula::Formula*> get_old_formulas() const;

    /**
     * @brief Get "next" formulas - those under Next or released by Until
     */
    std::vector<formula::Formula*> get_next_formulas() const;

    /**
     * @brief Evaluate literal formulas against assignment
     *
     * Removes literals that are false given the assignment.
     * Keeps literals that are true.
     *
     * @param formulas Current formulas
     * @param assignment Variable assignment
     * @return Filtered formulas
     */
    static FormulaSet evaluate_literals(const FormulaSet& formulas,
                                        const Assignment& assignment);

    /**
     * @brief Check if a literal formula is satisfied in current state
     * @param f Formula to check
     * @return true if formula is satisfied
     */
    bool is_literal_satisfied(formula::Formula* f) const;

    FormulaSet formulas_;
    size_t hash_;
};

/**
 * @brief Hash function for TableauState pointers
 */
struct TableauStateHash {
    size_t operator()(const TableauState* s) const {
        return s ? s->hash() : 0;
    }
};

/**
 * @brief Equality function for TableauState pointers
 */
struct TableauStateEqual {
    bool operator()(const TableauState* a, const TableauState* b) const {
        if (a == b) return true;
        if (!a || !b) return false;
        return *a == *b;
    }
};

/**
 * @brief Pool for managing tableau states with hash consing
 *
 * Ensures that structurally equivalent tableau states are represented
 * by a single object (same pointer).
 */
class TableauStatePool {
public:
    TableauStatePool() = default;
    ~TableauStatePool() = default;

    // No copy/move
    TableauStatePool(const TableauStatePool&) = delete;
    TableauStatePool& operator=(const TableauStatePool&) = delete;

    /**
     * @brief Get or create a tableau state
     *
     * If an equivalent state already exists, return it.
     * Otherwise, create a new one and store it.
     *
     * @param formulas Set of formulas in the state
     * @return Pointer to the (possibly new) tableau state
     */
    TableauState* get_or_create(TableauState::FormulaSet formulas);

    /**
     * @brief Number of unique states in the pool
     */
    size_t size() const { return states_.size(); }

    /**
     * @brief Clear all states (for testing)
     */
    void clear();

private:
    // States with custom hash/equal
    using StateSet = std::unordered_set<TableauState*, TableauStateHash, TableauStateEqual>;

    StateSet states_;

    // Ownership holder
    std::vector<std::unique_ptr<TableauState>> storage_;
};

/**
 * @brief On-the-fly DFA using tableau construction
 *
 * Builds DFA states lazily as needed during game solving.
 * Uses TableauState for state representation and caches transitions.
 */
class OnTheFlyDFA {
public:
    /**
     * @brief Construct on-the-fly DFA from formula
     * @param phi LTLf formula (will be converted to NNF internally)
     * @param pool Formula pool
     */
    OnTheFlyDFA(formula::Formula* phi, formula::FormulaPool& pool);

    /**
     * @brief Get initial DFA state
     */
    TableauState* initial_state() const { return initial_state_; }

    /**
     * @brief Check if a state is accepting
     *
     * For synthesis, a non-temporal state is accepting only if
     * the system can satisfy all formulas using only output variables.
     */
    bool is_accepting(TableauState* q) const;

    /**
     * @brief Check if a state is accepting (pure tableau, ignoring I/O)
     */
    bool is_tableau_accepting(TableauState* q) const {
        return q->is_accepting();
    }

    /**
     * @brief Get or compute successor state
     *
     * @param q Current state
     * @param assignment Variable assignment
     * @return Next state (cached after first computation)
     */
    TableauState* successor(TableauState* q, const Assignment& assignment) const;

    /**
     * @brief Get all states that have been expanded so far
     */
    const std::unordered_set<TableauState*, TableauStateHash, TableauStateEqual>&
    expanded_states() const { return expanded_states_; }

    /**
     * @brief Get number of expanded states
     */
    size_t num_expanded() const { return expanded_states_.size(); }

    /**
     * @brief Get transition cache size
     */
    size_t cache_size() const { return transition_cache_.size(); }

private:
    formula::FormulaPool& pool_;
    mutable TableauStatePool state_pool_;
    TableauState* initial_state_;
    int num_outputs_;  // Number of output variables (for synthesis acceptance check)

    // Transition cache: (state, assignment) -> next_state
    using CacheKey = std::pair<TableauState*, Assignment>;
    struct CacheKeyHash {
        size_t operator()(const CacheKey& k) const {
            size_t h = TableauStateHash{}(k.first);
            h ^= AssignmentHash{}(k.second) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };
    struct CacheKeyEqual {
        bool operator()(const CacheKey& a, const CacheKey& b) const {
            return TableauStateEqual{}(a.first, b.first) && a.second == b.second;
        }
    };

    mutable std::unordered_map<CacheKey, TableauState*, CacheKeyHash, CacheKeyEqual> transition_cache_;

    // Set of states that have been expanded (successors computed)
    mutable std::unordered_set<TableauState*, TableauStateHash, TableauStateEqual> expanded_states_;

    /**
     * @brief Check if formula requires an input variable to be true
     * @param f Formula to check
     * @param num_outputs Number of output variables
     * @return true if formula requires some input variable to be true
     */
    bool requires_input_true(formula::Formula* f, int num_outputs) const;
};

/**
 * @brief Generate all possible assignments for a set of variables
 */
class AssignmentGenerator {
public:
    /**
     * @brief Create assignment generator
     * @param num_variables Number of variables (generates 2^n assignments)
     */
    explicit AssignmentGenerator(int num_variables);

    /**
     * @brief Generate all possible assignments
     * @return Vector of all 2^n assignments
     */
    std::vector<Assignment> all_assignments() const;

    /**
     * @brief Get assignment as bitset
     * @param i Assignment index (0 to 2^n - 1)
     * @return Assignment with true values for set bits
     */
    static Assignment from_bitset(int i, int num_variables);

    /**
     * @brief Convert assignment to string for debugging
     */
    static std::string to_string(const Assignment& a, int num_variables);

    /**
     * @brief Number of variables
     */
    int num_variables() const { return num_variables_; }

private:
    int num_variables_;
};

} // namespace automata

#endif // AUTOMATA_TABLEAU_HPP
