#ifndef FORMULA_HPP
#define FORMULA_HPP

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace formula {

// Forward declaration
class FormulaPool;

/**
 * @brief Immutable LTLf formula representation
 *
 * Formula class represents an LTLf (Linear Temporal Logic on Finite Traces) formula
 * as an immutable AST node. All formulas are created through FormulaPool for
 * hash consing (canonicalization).
 *
 * Design decisions:
 * - Immutable: All fields are effectively const (no setters)
 * - Raw pointers: Managed by FormulaPool for performance
 * - Cached hash: Computed once for efficient comparisons
 * - Private constructor: Only FormulaPool can create formulas
 */
class Formula {
public:
    /**
     * @brief Operator types for LTLf formulas
     *
     * Note: Next represents Strong Next (X[!]), which requires a next state.
     * The End marker handles finite trace semantics.
     */
    enum class OpType {
        True,       // True constant
        False,      // False constant
        Not,        // Negation
        And,        // Conjunction
        Or,         // Disjunction
        Next,       // Strong Next (X[!])
        Until,      // Until (U)
        Release,    // Release (R)
        End,        // End marker for finite traces
        Literal     // Variable reference
    };

    // ========== Immutable Accessors ==========
    // Note: Formula is immutable via Hash Consing, but accessors return non-const
    // pointers for internal use. A future refactor should add const versions.

    /** @brief Get the operator type */
    OpType op() const { return op_; }

    /** @brief Get left child (for binary/unary operators) */
    Formula* left() const { return left_; }

    /** @brief Get right child (for binary operators) */
    Formula* right() const { return right_; }

    /** @brief Get variable ID (only for Literal) */
    int var_id() const { return var_id_; }

    /** @brief Get cached hash value */
    size_t hash() const { return hash_; }

    /** @brief Get pool index for sorting */
    size_t pool_index() const { return pool_index_; }

    // ========== Type Predicates ==========

    bool is_true() const { return op_ == OpType::True; }
    bool is_false() const { return op_ == OpType::False; }
    bool is_literal() const { return op_ == OpType::Literal; }
    bool is_not() const { return op_ == OpType::Not; }
    bool is_and() const { return op_ == OpType::And; }
    bool is_or() const { return op_ == OpType::Or; }
    bool is_next() const { return op_ == OpType::Next; }
    bool is_until() const { return op_ == OpType::Until; }
    bool is_release() const { return op_ == OpType::Release; }
    bool is_end() const { return op_ == OpType::End; }

    // ========== Structure Predicates ==========

    /** @brief Check if this is a binary operator (And, Or, Until, Release) */
    bool is_binary() const {
        return op_ == OpType::And || op_ == OpType::Or ||
               op_ == OpType::Until || op_ == OpType::Release;
    }

    /** @brief Check if this is a unary operator (Not, Next) */
    bool is_unary() const {
        return op_ == OpType::Not || op_ == OpType::Next;
    }

    /** @brief Check if this is a temporal operator (Next, Until, Release) */
    bool is_temporal() const {
        return op_ == OpType::Next || op_ == OpType::Until ||
               op_ == OpType::Release;
    }

    /** @brief Check if this is a constant (True, False, End) */
    bool is_constant() const {
        return op_ == OpType::True || op_ == OpType::False ||
               op_ == OpType::End;
    }

    // ========== Operations (return NEW Formula) ==========

    /**
     * @brief Convert formula to Negation Normal Form (NNF)
     * @param pool The FormulaPool to create new formulas
     * @return New formula in NNF
     *
     * NNF: All negations appear only directly in front of literals.
     * Time complexity: O(n) where n is formula size.
     */
    Formula* nnf(FormulaPool& pool) const;

    /**
     * @brief Simplify formula using algebraic rules
     * @param pool The FormulaPool to create new formulas
     * @return New simplified formula
     *
     * Applies rules like:
     * - a & True -> a
     * - a | False -> a
     * - a & a -> a
     * - etc.
     *
     * Time complexity: O(n) using HashSet for deduplication.
     */
    Formula* simplify(FormulaPool& pool) const;

    /**
     * @brief Convert to neXt Normal Form (XNF)
     * @param pool The FormulaPool to create new formulas
     * @return New formula in XNF
     *
     * XNF: All primitive subformulas contain only literals and Next operators.
     * Until/Release are expanded to the top level.
     *
     * Time complexity: O(n) - single pass expansion.
     */
    Formula* xnf_with_end_marker(FormulaPool& pool) const;

    /**
     * @brief Replace all Strong Next (X[!]) subformulas with True
     * @param pool The FormulaPool to create new formulas
     * @return New formula with X[!] replaced by True
     *
     * This is a simplification operation for XNF formulas.
     * Input MUST be in XNF format (no Until/Release in primitives).
     *
     * Rules:
     * - X[!] φ → True
     * - Other operators are recursively processed
     *
     * Time complexity: O(n) where n is formula size.
     */
    Formula* replaceNext2True(FormulaPool& pool) const;

    // ========== Utilities ==========

    /**
     * @brief Collect all variable IDs from a formula
     * @param f The formula to collect variables from
     * @return Set of variable IDs appearing in the formula
     *
     * This is a utility method for extracting all variables used in a formula.
     * Useful when partition information is not available.
     *
     * Time complexity: O(n) where n is formula size.
     */
    static std::unordered_set<int> collect_variables(Formula* f);

    // ========== String Representation ==========

    /**
     * @brief Convert formula to string representation with original variable names
     * @param pool The FormulaPool for variable name lookup
     * @return String representation with original variable names (e.g., "p1", "p2")
     */
    std::string to_string(const FormulaPool& pool) const;

    /**
     * @brief Get operator name as string
     * @return String representation of the operator
     */
    const char* op_name() const;

    // ========== Comparison (for sorting in simplify) ==========
    /**
     * @brief Less-than operator using pool_index_
     *
     * Provides strict weak ordering for sorting formulas.
     * Uses pool_index_ which is guaranteed unique.
     *
     * See HASH_CONSING_ANALYSIS.md section 11.2.5 for details.
     */
    bool operator<(const Formula& other) const {
        return pool_index_ < other.pool_index_;
    }

    /**
     * @brief Clear the simplify cache
     *
     * Resets the cached simplified formula. This is useful when
     * the pool is being reused or when formulas need to be re-simplified.
     */
    void clear_simplify_cache() const { simp_ = nullptr; }

private:
    /**
     * @brief Type for resolving variable names to strings
     *
     * Used by to_string to convert var_id to variable name.
     */
    using VarNameResolver = std::function<std::string(int)>;

    /**
     * @brief Internal implementation for to_string
     *
     * @param resolver Function to convert var_id to string (e.g., "p1", "p2")
     * @param end_marker String to use for End marker (e.g., "End")
     * @return String representation of the formula
     */
    std::string to_string_impl(const VarNameResolver& resolver,
                               const char* end_marker) const;
    /**
     * @brief Private constructor (only FormulaPool can create)
     *
     * @param op Operator type
     * @param left Left child (or operand for unary)
     * @param right Right child (null for unary/constant/literal)
     * @param var_id Variable ID (only when op == Literal)
     * @param hash Cached hash value
     * @param pool_index Index in FormulaPool (for sorting)
     */
    Formula(OpType op, Formula* left, Formula* right,
            int var_id, size_t hash, size_t pool_index);

    // ========== Immutable Fields ==========
    OpType op_;           // Operator type
    Formula* left_;       // Left child (or operand for unary)
    Formula* right_;      // Right child (null for unary/constant)
    int var_id_;          // Variable ID (only for Literal)
    size_t hash_;         // Cached hash value
    size_t pool_index_;   // Index in FormulaPool (for sorting)

    // ========== Simplify Cache ==========
    /**
     * @brief Cached simplified formula
     *
     * Mutable to allow modification in const member functions.
     * This matches the design of the original aalta implementation.
     *
     * The cache is per-formula, not per-pool, which means:
     * - Fast access (no map lookup overhead)
     * - Works correctly even if formula is used with multiple pools
     * - Automatically invalidated when formula is destroyed
     */
    mutable Formula* simp_ = nullptr;  // Simplify cache (like aalta's _simp)

    // FormulaPool needs access to private constructor and members
    friend class FormulaPool;
};

} // namespace formula

#endif // FORMULA_HPP
