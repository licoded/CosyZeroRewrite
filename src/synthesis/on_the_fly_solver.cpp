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
    std::vector<int> relevant_output_var_ids;
    std::vector<int> relevant_input_var_ids;  // Local indices (0-based for inputs)

    // Helper function to recursively extract all literals from a formula
    // This is needed because prop_atoms may contain non-literal formulas (Next, Until, Release)
    // which have literals as subformulas
    std::function<void(formula::Formula*)> extract_literals = [&](formula::Formula* f) {
        if (!f) return;
        if (f->is_literal()) {
            int var_id = f->var_id();
            if (pool_.is_output_variable(var_id)) {
                // Avoid duplicates
                if (std::find(relevant_output_var_ids.begin(), relevant_output_var_ids.end(), var_id)
                    == relevant_output_var_ids.end()) {
                    relevant_output_var_ids.push_back(var_id);
                }
            } else if (pool_.is_input_variable(var_id)) {
                // Convert global input ID to local index (0-based for input_gen)
                int local_idx = var_id - pool_.num_outputs();
                if (std::find(relevant_input_var_ids.begin(), relevant_input_var_ids.end(), local_idx)
                    == relevant_input_var_ids.end()) {
                    relevant_input_var_ids.push_back(local_idx);
                }
            }
        } else {
            // Recurse on children for non-literal formulas
            extract_literals(f->left());
            extract_literals(f->right());
        }
    };

    for (formula::Formula* f : state.dfa_state->prop_atoms()) {
        extract_literals(f);
    }

    if (state.player == Player::System) {
        // System's turn: choose output assignment
        // Only enumerate assignments for variables actually in prop_atoms
        auto outputs = relevant_output_var_ids.empty()
            ? output_gen_.all_assignments()
            : output_gen_.all_assignments_for_subset(relevant_output_var_ids);

        for (const auto& out : outputs) {
            succs.push_back(environment_state(state.dfa_state, out));
        }

        LOG_DEBUG("OnTheFlyGameSolver: system state -> ", succs.size(), " environment states");
    } else {
        // Environment's turn: choose input assignment
        // Only enumerate assignments for variables actually in prop_atoms
        auto inputs = relevant_input_var_ids.empty()
            ? input_gen_.all_assignments()
            : input_gen_.all_assignments_for_subset(relevant_input_var_ids);

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

    // DEBUG: Log SCC info
    LOG_DEBUG("classify_scc: processing SCC with ", scc.size(), " states");

    // Step 1: Initialize seed set (accepting states are Swin)
    std::unordered_set<GameState, GameStateHash, GameStateEqual> swin_states;
    size_t accepting_seed_count = 0;
    for (const auto& s : scc) {
        // Skip if already classified
        if (classification_.count(s)) {
            if (classification_[s] == StateClass::Swin) {
                swin_states.insert(s);
            }
            continue;
        }

        // Empty-string accepting states are Swin seeds
        // (no U/X obligations, no contradictions)
        bool esa = is_empty_string_accepting(s.dfa_state);
        // Debug output (controlled by COSY_DEBUG_CLASSIFY environment variable)
        static const bool debug_classify = (std::getenv("COSY_DEBUG_CLASSIFY") != nullptr);
        if (debug_classify) {
            std::cerr << "DEBUG classify_scc: state=" << s.to_string()
                      << ", phi=" << (s.dfa_state ? s.dfa_state->phi()->to_string() : "null")
                      << ", esa=" << esa << std::endl;
        }
        if (esa) {
            swin_states.insert(s);
            classification_[s] = StateClass::Swin;
            accepting_seed_count++;
            LOG_DEBUG("  Seed Swin: ", s.to_string(), " (empty-string accepting)");
        }
    }
    LOG_DEBUG("  Initialized ", swin_states.size(), " Swin seeds (", accepting_seed_count, " empty-string accepting)");

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
