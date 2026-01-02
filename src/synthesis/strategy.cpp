/**
 * @file strategy.cpp
 * @brief Implementation of strategy extraction and export
 */

#include "synthesis/strategy.hpp"
#include "log/logger.hpp"
#include <sstream>
#include <queue>

namespace synthesis {

//==============================================================================
// StrategyMove
//==============================================================================

std::string StrategyMove::to_string() const {
    std::ostringstream oss;
    oss << "{out=[";
    bool first = true;
    for (int v : output) {
        if (!first) oss << ",";
        oss << v;
        first = false;
    }
    oss << "]}";
    return oss.str();
}

//==============================================================================
// Strategy
//==============================================================================

std::optional<Strategy> Strategy::extract(const OnTheFlyGameSolver& solver) {
    return StrategyExtractor::extract(solver);
}

std::optional<StrategyMove> Strategy::get_move(const GameState& state) const {
    auto it = moves_.find(state);
    if (it != moves_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::string Strategy::to_json(const formula::FormulaPool& pool) const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"type\": \"ltlf_synthesis_strategy\",\n";
    oss << "  \"num_states\": " << moves_.size() << ",\n";
    oss << "  \"states\": {\n";

    bool first_state = true;
    for (const auto& pair : moves_) {
        if (!first_state) oss << ",\n";
        first_state = false;

        const GameState& state = pair.first;
        const StrategyMove& move = pair.second;

        std::string state_id = StrategyExtractor::state_to_id(state);

        oss << "    \"" << state_id << "\": {\n";
        oss << "      \"dfa_state\": \"" << state.dfa_state->hash() << "\",\n";
        oss << "      \"player\": \"" << (state.player == Player::System ? "System" : "Environment") << "\",\n";
        oss << "      \"output\": [" << StrategyExtractor::assignment_to_string(move.output, pool) << "],\n";
        oss << "      \"env_responses\": {\n";

        bool first_resp = true;
        for (const auto& resp : move.env_responses) {
            if (!first_resp) oss << ",\n";
            first_resp = false;
            oss << "        \"" << resp.second << "\": ["
                 << StrategyExtractor::assignment_to_string(resp.first, pool) << "]";
        }
        oss << "\n      }\n";
        oss << "    }";
    }

    oss << "\n  }\n";
    oss << "}\n";
    return oss.str();
}

std::string Strategy::to_dot(const formula::FormulaPool& pool) const {
    std::ostringstream oss;
    oss << "digraph Strategy {\n";
    oss << "  rankdir=LR;\n";
    oss << "  node [shape=circle];\n";
    oss << "  label=\"LTLf Synthesis Strategy\";\n\n";

    // Add nodes
    std::unordered_map<std::string, std::string> state_labels;
    for (const auto& pair : moves_) {
        const GameState& state = pair.first;
        std::string state_id = StrategyExtractor::state_to_id(state);

        // Create node label
        std::ostringstream label;
        label << state_id << "\\n";
        label << "[" << StrategyExtractor::assignment_to_string(state.current_output, pool) << "]";
        state_labels[state_id] = label.str();

        oss << "  \"" << state_id << "\" [label=\"" << label.str() << "\"];\n";
    }

    // Add edges
    for (const auto& pair : moves_) {
        const GameState& state = pair.first;
        const StrategyMove& move = pair.second;
        std::string from_id = StrategyExtractor::state_to_id(state);

        for (const auto& resp : move.env_responses) {
            std::string to_id = resp.second;
            std::string input = StrategyExtractor::assignment_to_string(resp.first, pool);

            oss << "  \"" << from_id << "\" -> \"" << to_id << "\" "
                 << "[label=\"" << input << "\"];\n";
        }
    }

    oss << "}\n";
    return oss.str();
}

bool Strategy::verify(formula::Formula* phi, formula::FormulaPool& pool) const {
    return StrategyVerifier::verify(*this, phi, pool);
}

//==============================================================================
// StrategyExtractor
//==============================================================================

std::optional<Strategy> StrategyExtractor::extract(const OnTheFlyGameSolver& solver) {
    // Check if initial state is winning
    auto initial_class = solver.get_classification(solver.get_initial_state());
    if (initial_class != StateClass::Swin) {
        LOG_WARN("StrategyExtractor: formula is not realizable");
        return std::nullopt;
    }

    LOG_DEBUG("StrategyExtractor: extracting winning strategy");

    Strategy strategy(solver.get_initial_state());

    // Get classification from solver
    const auto& classification = solver.get_classification();

    // Use BFS from initial state, following winning transitions
    std::queue<GameState> worklist;
    std::unordered_set<GameState, GameStateHash, GameStateEqual> visited;

    worklist.push(solver.get_initial_state());
    visited.insert(solver.get_initial_state());

    while (!worklist.empty()) {
        GameState current = worklist.front();
        worklist.pop();

        LOG_DEBUG("StrategyExtractor: processing state");

        // Only process system states
        if (current.player != Player::System) {
            continue;
        }

        // Skip if already processed
        if (strategy.has_move(current)) {
            continue;
        }

        // Get successors
        auto succs = solver.get_successors(current);
        if (!succs) {
            LOG_WARN("StrategyExtractor: state not expanded");
            continue;
        }

        // Find winning output
        auto output = find_winning_output(current, solver);
        if (!output) {
            LOG_WARN("StrategyExtractor: no winning output found");
            continue;
        }

        // Build the strategy move with all environment responses
        StrategyMove move;
        move.output = *output;

        // For each possible input, find the successor state
        const auto& input_gen = solver.get_input_generator();
        const auto& output_gen = solver.get_output_generator();
        const auto& dfa = solver.dfa();

        for (const auto& input : input_gen.all_assignments()) {
            // Combine output and input into full assignment
            automata::Assignment full = *output;

            // Offset input variable IDs by number of outputs
            for (int v : input) {
                full.insert(v + output_gen.num_variables());
            }

            // Compute next DFA state
            automata::TableauState* next_dfa = dfa.successor(current.dfa_state, full);
            GameState next_sys_state(next_dfa, Player::System);

            // Record this transition: input -> next state
            std::string state_id = state_to_id(next_sys_state);
            move.env_responses.push_back({input, state_id});

            // Add winning successor to worklist
            if (classification.count(next_sys_state) &&
                classification.at(next_sys_state) == StateClass::Swin &&
                !visited.count(next_sys_state)) {
                worklist.push(next_sys_state);
                visited.insert(next_sys_state);
            }
        }

        strategy.add_move(current, move);
    }

    LOG_DEBUG("StrategyExtractor: extracted strategy with ", strategy.size(), " states");
    return strategy;
}

std::optional<automata::Assignment> StrategyExtractor::find_winning_output(
    const GameState& sys_state,
    const OnTheFlyGameSolver& solver
) {
    // A system state is winning if there exists an output assignment such that
    // for all environment inputs, all successor system states are Swin.

    const auto& output_gen = solver.get_output_generator();
    const auto& input_gen = solver.get_input_generator();
    const auto& dfa = solver.dfa();
    const auto& classification = solver.get_classification();

    // Try each possible output assignment
    for (const auto& output : output_gen.all_assignments()) {
        bool all_successors_winning = true;

        // Create the environment state with this output
        GameState env_state(sys_state.dfa_state, Player::Environment, output);

        // Get all inputs (environment choices)
        for (const auto& input : input_gen.all_assignments()) {
            // Combine output and input into full assignment
            automata::Assignment full = output;

            // Offset input variable IDs by number of outputs
            for (int v : input) {
                full.insert(v + output_gen.num_variables());
            }

            // Compute next DFA state
            automata::TableauState* next_dfa = dfa.successor(sys_state.dfa_state, full);
            GameState next_sys_state(next_dfa, Player::System);

            // Check if this successor is Swin
            auto it = classification.find(next_sys_state);
            if (it == classification.end() || it->second != StateClass::Swin) {
                // This input leads to a non-winning state
                all_successors_winning = false;
                break;
            }
        }

        if (all_successors_winning) {
            // Found a winning output!
            LOG_DEBUG("StrategyExtractor: found winning output for system state");
            return output;
        }
    }

    LOG_DEBUG("StrategyExtractor: no winning output found");
    return std::nullopt;
}

std::string StrategyExtractor::state_to_id(const GameState& state) {
    std::ostringstream oss;
    oss << "s" << state.dfa_state->hash() << "_"
        << (state.player == Player::System ? "sys" : "env");
    return oss.str();
}

std::string StrategyExtractor::assignment_to_string(
    const automata::Assignment& assignment,
    const formula::FormulaPool& pool
) {
    if (assignment.empty()) {
        return "{}";
    }

    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (int var_id : assignment) {
        if (!first) oss << ", ";
        oss << "\"" << pool.get_variable_name(var_id) << "\"";
        first = false;
    }
    oss << "}";
    return oss.str();
}

//==============================================================================
// StrategyVerifier
//==============================================================================

bool StrategyVerifier::verify(
    const Strategy& strategy,
    formula::Formula* phi,
    formula::FormulaPool& pool
) {
    (void)phi;  // Will be used for full simulation-based verification

    // Basic verification: check that all moves have valid outputs
    for (const auto& pair : strategy.moves_) {
        const GameState& state = pair.first;
        const StrategyMove& move = pair.second;

        if (state.player != Player::System) {
            LOG_WARN("StrategyVerifier: non-system state in strategy");
            return false;
        }

        // Check that output assignment only contains output variables
        for (int var_id : move.output) {
            if (!pool.is_output_variable(var_id)) {
                LOG_WARN("StrategyVerifier: output contains input variable");
                return false;
            }
        }
    }

    // TODO: Add full simulation-based verification
    // This would involve:
    // 1. Starting from initial state
    // 2. Following strategy choices
    // 3. Checking that all accepting conditions are met

    LOG_DEBUG("StrategyVerifier: basic validation passed");
    return true;
}

} // namespace synthesis
