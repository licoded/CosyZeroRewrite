/**
 * @file game_graph_export.cpp
 * @brief Game graph visualization (DOT + JSON export)
 *
 * Implements to_dot(), to_json(), and write_dot() methods for
 * OnTheFlyGameSolver. Separated for better code organization.
 */

#include "synthesis/on_the_fly_solver.hpp"
#include "log/logger.hpp"
#include <sstream>
#include <fstream>
#include <algorithm>

namespace synthesis {

//==============================================================================
// State ID Mapping Helper
//==============================================================================

namespace {

/**
 * @brief Generate short ID for a game state
 * System states: S0, S1, S2, ...
 * Environment states: E0, E1, E2, ...
 */
struct StateIdMap {
    std::unordered_map<GameState, std::string, GameStateHash, GameStateEqual> to_id;
    std::unordered_map<std::string, GameState> from_id;
    size_t sys_count = 0;
    size_t env_count = 0;

    std::string get_id(const GameState& s) {
        auto it = to_id.find(s);
        if (it != to_id.end()) {
            return it->second;
        }

        std::string id;
        if (s.player == Player::System) {
            id = "S" + std::to_string(sys_count++);
        } else {
            id = "E" + std::to_string(env_count++);
        }

        to_id[s] = id;
        from_id[id] = s;
        return id;
    }

    std::string get_assignment_string(const automata::Assignment& a) const {
        if (a.empty()) return "{}";
        std::ostringstream oss;
        oss << "{";
        bool first = true;
        for (int v : a) {
            if (!first) oss << ",";
            oss << v;
            first = false;
        }
        oss << "}";
        return oss.str();
    }
};

} // anonymous namespace

//==============================================================================
// DOT Format Export
//==============================================================================

std::string OnTheFlyGameSolver::to_dot() const {
    std::ostringstream oss;
    StateIdMap id_map;

    // Build ID map first
    for (const auto& pair : successors_) {
        id_map.get_id(pair.first);
        for (const auto& succ : pair.second) {
            id_map.get_id(succ);
        }
    }

    // DOT header
    oss << "digraph GameGraph {\n";
    oss << "  rankdir=LR;\n";
    oss << "  node [style=filled];\n";
    oss << "  // Visual legend:\n";
    oss << "  // System states: circles (blue border)\n";
    oss << "  // Environment states: boxes (orange border)\n";
    oss << "  // Swin: light green fill | Ewin: light red fill\n";
    oss << "  // Sys moves: blue solid lines | Env moves: red dashed lines\n\n";

    // Define nodes
    oss << "  // === Nodes ===\n";
    for (const auto& pair : id_map.to_id) {
        const GameState& state = pair.first;
        const std::string& id = pair.second;
        bool is_sys = (state.player == Player::System);
        bool is_initial = (state == initial_state_);

        // Get classification
        auto cls_it = classification_.find(state);
        StateClass cls = (cls_it != classification_.end()) ? cls_it->second : StateClass::Unknown;

        // Node shape and color
        if (is_sys) {
            oss << "  " << id << " [shape=circle";
        } else {
            oss << "  " << id << " [shape=box";
        }

        // Fill color based on classification
        if (cls == StateClass::Swin) {
            oss << ", fillcolor=lightgreen";
        } else if (cls == StateClass::Ewin) {
            oss << ", fillcolor=lightcoral";
        } else {
            oss << ", fillcolor=lightgray";
        }

        // Border color
        if (is_sys) {
            oss << ", color=blue";
        } else {
            oss << ", color=orange";
        }

        // Penwidth for initial state
        if (is_initial) {
            oss << ", penwidth=3";
        }

        // Label: just show ID and classification
        oss << ", label=\"" << id;
        if (is_initial) oss << " (init)";
        oss << "\\n" << to_string(cls) << "\"];\n";
    }

    oss << "\n  // === Transitions ===\n";

    // Define edges
    for (const auto& pair : successors_) {
        const GameState& from = pair.first;
        const std::string from_id = id_map.get_id(from);
        bool from_is_sys = (from.player == Player::System);

        for (const auto& succ : pair.second) {
            const std::string to_id = id_map.get_id(succ);

            if (from_is_sys) {
                // Sys move: blue solid line
                oss << "  " << from_id << " -> " << to_id
                    << " [color=blue, style=solid";
                // Label with output assignment
                if (succ.system_chosen_output.has_value()) {
                    oss << ", label=\"out="
                        << id_map.get_assignment_string(succ.system_chosen_output.value())
                        << "\"";
                }
                oss << "];\n";
            } else {
                // Env move: red dashed line
                oss << "  " << from_id << " -> " << to_id
                    << " [color=red, style=dashed";
                oss << "];\n";
            }
        }
    }

    oss << "}\n";
    return oss.str();
}

//==============================================================================
// JSON Format Export
//==============================================================================

std::string OnTheFlyGameSolver::to_json() const {
    std::ostringstream oss;
    StateIdMap id_map;

    // Build ID map first
    for (const auto& pair : successors_) {
        id_map.get_id(pair.first);
        for (const auto& succ : pair.second) {
            id_map.get_id(succ);
        }
    }

    oss << "{\n";

    // Formula info
    oss << "  \"formula\": \"";
    if (original_formula_) {
        oss << original_formula_->to_string_with_names(pool_);
    } else {
        oss << "null";
    }
    oss << "\",\n";

    // Statistics
    size_t swin_count = 0, ewin_count = 0, unknown_count = 0;
    for (const auto& pair : classification_) {
        if (pair.second == StateClass::Swin) swin_count++;
        else if (pair.second == StateClass::Ewin) ewin_count++;
        else unknown_count++;
    }

    oss << "  \"statistics\": {\n";
    oss << "    \"total_states\": " << successors_.size() << ",\n";
    oss << "    \"swin_states\": " << swin_count << ",\n";
    oss << "    \"ewin_states\": " << ewin_count << ",\n";
    oss << "    \"unknown_states\": " << unknown_count << ",\n";
    oss << "    \"num_sccs_found\": " << num_sccs_found_ << "\n";
    oss << "  },\n";

    // Initial state
    oss << "  \"initial_state\": \"" << id_map.get_id(initial_state_) << "\",\n";

    // States
    oss << "  \"states\": [\n";
    bool first_state = true;
    for (const auto& pair : id_map.to_id) {
        const GameState& state = pair.first;
        const std::string& id = pair.second;

        if (!first_state) oss << ",\n";
        first_state = false;

        oss << "    {\n";
        oss << "      \"id\": \"" << id << "\",\n";
        oss << "      \"type\": \"" << (state.player == Player::System ? "System" : "Environment") << "\",\n";
        oss << "      \"dfa_state_ptr\": \"" << state.dfa_state << "\",\n";

        // Classification
        auto cls_it = classification_.find(state);
        if (cls_it != classification_.end()) {
            oss << "      \"classification\": \"" << to_string(cls_it->second) << "\",\n";
        } else {
            oss << "      \"classification\": \"Unknown\",\n";
        }

        oss << "      \"is_initial\": " << (state == initial_state_ ? "true" : "false") << ",\n";

        // Formula info
        if (state.dfa_state) {
            formula::Formula* phi = state.dfa_state->phi();
            oss << "      \"phi\": \"" << (phi ? phi->to_string_with_names(pool_) : "null") << "\",\n";

            formula::Formula* xnf_phi = state.dfa_state->xnf_phi();
            oss << "      \"xnf_phi\": \"" << (xnf_phi ? xnf_phi->to_string_with_names(pool_) : "null") << "\",\n";

            // Prop atoms
            const auto& prop_atoms = state.dfa_state->prop_atoms();
            oss << "      \"prop_atoms\": [";
            bool first_atom = true;
            for (auto* f : prop_atoms) {
                if (!first_atom) oss << ", ";
                oss << "\"" << f->to_string_with_names(pool_) << "\"";
                first_atom = false;
            }
            oss << "]\n";
        } else {
            oss << "      \"phi\": null,\n";
            oss << "      \"xnf_phi\": null,\n";
            oss << "      \"prop_atoms\": []\n";
        }

        oss << "    }";
    }
    oss << "\n  ],\n";

    // Transitions
    oss << "  \"transitions\": [\n";
    bool first_trans = true;
    for (const auto& pair : successors_) {
        const GameState& from = pair.first;
        const std::string from_id = id_map.get_id(from);

        for (const auto& succ : pair.second) {
            const std::string to_id = id_map.get_id(succ);

            if (!first_trans) oss << ",\n";
            first_trans = false;

            oss << "    {\n";
            oss << "      \"from\": \"" << from_id << "\",\n";
            oss << "      \"to\": \"" << to_id << "\",\n";
            oss << "      \"type\": \"" << (from.player == Player::System ? "sys_move" : "env_move") << "\",\n";

            // Assignment info
            if (from.player == Player::System && succ.system_chosen_output.has_value()) {
                oss << "      \"output\": " << id_map.get_assignment_string(succ.system_chosen_output.value()) << ",\n";
            }
            oss << "      \"is_sys_move\": " << (from.player == Player::System ? "true" : "false") << "\n";
            oss << "    }";
        }
    }
    oss << "\n  ]\n";

    oss << "}\n";
    return oss.str();
}

//==============================================================================
// File Writing
//==============================================================================

bool OnTheFlyGameSolver::write_dot(const std::string& base_path) const {
    // Create directory if needed
    size_t last_slash = base_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        std::string dir = base_path.substr(0, last_slash);
        std::string cmd = "mkdir -p \"" + dir + "\"";
        if (system(cmd.c_str()) != 0) {
            LOG_WARN("Failed to create directory: ", dir);
            return false;
        }
    }

    // Write DOT file
    std::string dot_path = base_path + ".dot";
    std::ofstream dot_file(dot_path);
    if (!dot_file.is_open()) {
        LOG_WARN("Failed to open DOT file for writing: ", dot_path);
        return false;
    }
    dot_file << to_dot();
    dot_file.close();
    LOG_INFO("Game graph DOT written to: ", dot_path);

    // Write JSON file
    std::string json_path = base_path + ".json";
    std::ofstream json_file(json_path);
    if (!json_file.is_open()) {
        LOG_WARN("Failed to open JSON file for writing: ", json_path);
        return false;
    }
    json_file << to_json();
    json_file.close();
    LOG_INFO("Game graph JSON written to: ", json_path);

    return true;
}

} // namespace synthesis
