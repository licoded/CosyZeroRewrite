#ifndef FORMULA_POOL_HPP
#define FORMULA_POOL_HPP

#include "formula/formula.hpp"
#include "formula/formula_exception.hpp"
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace formula {

/**
 * @brief Manages formula creation, canonicalization, and variable declarations
 *
 * FormulaPool is the owner of all Formula objects in a per-DFA scope.
 * It provides:
 * - Hash consing: Automatic deduplication of structurally identical formulas
 * - Variable management: Declaration and lookup of atomic propositions
 * - Memory management: RAII cleanup of all formulas
 *
 * Key design decisions:
 * - Non-copyable: Prevents accidental sharing between DFAs
 * - Canonicalization: unique_table_ ensures structural uniqueness
 * - Pre-allocated variables: BDD-ready with fixed ordering
 */
class FormulaPool {
public:
    // ========== Construction ==========

    FormulaPool();
    ~FormulaPool();

    // Non-copyable
    FormulaPool(const FormulaPool&) = delete;
    FormulaPool& operator=(const FormulaPool&) = delete;

    // Movable (for std::vector)
    FormulaPool(FormulaPool&& other) noexcept;
    FormulaPool& operator=(FormulaPool&& other) noexcept;

    // ========== Variable Management ==========

    /**
     * @brief Load variable declarations from a partition file
     * @param partition_file Path to .part file
     *
     * Partition file format:
     *   .outputs: s1 s2 s3
     *   .inputs: p1 p2 p3
     *
     * Throws std::runtime_error if file cannot be read.
     */
    void load_from_partition(const std::string& partition_file);

    /**
     * @brief Declare all variables at once
     * @param outputs Output variable names (state variables)
     * @param inputs Input variable names (input variables)
     *
     * Variable IDs are assigned: outputs first (0..m-1), then inputs (m..m+n-1).
     *
     * Throws std::invalid_argument if:
     * - Variables already declared
     * - Duplicate names in outputs or inputs
     * - Overlap between outputs and inputs
     */
    void declare_variables(const std::vector<std::string>& outputs,
                          const std::vector<std::string>& inputs);

    /**
     * @brief Declare output variables
     * @param outputs Output variable names
     */
    void declare_outputs(const std::vector<std::string>& outputs);

    /**
     * @brief Declare input variables
     * @param inputs Input variable names
     */
    void declare_inputs(const std::vector<std::string>& inputs);

    /**
     * @brief Auto-extract variables from a formula
     * @param root Root formula to extract from
     *
     * Traverses the formula tree and declares all literals as outputs.
     * Used for testing without partition files.
     *
     * Postconditions:
     * - All literals are declared as outputs
     * - No inputs declared
     * - is_fully_declared() == true
     */
    void extract_variables_from_formula(Formula* root);

    /**
     * @brief Check if all variables are declared
     * @return true if both outputs and inputs are declared
     */
    bool is_fully_declared() const {
        return outputs_declared_ && inputs_declared_;
    }

    /**
     * @brief Get total number of variables
     * @return Number of declared variables
     */
    int num_variables() const { return static_cast<int>(var_names_.size()); }

    /**
     * @brief Get number of output variables
     * @return Number of output variables
     */
    int num_outputs() const { return num_outputs_; }

    /**
     * @brief Get number of input variables
     * @return Number of input variables
     */
    int num_inputs() const { return num_inputs_; }

    // ========== Variable Info ==========

    /**
     * @brief Get variable ID by name
     * @param name Variable name
     * @return Variable ID
     * @throws std::runtime_error if variable not declared
     */
    int get_variable_id(const std::string& name) const;

    /**
     * @brief Get variable name by ID
     * @param var_id Variable ID
     * @return Variable name
     * @throws std::runtime_error if var_id is invalid
     */
    std::string get_variable_name(int var_id) const;

    /**
     * @brief Check if variable is an output
     * @param var_id Variable ID
     * @return true if variable is an output
     */
    bool is_output_variable(int var_id) const {
        return var_id >= 0 && var_id < num_outputs_;
    }

    /**
     * @brief Check if variable is an input
     * @param var_id Variable ID
     * @return true if variable is an input
     */
    bool is_input_variable(int var_id) const {
        return var_id >= num_outputs_ && var_id < num_variables();
    }

    /**
     * @brief Check if variable exists
     * @param name Variable name
     * @return true if variable is declared
     */
    bool has_variable(const std::string& name) const;

    /**
     * @brief Get all declared variable names in order
     * @return Vector of variable names (outputs first, then inputs)
     *
     * This is used by FormulaParser to initialize its lexer with
     * multi-character variable names.
     */
    const std::vector<std::string>& get_all_variable_names() const {
        return var_names_;
    }

    /**
     * @brief Get or auto-declare a variable (for parser convenience)
     * @param name Variable name
     * @return Variable ID (auto-declared as output if not exists)
     *
     * This method is primarily for the parser to auto-declare variables.
     * Variables are auto-declared as outputs.
     */
    int get_or_create_variable(const std::string& name);

    // ========== Formula Creation (Canonicalized) ==========

    /**
     * @brief Create a formula with canonicalization
     * @param op Operator type
     * @param left Left child
     * @param right Right child (can be nullptr for unary/constant)
     * @param var_id Variable ID (only for Literal)
     * @return Canonicalized formula pointer
     *
     * If a structurally identical formula exists, returns the existing one.
     * Otherwise creates a new formula and adds it to the unique table.
     */
    Formula* create(Formula::OpType op, Formula* left,
                   Formula* right = nullptr, int var_id = -1);

    /**
     * @brief Create a literal formula (variable reference)
     * @param name Variable name
     * @return Formula* with OpType::Literal
     * @throws std::runtime_error if variable not declared
     */
    Formula* create_variable(const std::string& name);

    /** @brief Create True constant */
    Formula* create_true();

    /** @brief Create False constant */
    Formula* create_false();

    /** @brief Create Not formula */
    Formula* create_not(Formula* operand);

    /** @brief Create And formula */
    Formula* create_and(Formula* left, Formula* right);

    /** @brief Create Or formula */
    Formula* create_or(Formula* left, Formula* right);

    /** @brief Create Next formula */
    Formula* create_next(Formula* operand);

    /** @brief Create Until formula */
    Formula* create_until(Formula* left, Formula* right);

    /** @brief Create Release formula */
    Formula* create_release(Formula* left, Formula* right);

    /**
     * @brief Create or get End marker (singleton)
     * @return End marker formula
     *
     * The End marker is a special atomic proposition representing
     * "this is the last position in the finite trace".
     */
    Formula* create_end();

    // ========== Resource Management ==========

    /**
     * @brief Clear all formulas and reset state
     *
     * Frees all formulas and resets the unique table.
     * Variable declarations are preserved.
     */
    void clear();

    /**
     * @brief Get total number of formulas created
     * @return Total count of formulas
     */
    size_t size() const { return formulas_.size(); }

    // ========== Statistics ==========

    /**
     * @brief Get number of unique formulas (after deduplication)
     * @return Size of unique table
     */
    size_t unique_count() const { return unique_table_.size(); }

    /**
     * @brief Get total number of formulas created
     * @return Total count
     */
    size_t total_count() const { return formulas_.size(); }

    /**
     * @brief Get deduplication ratio
     * @return unique / total
     *
     * Values closer to 1.0 indicate good deduplication.
     * Lower values indicate more structural sharing.
     */
    double deduplication_ratio() const {
        return total_count() > 0
            ? static_cast<double>(unique_count()) / total_count()
            : 0.0;
    }

private:
    // ========== Variable Name Validation ==========

    /**
     * @brief Maximum length for variable names
     */
    static constexpr size_t MAX_VAR_NAME_LENGTH = 64;

    /**
     * @brief Reserved keywords that cannot be used as variable names
     *
     * Includes logical operators and special constants.
     */
    static const std::unordered_set<std::string>& reserved_keywords() {
        static const std::unordered_set<std::string> keywords = {
            // Constants
            "true", "false", "TRUE", "FALSE",
            // Operators
            "not", "and", "or", "next", "until", "release",
            "NOT", "AND", "OR", "NEXT", "UNTIL", "RELEASE",
            "X", "U", "R", "G", "F", "W",  // Single-letter operators
            // Special
            "end", "End", "END", "last", "Last", "LAST",
            "tail", "Tail", "TAIL"
        };
        return keywords;
    }

    /**
     * @brief Validate a variable name
     * @param name Variable name to validate
     * @throws InvalidVariableNameException if name is invalid
     *
     * Naming rules:
     * - Must match: [a-zA-Z_][a-zA-Z0-9_]*
     * - Maximum length: 64 characters
     * - Cannot be a reserved keyword
     * - Cannot be empty
     */
    static void validate_variable_name(const std::string& name);

    /**
     * @brief Check if a string is a valid variable name
     * @param name String to check
     * @return true if valid, false otherwise
     */
    static bool is_valid_variable_name(const std::string& name);

    // ========== Canonicalization ==========

    /**
     * @brief Hash function for formulas
     */
    struct FormulaHash {
        size_t operator()(Formula* f) const noexcept {
            return f ? f->hash() : 0;
        }
    };

    /**
     * @brief Equality function for hash consing
     *
     * See HASH_CONSING_ANALYSIS.md section 11.2 for details.
     * - Fast path: pointer comparison
     * - Core logic: structural comparison (hash + op + children + var_id)
     */
    struct FormulaEqual {
        bool operator()(Formula* a, Formula* b) const noexcept;
    };

    using UniqueTable = std::unordered_set<Formula*, FormulaHash, FormulaEqual>;

    /**
     * @brief Compute hash for a formula specification
     * @param op Operator type
     * @param left Left child
     * @param right Right child
     * @param var_id Variable ID
     * @return Hash value
     */
    static size_t compute_hash(Formula::OpType op, Formula* left,
                               Formula* right, int var_id);

    /**
     * @brief Validate formula arguments
     * @throws std::invalid_argument if arguments are invalid
     */
    static void validate_create(Formula::OpType op, Formula* left,
                               Formula* right, int var_id);

    // ========== Member Variables ==========

    // Canonicalization cache
    UniqueTable unique_table_;

    // Formula ownership (all formulas owned by pool)
    std::vector<std::unique_ptr<Formula>> formulas_;

    // Variable management
    std::vector<std::string> var_names_;     // var_names_[id] = name
    std::unordered_map<std::string, int> var_ids_;  // var_ids_[name] = id
    int num_outputs_;
    int num_inputs_;
    bool outputs_declared_;
    bool inputs_declared_;

    // Singleton markers
    Formula* true_marker_;
    Formula* false_marker_;
    Formula* end_marker_;

    // Pool state
    bool moved_from_;  // True if this pool was moved from
};

} // namespace formula

#endif // FORMULA_POOL_HPP
