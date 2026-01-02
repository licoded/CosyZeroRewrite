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
    if (system_chosen_output.has_value()) {
        oss << ", out={";
        bool first = true;
        for (int v : system_chosen_output.value()) {
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

    // Debug mode: Set COSY_DEBUG_PROPAGATION=1 to enable consistency checking
    const char* debug_prop = std::getenv("COSY_DEBUG_PROPAGATION");
    bool enable_consistency_check = (debug_prop && std::string(debug_prop) == "1");

    // ========================================================================
    // PHASE 1: Expand ALL reachable states (no early SCC/classify/propagate)
    // ========================================================================
    LOG_DEBUG("=== PHASE 1: Expanding all reachable states ===");

    worklist_.clear();
    worklist_.push_back(initial_state_);

    int expand_iteration = 0;
    while (!worklist_.empty()) {
        expand_iteration++;

        // Pop a state to expand
        GameState state = worklist_.back();
        worklist_.pop_back();

        // Skip if already expanded
        if (expanded_.count(state)) {
            continue;
        }

        // Expand state (compute successors)
        expand_state(state);

        LOG_DEBUG("Phase 1: expanded state #", expanded_.size(),
                  " (worklist remaining: ", worklist_.size(), ")");

        // Add all unexpanded successors to worklist
        auto succ_it = successors_.find(state);
        if (succ_it != successors_.end()) {
            for (const GameState& succ : succ_it->second) {
                if (!expanded_.count(succ)) {
                    worklist_.push_back(succ);
                }
            }
        }

        // Safety limit
        if (expand_iteration > 100000) {
            LOG_WARN("Phase 1: expansion limit reached");
            break;
        }
    }

    LOG_DEBUG("Phase 1 complete: expanded ", expanded_.size(), " states");

    // ========================================================================
    // PHASE 2: SCC decomposition, classification, and propagation
    // ========================================================================
    LOG_DEBUG("=== PHASE 2: SCC decomposition and classification ===");

    // Run SCC decomposition on the COMPLETE graph
    auto sccs = find_sccs();
    LOG_DEBUG("Phase 2: found ", sccs.size(), " SCCs in complete graph");
    num_sccs_found_ = sccs.size();

    // Classify each SCC using fixed-point iteration
    for (const auto& scc : sccs) {
        if (scc.empty()) continue;

        LOG_DEBUG("Phase 2: processing SCC with ", scc.size(), " states");
        classify_scc(scc);
    }

    // ========================================================================
    // PHASE 3: Terminal state classification (for states without SCC)
    // ========================================================================
    LOG_DEBUG("=== PHASE 3: Terminal state classification ===");

    for (const auto& pair : successors_) {
        const GameState& s = pair.first;
        if (classification_.count(s)) continue;

        auto succ_it = successors_.find(s);
        if (succ_it != successors_.end() && succ_it->second.empty()) {
            // Terminal state
            bool is_accepting = dfa_.is_accepting(s.dfa_state);
            if (s.player == Player::System) {
                classification_[s] = StateClass::Ewin;
                LOG_DEBUG("Phase 3: terminal System state -> Ewin");
            } else {
                classification_[s] = is_accepting ? StateClass::Swin : StateClass::Ewin;
                LOG_DEBUG("Phase 3: terminal Env state -> ",
                          is_accepting ? "Swin" : "Ewin");
            }
        }
    }

    // Any remaining unclassified non-terminal states are in losing SCCs
    for (const auto& pair : successors_) {
        const GameState& s = pair.first;
        if (!classification_.count(s) && !successors_[s].empty()) {
            classification_[s] = StateClass::Ewin;
        }
    }

    // ========================================================================
    // PHASE 4: Propagate classification to fixed point
    // ========================================================================
    LOG_DEBUG("=== PHASE 4: Propagating classification ===");

    int prop_iteration = 0;
    while (propagate_classification() && prop_iteration < 1000) {
        prop_iteration++;
        LOG_DEBUG("Phase 4: propagation iteration ", prop_iteration);
    }
    LOG_DEBUG("Phase 4: propagation complete after ", prop_iteration, " iterations");

    // ========================================================================
    // FINAL: Check consistency and return result
    // ========================================================================
    StateClass result = get_initial_classification();
    LOG_DEBUG("OnTheFlyGameSolver: final classification: ", to_string(result));

    if (enable_consistency_check) {
        LOG_DEBUG("=== Running propagation consistency check ===");
        size_t violations = check_propagation_consistency();
        if (violations > 0) {
            LOG_WARN("OnTheFlyGameSolver: ", violations, " propagation violations detected!");
        }
    }

    return result == StateClass::Swin;
}

//==============================================================================
// State Expansion
//==============================================================================

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
            // system_chosen_output must have value for Environment turn (by construction)
            automata::Assignment full = state.system_chosen_output.value();

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

// Testing-only version that doesn't trigger expand_state()
// This version only works with states explicitly added to successors_ map
std::vector<std::vector<GameState>> OnTheFlyGameSolver::find_sccs_for_testing() {
    std::vector<std::vector<GameState>> result;

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

        // Directly access successors_ map - don't trigger expand_state()
        auto succ_it = successors_.find(v);
        if (succ_it != successors_.end()) {
            for (const GameState& w : succ_it->second) {
                // If successor doesn't have its own entry, auto-register as terminal
                if (successors_.count(w) == 0) {
                    successors_[w] = {};  // Empty successor list for terminal state
                }

                if (indices.count(w) == 0) {
                    strongconnect(w);
                    lowlinks[v] = std::min(lowlinks[v], lowlinks[w]);
                } else if (on_stack[w]) {
                    lowlinks[v] = std::min(lowlinks[v], indices[w]);
                }
            }
        }

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

    // Visit all states in successors_ map
    for (const auto& pair : successors_) {
        const GameState& v = pair.first;
        if (indices.count(v) == 0) {
            strongconnect(v);
        }
    }

    return result;
}

bool OnTheFlyGameSolver::classify_scc(const std::vector<GameState>& scc) {
    // Create a set for fast SCC membership test
    std::unordered_set<GameState, GameStateHash, GameStateEqual> scc_set;
    for (const auto& s : scc) {
        scc_set.insert(s);
    }

    // Build predecessor map within SCC: state -> predecessors in SCC
    std::unordered_map<GameState, std::vector<GameState>, GameStateHash, GameStateEqual> predecessors;
    for (const auto& state : scc) {
        auto succ_it = successors_.find(state);
        if (succ_it != successors_.end()) {
            for (const auto& succ : succ_it->second) {
                // Only consider predecessors that are in the SCC
                if (scc_set.count(succ)) {
                    predecessors[succ].push_back(state);
                }
            }
        }
    }

    // Step 1: Initialize seed set (accepting states are Swin)
    std::unordered_set<GameState, GameStateHash, GameStateEqual> swin_states;
    for (const auto& s : scc) {
        // Skip if already classified
        if (classification_.count(s)) {
            if (classification_[s] == StateClass::Swin) {
                swin_states.insert(s);
            }
            continue;
        }

        // Accepting DFA states are Swin seeds
        if (dfa_.is_accepting(s.dfa_state)) {
            swin_states.insert(s);
            classification_[s] = StateClass::Swin;
        }
    }

    // Step 2: Fixed-point iteration
    bool changed = true;
    while (changed) {
        changed = false;
        std::unordered_set<GameState, GameStateHash, GameStateEqual> new_swin_states;

        // Find predecessors of current Swin states
        for (const GameState& swin : swin_states) {
            auto pred_it = predecessors.find(swin);
            if (pred_it == predecessors.end()) continue;

            for (const GameState& pred : pred_it->second) {
                // Skip if already classified
                if (classification_.count(pred)) continue;

                // Check if predecessor can be classified as Swin
                auto succ_it = successors_.find(pred);
                if (succ_it == successors_.end()) continue;

                const auto& succs = succ_it->second;

                if (pred.player == Player::System) {
                    // System: ANY successor Swin → Swin
                    bool has_swin_succ = false;
                    for (const auto& succ : succs) {
                        auto cls_it = classification_.find(succ);
                        if (cls_it != classification_.end() &&
                            cls_it->second == StateClass::Swin) {
                            has_swin_succ = true;
                            break;
                        }
                    }
                    if (has_swin_succ) {
                        new_swin_states.insert(pred);
                        classification_[pred] = StateClass::Swin;
                        changed = true;
                    }
                } else {
                    // Environment: ALL successors Swin → Swin
                    bool all_swin_succ = !succs.empty();
                    for (const auto& succ : succs) {
                        auto cls_it = classification_.find(succ);
                        if (cls_it == classification_.end() ||
                            cls_it->second != StateClass::Swin) {
                            all_swin_succ = false;
                            break;
                        }
                    }
                    if (all_swin_succ) {
                        new_swin_states.insert(pred);
                        classification_[pred] = StateClass::Swin;
                        changed = true;
                    }
                }
            }
        }

        swin_states.insert(new_swin_states.begin(), new_swin_states.end());
    }

    // Step 3: Mark remaining states as Ewin
    for (const auto& s : scc) {
        if (!classification_.count(s)) {
            classification_[s] = StateClass::Ewin;
        }
    }

    // All states in SCC are now classified
    return true;
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
                // Environment's turn: Environment chooses input
                // Environment wants System to lose, so will choose input leading to Ewin
                // If ANY successor is Ewin → Environment chooses it → current is Ewin
                // If ALL successors are Swin → Environment cannot avoid → current is Swin
                bool has_ewin = false;
                bool all_swin = true;

                for (const auto& succ : succs) {
                    auto it = classification_.find(succ);
                    if (it != classification_.end()) {
                        if (it->second == StateClass::Ewin) {
                            has_ewin = true;
                            all_swin = false;
                        } else if (it->second == StateClass::Swin) {
                            // Continue checking
                        } else {
                            all_swin = false;
                        }
                    } else {
                        all_swin = false;
                    }
                }

                if (has_ewin) {
                    classification_[state] = StateClass::Ewin;
                    changed = true;
                    LOG_DEBUG("OnTheFlyGameSolver: propagate: environment state -> Ewin (has Ewin succ)");
                } else if (all_swin && !succs.empty()) {
                    classification_[state] = StateClass::Swin;
                    changed = true;
                    LOG_DEBUG("OnTheFlyGameSolver: propagate: environment state -> Swin (all Swin succs)");
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
// Debug: Propagation Consistency Checker
//==============================================================================

size_t OnTheFlyGameSolver::check_propagation_consistency() const {
    size_t violations = 0;

    for (const auto& pair : successors_) {
        const GameState& state = pair.first;
        const std::vector<GameState>& succs = pair.second;

        // Skip unclassified states
        auto cls_it = classification_.find(state);
        if (cls_it == classification_.end()) {
            continue;
        }

        StateClass cls = cls_it->second;

        if (state.player == Player::System) {
            // System state rules
            if (cls == StateClass::Swin) {
                // Swin: should have at least one Swin successor
                bool has_swin_succ = false;
                for (const auto& succ : succs) {
                    auto succ_cls = classification_.find(succ);
                    if (succ_cls != classification_.end() &&
                        succ_cls->second == StateClass::Swin) {
                        has_swin_succ = true;
                        break;
                    }
                }
                if (!has_swin_succ && !succs.empty()) {
                    LOG_WARN("Propagation violation: System state ", state.to_string(),
                             " is Swin but has no Swin successor (has ", succs.size(), " successors)");
                    violations++;
                }
            } else if (cls == StateClass::Ewin) {
                // Ewin: should have all successors as Ewin
                bool all_ewin = true;
                for (const auto& succ : succs) {
                    auto succ_cls = classification_.find(succ);
                    if (succ_cls == classification_.end() ||
                        succ_cls->second != StateClass::Ewin) {
                        all_ewin = false;
                        LOG_WARN("Propagation violation: System state ", state.to_string(),
                                 " is Ewin but successor is not Ewin");
                        break;
                    }
                }
                if (!all_ewin && !succs.empty()) {
                    violations++;
                }
            }
        } else {
            // Environment state rules
            if (cls == StateClass::Swin) {
                // Swin: should have all successors as Swin
                bool all_swin = !succs.empty();
                for (const auto& succ : succs) {
                    auto succ_cls = classification_.find(succ);
                    if (succ_cls == classification_.end() ||
                        succ_cls->second != StateClass::Swin) {
                        all_swin = false;
                        LOG_WARN("Propagation violation: Environment state ", state.to_string(),
                                 " is Swin but not all successors are Swin");
                        break;
                    }
                }
                if (!all_swin && !succs.empty()) {
                    violations++;
                }
            } else if (cls == StateClass::Ewin) {
                // Ewin: should have at least one Ewin successor
                bool has_ewin_succ = false;
                for (const auto& succ : succs) {
                    auto succ_cls = classification_.find(succ);
                    if (succ_cls != classification_.end() &&
                        succ_cls->second == StateClass::Ewin) {
                        has_ewin_succ = true;
                        break;
                    }
                }
                if (!has_ewin_succ && !succs.empty()) {
                    LOG_WARN("Propagation violation: Environment state ", state.to_string(),
                             " is Ewin but has no Ewin successor (has ", succs.size(), " successors)");
                    violations++;
                }
            }
        }
    }

    if (violations > 0) {
        LOG_ERROR("Found ", violations, " propagation consistency violations!");
    } else {
        LOG_DEBUG("Propagation consistency check: PASSED");
    }

    return violations;
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
