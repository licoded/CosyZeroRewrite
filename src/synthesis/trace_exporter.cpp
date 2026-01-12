/**
 * @file trace_exporter.cpp
 * @brief Implementation of trace exporter for synthesis visualization
 */

#include "synthesis/trace_exporter.hpp"
#include "log/logger.hpp"
#include "DSViz/dsv.hpp"  // For DOT generation
#include <nlohmann/json.hpp>
#include <algorithm>    // for std::sort
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <spdlog/fmt/fmt.h>
#include <cppitertools/imap.hpp>  // for iter::imap

namespace synthesis {

//==============================================================================
// Helper Functions
//==============================================================================

namespace {

/**
 * @brief Get current timestamp in ISO 8601 format
 */
std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);

    std::ostringstream oss;
    oss << std::setfill('0');
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

} // anonymous namespace

//==============================================================================
// Internal DOT generation function for TraceExporter
//==============================================================================

/**
 * @brief Generate DOT format from solver (internal function for TraceExporter)
 */
static std::string solver_to_dot(const OnTheFlyGameSolver& solver, StateIdMap& id_map) {
    // Set pool for variable name lookup
    if (!id_map.pool) {
        id_map.pool = &solver.get_pool();
    }

    // Build ID map first
    for (const auto& pair : solver) {
        id_map.get_id(pair.first);
        for (const auto& succ : pair.second) {
            id_map.get_id(succ);
        }
    }

    // Create DSViz graph
    DSViz::Config config;
    config.node_style = "style=filled";
    config.graph_style = "rankdir=LR;";
    config.other = R"(  // Visual legend:
  // System states: circles (blue border)
  // Environment states: boxes (orange border)
  // Swin: light green fill | Ewin: light red fill
  // Sys moves: blue solid lines | Env moves: red dashed lines
)";

    DSViz::Dot dot(config);

    // Add nodes
    for (const auto& pair : id_map.to_id) {
        const GameState& state = pair.first;
        const std::string& id = pair.second;
        bool is_sys = (state.player == Player::System);

        StateClass cls = solver.get_classification(state);

        std::string shape = is_sys ? "circle" : "box";
        std::string fillcolor;
        if (cls == StateClass::Swin) fillcolor = "lightgreen";
        else if (cls == StateClass::Ewin) fillcolor = "lightcoral";
        else fillcolor = "lightgray";

        std::string color = is_sys ? "blue" : "orange";
        std::string label = id + "\\n" + to_string(cls);

        std::string attrs = fmt::format(
            "[shape={}, fillcolor={}, color={} label=\"{}\"]",
            shape, fillcolor, color, label
        );

        dot.addNode(id, attrs);
    }

    // Add edges (transitions)
    for (const auto& pair : solver) {
        const GameState& from = pair.first;
        const std::string from_id = id_map.get_id(from);
        bool from_is_sys = (from.player == Player::System);

        for (const auto& succ : pair.second) {
            const std::string to_id = id_map.get_id(succ);

            std::string attrs;
            if (from_is_sys) {
                attrs = "[color=blue, style=solid";
                if (succ.system_chosen_output.has_value() && from.dfa_state) {
                    attrs += fmt::format(", label=\"sys={}\"",
                        id_map.get_assignment_label(succ.system_chosen_output.value(), true,
                                                  &from.dfa_state->prop_atoms())
                    );
                }
                attrs += "]";
            } else {
                attrs = "[color=red, style=dashed";
                if (succ.environment_chosen_input.has_value() && from.dfa_state) {
                    attrs += fmt::format(", label=\"env={}\"",
                        id_map.get_assignment_label(succ.environment_chosen_input.value(), false,
                                                  &from.dfa_state->prop_atoms())
                    );
                }
                attrs += "]";
            }

            dot.addEdge(from_id, to_id, attrs);
        }
    }

    return dot.print();
}

//==============================================================================
// Public Helper Functions
//==============================================================================

std::string get_trace_output_path() {
    namespace fs = std::filesystem;

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);

    // Generate date string: YYYY-MM-DD
    char date_buf[16];
    std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tm);

    // Build path: output/results/trace/YYYY-MM-DD/
    // Matches the game_graph directory structure
    std::ostringstream path;
    path << "output/results/trace/" << date_buf;

    return path.str();
}

//==============================================================================
// TraceExporter Implementation
//==============================================================================

TraceExporter::TraceExporter(formula::Formula* formula,
                             formula::FormulaPool& pool,
                             const std::string& output_dir)
    : formula_(formula),
      pool_(pool),
      output_dir_(output_dir),
      output_path_(),
      enabled_(true),
      finalized_(false),
      stage_counter_(0),
      step_counter_(0),
      current_stage_index_(-1)
{
    start_time_ = std::chrono::steady_clock::now();

    // Create output directory
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(output_dir_, ec);
    if (ec) {
        LOG_WARN("Failed to create trace output directory: ", output_dir_);
        enabled_ = false;
        return;
    }

    // Generate output file path with timestamp
    // Format: trace_YYYYMMDD_HHMMSS.json
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_r(&time_t_now, &tm_now);

    char time_buf[64];
    std::strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", &tm_now);
    std::string timestamp = time_buf;

    std::ostringstream oss;
    oss << output_dir_ << "/trace_" << timestamp << ".json";
    output_path_ = oss.str();

    LOG_DEBUG("TraceExporter: initialized, output: {}", output_path_);
}

TraceExporter::~TraceExporter() {
    if (!finalized_ && enabled_) {
        finalize(false);
    }
}

//==============================================================================
// Stage Management
//==============================================================================

void TraceExporter::begin_stage(const std::string& stage_type,
                                const std::string& description) {
    if (!enabled_) return;

    // End previous stage if any
    if (current_stage_index_ >= 0) {
        end_stage();
    }

    TraceStage stage;
    stage.stage_id = generate_stage_id();
    stage.stage_type = stage_type;
    stage.description = description;

    stages_.push_back(std::move(stage));
    current_stage_index_ = static_cast<int>(stages_.size()) - 1;
    stage_start_time_ = std::chrono::steady_clock::now();

    LOG_DEBUG("TraceExporter: began stage {} ({})", stage_counter_, stage_type);
}

void TraceExporter::end_stage() {
    if (!enabled_ || current_stage_index_ < 0) return;

    auto end_time = std::chrono::steady_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(
        end_time - stage_start_time_).count();

    LOG_DEBUG("TraceExporter: ended stage {} ({}), duration: {}ms", current_stage_index_,
              stages_[current_stage_index_].stage_type,
              duration_ms);

    current_stage_index_ = -1;
}

//==============================================================================
// Sub-Step Management
//==============================================================================

void TraceExporter::begin_sub_step(const std::string& description) {
    // All validation checks done once here
    if (!enabled_ || current_stage_index_ < 0) return;

    TraceStage& stage = stages_[current_stage_index_];

    // Create new SubStep
    stage.sub_steps.emplace_back();
    SubStep& step = stage.sub_steps.back();
    step.step_id = generate_step_id();
    step.description = description;
    step.metrics.duration_ms = 0;

    // Cache the pointer for subsequent operations
    current_sub_step_ = &step;
    step_start_time_ = std::chrono::steady_clock::now();
}

void TraceExporter::end_sub_step(bool include_graph) {
    (void)include_graph;  // Reserved for future graph export option
    if (!enabled_) return;

    // Record duration using cached pointer
    if (current_sub_step_) {
        auto end_time = std::chrono::steady_clock::now();
        current_sub_step_->metrics.duration_ms = std::chrono::duration<double, std::milli>(
            end_time - step_start_time_).count();
    }

    // Clear the cached pointer
    current_sub_step_ = nullptr;

    // Note: Graph data is typically set via set_graph_dot() or capture_state()
    // before calling end_sub_step()

    // Get the description for logging (need to access the step again)
    if (enabled_ && current_stage_index_ >= 0) {
        TraceStage& stage = stages_[current_stage_index_];
        if (!stage.sub_steps.empty()) {
            const SubStep& step = stage.sub_steps.back();
            LOG_DEBUG("TraceExporter: ended step {} ({}), duration: {}ms", step_counter_,
                      step.description, step.metrics.duration_ms);
        }
    }
}

//==============================================================================
// Data Recording
//==============================================================================

void TraceExporter::set_graph_dot(const std::string& dot,
                                   size_t num_nodes,
                                   size_t num_edges) {
    if (current_sub_step_) {
        current_sub_step_->graph_data.dot = dot;
        current_sub_step_->graph_data.num_nodes = num_nodes;
        current_sub_step_->graph_data.num_edges = num_edges;
    }
}

void TraceExporter::add_highlight_node(const std::string& node_id,
                                       HighlightType type) {
    if (current_sub_step_) {
        switch (type) {
            case HighlightType::NewNode:
                current_sub_step_->highlights.new_nodes.push_back(node_id);
                break;
            case HighlightType::SCCNode:
                current_sub_step_->highlights.scc_nodes.push_back(node_id);
                break;
            case HighlightType::PendingNode:
                current_sub_step_->highlights.pending_nodes.push_back(node_id);
                break;
            case HighlightType::UpdatedNode:
                current_sub_step_->highlights.updated_nodes.push_back(node_id);
                break;
            case HighlightType::AttractorNode:
                current_sub_step_->highlights.attractor_nodes.push_back(node_id);
                break;
            default:
                break;
        }
    }
}

void TraceExporter::add_highlight_nodes(const std::vector<std::string>& node_ids,
                                        HighlightType type) {
    for (const auto& id : node_ids) {
        add_highlight_node(id, type);
    }
}

void TraceExporter::add_highlight_edge(const std::string& from,
                                       const std::string& to,
                                       const std::string& label,
                                       const std::string& type) {
    if (current_sub_step_) {
        current_sub_step_->highlights.new_edges.emplace_back(from, to, label, type);
    }
}

void TraceExporter::set_state_info(int swin, int ewin, int unknown, int total) {
    if (current_sub_step_) {
        current_sub_step_->state_info.swin_count = swin;
        current_sub_step_->state_info.ewin_count = ewin;
        current_sub_step_->state_info.unknown_count = unknown;
        current_sub_step_->state_info.total_states = (total >= 0) ? total : (swin + ewin + unknown);
    }
}

void TraceExporter::set_scc_id(const std::string& scc_id) {
    if (current_sub_step_) {
        current_sub_step_->highlights.scc_id = scc_id;
    }
}

//==============================================================================
// Direct Recording Methods
//==============================================================================

void TraceExporter::capture_state(const std::string& description,
                                  const OnTheFlyGameSolver& solver,
                                  const SubStepHighlights* highlights) {
    if (!enabled_) return;

    begin_sub_step(description);

    // Get current DOT from solver (using shared StateIdMap)
    std::string dot = solver_to_dot(solver, id_map_);
    set_graph_dot(dot, solver.num_expanded_states(), 0);  // edges count not readily available

    // Collect state data for tooltips (phi, xnf_phi, prop_atoms)
    if (current_sub_step_) {
        collect_state_data(current_sub_step_->graph_data, solver);
    }

    // Get classification counts
    const auto& classification = solver.get_classification();
    int swin = 0, ewin = 0, unknown = 0;
    for (const auto& pair : classification) {
        if (pair.second == StateClass::Swin) swin++;
        else if (pair.second == StateClass::Ewin) ewin++;
        else unknown++;
    }
    set_state_info(swin, ewin, unknown, static_cast<int>(classification.size()));

    // Apply highlights if provided
    if (highlights) {
        for (const auto& id : highlights->new_nodes) {
            add_highlight_node(id, HighlightType::NewNode);
        }
        for (const auto& e : highlights->new_edges) {
            add_highlight_edge(e.from, e.to, e.label, e.type);
        }
        for (const auto& id : highlights->scc_nodes) {
            add_highlight_node(id, HighlightType::SCCNode);
        }
        for (const auto& id : highlights->pending_nodes) {
            add_highlight_node(id, HighlightType::PendingNode);
        }
        for (const auto& id : highlights->updated_nodes) {
            add_highlight_node(id, HighlightType::UpdatedNode);
        }
        for (const auto& id : highlights->attractor_nodes) {
            add_highlight_node(id, HighlightType::AttractorNode);
        }
        if (!highlights->scc_id.empty()) {
            set_scc_id(highlights->scc_id);
        }
    }

    end_sub_step(true);
}

void TraceExporter::record_expansion(const GameState& state,
                                     const std::vector<GameState>& successors,
                                     const OnTheFlyGameSolver& solver) {
    if (!enabled_) return;

    begin_sub_step("Expand state: " + id_map_.get_id(state));

    // Get current DOT
    std::string dot = solver_to_dot(solver, id_map_);
    set_graph_dot(dot, solver.num_expanded_states(), 0);

    // Collect state data for tooltips
    if (current_sub_step_) {
        collect_state_data(current_sub_step_->graph_data, solver);
    }

    // Highlight the expanded state and new successors
    add_highlight_node(id_map_.get_id(state), HighlightType::NewNode);

    // Mark successors as new
    for (const auto& succ : successors) {
        std::string succ_id = id_map_.get_id(succ);
        add_highlight_node(succ_id, HighlightType::NewNode);

        // Add edge highlight with label (show implicit false variables)
        std::string edge_type = (state.player == Player::System) ? "sys_move" : "env_move";
        std::string edge_label = format_assignment_label(state, succ);

        add_highlight_edge(id_map_.get_id(state), succ_id, edge_label, edge_type);
    }

    // Get classification counts
    const auto& classification = solver.get_classification();
    int swin = 0, ewin = 0, unknown = 0;
    for (const auto& pair : classification) {
        if (pair.second == StateClass::Swin) swin++;
        else if (pair.second == StateClass::Ewin) ewin++;
        else unknown++;
    }
    set_state_info(swin, ewin, unknown, static_cast<int>(classification.size()));

    end_sub_step(true);
}

void TraceExporter::record_scc(const std::vector<GameState>& scc,
                               const std::string& scc_id,
                               const OnTheFlyGameSolver& solver) {
    if (!enabled_) return;

    begin_sub_step("Found SCC: " + scc_id);

    // Get current DOT
    std::string dot = solver_to_dot(solver, id_map_);
    set_graph_dot(dot, solver.num_expanded_states(), 0);

    // Collect state data for tooltips
    if (current_sub_step_) {
        collect_state_data(current_sub_step_->graph_data, solver);
    }

    // Highlight SCC nodes
    std::vector<std::string> scc_node_ids;
    for (const auto& state : scc) {
        std::string state_id = id_map_.get_id(state);
        scc_node_ids.push_back(state_id);
    }
    add_highlight_nodes(scc_node_ids, HighlightType::SCCNode);
    set_scc_id(scc_id);

    // Get classification counts
    const auto& classification = solver.get_classification();
    int swin = 0, ewin = 0, unknown = 0;
    for (const auto& pair : classification) {
        if (pair.second == StateClass::Swin) swin++;
        else if (pair.second == StateClass::Ewin) ewin++;
        else unknown++;
    }
    set_state_info(swin, ewin, unknown, static_cast<int>(classification.size()));

    end_sub_step(true);
}

void TraceExporter::record_classification_change(const GameState& state,
                                                  StateClass old_class,
                                                  StateClass new_class,
                                                  const OnTheFlyGameSolver& solver) {
    if (!enabled_) return;

    std::ostringstream oss;
    oss << "Classification change: " << id_map_.get_id(state)
        << " from " << to_string(old_class) << " to " << to_string(new_class);

    begin_sub_step(oss.str());

    // Get current DOT
    std::string dot = solver_to_dot(solver, id_map_);
    set_graph_dot(dot, solver.num_expanded_states(), 0);

    // Collect state data for tooltips
    if (current_sub_step_) {
        collect_state_data(current_sub_step_->graph_data, solver);
    }

    // Highlight the changed state
    add_highlight_node(id_map_.get_id(state), HighlightType::UpdatedNode);

    // Get classification counts
    const auto& classification = solver.get_classification();
    int swin = 0, ewin = 0, unknown = 0;
    for (const auto& pair : classification) {
        if (pair.second == StateClass::Swin) swin++;
        else if (pair.second == StateClass::Ewin) ewin++;
        else unknown++;
    }
    set_state_info(swin, ewin, unknown, static_cast<int>(classification.size()));

    end_sub_step(true);
}

//==============================================================================
// Finalization
//==============================================================================

void TraceExporter::finalize(bool realizable) {
    if (!enabled_ || finalized_) return;

    // End any open stage
    if (current_stage_index_ >= 0) {
        end_stage();
    }

    // Build summary
    TraceSummary summary;
    summary.realizable = realizable;

    auto end_time = std::chrono::steady_clock::now();
    summary.duration_ms = std::chrono::duration<double, std::milli>(
        end_time - start_time_).count();

    // Count total steps and states
    for (const auto& stage : stages_) {
        summary.total_steps += static_cast<int>(stage.sub_steps.size());

        // Track stage types for summary
        bool found = false;
        for (auto& p : summary.stages_summary) {
            if (p.first == stage.stage_type) {
                p.second += static_cast<int>(stage.sub_steps.size());
                found = true;
                break;
            }
        }
        if (!found) {
            summary.stages_summary.push_back(
                {stage.stage_type, static_cast<int>(stage.sub_steps.size())});
        }
    }

    // Write JSON
    write_json();

    finalized_ = true;
    NOP_LOG_INFO("TraceExporter: finalized, trace written to {}", output_path_);
}

void TraceExporter::finalize(bool realizable, const OnTheFlyGameSolver& solver) {
    if (!enabled_ || finalized_) {
        // If already finalized or disabled, just call the base finalize
        finalize(realizable);
        return;
    }

    // End any open stage
    if (current_stage_index_ >= 0) {
        end_stage();
    }

    // Create a final summary stage to capture the complete final state
    begin_stage("final", "Final State");

    // Capture final state as a sub-step
    begin_sub_step("Complete game graph with final classifications");

    // Get current DOT from solver
    std::string dot = solver_to_dot(solver, id_map_);
    set_graph_dot(dot, solver.num_expanded_states(), 0);

    // Collect state data for tooltips
    if (current_sub_step_) {
        collect_state_data(current_sub_step_->graph_data, solver);
    }

    // Get classification counts
    const auto& classification = solver.get_classification();
    int swin = 0, ewin = 0, unknown = 0;
    for (const auto& pair : classification) {
        if (pair.second == StateClass::Swin) swin++;
        else if (pair.second == StateClass::Ewin) ewin++;
        else unknown++;
    }
    set_state_info(swin, ewin, unknown, static_cast<int>(classification.size()));

    end_sub_step(true);
    end_stage();

    // Now call the base finalize to write the JSON
    finalize(realizable);
}

//==============================================================================
// Helper Methods
//==============================================================================

std::string TraceExporter::generate_stage_id() {
    std::ostringstream oss;
    oss << "stage_" << std::setfill('0') << std::setw(3) << stage_counter_++;
    return oss.str();
}

std::string TraceExporter::generate_step_id() {
    std::ostringstream oss;
    oss << "step_" << std::setfill('0') << std::setw(3) << step_counter_++;
    return oss.str();
}

std::string TraceExporter::format_assignment_label(const GameState& state,
                                                    const GameState& succ) {
    const auto& var_names = pool_.get_all_variable_names();
    int num_outputs = pool_.num_outputs();
    bool is_sys_move = (state.player == Player::System);

    // Build set of true variables for quick lookup
    std::unordered_set<int> true_vars;
    if (is_sys_move && succ.system_chosen_output.has_value()) {
        true_vars.insert(succ.system_chosen_output.value().begin(),
                       succ.system_chosen_output.value().end());
    } else if (!is_sys_move && succ.environment_chosen_input.has_value()) {
        true_vars.insert(succ.environment_chosen_input.value().begin(),
                       succ.environment_chosen_input.value().end());
    }

    // Collect relevant variable indices from prop_atoms
    std::vector<int> relevant_vars;
    if (state.dfa_state) {
        for (const auto* phi : state.dfa_state->prop_atoms()) {
            if (phi && phi->op() == formula::Formula::OpType::Literal) {
                int var_id = phi->var_id();
                if (is_sys_move && var_id >= 0 && var_id < num_outputs) {
                    relevant_vars.push_back(var_id);
                } else if (!is_sys_move && var_id >= num_outputs &&
                          var_id < static_cast<int>(var_names.size())) {
                    relevant_vars.push_back(var_id);
                }
            }
        }
    }

    // Sort for consistent output
    std::sort(relevant_vars.begin(), relevant_vars.end());

    // Build label: show true vars as "var", false vars as "!var"
    std::vector<std::string> strs;
    strs.reserve(relevant_vars.size());
    for (int idx : relevant_vars) {
        if (true_vars.count(idx)) {
            strs.push_back(var_names[idx]);
        } else {
            strs.push_back(fmt::format("!{}", var_names[idx]));
        }
    }

    return fmt::format("{}{{{}}}", is_sys_move ? "sys=" : "env=", fmt::join(strs, ", "));
}

//==============================================================================
// JSON Writing (using nlohmann/json)
//==============================================================================

nlohmann::json TraceExporter::state_to_json(const StateData& data) const {
    nlohmann::json j;
    j["id"] = data.id;
    j["classification"] = to_string(data.classification);
    j["type"] = to_string(data.type);
    j["is_initial"] = data.is_initial;
    j["phi"] = data.phi;
    j["xnf_phi"] = data.xnf_phi;
    j["prop_atoms"] = data.prop_atoms;
    return j;
}

nlohmann::json TraceExporter::highlights_to_json(const SubStepHighlights& h) const {
    nlohmann::json j;

    if (!h.new_nodes.empty()) j["new_nodes"] = h.new_nodes;
    if (!h.scc_nodes.empty()) j["scc_nodes"] = h.scc_nodes;
    if (!h.pending_nodes.empty()) j["pending_nodes"] = h.pending_nodes;
    if (!h.updated_nodes.empty()) j["updated_nodes"] = h.updated_nodes;
    if (!h.attractor_nodes.empty()) j["attractor_nodes"] = h.attractor_nodes;
    if (!h.scc_id.empty()) j["scc_id"] = h.scc_id;

    if (!h.new_edges.empty()) {
        nlohmann::json edges = nlohmann::json::array();
        for (const auto& e : h.new_edges) {
            nlohmann::json edge;
            edge["from"] = e.from;
            edge["to"] = e.to;
            if (!e.label.empty()) edge["label"] = e.label;
            if (!e.type.empty()) edge["type"] = e.type;
            edges.push_back(edge);
        }
        j["new_edges"] = edges;
    }

    return j;
}

nlohmann::json TraceExporter::step_to_json(const SubStep& step) const {
    nlohmann::json j;
    j["step_id"] = step.step_id;
    j["description"] = step.description;

    // Graph data
    j["graph_data"]["dot"] = step.graph_data.dot;
    j["graph_data"]["num_nodes"] = step.graph_data.num_nodes;
    j["graph_data"]["num_edges"] = step.graph_data.num_edges;

    if (!step.graph_data.state_data.empty()) {
        nlohmann::json state_data;
        for (const auto& pair : step.graph_data.state_data) {
            state_data[pair.first] = state_to_json(pair.second);
        }
        j["graph_data"]["state_data"] = state_data;
    }

    // Highlights
    j["highlights"] = highlights_to_json(step.highlights);

    // State info
    j["state_info"]["swin_count"] = step.state_info.swin_count;
    j["state_info"]["ewin_count"] = step.state_info.ewin_count;
    j["state_info"]["unknown_count"] = step.state_info.unknown_count;
    j["state_info"]["total_states"] = step.state_info.total_states;
    if (step.state_info.current_scc >= 0) {
        j["state_info"]["current_scc"] = step.state_info.current_scc;
    }

    // Metrics
    j["metrics"]["duration_ms"] = step.metrics.duration_ms;
    if (step.metrics.memory_kb > 0) {
        j["metrics"]["memory_kb"] = step.metrics.memory_kb;
    }

    return j;
}

nlohmann::json TraceExporter::stage_to_json(const TraceStage& stage) const {
    nlohmann::json j;
    j["stage_id"] = stage.stage_id;
    j["stage_type"] = stage.stage_type;
    j["description"] = stage.description;

    nlohmann::json sub_steps = nlohmann::json::array();
    for (const auto& step : stage.sub_steps) {
        sub_steps.push_back(step_to_json(step));
    }
    j["sub_steps"] = sub_steps;

    return j;
}

nlohmann::json TraceExporter::summary_to_json() const {
    nlohmann::json j;
    j["total_steps"] = step_counter_;

    nlohmann::json stages_summary = nlohmann::json::array();
    for (const auto& stage : stages_) {
        nlohmann::json s;
        s["stage_type"] = stage.stage_type;
        s["steps_count"] = static_cast<int>(stage.sub_steps.size());
        stages_summary.push_back(s);
    }
    j["stages_summary"] = stages_summary;

    return j;
}

void TraceExporter::write_json() {
    nlohmann::json root;

    root["formula"] = formula_ ? formula_->to_string_with_names(pool_) : nullptr;
    root["timestamp"] = get_timestamp();

    auto [outputs, inputs] = pool_.get_variable_partition();
    root["partition"] = {{"outputs", outputs}, {"inputs", inputs}};

    nlohmann::json stages = nlohmann::json::array();
    for (const auto& stage : stages_) {
        stages.push_back(stage_to_json(stage));
    }
    root["stages"] = stages;
    root["summary"] = summary_to_json();

    std::ofstream out(output_path_);
    if (!out.is_open()) {
        LOG_ERROR("Failed to open trace file for writing: {}", output_path_);
        return;
    }
    out << root.dump(2) << std::endl;
}

//==============================================================================
// State Data Collection
//==============================================================================

void TraceExporter::collect_state_data(SubStepGraphData& graph_data,
                                       const OnTheFlyGameSolver& solver) {
    // Get the state information from solver
    const auto& successors = solver.get_all_successors();
    const auto& classification = solver.get_classification();
    const GameState& initial_state = solver.get_initial_state();

    // Build state data map - iterate only over keys (states), not values (successor lists)
    for (const GameState& state : iter::imap([](const auto& p) { return p.first; }, successors)) {
        std::string state_id = id_map_.get_id(state);

        StateData data;
        data.id = state_id;
        data.type = state.player;
        data.is_initial = (state == initial_state);

        // Get classification
        auto cls_it = classification.find(state);
        if (cls_it != classification.end()) {
            data.classification = cls_it->second;
        } else {
            data.classification = StateClass::Unknown;
        }

        // Get formula information from dfa_state
        if (state.dfa_state) {
            formula::Formula* phi = state.dfa_state->phi();
            data.phi = phi ? phi->to_string_with_names(pool_) : "null";

            formula::Formula* xnf_phi = state.dfa_state->xnf_phi();
            data.xnf_phi = xnf_phi ? xnf_phi->to_string_with_names(pool_) : "null";

            // Get propositional atoms
            const auto& prop_atoms = state.dfa_state->prop_atoms();
            for (auto* f : prop_atoms) {
                data.prop_atoms.push_back(f ? f->to_string_with_names(pool_) : "null");
            }
        } else {
            data.phi = "null";
            data.xnf_phi = "null";
        }

        graph_data.state_data[state_id] = std::move(data);
    }
}

} // namespace synthesis
