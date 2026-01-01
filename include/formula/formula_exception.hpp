#ifndef FORMULA_EXCEPTION_HPP
#define FORMULA_EXCEPTION_HPP

#include <stdexcept>
#include <string>

namespace formula {

/**
 * @brief Base exception class for all formula-related errors
 *
 * All formula library exceptions inherit from this class,
 * allowing users to catch all formula errors with a single catch block.
 *
 * Example:
 * @code
 * try {
 *     Formula* f = pool.create_variable("x");
 * } catch (const FormulaException& e) {
 *     std::cerr << "Formula error: " << e.what() << std::endl;
 * }
 * @endcode
 */
class FormulaException : public std::runtime_error {
public:
    /**
     * @brief Construct a formula exception with a message
     * @param msg Error message describing the exception
     */
    explicit FormulaException(const std::string& msg)
        : std::runtime_error(msg) {}

    /**
     * @brief Virtual destructor for proper inheritance
     */
    ~FormulaException() noexcept override = default;
};

/**
 * @brief Exception thrown when parsing fails
 *
 * This can be caused by:
 * - Syntax errors in the formula string
 * - Invalid token sequences
 * - Unexpected end of input
 * - Unrecognized operators
 */
class ParseException : public FormulaException {
public:
    /**
     * @brief Construct a parse exception
     * @param msg Error message describing the parse error
     */
    explicit ParseException(const std::string& msg)
        : FormulaException("Parse error: " + msg) {}
};

/**
 * @brief Exception thrown when validation fails
 *
 * This can be caused by:
 * - Invalid variable names
 * - Undeclared variables
 * - Duplicate variable declarations
 * - Invalid formula structure
 */
class ValidationException : public FormulaException {
public:
    /**
     * @brief Construct a validation exception
     * @param msg Error message describing the validation error
     */
    explicit ValidationException(const std::string& msg)
        : FormulaException("Validation error: " + msg) {}
};

/**
 * @brief Exception thrown when an undeclared variable is referenced
 *
 * This exception is thrown when attempting to:
 * - Create a formula with a variable that hasn't been declared
 * - Parse a formula containing undeclared variables
 * - Get the ID of a variable that doesn't exist
 */
class UndeclaredVariableException : public ValidationException {
public:
    /**
     * @brief Construct an exception for an undeclared variable
     * @param name The name of the undeclared variable
     */
    explicit UndeclaredVariableException(const std::string& name)
        : ValidationException("Undeclared variable: " + name) {}
};

/**
 * @brief Exception thrown when a duplicate variable is declared
 *
 * This exception is thrown when attempting to:
 * - Declare the same variable name twice
 * - Declare a name that conflicts with a reserved keyword
 */
class DuplicateVariableException : public ValidationException {
public:
    /**
     * @brief Construct an exception for a duplicate variable
     * @param name The name of the duplicate variable
     */
    explicit DuplicateVariableException(const std::string& name)
        : ValidationException("Duplicate variable: " + name) {}
};

/**
 * @brief Exception thrown when an invalid variable name is used
 *
 * This exception is thrown when a variable name:
 * - Contains invalid characters
 * - Exceeds maximum length
 * - Is a reserved keyword
 * - Is empty
 */
class InvalidVariableNameException : public ValidationException {
public:
    /**
     * @brief Construct an exception for an invalid variable name
     * @param name The invalid variable name
     * @param reason Explanation of why the name is invalid
     */
    InvalidVariableNameException(const std::string& name, const std::string& reason)
        : ValidationException("Invalid variable name '" + name + "': " + reason) {}

    /**
     * @brief Construct an exception for an invalid variable name (simple)
     * @param name The invalid variable name
     */
    explicit InvalidVariableNameException(const std::string& name)
        : InvalidVariableNameException(name, "does not match naming rules") {}
};

/**
 * @brief Exception thrown when a formula transformation fails
 *
 * This can be caused by:
 * - Unsupported operators in specific contexts
 * - Malformed formulas that violate invariants
 * - Resource constraints during transformation
 */
class TransformationException : public FormulaException {
public:
    /**
     * @brief Construct a transformation exception
     * @param msg Error message describing the transformation error
     */
    explicit TransformationException(const std::string& msg)
        : FormulaException("Transformation error: " + msg) {}
};

/**
 * @brief Exception thrown when equivalence checking fails
 *
 * This can be caused by:
 * - Timeout during checking
 * - SMT solver errors
 * - Resource exhaustion
 */
class EquivalenceException : public FormulaException {
public:
    /**
     * @brief Construct an equivalence exception
     * @param msg Error message describing the equivalence check error
     */
    explicit EquivalenceException(const std::string& msg)
        : FormulaException("Equivalence check error: " + msg) {}
};

} // namespace formula

#endif // FORMULA_EXCEPTION_HPP
