#include "synthesis/game_solver.hpp"
#include <algorithm>
#include <stack>
#include <queue>
#include <iostream>
#include <sstream>

namespace synthesis {

// ========== GameGraph ==========

GameGraph::GameGraph(
    const automata::DFA* dfa,
    const std::vector<std::string>& output_vars,
    const std::vector<std::string>& input_vars,
    const formula::FormulaPool& pool
) {
    if (!dfa) {
        throw std::invalid_argument("DFA cannot be null");
    }

    // Create game nodes for each DFA state
    size_t num_dfa_states = dfa->num_states();
    nodes_.reserve(num_dfa_states);

    for (size_t i = 0; i < num_dfa_states; ++i) {
        nodes_.emplace_back(i);
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

std::vector<size_t> GameSolver::get_scc_processing_order(
    const GameGraph& graph,
    const std::vector<std::vector<size_t>>& sccs
) {
    if (sccs.empty()) {
        return {};
    }

    // Map node_id to its SCC index
    std::vector<size_t> node_to_scc(graph.num_nodes());
    for (size_t scc_idx = 0; scc_idx < sccs.size(); ++scc_idx) {
        for (size_t node_id : sccs[scc_idx]) {
            node_to_scc[node_id] = scc_idx;
        }
    }

    // Build SCC graph (edges between SCCs) and compute in-degrees
    std::vector<std::unordered_set<size_t>> scc_graph(sccs.size());
    std::vector<int> in_degree(sccs.size(), 0);

    for (size_t scc_idx = 0; scc_idx < sccs.size(); ++scc_idx) {
        for (size_t node_id : sccs[scc_idx]) {
            const GameNode& node = graph.get_node(node_id);
            for (size_t succ_id : node.successors) {
                size_t succ_scc_idx = node_to_scc[succ_id];
                if (succ_scc_idx != scc_idx && scc_graph[scc_idx].insert(succ_scc_idx).second) {
                    in_degree[succ_scc_idx]++;
                }
            }
        }
    }

    // Kahn's algorithm for topological sort
    std::queue<size_t> queue;
    for (size_t i = 0; i < sccs.size(); ++i) {
        if (in_degree[i] == 0) {
            queue.push(i);
        }
    }

    std::vector<size_t> topo_order;
    while (!queue.empty()) {
        size_t u = queue.front();
        queue.pop();
        topo_order.push_back(u);

        for (size_t v : scc_graph[u]) {
            if (--in_degree[v] == 0) {
                queue.push(v);
            }
        }
    }

    // Return reverse topological order (process successors first)
    return std::vector<size_t>(topo_order.rbegin(), topo_order.rend());
}

void GameSolver::classify_states(GameGraph& graph, const std::vector<std::vector<size_t>>& sccs) {
    // Get processing order (reverse topological order of SCC graph)
    std::vector<size_t> order = get_scc_processing_order(graph, sccs);

    // Process each SCC in order
    for (size_t scc_idx : order) {
        propagate_status(graph, sccs[scc_idx]);
    }
}

void GameSolver::propagate_status(GameGraph& graph, const std::vector<size_t>& scc) {
    // Build a set of SCC nodes for fast lookup
    std::unordered_set<size_t> scc_set(scc.begin(), scc.end());

    // Initial Swin: all successors of SCC nodes that are already Winning
    // (including successors outside the SCC)
    std::unordered_set<size_t> swin;

    // Collect all Winning successors
    for (size_t node_id : scc) {
        const GameNode& node = graph.get_node(node_id);
        for (size_t succ_id : node.successors) {
            const GameNode& succ = graph.get_node(succ_id);
            if (succ.status == StateStatus::Winning) {
                swin.insert(succ_id);
            }
        }
    }

    // Work list algorithm for backward propagation within SCC
    std::vector<size_t> work_list(scc.begin(), scc.end());

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
            bool has_winning_successor = false;
            bool all_successors_known = true;
            bool has_unknown_successor = false;

            for (size_t succ_id : node.successors) {
                const GameNode& succ = graph.get_node(succ_id);
                if (succ.status == StateStatus::Winning) {
                    has_winning_successor = true;
                } else if (succ.status == StateStatus::Unknown) {
                    has_unknown_successor = true;
                    all_successors_known = false;
                }
            }

            // Classification rules:
            // - If all successors are Winning, this node is Winning (can force win)
            // - If any successor is Winning and we're in the SCC, mark as Winning
            // - Otherwise remains Unknown (will be marked Losing at the end)

            if (has_winning_successor) {
                node.status = StateStatus::Winning;
                changed = true;
            }
        }
    }

    // Any remaining unknown nodes in SCC are marked as losing
    for (size_t node_id : scc) {
        if (graph.get_node(node_id).status == StateStatus::Unknown) {
            graph.get_node(node_id).status = StateStatus::Losing;
        }
    }
}

} // namespace synthesis
