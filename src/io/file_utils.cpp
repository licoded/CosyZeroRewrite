/**
 * @file file_utils.cpp
 * @brief Implementation of common file I/O utilities
 */

#include "io/file_utils.hpp"

#include "log/logger.hpp"

#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace io {

//==============================================================================
// File Reading
//==============================================================================

std::string read_file(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        LOG_ERROR("Cannot open file: {}", filename);
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::optional<std::string> read_file_opt(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        return std::nullopt;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<std::string> read_lines(const std::string &filename)
{
    std::vector<std::string> lines;
    std::ifstream file(filename);

    if (!file.is_open())
    {
        LOG_ERROR("Cannot open file: {}", filename);
        return lines;
    }

    std::string line;
    while (std::getline(file, line))
    {
        lines.push_back(line);
    }

    return lines;
}

//==============================================================================
// Partition File Parsing
//==============================================================================

static std::string to_upper(const std::string &s)
{
    std::string result = s;
    for (char &c : result)
    {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

static std::string trim_whitespace(const std::string &s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

tl::expected<Partition, std::string> parse_partition_file(const std::string &filename)
{
    Partition result;
    std::ifstream file(filename);

    if (!file.is_open())
    {
        return tl::unexpected(fmt::format("Cannot open partition file: {}", filename));
    }

    enum class Section
    {
        None,
        Outputs,
        Inputs
    };
    Section section = Section::None;
    std::string line;

    while (std::getline(file, line))
    {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#')
            continue;

        // Trim leading/trailing whitespace
        line = trim_whitespace(line);
        if (line.empty())
            continue;

        // Convert to uppercase for section matching
        std::string upper_line = to_upper(line);

        // Check for section headers (with or without dot, with or without colon)
        if (upper_line == ".OUTPUTS:" || upper_line == "OUTPUTS:" || upper_line == ".OUTPUTS"
            || upper_line == "OUTPUTS")
        {
            section = Section::Outputs;
            continue;
        }
        else if (upper_line == ".INPUTS:" || upper_line == "INPUTS:" || upper_line == ".INPUTS"
                 || upper_line == "INPUTS")
        {
            section = Section::Inputs;
            continue;
        }

        // Check if line starts with a section header followed by variables on same line
        // e.g., ".outputs: p1 p2" or "outputs: p1 p2"
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos)
        {
            std::string prefix = line.substr(0, colon_pos);
            std::string upper_prefix = to_upper(prefix);

            if (upper_prefix == ".OUTPUTS" || upper_prefix == "OUTPUTS")
            {
                section = Section::Outputs;
                // Extract variables after colon
                std::string rest = line.substr(colon_pos + 1);
                std::istringstream iss(rest);
                std::string token;
                while (iss >> token)
                {
                    if (!token.empty())
                        result.outputs.push_back(token);
                }
                continue;
            }
            else if (upper_prefix == ".INPUTS" || upper_prefix == "INPUTS")
            {
                section = Section::Inputs;
                // Extract variables after colon
                std::string rest = line.substr(colon_pos + 1);
                std::istringstream iss(rest);
                std::string token;
                while (iss >> token)
                {
                    if (!token.empty())
                        result.inputs.push_back(token);
                }
                continue;
            }
        }

        // Parse variables in current section
        std::istringstream iss(line);
        std::string token;
        while (iss >> token)
        {
            if (token.empty() || token[0] == '#')
                break; // Stop at comment
            if (section == Section::Outputs)
            {
                result.outputs.push_back(token);
            }
            else if (section == Section::Inputs)
            {
                result.inputs.push_back(token);
            }
        }
    }

    return result;
}

//==============================================================================
// String Cleaning
//==============================================================================

std::string clean_formula(const std::string &raw)
{
    std::string result;
    bool in_comment = false;

    for (size_t i = 0; i < raw.size(); ++i)
    {
        if (in_comment)
        {
            if (raw[i] == '\n')
                in_comment = false;
            continue;
        }

        if (raw[i] == '#')
        {
            in_comment = true;
            continue;
        }

        // Replace newlines with spaces
        if (raw[i] == '\n' || raw[i] == '\r')
        {
            result += ' ';
        }
        else
        {
            result += raw[i];
        }
    }

    // Trim leading/trailing whitespace
    return trim(result);
}

std::string trim(const std::string &s)
{
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

} // namespace io
