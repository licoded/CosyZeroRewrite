/**
 * @file tableau.cpp
 * @brief Implementation of on-the-fly tableau construction
 */

#include "automata/tableau.hpp"
#include "log/logger.hpp"
#include <algorithm>
#include <functional>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>

namespace automata {

// Debug log file (shared across all calls)
namespace {
    std::ofstream g_debug_log;
    bool g_debug_log_initialized = false;

    void init_debug_log() {
        if (!g_debug_log_initialized) {
            // Create log path: logs/tableau/YYYY-MM-DD/period/tableau_debug.log
            namespace fs = std::filesystem;
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            struct tm* tm_info = std::localtime(&time_t);
            int hour = tm_info->tm_hour;

            // Determine period: 01-morning(6-12), 02-afternoon(12-18), 03-evening(18-24), 04-night(0-6)
            std::string period;
            if (hour >= 6 && hour < 12) period = "01-morning";
            else if (hour >= 12 && hour < 18) period = "02-afternoon";
            else if (hour >= 18) period = "03-evening";
            else period = "04-night";

            std::ostringstream log_path;
            log_path << "logs/tableau/"
                      << std::put_time(tm_info, "%Y-%m-%d")
                      << "/" << period
                      << "/tableau_debug_"
                      << std::put_time(tm_info, "%Y%m%d_%H%M%S")
                      << ".log";

            // Create directory
            fs::create_directories(fs::path(log_path.str()).parent_path());

            g_debug_log.open(log_path.str(), std::ios::out | std::ios::trunc);
            g_debug_log_initialized = true;
        }
    }

    void debug_log(const std::string& msg) {
        init_debug_log();
        std::cerr << msg;
        g_debug_log << msg;
        g_debug_log.flush();
    }
}

//==============================================================================
// FormulaEqual implementation
//==============================================================================

bool FormulaEqual::operator()(formula::Formula* a, formula::Formula* b) const noexcept {
    if (a == b) return true;
    if (!a || !b) return false;

    // Fast path: different hash means different formulas
    if (a->hash() != b->hash()) return false;

    // Structural comparison
    if (a->op() != b->op()) return false;
    if (a->var_id() != b->var_id()) return false;

    // Compare children
    formula::Formula* a_left = a->left();
    formula::Formula* a_right = a->right();
    formula::Formula* b_left = b->left();
    formula::Formula* b_right = b->right();

    if (a_left == b_left && a_right == b_right) return true;
    if (!a_left || !b_left || !a_right || !b_right) return false;

    return (*this)(a_left, b_left) && (*this)(a_right, b_right);
}

//==============================================================================
// TableauState
//==============================================================================

//------------------------------------------------------------------------------
// New TableauState Implementation (based on AAAI2019)
//------------------------------------------------------------------------------

// Compute PA(φ) - Propositional Atoms per AAAI2019 Definition 2
// PA(φ) = {φ} if φ is atom, Next, Until, or Release
// PA(¬ψ) = PA(ψ)
// PA(φ₁ ∧ φ₂) = PA(φ₁) ∪ PA(φ₂)
// PA(φ₁ ∨ φ₂) = PA(φ₁) ∪ PA(φ₂)
void TableauState::compute_prop_atoms(formula::Formula* phi, FormulaSet& result) {
    if (!phi) return;

    auto op = phi->op();

    // Base cases: atom, Next, Until, or Release - add as-is
    if (op == formula::Formula::OpType::Literal ||
        op == formula::Formula::OpType::True ||
        op == formula::Formula::OpType::False ||
        op == formula::Formula::OpType::Next ||
        op == formula::Formula::OpType::Until ||
        op == formula::Formula::OpType::Release) {
        result.insert(phi);
        return;
    }

    // Not: recurse on child
    if (op == formula::Formula::OpType::Not) {
        compute_prop_atoms(phi->left(), result);
        return;
    }

    // And/Or: union of children's PA
    if (op == formula::Formula::OpType::And ||
        op == formula::Formula::OpType::Or) {
        compute_prop_atoms(phi->left(), result);
        compute_prop_atoms(phi->right(), result);
        return;
    }
}

// New constructor taking phi, xnf_phi, and prop_atoms
TableauState::TableauState(formula::Formula* phi, formula::Formula* xnf_phi, FormulaSet prop_atoms)
    : phi_(phi), xnf_phi_(xnf_phi), prop_atoms_(std::move(prop_atoms)),
      hash_(phi ? phi->hash() : 0) {}

// Create initial tableau state
std::unique_ptr<TableauState> TableauState::initial(formula::Formula* phi, formula::FormulaPool& pool) {
    // Convert to NNF first
    formula::Formula* nnf_phi = phi->nnf(pool);

    // Convert to XNF for proper state construction
    formula::Formula* xnf_phi = nnf_phi->xnf_with_tail(pool);

    // Compute PA(xnf_phi) - Propositional Atoms
    FormulaSet prop_atoms;
    compute_prop_atoms(xnf_phi, prop_atoms);

    return std::unique_ptr<TableauState>(new TableauState(phi, xnf_phi, std::move(prop_atoms)));
}

//------------------------------------------------------------------------------
// Formula Progression fp(φ, σ) per AAAI2019
// Simplified version (ignoring ♢true and □false):
// - fp(tt, σ) = tt, fp(ff, σ) = ff
// - fp(p, σ) = tt if p∈σ, else ff
// - fp(¬p, σ) = tt if p∉σ, else ff
// - fp(φ₁ ∧ φ₂, σ) = fp(φ₁, σ) ∧ fp(φ₂, σ)
// - fp(φ₁ ∨ φ₂, σ) = fp(φ₁, σ) ∨ fp(φ₂, σ)
// - fp(Xφ, σ) = φ
// - fp(WXφ, σ) = φ
// - fp(φ₁ U φ₂, σ) = fp(φ₂, σ) ∨ (fp(φ₁, σ) ∧ fp(X(φ₁ U φ₂), σ))
// - fp(φ₁ R φ₂, σ) = fp(φ₂, σ) ∧ (fp(φ₁, σ) ∨ fp(WX(φ₁ R φ₂), σ))
//------------------------------------------------------------------------------

namespace {

/**
 * @brief Formula progression fp(φ, σ) with immediate simplification
 *
 * Based on AAAI2019 Li et al. with simplified handling of ♢true/□false.
 * Includes immediate simplification to prevent formula explosion:
 * - (true & false) → false, (false | anything) → anything, etc.
 *
 * @param phi Formula to progress (must be in XNF)
 * @param sigma Assignment σ (set of true variables)
 * @param pool Formula pool for creating new formulas
 * @return Progressed formula (fp(φ, σ))
 */
formula::Formula* formula_progression(
    formula::Formula* phi,
    const Assignment& sigma,
    formula::FormulaPool& pool)
{
    if (!phi) return pool.create_false();

    auto op = phi->op();

    switch (op) {
        case formula::Formula::OpType::True:
            return phi;

        case formula::Formula::OpType::False:
            return phi;

        case formula::Formula::OpType::Literal:
            // fp(p, σ) = tt if p ∈ σ, else ff
            return sigma.count(phi->var_id()) > 0
                ? pool.create_true()
                : pool.create_false();

        case formula::Formula::OpType::Not: {
            formula::Formula* child = phi->left();
            if (!child) return pool.create_true();

            // fp(¬p, σ) = tt if p ∉ σ, else ff
            if (child->op() == formula::Formula::OpType::Literal) {
                return sigma.count(child->var_id()) == 0
                    ? pool.create_true()
                    : pool.create_false();
            }

            // For complex ¬φ, compute ¬fp(φ, σ)
            formula::Formula* child_prog = formula_progression(child, sigma, pool);

            // Immediate simplification: !!φ → φ
            if (child_prog->op() == formula::Formula::OpType::Not) {
                return child_prog->left();
            }
            // Immediate simplification: !true → false, !false → true
            if (child_prog->is_true()) return pool.create_false();
            if (child_prog->is_false()) return pool.create_true();

            return pool.create_not(child_prog);
        }

        case formula::Formula::OpType::And: {
            // fp(φ₁ ∧ φ₂, σ) = fp(φ₁, σ) ∧ fp(φ₂, σ)
            formula::Formula* left_prog = formula_progression(phi->left(), sigma, pool);
            formula::Formula* right_prog = formula_progression(phi->right(), sigma, pool);

            // Immediate simplification for And
            // (false & anything) → false, (anything & false) → false
            if (left_prog->is_false() || right_prog->is_false()) return pool.create_false();
            // (true & true) → true
            if (left_prog->is_true() && right_prog->is_true()) return pool.create_true();
            // (true & φ) → φ, (φ & true) → φ
            if (left_prog->is_true()) return right_prog;
            if (right_prog->is_true()) return left_prog;

            return pool.create_and(left_prog, right_prog);
        }

        case formula::Formula::OpType::Or: {
            // fp(φ₁ ∨ φ₂, σ) = fp(φ₁, σ) ∨ fp(φ₂, σ)
            formula::Formula* left_prog = formula_progression(phi->left(), sigma, pool);
            formula::Formula* right_prog = formula_progression(phi->right(), sigma, pool);

            // Immediate simplification for Or
            // (true | anything) → true, (anything | true) → true
            if (left_prog->is_true() || right_prog->is_true()) return pool.create_true();
            // (false | false) → false
            if (left_prog->is_false() && right_prog->is_false()) return pool.create_false();
            // (false | φ) → φ, (φ | false) → φ
            if (left_prog->is_false()) return right_prog;
            if (right_prog->is_false()) return left_prog;

            return pool.create_or(left_prog, right_prog);
        }

        case formula::Formula::OpType::Next: {
            // fp(Xφ, σ) = φ (ignoring ∧ ♢true)
            return phi->left();
        }

        // Note: Until/Release are not in XNF form, so shouldn't appear here
        // They are transformed during XNF conversion

        default:
            // Unknown operator, conservatively return false
            return pool.create_false();
    }
}

} // anonymous namespace

// Compute next phi using formula progression
// Returns the progressed formula, which should be used with
// TableauStatePool::get_or_create() to obtain the actual TableauState.
formula::Formula* TableauState::next_phi(const Assignment& assignment,
                                          formula::FormulaPool& pool) const {
    // DEBUG: Log input
    debug_log("DEBUG next_phi:\n");
    debug_log("  xnf_phi_ = " + (xnf_phi_ ? xnf_phi_->to_string() : "null") + "\n");
    debug_log("  assignment = {");
    for (int v : assignment) debug_log(std::to_string(v) + " ");
    debug_log("}\n");

    // Apply formula progression: next_phi = fp(xnf_phi_, assignment)
    formula::Formula* next_phi = formula_progression(xnf_phi_, assignment, pool);

    // DEBUG: Log output
    debug_log("  next_phi = " + (next_phi ? next_phi->to_string() : "null") + "\n");

    // TODO: Simplify the result (currently disabled)
    // formula::Formula* next_phi_simplified = next_phi->simplify(pool);

    return next_phi;
}

//------------------------------------------------------------------------------
// Remaining TableauState methods
//------------------------------------------------------------------------------

bool TableauState::operator==(const TableauState& other) const {
    // Equality based on phi only
    if (this == &other) return true;
    if (!phi_ || !other.phi_) return false;
    return phi_->hash() == other.phi_->hash();
}

std::string TableauState::to_string() const {
    std::ostringstream oss;
    oss << "{phi: " << (phi_ ? phi_->to_string() : "null");
    oss << ", atoms: [";

    bool first = true;
    for (formula::Formula* f : prop_atoms_) {
        if (!first) oss << ", ";
        first = false;

        if (f) {
            oss << f->to_string();
        } else {
            oss << "null";
        }
    }

    oss << "]}";
    return oss.str();
}

bool TableauState::is_temporal(formula::Formula* f) {
    if (!f) return false;
    auto op = f->op();
    return op == formula::Formula::OpType::Next ||
           op == formula::Formula::OpType::Until ||
           op == formula::Formula::OpType::Release;
}

//==============================================================================
// TableauStatePool
//==============================================================================

// Updated get_or_create to take phi instead of formulas
TableauState* TableauStatePool::get_or_create(formula::Formula* phi, formula::FormulaPool& pool) {
    // Check if phi is null
    if (!phi) return nullptr;

    // DEBUG: Log input
    debug_log("DEBUG get_or_create: phi = " + phi->to_string() + "\n");

    // Create a temporary state to check for existence
    // We need the xnf and prop_atoms, but for checking existence we just need phi hash
    auto temp = std::unique_ptr<TableauState>(new TableauState(phi, phi, {}));

    // Check if equivalent state exists (based on phi hash)
    auto it = states_.find(temp.get());
    if (it != states_.end()) {
        debug_log("  -> found existing state\n");
        return *it;
    }

    // Create the actual state with proper initialization
    // First convert to NNF, then XNF
    formula::Formula* nnf_phi = phi->nnf(pool);
    formula::Formula* xnf_phi = nnf_phi->xnf_with_tail(pool);

    // DEBUG: Log NNF and XNF
    debug_log("  nnf_phi = " + nnf_phi->to_string() + "\n");
    debug_log("  xnf_phi = " + xnf_phi->to_string() + "\n");

    // Compute PA(xnf_phi)
    TableauState::FormulaSet prop_atoms;
    TableauState::compute_prop_atoms(xnf_phi, prop_atoms);

    debug_log("  prop_atoms size = " + std::to_string(prop_atoms.size()) + "\n");

    // Create the actual state
    auto actual_state = std::unique_ptr<TableauState>(
        new TableauState(phi, xnf_phi, std::move(prop_atoms)));

    // Add to pool
    TableauState* raw_ptr = actual_state.get();
    states_.insert(raw_ptr);
    storage_.push_back(std::move(actual_state));

    debug_log("  -> created new state, total states = " + std::to_string(states_.size()) + "\n");

    return raw_ptr;
}

void TableauStatePool::clear() {
    states_.clear();
    storage_.clear();
}

//==============================================================================
// OnTheFlyDFA
//==============================================================================

OnTheFlyDFA::OnTheFlyDFA(formula::Formula* phi, formula::FormulaPool& pool)
    : pool_(pool), state_pool_(), initial_state_(nullptr) {
    LOG_DEBUG("OnTheFlyDFA: constructing from formula: ", phi->to_string());

    // Create initial state using TableauState::initial
    auto init_state = TableauState::initial(phi, pool_);
    // Get or create from pool (uses phi for hash consing)
    initial_state_ = state_pool_.get_or_create(init_state->phi(), pool_);

    LOG_DEBUG("OnTheFlyDFA: initial state: ", initial_state_->to_string());
}

TableauState* OnTheFlyDFA::successor(TableauState* q, const Assignment& assignment) const {
    // Check cache
    auto key = std::make_pair(q, assignment);
    auto it = transition_cache_.find(key);
    if (it != transition_cache_.end()) {
        return it->second;
    }

    // Compute next phi using formula progression
    formula::Formula* next_phi = q->next_phi(assignment, pool_);

    // Get or create from pool
    TableauState* result = state_pool_.get_or_create(next_phi, pool_);

    // Cache and track
    transition_cache_[key] = result;
    expanded_states_.insert(q);

    LOG_DEBUG("OnTheFlyDFA: successor = ", result->to_string());

    return result;
}

//==============================================================================
// AssignmentGenerator
//==============================================================================

AssignmentGenerator::AssignmentGenerator(int num_variables)
    : num_variables_(num_variables) {
}

std::vector<Assignment> AssignmentGenerator::all_assignments() const {
    std::vector<Assignment> result;
    int num = 1 << num_variables_;  // 2^n

    result.reserve(num);
    for (int i = 0; i < num; ++i) {
        result.push_back(from_bitset(i, num_variables_));
    }

    return result;
}

Assignment AssignmentGenerator::from_bitset(int i, int num_variables) {
    Assignment result;
    for (int v = 0; v < num_variables; ++v) {
        if (i & (1 << v)) {
            result.insert(v);
        }
    }
    return result;
}

std::string AssignmentGenerator::to_string(const Assignment& a, int num_variables) {
    std::ostringstream oss;
    oss << "{";
    for (int v = 0; v < num_variables; ++v) {
        if (v > 0) oss << ", ";
        oss << "x" << v << "=" << (a.count(v) ? "1" : "0");
    }
    oss << "}";
    return oss.str();
}

} // namespace automata
