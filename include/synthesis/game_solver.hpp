#ifndef SYNTHESIS_GAME_SOLVER_HPP
#define SYNTHESIS_GAME_SOLVER_HPP

#include "automata/dfa.hpp"
#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <memory>

namespace synthesis {

/**
 * @brief State classification for game solving
 */
enum class StateStatus {
    Unknown,    // Not yet classified
    Winning,    // System can force a win (Swin)
    Losing      // Environment can force system loss (Ewin)
};

/**
 * @brief A node in the game graph
 *
 * Extends DFA state with game-theoretic information:
 * - Which player controls the transition (system vs environment)
 * - Classification as winning/losing
 */
struct GameNode {
    size_t dfa_state_id;      // Original DFA state ID
    StateStatus status = StateStatus::Unknown;
    int dfs_index = -1;       // For Tarjan SCC algorithm
    int low_link = -1;        // For Tarjan SCC algorithm
    bool on_stack = false;    // For Tarjan SCC algorithm
    std::vector<size_t> successors;  // Successor game nodes

    GameNode(size_t id) : dfa_state_id(id) {}
};

/**
 * @brief Game graph for LTLf synthesis
 *
 * The game graph is derived from the DFA:
 * - States: DFA states (augmented with player information)
 * - Transitions: Split into system (output) and environment (input) moves
 *
 * The game is a turn-based game:
 * 1. System chooses output variable values
 * 2. Environment chooses input variable values
 * 3. Next state is determined
 */
class GameGraph {
public:
    /**
     * @brief Construct a game graph from a DFA
     * @param dfa The DFA
     * @param output_vars Output variable names (system controls)
     * @param input_vars Input variable names (environment controls)
     * @param pool Formula pool (for variable lookups)
     */
    GameGraph(
        const automata::DFA* dfa,
        const std::vector<std::string>& output_vars,
        const std::vector<std::string>& input_vars,
        const formula::FormulaPool& pool
    );

    /**
     * @brief Get number of game nodes
     */
    size_t num_nodes() const { return nodes_.size(); }

    /**
     * @brief Get a game node
     */
    const GameNode& get_node(size_t id) const { return nodes_[id]; }

    /**
     * @brief Get a game node (mutable)
     */
    GameNode& get_node(size_t id) { return nodes_[id]; }

    /**
     * @brief Get initial game node
     */
    size_t get_initial_node() const { return initial_node_; }

    /**
     * @brief Check if initial node is winning
     */
    bool is_realizable() const;

    /**
     * @brief Check if a DFA state is accepting
     */
    bool is_dfa_accepting(size_t node_id) const;

    /**
     * @brief Debug print
     */
    void print() const;

private:
    std::vector<GameNode> nodes_;
    size_t initial_node_;
    std::unordered_map<size_t, size_t> dfa_to_game_;  // DFA state -> Game node mapping
    const automata::DFA* dfa_;  // Pointer to DFA for accepting state checks
};

/**
 * @brief Game solver using Tarjan SCC and backward propagation
 *
 * Algorithm:
 * 1. Build game graph from DFA
 * 2. Find SCCs using Tarjan algorithm
 * 3. Classify states as winning/losing using backward search
 * 4. A state is winning if:
 *    - It's accepting (contains 'end')
 *    - System can move to a winning state (for all env moves)
 *    - It's in a winning SCC (no dead-end outgoing transitions)
 */
class GameSolver {
public:
    /**
     * @brief Check if a formula is realizable
     * @param formula The LTLf formula
     * @param output_vars Output variable names
     * @param input_vars Input variable names
     * @param pool Formula pool
     * @return true if realizable, false if not, std::nullopt if unknown
     */
    static std::optional<bool> is_realizable(
        formula::Formula* formula,
        const std::vector<std::string>& output_vars,
        const std::vector<std::string>& input_vars,
        formula::FormulaPool& pool
    );

private:
    /**
     * @brief Find SCCs using Tarjan algorithm
     * @param graph The game graph
     * @return Vector of SCCs (each SCC is a vector of node IDs)
     */
    static std::vector<std::vector<size_t>> find_sccs(GameGraph& graph);

    /**
     * @brief Classify states as winning/losing
     * @param graph The game graph
     * @param sccs The SCCs from Tarjan
     */
    static void classify_states(GameGraph& graph, const std::vector<std::vector<size_t>>& sccs);

    /**
     * @brief Check if an SCC is accepting (winning for system)
     * @param graph The game graph
     * @param scc The SCC (set of node IDs)
     * @return true if the SCC is accepting
     */
    static bool is_scc_accepting(const GameGraph& graph, const std::vector<size_t>& scc);

    /**
     * @brief Propagate winning/losing status backward through the graph
     * @param graph The game graph
     */
    static void propagate_status(GameGraph& graph);
};

} // namespace synthesis

#endif // SYNTHESIS_GAME_SOLVER_HPP
