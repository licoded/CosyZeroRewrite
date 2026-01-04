/**
 * @file trace_exporter.hpp
 * @brief Trace exporter for LTLf synthesis execution visualization
 *
 * Records the execution process of the on-the-fly synthesis algorithm,
 * including state expansion, SCC detection, and fixed-point iteration.
 * Outputs trace data in JSON format for web visualization.
 */

#ifndef SYNTHESIS_TRACE_EXPORTER_HPP
#define SYNTHESIS_TRACE_EXPORTER_HPP

#include "synthesis/on_the_fly_solver.hpp"
#include "formula/formula_pool.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <chrono>
#include <functional>

namespace synthesis {

//==============================================================================
// Trace Data Structures
//==============================================================================

/**
 * @brief Type of trace highlight
 */
enum class HighlightType {
    NewNode,       // Newly added node
    NewEdge,       // Newly added edge
    SCCNode,       // Node in current SCC
    PendingNode,   // Node pending processing
    UpdatedNode,   // Node whose classification changed
    AttractorNode  // Node in attractor set
};

/**
 * @brief Edge representation for highlights
 */
struct TraceEdge {
    std::string from;
    std::string to;
    std::string label;
    std::string type;  // "sys_move" or "env_move"

    TraceEdge() = default;
    TraceEdge(const std::string& f, const std::string& t,
              const std::string& l = "", const std::string& tp = "")
        : from(f), to(t), label(l), type(tp) {}
};

/**
 * @brief Highlight information for a sub-step
 */
struct SubStepHighlights {
    std::vector<std::string> new_nodes;
    std::vector<TraceEdge> new_edges;
    std::vector<std::string> scc_nodes;
    std::vector<std::string> pending_nodes;
    std::vector<std::string> updated_nodes;
    std::vector<std::string> attractor_nodes;
    std::string scc_id;  // Optional SCC identifier
};

/**
 * @brief State information for a sub-step
 */
struct SubStepStateInfo {
    int swin_count = 0;
    int ewin_count = 0;
    int unknown_count = 0;
    int total_states = 0;
    int current_scc = -1;
};

/**
 * @brief Metrics for a sub-step
 */
struct SubStepMetrics {
    double duration_ms = 0.0;
    size_t memory_kb = 0;
};

/**
 * @brief Detailed state data for tooltip display
 * Matches the format used in game_graph HTML visualization
 */
struct StateData {
    std::string id;                  // e.g., "S0", "E1"
    std::string classification;      // "Swin", "Ewin", "Unknown"
    std::string type;                // "System" or "Environment"
    bool is_initial = false;
    std::string phi;                 // Formula string
    std::string xnf_phi;             // XNF Formula string
    std::vector<std::string> prop_atoms;  // Propositional atoms
};

/**
 * @brief Graph data (DOT string)
 */
struct SubStepGraphData {
    std::string dot;
    size_t num_nodes = 0;
    size_t num_edges = 0;
    std::unordered_map<std::string, StateData> state_data;  // state_id -> StateData
};

/**
 * @brief A single sub-step in the trace
 */
struct SubStep {
    std::string step_id;              // e.g., "step_001"
    std::string description;
    SubStepGraphData graph_data;
    SubStepHighlights highlights;
    SubStepStateInfo state_info;
    SubStepMetrics metrics;
};

/**
 * @brief A stage in the trace (expand, scc, fixed_point, etc.)
 */
struct TraceStage {
    std::string stage_id;             // e.g., "stage_000"
    std::string stage_type;           // "expand", "scc", "fixed_point", "attractor"
    std::string description;
    std::vector<SubStep> sub_steps;
};

/**
 * @brief Summary statistics for the entire trace
 */
struct TraceSummary {
    int total_steps = 0;
    int total_states = 0;
    int total_sccs = 0;
    bool realizable = false;
    double duration_ms = 0.0;
    std::vector<std::pair<std::string, int>> stages_summary;  // (type, count)
};

/**
 * @brief Variable partition information (input/output variable names)
 * Used by frontend to format edge labels and assignment formulas
 */
struct TracePartition {
    std::vector<std::string> inputs;   // Input variable names
    std::vector<std::string> outputs;  // Output variable names
};

//==============================================================================
// Trace Exporter
//==============================================================================

/**
 * @brief Exports synthesis execution trace for visualization
 *
 * The TraceExporter records key moments during synthesis:
 * - State expansion (each new state added)
 * - SCC discovery (each SCC found)
 * - Fixed-point iteration (classification propagation)
 *
 * Output format: JSON file conforming to the schema in
 * docs/TRACE_VISUALIZATION/json_schema.md
 */
class TraceExporter {
public:
    /**
     * @brief Construct a trace exporter
     *
     * @param formula The original LTLf formula
     * @param pool Formula pool for string conversion
     * @param output_dir Directory to write trace files
     */
    TraceExporter(formula::Formula* formula,
                  formula::FormulaPool& pool,
                  const std::string& output_dir);

    /**
     * @brief Destructor - finalizes and writes the trace file
     */
    ~TraceExporter();

    // Disable copy/move
    TraceExporter(const TraceExporter&) = delete;
    TraceExporter& operator=(const TraceExporter&) = delete;
    TraceExporter(TraceExporter&&) = delete;
    TraceExporter& operator=(TraceExporter&&) = delete;

    //==========================================================================
    // Stage Management
    //==========================================================================

    /**
     * @brief Begin a new stage
     * @param stage_type Type of stage ("expand", "scc", "fixed_point", "attractor")
     * @param description Human-readable description
     */
    void begin_stage(const std::string& stage_type,
                     const std::string& description = "");

    /**
     * @brief End the current stage
     */
    void end_stage();

    //==========================================================================
    // Sub-Step Management
    //==========================================================================

    /**
     * @brief Begin a new sub-step within the current stage
     * @param description Human-readable description
     */
    void begin_sub_step(const std::string& description = "");

    /**
     * @brief End the current sub-step
     * @param include_graph Whether to include current graph state
     */
    void end_sub_step(bool include_graph = true);

    //==========================================================================
    // Data Recording
    //==========================================================================

    /**
     * @brief Set the DOT graph data for the current sub-step
     *
     * This is typically called by end_sub_step() if include_graph is true.
     *
     * @param dot The complete DOT string
     * @param num_nodes Number of nodes in the graph
     * @param num_edges Number of edges in the graph
     */
    void set_graph_dot(const std::string& dot,
                       size_t num_nodes = 0,
                       size_t num_edges = 0);

    /**
     * @brief Add a highlighted node
     * @param node_id State ID string (e.g., "S0", "E1")
     * @param type Type of highlight
     */
    void add_highlight_node(const std::string& node_id,
                            HighlightType type);

    /**
     * @brief Add multiple highlighted nodes
     * @param node_ids List of state IDs
     * @param type Type of highlight
     */
    void add_highlight_nodes(const std::vector<std::string>& node_ids,
                             HighlightType type);

    /**
     * @brief Add a highlighted edge
     * @param from Source state ID
     * @param to Target state ID
     * @param label Edge label (output assignment)
     * @param type "sys_move" or "env_move"
     */
    void add_highlight_edge(const std::string& from,
                            const std::string& to,
                            const std::string& label = "",
                            const std::string& type = "");

    /**
     * @brief Set state info for the current sub-step
     */
    void set_state_info(int swin, int ewin, int unknown, int total = -1);

    /**
     * @brief Set the current SCC ID for highlighting
     */
    void set_scc_id(const std::string& scc_id);

    //==========================================================================
    // Direct Recording Methods (convenience wrappers)
    //==========================================================================

    /**
     * @brief Capture current solver state as a sub-step
     *
     * This is a convenience method that:
     * 1. Begins a new sub-step
     * 2. Captures the solver's current state (DOT, classifications)
     * 3. Ends the sub-step
     *
     * @param description Sub-step description
     * @param solver Reference to the solver
     * @param highlights Optional highlights to apply
     */
    void capture_state(const std::string& description,
                       const OnTheFlyGameSolver& solver,
                       const SubStepHighlights* highlights = nullptr);

    /**
     * @brief Record state expansion
     *
     * Call this after expanding a new state.
     *
     * @param state The expanded state
     * @param successors Successors of the expanded state
     * @param solver Reference to the solver
     */
    void record_expansion(const GameState& state,
                          const std::vector<GameState>& successors,
                          const OnTheFlyGameSolver& solver);

    /**
     * @brief Record SCC discovery
     *
     * Call this when a new SCC is found.
     *
     * @param scc The SCC (list of states)
     * @param scc_id SCC identifier
     * @param solver Reference to the solver
     */
    void record_scc(const std::vector<GameState>& scc,
                    const std::string& scc_id,
                    const OnTheFlyGameSolver& solver);

    /**
     * @brief Record classification change
     *
     * Call this when a state's classification changes.
     *
     * @param state The state whose classification changed
     * @param old_class Previous classification
     * @param new_class New classification
     * @param solver Reference to the solver
     */
    void record_classification_change(const GameState& state,
                                       StateClass old_class,
                                       StateClass new_class,
                                       const OnTheFlyGameSolver& solver);

    //==========================================================================
    // Finalization
    //==========================================================================

    /**
     * @brief Finalize the trace and write to file
     *
     * Called automatically by destructor, but can be called earlier.
     *
     * @param realizable Whether the formula is realizable
     */
    void finalize(bool realizable);

    /**
     * @brief Finalize the trace with final state capture
     *
     * Captures the final solver state as a sub-step before finalizing.
     *
     * @param realizable Whether the formula is realizable
     * @param solver Reference to the solver for capturing final state
     */
    void finalize(bool realizable, const OnTheFlyGameSolver& solver);

    /**
     * @brief Check if tracing is enabled
     */
    bool is_enabled() const { return enabled_; }

    /**
     * @brief Get the output file path
     */
    const std::string& output_path() const { return output_path_; }

private:
    // Configuration
    formula::Formula* formula_;
    formula::FormulaPool& pool_;
    std::string output_dir_;
    std::string output_path_;
    bool enabled_;
    bool finalized_;
    TracePartition partition_;  // Input/output variable names

    // Timing
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point stage_start_time_;
    std::chrono::steady_clock::time_point step_start_time_;

    // Current state
    std::vector<TraceStage> stages_;
    int stage_counter_;
    int step_counter_;
    int current_stage_index_;  // -1 if no current stage

    // State ID mapping (same as in game_graph_export.cpp)
    struct StateIdMap {
        std::unordered_map<GameState, std::string, GameStateHash, GameStateEqual> to_id;
        size_t sys_count = 0;
        size_t env_count = 0;

        std::string get_id(const GameState& s);
        void clear() {
            to_id.clear();
            sys_count = 0;
            env_count = 0;
        }
    };
    StateIdMap id_map_;

    // Helper methods
    std::string generate_stage_id();
    std::string generate_step_id();
    std::string escape_json(const std::string& s) const;

    /**
     * @brief Collect state data from solver for tooltip display
     * Populates the state_data map in graph_data with phi, xnf_phi, prop_atoms
     */
    void collect_state_data(SubStepGraphData& graph_data,
                           const OnTheFlyGameSolver& solver);

    // JSON writing (using nlohmann/json library)
    void write_json();
};

//==============================================================================
// State ID Mapping Helper (for trace recording)
//==============================================================================

/**
 * @brief Get state ID for a game state
 * Generates IDs like S0, S1, ... for system states
 *              and E0, E1, ... for environment states
 */
inline std::string TraceExporter::StateIdMap::get_id(const GameState& s) {
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
    return id;
}

//==============================================================================
// Convenience Functions
//==============================================================================

/**
 * @brief Generate trace output directory path
 *
 * Creates path: results/trace_{timestamp}/HH-period/
 *
 * @return Generated path
 */
std::string get_trace_output_path();

/**
 * @brief Get state ID string from GameState
 * (Convenience function for external use)
 *
 * @param state The game state
 * @param sys_count Current system state count (reference)
 * @param env_count Current environment state count (reference)
 * @return State ID string
 */
std::string get_state_id(const GameState& state,
                         size_t& sys_count,
                         size_t& env_count);

} // namespace synthesis

#endif // SYNTHESIS_TRACE_EXPORTER_HPP
