#include "synthesis/game_solver.hpp"
#include <algorithm>
#include <stack>
#include <iostream>
#include <sstream>

namespace synthesis {

// ========== GameGraph ==========

GameGraph::GameGraph(
    const automata::DFA* dfa,
    const std::vector<std::string>& output_vars,
    const std::vector<std::string>& input_vars,
    const formula::FormulaPool& pool
) : dfa_(dfa) {
    if (!dfa) {
        throw std::invalid_argument("DFA cannot be null");
    }

    // Create game nodes for each DFA state
    size_t num_dfa_states = dfa->num_states();
    nodes_.reserve(num_dfa_states);

    for (size_t i = 0; i < num_dfa_states; ++i) {
        nodes_.emplace_back(i);
        dfa_to_game_[i] = i;
    }

    initial_node_ = dfa->get_initial_state();

    // Build successor relationships
    // For simplicity, we create direct transitions based on DFA
    // A full implementation would handle input/output separation
    for (size_t i = 0; i < num_dfa_states; ++i) {
        const auto& transitions = dfa->get_transitions(i);
        for (const auto& trans : transitions) {
            nodes_[i].successors.push_back(trans.to);
        }
    }

    // If a state has no transitions, add self-loop for terminal states
    for (size_t i = 0; i < num_dfa_states; ++i) {
        if (nodes_[i].successors.empty()) {
            nodes_[i].successors.push_back(i);
        }
    }

    // Suppress unused warnings
    (void)output_vars;
    (void)input_vars;
    (void)pool;
}

bool GameGraph::is_realizable() const {
    if (initial_node_ >= nodes_.size()) {
        return false;
    }
    return nodes_[initial_node_].status == StateStatus::Winning;
}

bool GameGraph::is_dfa_accepting(size_t node_id) const {
    if (!dfa_ || node_id >= nodes_.size()) {
        return false;
    }
    size_t dfa_state_id = nodes_[node_id].dfa_state_id;
    return dfa_->is_accepting(dfa_state_id);
}

void GameGraph::print() const {
    std::cout << "Game Graph with " << nodes_.size() << " nodes\n";
    std::cout << "Initial: " << initial_node_ << "\n";

    for (size_t i = 0; i < nodes_.size(); ++i) {
        const auto& node = nodes_[i];
        std::cout << "Node " << i << " (DFA state " << node.dfa_state_id << "): ";

        switch (node.status) {
            case StateStatus::Unknown: std::cout << "Unknown"; break;
            case StateStatus::Winning: std::cout << "Winning"; break;
            case StateStatus::Losing: std::cout << "Losing"; break;
        }
        std::cout << "\n";

        std::cout << "  Successors: ";
        for (auto succ : node.successors) {
            std::cout << succ << " ";
        }
        std::cout << "\n";
    }
}

// ========== GameSolver ==========

std::optional<bool> GameSolver::is_realizable(
    formula::Formula* formula,
    const std::vector<std::string>& output_vars,
    const std::vector<std::string>& input_vars,
    formula::FormulaPool& pool
) {
    try {
        // Step 1: Build DFA from formula
        auto dfa = automata::DFABuilder::build_from_formula(formula, pool);
        if (!dfa || dfa->num_states() == 0) {
            return std::nullopt;
        }

        // Step 2: Build game graph
        GameGraph graph(dfa.get(), output_vars, input_vars, pool);

        // Step 3: Find SCCs using Tarjan
        auto sccs = find_sccs(graph);

        // Step 4: Classify states
        classify_states(graph, sccs);

        // Step 5: Check realizability
        return graph.is_realizable();

    } catch (const std::exception& e) {
        std::cerr << "Error in is_realizable: " << e.what() << "\n";
        return std::nullopt;
    }
}

std::vector<std::vector<size_t>> GameSolver::find_sccs(GameGraph& graph) {
    std::vector<std::vector<size_t>> sccs;
    std::stack<size_t> stack;
    int index = 0;

    std::function<void(size_t)> strongconnect = [&](size_t v) {
        GameNode& node = graph.get_node(v);
        node.dfs_index = node.low_link = index++;
        stack.push(v);
        node.on_stack = true;

        // Consider successors
        for (size_t w : node.successors) {
            GameNode& succ = graph.get_node(w);
            if (succ.dfs_index == -1) {
                // Successor not visited
                strongconnect(w);
                node.low_link = std::min(node.low_link, succ.low_link);
            } else if (succ.on_stack) {
                // Successor is on stack, hence in current SCC
                node.low_link = std::min(node.low_link, succ.dfs_index);
            }
        }

        // If v is root of SCC
        if (node.low_link == node.dfs_index) {
            std::vector<size_t> scc;
            size_t w;
            do {
                w = stack.top();
                stack.pop();
                graph.get_node(w).on_stack = false;
                scc.push_back(w);
            } while (w != v);
            sccs.push_back(std::move(scc));
        }
    };

    // Initialize all nodes
    for (size_t i = 0; i < graph.num_nodes(); ++i) {
        GameNode& node = graph.get_node(i);
        node.dfs_index = -1;
        node.low_link = -1;
        node.on_stack = false;
    }

    // Find SCCs
    for (size_t i = 0; i < graph.num_nodes(); ++i) {
        if (graph.get_node(i).dfs_index == -1) {
            strongconnect(i);
        }
    }

    return sccs;
}

bool GameSolver::is_scc_accepting(const GameGraph& graph, const std::vector<size_t>& scc) {
    // An SCC is accepting if:
    // 1. All states in the SCC are accepting DFA states (can end the trace)
    // 2. For sink SCCs (no outgoing edges), the state must be accepting

    // Check if all states in SCC are DFA-accepting
    for (size_t node_id : scc) {
        if (!graph.is_dfa_accepting(node_id)) {
            return false;  // Contains a rejecting DFA state
        }
    }

    // A single-node SCC that is accepting in DFA is winning
    if (scc.size() == 1) {
        size_t node_id = scc[0];
        const GameNode& node = graph.get_node(node_id);

        // If it's a sink (only self-loop or no outgoing transitions)
        bool is_sink = true;
        for (size_t succ : node.successors) {
            if (succ != node_id) {
                is_sink = false;
                break;
            }
        }

        if (is_sink) {
            // Sink SCC is winning iff DFA state is accepting
            return graph.is_dfa_accepting(node_id);
        }
    }

    // Multi-node SCCs with accepting states are winning
    // (system can cycle forever)
    return true;
}

void GameSolver::classify_states(GameGraph& graph, const std::vector<std::vector<size_t>>& sccs) {
    // Reset all statuses to Unknown
    for (size_t i = 0; i < graph.num_nodes(); ++i) {
        graph.get_node(i).status = StateStatus::Unknown;
    }

    // First pass: mark SCCs as winning or losing
    for (const auto& scc : sccs) {
        bool accepting = is_scc_accepting(graph, scc);
        StateStatus status = accepting ? StateStatus::Winning : StateStatus::Losing;

        for (size_t node_id : scc) {
            graph.get_node(node_id).status = status;
        }
    }

    // Second pass: propagate status backward
    propagate_status(graph);
}

void GameSolver::propagate_status(GameGraph& graph) {
    // Work list algorithm for backward propagation
    std::vector<size_t> work_list;

    // Initialize work list with nodes that have successors
    for (size_t i = 0; i < graph.num_nodes(); ++i) {
        if (!graph.get_node(i).successors.empty()) {
            work_list.push_back(i);
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;

        for (size_t node_id : work_list) {
            GameNode& node = graph.get_node(node_id);

            // Skip if already classified
            if (node.status != StateStatus::Unknown) {
                continue;
            }

            // Check all successors
            bool all_winning = true;
            bool any_losing = false;

            for (size_t succ_id : node.successors) {
                const GameNode& succ = graph.get_node(succ_id);
                if (succ.status != StateStatus::Winning) {
                    all_winning = false;
                }
                if (succ.status == StateStatus::Losing) {
                    any_losing = true;
                }
            }

            // Classification rules:
            // - If all successors are winning, this node is winning
            // - If any successor is losing, this node might be winning
            //   (system can choose to move there)
            // This is simplified - full version needs to consider game semantics

            if (all_winning && !node.successors.empty()) {
                node.status = StateStatus::Winning;
                changed = true;
            } else if (node.successors.empty()) {
                // Dead end - typically losing
                node.status = StateStatus::Losing;
                changed = true;
            }
        }
    }

    // Any remaining unknown nodes are marked as losing (conservative)
    for (size_t i = 0; i < graph.num_nodes(); ++i) {
        if (graph.get_node(i).status == StateStatus::Unknown) {
            graph.get_node(i).status = StateStatus::Losing;
        }
    }
}

} // namespace synthesis
