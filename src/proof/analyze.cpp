#include "../ass.h"
#include "../io.h"
#include "analyze.h"
#include "compspan.h"
#include "verify.h"

// Subroutine for building AST.
// Process a proof step referencing an assertion (i.e., not a hypothesis).
// Add the corresponding node to the AST. Return true if okay.
static bool addASTnode
    (Assertion const & assertion, std::vector<RPNsize> & stack,
     AST::reference node)
{
    Hypsize const nhyps = assertion.nhyps(), stacksz = stack.size();
    if (!enoughitemonstack(nhyps, stacksz, ""))
        return false;

    ASTnode::size_type const base = stack.size() - nhyps;
    RPNsize const index = stack.empty() ? 0 : stack.back() + 1;
    node.assign(stack.begin() + base, stack.end());
    // Remove hypotheses from stack.
    stack.erase(stack.begin() + base, stack.end());
    // Add conclusion to stack.
    stack.push_back(index);

    return true;
}

// Return the AST.
// Retval[i] = {index of hyp1, index of hyp2, ...}
// Return an empty AST if not okay. Only for uncompressed proofs
AST ast(RPN const & exp)
{
    std::vector<RPNsize> stack;
    // Preallocate for efficiency
    stack.reserve(exp.size());
    AST tree(exp.size());

    for (RPNsize i = 0; i < exp.size(); ++i)
    {
        RPNstep const step = exp[i];
        if (step.ishyp())
            stack.push_back(i);
        else
        if (step.isthm() &&
            addASTnode(exp[i].pass->second, stack, tree[i]))
            continue;
        else
            return AST();
    }

    return stack.size() == 1 ? tree : AST();
}

// Return the indentations of all the subASTs.
static void indentations(ASTiter begin, ASTiter end, Indentations & result)
{
    // Iterator to root of subAST
    ASTiter const back = end - 1;
    if (back->empty())
        return;
    // Index and level of root
    RPNsize const index = back->back() + 1, level = result[index];
    for (ASTnode::size_type i = 0; i < back->size(); ++i)
    {
        // Index of root of subAST
        RPNsize const subroot = (*back)[i];
        // Indent one more level.
        result[subroot] = level + 1;
        // Recurse to the subAST.
        ASTiter const newend = end - (index - subroot);
        ASTiter const newbegin = i ? end - (index - (*back)[i - 1]): begin;
        indentations(newbegin, newend, result);
    }
}
Indentations indentations(AST const & ast)
{
    Indentations result(ast.size());
    if (!ast.empty())
        indentations(ast.begin(), ast.end(), result);
    return result;
}

// Return true if the RPN of an expression matches a template.
bool findsubst(RPNspanAST exp, RPNspanAST tmp, RPNspans & subst)
{
    if (exp.empty() || tmp.empty() || exp.size() < tmp.size())
        return false;
// std::cout << "Matching " << exp.first << "Against " << tmp.first;
    RPNstep exproot = *(exp.first.second-1), tmproot = *(tmp.first.second-1);
    switch(tmproot.type)
    {
    case RPNstep::HYP:
        if (Symbol2::ID const id = tmproot.id())
        {
// std::cout << "Var " << tmproot << " ID = " << id << std::endl;
            // Template hypothesis is floating. Check if it has been seen.
            if (subst[id].second > subst[id].first) // seen
                return exp.first == subst[id];
            subst[id] = exp.first;// unseen
            return true;
        }
        else
            return false;
    case RPNstep::THM:
//std::cout << "Ctor " << tmproot << std::endl;
        if (exproot != tmproot)
            return false;
        // Match children.
        for (ASTnode::size_type i = 0; i < exp.ASTroot().size(); ++i)
            if (!findsubst(exp.child(i), tmp.child(i), subst))
                return false;
        return true;
    default:
        return false;
    }
}

// Return true if span1 has all the variables in span2
static bool hasallvars(RPNspan span1, RPNspan span2)
{
    for (RPNiter iter2 = span2.first; iter2 < span2.second; ++iter2)
    {
        if (!iter2->ishyp())
            continue;
        if (std::find(span1.first, span1.second, *iter2) == span1.second)
            return false;
    }
    return true;
}

static Hypsize number(RPNstep x)
{ return x.ishyp() ? 0-x.var().id : x.isthm() ? x.pass->second.number : 0; }
static bool compstepstable(RPNstep x, RPNstep y)
{ return number(x) < number(y); }
static bool compspanstable(RPNspan x, RPNspan y)
{
    return std::lexicographical_compare
    (x.first, x.second, y.first, y.second, compstepstable);
}

// Map: RPNstep -> is in a tree governed by the step
typedef std::map<RPNstep, bool, std::less<const char *> > Instep;

static void maxabs
(RPNspanAST subexp, RPNspan exp, Instep & instep,
    GovernedRPNspansbystep & result)
{
    RPNstep const root = subexp.RPNroot();
    if (!root.isthm())
        return;
    // Root is THM.
    bool * pinstep = NULL;
    if (root != exp.root())
    {
        // Not the same root as full expression
        pinstep = &instep[root];
        if (!*pinstep && hasallvars(subexp.first, exp))
        {
            // Record the spans.
            GovernedRPNspans & spans = result[root];
            if (spans.empty())
                spans = GovernedRPNspans(compspanstable);
            spans[subexp.first] = ast(subexp.first);
        }
        *pinstep = true;
    }
    // Recurse to all children.
    for (ASTnode::size_type i = 0; i < subexp.ASTroot().size(); ++i)
        maxabs(subexp.child(i), exp, instep, result);
    if (pinstep)
        *pinstep = false;
}

// Find all maximal abstractions governed by a syntax axiom.
GovernedRPNspansbystep maxabs(RPNspan exp, AST ast)
{
    GovernedRPNspansbystep result;
    Instep instep;

    maxabs(RPNspanAST(exp, ast), exp, instep, result);

    return result;
}
