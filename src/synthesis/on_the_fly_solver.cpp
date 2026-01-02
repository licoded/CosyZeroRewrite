/**
 * @file on_the_fly_solver.cpp
 * @brief Implementation of on-the-fly game solver
 */

#include "synthesis/on_the_fly_solver.hpp"
#include "log/logger.hpp"
#include <algorithm>
#include <sstream>
#include <functional>

namespace synthesis {

//==============================================================================
// GameState
//==============================================================================

std::string GameState::to_string() const {
    std::ostringstream oss;
    oss << "{dfa=" << dfa_state << ", player="
        << (player == Player::System ? "Sys" : "Env");
    if (!current_output.empty()) {
        oss << ", out={";
        bool first = true;
        for (int v : current_output) {
            if (!first) oss << ",";
            oss << v;
            first = false;
        }
        oss << "}";
    }
    oss << "}";
    return oss.str();
}

//==============================================================================
// OnTheFlyGameSolver
//==============================================================================

namespace {

// Count unique variables in a formula
std::unordered_set<int> collect_variables(formula::Formula* phi) {
    std::unordered_set<int> vars;

    std::function<void(formula::Formula*)> visit = [&](formula::Formula* f) {
        if (!f) return;

        if (f->op() == formula::Formula::OpType::Literal) {
            vars.insert(f->var_id());
        } else {
            visit(f->left());
            visit(f->right());
        }
    };

    visit(phi);
    return vars;
}

} // namespace

OnTheFlyGameSolver::OnTheFlyGameSolver(formula::Formula* phi,
                                       formula::FormulaPool& pool,
                                       int num_outputs,
                                       int num_inputs)
    : pool_(pool),
      original_formula_(phi),
      dfa_(phi, pool),
      output_gen_(num_outputs > 0 ? num_outputs : count_variables(phi)),
      input_gen_(num_inputs > 0 ? num_inputs : 0),  // No inputs if not specified
      num_sccs_found_(0),
      initial_state_(dfa_.initial_state(), Player::System)
{
    LOG_DEBUG("OnTheFlyGameSolver: initialized with formula: ", phi->to_string());
}

int OnTheFlyGameSolver::count_variables(formula::Formula* phi) {
    auto vars = collect_variables(phi);
    return static_cast<int>(vars.size());
}

bool OnTheFlyGameSolver::is_realizable() {
    LOG_DEBUG("OnTheFlyGameSolver: starting realizability check");

    // Initialize worklist with initial state
    worklist_.clear();
    worklist_.push_back(initial_state_);

    // Main loop: expand, classify SCCs, propagate, repeat
    int iteration = 0;
    while (!worklist_.empty() && !is_initial_classified()) {
        iteration++;
        LOG_DEBUG("OnTheFlyGameSolver: iteration ", iteration,
                  ", worklist size=", worklist_.size(),
                  ", expanded=", expanded_.size());

        // Pop a state to expand
        GameState state = worklist_.back();
        worklist_.pop_back();

        // Skip if already expanded
        if (expanded_.count(state)) {
            continue;
        }

        // Expand state (compute successors)
        expand_state(state);

        // Run SCC decomposition on current graph
        auto sccs = find_sccs();
        LOG_DEBUG("OnTheFlyGameSolver: found ", sccs.size(), " SCCs");

        // Try to classify each SCC
        for (const auto& scc : sccs) {
            if (scc.empty()) continue;

            LOG_DEBUG("OnTheFlyGameSolver: processing SCC with ", scc.size(), " states");
            auto cls = try_classify_scc(scc);
            if (cls) {
                LOG_DEBUG("OnTheFlyGameSolver: SCC classified as ", to_string(*cls));
                num_sccs_found_++;

                // Classify all states in SCC
                for (const auto& s : scc) {
                    classification_[s] = *cls;
                }

                // Propagate backward
                bool initial_done = propagate_classification();
                if (initial_done) {
                    LOG_DEBUG("OnTheFlyGameSolver: initial state classified!");
                    break;
                }
            }
        }

        // Add unclassified successors to worklist
        for (const auto& pair : successors_) {
            // pair.first is the source state, pair.second is the list of successors
            for (const GameState& succ : pair.second) {
                if (!classification_.count(succ) && !expanded_.count(succ)) {
                    worklist_.push_back(succ);
                }
            }
        }

        // Limit iterations to prevent infinite loop (should not happen)
        if (iteration > 10000) {
            LOG_WARN("OnTheFlyGameSolver: iteration limit reached");
            break;
        }
    }

    // If initial state is still unclassified after main loop,
    // check if we've exhausted all reachable states
    if (!is_initial_classified() && worklist_.empty()) {
        LOG_DEBUG("OnTheFlyGameSolver: worklist empty, checking for terminal classification");

        // Check if there's any unexpanded state reachable from initial state
        // If not, classify all unclassified states as Ewin and propagate
        bool has_unexpanded = false;
        for (const auto& pair : successors_) {
            const GameState& s = pair.first;
            if (!expanded_.count(s)) {
                has_unexpanded = true;
                break;
            }
        }

        if (!has_unexpanded && successors_.size() > 0) {
            LOG_DEBUG("OnTheFlyGameSolver: all states expanded, classifying terminal states");

            // Classify terminal states (states with no successors)
            // Terminal state semantics:
            // - System turn, no successors: System can't move, loses → Ewin
            // - Environment turn, no successors: Environment can't move
            //   - If accepting: System wins → Swin
            //   - If not accepting: formula violated, System loses → Ewin
            for (const auto& pair : successors_) {
                const GameState& s = pair.first;
                if (classification_.count(s)) continue;

                auto succ_it = successors_.find(s);
                if (succ_it != successors_.end() && succ_it->second.empty()) {
                    // Terminal state
                    bool is_accepting = dfa_.is_accepting(s.dfa_state);
                    if (s.player == Player::System) {
                        // System has no moves → loses
                        classification_[s] = StateClass::Ewin;
                        LOG_DEBUG("OnTheFlyGameSolver: terminal System state -> Ewin");
                    } else {
                        // Environment has no moves → Environment loses
                        // System wins if formula is satisfied (accepting)
                        classification_[s] = is_accepting ? StateClass::Swin : StateClass::Ewin;
                        LOG_DEBUG("OnTheFlyGameSolver: terminal Env state -> ",
                                  is_accepting ? "Swin" : "Ewin");
                    }
                }
            }

            // For remaining non-terminal unclassified states in SCCs,
            // they form cycles without accepting states → Ewin
            for (const auto& pair : successors_) {
                const GameState& s = pair.first;
                if (!classification_.count(s) && !successors_[s].empty()) {
                    classification_[s] = StateClass::Ewin;
                    LOG_DEBUG("OnTheFlyGameSolver: non-terminal unclassified -> Ewin");
                }
            }

            // Final propagation
            propagate_classification();
        }
    }

    StateClass result = get_initial_classification();
    LOG_DEBUG("OnTheFlyGameSolver: final classification: ", to_string(result));

    return result == StateClass::Swin;
}

void OnTheFlyGameSolver::expand_state(const GameState& state) {
    if (expanded_.count(state)) {
        return;  // Already expanded
    }

    LOG_DEBUG("OnTheFlyGameSolver: expanding state");

    std::vector<GameState> succs;

    if (state.player == Player::System) {
        // System's turn: choose output assignment
        // All outputs lead to environment states with different outputs
        auto outputs = output_gen_.all_assignments();

        for (const auto& out : outputs) {
            succs.push_back(environment_state(state.dfa_state, out));
        }

        LOG_DEBUG("OnTheFlyGameSolver: system state -> ", succs.size(), " environment states");
    } else {
        // Environment's turn: choose input assignment
        // Combine with current output (chosen by system) and compute next DFA state
        auto inputs = input_gen_.all_assignments();

        for (const auto& in : inputs) {
            // Combine output and input into full assignment
            automata::Assignment full = state.current_output;

            // Offset input variable IDs by number of outputs
            for (int v : in) {
                full.insert(v + output_gen_.num_variables());
            }

            // Compute next DFA state
            automata::TableauState* next_dfa = dfa_.successor(state.dfa_state, full);
            succs.push_back(system_state(next_dfa));
        }

        LOG_DEBUG("OnTheFlyGameSolver: environment state -> ", succs.size(), " system states");
    }

    successors_[state] = succs;
    expanded_.insert(state);
}

const std::vector<GameState>& OnTheFlyGameSolver::get_successors(const GameState& state) {
    auto it = successors_.find(state);
    if (it == successors_.end()) {
        expand_state(state);
        it = successors_.find(state);
    }
    return it->second;
}

std::vector<std::vector<GameState>> OnTheFlyGameSolver::find_sccs() {
    std::vector<std::vector<GameState>> result;

    // Tarjan's SCC algorithm
    std::unordered_map<GameState, int, GameStateHash, GameStateEqual> indices;
    std::unordered_map<GameState, int, GameStateHash, GameStateEqual> lowlinks;
    std::unordered_map<GameState, bool, GameStateHash, GameStateEqual> on_stack;
    std::vector<GameState> stack;

    int index = 0;

    std::function<void(const GameState&)> strongconnect = [&](const GameState& v) {
        indices[v] = index;
        lowlinks[v] = index;
        index++;
        stack.push_back(v);
        on_stack[v] = true;

        // Consider successors
        const auto& succs = get_successors(v);
        for (const GameState& w : succs) {
            if (indices.count(w) == 0) {
                // Successor not visited
                strongconnect(w);
                lowlinks[v] = std::min(lowlinks[v], lowlinks[w]);
            } else if (on_stack[w]) {
                // Successor is on stack
                lowlinks[v] = std::min(lowlinks[v], indices[w]);
            }
        }

        // If v is root of SCC
        if (lowlinks[v] == indices[v]) {
            std::vector<GameState> scc;
            GameState w;
            do {
                w = stack.back();
                stack.pop_back();
                on_stack[w] = false;
                scc.push_back(w);
            } while (w != v);
            result.push_back(scc);
        }
    };

    // Visit all expanded states
    for (const auto& pair : successors_) {
        const GameState& v = pair.first;
        if (indices.count(v) == 0) {
            strongconnect(v);
        }
    }

    return result;
}

std::optional<StateClass>
OnTheFlyGameSolver::try_classify_scc(const std::vector<GameState>& scc) {
    // Check if any state in SCC is already classified
    StateClass existing = StateClass::Unknown;
    bool has_classified = false;
    for (const auto& s : scc) {
        auto it = classification_.find(s);
        if (it != classification_.end()) {
            existing = it->second;
            has_classified = true;
            break;
        }
    }
    if (has_classified) {
        return existing;
    }

    // Check if SCC contains an accepting DFA state
    // In LTLf, accepting states are those where all Until obligations are satisfied
    bool has_accepting = false;
    for (const auto& s : scc) {
        if (dfa_.is_accepting(s.dfa_state)) {
            has_accepting = true;
            break;
        }
    }

    // Classification rule from paper:
    // - SCC with accepting state → System winning (Swin)
    if (has_accepting) {
        return StateClass::Swin;
    }

    // For SCCs without accepting states, we need to check if all successors
    // lead to Ewin states. This check happens in propagate_classification.
    // For now, return nullopt to indicate this SCC can't be classified yet.
    return std::nullopt;
}

bool OnTheFlyGameSolver::propagate_classification() {
    bool changed = true;
    int iterations = 0;

    while (changed && iterations < 1000) {
        iterations++;
        changed = false;

        for (auto& pair : successors_) {
            const GameState& state = pair.first;
            const std::vector<GameState>& succs = pair.second;

            // Skip if already classified
            if (classification_.count(state)) {
                continue;
            }

            if (state.player == Player::System) {
                // System wins if ANY successor is Swin
                // System loses if ALL successors are Ewin
                bool has_swin = false;
                bool all_ewin = true;

                for (const auto& succ : succs) {
                    auto it = classification_.find(succ);
                    if (it != classification_.end()) {
                        if (it->second == StateClass::Swin) {
                            has_swin = true;
                        } else if (it->second == StateClass::Ewin) {
                            // Continue checking
                        } else {
                            all_ewin = false;
                        }
                    } else {
                        all_ewin = false;
                    }
                }

                if (has_swin) {
                    classification_[state] = StateClass::Swin;
                    changed = true;
                    LOG_DEBUG("OnTheFlyGameSolver: propagate: system state -> Swin (has Swin succ)");
                } else if (all_ewin && !succs.empty()) {
                    classification_[state] = StateClass::Ewin;
                    changed = true;
                    LOG_DEBUG("OnTheFlyGameSolver: propagate: system state -> Ewin (all Ewin succs)");
                }
            } else {
                // Environment wins if ALL successors are Ewin
                // Environment loses (system wins) if ANY successor is Swin
                bool has_swin = false;
                bool all_ewin = true;

                for (const auto& succ : succs) {
                    auto it = classification_.find(succ);
                    if (it != classification_.end()) {
                        if (it->second == StateClass::Swin) {
                            has_swin = true;
                            all_ewin = false;
                        } else if (it->second == StateClass::Ewin) {
                            // Continue checking
                        } else {
                            all_ewin = false;
                        }
                    } else {
                        all_ewin = false;
                    }
                }

                if (all_ewin && !succs.empty()) {
                    classification_[state] = StateClass::Ewin;
                    changed = true;
                    LOG_DEBUG("OnTheFlyGameSolver: propagate: environment state -> Ewin (all Ewin succs)");
                } else if (has_swin) {
                    classification_[state] = StateClass::Swin;
                    changed = true;
                    LOG_DEBUG("OnTheFlyGameSolver: propagate: environment state -> Swin (has Swin succ)");
                }
            }

            // Check if initial state is now classified
            if (state == initial_state_ && classification_.count(state)) {
                return true;
            }
        }
    }

    return is_initial_classified();
}

//==============================================================================
// Convenience function
//==============================================================================

bool is_realizable_on_the_fly(formula::Formula* phi,
                              formula::FormulaPool& pool) {
    // Get variable counts from the pool
    int num_outputs = pool.num_outputs();
    int num_inputs = pool.num_inputs();

    // If variables not declared, extract them from the formula
    if (num_outputs == 0 && num_inputs == 0) {
        // Count all variables as outputs (simplified)
        std::unordered_set<int> vars;
        std::function<void(formula::Formula*)> collect = [&](formula::Formula* f) {
            if (!f) return;
            if (f->op() == formula::Formula::OpType::Literal) {
                vars.insert(f->var_id());
            } else {
                collect(f->left());
                collect(f->right());
            }
        };
        collect(phi);
        num_outputs = static_cast<int>(vars.size());
    }

    synthesis::OnTheFlyGameSolver solver(phi, pool, num_outputs, num_inputs);
    return solver.is_realizable();
}

} // namespace synthesis
