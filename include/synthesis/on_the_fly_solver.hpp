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

#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace synthesis {

// Forward declarations (to avoid circular dependency)
class TraceExporter;
class BddManager;

/**
 * @brief Player in the synthesis game
 */
enum class Player
{
    System,     // System controls output variables
    Environment // Environment controls input variables
};

/**
 * @brief Classification of game states
 */
enum class StateClass
{
    Unknown, // Not yet classified
    Swin,    // System winning (system can force a win)
    Ewin,    // Environment winning (environment can force system loss)
    Draw     // Not applicable for LTLf (finite traces)
};

/**
 * @brief Convert StateClass to string
 */
inline const char *to_string(StateClass cls)
{
    switch (cls)
    {
        case StateClass::Unknown:
            return "Unknown";
        case StateClass::Swin:
            return "Swin";
        case StateClass::Ewin:
            return "Ewin";
        case StateClass::Draw:
            return "Draw";
    }
    return "?";
}

/**
 * @brief Convert Player to string
 */
inline const char *to_string(Player player)
{
    switch (player)
    {
        case Player::System:
            return "System";
        case Player::Environment:
            return "Environment";
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
 * - Input assignment chosen by environment (only valid during system's turn)
 *
 * Invariant:
 * - When player == System: system_chosen_output is nullopt, environment_chosen_input has value
 * - When player == Environment: system_chosen_output has value, environment_chosen_input is nullopt
 */
struct GameState {
    automata::TableauState *dfa_state;
    Player player;

    // For environment turn, track the output assignment chosen by system
    // nullopt when player == System, has value when player == Environment
    std::optional<automata::Assignment> system_chosen_output;

    // For system turn, track the input assignment chosen by environment
    // has value when player == System, nullopt when player == Environment
    std::optional<automata::Assignment> environment_chosen_input;

    // Default constructor (for uninitialized states)
    GameState()
        : dfa_state(nullptr),
          player(Player::System),
          system_chosen_output(std::nullopt),
          environment_chosen_input(std::nullopt)
    {
    }

    GameState(automata::TableauState *q,
              Player p,
              const std::optional<automata::Assignment> &out = std::nullopt,
              const std::optional<automata::Assignment> &in = std::nullopt)
        : dfa_state(q), player(p), system_chosen_output(out), environment_chosen_input(in)
    {
    }

    bool operator==(const GameState &other) const
    {
        // Note: environment_chosen_input is NOT part of identity
        // It's only stored for labeling the env move edge in DOT output
        return dfa_state == other.dfa_state && player == other.player
               && system_chosen_output == other.system_chosen_output;
    }

    bool operator!=(const GameState &other) const { return !(*this == other); }

    std::string to_string() const;
};

/**
 * @brief Hash function for GameState
 *
 * Note: environment_chosen_input is NOT part of the hash.
 * It's only stored for labeling the env move edge in DOT output.
 */
struct GameStateHash {
    size_t operator()(const GameState &s) const
    {
        size_t h = reinterpret_cast<size_t>(s.dfa_state);
        h ^= (static_cast<size_t>(s.player) << 1);
        // Hash the output assignment if present
        if (s.system_chosen_output.has_value())
        {
            for (int v : s.system_chosen_output.value())
            {
                h ^= std::hash<int> {}(v) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
        }
        // Note: environment_chosen_input is NOT hashed (not part of identity)
        return h;
    }
};

/**
 * @brief Equality function for GameState
 */
struct GameStateEqual {
    bool operator()(const GameState &a, const GameState &b) const { return a == b; }
};

/**
 * @brief Shared state ID mapping for DOT/JSON export
 *
 * This structure is used by both OnTheFlyGameSolver and TraceExporter
 * to ensure consistent state IDs across different output formats.
 * System states: S0, S1, S2, ...
 * Environment states: E0, E1, E2, ...
 */
struct StateIdMap {
    std::unordered_map<GameState, std::string, GameStateHash, GameStateEqual> to_id;
    std::unordered_map<std::string, GameState> from_id;
    size_t sys_count = 0;
    size_t env_count = 0;
    formula::FormulaPool *pool = nullptr; // For variable name lookup

    std::string get_id(const GameState &s)
    {
        auto it = to_id.find(s);
        if (it != to_id.end())
        {
            return it->second;
        }

        std::string id;
        if (s.player == Player::System)
        {
            id = "S" + std::to_string(sys_count++);
        }
        else
        {
            id = "E" + std::to_string(env_count++);
        }

        to_id[s] = id;
        from_id[id] = s;
        return id;
    }

    /**
     * @brief Get edge label in format "sys={p, !q}" or "env={!r, s}" (2026-01-04)
     *
     * Shows all relevant variables:
     * - For sys moves: shows all outputs that are in prop_atoms
     * - For env moves: shows all inputs that are in prop_atoms
     * - Variables set to TRUE are shown as "var"
     * - Variables set to FALSE (implicit) are shown as "!var"
     *
     * @param a Assignment (indices of variables set to TRUE)
     * @param is_output true for system moves (outputs), false for env moves (inputs)
     * @param prop_atoms Optional pointer to prop_atoms set to filter relevant variables
     * @return Label string with variable names
     */
    std::string get_assignment_label(const automata::Assignment &a,
                                     bool is_output,
                                     const automata::TableauState::FormulaSet *prop_atoms = nullptr) const
    {
        if (!pool)
            return "{}";

        const auto &var_names = pool->get_all_variable_names();
        int num_outputs = pool->num_outputs();

        // Build set of true variables for quick lookup
        std::unordered_set<int> true_vars(a.begin(), a.end());

        // Collect relevant variable indices
        std::vector<int> relevant_vars;

        if (prop_atoms && !prop_atoms->empty())
        {
            // Use prop_atoms to filter relevant variables
            for (const auto *phi : *prop_atoms)
            {
                if (phi && phi->op() == formula::Formula::OpType::Literal)
                {
                    int var_id = phi->var_id();
                    // Check if this variable belongs to the correct category
                    if (is_output && var_id >= 0 && var_id < num_outputs)
                    {
                        // Output variable for sys move
                        relevant_vars.push_back(var_id);
                    }
                    else if (!is_output && var_id >= num_outputs && var_id < static_cast<int>(var_names.size()))
                    {
                        // Input variable for env move
                        relevant_vars.push_back(var_id);
                    }
                }
            }
        }
        else
        {
            // Fallback: use all variables in the range (old behavior)
            int start_idx = is_output ? 0 : num_outputs;
            int end_idx = is_output ? num_outputs : static_cast<int>(var_names.size());
            for (int idx : a)
            {
                if (idx >= start_idx && idx < end_idx)
                {
                    relevant_vars.push_back(idx);
                }
            }
        }

        // Sort for consistent output
        std::sort(relevant_vars.begin(), relevant_vars.end());

        // Build label: show true vars as "var", false vars as "!var"
        std::ostringstream oss;
        oss << "{";

        bool first = true;
        for (int idx : relevant_vars)
        {
            if (!first)
                oss << ", ";
            if (true_vars.count(idx))
            {
                oss << var_names[idx]; // TRUE: just variable name
            }
            else
            {
                oss << "!" << var_names[idx]; // FALSE: !variable name
            }
            first = false;
        }

        oss << "}";
        return oss.str();
    }

    void clear()
    {
        to_id.clear();
        from_id.clear();
        sys_count = 0;
        env_count = 0;
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
class OnTheFlyGameSolver
{
  public:
    /**
     * @brief Construct solver
     * @param phi LTLf formula to synthesize
     * @param pool Formula pool
     * @param num_outputs Number of output variables
     * @param num_inputs Number of input variables
     */
    OnTheFlyGameSolver(formula::Formula *phi, formula::FormulaPool &pool);

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
    StateClass get_classification(const GameState &s) const
    {
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
    const automata::OnTheFlyDFA &dfa() const { return dfa_; }

    /**
     * @brief Get initial state
     */
    const GameState &get_initial_state() const { return initial_state_; }

    /**
     * @brief Get assignment generators (for strategy extraction)
     */
    const automata::AssignmentGenerator &get_output_generator() const { return output_gen_; }
    const automata::AssignmentGenerator &get_input_generator() const { return input_gen_; }

    /**
     * @brief Get successors of a state (for strategy extraction)
     * Returns nullptr if state not expanded
     */
    const std::vector<GameState> *get_successors(const GameState &state) const
    {
        auto it = successors_.find(state);
        return (it != successors_.end()) ? &it->second : nullptr;
    }

    /**
     * @brief Get all successors map (for strategy extraction)
     */
    const std::unordered_map<GameState, std::vector<GameState>, GameStateHash, GameStateEqual> &get_all_successors()
        const
    {
        return successors_;
    }

    /**
     * @brief Get classification map (for strategy extraction)
     */
    const std::unordered_map<GameState, StateClass, GameStateHash, GameStateEqual> &get_classification() const
    {
        return classification_;
    }

    /**
     * @brief Test interface: add a test transition (for unit testing SCC)
     * Allows building custom graph structures for testing the Tarjan algorithm.
     * Note: This appends to existing successors rather than replacing them.
     */
    void add_test_transition(const GameState &from, const std::vector<GameState> &to)
    {
        auto &existing = successors_[from];
        existing.insert(existing.end(), to.begin(), to.end());
    }

    /**
     * @brief Test interface: get all successors map
     */
    std::unordered_map<GameState, std::vector<GameState>, GameStateHash, GameStateEqual> &get_successors_map()
    {
        return successors_;
    }

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
     * @brief Enable execution trace recording for visualization
     *
     * When enabled, the solver will record each step of the synthesis process
     * for later visualization. Trace data is written to JSON format.
     *
     * @param output_dir Directory to write trace files (uses default if empty)
     */
    void enable_trace(const std::string &output_dir = "");

    /**
     * @brief Check if tracing is enabled
     */
    bool is_trace_enabled() const;

    /**
     * @brief Get the formula pool (for TraceExporter)
     */
    formula::FormulaPool &get_pool() const { return pool_; }

    //==========================================================================
    // Iterators for TraceExporter (internal use)
    //==========================================================================

    /**
     * @brief Iterator type for state-successor pairs
     */
    using const_iterator =
        typename std::unordered_map<GameState, std::vector<GameState>, GameStateHash, GameStateEqual>::const_iterator;

    /**
     * @brief Begin iterator over state->successors map
     */
    const_iterator begin() const { return successors_.begin(); }

    /**
     * @brief End iterator over state->successors map
     */
    const_iterator end() const { return successors_.end(); }

  private:
    // Formula and pool
    formula::FormulaPool &pool_;
    formula::Formula *original_formula_;

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

    // BDD manager for Safe System Move optimization (Rule B)
    std::unique_ptr<BddManager> bdd_manager_;

    // Flag to enable BDD safe move filtering
    bool enable_bdd_filtering_;

    /**
     * @brief Expand a game state (compute successors)
     */
    void expand_state(const GameState &state);

    /**
     * @brief Get successors of a state (computes if not cached)
     */
    const std::vector<GameState> &get_successors(const GameState &state);

    /**
     * @brief Get full-round successors (sys move + env move) from a System state
     *
     * Returns the set of System states reachable by a complete sys+env move:
     * { s' | ∃e. sys_state →[sys] e →[env] s' }
     *
     * This is used for SCC decomposition where only complete rounds (sys move
     * followed by env move) are considered as edges.
     *
     * @param sys_state A System state (asserts player == System)
     * @return Vector of System states after complete sys+env moves
     */
    std::vector<GameState> get_full_round_successors(const GameState &sys_state);

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
    bool is_empty_string_accepting(automata::TableauState *q) const;

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
    bool classify_scc(const std::vector<GameState> &scc);

    /**
     * @brief Check if initial state is classified
     */
    bool is_initial_classified() const
    {
        auto it = classification_.find(initial_state_);
        return it != classification_.end();
    }

    /**
     * @brief Get initial state classification
     */
    StateClass get_initial_classification() const
    {
        auto it = classification_.find(initial_state_);
        return it != classification_.end() ? it->second : StateClass::Unknown;
    }

    /**
     * @brief Create system turn state
     * @param q DFA state
     * @param in Input assignment chosen by environment (optional, nullopt for initial state)
     */
    GameState system_state(automata::TableauState *q, const std::optional<automata::Assignment> &in = std::nullopt)
    {
        return GameState(q, Player::System, std::nullopt, in);
    }

    /**
     * @brief Create environment turn state with output
     */
    GameState environment_state(automata::TableauState *q, const automata::Assignment &out)
    {
        return GameState(q, Player::Environment, out);
    }

    //==========================================================================
    // Private helper methods for is_realizable() phases
    //==========================================================================

    /**
     * @brief Phase 1: Expand all reachable states
     * @return Number of states expanded
     */
    size_t expand_all_reachable_states();

    /**
     * @brief Phase 2: SCC decomposition and classification
     * @return Number of SCCs found
     */
    size_t decompose_and_classify_sccs();

    /**
     * @brief Log classification summary (debug)
     */
    void log_classification_summary() const;

    /**
     * @brief Check if debug propagation consistency is enabled
     */
    bool is_consistency_check_enabled() const;

    /**
     * @brief Run propagation consistency check (if enabled) and report violations
     */
    void run_consistency_check_if_enabled() const;
};

} // namespace synthesis

#endif // SYNTHESIS_ON_THE_FLY_SOLVER_HPP
