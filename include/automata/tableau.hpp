/**
 * @file tableau.hpp
 * @brief On-the-fly tableau construction for LTLf synthesis
 *
 * Based on: arXiv:2408.07324 - "On-the-fly Synthesis for LTL over Finite Traces"
 * Formula Progression: AAAI2019 - De Giacomo et al.
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
 * @brief Tableau state - represents a DFA state in LTLf tableau construction
 *
 * Based on AAAI2019 Definition 2 (Propositional Atoms):
 * Each state contains:
 * - phi: The original formula
 * - xnf_phi: The formula in XNF form (for progression)
 * - prop_atoms_: PA(xnf_phi) - propositional atoms, expanded at And/Or,
 *                stopping at Next/Until/Release formulas
 *
 * Example: phi = a U b, xnf_phi = (a & X(a U b)) | b
 *          prop_atoms_ = {a, X(a U b), b}
 */
class TableauState {
public:
    using FormulaSet = std::unordered_set<formula::Formula*, FormulaHash, FormulaEqual>;

    /**
     * @brief Create the initial tableau state from a formula
     * @param phi The LTLf formula (will be converted to NNF, then XNF)
     * @param pool Formula pool for creating new formulas
     * @return Initial tableau state
     */
    static std::unique_ptr<TableauState> initial(formula::Formula* phi, formula::FormulaPool& pool);

    /**
     * @brief Expand to next state given an assignment
     *
     * Uses formula progression fp(xnf_phi, assignment) to compute next state.
     * Based on AAAI2019 Li et al. - Formula Progression for LTLf.
     *
     * @param assignment Set of variable IDs that are true
     * @param pool Formula pool for creating new formulas
     * @return Next tableau state
     */
    std::unique_ptr<TableauState> next(const Assignment& assignment,
                                        formula::FormulaPool& pool) const;

    /**
     * @brief Get the original formula phi
     */
    formula::Formula* phi() const { return phi_; }

    /**
     * @brief Get the XNF formula
     */
    formula::Formula* xnf_phi() const { return xnf_phi_; }

    /**
     * @brief Get propositional atoms PA(xnf_phi)
     *
     * Per AAAI2019 Definition 2: PA(φ) expands at And/Or,
     * stops at Next/Until/Release formulas.
     */
    const FormulaSet& prop_atoms() const { return prop_atoms_; }

    /**
     * @brief Get all formulas in this state (backward compatibility alias for prop_atoms)
     */
    const FormulaSet& formulas() const { return prop_atoms_; }

    /**
     * @brief Get hash value of this state (based on phi only)
     */
    size_t hash() const { return hash_; }

    /**
     * @brief Check equality of two tableau states (based on phi only)
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
    friend class OnTheFlyDFA;

private:
    /**
     * @brief Private constructor - use initial() or next() factory methods
     * @param phi Original formula
     * @param xnf_phi XNF form of phi
     * @param prop_atoms PA(xnf_phi) - propositional atoms
     */
    TableauState(formula::Formula* phi, formula::Formula* xnf_phi, FormulaSet prop_atoms);

    /**
     * @brief Compute PA(φ) - Propositional Atoms
     *
     * Based on AAAI2019 Definition 2:
     * - PA(φ) = {φ} if φ is atom, Next, Until, or Release
     * - PA(¬ψ) = PA(ψ)
     * - PA(φ₁ ∧ φ₂) = PA(φ₁) ∪ PA(φ₂)
     * - PA(φ₁ ∨ φ₂) = PA(φ₁) ∪ PA(φ₂)
     *
     * @param phi Formula to compute PA for
     * @param result Output set for accumulated atoms
     */
    static void compute_prop_atoms(formula::Formula* phi, FormulaSet& result);

    formula::Formula* phi_;        // Original formula (used for hash and empty-string check)
    formula::Formula* xnf_phi_;    // XNF form (used for formula progression)
    FormulaSet prop_atoms_;         // PA(xnf_phi) - propositional atoms
    size_t hash_;                   // Hash based on phi only
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
 * States are uniquely identified by their phi formula.
 */
class TableauStatePool {
public:
    TableauStatePool() = default;
    ~TableauStatePool() = default;

    // No copy/move
    TableauStatePool(const TableauStatePool&) = delete;
    TableauStatePool& operator=(const TableauStatePool&) = delete;

    /**
     * @brief Get or create a tableau state from phi
     *
     * If a state with equivalent phi already exists, return it.
     * Otherwise, create a new one and store it.
     *
     * @param phi Original formula (used as unique identifier)
     * @param pool Formula pool for computing XNF and prop atoms
     * @return Pointer to the (possibly new) tableau state
     */
    TableauState* get_or_create(formula::Formula* phi, formula::FormulaPool& pool);

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
