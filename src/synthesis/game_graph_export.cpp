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
#include <iostream>
#include <algorithm>

namespace synthesis {

//==============================================================================
// DOT Format Export
//==============================================================================

std::string OnTheFlyGameSolver::to_dot(StateIdMap* external_id_map) const {
    std::ostringstream oss;

    // Use external StateIdMap if provided, otherwise create a local one
    StateIdMap local_id_map;
    StateIdMap& id_map = (external_id_map ? *external_id_map : local_id_map);

    // Set pool for variable name lookup (only if not already set)
    if (!id_map.pool) {
        id_map.pool = &pool_;
    }

    // Build ID map first (only if using local map or external map is empty)
    if (!external_id_map || id_map.to_id.empty()) {
        for (const auto& pair : successors_) {
            id_map.get_id(pair.first);
            for (const auto& succ : pair.second) {
                id_map.get_id(succ);
            }
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
                // Label with output assignment using variable names
                if (succ.system_chosen_output.has_value()) {
                    oss << ", label=\"sys="
                        << id_map.get_assignment_label(succ.system_chosen_output.value(), true)
                        << "\"";
                }
                oss << "];\n";
            } else {
                // Env move: red dashed line
                oss << "  " << from_id << " -> " << to_id
                    << " [color=red, style=dashed";
                // Label with input assignment using variable names
                if (succ.environment_chosen_input.has_value()) {
                    oss << ", label=\"env="
                        << id_map.get_assignment_label(succ.environment_chosen_input.value(), false)
                        << "\"";
                }
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
    id_map.pool = &pool_;  // Set pool for variable name lookup

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

            // Assignment info (with variable names)
            if (from.player == Player::System && succ.system_chosen_output.has_value()) {
                oss << "      \"output\": " << id_map.get_assignment_label(succ.system_chosen_output.value(), true) << ",\n";
            } else if (from.player == Player::Environment && succ.environment_chosen_input.has_value()) {
                oss << "      \"input\": " << id_map.get_assignment_label(succ.environment_chosen_input.value(), false) << ",\n";
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
// HTML Format Export (Interactive Visualization)
//==============================================================================

namespace {

/**
 * @brief Escape special characters for HTML
 */
std::string escape_html(const std::string& s) {
    std::string result;
    result.reserve(s.size() * 1.2);
    for (char c : s) {
        switch (c) {
            case '&':  result += "&amp;"; break;
            case '<':  result += "&lt;"; break;
            case '>':  result += "&gt;"; break;
            case '"':  result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            default:   result += c; break;
        }
    }
    return result;
}

} // anonymous namespace

std::string OnTheFlyGameSolver::to_html() const {
    std::ostringstream oss;
    StateIdMap id_map;

    // Build ID map first
    for (const auto& pair : successors_) {
        id_map.get_id(pair.first);
        for (const auto& succ : pair.second) {
            id_map.get_id(succ);
        }
    }

    // Get DOT content and escape it for JavaScript template literal
    std::string dot_content = to_dot();
    // Escape backslashes and backticks for JavaScript template literals
    std::string dot_content_escaped;
    dot_content_escaped.reserve(dot_content.size() * 1.2);
    for (char c : dot_content) {
        if (c == '\\') {
            dot_content_escaped += "\\\\";
        } else if (c == '`') {
            dot_content_escaped += "\\`";
        } else if (c == '$') {
            dot_content_escaped += "\\$";
        } else {
            dot_content_escaped += c;
        }
    }

    // Build JSON data for states (for tooltip)
    oss << "{\n";
    for (const auto& pair : id_map.to_id) {
        const GameState& state = pair.first;
        const std::string& id = pair.second;

        if (pair != *id_map.to_id.begin()) oss << ",\n";
        oss << "  \"" << id << "\": {\n";

        // Include ID for tooltip
        oss << "    \"id\": \"" << id << "\",\n";

        // Classification
        auto cls_it = classification_.find(state);
        if (cls_it != classification_.end()) {
            oss << "    \"classification\": \"" << to_string(cls_it->second) << "\",\n";
        } else {
            oss << "    \"classification\": \"Unknown\",\n";
        }

        oss << "    \"type\": \"" << (state.player == Player::System ? "System" : "Environment") << "\",\n";
        oss << "    \"is_initial\": " << (state == initial_state_ ? "true" : "false") << ",\n";

        // Formula info
        if (state.dfa_state) {
            formula::Formula* phi = state.dfa_state->phi();
            oss << "    \"phi\": \"" << (phi ? escape_html(phi->to_string_with_names(pool_)) : "null") << "\",\n";

            formula::Formula* xnf_phi = state.dfa_state->xnf_phi();
            oss << "    \"xnf_phi\": \"" << (xnf_phi ? escape_html(xnf_phi->to_string_with_names(pool_)) : "null") << "\",\n";

            // Prop atoms
            const auto& prop_atoms = state.dfa_state->prop_atoms();
            oss << "    \"prop_atoms\": [";
            bool first_atom = true;
            for (auto* f : prop_atoms) {
                if (!first_atom) oss << ", ";
                oss << "\"" << escape_html(f->to_string_with_names(pool_)) << "\"";
                first_atom = false;
            }
            oss << "]\n";
        } else {
            oss << "    \"phi\": null,\n";
            oss << "    \"xnf_phi\": null,\n";
            oss << "    \"prop_atoms\": []\n";
        }

        oss << "  }";
    }
    oss << "\n}";

    std::string json_data = oss.str();
    oss.str("");
    oss.clear();

    // Formula for title
    std::string formula_str = original_formula_ ?
        escape_html(original_formula_->to_string_with_names(pool_)) : "null";

    // Statistics
    size_t swin_count = 0, ewin_count = 0, unknown_count = 0;
    for (const auto& pair : classification_) {
        if (pair.second == StateClass::Swin) swin_count++;
        else if (pair.second == StateClass::Ewin) ewin_count++;
        else unknown_count++;
    }

    // Read viz.js content for embedding
    std::string vizjs_content;
    std::string vizjs_path = std::string(PROJECT_SOURCE_DIR) + "/resources/viz.js";
    std::ifstream vizjs_file(vizjs_path);
    if (vizjs_file.is_open()) {
        std::stringstream vizjs_buf;
        vizjs_buf << vizjs_file.rdbuf();
        vizjs_content = vizjs_buf.str();
        vizjs_file.close();
    } else {
        LOG_WARN("Failed to open viz.js for embedding: ", vizjs_path);
        vizjs_content = "// viz.js not found - visualization will not work";
    }

    // Build HTML
    oss << "<!DOCTYPE html>\n";
    oss << "<html lang=\"en\">\n";
    oss << "<head>\n";
    oss << "  <meta charset=\"UTF-8\">\n";
    oss << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    oss << "  <title>Game Graph: " << formula_str << "</title>\n";
    oss << "  <script>\n";
    oss << vizjs_content;
    oss << "\n  </script>\n";
    oss << "  <style>\n";
    oss << "    body {\n";
    oss << "      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;\n";
    oss << "      margin: 0;\n";
    oss << "      padding: 20px;\n";
    oss << "      background: #f5f5f5;\n";
    oss << "    }\n";
    oss << "    .header {\n";
    oss << "      background: white;\n";
    oss << "      padding: 15px 20px;\n";
    oss << "      border-radius: 8px;\n";
    oss << "      margin-bottom: 20px;\n";
    oss << "      box-shadow: 0 2px 4px rgba(0,0,0,0.1);\n";
    oss << "    }\n";
    oss << "    .header h1 {\n";
    oss << "      margin: 0 0 10px 0;\n";
    oss << "      font-size: 18px;\n";
    oss << "      color: #333;\n";
    oss << "    }\n";
    oss << "    .header .formula {\n";
    oss << "      font-family: monospace;\n";
    oss << "      color: #0066cc;\n";
    oss << "      word-break: break-all;\n";
    oss << "    }\n";
    oss << "    .header .stats {\n";
    oss << "      margin-top: 10px;\n";
    oss << "      font-size: 13px;\n";
    oss << "      color: #666;\n";
    oss << "    }\n";
    oss << "    .header .stats span {\n";
    oss << "      margin-right: 15px;\n";
    oss << "    }\n";
    oss << "    .header .stats .swin { color: #228b22; }\n";
    oss << "    .header .stats .ewin { color: #cd5c5c; }\n";
    oss << "    #graph-container {\n";
    oss << "      background: white;\n";
    oss << "      border-radius: 8px;\n";
    oss << "      padding: 20px;\n";
    oss << "      box-shadow: 0 2px 4px rgba(0,0,0,0.1);\n";
    oss << "      overflow: auto;\n";
    oss << "    }\n";
    oss << "    #tooltip {\n";
    oss << "      position: fixed;\n";
    oss << "      display: none;\n";
    oss << "      background: rgba(0, 0, 0, 0.95);\n";
    oss << "      color: white;\n";
    oss << "      padding: 12px 16px;\n";
    oss << "      border-radius: 6px;\n";
    oss << "      font-size: 13px;\n";
    oss << "      max-width: 500px;\n";
    oss << "      z-index: 1000;\n";
    oss << "      pointer-events: none;\n";
    oss << "      box-shadow: 0 4px 12px rgba(0,0,0,0.3);\n";
    oss << "    }\n";
    oss << "    #tooltip h3 {\n";
    oss << "      margin: 0 0 8px 0;\n";
    oss << "      font-size: 14px;\n";
    oss << "      border-bottom: 1px solid #444;\n";
    oss << "      padding-bottom: 5px;\n";
    oss << "    }\n";
    oss << "    #tooltip .label {\n";
    oss << "      color: #aaa;\n";
    oss << "      font-size: 11px;\n";
    oss << "      margin-top: 8px;\n";
    oss << "    }\n";
    oss << "    #tooltip .value {\n";
    oss << "      font-family: monospace;\n";
    oss << "      word-break: break-all;\n";
    oss << "    }\n";
    oss << "    #tooltip .classification {\n";
    oss << "      display: inline-block;\n";
    oss << "      padding: 2px 8px;\n";
    oss << "      border-radius: 4px;\n";
    oss << "      font-size: 12px;\n";
    oss << "      font-weight: bold;\n";
    oss << "      margin-top: 8px;\n";
    oss << "    }\n";
    oss << "    #tooltip .classification.Swin {\n";
    oss << "      background: #228b22;\n";
    oss << "    }\n";
    oss << "    #tooltip .classification.Ewin {\n";
    oss << "      background: #cd5c5c;\n";
    oss << "    }\n";
    oss << "    #tooltip .classification.Unknown {\n";
    oss << "      background: #888;\n";
    oss << "    }\n";
    oss << "    .legend {\n";
    oss << "      margin-top: 15px;\n";
    oss << "      padding: 10px;\n";
    oss << "      background: #f9f9f9;\n";
    oss << "      border-radius: 6px;\n";
    oss << "      font-size: 12px;\n";
    oss << "    }\n";
    oss << "    .legend-item {\n";
    oss << "      display: inline-flex;\n";
    oss << "      align-items: center;\n";
    oss << "      margin-right: 20px;\n";
    oss << "    }\n";
    oss << "    .legend-box {\n";
    oss << "      width: 16px;\n";
    oss << "      height: 16px;\n";
    oss << "      margin-right: 6px;\n";
    oss << "      border: 2px solid;\n";
    oss << "    }\n";
    oss << "    .legend-box.sys-circle {\n";
    oss << "      border-radius: 50%;\n";
    oss << "      border-color: blue;\n";
    oss << "    }\n";
    oss << "    .legend-box.env-box {\n";
    oss << "      border-color: orange;\n";
    oss << "    }\n";
    oss << "    .legend-box.swin {\n";
    oss << "      background: lightgreen;\n";
    oss << "    }\n";
    oss << "    .legend-box.ewin {\n";
    oss << "      background: lightcoral;\n";
    oss << "    }\n";
    oss << "  </style>\n";
    oss << "</head>\n";
    oss << "<body>\n";
    oss << "  <div class=\"header\">\n";
    oss << "    <h1>Game Graph Visualization</h1>\n";
    oss << "    <div class=\"formula\">" << formula_str << "</div>\n";
    oss << "    <div class=\"stats\">\n";
    oss << "      <span>Total: " << successors_.size() << " states</span>\n";
    oss << "      <span class=\"swin\">Swin: " << swin_count << "</span>\n";
    oss << "      <span class=\"ewin\">Ewin: " << ewin_count << "</span>\n";
    oss << "    </div>\n";
    oss << "    <div class=\"legend\">\n";
    oss << "      <span class=\"legend-item\">\n";
    oss << "        <span class=\"legend-box sys-circle\"></span>\n";
    oss << "        System (circle, blue)\n";
    oss << "      </span>\n";
    oss << "      <span class=\"legend-item\">\n";
    oss << "        <span class=\"legend-box env-box\"></span>\n";
    oss << "        Environment (box, orange)\n";
    oss << "      </span>\n";
    oss << "      <span class=\"legend-item\">\n";
    oss << "        <span class=\"legend-box swin\"></span>\n";
    oss << "        Swin (green)\n";
    oss << "      </span>\n";
    oss << "      <span class=\"legend-item\">\n";
    oss << "        <span class=\"legend-box ewin\"></span>\n";
    oss << "        Ewin (red)\n";
    oss << "      </span>\n";
    oss << "    </div>\n";
    oss << "  </div>\n";
    oss << "  <div id=\"graph-container\"></div>\n";
    oss << "  <div id=\"tooltip\"></div>\n";
    oss << "  <script>\n";
    oss << "    // Embedded DOT content\n";
    oss << "    const DOT = `" << dot_content_escaped << "`;\n\n";
    oss << "    // Embedded state data\n";
    oss << "    const STATE_DATA = " << json_data << ";\n\n";
    oss << "    // Render graph using viz.js (synchronous API from webgraphviz)\n";
    oss << "    try {\n";
    oss << "      const svgString = Viz(DOT, 'svg');\n";
    oss << "      if (svgString) {\n";
    oss << "        document.getElementById('graph-container').innerHTML = '<hr>' + svgString;\n\n";
    oss << "        // Add hover events to nodes\n";
    oss << "        const svgElement = document.querySelector('#graph-container svg');\n";
    oss << "        if (svgElement) {\n";
    oss << "          svgElement.addEventListener('mouseover', function(e) {\n";
    oss << "            const target = e.target;\n";
    oss << "            if (target.tagName === 'title') return;\n";
    oss << "            let node = target;\n";
    oss << "            while (node && node.tagName !== 'g') {\n";
    oss << "              node = node.parentNode;\n";
    oss << "            }\n";
    oss << "            if (!node) return;\n\n";
    oss << "            // Find node title (the state ID)\n";
    oss << "            const title = node.querySelector('title');\n";
    oss << "            if (!title) return;\n";
    oss << "            const stateId = title.textContent.trim().split('\\n')[0];\n\n";
    oss << "            // Get state data\n";
    oss << "            const data = STATE_DATA[stateId];\n";
    oss << "            if (!data) return;\n\n";
    oss << "            // Show tooltip\n";
    oss << "            showTooltip(e, data);\n";
    oss << "          });\n\n";
    oss << "          svgElement.addEventListener('mouseout', function() {\n";
    oss << "            hideTooltip();\n";
    oss << "          });\n";
    oss << "        }\n";
    oss << "      } else {\n";
    oss << "        throw new Error('Viz returned empty result');\n";
    oss << "      }\n";
    oss << "    } catch (error) {\n";
    oss << "      console.error('viz rendering error:', error);\n";
    oss << "      document.getElementById('graph-container').innerHTML =\n";
    oss << "        '<p style=\"color: red; padding: 20px;\">Error rendering graph: ' + error + '</p>';\n";
    oss << "    }\n\n";
    oss << "    function showTooltip(e, data) {\n";
    oss << "      const tooltip = document.getElementById('tooltip');\n";
    oss << "      tooltip.innerHTML = buildTooltip(data);\n";
    oss << "      tooltip.style.display = 'block';\n\n";
    oss << "      // Position tooltip\n";
    oss << "      const x = Math.min(e.clientX + 15, window.innerWidth - 520);\n";
    oss << "      const y = Math.min(e.clientY + 15, window.innerHeight - 200);\n";
    oss << "      tooltip.style.left = x + 'px';\n";
    oss << "      tooltip.style.top = y + 'px';\n";
    oss << "    }\n\n";
    oss << "    function hideTooltip() {\n";
    oss << "      document.getElementById('tooltip').style.display = 'none';\n";
    oss << "    }\n\n";
    oss << "    function buildTooltip(data) {\n";
    oss << "      let html = '<h3>' + data.id;\n";
    oss << "      if (data.is_initial) html += ' (initial)';\n";
    oss << "      html += '</h3>';\n";
    oss << "      html += '<span class=\"classification ' + data.classification + '\">' + data.classification + '</span>';\n";
    oss << "      html += '<div class=\"label\">Type:</div><div class=\"value\">' + data.type + '</div>';\n";
    oss << "      if (data.phi) {\n";
    oss << "        html += '<div class=\"label\">Formula (phi):</div><div class=\"value\">' + data.phi + '</div>';\n";
    oss << "      }\n";
    oss << "      if (data.xnf_phi) {\n";
    oss << "        html += '<div class=\"label\">XNF Formula:</div><div class=\"value\">' + data.xnf_phi + '</div>';\n";
    oss << "      }\n";
    oss << "      if (data.prop_atoms && data.prop_atoms.length > 0) {\n";
    oss << "        html += '<div class=\"label\">Propositional Atoms:</div><div class=\"value\">[' + data.prop_atoms.join(', ') + ']</div>';\n";
    oss << "      }\n";
    oss << "      return html;\n";
    oss << "    }\n";
    oss << "  </script>\n";
    oss << "</body>\n";
    oss << "</html>\n";

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

    // Write HTML file
    std::string html_path = base_path + ".html";
    std::ofstream html_file(html_path);
    if (!html_file.is_open()) {
        LOG_WARN("Failed to open HTML file for writing: ", html_path);
        return false;
    }
    html_file << to_html();
    html_file.close();
    LOG_INFO("Game graph HTML written to: ", html_path);

    // Copy viz.js to the output directory for local usage
    std::string src_path = PROJECT_SOURCE_DIR "/resources/viz.js";
    std::string output_dir = base_path.substr(0, base_path.find_last_of('/'));
    std::string vizjs_dest = output_dir + "/viz.js";
    std::string copy_cmd = "cp \"" + src_path + "\" \"" + vizjs_dest + "\"";
    if (system(copy_cmd.c_str()) != 0) {
        LOG_WARN("Failed to copy viz.js to: ", vizjs_dest);
    } else {
        LOG_INFO("viz.js copied to: ", vizjs_dest);
    }

    return true;
}

} // namespace synthesis
