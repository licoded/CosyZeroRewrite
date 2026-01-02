/**
 * @file strategy.hpp
 * @brief Strategy extraction and export for LTLf synthesis
 *
 * The strategy captures the winning moves for the system in each game state.
 * For realizability, the system can choose outputs such that no matter what
 * the environment does, the system can eventually force a win.
 */

#ifndef SYNTHESIS_STRATEGY_HPP
#define SYNTHESIS_STRATEGY_HPP

#include "on_the_fly_solver.hpp"
#include "formula/formula_pool.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <optional>

namespace synthesis {

//==============================================================================
// Strategy Data Structures
//==============================================================================

/**
 * @brief A move in the game
 */
struct StrategyMove {
    automata::Assignment output;      // Output assignment chosen by system
    std::vector<std::pair<automata::Assignment, std::string>> env_responses;
    // For each environment input choice: (assignment, next_state_id)

    std::string to_string() const;
};

/**
 * @brief Winning strategy for LTLf synthesis
 *
 * Maps each system turn game state to a winning output choice.
 * The strategy ensures that no matter what the environment does,
 * the system can force a win (reach accepting states).
 */
class Strategy {
public:
    /**
     * @brief Extract strategy from a solved game
     * @param solver The OnTheFlyGameSolver with completed classification
     * @return Strategy if formula is realizable, nullopt otherwise
     */
    static std::optional<Strategy> extract(const OnTheFlyGameSolver& solver);

    /**
     * @brief Get the winning move for a game state
     * @param state The game state (system turn)
     * @return The winning move, or nullopt if state not in strategy
     */
    std::optional<StrategyMove> get_move(const GameState& state) const;

    /**
     * @brief Check if strategy contains a move for this state
     */
    bool has_move(const GameState& state) const {
        return moves_.count(state) > 0;
    }

    /**
     * @brief Get number of states in strategy
     */
    size_t size() const { return moves_.size(); }

    /**
     * @brief Export strategy to JSON format
     * @param pool Formula pool for variable name lookup
     * @return JSON string
     */
    std::string to_json(const formula::FormulaPool& pool) const;

    /**
     * @brief Export strategy to Graphviz DOT format
     * @param pool Formula pool for variable name lookup
     * @return DOT string
     */
    std::string to_dot(const formula::FormulaPool& pool) const;

    /**
     * @brief Verify strategy correctness
     * @param phi Original formula
     * @param pool Formula pool
     * @return true if strategy is valid
     */
    bool verify(formula::Formula* phi, formula::FormulaPool& pool) const;

    /**
     * @brief Get initial state's winning move
     */
    std::optional<StrategyMove> get_initial_move() const {
        return get_move(initial_state_);
    }

private:
    // Mapping from system game states to winning moves
    std::unordered_map<GameState, StrategyMove, GameStateHash, GameStateEqual> moves_;

    // Initial state
    GameState initial_state_;

    // Private constructor - use extract() factory
    Strategy(const GameState& init) : initial_state_(init) {}

    // Add a move to the strategy
    void add_move(const GameState& state, const StrategyMove& move) {
        moves_[state] = move;
    }

    friend class StrategyExtractor;
    friend class StrategyVerifier;
};

//==============================================================================
// Strategy Extraction Algorithm
//==============================================================================

/**
 * @brief Extracts winning strategy from classified game graph
 */
class StrategyExtractor {
    friend class Strategy;  // Allow Strategy to access helper methods

public:
    /**
     * @brief Extract strategy from solver
     * @param solver The solved OnTheFlyGameSolver
     * @return Strategy if realizable, nullopt otherwise
     */
    static std::optional<Strategy> extract(const OnTheFlyGameSolver& solver);

private:
    /**
     * @brief Find winning output for a system state
     *
     * A system state is winning if there exists an output assignment such that
     * for all environment inputs, the successor is winning.
     */
    static std::optional<automata::Assignment> find_winning_output(
        const GameState& sys_state,
        const OnTheFlyGameSolver& solver
    );

    /**
     * @brief Generate unique ID for a game state
     */
    static std::string state_to_id(const GameState& state);

    /**
     * @brief Convert assignment to variable names
     */
    static std::string assignment_to_string(
        const automata::Assignment& assignment,
        const formula::FormulaPool& pool
    );
};

//==============================================================================
// Strategy Verification
//==============================================================================

/**
 * @brief Verify that a strategy is winning for the given formula
 */
class StrategyVerifier {
public:
    /**
     * @brief Verify strategy correctness
     * @param strategy The strategy to verify
     * @param phi Original formula
     * @param pool Formula pool
     * @return true if strategy is winning
     */
    static bool verify(
        const Strategy& strategy,
        formula::Formula* phi,
        formula::FormulaPool& pool
    );

private:
    /**
     * @brief Simulate one step of strategy execution
     */
    static bool simulate_step(
        const Strategy& strategy,
        const GameState& current,
        const automata::Assignment& env_input,
        formula::FormulaPool& pool,
        const automata::OnTheFlyDFA& dfa
    );
};

} // namespace synthesis

#endif // SYNTHESIS_STRATEGY_HPP
