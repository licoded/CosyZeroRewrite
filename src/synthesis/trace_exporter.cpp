/**
 * @file trace_exporter.cpp
 * @brief Implementation of trace exporter for synthesis visualization
 */

#include "synthesis/trace_exporter.hpp"
#include "log/logger.hpp"
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

    // Generate timestamp for directory: YYYYMMDD_HHMMSS
    std::ostringstream ts;
    ts << std::setfill('0');
    ts << std::put_time(&tm, "%Y%m%d_%H%M%S");

    // Get period
    std::string period = get_period_string(tm.tm_hour);

    // Build path: results/trace_{timestamp}/HH-period/
    std::ostringstream path;
    path << "results/trace_" << ts.str() << "/" << period;

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
// JSON Writing
//==============================================================================

void TraceExporter::write_json() {
    std::ofstream out(output_path_);
    if (!out.is_open()) {
        LOG_ERROR("Failed to open trace file for writing: ", output_path_);
        return;
    }

    // Write opening
    out << "{\n";

    // Formula
    out << "  \"formula\": \"";
    if (formula_) {
        out << escape_json(formula_->to_string_with_names(pool_));
    }
    out << "\",\n";

    // Timestamp
    out << "  \"timestamp\": \"" << get_timestamp() << "\",\n";

    // Stages
    out << "  \"stages\": [\n";
    for (size_t i = 0; i < stages_.size(); ++i) {
        write_stage(out, stages_[i]);
        if (i < stages_.size() - 1) {
            out << ",";
        }
        out << "\n";
    }
    out << "  ],\n";

    // Summary
    out << "  \"summary\": {\n";
    out << "    \"total_steps\": " << step_counter_ << ",\n";
    out << "    \"realizable\": " << (finalized_ ? "true" : "false") << ",\n";
    out << "    \"stages_summary\": [";
    for (size_t i = 0; i < stages_.size(); ++i) {
        out << "\n      {\"stage_type\": \"" << stages_[i].stage_type
            << "\", \"steps_count\": " << stages_[i].sub_steps.size() << "}";
        if (i < stages_.size() - 1) out << ",";
    }
    if (!stages_.empty()) out << "\n    ";
    out << "]\n";

    out << "  }\n";
    out << "}\n";

    out.close();
}

void TraceExporter::write_stage(std::ofstream& out, const TraceStage& stage) const {
    out << "    {\n";
    out << "      \"stage_id\": \"" << stage.stage_id << "\",\n";
    out << "      \"stage_type\": \"" << stage.stage_type << "\",\n";
    out << "      \"description\": \"" << escape_json(stage.description) << "\",\n";
    out << "      \"sub_steps\": [";

    for (size_t i = 0; i < stage.sub_steps.size(); ++i) {
        out << "\n";
        write_sub_step(out, stage.sub_steps[i]);
        if (i < stage.sub_steps.size() - 1) {
            out << ",";
        }
    }

    if (!stage.sub_steps.empty()) {
        out << "\n    ";
    }
    out << "]\n";
    out << "    }";
}

void TraceExporter::write_sub_step(std::ofstream& out, const SubStep& step) const {
    out << "        {\n";
    out << "          \"step_id\": \"" << step.step_id << "\",\n";
    out << "          \"description\": \"" << escape_json(step.description) << "\",\n";

    // Graph data
    out << "          \"graph_data\": {\n";
    out << "            \"dot\": \"" << escape_json(step.graph_data.dot) << "\",\n";
    out << "            \"num_nodes\": " << step.graph_data.num_nodes << ",\n";
    out << "            \"num_edges\": " << step.graph_data.num_edges << ",\n";
    out << "            \"state_data\": {\n";

    // Write state_data map
    bool first_state = true;
    for (const auto& pair : step.graph_data.state_data) {
        if (!first_state) out << ",\n";
        first_state = false;

        const StateData& data = pair.second;
        out << "              \"" << data.id << "\": {\n";
        out << "                \"id\": \"" << data.id << "\",\n";
        out << "                \"classification\": \"" << data.classification << "\",\n";
        out << "                \"type\": \"" << data.type << "\",\n";
        out << "                \"is_initial\": " << (data.is_initial ? "true" : "false") << ",\n";
        out << "                \"phi\": \"" << escape_json(data.phi) << "\",\n";
        out << "                \"xnf_phi\": \"" << escape_json(data.xnf_phi) << "\",\n";

        // Prop atoms
        out << "                \"prop_atoms\": [";
        for (size_t i = 0; i < data.prop_atoms.size(); ++i) {
            if (i > 0) out << ", ";
            out << "\"" << escape_json(data.prop_atoms[i]) << "\"";
        }
        out << "]\n";

        out << "              }";
    }

    if (!step.graph_data.state_data.empty()) {
        out << "\n            ";
    }
    out << "          },\n";

    out << "          \"highlights\": {\n";
    write_highlights(out, step.highlights);
    out << "          },\n";

    // State info
    out << "          \"state_info\": {\n";
    write_state_info(out, step.state_info);
    out << "          },\n";

    // Metrics
    out << "          \"metrics\": {\n";
    out << "            \"duration_ms\": " << step.metrics.duration_ms << "\n";
    out << "          }\n";

    out << "        }";
}

void TraceExporter::write_highlights(std::ofstream& out, const SubStepHighlights& h) const {
    bool first = true;

    auto write_array = [&first, &out](const char* key, const auto& vec) {
        if (vec.empty()) return;
        if (!first) out << ",\n";
        first = false;
        out << "            \"" << key << "\": [";
        for (size_t i = 0; i < vec.size(); ++i) {
            out << "\"" << vec[i] << "\"";
            if (i < vec.size() - 1) out << ", ";
        }
        out << "]";
    };

    auto write_edge_array = [&first, &out](const char* key, const auto& vec) {
        if (vec.empty()) return;
        if (!first) out << ",\n";
        first = false;
        out << "            \"" << key << "\": [";
        for (size_t i = 0; i < vec.size(); ++i) {
            out << "\n              {";
            out << "\"from\": \"" << vec[i].from << "\", ";
            out << "\"to\": \"" << vec[i].to << "\"";
            if (!vec[i].label.empty()) out << ", \"label\": \"" << vec[i].label << "\"";
            if (!vec[i].type.empty()) out << ", \"type\": \"" << vec[i].type << "\"";
            out << "}";
            if (i < vec.size() - 1) out << ",";
        }
        if (!vec.empty()) out << "\n            ";
        out << "]";
    };

    write_array("new_nodes", h.new_nodes);
    write_edge_array("new_edges", h.new_edges);
    write_array("scc_nodes", h.scc_nodes);
    write_array("pending_nodes", h.pending_nodes);
    write_array("updated_nodes", h.updated_nodes);
    write_array("attractor_nodes", h.attractor_nodes);

    if (!h.scc_id.empty()) {
        if (!first) out << ",\n";
        out << "            \"scc_id\": \"" << h.scc_id << "\"";
    }

    if (!first) out << "\n";
}

void TraceExporter::write_state_info(std::ofstream& out, const SubStepStateInfo& info) const {
    out << "            \"swin_count\": " << info.swin_count << ",\n";
    out << "            \"ewin_count\": " << info.ewin_count << ",\n";
    out << "            \"unknown_count\": " << info.unknown_count << ",\n";
    out << "            \"total_states\": " << info.total_states;
    if (info.current_scc >= 0) {
        out << ",\n";
        out << "            \"current_scc\": " << info.current_scc;
    }
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
