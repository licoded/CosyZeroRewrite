#include "formula/formula_checker.hpp"

#include <algorithm>
#include <stdexcept>

namespace formula {

// =============================================================================
// Equivalence Checking
// =============================================================================

bool FormulaChecker::are_equivalent(FormulaPool &pool, Formula *f1, Formula *f2)
{
    // Collect all variables from both formulas
    std::unordered_set<int> vars = get_variables(f1);
    std::unordered_set<int> vars2 = get_variables(f2);
    vars.insert(vars2.begin(), vars2.end());

    // For truth table enumeration, limit to 4 variables
    if (vars.size() > 4)
    {
        // Fallback to sampling with warning
        return likely_equivalent(pool, f1, f2, 1000);
    }

    // Convert to sorted list for consistent enumeration
    std::vector<int> var_list(vars.begin(), vars.end());
    std::sort(var_list.begin(), var_list.end());

    // Generate all possible assignments
    std::vector<Assignment> assignments;
    Assignment current;
    generate_assignments(vars, 0, var_list, current, assignments);

    // Check each assignment
    for (const auto &assign : assignments)
    {
        bool val1 = evaluate(f1, assign);
        bool val2 = evaluate(f2, assign);
        if (val1 != val2)
        {
            return false; // Found counterexample
        }
    }

    return true; // All assignments agree
}

bool FormulaChecker::likely_equivalent(FormulaPool &pool, Formula *f1, Formula *f2, size_t samples)
{
    (void)pool; // Reserved for future variable lookups
    // Collect all variables from both formulas
    std::unordered_set<int> vars = get_variables(f1);
    std::unordered_set<int> vars2 = get_variables(f2);
    vars.insert(vars2.begin(), vars2.end());

    if (vars.empty())
    {
        // No variables - just evaluate both formulas
        return evaluate(f1, Assignment {}) == evaluate(f2, Assignment {});
    }

    // Convert to vector for random sampling
    std::vector<int> var_list(vars.begin(), vars.end());

    // Random sampling
    for (size_t i = 0; i < samples; ++i)
    {
        Assignment assign;

        // Random assignment: each variable true/false with 50% probability
        for (int var : var_list)
        {
            if (rand() % 2 == 1)
            {
                assign.true_vars.insert(var);
            }
        }

        bool val1 = evaluate(f1, assign);
        bool val2 = evaluate(f2, assign);
        if (val1 != val2)
        {
            return false; // Found counterexample
        }
    }

    return true; // All samples agree
}

// =============================================================================
// Property Checking
// =============================================================================

bool FormulaChecker::is_nnf(Formula *f)
{
    if (!f)
        return true;
    return check_nnf_recursive(f);
}

bool FormulaChecker::check_nnf_recursive(Formula *f)
{
    if (!f)
        return true;

    switch (f->op())
    {
        case Formula::OpType::True:
        case Formula::OpType::False:
        case Formula::OpType::Literal:
            return true; // Base cases: in NNF

        case Formula::OpType::Not:
            // Negation is only allowed in front of literals
            return f->left()->is_literal() || f->left()->is_end();

        case Formula::OpType::And:
        case Formula::OpType::Or:
        case Formula::OpType::Until:
        case Formula::OpType::Release:
            // Binary operators: recurse on children
            return check_nnf_recursive(f->left()) && check_nnf_recursive(f->right());

        case Formula::OpType::Next:
            // Next: recurse on child
            return check_nnf_recursive(f->left());

        case Formula::OpType::End:
            return true; // End marker is like a literal
    }

    return false;
}

bool FormulaChecker::is_xnf(Formula *f)
{
    if (!f)
        return true;

    std::unordered_set<Formula *> visited;
    return check_xnf_recursive(f, visited, false);
}

bool FormulaChecker::check_xnf_recursive(Formula *f, std::unordered_set<Formula *> &visited, bool inside_next)
{
    if (!f)
        return true;
    if (visited.count(f))
        return true;
    visited.insert(f);

    switch (f->op())
    {
        case Formula::OpType::True:
        case Formula::OpType::False:
        case Formula::OpType::Literal:
        case Formula::OpType::Not:
        case Formula::OpType::End:
            return true; // Literals and negations are OK

        case Formula::OpType::Next:
        {
            // X is OK, check child with inside_next=true
            // This allows U/R to appear as direct children of X
            Formula *child = f->left();
            return check_xnf_recursive(child, visited, true);
        }

        case Formula::OpType::And:
        case Formula::OpType::Or:
            // Boolean operators: recurse (not inside Next anymore)
            return check_xnf_recursive(f->left(), visited, false) && check_xnf_recursive(f->right(), visited, false);

        case Formula::OpType::Until:
        case Formula::OpType::Release:
            // U/R are only allowed as direct children of Next (inside_next=true)
            // They are NOT allowed in primitive subformulas (inside_next=false)
            return inside_next;
    }

    return false;
}

bool FormulaChecker::is_literal(Formula *f)
{
    if (!f)
        return false;

    if (f->is_literal())
    {
        return true; // Positive literal
    }

    if (f->is_not() && f->left()->is_literal())
    {
        return true; // Negated literal
    }

    return false;
}

bool FormulaChecker::is_temporal(Formula *f)
{
    if (!f)
        return false;

    switch (f->op())
    {
        case Formula::OpType::Next:
        case Formula::OpType::Until:
        case Formula::OpType::Release:
            return true;

        case Formula::OpType::And:
        case Formula::OpType::Or:
        case Formula::OpType::Not:
            return is_temporal(f->left()) || (f->right() && is_temporal(f->right()));

        default:
            return false;
    }
}

// =============================================================================
// Formula Analysis
// =============================================================================

size_t FormulaChecker::formula_size(Formula *f)
{
    if (!f)
        return 0;

    size_t size = 1; // Count this node
    if (f->left())
    {
        size += formula_size(f->left());
    }
    if (f->right())
    {
        size += formula_size(f->right());
    }
    return size;
}

size_t FormulaChecker::formula_depth(Formula *f)
{
    return compute_depth(f);
}

std::unordered_set<int> FormulaChecker::get_variables(Formula *f)
{
    return Formula::collect_variables(f);
}

std::vector<Formula *> FormulaChecker::get_literals(Formula *f)
{
    std::vector<Formula *> literals;
    collect_literals(f, literals);
    return literals;
}

std::unordered_set<Formula *> FormulaChecker::get_primitive_subformulas(Formula *f)
{
    std::unordered_set<Formula *> primitives;
    collect_primitives(f, primitives);
    return primitives;
}

// =============================================================================
// Internal Helpers
// =============================================================================

bool FormulaChecker::evaluate(Formula *f, const Assignment &assignment)
{
    if (!f)
        return false;

    switch (f->op())
    {
        case Formula::OpType::True:
            return true;

        case Formula::OpType::False:
            return false;

        case Formula::OpType::Literal:
            return assignment.true_vars.count(f->var_id()) > 0;

        case Formula::OpType::Not:
            return !evaluate(f->left(), assignment);

        case Formula::OpType::And:
            return evaluate(f->left(), assignment) && evaluate(f->right(), assignment);

        case Formula::OpType::Or:
            return evaluate(f->left(), assignment) || evaluate(f->right(), assignment);

        case Formula::OpType::Next:
            // For truth table checking, X acts on the same assignment
            // (finite trace semantics for single position)
            return evaluate(f->left(), assignment);

        case Formula::OpType::Until:
        {
            // f U g means: eventually g, with f holding until then
            // For truth table: evaluate as g | (f & X(f U g))
            // For single position (no next state): X(f U g) is always false
            // So the formula reduces to just g (whether g is true NOW)
            bool right_val = evaluate(f->right(), assignment);
            return right_val;
        }

        case Formula::OpType::Release:
        {
            // f R g means: g always holds, until f becomes true
            // For truth table: g & (f | X(f R g))
            // Simplified for single position: if g is true, return true; otherwise g & f
            // Special case: true R g → g (but here we check right first)
            bool right_val = evaluate(f->right(), assignment);
            if (right_val)
            {
                // If right (g) is true, the formula requires g to always hold
                // which is satisfied since g=true
                return true;
            }
            // g is false, so f R g is false
            return false;
        }

        case Formula::OpType::End:
            // End marker: false for truth table evaluation
            // (represents end of trace, not a proposition)
            return false;
    }

    return false;
}

void FormulaChecker::generate_assignments(const std::unordered_set<int> &vars,
                                          size_t index,
                                          std::vector<int> &var_list,
                                          Assignment &current,
                                          std::vector<Assignment> &result)
{
    if (index >= var_list.size())
    {
        result.push_back(current);
        return;
    }

    int var = var_list[index];

    // Branch 1: variable is false (not in true_vars)
    generate_assignments(vars, index + 1, var_list, current, result);

    // Branch 2: variable is true (in true_vars)
    current.true_vars.insert(var);
    generate_assignments(vars, index + 1, var_list, current, result);
    current.true_vars.erase(var);
}

void FormulaChecker::collect_literals(Formula *f, std::vector<Formula *> &literals)
{
    if (!f)
        return;

    if (f->is_literal())
    {
        literals.push_back(f);
        return;
    }

    if (f->is_not())
    {
        if (f->left()->is_literal())
        {
            literals.push_back(f);
        }
        else
        {
            collect_literals(f->left(), literals);
        }
        return;
    }

    if (f->left())
    {
        collect_literals(f->left(), literals);
    }
    if (f->right())
    {
        collect_literals(f->right(), literals);
    }
}

void FormulaChecker::collect_primitives(Formula *f, std::unordered_set<Formula *> &primitives)
{
    if (!f)
        return;

    // A formula is a primitive if it's a literal or temporal
    if (f->is_literal() || f->is_temporal())
    {
        primitives.insert(f);
        return;
    }

    if (f->is_not())
    {
        collect_primitives(f->left(), primitives);
        return;
    }

    if (f->left())
    {
        collect_primitives(f->left(), primitives);
    }
    if (f->right())
    {
        collect_primitives(f->right(), primitives);
    }
}

size_t FormulaChecker::compute_depth(Formula *f)
{
    if (!f)
        return 0;

    size_t left_depth = f->left() ? compute_depth(f->left()) : 0;
    size_t right_depth = f->right() ? compute_depth(f->right()) : 0;

    return 1 + std::max(left_depth, right_depth);
}

} // namespace formula
