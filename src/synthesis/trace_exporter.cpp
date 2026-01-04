/**
 * @file trace_exporter.cpp
 * @brief Implementation of trace exporter for synthesis visualization
 */

#include "synthesis/trace_exporter.hpp"
#include "log/logger.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <iomanip>
#include <sstream>

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

/**
 * @brief Get period string based on hour
 */
std::string get_period_string(int hour) {
    if (hour >= 6 && hour < 12) return "01-morning";
    if (hour >= 12 && hour < 18) return "02-afternoon";
    if (hour >= 18) return "03-evening";
    return "04-night";
}

/**
 * @brief Escape string for JSON
 */
std::string escape_json_string(const std::string& s) {
    std::string result;
    result.reserve(s.size() * 1.2);
    for (char c : s) {
        switch (c) {
            case '\\': result += "\\\\"; break;
            case '"':  result += "\\\""; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (c < 0x20) {
                    // Control characters
                    result += "\\u";
                    char buf[5];
                    snprintf(buf, sizeof(buf), "%04x", static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result += c;
                }
                break;
        }
    }
    return result;
}

} // anonymous namespace

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

    // Get period string (e.g., "01-morning", "02-afternoon", etc.)
    std::string period = get_period_string(tm.tm_hour);

    // Build path: results/trace/YYYY-MM-DD/HH-period/
    // Matches the game_graph directory structure
    std::ostringstream path;
    path << "results/trace/" << date_buf << "/" << period;

    return path.str();
}

std::string get_state_id(const GameState& state,
                         size_t& sys_count,
                         size_t& env_count) {
    if (state.player == Player::System) {
        return "S" + std::to_string(sys_count++);
    } else {
        return "E" + std::to_string(env_count++);
    }
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
      partition_(),
      stage_counter_(0),
      step_counter_(0),
      current_stage_index_(-1)
{
    // Extract partition information from FormulaPool
    // Variables are indexed: outputs (0 to num_outputs-1), inputs (num_outputs to num_outputs+num_inputs-1)
    const auto& all_vars = pool.get_all_variable_names();
    int num_outputs = pool.num_outputs();
    int num_inputs = pool.num_inputs();

    // Outputs are first
    for (int i = 0; i < num_outputs && i < static_cast<int>(all_vars.size()); ++i) {
        partition_.outputs.push_back(all_vars[i]);
    }

    // Inputs come after outputs
    for (int i = num_outputs; i < num_outputs + num_inputs && i < static_cast<int>(all_vars.size()); ++i) {
        partition_.inputs.push_back(all_vars[i]);
    }

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

    LOG_DEBUG("TraceExporter: initialized, output: ", output_path_);
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

    LOG_DEBUG("TraceExporter: began stage ", stage_counter_, " (", stage_type, ")");
}

void TraceExporter::end_stage() {
    if (!enabled_ || current_stage_index_ < 0) return;

    auto end_time = std::chrono::steady_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(
        end_time - stage_start_time_).count();

    LOG_DEBUG("TraceExporter: ended stage ", current_stage_index_,
              " (", stages_[current_stage_index_].stage_type,
              "), duration: ", duration_ms, "ms");

    current_stage_index_ = -1;
}

//==============================================================================
// Sub-Step Management
//==============================================================================

void TraceExporter::begin_sub_step(const std::string& description) {
    if (!enabled_ || current_stage_index_ < 0) return;

    SubStep step;
    step.step_id = generate_step_id();
    step.description = description;
    step.metrics.duration_ms = 0;  // Will be set on end

    stages_[current_stage_index_].sub_steps.push_back(std::move(step));
    step_start_time_ = std::chrono::steady_clock::now();
}

void TraceExporter::end_sub_step(bool include_graph) {
    if (!enabled_ || current_stage_index_ < 0) return;

    TraceStage& stage = stages_[current_stage_index_];
    if (stage.sub_steps.empty()) return;

    SubStep& step = stage.sub_steps.back();

    // Record duration
    auto end_time = std::chrono::steady_clock::now();
    step.metrics.duration_ms = std::chrono::duration<double, std::milli>(
        end_time - step_start_time_).count();

    // Note: Graph data is typically set via set_graph_dot() or capture_state()
    // before calling end_sub_step()

    LOG_DEBUG("TraceExporter: ended step ", step_counter_,
              " (", step.description, "), duration: ", step.metrics.duration_ms, "ms");
}

//==============================================================================
// Data Recording
//==============================================================================

void TraceExporter::set_graph_dot(const std::string& dot,
                                   size_t num_nodes,
                                   size_t num_edges) {
    if (!enabled_ || current_stage_index_ < 0) return;

    TraceStage& stage = stages_[current_stage_index_];
    if (stage.sub_steps.empty()) return;

    SubStep& step = stage.sub_steps.back();
    step.graph_data.dot = dot;
    step.graph_data.num_nodes = num_nodes;
    step.graph_data.num_edges = num_edges;
}

void TraceExporter::add_highlight_node(const std::string& node_id,
                                       HighlightType type) {
    if (!enabled_ || current_stage_index_ < 0) return;

    TraceStage& stage = stages_[current_stage_index_];
    if (stage.sub_steps.empty()) return;

    SubStep& step = stage.sub_steps.back();

    switch (type) {
        case HighlightType::NewNode:
            step.highlights.new_nodes.push_back(node_id);
            break;
        case HighlightType::SCCNode:
            step.highlights.scc_nodes.push_back(node_id);
            break;
        case HighlightType::PendingNode:
            step.highlights.pending_nodes.push_back(node_id);
            break;
        case HighlightType::UpdatedNode:
            step.highlights.updated_nodes.push_back(node_id);
            break;
        case HighlightType::AttractorNode:
            step.highlights.attractor_nodes.push_back(node_id);
            break;
        default:
            break;
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
    if (!enabled_ || current_stage_index_ < 0) return;

    TraceStage& stage = stages_[current_stage_index_];
    if (stage.sub_steps.empty()) return;

    SubStep& step = stage.sub_steps.back();
    step.highlights.new_edges.emplace_back(from, to, label, type);
}

void TraceExporter::set_state_info(int swin, int ewin, int unknown, int total) {
    if (!enabled_ || current_stage_index_ < 0) return;

    TraceStage& stage = stages_[current_stage_index_];
    if (stage.sub_steps.empty()) return;

    SubStep& step = stage.sub_steps.back();
    step.state_info.swin_count = swin;
    step.state_info.ewin_count = ewin;
    step.state_info.unknown_count = unknown;
    step.state_info.total_states = (total >= 0) ? total : (swin + ewin + unknown);
}

void TraceExporter::set_scc_id(const std::string& scc_id) {
    if (!enabled_ || current_stage_index_ < 0) return;

    TraceStage& stage = stages_[current_stage_index_];
    if (stage.sub_steps.empty()) return;

    SubStep& step = stage.sub_steps.back();
    step.highlights.scc_id = scc_id;
}

//==============================================================================
// Direct Recording Methods
//==============================================================================

void TraceExporter::capture_state(const std::string& description,
                                  const OnTheFlyGameSolver& solver,
                                  const SubStepHighlights* highlights) {
    if (!enabled_) return;

    begin_sub_step(description);

    // Get current DOT from solver
    std::string dot = solver.to_dot();
    set_graph_dot(dot, solver.num_expanded_states(), 0);  // edges count not readily available

    // Collect state data for tooltips (phi, xnf_phi, prop_atoms)
    TraceStage& stage = stages_[current_stage_index_];
    if (!stage.sub_steps.empty()) {
        collect_state_data(stage.sub_steps.back().graph_data, solver);
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
    std::string dot = solver.to_dot();
    set_graph_dot(dot, solver.num_expanded_states(), 0);

    // Collect state data for tooltips
    TraceStage& stage = stages_[current_stage_index_];
    if (!stage.sub_steps.empty()) {
        collect_state_data(stage.sub_steps.back().graph_data, solver);
    }

    // Highlight the expanded state and new successors
    add_highlight_node(id_map_.get_id(state), HighlightType::NewNode);

    // Mark successors as new
    for (const auto& succ : successors) {
        std::string succ_id = id_map_.get_id(succ);
        add_highlight_node(succ_id, HighlightType::NewNode);

        // Add edge highlight
        std::string edge_type = (state.player == Player::System) ? "sys_move" : "env_move";
        add_highlight_edge(id_map_.get_id(state), succ_id, "", edge_type);
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
    std::string dot = solver.to_dot();
    set_graph_dot(dot, solver.num_expanded_states(), 0);

    // Collect state data for tooltips
    TraceStage& stage = stages_[current_stage_index_];
    if (!stage.sub_steps.empty()) {
        collect_state_data(stage.sub_steps.back().graph_data, solver);
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
    std::string dot = solver.to_dot();
    set_graph_dot(dot, solver.num_expanded_states(), 0);

    // Collect state data for tooltips
    TraceStage& stage = stages_[current_stage_index_];
    if (!stage.sub_steps.empty()) {
        collect_state_data(stage.sub_steps.back().graph_data, solver);
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
    LOG_INFO("TraceExporter: finalized, trace written to {}", output_path_);
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
    std::string dot = solver.to_dot();
    set_graph_dot(dot, solver.num_expanded_states(), 0);

    // Collect state data for tooltips
    TraceStage& stage = stages_[current_stage_index_];
    if (!stage.sub_steps.empty()) {
        collect_state_data(stage.sub_steps.back().graph_data, solver);
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

std::string TraceExporter::escape_json(const std::string& s) const {
    return escape_json_string(s);
}

//==============================================================================
// JSON Writing (using nlohmann/json)
//==============================================================================

void TraceExporter::write_json() {
    using json = nlohmann::json;

    // Build root JSON object
    json root;

    // Formula
    if (formula_) {
        root["formula"] = formula_->to_string_with_names(pool_);
    } else {
        root["formula"] = nullptr;
    }

    // Timestamp
    root["timestamp"] = get_timestamp();

    // Partition (input/output variable names)
    if (!partition_.inputs.empty() || !partition_.outputs.empty()) {
        json partition_obj;
        partition_obj["inputs"] = partition_.inputs;
        partition_obj["outputs"] = partition_.outputs;
        root["partition"] = partition_obj;
    }

    // Build stages array
    json stages_array = json::array();
    for (const auto& stage : stages_) {
        json stage_obj;
        stage_obj["stage_id"] = stage.stage_id;
        stage_obj["stage_type"] = stage.stage_type;
        stage_obj["description"] = stage.description;

        // Build sub_steps array
        json sub_steps_array = json::array();
        for (const auto& step : stage.sub_steps) {
            json step_obj;
            step_obj["step_id"] = step.step_id;
            step_obj["description"] = step.description;

            // Graph data
            json graph_data_obj;
            graph_data_obj["dot"] = step.graph_data.dot;
            graph_data_obj["num_nodes"] = step.graph_data.num_nodes;
            graph_data_obj["num_edges"] = step.graph_data.num_edges;

            // Add state_data only if non-empty
            if (!step.graph_data.state_data.empty()) {
                json state_data_obj;
                for (const auto& pair : step.graph_data.state_data) {
                    const StateData& data = pair.second;
                    json data_obj;
                    data_obj["id"] = data.id;
                    data_obj["classification"] = data.classification;
                    data_obj["type"] = data.type;
                    data_obj["is_initial"] = data.is_initial;
                    data_obj["phi"] = data.phi;
                    data_obj["xnf_phi"] = data.xnf_phi;
                    data_obj["prop_atoms"] = data.prop_atoms;
                    state_data_obj[data.id] = data_obj;
                }
                graph_data_obj["state_data"] = state_data_obj;
            }
            step_obj["graph_data"] = graph_data_obj;

            // Highlights
            json highlights_obj;
            if (!step.highlights.new_nodes.empty()) {
                highlights_obj["new_nodes"] = step.highlights.new_nodes;
            }
            if (!step.highlights.new_edges.empty()) {
                json edges_array = json::array();
                for (const auto& e : step.highlights.new_edges) {
                    json edge_obj;
                    edge_obj["from"] = e.from;
                    edge_obj["to"] = e.to;
                    if (!e.label.empty()) edge_obj["label"] = e.label;
                    if (!e.type.empty()) edge_obj["type"] = e.type;
                    edges_array.push_back(edge_obj);
                }
                highlights_obj["new_edges"] = edges_array;
            }
            if (!step.highlights.scc_nodes.empty()) {
                highlights_obj["scc_nodes"] = step.highlights.scc_nodes;
            }
            if (!step.highlights.pending_nodes.empty()) {
                highlights_obj["pending_nodes"] = step.highlights.pending_nodes;
            }
            if (!step.highlights.updated_nodes.empty()) {
                highlights_obj["updated_nodes"] = step.highlights.updated_nodes;
            }
            if (!step.highlights.attractor_nodes.empty()) {
                highlights_obj["attractor_nodes"] = step.highlights.attractor_nodes;
            }
            if (!step.highlights.scc_id.empty()) {
                highlights_obj["scc_id"] = step.highlights.scc_id;
            }
            step_obj["highlights"] = highlights_obj;

            // State info
            json state_info_obj;
            state_info_obj["swin_count"] = step.state_info.swin_count;
            state_info_obj["ewin_count"] = step.state_info.ewin_count;
            state_info_obj["unknown_count"] = step.state_info.unknown_count;
            state_info_obj["total_states"] = step.state_info.total_states;
            if (step.state_info.current_scc >= 0) {
                state_info_obj["current_scc"] = step.state_info.current_scc;
            }
            step_obj["state_info"] = state_info_obj;

            // Metrics
            json metrics_obj;
            metrics_obj["duration_ms"] = step.metrics.duration_ms;
            if (step.metrics.memory_kb > 0) {
                metrics_obj["memory_kb"] = step.metrics.memory_kb;
            }
            step_obj["metrics"] = metrics_obj;

            sub_steps_array.push_back(step_obj);
        }
        stage_obj["sub_steps"] = sub_steps_array;
        stages_array.push_back(stage_obj);
    }
    root["stages"] = stages_array;

    // Summary
    json summary_obj;
    summary_obj["total_steps"] = step_counter_;
    summary_obj["realizable"] = finalized_;
    json stages_summary_array = json::array();
    for (const auto& stage : stages_) {
        json stage_summary;
        stage_summary["stage_type"] = stage.stage_type;
        stage_summary["steps_count"] = static_cast<int>(stage.sub_steps.size());
        stages_summary_array.push_back(stage_summary);
    }
    summary_obj["stages_summary"] = stages_summary_array;
    root["summary"] = summary_obj;

    // Write to file with pretty printing (4-space indent)
    std::ofstream out(output_path_);
    if (!out.is_open()) {
        LOG_ERROR("Failed to open trace file for writing: {}", output_path_);
        return;
    }
    out << root.dump(2) << std::endl;
    out.close();
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

    // Build state data map
    for (const auto& pair : successors) {
        const GameState& state = pair.first;
        std::string state_id = id_map_.get_id(state);

        StateData data;
        data.id = state_id;
        data.type = (state.player == Player::System) ? "System" : "Environment";
        data.is_initial = (state == initial_state);

        // Get classification
        auto cls_it = classification.find(state);
        if (cls_it != classification.end()) {
            data.classification = to_string(cls_it->second);
        } else {
            data.classification = "Unknown";
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
