/**
 * @file file_utils.hpp
 * @brief Common file I/O utilities for CosyZeroRewrite
 *
 * Provides reusable functions for:
 * - Reading file contents
 * - Parsing partition files (.part)
 * - Cleaning formula strings (remove comments, normalize whitespace)
 */

#ifndef COSY_ZERO_IO_FILE_UTILS_HPP
#define COSY_ZERO_IO_FILE_UTILS_HPP

#include <string>
#include <vector>
#include <optional>

namespace io {

//==============================================================================
// File Reading
//==============================================================================

/**
 * @brief Read entire file content into a string
 * @param filename Path to the file
 * @return File content as string, or empty string on error
 */
std::string read_file(const std::string& filename);

/**
 * @brief Read entire file content, returning optional
 * @param filename Path to the file
 * @return File content, or std::nullopt on error
 */
std::optional<std::string> read_file_opt(const std::string& filename);

/**
 * @brief Read file line by line
 * @param filename Path to the file
 * @return Vector of lines (without newline characters), empty on error
 */
std::vector<std::string> read_lines(const std::string& filename);

//==============================================================================
// Partition File Parsing
//==============================================================================

/**
 * @brief Partition specification for LTLf synthesis
 *
 * Format:
 *   .outputs: p1 p2 p3
 *   .inputs: i1 i2
 */
struct Partition {
    std::vector<std::string> outputs;  // Output variables (controlled)
    std::vector<std::string> inputs;   // Input variables (uncontrolled)
};

/**
 * @brief Parse a .part partition file
 * @param filename Path to the partition file
 * @return Parsed partition, or std::nullopt on error
 */
std::optional<Partition> parse_partition_file(const std::string& filename);

//==============================================================================
// String Cleaning
//==============================================================================

/**
 * @brief Clean formula string by removing comments and normalizing whitespace
 *
 * - Removes # comments (to end of line)
 * - Replaces newlines with spaces
 * - Trims leading/trailing whitespace
 *
 * @param raw Raw formula string (possibly with comments)
 * @return Cleaned formula string
 */
std::string clean_formula(const std::string& raw);

/**
 * @brief Trim leading and trailing whitespace from a string
 * @param s Input string
 * @return Trimmed string
 */
std::string trim(const std::string& s);

} // namespace io

#endif // COSY_ZERO_IO_FILE_UTILS_HPP
