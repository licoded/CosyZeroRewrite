/**
 * @file on_the_fly_solver.cpp
 * @brief Implementation of on-the-fly game solver
 */

#include "synthesis/on_the_fly_solver.hpp"
#include "synthesis/trace_exporter.hpp"
#include "log/logger.hpp"
#include <algorithm>
#include <sstream>
#include <functional>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <vector>

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
    if (environment_chosen_input.has_value()) {
        oss << ", in={";
        bool first = true;
        for (int v : environment_chosen_input.value()) {
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

OnTheFlyGameSolver::~OnTheFlyGameSolver() {
    // Destructor defined here for unique_ptr<TraceExporter> to work with forward declaration
    // Trace exporter will be automatically finalized by its own destructor
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

    if (trace_exporter_) {
        trace_exporter_->begin_stage("expand", "State Expansion Phase");
        trace_exporter_->capture_state("Initial state", *this);
    }

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

        // Trace: record expansion
        if (trace_exporter_) {
            auto succ_it = successors_.find(state);
            if (succ_it != successors_.end()) {
                trace_exporter_->record_expansion(state, succ_it->second, *this);
            }
        }

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

    if (trace_exporter_) {
        trace_exporter_->end_stage();
    }

    // ========================================================================
    // PHASE 2: SCC decomposition, classification, and propagation
    // ========================================================================
    LOG_DEBUG("=== PHASE 2: SCC decomposition and classification ===");

    if (trace_exporter_) {
        trace_exporter_->begin_stage("scc", "SCC Decomposition and Classification");
        trace_exporter_->capture_state("Before SCC", *this);
    }

    // Run SCC decomposition on the COMPLETE graph
    auto sccs = find_sccs();
    LOG_DEBUG("Phase 2: found ", sccs.size(), " SCCs in complete graph");
    num_sccs_found_ = sccs.size();

    if (trace_exporter_) {
        trace_exporter_->capture_state("Found " + std::to_string(sccs.size()) + " SCCs", *this);
    }

    // Classify each SCC using fixed-point iteration
    for (size_t i = 0; i < sccs.size(); ++i) {
        const auto& scc = sccs[i];
        if (scc.empty()) continue;

        LOG_DEBUG("Phase 2: processing SCC with ", scc.size(), " states");

        // Trace: record SCC
        if (trace_exporter_) {
            std::ostringstream oss;
            oss << "scc_" << std::setfill('0') << std::setw(3) << i;
            trace_exporter_->record_scc(scc, oss.str(), *this);
        }

        classify_scc(scc);
    }

    if (trace_exporter_) {
        trace_exporter_->end_stage();
    }

    // ========================================================================
    // FINAL: Check consistency and return result
    // ========================================================================
    StateClass result = get_initial_classification();

    // DEBUG: Log detailed classification summary
    size_t total_states = successors_.size();
    size_t swin_count = 0, ewin_count = 0, unknown_count = 0;
    for (const auto& pair : successors_) {
        auto it = classification_.find(pair.first);
        if (it == classification_.end()) {
            unknown_count++;
        } else if (it->second == StateClass::Swin) {
            swin_count++;
        } else if (it->second == StateClass::Ewin) {
            ewin_count++;
        }
    }

    LOG_DEBUG("=== Classification Summary ===");
    LOG_DEBUG("  Total states: ", total_states);
    LOG_DEBUG("  Swin: ", swin_count, " | Ewin: ", ewin_count, " | Unknown: ", unknown_count);
    LOG_DEBUG("  Initial state player: ", initial_state_.player == Player::System ? "Sys" : "Env");
    LOG_DEBUG("  Initial state classification: ", to_string(result));

    if (enable_consistency_check) {
        LOG_DEBUG("=== Running propagation consistency check ===");
        size_t violations = check_propagation_consistency();
        if (violations > 0) {
            LOG_WARN("OnTheFlyGameSolver: ", violations, " propagation violations detected!");
        }
    }

    // Finalize trace before returning
    if (trace_exporter_) {
        trace_exporter_->finalize(result == StateClass::Swin, *this);
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

    // Extract relevant variable IDs from prop_atoms
    // This optimization reduces the enumeration space from 2^n to 2^k
    // where k = number of variables actually present in the current state
    std::set<int> relevant_output_var_ids;
    std::set<int> relevant_input_var_ids;  // Local indices (0-based for inputs)

    for (formula::Formula* pa : state.dfa_state->prop_atoms()) {
        if (pa->is_next()) continue;  // Skip Next formulas
        assert(pa->op() == formula::Formula::OpType::Literal);
        int var_id = pa->var_id();
        if (var_id < output_gen_.num_variables()) {
            relevant_output_var_ids.insert(var_id);
        } else {
            // Input variable: adjust to local index
            relevant_input_var_ids.insert(var_id - output_gen_.num_variables());
        }
    }

    if (state.player == Player::System) {
        // System's turn: choose output assignment
        // Only enumerate assignments for variables actually in prop_atoms
        std::vector<automata::Assignment> outputs;
        if (relevant_output_var_ids.empty()) {
            outputs = {{}};  // Empty assignment = {} (one true edge)
        } else {
            // Convert set to vector for all_assignments_for_subset
            std::vector<int> output_vec(relevant_output_var_ids.begin(), relevant_output_var_ids.end());
            outputs = output_gen_.all_assignments_for_subset(output_vec);
        }

        for (const auto& out : outputs) {
            succs.push_back(environment_state(state.dfa_state, out));
        }

        LOG_DEBUG("OnTheFlyGameSolver: system state -> ", succs.size(), " environment states");
    } else {
        // Environment's turn: choose input assignment
        // Only enumerate assignments for variables actually in prop_atoms
        std::vector<automata::Assignment> inputs;
        if (relevant_input_var_ids.empty()) {
            inputs = {{}};  // Empty assignment = {} (one true edge)
        } else {
            // Convert set to vector for all_assignments_for_subset
            std::vector<int> input_vec(relevant_input_var_ids.begin(), relevant_input_var_ids.end());
            inputs = input_gen_.all_assignments_for_subset(input_vec);
        }

        for (const auto& in : inputs) {
            // Combine output and input into full assignment
            // system_chosen_output must have value for Environment turn (by construction)
            automata::Assignment full = state.system_chosen_output.value();

            // Offset input variable IDs by number of outputs
            // Note: 'in' already contains local indices from all_assignments_for_subset
            for (int v : in) {
                full.insert(v + output_gen_.num_variables());
            }

            // Compute next DFA state
            automata::TableauState* next_dfa = dfa_.successor(state.dfa_state, full);

            // Debug: print env edge details
            std::cerr << "  [ENV EDGE] from dfa=" << state.dfa_state << " to dfa=" << next_dfa << std::endl;
            std::cerr << "    input assignment (local indices): {";
            for (auto it = in.begin(); it != in.end(); ++it) {
                if (it != in.begin()) std::cerr << ", ";
                std::cerr << *it;
            }
            std::cerr << "}" << std::endl;
            std::cerr << "    input assignment (global ids): {";
            for (auto it = in.begin(); it != in.end(); ++it) {
                if (it != in.begin()) std::cerr << ", ";
                std::cerr << (*it + output_gen_.num_variables());
            }
            std::cerr << "}" << std::endl;
            std::cerr << "    system_chosen_output: {";
            for (auto it = state.system_chosen_output.value().begin(); it != state.system_chosen_output.value().end(); ++it) {
                if (it != state.system_chosen_output.value().begin()) std::cerr << ", ";
                std::cerr << *it;
            }
            std::cerr << "}" << std::endl;
            std::cerr << "    full assignment: {";
            for (auto it = full.begin(); it != full.end(); ++it) {
                if (it != full.begin()) std::cerr << ", ";
                std::cerr << *it;
            }
            std::cerr << "}" << std::endl;

            // Create system state with the input assignment stored
            // This allows us to label the env move edge with the input assignment
            // Note: Reaching true/false is NOT forced termination in LTLf.
            // Only an explicit End marker forces termination.
            // Even if prop_atoms is empty (e.g., for true), we continue the game.
            succs.push_back(system_state(next_dfa, in));
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

/**
 * @brief Recursively check if a formula can be satisfied by the empty string
 *
 * Empty-string acceptance (ESA) means the formula is satisfied when the
 * trace has ended (no more states to evaluate).
 *
 * Rules for empty-string acceptance:
 * - True: ALWAYS accepts (trivially satisfied)
 * - False: NEVER accepts (trivially unsatisfied)
 * - Literal: NEVER accepts (needs a state to evaluate the variable)
 * - And(φ, ψ): accept iff φ accepts AND ψ accepts
 * - Or(φ, ψ): accept iff φ accepts OR ψ accepts
 * - Not(φ): NEVER accepts (simplified - TODO: handle negation properly)
 * - Until(φ, ψ): NEVER accepts (requires ψ in the future)
 * - Next(φ): NEVER accepts (requires the next state)
 * - Release(φ, ψ): ALWAYS accepts (weak semantics: holds by default)
 *
 * @param f The formula to check
 * @return true if the formula can be satisfied by the empty string
 */
static bool formula_empty_string_accepting(formula::Formula* f) {
    if (!f) return true;  // Empty is vacuously true

    switch (f->op()) {
        case formula::Formula::OpType::True:
            return true;  // Trivially satisfied

        case formula::Formula::OpType::Literal:
        case formula::Formula::OpType::False:
            return false;  // Needs a state to evaluate (literal) or never satisfied (false)

        case formula::Formula::OpType::And:
            return formula_empty_string_accepting(f->left()) &&
                   formula_empty_string_accepting(f->right());

        case formula::Formula::OpType::Or:
            return formula_empty_string_accepting(f->left()) ||
                   formula_empty_string_accepting(f->right());

        case formula::Formula::OpType::Not:
            // TODO: Handle negation properly (!p should be ESA when p is not ESA)
            return false;  // Simplified: negation never accepts by empty string

        case formula::Formula::OpType::Until:
            // φ U ψ requires ψ to be true in the future
            return false;  // Cannot be satisfied by empty string

        case formula::Formula::OpType::Next:
            // X φ requires the next state to satisfy φ
            return false;  // Cannot be satisfied by empty string

        case formula::Formula::OpType::Release:
            // φ R ψ has weak semantics: holds by default
            return true;  // Always satisfied by empty string

        default:
            return false;  // Unknown operator, conservative
    }
}

/**
 * @brief Check if a TableauState can be satisfied by the empty string
 *
 * In the new TableauState design, each state has a single phi_ formula
 * that represents the entire state. We check if this phi can be satisfied
 * by the empty string.
 *
 * @param q The TableauState to check
 * @return true if the state can be satisfied by the empty string
 */
bool OnTheFlyGameSolver::is_empty_string_accepting(automata::TableauState* q) const {
    if (!q) return false;

    // Check the phi_ formula (original formula representing this state)
    formula::Formula* phi = q->phi();
    if (!phi) return true;  // Empty phi is vacuously true

    return formula_empty_string_accepting(phi);
}

bool OnTheFlyGameSolver::classify_scc(const std::vector<GameState>& scc) {
    // ========== Step 0: Build predecessors map ==========
    // Key: states about to perform sys move (System states only)
    // Value: set of predecessor System states that can reach key via complete sys+env move
    // Important: predecessors NOT limited to SCC, includes all external preds
    std::unordered_map<GameState, std::unordered_set<GameState, GameStateHash>, GameStateHash, GameStateEqual> predecessors;

    for (const auto& s : scc) {
        // Only System states perform sys moves
        if (s.player != Player::System) continue;

        // First level: sys move → env states
        auto succ_it = successors_.find(s);
        if (succ_it == successors_.end()) continue;

        for (const auto& e : succ_it->second) {  // env states
            // Second level: env move → sys states
            auto env_succ_it = successors_.find(e);
            if (env_succ_it == successors_.end()) continue;

            for (const auto& s_prime : env_succ_it->second) {  // sys states
                // s → s' is a complete sys+env move
                // Assert: both s and s_prime are System states
                assert(s_prime.player == Player::System);
                predecessors[s_prime].insert(s);
            }
        }
    }

    // DEBUG: Log SCC info
    LOG_DEBUG("classify_scc: processing SCC with ", scc.size(), " states");

    // ========== Step 1: Initialize seed set (swin_states) ==========
    // swin_states only contains System states (about to perform sys move)
    std::unordered_set<GameState, GameStateHash, GameStateEqual> swin_states;
    size_t accepting_seed_count = 0;

    // Current SCC: all esa (empty-string accepting) states
    for (const auto& s : scc) {
        // Skip if already classified
        if (classification_.count(s)) {
            if (classification_[s] == StateClass::Swin && s.player == Player::System) {
                swin_states.insert(s);
            }
            continue;
        }

        bool esa = is_empty_string_accepting(s.dfa_state);
        // Debug output (controlled by COSY_DEBUG_CLASSIFY environment variable)
        static const bool debug_classify = (std::getenv("COSY_DEBUG_CLASSIFY") != nullptr);
        if (debug_classify) {
            std::cerr << "DEBUG classify_scc: state=" << s.to_string()
                      << ", phi=" << (s.dfa_state ? s.dfa_state->phi()->to_string() : "null")
                      << ", esa=" << esa << std::endl;
        }
        if (esa) {
            // Only System states in swin_states
            if (s.player == Player::System) {
                swin_states.insert(s);
                classification_[s] = StateClass::Swin;
                accepting_seed_count++;
                LOG_DEBUG("  Seed Swin: ", s.to_string(), " (empty-string accepting)");
            } else {
                // Environment states are classified but not added to swin_states
                classification_[s] = StateClass::Swin;
            }
        }
    }

    // Add predecessors map keys that are already marked as Swin
    for (const auto& pair : predecessors) {
        const GameState& key = pair.first;
        assert(key.player == Player::System);  // Key must be System state
        auto cls_it = classification_.find(key);
        if (cls_it != classification_.end() && cls_it->second == StateClass::Swin) {
            swin_states.insert(key);
        }
    }

    LOG_DEBUG("  Initialized ", swin_states.size(), " Swin seeds (", accepting_seed_count, " empty-string accepting)");

    // ========== Step 2: Fixed-point iteration ==========
    bool changed = true;
    while (changed) {
        changed = false;
        std::unordered_set<GameState, GameStateHash, GameStateEqual> new_swin_states;

        // 2a. Find all predecessors of current swin_states → tmpSet
        std::unordered_set<GameState, GameStateHash, GameStateEqual> tmpSet;
        for (const GameState& swin : swin_states) {
            assert(swin.player == Player::System);  // swin_states only contains System states

            auto pred_it = predecessors.find(swin);
            if (pred_it == predecessors.end()) continue;

            for (const GameState& pred : pred_it->second) {
                assert(pred.player == Player::System);  // Predecessors are System states
                if (!classification_.count(pred)) {
                    tmpSet.insert(pred);
                }
            }
        }

        // 2b. Classify states in tmpSet
        for (const GameState& s : tmpSet) {
            assert(s.player == Player::System);  // tmpSet only contains System states
            if (classification_.count(s)) continue;

            auto succ_it = successors_.find(s);
            if (succ_it == successors_.end()) continue;

            // Check if EXISTS a sys move such that ALL subsequent env moves lead to Swin
            bool has_safe_sys_move = false;
            bool all_sys_moves_ewin = true;
            for (const auto& e : succ_it->second) {  // sys move → env states
                // For this env state, check if ALL env moves lead to Swin
                bool all_env_moves_swin = true;
                auto env_succ_it = successors_.find(e);
                assert(env_succ_it != successors_.end());

                for (const auto& s_prime : env_succ_it->second) {  // env move → sys states
                    auto cls_it = classification_.find(s_prime);
                    if(cls_it == classification_.end()) {
                        // This sys move leads to an unclassified state
                        all_env_moves_swin = false;
                        break;
                    }
                    if (cls_it->second == StateClass::Swin) {
                        // This env move leads to a Swin state
                        continue;
                    }
                    if (cls_it->second == StateClass::Ewin) {
                        classification_[e] = StateClass::Ewin;
                        LOG_DEBUG("  [has_env_moves_ewin] Classified env state as Ewin: ", e.to_string());
                    }
                    all_env_moves_swin = true;
                    break;
                }

                // If this sys move has all env moves leading to Swin, it's safe
                if (all_env_moves_swin) {
                    classification_[e] = StateClass::Swin;
                    LOG_DEBUG("  [all_env_moves_swin] Classified env state as Swin: ", e.to_string());
                    classification_[s] = StateClass::Swin;
                    LOG_DEBUG("  [all_env_moves_swin] Classified sys state as Swin: ", s.to_string());
                    has_safe_sys_move = true;
                    // break; // NOTE: comment this to detect all safe sys moves
                }

                if (all_sys_moves_ewin && classification_.count(e) &&
                    classification_[e] != StateClass::Ewin) {
                    all_sys_moves_ewin = false;
                }
            }

            if (all_sys_moves_ewin) {
                classification_[s] = StateClass::Ewin;
                LOG_DEBUG("  [all_sys_moves_ewin] Classified sys state as Ewin: ", s.to_string());
            }
            if (has_safe_sys_move) {
                classification_[s] = StateClass::Swin;
                new_swin_states.insert(s);
                changed = true;
                LOG_DEBUG("  [has_safe_sys_move] Classified sys state as Swin: ", s.to_string());
            }
        }

        swin_states.insert(new_swin_states.begin(), new_swin_states.end());
    }

    // ========== Step 3: Mark remaining states as Ewin ==========
    for (const auto& s : scc) {
        if (!classification_.count(s)) {
            classification_[s] = StateClass::Ewin;
            if (s.player == Player::System) {
                LOG_DEBUG("  Classified sys state as Ewin: ", s.to_string());
            } else {
                LOG_DEBUG("  Classified env state as Ewin: ", s.to_string());
            }
        }
    }

    // All states in SCC are now classified
    return true;
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
// Trace Exporter Integration
//==============================================================================

void OnTheFlyGameSolver::enable_trace(const std::string& output_dir) {
    if (trace_exporter_) {
        LOG_WARN("Trace already enabled, ignoring");
        return;
    }

    std::string trace_dir = output_dir;
    if (trace_dir.empty()) {
        trace_dir = get_trace_output_path();
    }

    trace_exporter_ = std::make_unique<TraceExporter>(
        original_formula_, pool_, trace_dir);

    LOG_INFO("Trace enabled, output: {}", trace_exporter_->output_path());
}

bool OnTheFlyGameSolver::is_trace_enabled() const {
    return trace_exporter_ != nullptr && trace_exporter_->is_enabled();
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
