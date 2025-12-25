#include "../ass.h"
#include "../io.h"
#include "analyze.h"
#include "compranges.h"
#include "verify.h"

// Subroutine for building AST.
// Process a proof step referencing an assertion (i.e., not a hypothesis).
// Add the corresponding node to the AST. Return true if okay.
static bool addASTnode
    (Assertion const & assertion, std::vector<Proofsize> & stack,
     AST::reference node)
{
    Hypsize const nhyps = assertion.nhyps(), stacksz = stack.size();
    if (!enoughitemonstack(nhyps, stacksz, ""))
        return false;

    ASTnode::size_type const base = stack.size() - nhyps;
    Proofsize const index = stack.empty() ? 0 : stack.back() + 1;
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
AST ast(Proofsteps const & proof)
{
    std::vector<Proofsize> stack;
    // Preallocate for efficiency
    stack.reserve(proof.size());
    AST tree(proof.size());

    for (Proofsize i = 0; i < proof.size(); ++i)
    {
        Proofstep const step = proof[i];
        if (step.ishyp())
            stack.push_back(i);
        else
        if (step.isthm() &&
            addASTnode(proof[i].pass->second, stack, tree[i]))
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
    Proofsize const index = back->back() + 1, level = result[index];
    for (ASTnode::size_type i = 0; i < back->size(); ++i)
    {
        // Index of root of subAST
        Proofsize const subroot = (*back)[i];
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
bool findsubstitutions(SteprangeAST exp, SteprangeAST tmp, Stepranges & subst)
{
    if (exp.empty() || tmp.empty() || exp.size() < tmp.size())
        return false;
// std::cout << "Matching " << Proofsteps(exp.first.first, exp.first.second);
// std::cout << "Against " << Proofsteps(tmp.first.first, tmp.first.second);
    Proofstep exproot = *(exp.first.second-1), tmproot = *(tmp.first.second-1);
    switch(tmproot.type)
    {
    case Proofstep::HYP:
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
    case Proofstep::THM:
//std::cout << "Ctor " << tmproot << std::endl;
        if (exproot != tmproot)
            return false;
        // Match children.
        for (ASTnode::size_type i = 0; i < exp.ASTroot().size(); ++i)
            if (!findsubstitutions(exp.child(i), tmp.child(i), subst))
                return false;
        return true;
    default:
        return false;
    }
}

// Return true if range1 has all the variables in range2
static bool hasallvars(Steprange range1, Steprange range2)
{
    for (Stepiter iter2 = range2.first; iter2 < range2.second; ++iter2)
    {
        if (!iter2->ishyp())
            continue;
        if (std::find(range1.first, range1.second, *iter2) == range1.second)
            return false;
    }
    return true;
}

// Map: proofstep -> is in a tree governed by the step
typedef std::map<Proofstep, bool, std::less<const char *> > Instep;

static void maxabs
(SteprangeAST subexp, Steprange exp, Instep & instep,
    GovernedSteprangesbystep & result)
{
    Proofstep const root = subexp.RPNroot();
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
            // Record the range.
            GovernedStepranges & ranges = result[root];
            if (ranges.empty())
                ranges = GovernedStepranges(compranges);
            Proofsteps range(subexp.first.first,subexp.first.second);
            ranges[subexp.first] = ast(range);
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
GovernedSteprangesbystep maxabs(Steprange range, AST ast)
{
    GovernedSteprangesbystep result;
    Instep instep;

    maxabs(SteprangeAST(range, ast), range, instep, result);

    return result;
}
