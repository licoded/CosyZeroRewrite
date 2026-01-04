/**
 * @file on_the_fly_solver.hpp
 * @brief On-the-fly game solver for LTLf synthesis
 *
 * Based on: arXiv:2408.07324 - "On-the-fly Synthesis for LTL over Finite Traces"
 *
 * The solver constructs the game graph on-demand and uses SCC decomposition
 * to classify states as winning (Swin) or losing (Ewin) for the system.
 */

#ifndef SYNTHESIS_ON_THE_FLY_SOLVER_HPP
#define SYNTHESIS_ON_THE_FLY_SOLVER_HPP

#include "automata/tableau.hpp"
#include "formula/formula_pool.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <optional>
#include <memory>

namespace synthesis {

// Forward declaration (to avoid circular dependency)
class TraceExporter;

/**
 * @brief Player in the synthesis game
 */
enum class Player {
    System,      // System controls output variables
    Environment  // Environment controls input variables
};

/**
 * @brief Classification of game states
 */
enum class StateClass {
    Unknown,  // Not yet classified
    Swin,     // System winning (system can force a win)
    Ewin,     // Environment winning (environment can force system loss)
    Draw      // Not applicable for LTLf (finite traces)
};

/**
 * @brief Convert StateClass to string
 */
inline const char* to_string(StateClass cls) {
    switch (cls) {
        case StateClass::Unknown: return "Unknown";
        case StateClass::Swin: return "Swin";
        case StateClass::Ewin: return "Ewin";
        case StateClass::Draw: return "Draw";
    }
    return "?";
}

/**
 * @brief Game state in the synthesis game
 *
 * A game state consists of:
 * - DFA state (tableau state)
 * - Player to move
 * - Output assignment chosen by system (only valid during environment's turn)
 *
 * Invariant:
 * - When player == System: system_chosen_output is nullopt
 * - When player == Environment: system_chosen_output has a value
 */
struct GameState {
    automata::TableauState* dfa_state;
    Player player;

    // For environment turn, track the output assignment chosen by system
    // nullopt when player == System, has value when player == Environment
    std::optional<automata::Assignment> system_chosen_output;

    // Default constructor (for uninitialized states)
    GameState()
        : dfa_state(nullptr), player(Player::System), system_chosen_output(std::nullopt) {}

    GameState(automata::TableauState* q, Player p,
              const std::optional<automata::Assignment>& out = std::nullopt)
        : dfa_state(q), player(p), system_chosen_output(out) {}

    bool operator==(const GameState& other) const {
        return dfa_state == other.dfa_state &&
               player == other.player &&
               system_chosen_output == other.system_chosen_output;
    }

    bool operator!=(const GameState& other) const {
        return !(*this == other);
    }

    std::string to_string() const;
};

/**
 * @brief Hash function for GameState
 */
struct GameStateHash {
    size_t operator()(const GameState& s) const {
        size_t h = reinterpret_cast<size_t>(s.dfa_state);
        h ^= (static_cast<size_t>(s.player) << 1);
        // Hash the output assignment if present
        if (s.system_chosen_output.has_value()) {
            for (int v : s.system_chosen_output.value()) {
                h ^= std::hash<int>{}(v) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
        }
        return h;
    }
};

/**
 * @brief Equality function for GameState
 */
struct GameStateEqual {
    bool operator()(const GameState& a, const GameState& b) const {
        return a == b;
    }
};

/**
 * @brief On-the-fly game solver for LTLf synthesis
 *
 * Algorithm (from paper, Algorithm 2):
 * 1. Start from initial state (DFA initial, system's turn)
 * 2. Expand states on-demand, computing successors
 * 3. Run SCC decomposition on current subgraph
 * 4. Classify SCCs (Swin if contains accepting DFA state, Ewin otherwise)
 * 5. Propagate classification backward
 * 6. Early exit if initial state is classified
 */
class OnTheFlyGameSolver {
public:
    /**
     * @brief Construct solver
     * @param phi LTLf formula to synthesize
     * @param pool Formula pool
     * @param num_outputs Number of output variables
     * @param num_inputs Number of input variables
     */
    OnTheFlyGameSolver(formula::Formula* phi,
                       formula::FormulaPool& pool,
                       int num_outputs,
                       int num_inputs);

    /**
     * @brief Destructor (needed for unique_ptr<TraceExporter>)
     */
    ~OnTheFlyGameSolver();

    /**
     * @brief Run the on-the-fly synthesis algorithm
     * @return true if the formula is realizable
     */
    bool is_realizable();

    /**
     * @brief Get classification of a state
     */
    StateClass get_classification(const GameState& s) const {
        auto it = classification_.find(s);
        return it != classification_.end() ? it->second : StateClass::Unknown;
    }

    /**
     * @brief Get number of expanded states
     */
    size_t num_expanded_states() const { return successors_.size(); }

    /**
     * @brief Get number of SCCs found
     */
    size_t num_sccs_found() const { return num_sccs_found_; }

    /**
     * @brief Get the DFA
     */
    const automata::OnTheFlyDFA& dfa() const { return dfa_; }

    /**
     * @brief Get initial state
     */
    const GameState& get_initial_state() const { return initial_state_; }

    /**
     * @brief Get assignment generators (for strategy extraction)
     */
    const automata::AssignmentGenerator& get_output_generator() const {
        return output_gen_;
    }
    const automata::AssignmentGenerator& get_input_generator() const {
        return input_gen_;
    }

    /**
     * @brief Get successors of a state (for strategy extraction)
     * Returns nullptr if state not expanded
     */
    const std::vector<GameState>* get_successors(const GameState& state) const {
        auto it = successors_.find(state);
        return (it != successors_.end()) ? &it->second : nullptr;
    }

    /**
     * @brief Get all successors map (for strategy extraction)
     */
    const std::unordered_map<GameState, std::vector<GameState>, GameStateHash, GameStateEqual>&
    get_all_successors() const { return successors_; }

    /**
     * @brief Get classification map (for strategy extraction)
     */
    const std::unordered_map<GameState, StateClass, GameStateHash, GameStateEqual>&
    get_classification() const { return classification_; }

    /**
     * @brief Test interface: add a test transition (for unit testing SCC)
     * Allows building custom graph structures for testing the Tarjan algorithm.
     * Note: This appends to existing successors rather than replacing them.
     */
    void add_test_transition(const GameState& from, const std::vector<GameState>& to) {
        auto& existing = successors_[from];
        existing.insert(existing.end(), to.begin(), to.end());
    }

    /**
     * @brief Test interface: get all successors map
     */
    std::unordered_map<GameState, std::vector<GameState>, GameStateHash, GameStateEqual>&
    get_successors_map() { return successors_; }

    /**
     * @brief Test interface: run Tarjan SCC algorithm on current graph
     * Exposed for unit testing the Tarjan implementation.
     * This version only works with states explicitly added via add_test_transition()
     * and doesn't trigger expand_state() like the production find_sccs() does.
     */
    std::vector<std::vector<GameState>> find_sccs_for_testing();

    /**
     * @brief Debug: Check propagation logic consistency across all classified states
     *
     * Verifies that for every classified state:
     * - System state with Swin has at least one Swin successor
     * - System state with Ewin has all successors as Ewin
     * - Environment state with Swin has all successors as Swin
     * - Environment state with Ewin has at least one Ewin successor
     *
     * @return Number of violations found (0 = all consistent)
     */
    size_t check_propagation_consistency() const;

    /**
     * @brief Export game graph to DOT format (GraphViz)
     *
     * Visual elements:
     * - System states: circles, blue border
     * - Environment states: boxes, orange border
     * - Swin states: light green fill
     * - Ewin states: light red fill
     * - Initial state: bold border (penwidth=3)
     * - Sys moves: blue solid lines (System -> Environment)
     * - Env moves: red dashed lines (Environment -> System)
     *
     * @return DOT format string
     */
    std::string to_dot() const;

    /**
     * @brief Export game graph metadata to JSON format
     *
     * Contains complete formula information for each state:
     * - Formula strings (with variable names)
     * - State classifications
     * - Transition assignments
     *
     * @return JSON format string
     */
    std::string to_json() const;

    /**
     * @brief Export game graph to interactive HTML (single self-contained file)
     *
     * Generates an HTML file with:
     * - Embedded viz.js for graph rendering (loads from CDN)
     * - Embedded DOT content for the graph
     * - Embedded JSON data for state metadata
     * - Interactive tooltips showing formula information on hover
     * - Click-to-highlight functionality
     *
     * @return HTML content as string
     */
    std::string to_html() const;

    /**
     * @brief Write game graph to files (DOT + JSON + HTML)
     *
     * Creates three files:
     * - <base_path>.dot - GraphViz DOT format
     * - <base_path>.json - Metadata with full formulas
     * - <base_path>.html - Interactive HTML visualization
     *
     * The base_path should NOT include extension.
     * Example: write_dot("results/game_graph/2026-01-04/10-morning/game_graph_20260104_101500")
     *
     * @param base_path Output file path without extension
     * @return true if all files written successfully
     */
    bool write_dot(const std::string& base_path) const;

    /**
     * @brief Enable execution trace recording for visualization
     *
     * When enabled, the solver will record each step of the synthesis process
     * for later visualization. Trace data is written to JSON format.
     *
     * @param output_dir Directory to write trace files (uses default if empty)
     */
    void enable_trace(const std::string& output_dir = "");

    /**
     * @brief Check if tracing is enabled
     */
    bool is_trace_enabled() const;

private:
    // Formula and pool
    formula::FormulaPool& pool_;
    formula::Formula* original_formula_;

    // DFA and assignment generation
    automata::OnTheFlyDFA dfa_;
    automata::AssignmentGenerator output_gen_;
    automata::AssignmentGenerator input_gen_;

    // Game state classification
    std::unordered_map<GameState, StateClass, GameStateHash, GameStateEqual> classification_;

    // Successor map: state -> list of successors
    std::unordered_map<GameState, std::vector<GameState>, GameStateHash, GameStateEqual> successors_;

    // Worklist for expansion
    std::vector<GameState> worklist_;

    // Visited states (for expansion tracking)
    std::unordered_set<GameState, GameStateHash, GameStateEqual> expanded_;

    // Statistics
    size_t num_sccs_found_;

    // Initial state
    GameState initial_state_;

    // Trace exporter (for execution visualization)
    std::unique_ptr<TraceExporter> trace_exporter_;

    /**
     * @brief Expand a game state (compute successors)
     */
    void expand_state(const GameState& state);

    /**
     * @brief Get successors of a state (computes if not cached)
     */
    const std::vector<GameState>& get_successors(const GameState& state);

    /**
     * @brief Run Tarjan SCC algorithm on current graph
     * @return List of SCCs (each is a list of game states)
     */
    std::vector<std::vector<GameState>> find_sccs();

    /**
     * @brief Check if a TableauState can be satisfied by the empty string
     *
     * In LTLf, a state is empty-string accepting if:
     * 1. No U (Until) or X (Next) operators - these require future states
     * 2. Only R (Release) operators and non-temporal formulas are allowed
     * 3. The state is locally consistent (no contradictions like p & !p)
     *
     * @param q The TableauState to check
     * @return true if the state can be satisfied by the empty string
     */
    bool is_empty_string_accepting(automata::TableauState* q) const;

    /**
     * @brief Classify an SCC using fixed-point iteration
     *
     * Algorithm:
     * 1. Initialize seed set: empty-string accepting states are Swin
     * 2. Iterate: find predecessors of Swin states, classify new Swin
     * 3. Terminate: when no new Swin states found
     * 4. Remaining states: mark as Ewin
     *
     * @param scc The SCC to classify (list of states)
     * @return true if all states in SCC were classified
     */
    bool classify_scc(const std::vector<GameState>& scc);

    /**
     * @brief Check if initial state is classified
     */
    bool is_initial_classified() const {
        auto it = classification_.find(initial_state_);
        return it != classification_.end();
    }

    /**
     * @brief Get initial state classification
     */
    StateClass get_initial_classification() const {
        auto it = classification_.find(initial_state_);
        return it != classification_.end() ? it->second : StateClass::Unknown;
    }

    /**
     * @brief Create system turn state
     */
    GameState system_state(automata::TableauState* q) {
        return GameState(q, Player::System);
    }

    /**
     * @brief Create environment turn state with output
     */
    GameState environment_state(automata::TableauState* q,
                                const automata::Assignment& out) {
        return GameState(q, Player::Environment, out);
    }

    /**
     * @brief Count variables in formula
     */
    static int count_variables(formula::Formula* phi);
};

/**
 * @brief Convenience function: check if formula is realizable using on-the-fly solver
 *
 * @param phi LTLf formula
 * @param pool Formula pool (must have variables declared)
 * @return true if realizable
 */
bool is_realizable_on_the_fly(formula::Formula* phi,
                              formula::FormulaPool& pool);

} // namespace synthesis

#endif // SYNTHESIS_ON_THE_FLY_SOLVER_HPP
